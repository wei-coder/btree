//B+树的功能实现

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "btree.h"

#define log_debug(msg,...)  printf(msg, ##__VA_ARGS__)
#define new(type, num)		(malloc(sizeof(type)*(num)))
#define del(ptr)			{if(NULL != ptr)free(ptr);}

static inline void equal_divi(int value, int num, int * arry){
    if(NULL == arry)
        return;
    int n = value/num;
    int i = 0;
    for(i=0; i< num; i++){
        arry[i] = n;
    }   
    for(i=0; i<value%num; i++){
        arry[i]++;
    }   
}

/*alloc a new node,if it's a leaf node,*/
btnode_t * bt_new_node(btype_t type){
	int size = 0;
	btnode_t * pNode = (btnode_t *)new(char, NODE_SIZE);
	if(NULL == pNode){
		log_debug("Malloc new Node for B+Tree failed, please check!\n");
		return NULL;
	}
	memset(pNode, 0, NODE_SIZE);
	
	pNode->bn_type = type;
	if(TYPE_LEAF == type){
		list_init(&pNode->bn_leaf.lnode);
	}

	return pNode;
}

void bt_free_node(btp_t ptr){
	del(ptr.bn_ptr);
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
	for(i=0; i<pinter->bn_num; i++){
		if(bt_key_cmp(idx.bn_key, tmpidxs[i].bn_key)>0)
			continue;
		else{
			break;
		}
	}

	for(j=pinter->bn_num+1; j>i; j--){
		tmpidxs[j] = tmpidxs[j-1];
	}
	tmpidxs[j] = idx;
	pinter->bn_num++;
	return BTREE_OK;
}

static inline int bt_branch_split(btnode_t * pNode, btnode_t ** ppNewNode){
	if(INTER_KEY_NUM > pNode->bn_inter.bn_num){
		log_debug("The leaf no need split!\n");
		return BTREE_FAIL;
	}
	btnode_t * ptmp = bt_new_node(TYPE_INT);
	if(NULL == ptmp)
		return BTREE_FAIL;
	ptmp->bn_level = pNode->bn_level;
	ptmp->bn_type = TYPE_INT;
	memcpy(ptmp->bn_inter.bn_entry, pNode->bn_inter.bn_entry+LEAF_KEY_NUM/2, sizeof(bti_t)*(LEAF_KEY_NUM-LEAF_KEY_NUM/2));
	pNode->bn_inter.bn_num = LEAF_KEY_NUM;
	ptmp->bn_inter.bn_num = LEAF_KEY_NUM-LEAF_KEY_NUM/2;
	memset(pNode->bn_inter.bn_entry+LEAF_KEY_NUM/2, 0, sizeof(bti_t)*(LEAF_KEY_NUM-LEAF_KEY_NUM/2));
	*ppNewNode = ptmp;
	return BTREE_OK;
}

/*add array elem for inter node*/
istv_t bt_branch_add(btnode_t * pNode, bti_t idx, spt_info_t * psplit, rotate_t * prot){
	if(pNode->bn_inter.bn_num >= LEAF_KEY_NUM){
		btnode_t * pnewNode = NULL;
		if(BTREE_FAIL == bt_branch_split(pNode, &pnewNode)){
			return IST_FAIL;
		}
		if(NULL == pnewNode){
			return IST_FAIL;
		}
		int ret = bt_key_cmp(pNode->bn_inter.bn_entry[pNode->bn_inter.bn_num-1].bn_key, idx.bn_key);
		if(ret > 0){
			bt_branch_real_add(&pNode->bn_inter, idx);
		}
		else{
			bt_branch_real_add(&pnewNode->bn_inter, idx);
		}
		psplit->updkey = pNode->bn_inter.bn_entry[pNode->bn_inter.bn_num-1].bn_key;
		psplit->newidx.bn_key = pnewNode->bn_inter.bn_entry[pnewNode->bn_inter.bn_num-1].bn_key;
		psplit->newidx.bn_ptr.bn_ptr = pnewNode;
		return IST_SPLIT;
	}

	int oldnum = pNode->bn_leaf.bn_num;
	bt_branch_real_add(&pNode->bn_inter, idx);
	if(pNode->bn_inter.bn_num-oldnum < 1){
		log_debug("Add data num:%d\n",pNode->bn_inter.bn_num-oldnum);
		return BTREE_FAIL;
	}
	return BTREE_OK;
}

