
#include <stdlib.h>
#include "dlist.h"

static DListNode* dlist_node_create(void* data)
{
	DListNode* node = (DListNode*)malloc(sizeof(DListNode));

	if(node != NULL)
	{
		node->prev = NULL;
		node->next = NULL;
		node->data = data;
	}

	return node;
}

static void dlist_node_destroy(DListNode* node)
{
	if(node != NULL)
	{
		node->next = NULL;
		node->prev = NULL;
        if (node->data != NULL)
        {
            free(node->data);
        }
		free(node);
	}
}

static void dlist_node_destroy_ex(DListNode* node)
{
	if(node != NULL)
	{
		node->next = NULL;
		node->prev = NULL;
        /*
        if (node->data != NULL)
        {
            free(node->data);
        }
        */
		free(node);
	}
}

DList* dlist_create(void)
{
	DList* thiz =(DList*) malloc(sizeof(DList));

	if(thiz != NULL)
	{
		thiz->first = NULL;
	}

	return thiz;
}

static DListNode* dlist_get_node(DList* thiz, int index, int fail_return_last)
{
	DListNode* iter = thiz->first;

	while(iter != NULL && iter->next != NULL && index > 0)
	{
		iter = iter->next;
		index--;
	}

	if(!fail_return_last)
	{
		iter = index > 0 ? NULL : iter;
	}

	return iter;
}

DListRet dlist_insert(DList* thiz, int index, void* data)
{
	DListNode* node = NULL;
	DListNode* cursor = NULL;

	if((node = dlist_node_create(data)) == NULL)
	{
		return DLIST_RET_OOM; 
	}

	if(thiz->first == NULL)
	{
		thiz->first = node;

		return DLIST_RET_OK;
	}

	cursor = dlist_get_node(thiz, index, 1);
    if(cursor == NULL)
    {
        free(data);
        free(node);
        return DLIST_RET_FAIL;
    }
	
	if(index < dlist_length(thiz))
	{
		node->next = cursor;
		cursor->prev = node;
		if(thiz->first == cursor)
		{
			thiz->first = node;
		}
	}
	else
	{
		cursor->next = node;
		node->prev = cursor;
	}

	return DLIST_RET_OK;
}

DListRet dlist_prepend(DList* thiz, void* data)
{
	return dlist_insert(thiz, 0, data);
}

DListRet dlist_append(DList* thiz, void* data)
{
	return dlist_insert(thiz, dlist_length(thiz), data);
}

DListRet dlist_delete_node(DList* thiz, DListNode* node)
{
	if(node != NULL)
	{
		if(node == thiz->first)
		{
			thiz->first = node->next;
		}

		if(node->next != NULL)
		{
			node->next->prev = node->prev;
		}

		if(node->prev != NULL)
		{
			node->prev->next = node->next;
		}

		dlist_node_destroy(node);
	}

	return DLIST_RET_OK;

}

DListNode* dlist_delete_node_ex(DList* thiz, DListNode* node)
{
    DListNode* tmp = NULL;
	if(node != NULL)
	{
		if(node == thiz->first)
		{
			thiz->first = node->next;
		}

		if(node->next != NULL)
		{
			node->next->prev = node->prev;
		}

		if(node->prev != NULL)
		{
			node->prev->next = node->next;
		}

        tmp = node->next;
		dlist_node_destroy(node);
	}

	return tmp;

}

DListRet dlist_delete(DList* thiz, int index)
{
	DListNode* cursor = dlist_get_node(thiz, index, 0);

	if(cursor != NULL)
	{
		if(cursor == thiz->first)
		{
			thiz->first = cursor->next;
		}

		if(cursor->next != NULL)
		{
			cursor->next->prev = cursor->prev;
		}

		if(cursor->prev != NULL)
		{
			cursor->prev->next = cursor->next;
		}

		dlist_node_destroy(cursor);
	}

	return DLIST_RET_OK;
}

DListRet dlist_delete_ex(DList* thiz, int index)
{
	DListNode* cursor = dlist_get_node(thiz, index, 0);

	if(cursor != NULL)
	{
		if(cursor == thiz->first)
		{
			thiz->first = cursor->next;
		}

		if(cursor->next != NULL)
		{
			cursor->next->prev = cursor->prev;
		}

		if(cursor->prev != NULL)
		{
			cursor->prev->next = cursor->next;
		}

		dlist_node_destroy_ex(cursor);
	}

	return DLIST_RET_OK;
}

DListRet dlist_get_by_index(DList* thiz, int index, void** data)
{
	DListNode* cursor = dlist_get_node(thiz, index, 0);

	if(cursor != NULL)
	{
		*data = cursor->data;
	}

	return cursor != NULL ? DLIST_RET_OK : DLIST_RET_FAIL;
}

DListRet dlist_set_by_index(DList* thiz, int index, void* data)
{
	DListNode* cursor = dlist_get_node(thiz, index, 0);

	if(cursor != NULL)
	{
		cursor->data = data;
	}

	return cursor != NULL ? DLIST_RET_OK : DLIST_RET_FAIL;
}

int   dlist_length(DList* thiz)
{
    if(thiz == NULL)
    {
        return 0;
    }
	int length = 0;
	DListNode* iter = thiz->first;

	while(iter != NULL)
	{
		length++;
		iter = iter->next;
	}

	return length;
}

DListRet dlist_print(DList* thiz, DListDataPrintFunc print)
{
	DListRet ret = DLIST_RET_OK;
	DListNode* iter = thiz->first;

	while(iter != NULL)
	{
		print(iter->data);

		iter = iter->next;
	}

	return ret;
}

void dlist_destroy(DList* thiz)
{
    if(thiz == NULL)
    {
        return;
    }
	DListNode* iter = thiz->first;
	DListNode* next = NULL;

	while(iter != NULL)
	{
		next = iter->next;
		dlist_node_destroy(iter);
		iter = next;
	}

	thiz->first = NULL;

	free(thiz);

	return;
}


