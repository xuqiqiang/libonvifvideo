
#include "rtp.h"
#include "mp4v2/mp4v2.h"
#include "localsocket.h"
#include "writemp4file.h"
#include "common.h"

//#define TEST_SOCKET
#define RTP_PACKAGE_COUNT 200

typedef struct _rtp_header_t
{
    uint8_t i_version;
    uint8_t i_extend;
    uint8_t i_m_tag;
    uint8_t i_cc;
    uint8_t i_pt;
    uint32_t i_seq_num;
    uint32_t i_timestamp;
    uint32_t i_ssrc;
    uint32_t i_csrc;

    uint8_t i_nalu_header;
} rtp_header_t;



#ifndef MUTEX
#define MUTEX pthread_mutex_t
#endif

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef SOCKADDR_IN
#define SOCKADDR_IN struct sockaddr_in
#endif

#ifndef SOCKADDR
#define SOCKADDR struct sockaddr
#endif

#include "dlist.h"
#define MAX_RTP_TRAN_SIZE 		1200
#define MAX_VIDEO_FRAME_SIZE		409600

typedef struct _rtp_t
{
    uint32_t rtp_size;
    uint8_t* p_rtp;
} rtp_t;

typedef struct _rtp_s
{
    uint8_t* p_video_frame;
    uint32_t i_video_frame_index;
    uint32_t i_exit;
    pthread_t rtpThread;
    int m_count;
    char p_ip[40];
    int i_port;
    
    WRITEINFO writeInfo;
    SENDRTPDATA sendInfo;
} rtp_s;


void set_writeInfo(unsigned long hand, int time, int day, int size)
{
    rtp_s* p_rtp = (rtp_s *)hand;
    if(p_rtp != NULL)
    {
        p_rtp->writeInfo.m_day = day;
        p_rtp->writeInfo.m_size = size;
        p_rtp->writeInfo.m_time = time;

        return;
    }
    DPRINT(VIDEO_LOG_ERR,"set wirte info error\n");
}

int get_rtp_header(rtp_header_t* p_header, uint8_t* p_buf, uint32_t i_size);

#define PVOID void*
typedef PVOID THREAD(PVOID Arg);
int threadCreate(THREAD* funcThread, void* param);

static void parse_sps(uint8_t * sps, int sps_size, int * width, int * height);

static int free_frame(frame_t** pp_frame)
{
    if ((*pp_frame) != NULL)
    {
        free((*pp_frame));
        (*pp_frame) = NULL;
    }
    return 0;
}

static frame_t* new_frame(uint8_t* p_frame_data, uint32_t i_size)
{
    int i_ret = 0;
    frame_t* p_new = NULL;
    if (p_frame_data == NULL || i_size <= 0)
    {
        i_ret = -1;
    }
    else
    {
        p_new = malloc(i_size + sizeof (frame_t));
        if (p_new == NULL)
        {
            DPRINT(VIDEO_LOG_ERR,"malloc rtp frame error\n");
            i_ret = -1;
        }
        else
        {
            p_new->p_frame = ((uint8_t*) p_new) + sizeof (frame_t);

            p_new->i_frame_size = i_size;
            p_new->p_next = NULL;
            memcpy(p_new->p_frame, p_frame_data, i_size);
        }
    }

    if (i_ret < 0)
    {
        return NULL;
    }
    else
    {
        return p_new;
    }
}

static int clear_frame(rtp_s* p_rtp)
{
    frame_t* p_temp = NULL;
    pthread_mutex_lock(&p_rtp->writeInfo.mutex);

    while (p_rtp->writeInfo.p_frame_header != NULL)
    {
        p_temp = p_rtp->writeInfo.p_frame_header->p_next;
        free_frame(&p_rtp->writeInfo.p_frame_header);
        p_rtp->writeInfo.p_frame_header = p_temp;
    }
    p_rtp->writeInfo.p_frame_buf = NULL;
    p_rtp->writeInfo.p_frame_header = NULL;
    p_rtp->writeInfo.i_buf_num = 0;
    pthread_mutex_unlock(&p_rtp->writeInfo.mutex);
    return VIDEO_SUCCESS;
}


static int add_frame(rtp_s* p_rtp, uint8_t* p_frame, uint32_t i_size)
{
    int i_ret = 0;

    if (p_rtp->writeInfo.i_buf_num >= RTP_PACKAGE_COUNT)
    {
        DPRINT(VIDEO_LOG_DEBUG,"rtp frame buf overlow, notice this count:%d\n",p_rtp->writeInfo.i_buf_num);
    }
    else
    {
        pthread_mutex_lock(&p_rtp->writeInfo.mutex);
        if (p_rtp->writeInfo.p_frame_buf == NULL)
        {
            p_rtp->writeInfo.p_frame_buf = new_frame(p_frame, i_size);
            p_rtp->writeInfo.p_frame_header = p_rtp->writeInfo.p_frame_buf;
        }
        else
        {
            frame_t* p_new = new_frame(p_frame, i_size);
            p_rtp->writeInfo.p_frame_buf->p_next = p_new;
            p_rtp->writeInfo.p_frame_buf = p_new;
        }
        p_rtp->writeInfo.i_buf_num++;
        pthread_cond_signal(&p_rtp->writeInfo.cond);
        //DPRINT(VIDEO_LOG_DEBUG,"==========rtp mac:%s frame buf  count:%d\n",p_rtp->writeInfo.mac,p_rtp->writeInfo.i_buf_num);
        pthread_mutex_unlock(&p_rtp->writeInfo.mutex);
    }

    return i_ret;
}

static int dump_frame(uint8_t* p_frame, uint32_t size)
{
    DPRINT(VIDEO_LOG_DEBUG,"*********************************************************:%u\n", size);
    if(p_frame != NULL && size >0)
    {
        uint32_t i=0;
        for(; i<size; i++)
        {
            DPRINT(VIDEO_LOG_DEBUG,"%x ", p_frame[i]);

            if((i+1)%32 == 0)
            {
                DPRINT(VIDEO_LOG_DEBUG,"\n");
            }
        }
    }
    DPRINT(VIDEO_LOG_DEBUG,"\n");
}