void bt_branch_real_del(btinter_t * pbranch, int idx){
	int i;
	btp_t tmpptr = pbranch->bn_entry[idx].bn_ptr;
	for(i=idx; i>pbranch->bn_num-1; i++){
		pbranch->bn_entry[i] = pbranch->bn_entry[i+1];
	}
	pbranch->bn_num--;
	bt_free_node(tmpptr);
}

/*del array elem for inter node*/
delv_t bt_branch_del(btnode_t * pNode, int idx){
	if(pNode->bn_inter.bn_num-1 >= INTER_KEY_NUM/2){
		bt_branch_real_del(&pNode->bn_inter, idx);
		return DEL_OK;
	}

	bt_branch_real_del(&pNode->bn_inter, idx);
	return DEL_FAIL;
}

static inline void bt_branch_move2left(btinter_t * psrc, btinter_t * pdst, int move_num){
	memcpy(pdst->bn_entry+pdst->bn_num, psrc->bn_entry, sizeof(bti_t)*move_num);
	pdst->bn_num += move_num;
	int i;
	for(i=0; i<(psrc->bn_num - move_num); i++){
		psrc->bn_entry[i] = psrc->bn_entry[i+move_num];
	}
	psrc->bn_num -= move_num;
}

static inline void bt_branch_move2right(btinter_t * psrc, btinter_t * pdst, int move_num){
	int idx = psrc->bn_num-move_num-1;
	int i;
	for(i=pdst->bn_num; i>0; i--){
		pdst->bn_entry[i+move_num] = pdst->bn_entry[i];
	}
	memcpy(pdst->bn_entry, psrc->bn_entry+idx, sizeof(bti_t)*move_num);
	pdst->bn_num += move_num;
	psrc->bn_num -= move_num;
}

static inline void bt_branch_rotate(btinter_t * pmid, btinter_t * pleft, btinter_t * pright, rotate_t*  pbrot, rot_t mode){
	int num[ORDNUM] = {0};
	if(ROT_ADD == mode){
		equal_divi((pleft->bn_num+pright->bn_num+LEAF_KEY_NUM+1), ORDNUM, num);
	}
	else if(ROT_DEL == mode){
		equal_divi((pleft->bn_num+pright->bn_num + LEAF_KEY_NUM/2 - 1), ORDNUM, num);
	}

	int i;
	int move_num = 0;
	int idx = 0;
	/*left->this->right this mode need order*/
	if((num[LEFT] < pleft->bn_num)&&
		(num[RIGHT] > pright->bn_num)){
		/*move this->right at first*/
		move_num = num[RIGHT] - pright->bn_num;
		bt_branch_move2right(pmid, pright, move_num);
		/*move left->this at last*/
		move_num = pleft->bn_num - num[LEFT];
		bt_branch_move2right(pleft, pmid, move_num);
	}
	/*left<-this<-right this mode need order*/
	else if((num[LEFT] > pleft->bn_num)&&
		(num[RIGHT] < pright->bn_num)){
		/*move this->left at first*/
		move_num = num[LEFT] - pleft->bn_num;
		bt_branch_move2left(pmid, pleft, move_num);
		/*move right->this at last*/
		move_num = pright->bn_num - num[RIGHT];
		bt_branch_move2left(pright, pmid, move_num);
	}
	/*in other situation, we can rotate two leaf along*/
	else{
		/*left->this*/	
		if(num[LEFT] < pleft->bn_num){
			move_num = pleft->bn_num - num[LEFT];
			bt_branch_move2right(pleft, pmid, move_num);
		}

		/*this->right*/
		if(num[RIGHT] > pright->bn_num){
			move_num = num[RIGHT] - pright->bn_num;
			bt_branch_move2right(pmid, pright, move_num);
		}

		/*right->this*/
		if(num[RIGHT] < pright->bn_num){
			move_num = pright->bn_num - num[RIGHT];
			bt_branch_move2left(pright, pmid, move_num);
		}

		/*this->left*/
		if(num[LEFT] > pleft->bn_num){
			move_num = num[LEFT] - pleft->bn_num;
			bt_branch_move2left(pmid, pleft, move_num);
		}
	}

	pbrot->updkey = pmid->bn_entry[pmid->bn_num-1].bn_key;
	pbrot->leftkey = pleft->bn_entry[pleft->bn_num-1].bn_key;
	pbrot->rightkey = pright->bn_entry[pleft->bn_num-1].bn_key;
}

