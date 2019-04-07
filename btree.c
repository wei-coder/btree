//B+树的功能实现

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "btree.h"

bpt_t g_btree = {0};

#define log_debug(msg,...)  printf(msg, ##__VA_ARGS__)
#define new(type, num)	(((type) *)malloc(sizeof(type)*(num))

/*alloc a new node,if it's a leaf node,*/
btnode_t * bt_new_node(btlvl_t level){
	int size = 0;
	btnode_t * pNode = (btnode_t *)new(char, NODE_SIZE);
	if(NULL == pNode){
		log_debug("Malloc new Node for B+Tree failed, please check!\n");
		return NULL;
	}
	memset(pNode, 0, NODE_SIZE);
	
	if(BT_LVL_LEAF == level){
		pNode->bn_leaf.bn_sblock.bts_level = BT_LVL_LEAF;
		list_init(&pNode->bn_leaf.lnode);
	}
	else{
		pNode->bn_inter.bn_sblock.bts_level = level;
	}

	return pNode;
}

/*compare key*/
int bt_key_cmp(btk_t key1, btk_t key2){
	if(key1.bn_key > key2.bn_key)
		return 1;
	else if(key1.bn_key == key2.bn_key)
		return 0;
	else
		return -1;
}

/*add array elem for leaf node*/
int bt_branch_real_add(btinter_t * pinter, bti_t idx){
	bti_t * tmpidxs = pinter->bn_entry;
	int i,j;
	for(i=0; i<pinter->bn_sblock.bts_num; i++){
		if(bt_key_cmp(idx.bn_key, tmpidxs[i].bn_key)>0)
			continue;
		else{
			break;
		}
	}

	for(j=pinter->bn_sblock.bts_num+1; j>i; j--){
		tmpidxs[j] = tmpidxs[j-1];
	}
	tmpidxs[j] = idx;
	pinter->bn_sblock.bts_num++;
	return BTREE_OK;
}

/*add array elem for inter node*/
int bt_branch_add(btnode_t * pNode, bti_t idx, spt_info_t * psplit, rot_info_t * prot){
	if(pNode->bn_inter.bn_sblock.bts_num >= LEAF_KEY_NUM){
		btnode_t * pnewNode = NULL;
		if(BTREE_FAIL == bt_leaf_split(pNode, &pnewNode)){
			return IST_FAIL;
		}
		if(NULL == pnewNode){
			return IST_FAIL;
		}
		int ret = bt_key_cmp(pNode->bn_inter.bn_entry[pNode->bn_inter.bn_sblock.bts_num-1].bn_key, key);
		if(ret > 0){
			bt_branch_real_add(&pNode->bn_inter, idx);
		}
		else{
			bt_branch_real_add(&pnewNode->bn_inter, idx)
		}
		psplit->updkey = pNode->bn_inter.bn_data[pNode->bn_inter.bn_sblock.bts_num-1];
		psplit->newidx = pnewNode->bn_inter.bn_entry[pnewNode->bn_inter.bn_sblock.bts_num-1];
		return IST_SPLIT;
	}

	int oldnum = pNode->bn_leaf.bn_sblock.bts_num;
	bt_branch_real_add(&pNode->bn_leaf, idx);
	if(pNode->bn_inter.bn_sblock.bts_num-oldnum < 1){
		log_debug("Add data num:%d less than elem num[%d] you want\n",pNode->bn_inter.bn_sblock.bts_num-oldnum, num);
		return BTREE_FAIL;
	}
	return BTREE_OK;
}