int get_rtp_header(rtp_header_t* p_header, uint8_t* p_buf, uint32_t i_size)
{
    int i_ret = 0;

    if (p_header == NULL || p_buf == NULL || i_size < 0)
    {
        i_ret = -1;
    }
    else
    {
        p_header->i_version = (p_buf[0] & 0xC0) >> 6;
        p_header->i_extend = (p_buf[0] & 0x10) >> 4;
        p_header->i_cc = (p_buf[0] & 0x0F);
        p_header->i_m_tag = (p_buf[1] & 0x80) >> 7;
        p_header->i_pt = (p_buf[1] & 0x7F);
        p_header->i_seq_num = (p_buf[2] << 8);
        p_header->i_seq_num += p_buf[3];
        p_header->i_timestamp = (p_buf[4] << 24);
        p_header->i_timestamp += (p_buf[5] << 16);
        p_header->i_timestamp += (p_buf[6] << 8);
        p_header->i_timestamp += p_buf[7];

        p_header->i_ssrc = (p_buf[8] << 24);
        p_header->i_ssrc += (p_buf[9] << 16);
        p_header->i_ssrc += (p_buf[10] << 8);
        p_header->i_ssrc += p_buf[11];
        i_ret = 12;
        return i_ret;
    }
    return i_ret;
}


typedef struct
{
	/*  0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|V=2|P|X|  CC   |M|     PT      |       sequence number         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           timestamp                           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           synchronization source (SSRC) identifier            |
	+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	|            contributing source (CSRC) identifiers             |
	|                             ....                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	*/
	//intel 的cpu 是intel为小端字节序（低端存到底地址） 而网络流为大端字节序（高端存到低地址）  
	/*intel 的cpu ： 高端->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->低端
	在内存中存储 ：
	低->4001（内存地址）version:2
	4002（内存地址）padding:1
	4003（内存地址）extension:1
	高->4004（内存地址）csrc_len:4

	网络传输解析 ： 高端->version:2->padding:1->extension:1->csrc_len:4->低端  (为正确的文档描述格式)

	存入接收内存 ：
	低->4001（内存地址）version:2
	4002（内存地址）padding:1
	4003（内存地址）extension:1
	高->4004（内存地址）csrc_len:4
	本地内存解析 ：高端->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->低端 ，
	即：
	unsigned char csrc_len:4;        // expect 0
	unsigned char extension:1;       // expect 1
	unsigned char padding:1;         // expect 0
	unsigned char version:2;         // expect 2
	*/
	/* byte 0 */
	unsigned char csrc_len : 4;        /* expect 0 */
	unsigned char extension : 1;       /* expect 1, see RTP_OP below */
	unsigned char padding : 1;         /* expect 0 */
	unsigned char version : 2;         /* expect 2 */
	/* byte 1 */
	unsigned char payloadtype : 7;     /* RTP_PAYLOAD_RTSP */
	unsigned char marker : 1;          /* expect 1 */
	/* bytes 2,3 */
	unsigned short seq_no;
	/* bytes 4-7 */
	unsigned int timestamp;
	/* bytes 8-11 */
	unsigned int ssrc;              /* stream number is used here. */
} RTP_FIXED_HEADER;


/*
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
*/
typedef struct
{
	//byte 0  
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} NALU_HEADER; // 1 BYTE   

/*
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
*/
typedef struct
{
	//byte 0  
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} FU_INDICATOR; // 1 BYTE   

/*
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|S|E|R|  Type   |
+---------------+
*/
typedef struct
{
	//byte 0  
	unsigned char TYPE : 5;
	unsigned char R : 1;
	unsigned char E : 1;
	unsigned char S : 1;
} FU_HEADER;   // 1 BYTES   