static inline void bt_branch_merge(btinter_t * pmid, btinter_t * pleft, btinter_t * pright, merge_t*  bmerg){
	int num = 0;
	int move_num = 0;
	if(pleft->bn_num > pright->bn_num){
		num = (pright->bn_num + pmid->bn_num)/2;
		move_num = pright->bn_num;
		bt_branch_move2left(pright,pmid,move_num);
		bmerg->updkey = pmid->bn_entry[pmid->bn_num-1].bn_key;
		bmerg->node = RIGHT;
	}
	else{
		num = (pleft->bn_num + pmid->bn_num)/2;
		move_num = pleft->bn_num;
		bt_branch_move2right(pleft,pmid,move_num);
		bmerg->updkey = pmid->bn_entry[pmid->bn_num-1].bn_key;
		bmerg->node = LEFT;
	}
}

/*leaf node be split to two node*/
int bt_leaf_split(btnode_t * pNode, btnode_t ** ppNewNode){
	if(LEAF_KEY_NUM > pNode->bn_leaf.bn_num){
		log_debug("The leaf no need split!\n");
		return BTREE_FAIL;
	}
	btnode_t * ptmp = bt_new_node(pNode->bn_type);
	if(NULL == ptmp)
		return BTREE_FAIL;
	ptmp->bn_level = pNode->bn_level;
	ptmp->bn_leaf.lnode.next = pNode->bn_leaf.lnode.next;
	ptmp->bn_leaf.lnode.prev = &pNode->bn_leaf.lnode;
	pNode->bn_leaf.lnode.next = &ptmp->bn_leaf.lnode;
	memcpy(ptmp->bn_leaf.bn_data, pNode->bn_leaf.bn_data+LEAF_KEY_NUM/2, sizeof(btk_t)*(LEAF_KEY_NUM-LEAF_KEY_NUM/2));
	pNode->bn_leaf.bn_num = LEAF_KEY_NUM;
	ptmp->bn_leaf.bn_num = LEAF_KEY_NUM-LEAF_KEY_NUM/2;
	memset(pNode->bn_leaf.bn_data+LEAF_KEY_NUM/2, 0, sizeof(btk_t)*(LEAF_KEY_NUM-LEAF_KEY_NUM/2));
	*ppNewNode = ptmp;
	return BTREE_OK;
}

static inline void bt_leaf_move2left(btleaf_t * psrc, btleaf_t * pdst, int move_num){
	memcpy(pdst->bn_data+pdst->bn_num, psrc->bn_data, sizeof(btk_t)*move_num);
	pdst->bn_num += move_num;
	int i;
	for(i=0; i<(psrc->bn_num - move_num); i++){
		psrc->bn_data[i] = psrc->bn_data[i+move_num];
	}
	psrc->bn_num -= move_num;
}

static inline void bt_leaf_move2right(btleaf_t * psrc, btleaf_t * pdst, int move_num){
	int idx = psrc->bn_num-move_num-1;
	int i;
	for(i=pdst->bn_num; i>0; i--){
		pdst->bn_data[i+move_num] = pdst->bn_data[i];
	}
	memcpy(pdst->bn_data, psrc->bn_data+idx, sizeof(btk_t)*move_num);
	pdst->bn_num += move_num;
	psrc->bn_num -= move_num;
}

/*leaf node and brother node rotate*/
static inline void bt_leaf_rotate(btleaf_t * pmid, btleaf_t * pleft, btleaf_t * pright, rotate_t * pinfo, rot_t mode){
	int num[ORDNUM] = {0};
	if(ROT_ADD == mode){
		equal_divi((pleft->bn_num+pright->bn_num+pmid->bn_num+1), ORDNUM, num);
	}
	else if(ROT_DEL == mode){
		equal_divi((pleft->bn_num+pright->bn_num + pmid->bn_num), ORDNUM, num);
	}

	int i;
	int move_num = 0;
	int idx = 0;
	/*left->this->right this mode need order*/
	if((num[LEFT] < pleft->bn_num)&&
		(num[RIGHT] > pright->bn_num)){
		/*move this->right at first*/
		move_num = num[RIGHT] - pright->bn_num;
		bt_leaf_move2right(pmid, pright, move_num);
		/*move left->this at last*/
		move_num = pleft->bn_num - num[LEFT];
		bt_leaf_move2right(pleft, pmid, move_num);
	}
	/*left<-this<-right this mode need order*/
	else if((num[LEFT] > pleft->bn_num)&&
		(num[RIGHT] < pright->bn_num)){
		/*move this->left at first*/
		move_num = num[LEFT] - pleft->bn_num;
		bt_leaf_move2left(pmid, pleft, move_num);
		/*move right->this at last*/
		move_num = pright->bn_num - num[RIGHT];
		bt_leaf_move2left(pright, pmid, move_num);
	}
	/*in other situation, we can rotate two leaf along*/
	else{
		/*left->this*/	
		if(num[LEFT] < pleft->bn_num){
			move_num = pleft->bn_num - num[LEFT];
			bt_leaf_move2right(pleft, pmid, move_num);
		}

		/*this->right*/
		if(num[RIGHT] > pright->bn_num){
			move_num = num[RIGHT] - pright->bn_num;
			bt_leaf_move2right(pmid, pright, move_num);
		}

		/*right->this*/
		if(num[RIGHT] < pright->bn_num){
			move_num = pright->bn_num - num[RIGHT];
			bt_leaf_move2left(pright, pmid, move_num);
		}

		/*this->left*/
		if(num[LEFT] > pleft->bn_num){
			move_num = num[LEFT] - pleft->bn_num;
			bt_leaf_move2left(pmid, pleft, move_num);
		}
	}

	pinfo->updkey = pmid->bn_data[pmid->bn_num-1];
	pinfo->leftkey = pleft->bn_data[pleft->bn_num-1];
	pinfo->rightkey = pright->bn_data[pleft->bn_num-1];
}

