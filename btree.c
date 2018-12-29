//B+树的功能实现

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "btree.h"

bpt_t g_btree = {0};

btnode_t * bt_splitNode(btnode_t * pNode)
{
	btnode_t * newleaf = (btnode_t *)malloc(sizeof(btnode_t));
	newleaf->bn_next = pNode->bn_next;
	newleaf->bn_prev = pNode;
	if(NULL != pNode->bn_next)
	{
		pNode->bn_next->bn_prev = newleaf;
	}
	pNode->bn_next = newleaf;
	newleaf->bn_parent = pNode->bn_parent;
	newleaf->bn_parentidx = pNode->bn_parentidx;
	memcpy(newleaf->bn_key,pNode->bn_key+BTREE_KEY_NUM/2,(BTREE_KEY_NUM/2)*sizeof(unsigned int));
	memcpy(newleaf->bn_pointer, pNode->bn_pointer+BTREE_KEY_NUM/2,(BTREE_KEY_NUM/2)*sizeof(btnode_t *));
	memset(pNode->bn_key+BTREE_KEY_NUM/2,0,(BTREE_KEY_NUM/2)*sizeof(unsigned int));
	memset(pNode->bn_pointer+BTREE_KEY_NUM/2,0,(BTREE_KEY_NUM/2)*sizeof(btnode_t *));
	return newleaf;
}

//向pNode插入key，以及key对应的子节点指针,当节点分裂时，向父节点插入key，pointer
int bt_insert_key(btnode_t * pNode, unsigned int key, btnode_t * pointer)
{
	int iMaxIdx = 0;
	int iMaxIdx_Parent = 0;
	int iMaxKey = 0;
	btnode_t * pNewLeaf = NULL;
	btnode_t * pTargetSub = pNode;
	if(NULL == pNode)
	{
		return BTREE_FAIL;
	}
	if(pNode->bn_key[BTREE_KEY_NUM-1] != 0)	//已满
	{
		pNewLeaf = bt_splitNode(pNode);
		if(key > pNode->bn_key[BTREE_KEY_NUM/2-1])
		{
			pTargetSub = pNewLeaf;
		}
		pNode->bn_parent->bn_key[pNode->bn_parentidx] = pNode->bn_key[BTREE_KEY_NUM/2-1];
		iMaxKey = bt_GetMaxKey(pNewLeaf,&iMaxIdx);
		iMaxKey = iMaxKey>key?iMaxKey:key;
		bt_insert_key(pNewLeaf->bn_parent, iMaxKey, pNewLeaf);	//向父节点插入key
		bt_insert_key(pTargetSub, key, pointer);//向分裂的新节点插入key，pointer
	}
	else									//未满
	{
		unsigned int tmpkey = 0;
		btnode_t * tmpPointer = NULL;
		int j = 0;
		for(j = 0; j<BTREE_KEY_NUM; j++)
		{
			tmpkey = pNode->bn_key[j];
			tmpPointer = pNode->bn_pointer[j];
			if((0 != tmpkey)&&(key > tmpkey))
			{
				continue;
			}
			pNode->bn_key[j] = key;
			pNode->bn_pointer[j] = pointer;
			if(NULL != pNode->bn_pointer[j])
			{
				pNode->bn_pointer[j]->bn_parentidx = j;
			}
			key = tmpkey;
			pointer = tmpPointer;
			if(0 == key)
			{
				break;
			}
		}
		iMaxKey = bt_GetMaxKey(pNode, &iMaxIdx);
		while(pNode->bn_parent)
		{
			if(iMaxKey > bt_GetMaxKey(pNode->bn_parent,&iMaxIdx_Parent))
			{
				pNode->bn_parent->bn_key[iMaxIdx_Parent] = iMaxKey;
			}
			pNode = pNode->bn_parent;
		}
	}
	return BTREE_KEY_SUC;
}

int bt_GetMaxKey(btnode_t * pNode, int * idx)
{
	int i = 0;
	int ret = 0;
	for(i=0; i<BTREE_KEY_NUM; i++)
	{
		if(ret < pNode->bn_key[i])
		{
			ret = pNode->bn_key[i];
			*idx = i;
		}
	}
	return ret;
}

//根据key查找最深匹配节点
btnode_t * bt_search(btnode_t * root, unsigned int key)
{
	int i = 0;
	btnode_t * pSub = NULL;
	int index = 0;
	for(i=0; i<BTREE_KEY_NUM; i++)
	{
		if(key <= root->bn_key[i])
		{
			if(NULL != root->bn_pointer[i])//表示该节点为根节点或中间节点
			{
				//继续从子节点查起
				pSub = bt_search(root->bn_pointer[i], key);
				if(NULL != pSub)
				{
					return pSub;
				}
			}
			return root;
		}
	}
	return NULL;
}

//找到最大叶节点及其索引
btnode_t * bt_search_max(btnode_t * pNode, int *index)
{
	int i = 0;
	btnode_t * pSub = NULL;
	for(i=0; i<BTREE_KEY_NUM; i++)
	{
		if(0 == pNode->bn_key[i])
		{
			break;
		}
	}
	if(NULL != pNode->bn_pointer[i-1])//不是叶节点
	{
		//递归查找其子节点
		return bt_search_max(pNode->bn_pointer[i-1], index);
	}
	//表示该节点就是叶节点，返回最大key值对应的索引
	*index = i-1;
	return pNode;
}