int RtpTo264(char *bufIn, int len, char *bufOut, int *retlen, int *flags)
{
    NALU_HEADER * nalu_hdr = NULL;
	FU_INDICATOR    *fu_ind = NULL;
	FU_HEADER       *fu_hdr = NULL;
	int total_bytes = 0;                 //当前包传出的数据   
	int fwrite_number = 0;               //存入文件的数据长度  

	//////////////////////////////////////////////////////////////////////////  
	//begin rtp_payload and rtp_header  

	RTP_FIXED_HEADER * rtp_hdr = NULL;

	rtp_hdr = (RTP_FIXED_HEADER*)&bufIn[0];
	//DPRINT(VIDEO_LOG_DEBUG,"version    : %d\n", rtp_hdr->version);
	//DPRINT(VIDEO_LOG_DEBUG,"payloadtype    :%d\n", rtp_hdr->payloadtype);
	//DPRINT(VIDEO_LOG_DEBUG,"maker bit     : %d\n", rtp_hdr->marker);
	if (rtp_hdr->payloadtype != 96)
	{
		DPRINT(VIDEO_LOG_ERR,"not h264 data\n");
		return -1;
	}

	//DPRINT(VIDEO_LOG_DEBUG,"============================================seq no:%d, time:%d\n", ntohs(rtp_hdr->seq_no),ntohl(rtp_hdr->timestamp));
	//////////////////////////////////////////////////////////////////////////  
	//开始解包  
	nalu_hdr = (NALU_HEADER*)&bufIn[12];                        //网络传输过来的字节序 ，当存入内存还是和文档描述的相反，只要匹配网络字节序和文档描述即可传输正确。
	if (nalu_hdr->TYPE == 0x07 || nalu_hdr->TYPE == 0x08)
	{
		//DPRINT("current sps package\n");
		bufOut[0] = 0x00;
		bufOut[1] = 0x00;
		bufOut[2] = 0x00;
		bufOut[3] = 0x01;
		memcpy(&(bufOut[4]), &bufIn[12], len - 12);

		*retlen = len - 12 + 4;
        if(nalu_hdr->TYPE == 0x07)
        {
            *flags = 4;
        }
        else
        {
            *flags =5;
        }

		return 0;
	}

    //DPRINT("============================================type:%d\n", nalu_hdr->TYPE);
	if (nalu_hdr->TYPE == 0)
	{
		DPRINT(VIDEO_LOG_INFO,"======================================package error\n");
		return -1;
	}
	else if (nalu_hdr->TYPE >0 && nalu_hdr->TYPE < 24)  //单包  
	{
		//DPRINT("current single\n");
		bufOut[0] = 0x00;
		bufOut[1] = 0x00;
		bufOut[2] = 0x00;
		bufOut[3] = 0x01;
		memcpy(&bufOut[4], &bufIn[12], len - 12);
		*retlen = len - 12 + 4;
		*flags = 0;
		return 0;
	}
	else if (nalu_hdr->TYPE == 24)                    //STAP-A   单一时间的组合包  
	{
		DPRINT(VIDEO_LOG_INFO,"current package STAP-A\n");
	}
	else if (nalu_hdr->TYPE == 25)                    //STAP-B   单一时间的组合包  
	{
		DPRINT(VIDEO_LOG_INFO,"current packageSTAP-B\n");
	}
	else if (nalu_hdr->TYPE == 26)                     //MTAP16   多个时间的组合包  
	{
		DPRINT(VIDEO_LOG_INFO,"current packageMTAP16\n");
	}
	else if (nalu_hdr->TYPE == 27)                    //MTAP24   多个时间的组合包  
	{
		DPRINT(VIDEO_LOG_INFO,"current packageMTAP24\n");
	}
	else if (nalu_hdr->TYPE == 28)                    //FU-A分片包，解码顺序和传输顺序相同  
	{
		fu_ind = (FU_INDICATOR*)&bufIn[12];     //分片包用的是FU_INDICATOR而不是NALU_HEADER  
		//DPRINT(VIDEO_LOG_DEBUG,"FU_INDICATOR->F     :%d\n",fu_ind->F);  
		//DPRINT(VIDEO_LOG_DEBUG,"FU_INDICATOR->NRI   :%d\n",fu_ind->NRI);  
		//DPRINT(VIDEO_LOG_DEBUG,"FU_INDICATOR->TYPE  :%d\n",fu_ind->TYPE);  

		fu_hdr = (FU_HEADER*)&bufIn[13];        //FU_HEADER赋值  
		//DPRINT(VIDEO_LOG_DEBUG,"FU_HEADER->S        :%d\n",fu_hdr->S);  
		//DPRINT(VIDEO_LOG_DEBUG,"FU_HEADER->E        :%d\n",fu_hdr->E);  
		//DPRINT(VIDEO_LOG_DEBUG,"FU_HEADER->R        :%d\n",fu_hdr->R);  
		//DPRINT(VIDEO_LOG_DEBUG,"FU_HEADER->TYPE     :%d\n",fu_hdr->TYPE);  

		if (rtp_hdr->marker == 1)                      //分片包最后一个包  
		{
			//DPRINT(VIDEO_LOG_DEBUG,"current package FU-A pic last package\n");
			memcpy(&bufOut[0], &bufIn[14], len - 14);
			*retlen = len - 14;
			*flags = 3;
		}
		else if (rtp_hdr->marker == 0)                 //分片包 但不是最后一个包  
		{
			if (fu_hdr->S == 1)                        //分片的第一个包  
			{
				unsigned char F;
				unsigned char NRI;
				unsigned char TYPE;
				unsigned char nh;
				//DPRINT(VIDEO_LOG_DEBUG,"current package FU-A pic first package\n");
				bufOut[0] = 0x00;
				bufOut[1] = 0x00;
				bufOut[2] = 0x00;
				bufOut[3] = 0x01;


				F = fu_ind->F << 7;
				NRI = fu_ind->NRI << 5;
				TYPE = fu_hdr->TYPE;                                            //应用的是FU_HEADER的TYPE  
				//nh = n->forbidden_bit|n->nal_reference_idc|n->nal_unit_type;  //二进制文件也是按 大字节序存储  
				nh = F | NRI | TYPE;
				bufOut[4] = nh;
				memcpy(&bufOut[5], &bufIn[14], len - 14);
				*retlen = len - 14 + 5;
				*flags = 1;

			}
			else                                      //如果不是第一个包  
			{
				//DPRINT(VIDEO_LOG_DEBUG,"current package FU-A pic package \n");
				memcpy(&bufOut[0], &bufIn[14], len - 14);
				*retlen = len - 14;
				*flags = 2;
			}
		}
	}
	else if (nalu_hdr->TYPE == 29)                //FU-B分片包，解码顺序和传输顺序相同  
	{
		DPRINT(VIDEO_LOG_INFO,"FU-B package\n");
		return -1;
	}
	else
	{
		DPRINT(VIDEO_LOG_INFO,"package error, 30-31 not define\n");
		return -1;
	}

    //DPRINT(VIDEO_LOG_INFO,"============================================flag:%d\n", *flags);
	return 0;
}


typedef struct __uint24 {
	uint8_t b[3];
} uint24, uint24_be, uint24_le;
typedef struct __bit_buffer {
	uint8_t * start;   // buf 开始
	int size;    // buf size
	uint8_t * current; // 当前buf位置
	uint8_t read_bits;// 当前buf的第几个bit
} bit_buffer;
static void skip_bits(bit_buffer * bb, int nbits) {
	bb->current = bb->current + ((nbits + bb->read_bits) / 8);
	bb->read_bits = (uint8_t)((bb->read_bits + nbits) % 8);
}

/**

* 函数static uint8 get_bit(bit_buffer* bb)的功能是读取bb的某个字节的每一位，然后返回，实现很简单，这里略去。

*/
static uint8_t get_bit(bit_buffer* bb)
{ // andy
	uint8_t result = *bb->current;
	result >>= 7 - bb->read_bits;
	result &= 0x01;
	skip_bits(bb, 1);
	return result;
}

/**

* 函数static uint32 get_bits(bit_buffer* bb,size_t nbits)的功能相当于连续读取bb的nbits位，若nbits是8的整数倍，则相当于读取几个字节

*由uint32可知，nbits的取值范围为0~32，实现很简单，这里略去

*/
static uint32_t get_bits(bit_buffer* bb, int nbits)
{

	// 67        42         E0         0A        89 
	// 0110 0111 0100 0010  1110 0000  0000 1010 1000 1001


	uint32_t result = 0;
	uint8_t tmp = 0;
	int i = 0;
	for (i = 0; i<nbits; i++)
	{
		tmp = get_bit(bb);
		result <<= 1;
		result += tmp;
		printf("%d ", tmp);
	}

	return result;

}