/*add array elem for leaf node*/
static inline void bt_leaf_real_add(btleaf_t * pleaf, btk_t * pkey){
	btk_t * tmpkeys = pleaf->bn_data;
	btk_t key = *pkey;
	int i,j;
	for(i=0; i<pleaf->bn_num; i++){
		if(bt_key_cmp(key, tmpkeys[i])>0)
			continue;
		else{
			break;
		}
	}

	for(j=pleaf->bn_num+1; j>i; j--){
		tmpkeys[j] = tmpkeys[j-1];
	}
	tmpkeys[j] = key;
	pleaf->bn_num++;
}

/*add array elem for leaf node*/
istv_t bt_leaf_add(btnode_t * pNode, btk_t * pkey, spt_info_t * psplit, rotate_t * protate){
	btleaf_t * pleft = (btleaf_t *)pNode->bn_leaf.lnode.prev;
	btleaf_t * pright = (btleaf_t *)pNode->bn_leaf.lnode.next;
	if(pNode->bn_leaf.bn_num >= LEAF_KEY_NUM){
		if((pleft->bn_num >= LEAF_KEY_NUM)&&
			(pright->bn_num >= LEAF_KEY_NUM)){
			btnode_t * pnewNode = NULL;
			if(BTREE_FAIL == bt_leaf_split(pNode, &pnewNode)){
				return IST_FAIL;
			}
			if(NULL == pnewNode){
				return IST_FAIL;
			}
			int ret = bt_key_cmp(pNode->bn_leaf.bn_data[pNode->bn_leaf.bn_num-1], *pkey);
			if(ret > 0){
				bt_leaf_real_add(&pNode->bn_leaf, pkey);
			}
			else{
				bt_leaf_real_add(&pnewNode->bn_leaf, pkey);
			}
			psplit->updkey = pNode->bn_leaf.bn_data[pNode->bn_leaf.bn_num-1];
			psplit->newidx.bn_key = pnewNode->bn_leaf.bn_data[pnewNode->bn_leaf.bn_num-1];
			psplit->newidx.bn_ptr.bn_ptr = pnewNode;
			return IST_SPLIT;
		}
		else{
			bt_leaf_rotate(&pNode->bn_leaf, pleft, pright, protate, ROT_ADD);
			btk_t leftKey = pleft->bn_data[pleft->bn_num-1];
			btk_t thisKey = pNode->bn_leaf.bn_data[pNode->bn_leaf.bn_num-1];
			if(bt_key_cmp(leftKey, *pkey)>0){
				bt_leaf_real_add(pleft, pkey);
				protate->leftkey = pleft->bn_data[pleft->bn_num-1];
			}
			else if(bt_key_cmp(thisKey, *pkey)>0){
				bt_leaf_real_add(&pNode->bn_leaf, pkey);
				protate->updkey = pNode->bn_leaf.bn_data[pNode->bn_leaf.bn_num-1];
			}
			else{
				bt_leaf_real_add(pright, pkey);
				protate->rightkey = pright->bn_data[pright->bn_num-1];
			}	
			return IST_ROTATE;
		}
	}

	int oldnum = pNode->bn_leaf.bn_num;
	bt_leaf_real_add(&pNode->bn_leaf, pkey);
	if(pNode->bn_leaf.bn_num-oldnum < 1){
		log_debug("Add data num:%d please check!\n",pNode->bn_leaf.bn_num-oldnum);
		return BTREE_FAIL;
	}
	return BTREE_OK;
}

