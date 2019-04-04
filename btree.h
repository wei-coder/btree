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


#define BT_PTR struct btree_node_s *
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
	btlvl_t bts_level;		//层级
	int 	bts_num;		//key数量,此处表示本节点已经存储的key数量
	BT_PTR 	bts_left;		//右子树
	BT_PTR 	bts_right;		//左子树
}bts_t;

typedef struct btree_key_s
{
	BT_KEY bn_key;	//key
}btk_t;

typedef struct btree_pointer_s
{
	BT_PTR bn_ptr;	//指针
}btp_t;

typedef struct btree_node_s
{
	bts_t 	bn_sblock;			//节点元信息
	void *	bn_entry;			//节点数据表项
}btnode_t;

typedef struct bplus_tree_s
{
	btnode_t *	bpt_root;		//根节点
	btlvl_t		bpt_level;		//层级
	int			bpt_keynums;	//存储的key的个数
}bpt_t;