static uint32_t exp_golomb_ue(bit_buffer * bb) {
	uint8_t bit, significant_bits;
	significant_bits = 0;
	bit = get_bit(bb);
	while (bit == 0) {
		significant_bits++;
		bit = get_bit(bb);
	}
	return (1 << significant_bits) + get_bits(bb, significant_bits) - 1;
}
static int32_t exp_golomb_se(bit_buffer * bb) {
	int32_t ret;
	ret = exp_golomb_ue(bb);
	if ((ret & 0x1) == 0) {
		return -(ret >> 1);
	}
	else {
		return (ret + 1) >> 1;
	}
}
static void parse_scaling_list(uint32_t size, bit_buffer * bb) {
	uint32_t last_scale, next_scale, i;
	int32_t delta_scale;
	last_scale = 8;
	next_scale = 8;
	for (i = 0; i < size; i++) {
		if (next_scale != 0) {
			delta_scale = exp_golomb_se(bb);
			next_scale = (last_scale + delta_scale + 256) % 256;
		}
		if (next_scale != 0) {
			last_scale = next_scale;
		}
	}
}
/**
Parses a SPS NALU to retrieve video width and height
*/
// {0x67 ,0x42 ,0xE0 ,0x0A ,0x89 ,0x95 ,0x42 ,0xC1 ,0x2C ,0x80}
//  0100 0010 1110 0000
static void parse_sps(uint8_t * sps, int sps_size, int * width, int * height)
{
	bit_buffer bb;
	uint32_t profile, pic_order_cnt_type, width_in_mbs, height_in_map_units;
	uint32_t i, size, left, right, top, bottom;
	uint8_t frame_mbs_only_flag;


	bb.start = sps;
	bb.size = sps_size;
	bb.current = sps;
	bb.read_bits = 0;
    /*
	DPRINT(VIDEO_LOG_DEBUG,"---------------------\n");
	uint8_t tmp = 0;
	for (i = 0; i<sps_size * 8; i++)
	{
		tmp = get_bit(&bb);
		if (i % 4)
			printf("%d", tmp);
		else
			printf(" %d", tmp);
	}
	DPRINT(VIDEO_LOG_DEBUG,"\n");
	*/
	bb.current = sps;
	bb.read_bits = 0;
	/* skip first byte, since we already know we're parsing a SPS */
	skip_bits(&bb, 8);
	/* get profile */
	profile = get_bits(&bb, 8);
	DPRINT(VIDEO_LOG_DEBUG,"%s,%d: profile=0x%x\n", __FUNCTION__, __LINE__, profile);
	/* skip 4 bits + 4 zeroed bits + 8 bits = 32 bits = 4 bytes */
	skip_bits(&bb, 16);
	/* read sps id, first exp-golomb encoded value */
	exp_golomb_ue(&bb);   // sps id
	if (profile == 100 || profile == 110 || profile == 122 || profile == 144) {
		/* chroma format idx */
		if (exp_golomb_ue(&bb) == 3) {
			skip_bits(&bb, 1);
		}
		/* bit depth luma minus8 */
		exp_golomb_ue(&bb);
		/* bit depth chroma minus8 */
		exp_golomb_ue(&bb);
		/* Qpprime Y Zero Transform Bypass flag */
		skip_bits(&bb, 1);
		/* Seq Scaling Matrix Present Flag */
		if (get_bit(&bb)) {
			for (i = 0; i < 8; i++) {
				/* Seq Scaling List Present Flag */
				if (get_bit(&bb)) {
					parse_scaling_list(i < 6 ? 16 : 64, &bb);
				}
			}
		}
	}
	/* log2_max_frame_num_minus4 */
	int f1 = exp_golomb_ue(&bb);
	DPRINT(VIDEO_LOG_DEBUG,"%s,%d: f1=0x%x\n", __FUNCTION__, __LINE__, f1);
	//DPRINT("%s,%d: exp_golomb_ue=0x%x\n", __FUNCTION__,__LINE__,get_bits(&bb, 8));
	/* pic_order_cnt_type */
	pic_order_cnt_type = exp_golomb_ue(&bb);
	if (pic_order_cnt_type == 0) {
		/* log2_max_pic_order_cnt_lsb_minus4 */
		exp_golomb_ue(&bb);
	}
	else if (pic_order_cnt_type == 1) {
		/* delta_pic_order_always_zero_flag */
		skip_bits(&bb, 1);
		/* offset_for_non_ref_pic */
		exp_golomb_se(&bb);
		/* offset_for_top_to_bottom_field */
		exp_golomb_se(&bb);
		size = exp_golomb_ue(&bb);
		for (i = 0; i < size; i++) {
			/* offset_for_ref_frame */
			exp_golomb_se(&bb);
		}
	}
	/* num_ref_frames */
	exp_golomb_ue(&bb);
	/* gaps_in_frame_num_value_allowed_flag */
	skip_bits(&bb, 1);
	/* pic_width_in_mbs */
	width_in_mbs = exp_golomb_ue(&bb) + 1;
	/* pic_height_in_map_units */
	height_in_map_units = exp_golomb_ue(&bb) + 1;
	/* frame_mbs_only_flag */
	frame_mbs_only_flag = get_bit(&bb);
	if (!frame_mbs_only_flag) {
		/* mb_adaptive_frame_field */
		skip_bits(&bb, 1);
	}
	/* direct_8x8_inference_flag */
	skip_bits(&bb, 1);
	/* frame_cropping */
	left = right = top = bottom = 0;
	if (get_bit(&bb)) {
		left = exp_golomb_ue(&bb) * 2;
		right = exp_golomb_ue(&bb) * 2;
		top = exp_golomb_ue(&bb) * 2;
		bottom = exp_golomb_ue(&bb) * 2;
		if (!frame_mbs_only_flag) {
			top *= 2;
			bottom *= 2;
		}
	}
	/* width */
	*width = width_in_mbs * 16 - (left + right);
	/* height */
	*height = height_in_map_units * 16 - (top + bottom);
	if (!frame_mbs_only_flag) {
		*height *= 2;
	}
}




int SetPortReuse(SOCKET sock, int bReuse)
{
    int value = bReuse ? 1 : 0;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &value, sizeof (value)) < 0)
    {
        DPRINT(VIDEO_LOG_ERR,"setsockopt:SO_REUSEADDR failed - %s\n", strerror(errno));
        return 0;
    }
    else
    {
        return 1;
    }
}

