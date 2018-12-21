//B+树的功能实现

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bpt_t g_btree = {0};

int bt_insert_key(btnode_t * pnode, unsigned int key)
{
	if(pnode->bn_key[BTREE_KEY_NUM-1] != 0)
	{
		return BTREE_KEY_FULL;//表示元素已满
	}
	unsigned int tmpkey = 0;
	for(int j = 0; j<BTREE_KEY_NUM; j++)
	{
		tmpkey = pnode->bn_key[j];
		if(key > tmpkey)
		{
			continue;
		}
		else if(key == tmpkey)
		{
			return BTREE_KEY_EXIST;
		}
		else
		{
			pnode->bn_key[j] = key;
			key = tmpkey;
			if(0 == key)
			{
				break;
			}
		}
	}
	return BTREE_KEY_SUC;
}

btnode_t * bt_splitNode(btnode_t * pNode)
{
	btnode_t * newleaf = (btnode_t *)malloc(sizeof(btnode_t));
	newleaf->bn_next = pNode->bn_next;
	newleaf->bn_parent = pNode->bn_parent;
	pNode->bn_next = newleaf;
	memcpy(newleaf->bn_key,pNode->bn_key+BTREE_KEY_NUM/2,(BTREE_KEY_NUM/2)*sizeof(unsigned int));
	memcpy(newleaf->bn_pointer, pNode->bn_pointer+BTREE_KEY_NUM/2,(BTREE_KEY_NUM/2)*sizeof(btnode_t *));
	memset(pNode->bn_key+BTREE_KEY_NUM/2,0,(BTREE_KEY_NUM/2)*sizeof(unsigned int));
	memset(pNode->bn_pointer+BTREE_KEY_NUM/2,0,(BTREE_KEY_NUM/2)*sizeof(btnode_t *));
	return newleaf;
}


void bt_insert(btnode_t * root, unsigned int key)
{
	unsigned int side_key = 0;
	int index = bt_search_subtree(root,key);
	if(-1 == index)
	{
		if(0 == root->bn_key[BTREE_KEY_NUM-1])
		{
			int i = 0;
			while(0 != root->bn_key[i])
			{
				i++;
			}
			root->bn_key[i] = key;
		}
		root->bn_key[BTREE_KEY_NUM-1] = key;
	}
	
	btnode_t * pnode = bt_search(root,key);

	int ret = bt_insert_key(pnode,key);
	if(BTREE_KEY_FULL == ret)
	{
		//如果本叶节点已经占满，则需要进行分裂插入
		btnode_t * newnode = bt_splitNode(pnode);
		btnode_t * tmpnode = newnode;
		btnode_t * parent = pnode->bn_parent;
		side_key = parent->bn_key;
		if(key <= pnode->bn_key[BTREE_KEY_NUM/2-1])
		{
			tmpnode = pnode;
		}
		bt_insert_key(tmpnode,key);	//分裂后直接插入,成功或已经存在
		while(NULL != parent)
		{
			key = side_key;
			if(BTREE_KEY_FULL = bt_insert_key(parent,key))
			{
				newnode = bt_splitNode(parent);
				tmpnode = newnode;
				side_key = parent->bn_key[BTREE_KEY_NUM/2-1];
				if(key < parent->bn_key[BTREE_KEY_NUM/2-1])
				{
					tmpnode = parent;
				}
				bt_insert_key(tmpnode,key);
				parent = parent->bn_parent;
			}
			else
			{
				break;
			}
		}
	}
	else if(BTREE_KEY_EXIST == ret)
	{
		printf("this node is exist!\r\n");
	}
}

int bt_search_subtree(btnode_t * root, unsigned int key)
{
	for(int i=0; i<BTREE_KEY_NUM; i++)
	{
		if(key <= root->bn_key[i])
		{
			return i;
		}
	}
	return -1;
}

btnode_t * bt_search(btnode_t * root, unsigned int key)
{
	int index = bt_search_subtree(root,key);
	if(-1 = index)
	{
		return NULL;
	}
	btnode_t * sub = root->bn_pointer[index];
	btnode_t * ret = NULL;
	while(sub)
	{
		ret = sub;
		index = bt_search_subtree(sub->bn_pointer[index],key);
		sub = sub->bn_pointer[index];
	}
	return ret;
}

void bt_delete(btnode_t * root, unsigned int key)
{

}

void bt_init()
{
	g_btree.bpt_nodenum = 0;
	g_btree.bpt_start = NULL;
	memset(g_btree.bpt_root.bn_key,0, BTREE_KEY_NUM*sizeof(int));
	g_btree.bpt_root.bn_key[0] = 1;
	g_btree.bpt_root.bn_key[1] = 2;
	memset(g_btree.bpt_root.bn_pointer,0, BTREE_KEY_NUM*sizeof(struct btree_node *));
	g_btree.bpt_root.bn_next = NULL;
}

void print_btree(btnode_t * pnode, int blank_offset)
{
	char *blank_str = malloc(blank_offset);
	memset(blank_str,' ',blank_offset-1);
	blank_str[blank_offset-1] = 0;
	for(int i=0; i<BTREE_KEY_NUM; i++)
	{
		if(pnode->bn_pointer[i] != NULL)
		{
			printf("%s%d", blank_str,pnode->bn_key[i]);
			print_btree(pnode->bn_pointer[i],blank_offset+8);
		}
		break;
	}
	free(blank_str);
}

void print_prompt()
{
	printf("Insert node please press 'i'!\n");
	printf("Delete node please press 'd'!\n");
	printf("Print tree please press 'p'!\n");
	printf("Get prompt please press '?'!\n");
	printf("Quit this program please press 'q'!\n");
}

void main()
{
	char contor;
	unsigned int new_id= 0;
	bt_init();
	print_prompt();
	while (true)
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
				bt_insert();
				break;
			case 'd':
				//delete
				printf("\n please input node id which you want delete:");
				setbuf(stdin, NULL);
				scanf("%d", &new_id);
				setbuf(stdin, NULL);
				bt_delete();
				break;
			case 'q':
				//quit
				printf("it's time to quit!\n");
				return;
			case '?':
				//help
				print_prompt();
				break;
			case 'p':
				//print
				printf("***print tree list as follow*****\n");
				print_btree();
				break;
			default:
				break;
		};
	}
}
