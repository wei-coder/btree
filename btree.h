//B+树的数据结构定义

#define BTREE_KEY_NUM	10

#define BTREE_KEY_FULL	2
#define BTREE_KEY_EXIST	0
#define BTREE_KEY_SUC	1
#define BTREE_FAIL		-1
#define BTREE_OK		0

#define list_entry(ptr, type, member) \
        ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)

struct list_head {
        struct list_head *prev, *next;
};

static inline void list_init(struct list_head *link)
{
        link->prev = link;
        link->next = link;
}

static inline void
__list_add(struct list_head *link, struct list_head *prev, struct list_head *next)
{
        link->next = next;
        link->prev = prev;
        next->prev = link;
        prev->next = link;
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
        prev->next = next;
        next->prev = prev;
}

static inline void list_add(struct list_head *link, struct list_head *prev)
{
        __list_add(link, prev, prev->next);
}

static inline void list_add_tail(struct list_head *link, struct list_head *head)
{
	__list_add(link, head->prev, head);
}

static inline void list_del(struct list_head *link)
{
        __list_del(link->prev, link->next);
        list_init(link);
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}


#define BT_KEY unsigned long

typedef enum
{
	BT_LVL_LEAF = 1,
	BT_LVL_INT,
	BT_LVL_ROOT,
	BT_LVL_MAX
}btlvl_t;

typedef struct btree_sblock_s
{
	btlvl_t bts_level;		//本节点的level
	int 	bts_num;		//本节点的有效key数量
}bts_t;

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
	bts_t 	bn_sblock;			//本节点的元信息
	btk_t	bn_data[];			//叶子节点的数据内容
}btleaf_t;

typedef struct btree_index_s
{
	btk_t	bn_key;
	btp_t	bn_ptr;
}bti_t;

typedef struct btree_inter_s
{
	bts_t	bn_sblock;			//本节点的元信息
	bti_t	bn_entry[];			//中间节点的叶子信息
}btinter_t;

typedef union
{
	btleaf_t	bn_leaf;		//叶子节点
	btinter_t	bn_inter;		//中间节点
}btnode_t;

#define BT_PTR (btnode_t *)

typedef struct bplus_tree_s
{
	btnode_t *	bpt_root;		//树的根节点指针
	btlvl_t		bpt_level;		//树的总层级
	int			bpt_keynums;	//树的总关键字树
}bptree_t;

#define NODE_SIZE	512
#define INTER_KEY_NUM	((NODE_SIZE-sizeof(bts_t))/(sizeof(btk_t)+sizeof(btp_t)))
#define LEAF_KEY_NUM	((NODE_SIZE-sizeof(bts_t)-sizeof(struct list_head))/(sizeof(btk_t)))

typedef enum
{
	IST_OK = 0,
	IST_SPLIT,
	IST_ROTATE,
	IST_FAIL
}istv_t;		//insert return value

typedef struct
{
	btk_t updkey;
	btk_t rightkey;
	btk_t leftkey;
}rot_info_t;

typedef struct
{
	btk_t updkey;		//原节点的新key
	bti_t newidx;		//新节点的索引信息
}spt_info_t;

typedef struct
{
	btnode_t * pNode;
	int idx;
}btpath_t;