int JoinGroup(SOCKET sock, const char* strGroupIP)
{
    //sDPRINT("begin jion group\n");
    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(strGroupIP);

    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof (struct ip_mreq)) < 0)
    {
        DPRINT(VIDEO_LOG_ERR,"setsockopt:IP_ADD_MEMBERSHIP failed - %s\n", strerror(errno));
        return -1;
    }
    DPRINT(VIDEO_LOG_INFO,"JoinGroup:%s\n", strGroupIP);
    return 0;
}

static void* video_recv_thread(void* arg)
{
    rtp_s* p_rtp = (rtp_s*) arg;
    SOCKET sock = 0;
    SOCKADDR_IN addr;
    uint8_t p_recv_buf[8192];
    int i_recv_size = 0;
    char p_save_buf[8192];
    int i_time_out = 0;
    SOCKET sockRtcp = 0;
    SOCKADDR_IN addrRtcp;
    int recv_len = 8192;
    
    if (p_rtp == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"[%s][%d]--->>> arg is NULL\n", __func__, __LINE__);
        return NULL;
    }
    DPRINT(VIDEO_LOG_INFO,"===================rtp thread start id:%u==============================\n",pthread_self());

    while (p_rtp->i_exit)
    {
        //pthread_testcancel();
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        addr.sin_family = AF_INET;
        addr.sin_port = htons(p_rtp->i_port);
        if (p_rtp->p_ip[0] != 0)
        {
            addr.sin_addr.s_addr = inet_addr(p_rtp->p_ip);
        }
        else
        {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }

        SetPortReuse(sock, 1);

        // bind
        if (bind(sock, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN)) < 0)
        {
            DPRINT(VIDEO_LOG_ERR,"bind rtp socket error = %s\n", strerror(errno));
            return NULL;
        }

        #ifdef TEST_SOCKET
        #else

        sockRtcp= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        addrRtcp.sin_family = AF_INET;
        addrRtcp.sin_port = htons(p_rtp->i_port+1);
        if (p_rtp->p_ip[0] != 0)
        {
            addrRtcp.sin_addr.s_addr = inet_addr(p_rtp->p_ip);
        }
        else
        {
            addrRtcp.sin_addr.s_addr = htonl(INADDR_ANY);
        }

        SetPortReuse(sockRtcp, 1);

        // bind
        if (bind(sockRtcp, (SOCKADDR*)&addrRtcp, sizeof(SOCKADDR_IN)) < 0)
        {
			DPRINT(VIDEO_LOG_ERR,"bind rtcp socket error = %s\n", strerror(errno));
            return NULL;
        }
        #endif
        
        if (p_rtp->p_ip[0] != 0)
        {
            unsigned int lIP = inet_addr(p_rtp->p_ip);
            if ((lIP & 0x000000E0) == 0xE0)
            {
                if (JoinGroup(sock, p_rtp->p_ip) < 0)
                {
                    DPRINT(VIDEO_LOG_ERR,"JoinGroup failed, group ip = %s\n", p_rtp->p_ip);
                }
                else
                {
                    DPRINT(VIDEO_LOG_INFO,"jion group v4 success, %s\n", p_rtp->p_ip);
                }
            }
        }

        struct timeval t;
        t.tv_sec = 0;
        t.tv_usec = 800000;

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &t, sizeof (t)) == -1)
        {
            DPRINT(VIDEO_LOG_ERR,"set rtp scok recv time out failed %s\n", strerror(errno));
        }

        #ifdef TEST_SOCKET
        #else
        if (setsockopt(sockRtcp, SOL_SOCKET, SO_RCVTIMEO, (void *) &t, sizeof (t)) == -1)
        {
            DPRINT(VIDEO_LOG_ERR,"set rtcp scok recv time out failed %s\n", strerror(errno));
        }
        #endif


    	if ( -1 == setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&recv_len, sizeof(int)) )
    	{// set receive buffer size
    	    DPRINT(VIDEO_LOG_ERR,"set rtp scok recv buf  failed %s\n", strerror(errno));
    	}
        
        int nal_len = 0;
        int nal_flag = 0;
        
        while(p_rtp->i_exit)
        {
            //pthread_testcancel();

            i_recv_size = recv(sock, p_recv_buf, sizeof (p_recv_buf), 0);
            if (i_recv_size > 0)
            {
                i_time_out = 0;
                rtp_header_t rtp_header;

                //DPRINT(VIDEO_LOG_DEBUG,"==============len:%d========================\n", i_recv_size);
                get_rtp_header(&rtp_header, p_recv_buf, i_recv_size);
                if (rtp_header.i_pt == 0x60)// VIDEO
                {   

                    //解析rtp
                    int ret = RtpTo264(p_recv_buf, i_recv_size, p_save_buf,  &nal_len,&nal_flag);
                    if (ret == -1)
                	{
                		p_rtp->i_video_frame_index = 0;
                	}
                	else
                	{
                        //增加rtp包
                        pthread_mutex_lock(&p_rtp->sendInfo.rtpmutex);
                        if(dlist_length(p_rtp->sendInfo.rtpList) <= RTP_PACKAGE_COUNT && p_rtp->sendInfo.send_flag == 1)
                        {
                            if(nal_flag == 4 || nal_flag == 5)
                            {
                                p_rtp->sendInfo.spsflag = 1;
                            }

                            if( p_rtp->sendInfo.spsflag)
                            {
                                char* p_new = NULL;
                                p_new = malloc(i_recv_size + 4);
                                if (p_new == NULL)
                                {
                                    DPRINT(VIDEO_LOG_ERR,"malloc rtp package error\n");
                                }
                                else
                                {
                                    uint32_t* p = (uint32_t*)(&p_new[0]);
                                    *p = htonl(i_recv_size + 4);                                    
                                    memcpy(p_new+4, p_recv_buf, i_recv_size);
                                }
                                
                                dlist_append(p_rtp->sendInfo.rtpList, p_new);
                                pthread_cond_signal(&p_rtp->sendInfo.sCond);                                
                            }

                        }
                        pthread_mutex_unlock(&p_rtp->sendInfo.rtpmutex);
                    
                        memcpy(p_rtp->p_video_frame + p_rtp->i_video_frame_index, p_save_buf, nal_len);
                        p_rtp->i_video_frame_index += nal_len;
                		if (nal_flag== 3 || nal_flag == 0)
                		{                			
                            nal_flag = 0;
                		}
                        else 
                        {
                            pthread_mutex_lock(&p_rtp->writeInfo.mutex);
                            if(nal_flag == 4)
                            {
                                //sps
                                if(p_rtp->writeInfo.sps[4] == 0x00)
                                {
                                    DPRINT(VIDEO_LOG_DEBUG,"\n======================sps===============\n");
                                    memcpy(p_rtp->writeInfo.sps,  p_save_buf, nal_len);
                                    parse_sps(p_save_buf+4, nal_len -4, &p_rtp->writeInfo.width, &p_rtp->writeInfo.height);
                                    p_rtp->writeInfo.sps_len = nal_len;
                                    DPRINT(VIDEO_LOG_DEBUG,"\n======================video width:%d height:%d===============\n",p_rtp->writeInfo.width, p_rtp->writeInfo.height);
                                }
                                
                                nal_flag = 0;
                            }
                            else if(nal_flag == 5)
                            {
                                //pps
                                if(p_rtp->writeInfo.pps[4] ==0x00)
                                {
                                    DPRINT(VIDEO_LOG_DEBUG,"\n======================pps===============\n");
                                    memcpy(p_rtp->writeInfo.pps,  p_save_buf, nal_len);
                                    p_rtp->writeInfo.pps_len = nal_len;
                                }
                                
                                nal_flag = 0;
                            }
                            pthread_mutex_unlock(&p_rtp->writeInfo.mutex);
                            
                        }                        
 
                        if (p_rtp->i_video_frame_index > 0 && (nal_flag== 0) && (p_rtp->writeInfo.sps[4] !=0x00 && p_rtp->writeInfo.pps[4] != 0x00))
                        {
                            //DPRINT(VIDEO_LOG_DEBUG,"write add buf num :%d\n", p_rtp->writeInfo.i_buf_num);
                            add_frame(p_rtp, p_rtp->p_video_frame, p_rtp->i_video_frame_index);
                            p_rtp->i_video_frame_index = 0;
                        }
                	}
                    
                }
            }
            else
            {
                i_time_out += 500;
                if (i_time_out > 5000)
                {
                    DPRINT(VIDEO_LOG_INFO,"rtp no data recv\n");
                    i_time_out = 0;
                }
                p_rtp->m_count++;
            }
        }

        close(sock);
        sock = -1;