static inline void bt_leaf_real_del(btleaf_t * pleaf, btk_t key){
	int i, j;
	for(i=0; i<pleaf->bn_num; i++){
		if(bt_key_cmp(key, pleaf->bn_data[i]) != 0)
			continue;
		else{
			break;
		}
	}
	
	for(j=i; j<pleaf->bn_num; j++){
		if(INTER_KEY_NUM-1 == j)
			pleaf->bn_data[j].bn_key = 0;
		else
			pleaf->bn_data[j] = pleaf->bn_data[j+1];
	}
	pleaf->bn_num--;
}

/*del array elem for leaf node*/
delv_t bt_leaf_del(btnode_t * pNode, btk_t key){
	bt_leaf_real_del(&pNode->bn_leaf, key);
	if(pNode->bn_leaf.bn_num >= LEAF_KEY_NUM/2){
		return DEL_OK;
	}

	return DEL_FAIL;
}

void bt_leaf_merge(btleaf_t * pmid, btleaf_t * pleft, btleaf_t * pright, merge_t * pinfo){
	int num = 0;
	int move_num = 0;
	/*merge right node*/
	if(pleft->bn_num > pright->bn_num){
		pright->lnode.prev->next = pright->lnode.next;
		pright->lnode.next->prev = pright->lnode.prev;
		num = (pright->bn_num + pmid->bn_num)/2;
		move_num = pright->bn_num;
		bt_leaf_move2left(pright,pmid, move_num);
		pinfo->updkey = pmid->bn_data[pmid->bn_num-1];
		pinfo->node = RIGHT;
	}
	/*merge left node*/
	else{
		pleft->lnode.prev->next = pleft->lnode.next;
		pleft->lnode.next->prev = pleft->lnode.prev;
		num = (pleft->bn_num + pmid->bn_num)/2;
		move_num = pleft->bn_num;
		bt_leaf_move2right(pleft,pmid,move_num);
		pinfo->updkey = pmid->bn_data[pmid->bn_num-1];
		pinfo->node = LEFT;
	}
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

int bt_exactsearch_leaf(btnode_t * pNode, btk_t key){
	int i;
	for(i=0; i<LEAF_KEY_NUM; i++){
		if(bt_key_cmp(pNode->bn_leaf.bn_data[i], key) != 0)
			continue;
		else
			return i;
	}
	return BTREE_FAIL;
}

static inline void bt_branch_update(btnode_t * pNode, int idx, btk_t key){
	pNode->bn_inter.bn_entry[idx].bn_key = key;
}

static inline void bt_merge(btnode_t * pfather, int idx, merge_t * pinfo){
	btnode_t * pmid = (btnode_t *)pfather->bn_inter.bn_entry[idx].bn_ptr.bn_ptr;
	btnode_t * pleft = (btnode_t *)pfather->bn_inter.bn_entry[idx-1].bn_ptr.bn_ptr;
	btnode_t * pright = (btnode_t *)pfather->bn_inter.bn_entry[idx+1].bn_ptr.bn_ptr;
	if(TYPE_LEAF == pmid->bn_type){
		bt_leaf_merge(&pmid->bn_leaf, &pleft->bn_leaf, &pright->bn_leaf, pinfo);
	}
	else{
		bt_branch_merge(&pmid->bn_inter, &pleft->bn_inter, &pright->bn_inter, pinfo);
	}
}

static inline void bt_rotate(btnode_t * pfather, int idx, rotate_t * pinfo, rot_t mode){
	btnode_t * pmid = (btnode_t *)pfather->bn_inter.bn_entry[idx].bn_ptr.bn_ptr;
	btnode_t * pleft = (btnode_t *)pfather->bn_inter.bn_entry[idx-1].bn_ptr.bn_ptr;
	btnode_t * pright = (btnode_t *)pfather->bn_inter.bn_entry[idx+1].bn_ptr.bn_ptr;
	if(TYPE_LEAF == pmid->bn_type){
		bt_leaf_rotate(&pmid->bn_leaf, &pleft->bn_leaf, &pright->bn_leaf, pinfo, mode);
	}
	else{
		bt_branch_rotate(&pmid->bn_inter, &pleft->bn_inter, &pright->bn_inter, pinfo, mode);
	}
}

int bt_insert(bptree_t * ptree, btk_t key){
	btpath_t path[BT_LVL_ROOT] = {0};
	btnode_t * tmpNode = ptree->bpt_root;
	int idx = 0;
	int i;
	for(i=0; i<ptree->bpt_level; i++){
		path[i].pNode = tmpNode;
		if(tmpNode->bn_type != TYPE_LEAF){
			path[i].idx = bt_search_branch(tmpNode, key);
		}
		else{
			path[i].idx = bt_search_leaf(tmpNode, key);
		}
		if(path[i].idx == BTREE_FAIL){
			log_debug("The key you want delete is not exist!\n");
			return BTREE_FAIL;
		}
	}

	spt_info_t spt_info = {0};
	rotate_t rot_info = {0};
	istv_t ret = IST_FAIL;
	for(i=ptree->bpt_level-1; i>=0; i--){
		if(IST_OK == ret){
				break;
		}
		memset(&spt_info, 0, sizeof(spt_info_t));
		memset(&rot_info, 0, sizeof(rotate_t));
		tmpNode = path[i].pNode;
		idx = path[i].idx;
		if(TYPE_LEAF == tmpNode->bn_type){
			ret = bt_leaf_add(path[i].pNode, &key, &spt_info, &rot_info);
		}
		else{
			if(IST_SPLIT== ret){
				bt_branch_update(tmpNode, idx, spt_info.updkey);
				bti_t newIdx = {0};
				newIdx.bn_key = spt_info.newidx.bn_key;
				newIdx.bn_ptr = spt_info.newidx.bn_ptr;
				ret = bt_branch_add(tmpNode, newIdx, &spt_info, &rot_info);
			}
			else if(IST_ROTATE == ret){
				bt_branch_update(tmpNode, idx-1, rot_info.leftkey);
				bt_branch_update(tmpNode, idx, rot_info.updkey);
				bt_branch_update(tmpNode, idx+1, rot_info.rightkey);
				ret = IST_OK;
			}
			else{
				break;
			}
		}
	}
	if(IST_SPLIT == ret){
		/*in this situation, need increasing new level*/
		btnode_t * newroot = bt_new_node(TYPE_INT);
		btnode_t * newbranch = NULL;
		btp_t ptr;
		if(NULL == newroot){
			log_debug("alloc new root for increase new level failed!\n");
			return BTREE_FAIL;
		}
		newroot->bn_level = BT_LVL_ROOT;
		if(tmpNode->bn_type == TYPE_LEAF){
			if(BTREE_FAIL == bt_leaf_split(tmpNode,&newbranch)){
				ptr.bn_ptr = newroot;
				bt_free_node(ptr);
				return BTREE_FAIL;
			}
			tmpNode->bn_level = BT_LVL_INT;
			newbranch->bn_level = BT_LVL_INT;
			newroot->bn_inter.bn_entry[0].bn_key = tmpNode->bn_leaf.bn_data[tmpNode->bn_inter.bn_num-1];
			newroot->bn_inter.bn_entry[1].bn_key = newbranch->bn_leaf.bn_data[newbranch->bn_inter.bn_num-1];
			newroot->bn_inter.bn_num = 2;
		}
		else{
			if(BTREE_FAIL == bt_branch_split(tmpNode,&newbranch)){
					ptr.bn_ptr = newroot;
					bt_free_node(ptr);
					return BTREE_FAIL;
			}
			tmpNode->bn_level = BT_LVL_INT;
			newbranch->bn_level = BT_LVL_INT;
			newroot->bn_inter.bn_entry[0].bn_key = tmpNode->bn_inter.bn_entry[tmpNode->bn_inter.bn_num-1].bn_key;
			newroot->bn_inter.bn_entry[1].bn_key = newbranch->bn_inter.bn_entry[newbranch->bn_inter.bn_num-1].bn_key;
			newroot->bn_inter.bn_num = 2;
		}
		ret = IST_OK;
	}
	else if((IST_ROTATE == ret)||
		(IST_FAIL == ret)){
		log_debug("Root insert key and return %s, please check!\n", ret==IST_ROTATE?"ROTATE":"FAIL");
		return BTREE_FAIL;
	}
	return BTREE_OK;
}

/*delete key, each level del func return ok or fail, the father node decide rotate or merge*/
int bt_del(bptree_t * ptree, btk_t key){
	btpath_t path[BT_LVL_ROOT] = {0};
	btnode_t * tmpNode = ptree->bpt_root;
	int i,j;
	for(i=0; i<ptree->bpt_level; i++){
		path[i].pNode = tmpNode;
		if(tmpNode->bn_type != TYPE_LEAF){
			path[i].idx = bt_search_branch(tmpNode, key);
		}
		else{
			path[i].idx = bt_search_leaf(tmpNode, key);
		}
		if(path[i].idx == BTREE_FAIL){
			log_debug("The key you want delete is not exist!\n");
			return BTREE_FAIL;
		}
	}

	rotate_t rot_info = {0};
	merge_t mer_info = {0};
	delv_t ret = DEL_FAIL;
	int num[ORDNUM] = {0};
	btnode_t * pleft = NULL;
	btnode_t * pright = NULL;
	btnode_t * pmid = NULL;
	int idx = 0;
	int total = 0;

	for(i=ptree->bpt_level-1; i>=0; i--){
		if(DEL_OK == ret)
			break;
		tmpNode = path[i].pNode;
		idx = path[i].idx;
		if(TYPE_LEAF == tmpNode->bn_type){
			ret = bt_leaf_del(tmpNode, key);
		}
		else if(TYPE_INT == tmpNode->bn_type){
			pleft = (btnode_t *)tmpNode->bn_inter.bn_entry[path[i].idx-1].bn_ptr.bn_ptr;
			pright = (btnode_t *)tmpNode->bn_inter.bn_entry[path[i].idx+1].bn_ptr.bn_ptr;
			pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[path[i].idx].bn_ptr.bn_ptr;
			if(TYPE_LEAF == path[i+1].pNode->bn_type){
				total = pleft->bn_leaf.bn_num + pright->bn_leaf.bn_num + pmid->bn_leaf.bn_num;
			}
			else{
				total = pleft->bn_inter.bn_num + pright->bn_inter.bn_num + pmid->bn_inter.bn_num;
				equal_divi(total, ORDNUM, num);
				if((LEAF_KEY_NUM/2 > num[LEFT])||
					(LEAF_KEY_NUM/2 > num[MID])||
					(LEAF_KEY_NUM/2 > num[RIGHT])){
					bt_branch_merge(&pmid->bn_inter,&pleft->bn_inter,&pright->bn_inter,&mer_info);
				}
				else{
					bt_branch_rotate(&pmid->bn_inter,&pleft->bn_inter,&pright->bn_inter,&rot_info, ROT_DEL);
				}
			}
			
			equal_divi(total, ORDNUM, num);
			if((LEAF_KEY_NUM/2 > num[LEFT])||
				(LEAF_KEY_NUM/2 > num[MID])||
				(LEAF_KEY_NUM/2 > num[RIGHT])){
				bt_merge(tmpNode, idx, &mer_info);
				bt_branch_update(tmpNode, idx, mer_info.updkey);
				if(LEFT == mer_info.node){
					ret = bt_branch_del(tmpNode, idx-1);
				}
				else if(RIGHT == mer_info.node){
					ret = bt_branch_del(tmpNode, idx+1);
				}
			}
			else{
				bt_rotate(tmpNode, idx, &rot_info, ROT_DEL);
				bt_branch_update(tmpNode, idx, rot_info.updkey);
				bt_branch_update(tmpNode, idx-1, rot_info.leftkey);
				bt_branch_update(tmpNode, idx+1, rot_info.rightkey);
			}
		}
	}

	if(DEL_FAIL == ret){
		if(tmpNode->bn_type != TYPE_LEAF){
			total = 0;
			pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
			if(pmid->bn_type==TYPE_LEAF){
				for(i=0; i< tmpNode->bn_inter.bn_num; i++){
					pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[i].bn_ptr.bn_ptr;
					total += pmid->bn_leaf.bn_num;
				}
			}
			else{
				for(i=0; i< tmpNode->bn_inter.bn_num; i++){
					pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[i].bn_ptr.bn_ptr;
					total += pmid->bn_inter.bn_num;
				}
			}
			if(tmpNode->bn_inter.bn_num < INTER_KEY_NUM/2){
				/*in this situation, we need decrease a level*/
				pleft = (btnode_t *)tmpNode->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
				btnode_t * tmpbuf = new(char, NODE_SIZE);
				if(NULL == tmpbuf){
					log_debug("alloc new root buf for decrease level failed!\n");
					return BTREE_FAIL;
				}
				memset(tmpbuf, 0, NODE_SIZE);
				if(TYPE_LEAF == pmid->bn_type){
					if(total < LEAF_KEY_NUM){
						/*change root from index to data*/
						tmpbuf->bn_level = BT_LVL_ROOT;
						tmpbuf->bn_type = TYPE_LEAF;
						list_init(&tmpbuf->bn_leaf.lnode);
						tmpbuf->bn_leaf.bn_num = 0;
						for(i=0; i<tmpNode->bn_inter.bn_num; i++){
							pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[i].bn_ptr.bn_ptr;
							for(j=0; j<pmid->bn_leaf.bn_num; j++){
								tmpbuf->bn_leaf.bn_data[tmpbuf->bn_leaf.bn_num] = pmid->bn_leaf.bn_data[j];
								tmpbuf->bn_leaf.bn_num++;
							}
							bt_free_node(tmpNode->bn_inter.bn_entry[i].bn_ptr);
						}
						memcpy(tmpNode, tmpbuf, NODE_SIZE);
					}
				}
				else{
					if(total < INTER_KEY_NUM){
						/*move ptr from son node to root*/
						tmpbuf->bn_level = BT_LVL_ROOT;
						tmpbuf->bn_type = TYPE_INT;
						tmpbuf->bn_inter.bn_num = 0;
						for(i=0; i<tmpNode->bn_inter.bn_num; i++){
							pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[i].bn_ptr.bn_ptr;
							for(j=0; j<pmid->bn_inter.bn_num; j++){
								tmpbuf->bn_inter.bn_entry[tmpbuf->bn_leaf.bn_num] = pmid->bn_inter.bn_entry[j];
								tmpbuf->bn_inter.bn_num++;
							}
							bt_free_node(tmpNode->bn_inter.bn_entry[i].bn_ptr);
						}
						memcpy(tmpNode, tmpbuf, NODE_SIZE);
					}
				}
				del(tmpbuf);
			}
		}
		else{
			log_debug("there are only %d entries!\n",tmpNode->bn_leaf.bn_num);
		}
	}
	return BTREE_OK;
}

void bt_init(bptree_t * ptree){
	ptree->bpt_keynums = LEAF_KEY_NUM;
	ptree->bpt_level = BT_LVL_ROOT;
	ptree->bpt_root = bt_new_node(TYPE_LEAF);
	ptree->bpt_root->bn_level = BT_LVL_ROOT;
}

#if 1

void print_leaf(btleaf_t * pleaf, char * prefix){
	int i = 0;
	for(i=0; i<pleaf->bn_num-1; i++){
		printf("%s├──────%ld\n", prefix, pleaf->bn_data[i].bn_key);
	}
	printf("%s└──────%ld\n", prefix, pleaf->bn_data[i].bn_key);
}

void print_btree(bptree_t * ptree)
{
	if(NULL == ptree)
	{
		return;
	}

	btpath_t path[BT_LVL_ROOT] = {0};
	int cusor = 0;
	path[cusor].pNode = ptree->bpt_root;
	path[cusor].idx = 0;
	cusor++;
	btnode_t * tmpNode = NULL;
	btp_t ptr;
	int idx = 0;
	char prefix[100] = {0};
	while(cusor){
		tmpNode = path[cusor-1].pNode;
		if(TYPE_LEAF != tmpNode->bn_type){
			if(path[cusor-1].idx < tmpNode->bn_inter.bn_num){
				if(path[cusor-1].idx == tmpNode->bn_inter.bn_num-1){
					printf("%s└──────%ld\n", prefix, tmpNode->bn_inter.bn_entry[idx].bn_key.bn_key);
					sprintf(prefix,"%s 	   ", prefix);
				}
				else{
					printf("%s├──────%ld\n", prefix, tmpNode->bn_inter.bn_entry[path[cusor-1].idx].bn_key.bn_key);
					sprintf(prefix,"%s│	   ", prefix);
				}
				path[cusor-1].pNode = (btnode_t *)tmpNode->bn_inter.bn_entry[idx].bn_ptr.bn_ptr;
				path[cusor-1].idx = 0;
				cusor ++;
			}
			else{
				cusor--;
			}
		}
		else{
			print_leaf(&tmpNode->bn_leaf, prefix);
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
	char order;
	btk_t key = {0};
	bptree_t tree = {0};
	bt_init(&tree);
	print_prompt();
	print_eof();
	while(1)
	{
		setbuf(stdin, NULL);
		scanf("%c", &order);
		setbuf(stdin, NULL);
		switch(order)
		{
			case 'i':
				//insert
				log_debug("\n please input node id which you want insert:");
				setbuf(stdin, NULL);
				scanf("%ld", &key.bn_key);
				setbuf(stdin, NULL);
				bt_insert(&tree,key);
				print_eof();
				break;
			case 'd':
				//delete
				log_debug("\n please input node id which you want delete:");
				setbuf(stdin, NULL);
				scanf("%ld", &key.bn_key);
				setbuf(stdin, NULL);
				bt_del(&tree,key);
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
				print_btree(&tree);
				print_eof();
				break;
			default:
				break;
		};
	}
	return 0;
}
