//B+树的功能实现

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "btree.h"

bpt_t g_btree = {0};

#define log_debug(msg,...)  printf(msg, ##__VA_ARGS__)
#define new(type, num)	(((type) *)malloc(sizeof(type)*(num))

btnode_t * bt_new_node(btlvl_t level, int num){
	int size = 0;
	if(BT_LVL_LEAF == level)
		size = sizeof(btnode_t)+sizeof(btk_t)*num;
	else
		size = sizeof(btnode_t)+sizeof(btk_t)*num+sizeof(btp_t)*num;
	btnode_t * pnode = (btnode_t *)new(char, size);
	if(NULL == pnode){
		log_debug("Malloc new Node for B+Tree failed, please check!\n");
		return NULL;
	}
	pnode->bn_sblock.bts_level = level;
	pnode->bn_sblock.bts_num = num;
	pnode->bn_sblock.bts_left = NULL;
	pnode->bn_sblock.bts_right = NULL;
	pnode->bn_entry = (char *)pnode + sizeof(bts_t);
	return pnode;
}

/*compare key*/
int bt_key_cmp(BT_KEY key1, BT_KEY key2){
	if(key1 == key2)
		return 0;
	if(key1 > key2)
		return 1;
	else if(key1 == key2)
		return 0;
	else
		return -1;
}

/*add array elem for inter node*/
int bt_add_branch(btnode_t * pNode, void * elems, int num, int maxnum){
	if(pNode->bn_sblock.bts_num+num > maxnum)
		return BTREE_KEY_FULL;
	void * tmpbuf = new(char, (sizeof(BT_KEY)+sizeof(BT_PTR))*(pNode->bn_sblock.bts_num+num));
	if(NULL == tmpbuf){
		log_debug("malloc tmp buf for add branch failed!\n");
		return BTREE_FAIL;
	}
	BT_KEY * tmpkeys = (BT_KEY *)tmpbuf;
	BT_PTR * tmpptrs = (BT_PTR *)((char *)tmpbuf + (pNode->bn_sblock.bts_num+num)*sizeof(BT_KEY));
	int i;
	BT_KEY * keys = (BT_KEY *)elems;
	BT_PTR * ptrs = (BT_PTR *)((char *)elems + num*sizeof(BT_KEY));
	for(i=0; i<num; i++){
		if(bt_key_cmp(keys[i],))
	}
}

/*add array elem for leaf node*/
void bt_add_data(btnode_t * pNode, BT_KEY * elems, int num){
}

#if 0
void print_btree(bpt_t * ptree)
{
	if(NULL == pnode)
	{
		return;
	}
	
	char sub_prefix[100] = {0};
	memset(sub_prefix,0,100);
	int i = 0;
	for(i=0; i<BTREE_KEY_NUM; i++)
	{
		if(0 == pnode->bn_key[i])
		{
			break;
		}
		if(len > 0 )
		{
			if(0 != pnode->bn_key[i+1])
			{
				log_debug("%s├──────%d\r\n",prefix, pnode->bn_key[i]);
			}
			else
			{
				log_debug("%s└──────%d\r\n",prefix, pnode->bn_key[i]);
			}
		}
		else
		{
			if(0 == i)
			{
				log_debug("Root───%d\r\n",prefix, pnode->bn_key[i]);
			}
			else if(0 != pnode->bn_key[i+1])
			{
				log_debug("├──────%d\r\n", pnode->bn_key[i]);
			}
			else
			{
				log_debug("└──────%d\r\n", pnode->bn_key[i]);
			}
		}
		if(pnode->bn_pointer[i] != NULL)
		{
			if(NULL != prefix)
			{
				if(pnode->bn_key[i+1] != 0)
				{
					sprintf(sub_prefix,"%s│      ", prefix);
				}
				else
				{
					sprintf(sub_prefix,"%s       ", prefix);
				}
			}
			else
			{
				if(pnode->bn_key[i+1] != 0)
				{
					sprintf(sub_prefix,"│      ");
				}
				else
				{
					sprintf(sub_prefix,"       ");
				}
			}
			print_btree(pnode->bn_pointer[i],sub_prefix,len+8);
		}
	}
}
#endif

void print_prompt()
{
	log_debug("Insert node please press 'i'!\n");
	log_debug("Delete node please press 'd'!\n");
	log_debug("Print tree please press 'p'!\n");
	log_debug("Get prompt please press '?'!\n");
	log_debug("Quit this program please press 'q'!\n");
}

void print_eof()
{
	log_debug("#input order[insert/del/print/?/quit]:");
}

int main()
{
	char contor;
	unsigned int new_id= 0;
	bt_init();
	print_prompt();
	print_eof();
	while(1)
	{
		setbuf(stdin, NULL);
		scanf("%c", &contor);
		setbuf(stdin, NULL);
		switch (contor)
		{
			case 'i':
				//insert
				log_debug("\n please input node id which you want insert:");
				setbuf(stdin, NULL);
				scanf("%d", &new_id);
				setbuf(stdin, NULL);
				bt_insert(&g_btree.bpt_root,new_id);
				if(BTREE_FAIL == bt_checkBtree(&g_btree.bpt_root))
					log_debug("B+ Tree is invalid!\r");
				print_eof();
				break;
			case 'd':
				//delete
				log_debug("\n please input node id which you want delete:");
				setbuf(stdin, NULL);
				scanf("%d", &new_id);
				setbuf(stdin, NULL);
				bt_delete(&g_btree.bpt_root,new_id);
				if(BTREE_FAIL == bt_checkBtree(&g_btree.bpt_root))
					log_debug("B+ Tree is invalid!\r");
				print_eof();
				break;
			case 'q':
				//quit
				log_debug("it's time to quit!\n");
				return 0;
			case '?':
				//help
				print_prompt();
				break;
			case 'p':
				//print
				log_debug("***print tree list as follow*****\n");
				print_btree(&g_btree.bpt_root, NULL, 0);
				print_eof();
				break;
			default:
				break;
		};
	}
	return 0;
}