#ifdef TEST_SOCKET
#else
        close(sockRtcp);
#endif
        sockRtcp= -1;
    }

    if(sock >0)
    {
        close(sock);
        sock = -1;
    }
#ifdef TEST_SOCKET
#else
     if(sockRtcp>0)
    {
        close(sockRtcp);
        sockRtcp = -1;
    }
#endif
    DPRINT(VIDEO_LOG_INFO,"===================rtp thread exit id:%u==============================\n",pthread_self());
    return NULL;
}

static void* writeThread(void* arg)
{
    rtp_s* p_rtp = (rtp_s*) arg;
    if (p_rtp == NULL)
    {
        DPRINT(VIDEO_LOG_INFO,"arg is null\n");
        return;
    }

    DPRINT(VIDEO_LOG_INFO,"===================write thread start id:%u==============================\n",pthread_self());
    int ret = 0;
    while(p_rtp->writeInfo.m_exit)
    {
        //pthread_testcancel();
        if(writemp4file(&(p_rtp->writeInfo))< 0)
        {
            DPRINT(VIDEO_LOG_DEBUG,"write mp4 file error\n");
            sleep(1);
        }
    }

    DPRINT(VIDEO_LOG_INFO,"===================write thread exit id:%u==============================\n",pthread_self());
    return 0;
}


#define		DATA_PACKET_TIMEOUT		20
static void* sendRtpThread(void* arg)
{
    rtp_s* p_rtp = (rtp_s*) arg;
    if (p_rtp == NULL)
    {
        DPRINT(VIDEO_LOG_INFO,"arg is NULL\n");
        return;
    }

    DPRINT(VIDEO_LOG_INFO,"===================send rtp data thread start id:%u==============================\n",pthread_self());
    int dlistNull= 0;
    int ret = 0;

    while(p_rtp->sendInfo.m_exit)
    {
        //pthread_testcancel();
        
        char * tmpRtp =NULL;
        pthread_mutex_lock(&p_rtp->sendInfo.rtpmutex);
        if(dlist_length(p_rtp->sendInfo.rtpList) <= 0)
        {
            pthread_cond_wait(&p_rtp->sendInfo.sCond,&p_rtp->sendInfo.rtpmutex);
        }
        
        dlist_get_by_index(p_rtp->sendInfo.rtpList,0,(void *)&tmpRtp);
        if(tmpRtp == NULL)
        {
            dlist_delete(p_rtp->sendInfo.rtpList,0);
            pthread_mutex_unlock(&p_rtp->sendInfo.rtpmutex);
            continue;
        }
        else
        {
            dlist_delete_ex(p_rtp->sendInfo.rtpList,0);
        }
        pthread_mutex_unlock(&p_rtp->sendInfo.rtpmutex);
        
        if(p_rtp->sendInfo.send_flag && p_rtp->sendInfo.send_init)
        {
            RTP_FIXED_HEADER * rtp_hdr = NULL;
	        rtp_hdr = (RTP_FIXED_HEADER*)&tmpRtp[4];
	        DPRINT(VIDEO_LOG_DEBUG,"============================================seq no:%d\n", ntohs(rtp_hdr->seq_no));
            
            if(sendIPCData(p_rtp->sendInfo.send_sock, tmpRtp, ntohl(*((uint32_t*)tmpRtp)), DATA_PACKET_TIMEOUT) <= 0)
            {
                DPRINT(VIDEO_LOG_INFO, "rtp send data error\n");
                p_rtp->sendInfo.send_init = 0;
            }
        } 
        free(tmpRtp);
    }

    DPRINT(VIDEO_LOG_INFO,"===================send rtp data thread exit id:%u==============================\n",pthread_self());
    return 0;
}

