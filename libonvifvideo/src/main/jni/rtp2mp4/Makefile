OBJ = main.o dlist.o rtp.o rtsp_client.o rtsp_command.o rtsp_response.o rtsp_sock.o writemp4lib.o localsocket.o writemp4file.o crc32.o
exe = cmmb2rtp
CC = gcc -g -DCONFIG_VIDEO_LOG
CCOPT = -Wall -I /usr/local/include/ -L /usr/local/lib/ 
#-D_FILE_OFFSET_BITS=64
$(exe):$(OBJ)
	$(CC) $(CCOPT)  -o $(exe) $(OBJ) -lpthread -lmp4v2

clean:
	rm -f *.o $(exe) *.mp4
