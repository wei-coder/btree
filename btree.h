//B+�������ݽṹ����

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
	btlvl_t bts_level;		//���ڵ��level
	int 	bts_num;		//���ڵ����Чkey����
}bts_t;

typedef struct btree_key_s
{
	BT_KEY bn_key;	//�ؼ���
}btk_t;

typedef struct btree_pointer_s
{
	void * bn_ptr;	//����ָ��
}btp_t;

typedef struct btree_leaf_s
{
	struct list_head lnode;		//��������Ҷ�ӽڵ������
	bts_t 	bn_sblock;			//���ڵ��Ԫ��Ϣ
	btk_t	bn_data[];			//Ҷ�ӽڵ����������
}btleaf_t;

typedef struct btree_index_s
{
	btk_t	bn_key;
	btp_t	bn_ptr;
}bti_t;

typedef struct btree_inter_s
{
	bts_t	bn_sblock;			//���ڵ��Ԫ��Ϣ
	bti_t	bn_entry[];			//�м�ڵ��Ҷ����Ϣ
}btinter_t;

typedef union
{
	btleaf_t	bn_leaf;		//Ҷ�ӽڵ�
	btinter_t	bn_inter;		//�м�ڵ�
}btnode_t;

#define BT_PTR (btnode_t *)

typedef struct bplus_tree_s
{
	btnode_t *	bpt_root;		//���ĸ��ڵ�ָ��
	btlvl_t		bpt_level;		//�����ܲ㼶
	int			bpt_keynums;	//�����ܹؼ�����
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
	btk_t updkey;		//ԭ�ڵ����key
	bti_t newidx;		//�½ڵ��������Ϣ
}spt_info_t;

typedef struct
{
	btnode_t * pNode;
	int idx;
}btpath_t;