/*del array elem for inter node*/
int bt_branch_del(btnode_t * pNode, void * elems, int num){
	if(pNode->bn_inter.bn_sblock.bts_num-num < INTER_KEY_NUM/2)
		return pNode->bn_inter.bn_sblock.bts_num-num;

	int oldnum = pNode->bn_inter.bn_sblock.bts_num;
	bti_t * tmpidxs = (bti_t *)pNode->bn_inter.bn_entry;
	int i,j;
	bti_t * idxs = (bti_t *)elems;
	for(i=0; i<num; i++){
		for(j=0; j<pNode->bn_inter.bn_sblock.bts_num; j++){
			if(bt_key_cmp(idxs[i].bn_key, tmpidxs[j].bn_key) != 0)
				continue;
			else{
				int x;
				for(x=j; x<pNode->bn_inter.bn_sblock.bts_num; x++){
					if(INTER_KEY_NUM-1 == x)
						tmpidxs[x] = {0};
					else
						tmpidxs[x] = tmpidxs[x+1];
				}
				pNode->bn_inter.bn_sblock.bts_num--;
			}
		}
	}
	if(oldnum-pNode->bn_inter.bn_sblock.bts_num < num){
		log_debug("Del branch num:%d less than elem num[%d] you want\n",oldnum-pNode->bn_inter.bn_sblock.bts_num, num);
		return BTREE_FAIL;
	}
	return BTREE_OK;
}

/*leaf node be split to two node*/
int bt_leaf_split(btnode_t * pNode, btnode_t ** ppNewNode, spt_info_t * pinfo){
	if(LEAF_KEY_NUM > pNode->bn_leaf.bn_sblock.bts_num){
		log_debug("The leaf no need split!\n");
		return BTREE_FAIL;
	}
	btnode_t * ptmp = bt_new_node(pNode->bn_leaf.bn_sblock.bts_level);
	if(NULL == ptmp)
		return BTREE_FAIL;
	ptmp->bn_leaf.lnode.next = pNode->bn_leaf.lnode.next;
	ptmp->bn_leaf.lnode.prev = &pNode->bn_leaf.lnode;
	pNode->bn_leaf.lnode.next = &ptmp->bn_leaf.lnode;
	memcpy(ptmp->bn_leaf.bn_data, pNode->bn_leaf.bn_data+LEAF_KEY_NUM/2, sizeof(btk_t)*(LEAF_KEY_NUM-LEAF_KEY_NUM/2));
	pNode->bn_leaf.bn_sblock.bts_num = LEAF_KEY_NUM;
	ptmp->bn_leaf.bn_sblock.bts_num = LEAF_KEY_NUM-LEAF_KEY_NUM/2;
	memset(pNode->bn_leaf.bn_data+LEAF_KEY_NUM/2, 0, sizeof(btk_t)*(LEAF_KEY_NUM-LEAF_KEY_NUM/2));
	*ppNewNode = ptmp;
	return BTREE_OK;
}

static inline void bt_leaf_move2left(btleaf_t * pleaf, btleaf_t * pleft, int move_num){
	memcpy(pleft->bn_data+pleft->bn_sblock.bts_num, pleaf->bn_data, sizeof(btk_t)*move_num);
	pleft->bn_sblock.bts_num += move_num;
	int i;
	for(i=0; i<(pleaf->bn_sblock.bts_num - move_num); i++){
		pleaf->bn_data[i] = pleaf->bn_data[i+move_num];
	}
	pleaf->bn_sblock.bts_num -= move_num;
}

static inline void bt_leaf_move2right(btleaf_t * pleaf, btleaf_t * pright, int move_num){
	int idx = pleaf->bn_sblock.bts_num-move_num-1;
	int i;
	for(i=pright->bn_sblock.bts_num; i>0; i--){
		pright->bn_data[i+move_num] = pright->bn_data[i];
	}
	memcpy(pright->bn_data, pleaf->bn_data+idx, sizeof(btk_t)*move_num);
	pleaf->bn_sblock.bts_num -= move_num;
}