int threadCreate(THREAD* funcThread, void* param)
{
    
    pthread_attr_t attr;
    pthread_t Thrd;
    #if 1
    struct sched_param SchedParam;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched_getparam(0, &SchedParam);
    SchedParam.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &SchedParam);

    int s = pthread_create(&Thrd, &attr, funcThread, param);
    #else
    int s = pthread_create(&Thrd, NULL, funcThread, param);
    #endif
    if (s != 0)
    {
        DPRINT(VIDEO_LOG_ERR,"threadCreate failed.\n");
        //handle_error_en(s, "pthread_create");
        exit(-1);
    }

    return 0;
}



unsigned long rtp_init(char* p_ip, char * mac, int i_port, int flag)
{
    rtp_s *p_rtp;
	p_rtp = (rtp_s *) malloc(sizeof(rtp_s));
	if(p_rtp == NULL)
	{
		DPRINT(VIDEO_LOG_ERR, "malloc rtp_s error\n");
		return VIDEO_NEW_ERROR;
	}
    memset(p_rtp, 0, sizeof(rtp_s));
    memcpy(p_rtp->writeInfo.mac, mac, strlen(mac));
    p_rtp->writeInfo.m_day = 7*24*60;
    p_rtp->writeInfo.m_time= 5;
    p_rtp->writeInfo.m_size= 8;
    p_rtp->writeInfo.fileHand = NULL;
    
    p_rtp->i_video_frame_index = 0;
    p_rtp->p_video_frame = NULL;
    p_rtp->p_video_frame = (uint8_t*) malloc(MAX_VIDEO_FRAME_SIZE);
    if(p_rtp->p_video_frame == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"malloc p_rtp->p_video_frame error\n");
        free(p_rtp);
		return VIDEO_NEW_ERROR;
    }

    removeRcFile(mac);
    p_rtp->writeInfo.i_buf_num = 0;
    p_rtp->writeInfo.p_frame_buf = NULL;
    p_rtp->writeInfo.p_frame_header = NULL;
    p_rtp->writeInfo.m_flag = flag;
            
    p_rtp->i_exit = 1;
    pthread_mutex_init(&p_rtp->writeInfo.mutex, NULL);
    pthread_cond_init(&p_rtp->writeInfo.cond, NULL);
    
    if(p_ip == NULL)
    {
        memset(p_rtp->p_ip, 0, 40);
    }
    else
    {
        memcpy(p_rtp->p_ip, p_ip, strlen(p_ip));
    }
    p_rtp->i_port = i_port;      

    memset(p_rtp->writeInfo.sps, 0, 32);
    memset(p_rtp->writeInfo.pps, 0, 32);
    p_rtp->writeInfo.width = 352;
    p_rtp->writeInfo.height = 288;

    p_rtp->sendInfo.send_flag = 0;
    p_rtp->sendInfo.send_sock = -1;
    p_rtp->sendInfo.spsflag = 0;
    p_rtp->sendInfo.m_exit = 0;

    p_rtp->m_count =0;

    #ifdef WRITE_FILE_TEST 
    p_rtp->writeInfo.fileCount = 0;
    #else
    p_rtp->writeInfo.writeOpt.m_dlist = dlist_create();
    get_file(p_rtp->writeInfo.mac, p_rtp->writeInfo.writeOpt.m_dlist,p_rtp->writeInfo.m_day/p_rtp->writeInfo.m_time);
    p_rtp->writeInfo.writeOpt.m_index = 0;
    #endif

    pthread_mutex_init(&p_rtp->sendInfo.rtpmutex, NULL);
    p_rtp->sendInfo.rtpList = NULL;
            
      
    DPRINT(VIDEO_LOG_DEBUG,"ip:%s, port:%d\n", p_rtp->p_ip, p_rtp->i_port);

    int s = pthread_create(&p_rtp->rtpThread, NULL, video_recv_thread, p_rtp);
    if (s != 0)
    {
        pthread_mutex_destroy(&(p_rtp->sendInfo.rtpmutex));
        pthread_mutex_destroy(&(p_rtp->writeInfo.mutex));
        pthread_cond_destroy(&(p_rtp->writeInfo.cond));
        free(p_rtp);
        DPRINT(VIDEO_LOG_ERR,"======================rtp recv threadCreate failed.=========================\n");
        return VIDEO_NEW_ERROR;
    }

    p_rtp->writeInfo.m_exit = 1;
    s = pthread_create(&p_rtp->writeInfo.threadId, NULL, writeThread, p_rtp);
    if (s != 0)
    {
        //pthread_cancel(p_rtp->rtpThread);
        p_rtp->i_exit = 0;
        pthread_join(p_rtp->rtpThread,NULL);
        pthread_mutex_destroy(&(p_rtp->sendInfo.rtpmutex));
        pthread_mutex_destroy(&(p_rtp->writeInfo.mutex));
        pthread_cond_destroy(&(p_rtp->writeInfo.cond));
        free(p_rtp);
        DPRINT(VIDEO_LOG_ERR,"======================wrirte threadCreate failed.=========================\n");
        return VIDEO_NEW_ERROR;
    }

    return (unsigned long) p_rtp;
}

int rtp_deinit(unsigned long hand)
{
    rtp_s* p_rtp = (rtp_s *)hand;
    int i_ret = VIDEO_NEW_ERROR;

    if (p_rtp != NULL)
    {
        p_rtp->i_exit = 0;
        p_rtp->writeInfo.m_exit = 0;
        
        stop_send(hand, p_rtp->writeInfo.mac);
        p_rtp->m_count =0;
        
        //pthread_cancel(p_rtp->rtpThread);
        pthread_join(p_rtp->rtpThread,NULL);
        
        DPRINT(VIDEO_LOG_ERR,"rtp recv wait thread exit success\n");

        pthread_cond_signal(&p_rtp->writeInfo.cond);
        //pthread_cancel(p_rtp->writeInfo.threadId);
        pthread_join(p_rtp->writeInfo.threadId,NULL);

        DPRINT(VIDEO_LOG_ERR,"rtp exit write file\n");
        
        if (p_rtp->p_video_frame != NULL)
        {
            free(p_rtp->p_video_frame);
            p_rtp->p_video_frame = NULL;
        }

        clear_frame(p_rtp);
    
        DPRINT(VIDEO_LOG_DEBUG,"rtp thread exit success\n");
        pthread_mutex_destroy(&(p_rtp->sendInfo.rtpmutex));
        pthread_mutex_destroy(&(p_rtp->writeInfo.mutex));
        pthread_cond_destroy(&(p_rtp->writeInfo.cond));
        #ifdef WRITE_FILE_TEST
        p_rtp->writeInfo.fileCount = 0;
        #else
        if(p_rtp->writeInfo.writeOpt.m_dlist)
            dlist_destroy(p_rtp->writeInfo.writeOpt.m_dlist);
        p_rtp->writeInfo.writeOpt.m_dlist = NULL;
        #endif
        free(p_rtp);
    }
    else
    {
        DPRINT(VIDEO_LOG_ERR,"==============rtp_s is null\n");
        i_ret = VIDEO_ERR;
    }

    return i_ret;
}

