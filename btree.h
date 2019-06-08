//B+树的数据结构定义

#ifndef __BTREE_H_
#define __BTREE_H_

#include "vector.h"

#pragma pack()

#define BTREE_KEY_NUM	10

#define BTREE_KEY_FULL	2
#define BTREE_KEY_EXIST	0
#define BTREE_KEY_SUC	1
#define BTREE_FAIL		-1
#define BTREE_OK		0

#define BT_KEY unsigned long

typedef enum
{
	BT_LVL_LEAF = 1,
	BT_LVL_INT,
	BT_LVL_ROOT
}btlvl_t;

typedef enum
{
	TYPE_LEAF = 1,
	TYPE_INT
}btype_t;

typedef struct btree_key_s
{
	BT_KEY bn_key;	//关键字
}btk_t;

typedef struct btree_pointer_s
{
	void * bn_ptr;	//子树指针
}btp_t;

typedef struct btree_leaf_s
{
	struct list_head lnode;		//用于连接叶子节点的链表
	int 	bn_num;				//本节点的有效key数量
	btk_t	bn_data[];			//叶子节点的数据内容
}btleaf_t;

typedef struct btree_index_s
{
	btk_t	bn_key;
	btp_t	bn_ptr;
}bti_t;

typedef struct btree_inter_s
{
	int 	bn_num;				//本节点的有效key数量
	bti_t	bn_entry[];			//中间节点的叶子信息
}btinter_t;

typedef struct bptree_node_s
{
	btlvl_t 		bn_level;		//本节点的level
	btype_t			bn_type;		//本节点的类型
	union{
		btleaf_t	bn_leaf;		//叶子节点
		btinter_t	bn_inter;		//中间节点
	};
}btnode_t;

#define BT_PTR (btnode_t *)

typedef struct bplus_tree_s
{
	btnode_t *	bpt_root;			//树的根节点指针
	int			bpt_level;			//树的总层级
	int			bpt_branch_nums;	//树的最大叶子数
	int			bpt_leaf_nums;		//叶子节点的最大表项数
}bptree_t;

#define NODE_SIZE	128
#define INTER_KEY_NUM	((NODE_SIZE-sizeof(btlvl_t)-sizeof(btype_t)-sizeof(int))/sizeof(bti_t))
#define LEAF_KEY_NUM	((NODE_SIZE-sizeof(btlvl_t)-sizeof(btype_t)-sizeof(int)-sizeof(struct list_head))/sizeof(btk_t))

typedef enum
{
	MOD_OK = 0,
	MOD_NOK,
	MOD_FAIL
}modv_t;		//delete return value

typedef enum
{
	ROT_DEL = 1,
	ROT_ADD
}rot_t;		//rotate mode

typedef enum
{
	LEFT = 0,
	MID,
	RIGHT,
	ORDNUM
}ndflag_t;		//node flag for rotate/merge

typedef struct
{
	int leftkey;
	int midkey;
	int rightkey;
	ndflag_t invalidnode;
	bti_t newnode;
}modinfo_t;

typedef struct
{
	btk_t updkey;		//update key
	bti_t newidx;		//split new entry
}spt_info_t;

typedef struct
{
	btnode_t * pleft;			//左节点
	btnode_t * plfather;		//左子节点的父节点
	int leftidx;				//左子节点的索引
	btnode_t * pmid;			//子节点
	int mididx;					//子节点索引
	btnode_t * pright;			//右节点
	btnode_t * prfather;		//右子节点的父节点
	int rightidx;				//右子节点的索引
}btpath_t;

#endif
