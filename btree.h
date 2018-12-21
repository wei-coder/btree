//B+树的数据结构定义

#define BTREE_KEY_NUM	4

#define BTREE_KEY_FULL	-1
#define BTREE_KEY_EXIST	0
#define BTREE_KEY_SUC	1

typedef struct btree_node
{
	struct btree_node * bn_pointer[BTREE_KEY_NUM];
	unsigned int bn_key[BTREE_KEY_NUM];
	struct btree_node * bn_next;
	struct btree_node * bn_parent;
	int bn_parentidx;//父节点指向本节点的索引
}btnode_t;

typedef struct bplus_tree
{
	btnode_t bpt_root;
	unsigned int bpt_nodenum;
	btnode_t * bpt_start;
}bpt_t;