int start_send(unsigned long hand, char *mac, int localPort)
{
    int ret = 0;
    char * ip = "127.0.0.1";
    rtp_s* p_rtp = (rtp_s *)hand;
    if(strncmp(p_rtp->writeInfo.mac, mac, strlen(mac)) == 0)
    {
        if(p_rtp->sendInfo.send_flag == 1)
        {
            DPRINT(VIDEO_LOG_DEBUG,"please stop send\n");
            return 1;
        }
        DPRINT(VIDEO_LOG_INFO,"send mac =========%s=======\n",mac);
        
        ret = create_local_socket(ip,localPort);
        if(ret <= 0)
        {
            DPRINT(VIDEO_LOG_ERR,"create local socket error\n");
            return VIDEO_ERR;
        }
        pthread_cond_init(&p_rtp->sendInfo.sCond, NULL);
        pthread_mutex_lock(&p_rtp->sendInfo.rtpmutex);
        p_rtp->sendInfo.rtpList= dlist_create();
        pthread_mutex_unlock(&p_rtp->sendInfo.rtpmutex);
        p_rtp->sendInfo.send_sock = ret;
        p_rtp->sendInfo.send_flag = 1;
        p_rtp->sendInfo.spsflag= 0;
        p_rtp->sendInfo.m_exit = 1;
        p_rtp->sendInfo.send_init= 1;
        p_rtp->sendInfo.send_port = localPort;

        int s = pthread_create(&p_rtp->sendInfo.sThreadId, NULL, sendRtpThread, p_rtp);
        if (s != 0)
        {
            DPRINT(VIDEO_LOG_ERR,"======================rtp send data threadCreate failed.=========================\n");
            if(p_rtp->sendInfo.send_sock >0)
                close(p_rtp->sendInfo.send_sock);
            p_rtp->sendInfo.send_sock = -1;
            dlist_destroy(p_rtp->sendInfo.rtpList);
            p_rtp->sendInfo.rtpList = NULL;
            pthread_cond_destroy(&p_rtp->sendInfo.sCond);
            p_rtp->sendInfo.m_exit = 0;
            p_rtp->sendInfo.sThreadId = 0;
            
            return VIDEO_NEW_ERROR;
        }
        DPRINT(VIDEO_LOG_DEBUG,"start mac:%s send data success\n",mac);
        return VIDEO_SUCCESS;
    }

    return VIDEO_FATAL_ERROR;
}

int stop_send(unsigned long hand, char *mac)
{
    int ret = 0;
    rtp_s* p_rtp = (rtp_s *)hand;
    if(strncmp(p_rtp->writeInfo.mac, mac, strlen(mac)) == 0)
    {
        DPRINT(VIDEO_LOG_INFO,"stop mac ========%s========\n",mac);
        if(p_rtp->sendInfo.send_flag == 0)
        {
            DPRINT(VIDEO_LOG_INFO,"current mac %s is not send data\n",mac);
            return VIDEO_SUCCESS;
        }
        p_rtp->sendInfo.m_exit = 0;
        p_rtp->sendInfo.send_flag = 0;
        p_rtp->sendInfo.spsflag= 0;
        p_rtp->sendInfo.send_init = 0;
        pthread_cond_signal(&p_rtp->sendInfo.sCond);
        //pthread_cancel(p_rtp->sendInfo.sThreadId);
        pthread_join(p_rtp->sendInfo.sThreadId,NULL);
        pthread_cond_destroy(&p_rtp->sendInfo.sCond);
        p_rtp->sendInfo.sThreadId = 0;
        
        //sleep(1);
        if(p_rtp->sendInfo.send_sock >0)
            close(p_rtp->sendInfo.send_sock);
        p_rtp->sendInfo.send_sock = -1;
        pthread_mutex_lock(&p_rtp->sendInfo.rtpmutex);
        dlist_destroy(p_rtp->sendInfo.rtpList);
        p_rtp->sendInfo.rtpList = NULL;
        pthread_mutex_unlock(&p_rtp->sendInfo.rtpmutex);
        p_rtp->sendInfo.rtpList = NULL;
        DPRINT(VIDEO_LOG_DEBUG,"stop mac:%s send data success\n",mac);
        return VIDEO_SUCCESS;
    }
    return VIDEO_FATAL_ERROR;
}

int initSendRtpSocket(unsigned long hand)
{
    rtp_s* p_rtp = (rtp_s *)hand;
    int ret = 0;

    if(p_rtp->sendInfo.m_exit && !p_rtp->sendInfo.send_init)
    {
        if(p_rtp->sendInfo.send_sock > 0)
        {
            close(p_rtp->sendInfo.send_sock);
            p_rtp->sendInfo.send_sock =-1;
        }
        ret = create_local_socket("127.0.0.1", p_rtp->sendInfo.send_port);
        if(ret <= 0)
        {
            DPRINT(VIDEO_LOG_ERR,"send rtp create local socket error\n");
            return VIDEO_ERR;
        }
        p_rtp->sendInfo.send_sock = ret;
        p_rtp->sendInfo.send_init= 1; 
        return VIDEO_SUCCESS;
    }
    return VIDEO_FATAL_ERROR;
}
int get_err_count(unsigned long hand)
{
    rtp_s* p_rtp = (rtp_s *)hand;
    if(p_rtp != NULL)
    {
        if(p_rtp->m_count > 40)
        {
            return -1;
        }
        return 0;
    }

    return -1;
}