//向B+树插入节点
void bt_insert(btnode_t * root, unsigned int key)
{
	unsigned int side_key = 0;
	int ret = 0;

	btnode_t *	pMaxMatchNode = bt_search(root,key);//最深匹配节点
	btnode_t *	pMaxLeaf = NULL;					//最大叶节点
	int			iMaxIdx =	0;						//最大叶节点的最大索引

	if(NULL == pMaxMatchNode)						//表示没有找到，根节点需要添加key
	{
		pMaxMatchNode = root;
	}

	pMaxLeaf = bt_search_max(pMaxMatchNode,&iMaxIdx);
	bt_insert_key(pMaxLeaf,key,NULL);
}

void bt_delete(btnode_t * root, unsigned int key)
{
	btnode_t * sub = bt_search(root,key);
	if(NULL == sub)
	{
		return;
	}
	btnode_t * parent = sub->bn_parent;
	parent->bn_pointer[sub->bn_parentidx] = NULL;
	parent->bn_key[sub->bn_parentidx] = 0;
	sub->bn_prev->bn_next = sub->bn_next;
	sub->bn_next->bn_prev = sub->bn_prev;
	free(sub);
}

void bt_init()
{
	//根节点
	g_btree.bpt_nodenum = 0;
	g_btree.bpt_start = NULL;
	g_btree.bpt_root.bn_next = NULL;
	g_btree.bpt_root.bn_parent = NULL;
	g_btree.bpt_root.bn_parentidx = 0;
	g_btree.bpt_root.bn_prev = NULL;
	memset(g_btree.bpt_root.bn_key,0, BTREE_KEY_NUM*sizeof(int));
	g_btree.bpt_root.bn_key[0] = 10;
	memset(g_btree.bpt_root.bn_pointer,0, BTREE_KEY_NUM*sizeof(struct btree_node *));
	g_btree.bpt_root.bn_pointer[0] = (btnode_t *)malloc(sizeof(btnode_t));

	//中间节点
	memset(g_btree.bpt_root.bn_pointer[0],0, sizeof(struct btree_node *));
	g_btree.bpt_root.bn_pointer[0]->bn_next = NULL;
	g_btree.bpt_root.bn_pointer[0]->bn_prev = NULL;
	g_btree.bpt_root.bn_pointer[0]->bn_parent = &g_btree.bpt_root;
	g_btree.bpt_root.bn_pointer[0]->bn_parentidx = 0;
	g_btree.bpt_root.bn_pointer[0]->bn_key[0] = 5;
	g_btree.bpt_root.bn_pointer[0]->bn_pointer[0] = (btnode_t *)malloc(sizeof(btnode_t));

	//叶节点
	memset(g_btree.bpt_root.bn_pointer[0]->bn_pointer[0],0, sizeof(struct btree_node *));
	g_btree.bpt_root.bn_pointer[0]->bn_pointer[0]->bn_next = NULL;
	g_btree.bpt_root.bn_pointer[0]->bn_pointer[0]->bn_prev = NULL;
	g_btree.bpt_root.bn_pointer[0]->bn_pointer[0]->bn_parent = g_btree.bpt_root.bn_pointer[0];
	g_btree.bpt_root.bn_pointer[0]->bn_pointer[0]->bn_parentidx = 0;
	g_btree.bpt_root.bn_pointer[0]->bn_pointer[0]->bn_key[0] = 1;
}

#if 0
void read_tree(char * filename)
{
	struct stat stat_len = {0};
	FILE* treef = fopen(filename,"b+");
	fstat(treef, &stat_len);
	char * buf = mmap(NULL,stat_len.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, treef, 0);
	char line[10] = {0};
	int index = 0 ;
	while(index < stat_len.st_size)
	{
		int i = 0;
		while(buf[index] != '\r')
		{
			if(buf[index] != ' ')
			{
				line[i] = buf[index];
				i++;
			}
			else
			{
				if(i>0)
				{
					int key = atoi(line);
				}
			}
			index ++;
		}
		
	}
}
#endif

void print_btree(btnode_t * pnode, char * prefix, int len)
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
				printf("%s├──────%d\r\n",prefix, pnode->bn_key[i]);
			}
			else
			{
				printf("%s└──────%d\r\n",prefix, pnode->bn_key[i]);
			}
		}
		else
		{
			printf("%d\r\n",pnode->bn_key[i]);
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
			print_btree(pnode->bn_pointer[i],sub_prefix,len+8);
		}
	}
}

void print_prompt()
{
	printf("Insert node please press 'i'!\n");
	printf("Delete node please press 'd'!\n");
	printf("Print tree please press 'p'!\n");
	printf("Get prompt please press '?'!\n");
	printf("Quit this program please press 'q'!\n");
}

int main()
{
	char contor;
	unsigned int new_id= 0;
	bt_init();
	print_prompt();
	while(1)
	{
		setbuf(stdin, NULL);
		scanf("%c", &contor);
		setbuf(stdin, NULL);
		switch (contor)
		{
			case 'i':
				//insert
				printf("\n please input node id which you want insert:");
				setbuf(stdin, NULL);
				scanf("%d", &new_id);
				setbuf(stdin, NULL);
				bt_insert(&g_btree.bpt_root,new_id);
				break;
			case 'd':
				//delete
				printf("\n please input node id which you want delete:");
				setbuf(stdin, NULL);
				scanf("%d", &new_id);
				setbuf(stdin, NULL);
				bt_delete(&g_btree.bpt_root,new_id);
				break;
			case 'q':
				//quit
				printf("it's time to quit!\n");
				return 0;
			case '?':
				//help
				print_prompt();
				break;
			case 'p':
				//print
				printf("***print tree list as follow*****\n");
				print_btree(&g_btree.bpt_root, NULL, 0);
				break;
			default:
				break;
		};
	}
	return 0;
}