/*leaf node and brother node rotate*/
int bt_leaf_rotate(btnode_t * pNode, rot_info_t * pinfo){
	btleaf_t * pleft = (btleaf_t *)pNode->bn_leaf.lnode.prev;
	btleaf_t * pright = (btleaf_t *)pNode->bn_leaf.lnode.next;
	int lnew_num = 0;
	int rnew_num = 0;
	int new_num = 0;
	if((pleft->bn_sblock.bts_num < LEAF_KEY_NUM)&&
		(pright->bn_sblock.bts_num < LEAF_KEY_NUM)){
		lnew_num = (pleft->bn_sblock.bts_num+pright->bn_sblock.bts_num+LEAF_KEY_NUM+1)/3;
		rnew_num = lnew_num;
		new_num = pleft->bn_sblock.bts_num + pright->bn_sblock.bts_num + LEAF_KEY_NUM + 1 - lnew_num - lnew_num;
	}
	else if(pleft->bn_sblock.bts_num < LEAF_KEY_NUM){
		lnew_num = (pleft->bn_sblock.bts_num + LEAF_KEY_NUM + 1)/2;
		rnew_num = pright->bn_sblock.bts_num;
		new_num = pleft->bn_sblock.bts_num + LEAF_KEY_NUM + 1 - lnew_num;
	}
	else if(pright->bn_sblock.bts_num < LEAF_KEY_NUM){
		lnew_num = pleft->bn_sblock.bts_num;
		rnew_num = (pright->bn_sblock.bts_num + LEAF_KEY_NUM + 1)/2;
		new_num = pright->bn_sblock.bts_num + LEAF_KEY_NUM + 1 - rnew_num;
	}
	else{
		log_debug("brother node have no free entry,please check!\n");
		return BTREE_FAIL;
	}
	int i;
	int move_num = 0;
	int idx = 0;
	/*move to both brother node*/
	if((lnew_num > pleft->bn_sblock.bts_num)&&
		(rnew_num > pright->bn_sblock.bts_num){
		move_num = lnew_num-pleft->bn_sblock.bts_num;
		bt_leaf_move2left(&pNode->bn_leaf, pleft, move_num);
		move_num = rnew_num-pright->bn_sblock.bts_num;
		bt_leaf_move2right(&pNode->bn_leaf, pright, move_num);
		return BTREE_OK;
	}
	/*move to only left brother node*/
	else if(lnew_num > pleft->bn_sblock.bts_num){
		move_num = lnew_num-pleft->bn_sblock.bts_num;
		bt_leaf_move2left(&pNode->bn_leaf, pleft, move_num);
		if((rnew_num < pright->bn_sblock.bts_num) &&
			(new_num > pNode->bn_leaf.bn_sblock.bts_num)){
			move_num = pNode->bn_leaf.bn_sblock.bts_num - new_num;
			if(move_num == pright->bn_sblock.bts_num -rnew_num){
				bt_leaf_move2left(pright, &pNode->bn_leaf, move_num);
				return BTREE_OK;
			}
			else{
				log_debug("Rotate num caculate wrong! rnew_num move %d to leaf, but leaf need %d\n",
					pright->bn_sblock.bts_num - rnew_num, move_num);
				return BTREE_FAIL;
			}
		}
	}
	/*move to only right brother node*/
	else if(rnew_num > pright->bn_sblock.bts_num){
		move_num = rnew_num-pright->bn_sblock.bts_num;
		bt_leaf_move2right(&pNode->bn_leaf, pright, move_num);
		if((lnew_num < pleft->bn_sblock.bts_num) &&
			(new_num > pNode->bn_leaf.bn_sblock.bts_num)){
			move_num = new_num - pNode->bn_leaf.bn_sblock.bts_num;
			if(move_num == pleft->bn_sblock.bts_num - lnew_num){
				bt_leaf_move2left(pleft, &pNode->bn_leaf, move_num);
				return BTREE_OK;
			}
			else{
				log_debug("Rotate num caculate wrong! lnew_num move %d to leaf, but leaf need %d\n",
					pleft->bn_sblock.bts_num - lnew_num, move_num);
				return BTREE_FAIL;
			}
		}
	}
	else{
		log_debug("Rotate num calculate wrong, please check!\n");
		return BTREE_FAIL;
	}
}

/*add array elem for leaf node*/
int bt_leaf_real_add(btleaf_t * pleaf, btk_t * pkey){
	btk_t * tmpkeys = pleaf->bn_data;
	btk_t key = *pkey;
	int i,j;
	for(i=0; i<pleaf->bn_sblock.bts_num; i++){
		if(bt_key_cmp(key, tmpkeys[i])>0)
			continue;
		else{
			break;
		}
	}

	for(j=pleaf->bn_sblock.bts_num+1; j>i; j--){
		tmpkeys[j] = tmpkeys[j-1];
	}
	tmpkeys[j] = key;
	pleaf->bn_sblock.bts_num++;
	return BTREE_OK;
}

/*add array elem for leaf node*/
istv_t bt_leaf_add(btnode_t * pNode, btk_t * pkey, spt_info_t * psplit, rot_info_t * protate){
	btnode_t * pleft = NULL;
	btnode_t * pright = NULL;
	if(pNode->bn_leaf.bn_sblock.bts_num >= LEAF_KEY_NUM){
		pleft = (btnode_t *)pNode->bn_leaf.lnode.prev;
		pright = (btnode_t *)pNode->bn_leaf.lnode.next;
		if((pleft->bn_leaf.bn_sblock.bts_num >= LEAF_KEY_NUM)&&
			(pright->bn_leaf.bn_sblock.bts_num >= LEAF_KEY_NUM)){
			btnode_t * pnewNode = NULL;
			if(BTREE_FAIL == bt_leaf_split(pNode, &pnewNode)){
				return IST_FAIL;
			}
			if(NULL == pnewNode){
				return IST_FAIL;
			}
			int ret = bt_key_cmp(pNode->bn_leaf.bn_data[pNode->bn_leaf.bn_sblock.bts_num-1], pkey);
			if(ret > 0){
				bt_leaf_real_add(&pNode->bn_leaf, pkey);
			}
			else{
				bt_leaf_real_add(&pnewNode->bn_leaf, pkey)
			}
			psplit->updkey = pNode->bn_leaf.bn_data[pNode->bn_leaf.bn_sblock.bts_num-1];
			psplit->newidx.bn_key = pnewNode->bn_leaf.bn_data[pnewNode->bn_leaf.bn_sblock.bts_num-1];
			psplit->newidx.bn_ptr = (btp_t)pnewNode;
			return IST_SPLIT;
		}
		else{
			bt_leaf_rotate(pNode, pkey);
			protate->leftkey = pleft->bn_leaf.bn_data[pleft->bn_leaf.bn_sblock.bts_num-1];
			protate->newkey = pNode->bn_leaf.bn_data[pNode->bn_leaf.bn_sblock.bts_num-1];
			protate->rightkey = pright->bn_leaf.bn_data[pright->bn_leaf.bn_sblock.bts_num-1];
			return IST_ROTATE;
		}
	}

	int oldnum = pNode->bn_leaf.bn_sblock.bts_num;
	bt_leaf_real_add(&pNode->bn_leaf, pkey);
	if(pNode->bn_leaf.bn_sblock.bts_num-oldnum < 1){
		log_debug("Add data num:%d less than elem num[%d] you want\n",pNode->bn_leaf.bn_sblock.bts_num-oldnum, num);
		return BTREE_FAIL;
	}
	return BTREE_OK;
}

/*del array elem for leaf node*/
void bt_leaf_del(btnode_t * pNode, void * elems, int num){
	if(pNode->bn_leaf.bn_sblock.bts_num-num < LEAF_KEY_NUM/2)
		return pNode->bn_leaf.bn_sblock.bts_num-num;

	int oldnum = pNode->bn_leaf.bn_sblock.bts_num;
	btk_t * tmpkeys = pNode->bn_leaf.bn_data;
	int i,j;
	btk_t * keys = (btk_t *)elems;
	for(i=0; i<num; i++){
		for(j=0; j<pNode->bn_leaf.bn_sblock.bts_num; j++){
			if(bt_key_cmp(keys[i], tmpkeys[j]) != 0)
				continue;
			else{
				int x;
				for(x=j; x<pNode->bn_leaf.bn_sblock.bts_num; x++){
					if(INTER_KEY_NUM-1 == x)
						tmpkeys[x] = {0};
					else
						tmpkeys[x] = tmpkeys[x+1];
				}
				pNode->bn_leaf.bn_sblock.bts_num--;
			}
		}
	}
	if(oldnum - pNode->bn_leaf.bn_sblock.bts_num < num){
		log_debug("Add data num:%d less than elem num[%d] you want\n",oldnum - pNode->bn_leaf.bn_sblock.bts_num, num);
		return BTREE_FAIL;
	}
	return BTREE_OK;
}

int bt_search_branch(btnode_t * pNode, btk_t key){
	int i;
	for(i=0; i<INTER_KEY_NUM; i++){
		if(bt_key_cmp(pNode->bn_inter.bn_entry[i].bn_key, key)<0)
			continue;
		else
			return i;
	}
	return BTREE_FAIL;
}

int bt_search_leaf(btnode_t * pNode, btk_t key){
	int i;
	for(i=0; i<LEAF_KEY_NUM; i++){
		if(bt_key_cmp(pNode->bn_leaf.bn_data[i], key)<0)
			continue;
		else
			return i;
	}
	return BTREE_FAIL;
}

static inline void bt_branch_update(btnode_t * pNode, int idx, btk_t key){
	pNode->bn_inter.bn_entry[idx].bn_key = key;
}

int bt_insert(bptree_t * ptree, btk_t key){
	btpath_t * path[ptree->bpt_level] = {0};
	btnode_t * tmpNode = ptree->bpt_root;
	int idx = 0;
	int i;
	for(i=BT_LVL_ROOT; i>0; i--){
		path[i-1].pNode = tmpNode;
		if(i != BT_LVL_LEAF){
			idx = bt_search_branch(tmpNode, key);
			if(idx < 0){
				log_debug("have not found insert branch!\n");
				return BTREE_FAIL;
			}
			tmpNode = (btnode_t *)tmpNode->bn_inter.bn_entry[idx].bn_ptr;
		}
		else{
			idx = bt_search_leaf(tmpNode, key);
			if(idx < 0){
				log_debug("have not found insert entry!\n");
				return BTREE_FAIL;
			}
			tmpNode = NULL;
		}
		path[i-1].idx = idx;
	}
	spt_info_t spt_info = {0};
	rot_info_t rot_info = {0};
	istv_t ret = IST_FAIL;
	for(i=0; i<BT_LVL_ROOT; i++){
		memset(spt_info, 0, sizeof(spt_info_t));
		memset(rot_info, 0, sizeof(rot_info_t));
		if(0 == i){
			ret = bt_leaf_add(path[i].pNode, &key, &spt_info, &rot_info);
		}
		else{
			if(IST_OK == ret){
				break;
			}
			else{
				if(IST_SPLIT== ret){
					bt_branch_update(path[i].pNode, path[i].idx, spt_info.updkey);
					bti_t newIdx = {0};
					newIdx.bn_key = spt_info.newkey;
					newIdx.bn_ptr = spt_info.newPtr;
					ret = bt_branch_add(path[i].pNode, newIdx, &spt_info, &rot_info);
				}
				else if(IST_ROTATE == ret){
					bt_branch_update(path[i].pNode, path[i].idx-1, rot_info.leftkey);
					bt_branch_update(path[i].pNode, path[i].idx, rot_info.updkey);
					bt_branch_update(path[i].pNode, path[i].idx+1, rot_info.rightkey);
					ret = IST_OK;
				}
				else{
					break;
				}
			}
		}
	}
	if(IST_OK == ret)
		return BTREE_OK;
	return BTREE_FAIL;
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
