//B+树的功能实现

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include "btree.h"

#define log_debug(msg,...)  printf(msg, ##__VA_ARGS__)
#define log_info(msg,...)  printf(msg, ##__VA_ARGS__)
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
		pNode->bn_leaf.lnode.next = NULL;
		pNode->bn_leaf.lnode.prev = NULL;
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

int bt_search_branch(btnode_t * pNode, btk_t key){
	int i;
	for(i=0; i<pNode->bn_inter.bn_num; i++){
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

	for(j=pinter->bn_num; j>i; j--){
		tmpidxs[j] = tmpidxs[j-1];
	}
	tmpidxs[i] = idx;
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
	memcpy(ptmp->bn_inter.bn_entry, pNode->bn_inter.bn_entry+INTER_KEY_NUM/2, sizeof(bti_t)*(INTER_KEY_NUM-INTER_KEY_NUM/2));
	pNode->bn_inter.bn_num = INTER_KEY_NUM/2;
	ptmp->bn_inter.bn_num = INTER_KEY_NUM-INTER_KEY_NUM/2;
	*ppNewNode = ptmp;
	return BTREE_OK;
}

/*add array elem for inter node*/
istv_t bt_branch_add(btnode_t * pNode, bti_t idx, spt_info_t * psplit, rotate_t * prot){
	if(pNode->bn_inter.bn_num >= INTER_KEY_NUM){
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

	int oldnum = pNode->bn_inter.bn_num;
	btk_t oldkey = pNode->bn_inter.bn_entry[oldnum].bn_key;
	bt_branch_real_add(&pNode->bn_inter, idx);
	if(pNode->bn_inter.bn_num-oldnum < 1){
		log_debug("Add data num:%d\n",pNode->bn_inter.bn_num-oldnum);
		return IST_FAIL;
	}
	else if(bt_key_cmp(oldkey, pNode->bn_inter.bn_entry[pNode->bn_inter.bn_num].bn_key) < 0){
		prot->updkey = pNode->bn_inter.bn_entry[pNode->bn_inter.bn_num].bn_key;
		prot->leftkey.bn_key = 0;
		prot->rightkey.bn_key = 0;
		return IST_UPDATE;
	}
	return IST_OK;
}

void bt_branch_real_del(btinter_t * pbranch, int idx){
	int i;
	btp_t tmpptr = {0};
	tmpptr.bn_ptr = pbranch->bn_entry[idx].bn_ptr.bn_ptr;
	for(i=idx; i<pbranch->bn_num-1; i++){
		pbranch->bn_entry[i] = pbranch->bn_entry[i+1];
	}
	pbranch->bn_num--;
	bt_free_node(tmpptr);
}

/*del array elem for inter node*/
delv_t bt_branch_del(btnode_t * pNode, int idx){
	bt_branch_real_del(&pNode->bn_inter, idx);
	if(pNode->bn_inter.bn_num >= INTER_KEY_NUM/2){
		return DEL_OK;
	}
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

static inline void bt_branch_rotate(btnode_t * plNode, btnode_t * pmNode, btnode_t * prNode, int idx, rotate_t*  pbrot, rot_t mode){
	int i;
	int value = 0;
	int node_num = 0;
	btk_t oldkey[ORDNUM] = {0};
	btinter_t * pleft = NULL;
	btinter_t * pmid = NULL;
	btinter_t * pright = NULL;
	btinter_t * target = NULL;
	if(NULL != plNode){
		pleft = &plNode->bn_inter;
		value += pleft->bn_num;
		oldkey[LEFT] = pleft->bn_entry[pleft->bn_num-1].bn_key;
		node_num ++;
	}
	if(NULL != prNode){
		pright = &prNode->bn_inter;
		value += pright->bn_num;
		oldkey[RIGHT] = pright->bn_entry[pright->bn_num-1].bn_key;
		node_num ++;
	}
	pmid = &pmNode->bn_inter;
	value += pmid->bn_num;
	oldkey[MID] = pmid->bn_entry[pmid->bn_num-1].bn_key;
	node_num++;
	int num[ORDNUM] = {0};
	equal_divi(value, node_num, num);
	int move_num = 0;
	if(pleft != NULL && pright != NULL){
		idx += pleft->bn_num;
		if(ROT_ADD == mode){
			if(idx < num[LEFT]){
				target = pleft;
				if(num[LEFT] >= LEAF_KEY_NUM){
					num[LEFT]--;
					if(num[MID] < LEAF_KEY_NUM){
						num[MID]++;
					}
					else{
						num[RIGHT]++;
					}
				}
			}
			else if(idx < num[LEFT]+num[MID]){
				target = pmid;
				idx -= num[LEFT];
				if(num[MID] >= LEAF_KEY_NUM){
					num[MID]--;
					num[RIGHT]++;
				}
			}
			else{
				target = pright;
				idx -= (num[LEFT] + num[MID]);
			}
		}
	}
	/*in other situation, we can rotate two leaf along*/
	else if(NULL != pleft){
		idx += pleft->bn_num;
		if(ROT_ADD == mode){
			if(idx <= num[LEFT]-1){
				target = pleft;
				if(num[LEFT] >= LEAF_KEY_NUM){
					num[LEFT]--;
					num[MID]++;
				}
			}
			else{
				idx -= num[LEFT];
				target = pmid;
			}
		}
	}
	else if(NULL != pright){
		num[RIGHT] = num[MID];
		num[MID] = num[LEFT];
		num[LEFT] = 0;
		if(ROT_ADD == mode){
			if(idx < num[MID]){
				target = pmid;
				if(num[MID] >= LEAF_KEY_NUM){
					num[MID]--;
					num[RIGHT]++;
				}
			}
			else{
				idx -= num[MID];
				target = pright;
			}
		}
	}
	if(NULL != pleft && NULL != pright){
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
	}

	if(NULL != pleft){
		/*left->this*/
		if(num[LEFT] < pleft->bn_num){
			move_num = pleft->bn_num - num[LEFT];
			bt_branch_move2right(pleft, pmid, move_num);
		}
		
		/*this->left*/
		if(num[LEFT] > pleft->bn_num){
			move_num = num[LEFT] - pleft->bn_num;
			bt_branch_move2left(pmid, pleft, move_num);
		}
	}

	if(NULL != pright){
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
	}

	if(ROT_ADD == mode){
		//bt_branch_real_add(target, idx, key);
	}
	if(0 == bt_key_cmp(oldkey[MID],pmid->bn_entry[pmid->bn_num-1].bn_key)){
		log_debug("old mid key: %ld new mid key: %ld ", oldkey[MID].bn_key, pmid->bn_entry[pmid->bn_num-1].bn_key.bn_key);
		pbrot->updkey.bn_key = 0;
	}
	else{
		pbrot->updkey = pmid->bn_entry[pmid->bn_num-1].bn_key;
	}
	if(NULL != pleft && 0 != bt_key_cmp(oldkey[LEFT], pleft->bn_entry[pleft->bn_num-1].bn_key)){
		pbrot->leftkey = pleft->bn_entry[pleft->bn_num-1].bn_key;
	}
	else{
		if(NULL != pleft){
			log_debug("old left key: %ld new left key: %ld ", oldkey[LEFT].bn_key, pleft->bn_entry[pleft->bn_num-1].bn_key.bn_key);
		}
		pbrot->leftkey.bn_key = 0;
	}
	if(NULL != pright && 0 != bt_key_cmp(oldkey[RIGHT], pright->bn_entry[pright->bn_num-1].bn_key)){
		pbrot->rightkey = pright->bn_entry[pright->bn_num-1].bn_key;
	}
	else{
		if(NULL != pright){
			log_debug("old right key: %ld new right key: %ld ", oldkey[RIGHT].bn_key, pright->bn_entry[pright->bn_num-1].bn_key.bn_key);
		}
		pbrot->rightkey.bn_key = 0;
	}
	log_debug("\n");
}

static inline void bt_branch_merge(btnode_t * pleft, btnode_t * pmid, btnode_t * pright, merge_t*  bmerg){
	int num = 0;
	int move_num = 0;
	if((pright == NULL) || (pleft != NULL && pleft != NULL && pleft->bn_inter.bn_num <= pright->bn_inter.bn_num)){
		num = (pleft->bn_inter.bn_num + pmid->bn_inter.bn_num)/2;
		move_num = pleft->bn_inter.bn_num;
		bt_branch_move2right(&pleft->bn_inter, &pmid->bn_inter, move_num);
		bmerg->updkey = pmid->bn_inter.bn_entry[pmid->bn_inter.bn_num-1].bn_key;
		bmerg->node = LEFT;
	}
	else if((pleft == NULL) ||(pleft != NULL && pleft != NULL && pleft->bn_inter.bn_num >= pright->bn_inter.bn_num)){
		num = (pright->bn_inter.bn_num + pmid->bn_inter.bn_num)/2;
		move_num = pright->bn_inter.bn_num;
		bt_branch_move2left(&pright->bn_inter, &pmid->bn_inter,move_num);
		bmerg->updkey = pmid->bn_inter.bn_entry[pmid->bn_inter.bn_num-1].bn_key;
		bmerg->node = RIGHT;
	}
}

static inline void bt_leaf_move2left(btleaf_t * psrc, btleaf_t * pdst, int move_num){
	if(0 == move_num)
		return;
	memcpy(pdst->bn_data+pdst->bn_num, psrc->bn_data, sizeof(btk_t)*move_num);
	pdst->bn_num += move_num;
	int i;
	for(i=0; i<(psrc->bn_num - move_num); i++){
		psrc->bn_data[i] = psrc->bn_data[i+move_num];
	}
	psrc->bn_num -= move_num;
}

static inline void bt_leaf_move2right(btleaf_t * psrc, btleaf_t * pdst, int move_num){
	int i;
	for(i=pdst->bn_num-1; i>=0; i--){
		pdst->bn_data[i+move_num] = pdst->bn_data[i];
	}
	memcpy(pdst->bn_data, psrc->bn_data+psrc->bn_num-move_num, sizeof(btk_t)*move_num);
	pdst->bn_num += move_num;
	psrc->bn_num -= move_num;
}

/*leaf node be split to two node, and new node locate right of node*/
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
	if(NULL != pNode->bn_leaf.lnode.next){
		pNode->bn_leaf.lnode.next->prev = &ptmp->bn_leaf.lnode;
	}
	pNode->bn_leaf.lnode.next = &ptmp->bn_leaf.lnode;
	bt_leaf_move2right(&pNode->bn_leaf, &ptmp->bn_leaf, LEAF_KEY_NUM/2);
	*ppNewNode = ptmp;
	return BTREE_OK;
}

/*add array elem for leaf node*/
static inline void bt_leaf_real_add(btleaf_t * pleaf, int idx, btk_t key){
	btk_t * tmpkeys = pleaf->bn_data;
	if(0 == pleaf->bn_num){
		tmpkeys[0] = key;
	}
	else if(idx >= pleaf->bn_num){
		tmpkeys[idx] = key;
	}
	else if(bt_key_cmp(key, tmpkeys[idx]) < 0){
		int j;
		for(j=pleaf->bn_num; j>idx; j--){
			tmpkeys[j] = tmpkeys[j-1];
		}
		tmpkeys[idx] = key;
	}
	else{
		tmpkeys[idx+1] = key;
	}
	pleaf->bn_num++;
}

/*leaf node and brother node rotate*/
static inline void bt_leaf_rotate(btnode_t * plNode, btnode_t * pmNode, btnode_t * prNode, int idx, btk_t key, rotate_t * pinfo, rot_t mode){
	int i;
	int value = 0;
	int node_num = 0;
	btk_t oldkey[ORDNUM] = {0};
	btleaf_t * pleft = NULL;
	btleaf_t * pmid = NULL;
	btleaf_t * pright = NULL;
	btleaf_t * target = NULL;
	if(NULL != plNode){
		pleft = &plNode->bn_leaf;
		value += pleft->bn_num;
		oldkey[LEFT] = pleft->bn_data[pleft->bn_num-1];
		node_num ++;
	}
	if(NULL != prNode){
		pright = &prNode->bn_leaf;
		value += pright->bn_num;
		oldkey[RIGHT] = pright->bn_data[pright->bn_num-1];
		node_num ++;
	}
	pmid = &pmNode->bn_leaf;
	value += pmid->bn_num;
	oldkey[MID] = pmid->bn_data[pmid->bn_num-1];
	node_num++;
	int num[ORDNUM] = {0};
	equal_divi(value, node_num, num);
	int move_num = 0;
	if(pleft != NULL && pright != NULL){
		idx += pleft->bn_num;
		if(ROT_ADD == mode){
			if(idx < num[LEFT]){
				target = pleft;
				if(num[LEFT] >= LEAF_KEY_NUM){
					num[LEFT]--;
					if(num[MID] < LEAF_KEY_NUM){
						num[MID]++;
					}
					else{
						num[RIGHT]++;
					}
				}
			}
			else if(idx < num[LEFT]+num[MID]){
				target = pmid;
				idx -= num[LEFT];
				if(num[MID] >= LEAF_KEY_NUM){
					num[MID]--;
					num[RIGHT]++;
				}
			}
			else{
				target = pright;
				idx -= (num[LEFT] + num[MID]);
			}
		}
	}
	/*in other situation, we can rotate two leaf along*/
	else if(NULL != pleft){
		idx += pleft->bn_num;
		if(ROT_ADD == mode){
			if(idx <= num[LEFT]-1){
				target = pleft;
				if(num[LEFT] >= LEAF_KEY_NUM){
					num[LEFT]--;
					num[MID]++;
				}
			}
			else{
				idx -= num[LEFT];
				target = pmid;
			}
		}
	}
	else if(NULL != pright){
		num[RIGHT] = num[MID];
		num[MID] = num[LEFT];
		num[LEFT] = 0;
		if(ROT_ADD == mode){
			if(idx < num[MID]){
				target = pmid;
				if(num[MID] >= LEAF_KEY_NUM){
					num[MID]--;
					num[RIGHT]++;
				}
			}
			else{
				idx -= num[MID];
				target = pright;
			}
		}
	}
	if(NULL != pleft && NULL != pright){
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
	}

	if(NULL != pleft){
		/*left->this*/
		if(num[LEFT] < pleft->bn_num){
			move_num = pleft->bn_num - num[LEFT];
			bt_leaf_move2right(pleft, pmid, move_num);
		}
		
		/*this->left*/
		if(num[LEFT] > pleft->bn_num){
			move_num = num[LEFT] - pleft->bn_num;
			bt_leaf_move2left(pmid, pleft, move_num);
		}
	}

	if(NULL != pright){
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
	}

	if(ROT_ADD == mode){
		bt_leaf_real_add(target, idx, key);
	}
	if(0 == bt_key_cmp(oldkey[MID],pmid->bn_data[pmid->bn_num-1])){
		log_debug("old mid key: %ld new mid key: %ld ", oldkey[MID].bn_key, pmid->bn_data[pmid->bn_num-1].bn_key);
		pinfo->updkey.bn_key = 0;
	}
	else{
		pinfo->updkey = pmid->bn_data[pmid->bn_num-1];
	}
	if(NULL != pleft && 0 != bt_key_cmp(oldkey[LEFT], pleft->bn_data[pleft->bn_num-1])){
		pinfo->leftkey = pleft->bn_data[pleft->bn_num-1];
	}
	else{
		if(NULL != pleft){
			log_debug("old left key: %ld new left key: %ld ", oldkey[LEFT].bn_key, pleft->bn_data[pleft->bn_num-1].bn_key);
		}
		pinfo->leftkey.bn_key = 0;
	}
	if(NULL != pright && 0 != bt_key_cmp(oldkey[RIGHT], pright->bn_data[pright->bn_num-1])){
		pinfo->rightkey = pright->bn_data[pright->bn_num-1];
	}
	else{
		if(NULL != pright){
			log_debug("old right key: %ld new right key: %ld ", oldkey[RIGHT].bn_key, pright->bn_data[pright->bn_num-1].bn_key);
		}
		pinfo->rightkey.bn_key = 0;
	}
	log_debug("\n");
}

/*add array elem for leaf node*/
istv_t bt_leaf_add(btpath_t path_node, btk_t key, spt_info_t * psplit, rotate_t * protate){
	btnode_t * pleft = NULL;
	btnode_t * pright = NULL;
	btnode_t * pmid = NULL;
	int lnum = 0x0FFFFFFF;
	if(NULL != path_node.pleft){
		pleft = path_node.pleft;
		lnum = pleft->bn_leaf.bn_num;
	}
	int rnum = 0x0FFFFFFF;
	if(NULL != path_node.pright){
		pright = path_node.pright;
		rnum = pright->bn_leaf.bn_num;
	}
	pmid = path_node.pmid;
	int idx = path_node.idx;
	if(pmid->bn_leaf.bn_num >= LEAF_KEY_NUM){
		if((lnum >= LEAF_KEY_NUM)&&
			(rnum >= LEAF_KEY_NUM)){
			btnode_t * pnewNode = NULL;
			if(BTREE_FAIL == bt_leaf_split(pmid, &pnewNode)){
				return IST_FAIL;
			}
			if(NULL == pnewNode){
				return IST_FAIL;
			}
			int ret = bt_key_cmp(key, pmid->bn_leaf.bn_data[pmid->bn_leaf.bn_num-1]);
			if(ret < 0){
				bt_leaf_real_add(&pmid->bn_leaf, idx, key);
			}
			else{
				idx = bt_search_leaf(pnewNode, key);
				if(BTREE_FAIL == idx){
					idx = pnewNode->bn_leaf.bn_num-1;
				}
				bt_leaf_real_add(&pnewNode->bn_leaf, idx, key);
			}
			psplit->updkey = pmid->bn_leaf.bn_data[pmid->bn_leaf.bn_num-1];
			psplit->newidx.bn_key = pnewNode->bn_leaf.bn_data[pnewNode->bn_leaf.bn_num-1];
			psplit->newidx.bn_ptr.bn_ptr = pnewNode;
			return IST_SPLIT;
		}
		else{
			bt_leaf_rotate(pleft, pmid, pright, idx, key, protate, ROT_ADD);
			return IST_ROTATE;
		}
	}

	int oldnum = pmid->bn_leaf.bn_num;
	btk_t oldkey = pmid->bn_leaf.bn_data[oldnum-1];
	bt_leaf_real_add(&pmid->bn_leaf, idx, key);
	if(pmid->bn_leaf.bn_num-oldnum < 1){
		log_debug("Add data num:%d failed, please check!\n",pmid->bn_leaf.bn_num-oldnum);
		return IST_FAIL;
	}
	else if(bt_key_cmp(oldkey, pmid->bn_leaf.bn_data[pmid->bn_leaf.bn_num-1])<0){
		protate->updkey = pmid->bn_leaf.bn_data[pmid->bn_leaf.bn_num-1];
		protate->leftkey.bn_key = 0;
		protate->rightkey.bn_key = 0;
		return IST_UPDATE;
	}
	return IST_OK;
}

static inline void bt_leaf_real_del(btleaf_t * pleaf, btk_t key){
	int i, j;
	for(i=0; i<pleaf->bn_num; i++){
		if(bt_key_cmp(key, pleaf->bn_data[i]) != 0)
			continue;
		else{
			for(j=i+1; j<pleaf->bn_num; j++){
				pleaf->bn_data[j-1] = pleaf->bn_data[j];
			}
			pleaf->bn_num--;
			break;
		}
	}
}

/*del array elem for leaf node*/
delv_t bt_leaf_del(btnode_t * pNode, btk_t key){
	bt_leaf_real_del(&pNode->bn_leaf, key);
	if(pNode->bn_leaf.bn_num >= LEAF_KEY_NUM/2){
		return DEL_OK;
	}

	return DEL_FAIL;
}

void bt_leaf_merge(btnode_t * pNode0, btnode_t * pNode1, btnode_t * pNode2, merge_t * pinfo){
	btleaf_t * pleft = NULL;
	if(NULL != pNode0) pleft = &pNode0->bn_leaf;
	btleaf_t * pmid = &pNode1->bn_leaf;
	btleaf_t * pright = NULL;
	if(NULL != pNode2) pright = &pNode2->bn_leaf;
	int move_num = 0;
	/*merge left node*/
	if(pright == NULL || (pright != NULL && pleft != NULL && pright->bn_num >= pleft->bn_num)){
		if(NULL != pleft->lnode.prev){
			pleft->lnode.prev->next = pleft->lnode.next;
		}
		if(NULL != pleft->lnode.next){
			pleft->lnode.next->prev = pleft->lnode.prev;
		}
		move_num = pleft->bn_num;
		bt_leaf_move2right(pleft,pmid,move_num);
		pinfo->updkey = pmid->bn_data[pmid->bn_num-1];
		pinfo->node = LEFT;
	}
	/*merge right node*/
	else if(pleft == NULL || ((pright != NULL && pleft!= NULL) && pleft->bn_num >= pright->bn_num)){
		if(NULL != pright->lnode.prev){
			pright->lnode.prev->next = pright->lnode.next;
		}
		if(NULL != pright->lnode.next){
			pright->lnode.next->prev = pright->lnode.prev;
		}
		move_num = pright->bn_num;
		bt_leaf_move2left(pright,pmid, move_num);
		pinfo->updkey = pmid->bn_data[pmid->bn_num-1];
		pinfo->node = RIGHT;
	}
}

static inline int bt_branch_update(btnode_t * pNode, int idx){
	if((0 > idx)||(pNode->bn_inter.bn_num <= idx))
		return IST_OK;
	btnode_t * tmpSon = pNode->bn_inter.bn_entry[idx].bn_ptr.bn_ptr;
	if(TYPE_INT == tmpSon->bn_type){
		pNode->bn_inter.bn_entry[idx].bn_key = tmpSon->bn_inter.bn_entry[tmpSon->bn_inter.bn_num-1].bn_key;
	}
	else{
		pNode->bn_inter.bn_entry[idx].bn_key = tmpSon->bn_leaf.bn_data[tmpSon->bn_leaf.bn_num-1];
	}
	if(idx >= pNode->bn_inter.bn_num-1)
		return IST_UPDATE;
	return IST_OK;
}

int bt_insert(bptree_t * ptree, btk_t key){
	btpath_t path[BT_LVL_ROOT] = {0};
	btnode_t * tmpNode = ptree->bpt_root;
	btnode_t * tmpLeft = NULL;
	btnode_t * tmpRight = NULL;
	int idx = 0;
	int i;
	for(i=0; i<ptree->bpt_level; i++){
		path[i].pleft = tmpLeft;
		path[i].pmid = tmpNode;
		path[i].pright = tmpRight;
		if(path[i].pmid->bn_type != TYPE_LEAF){
			path[i].idx = bt_search_branch(path[i].pmid, key);
			if(path[i].idx == BTREE_FAIL){
				path[i].idx = path[i].pmid->bn_inter.bn_num-1;
			}
			if(path[i].idx > 0){
				tmpLeft = path[i].pmid->bn_inter.bn_entry[path[i].idx-1].bn_ptr.bn_ptr;
			}
			else if(NULL != tmpLeft){
				tmpLeft = tmpLeft->bn_inter.bn_entry[tmpLeft->bn_inter.bn_num-1].bn_ptr.bn_ptr;
			}
			tmpNode = path[i].pmid->bn_inter.bn_entry[path[i].idx].bn_ptr.bn_ptr;
			if(path[i].idx < path[i].pmid->bn_inter.bn_num-1){
				tmpRight = path[i].pmid->bn_inter.bn_entry[path[i].idx+1].bn_ptr.bn_ptr;
			}
			else if(NULL != tmpRight){
				tmpRight = tmpRight->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
			}
		}
		else{
			path[i].idx = bt_search_leaf(tmpNode, key);
			if(path[i].idx == BTREE_FAIL){
				if(path[i].pmid->bn_leaf.bn_num == 0)
					path[i].idx = 0;
				else
					path[i].idx = path[i].pmid->bn_leaf.bn_num-1;
			}
			else if(bt_key_cmp(key, tmpNode->bn_leaf.bn_data[path[i].idx]) == 0){
				log_debug("The key you want insert is exist!\n");
				return BTREE_OK;
			}
		}
	}

	spt_info_t spt_info = {0};
	rotate_t rot_info = {0};
	istv_t ret = IST_FAIL;
	btnode_t * pleft = NULL;
	btnode_t * pright = NULL;
	btnode_t * pmid = NULL;
	for(i=ptree->bpt_level-1; i>=0; i--){
		if(IST_OK == ret){
			break;
		}
		if(TYPE_LEAF == path[i].pmid->bn_type){
			ret = bt_leaf_add(path[i], key, &spt_info, &rot_info);
		}
		else{
			if(path[i].idx == 0){
				if(NULL != path[i].pleft){
					pleft = path[i].pleft->bn_inter.bn_entry[path[i].pleft->bn_inter.bn_num-1].bn_ptr.bn_ptr;
				}
				else{
					pleft = NULL;
				}
			}
			else{
				pleft = (btnode_t *)path[i].pmid->bn_inter.bn_entry[path[i].idx-1].bn_ptr.bn_ptr;
			}
			if(path[i].idx >= path[i].pmid->bn_inter.bn_num-1){
				if(NULL != path[i].pright){
					pright = path[i].pright->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
				}
				else{
					pright = NULL;
				}
			}
			else{
				pright = (btnode_t *)path[i].pmid->bn_inter.bn_entry[path[i].idx+1].bn_ptr.bn_ptr;
			}
			pmid = (btnode_t *)path[i].pmid->bn_inter.bn_entry[path[i].idx].bn_ptr.bn_ptr;
			if(IST_SPLIT== ret){
				bt_branch_update(path[i].pmid, path[i].idx);
				ret = bt_branch_add(path[i].pmid, spt_info.newidx, &spt_info, &rot_info);
			}
			else if((IST_UPDATE == ret) || (IST_ROTATE == ret)){
				if(0 != rot_info.leftkey.bn_key){
					if(0 == path[i].idx){
						if(NULL != path[i].pleft){
							ret += bt_branch_update(path[i].pleft, path[i].pleft->bn_inter.bn_num-1);
						}
					}
					else{
						ret += bt_branch_update(path[i].pmid, path[i].idx-1);
					}
				}
				
				if(0 != rot_info.updkey.bn_key){
					ret += bt_branch_update(path[i].pmid, path[i].idx);
				}

				if(0 != rot_info.rightkey.bn_key){
					if(path[i].pmid->bn_inter.bn_num-1 == path[i].idx){
						if(NULL != path[i].pright){
							ret += bt_branch_update(path[i].pright, 0);
						}
					}
					else{
						ret += bt_branch_update(path[i].pmid, path[i].idx+1);
					}
				}
				ret = ret==IST_OK?IST_OK:IST_UPDATE;
			}
			else{
				break;
			}
		}
	}
	if(IST_SPLIT == ret){
		/*in this situation, need increasing new level*/
		btnode_t * newroot = bt_new_node(TYPE_INT);
		btnode_t * newbranch = (btnode_t *)spt_info.newidx.bn_ptr.bn_ptr;
		btnode_t * tmpNode = ptree->bpt_root;
		btp_t ptr;
		if(NULL == newroot){
			log_debug("alloc new root for increase new level failed!\n");
			return BTREE_FAIL;
		}
		newroot->bn_level = BT_LVL_ROOT;
		if(tmpNode->bn_type == TYPE_LEAF){
			tmpNode->bn_level = BT_LVL_LEAF;
			newbranch->bn_level = BT_LVL_LEAF;
		}
		else{
			tmpNode->bn_level = BT_LVL_INT;
			newbranch->bn_level = BT_LVL_INT;
		}
		newroot->bn_inter.bn_entry[0].bn_key = spt_info.updkey;
		newroot->bn_inter.bn_entry[0].bn_ptr.bn_ptr = tmpNode;
		newroot->bn_inter.bn_entry[1] = spt_info.newidx;
		newroot->bn_inter.bn_num = 2;
		ptree->bpt_level++;
		ptree->bpt_root = newroot;
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
	btnode_t * tmpLeft = NULL;
	btnode_t * tmpRight = NULL;
	int i,j;
	for(i=0; i<ptree->bpt_level; i++){
		path[i].pleft = tmpLeft;
		path[i].pmid = tmpNode;
		path[i].pright = tmpRight;
		if(tmpNode->bn_type != TYPE_LEAF){
			path[i].idx = bt_search_branch(tmpNode, key);
			if(path[i].idx != BTREE_FAIL){
				if(path[i].idx > 0){
					tmpLeft = tmpNode->bn_inter.bn_entry[path[i].idx-1].bn_ptr.bn_ptr;
				}
				else if(NULL != tmpLeft){
					tmpLeft = tmpLeft->bn_inter.bn_entry[tmpLeft->bn_inter.bn_num-1].bn_ptr.bn_ptr;
				}
				
				if(path[i].idx < tmpNode->bn_inter.bn_num-1){
					tmpRight = tmpNode->bn_inter.bn_entry[path[i].idx+1].bn_ptr.bn_ptr;
				}
				else if(NULL != tmpRight){
					tmpRight = tmpRight->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
				}

				tmpNode = tmpNode->bn_inter.bn_entry[path[i].idx].bn_ptr.bn_ptr;
			}
			else{
				path[i].idx = tmpNode->bn_inter.bn_num-1;
			}
		}
		else{
			path[i].idx = bt_exactsearch_leaf(tmpNode, key);
		}
		if(path[i].idx == BTREE_FAIL){
			log_debug("The key you want delete is not exist!\n");
			return BTREE_OK;
		}
		log_debug("path[%d].idx:%ld\n",i,path[i].idx);
	}

	rotate_t rot_info = {0};
	merge_t mer_info = {0};
	delv_t ret = DEL_FAIL;
	int num[ORDNUM] = {0};
	btnode_t * pleft = NULL;
	btnode_t * pright = NULL;
	btnode_t * pmid = NULL;
	int total = 0;
	int bc_num = 0;

	for(i=ptree->bpt_level-1; i>=0; i--){
		if(DEL_OK == ret)
			break;
		if(TYPE_LEAF == path[i].pmid->bn_type){
			ret = bt_leaf_del(path[i].pmid, key);
		}
		else{
			total = 0;
			bc_num = 0;
			if(0 == path[i].idx){
				if(NULL != path[i].pleft){
					pleft = path[i].pleft->bn_inter.bn_entry[path[i].pleft->bn_inter.bn_num-1].bn_ptr.bn_ptr;
				}
				else{
					pleft = NULL;
				}
			}
			else{
				pleft = (btnode_t *)path[i].pmid->bn_inter.bn_entry[path[i].idx-1].bn_ptr.bn_ptr;
			}
			if(NULL != pleft){
				if(TYPE_INT == pleft->bn_type){
					total += pleft->bn_inter.bn_num;
				}
				else{
					total += pleft->bn_leaf.bn_num;
				}
				bc_num += 1;
			}
			if(path[i].idx >= path[i].pmid->bn_inter.bn_num-1){
				if(NULL != path[i].pright){
					pright = path[i].pright->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
				}
				else{
					pright = NULL;
				}
			}
			else{
				pright = (btnode_t *)path[i].pmid->bn_inter.bn_entry[path[i].idx+1].bn_ptr.bn_ptr;
			}
			if(NULL != pright){
				if(TYPE_INT == pright->bn_type){
					total += pright->bn_inter.bn_num;
				}
				else{
					total += pright->bn_leaf.bn_num;
				}
				bc_num += 1;
			}
			pmid = (btnode_t *)path[i].pmid->bn_inter.bn_entry[path[i].idx].bn_ptr.bn_ptr;
			if(pmid->bn_type == TYPE_LEAF){
				total += pmid->bn_leaf.bn_num;
			}
			else{
				total += pmid->bn_inter.bn_num;
			}
			bc_num += 1;
			if(DEL_UPDATE != ret){
				//if son node is inter node
				if(TYPE_INT == pmid->bn_type){
					if(total < (bc_num * (INTER_KEY_NUM/2))){
						ret = DEL_OK;
						bt_branch_merge(pleft, pmid, pright, &mer_info);
						if(LEFT == mer_info.node){
							if(0 == path[i].idx){
								if(NULL != path[i].pleft){
									ret = bt_branch_del(path[i].pleft, path[i].pleft->bn_inter.bn_num-1);
								}
							}
							else{
								ret = bt_branch_del(path[i].pmid, path[i].idx-1);
							}
						}
						else if(RIGHT == mer_info.node){
							if(path[i].idx >= path[i].pmid->bn_inter.bn_num-1){
								if(NULL != path[i].pright){
									ret = bt_branch_del(path[i].pright, 0);
								}
							}
							else{
								ret = bt_branch_del(path[i].pmid, path[i].idx+1);
							}
						}
						ret += bt_branch_update(path[i].pmid, path[i].idx);
						if(ret > DEL_FAIL)
							ret = DEL_FAIL;
						else if(ret == DEL_UPDATE){
							rot_info.updkey = path[i].pmid->bn_inter.bn_entry[path[i].pmid->bn_inter.bn_num-1].bn_key;
							rot_info.leftkey.bn_key = 0;
							rot_info.rightkey.bn_key = 0;
						}
						continue;
					}
					else{
						ret = DEL_OK;
						log_debug("total: %d node num: %d\n", total, bc_num);
						bt_branch_rotate(pleft, pmid, pright, path[i].idx, &rot_info, ROT_DEL);
					}
				}
				//if son node is leaf
				else{
					if(total < (bc_num * (LEAF_KEY_NUM/2))){
						bt_leaf_merge(pleft, pmid, pright, &mer_info);
						ret = bt_branch_update(path[i].pmid, path[i].idx);
						if(LEFT == mer_info.node){
							if(0 == path[i].idx){
								if(NULL != path[i].pleft){
									ret += bt_branch_del(path[i].pleft, path[i].pleft->bn_inter.bn_num-1);
								}
							}
							else{
								ret += bt_branch_del(path[i].pmid, path[i].idx-1);
							}
						}
						else if(RIGHT == mer_info.node){
							if(path[i].idx >= path[i].pmid->bn_inter.bn_num-1){
								if(NULL != path[i].pright){
									ret += bt_branch_del(path[i].pright, 0);
								}
							}
							else{
								ret += bt_branch_del(path[i].pmid, path[i].idx+1);
							}
						}
						if(ret >= DEL_FAIL)
							ret = DEL_FAIL;
						else if(DEL_UPDATE == ret){
							rot_info.updkey = path[i].pmid->bn_inter.bn_entry[path[i].pmid->bn_inter.bn_num-1].bn_key;
							rot_info.leftkey.bn_key = 0;
							rot_info.rightkey.bn_key = 0;
						}
						continue;
					}
					else{
						ret = DEL_OK;
						bt_leaf_rotate(pleft, pmid, pright, path[i].idx, key, &rot_info, ROT_DEL);
					}
				}
			}
			if(TYPE_INT == pmid->bn_type){
				if(0 != rot_info.leftkey.bn_key){
					if(0 == path[i].idx){
						if(NULL != path[i].pleft){
							ret += bt_branch_update(path[i].pleft, path[i].pleft->bn_inter.bn_num-1);
						}
					}
					else{
						ret += bt_branch_update(path[i].pmid, path[i].idx-1);
					}
				}
				if(0 != rot_info.updkey.bn_key){
					ret += bt_branch_update(path[i].pmid, path[i].idx);
				}
				if(0 != rot_info.rightkey.bn_key){
					if(path[i].idx >= path[i].pmid->bn_inter.bn_num-1){
						if(NULL != path[i].pright){
							ret += bt_branch_update(path[i].pright, 0);
						}
					}
					else{
						ret += bt_branch_update(path[i].pmid, path[i].idx+1);
					}
				}
				ret = ret==DEL_OK?DEL_OK:DEL_UPDATE;
			}
			else{
				if(0 != rot_info.leftkey.bn_key){
					log_debug("new left key:%d, path[%d].idx:%d, path[].pleft:%s\n",rot_info.leftkey.bn_key, i, path[i].idx, path[i].pleft==NULL?"NULL":"Not NULL");
					if(0 == path[i].idx){
						if(NULL != path[i].pleft){
							ret += bt_branch_update(path[i].pleft, path[i].pleft->bn_inter.bn_num-1);
						}
					}
					else{
						ret += bt_branch_update(path[i].pmid, path[i].idx-1);
					}
				}
				if(0 != rot_info.updkey.bn_key){
					log_debug("new mid key:%d, path[%d].idx:%d\n", rot_info.updkey.bn_key, i, path[i].idx);
					ret += bt_branch_update(path[i].pmid, path[i].idx);
				}
				if(0 != rot_info.rightkey.bn_key){
					log_debug("new right key:%d, path[%d].idx:%d\n",rot_info.rightkey.bn_key, i, path[i].idx);
					if(path[i].pmid->bn_inter.bn_num-1 <= path[i].idx){
						if(NULL != path[i].pright){
							ret += bt_branch_update(path[i].pright, 0);
						}
					}
					else{
						ret += bt_branch_update(path[i].pmid, path[i].idx+1);
					}
				}
				ret = ret==DEL_OK?DEL_OK:DEL_UPDATE;
			}
		}
	}

	if(DEL_FAIL == ret){
		tmpNode = ptree->bpt_root;
		if(tmpNode->bn_type != TYPE_LEAF){
			if(tmpNode->bn_inter.bn_num <= 1){
				pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
				pmid->bn_level = BT_LVL_ROOT;
				ptree->bpt_root = pmid;
				del(tmpNode);
			}
		}
		else{
			log_debug("there are only %d entries!\n",tmpNode->bn_leaf.bn_num);
		}
	}
	return BTREE_OK;
}

int bt_leaf_check(btnode_t * pNode){
	btleaf_t * pleaf = &(pNode->bn_leaf);
	int i;
	if((LEAF_KEY_NUM < pleaf->bn_num) || (pNode->bn_level != BT_LVL_ROOT && LEAF_KEY_NUM/2 > pleaf->bn_num))
		return BTREE_FAIL;
	btleaf_t * pleft = (btleaf_t *)pleaf->lnode.prev;
	btleaf_t * pright = (btleaf_t *)pleaf->lnode.next;
	if(NULL != pleft){
		if(bt_key_cmp(pleft->bn_data[pleft->bn_num-1], pleaf->bn_data[0]) >= 0)
			return BTREE_FAIL;
	}
	if(NULL != pright){
		if(bt_key_cmp(pright->bn_data[0], pleaf->bn_data[pleaf->bn_num-1]) <= 0)
			return BTREE_FAIL;
	}
	for(i=1; i<pleaf->bn_num; i++){
		if(bt_key_cmp(pleaf->bn_data[i-1], pleaf->bn_data[i]) >= 0)
			return BTREE_FAIL;
	}
	return BTREE_OK;
}

int bt_branch_check(btinter_t * pinter, btlvl_t level){
	int i;
	btnode_t * sonNode = NULL;
	btnode_t * nextSon = NULL;
	btk_t maxkey = {0};
	btk_t minkey = {0};
	if((INTER_KEY_NUM < pinter->bn_num) ||
		(0 >= pinter->bn_num) || 
		(BT_LVL_INT == level && INTER_KEY_NUM/2 > pinter->bn_num))
		return BTREE_FAIL;
	for(i=0; i<pinter->bn_num; i++){
		if(i<pinter->bn_num-1){
			if(bt_key_cmp(pinter->bn_entry[i].bn_key, pinter->bn_entry[i+1].bn_key) >= 0)
				return BTREE_FAIL;
			nextSon = pinter->bn_entry[i+1].bn_ptr.bn_ptr;
		}
		else{
			nextSon = NULL;
		}
		sonNode = pinter->bn_entry[i].bn_ptr.bn_ptr;
		if(TYPE_INT == sonNode->bn_type){
			maxkey = sonNode->bn_inter.bn_entry[sonNode->bn_inter.bn_num-1].bn_key;
			if(NULL != nextSon){
				minkey = nextSon->bn_inter.bn_entry[0].bn_key;
			}
			else{
				minkey.bn_key = 0x7FFFFFFFF;
			}
		}
		else{
			maxkey = sonNode->bn_leaf.bn_data[sonNode->bn_leaf.bn_num -1];
			if(NULL != nextSon){
				minkey = nextSon->bn_leaf.bn_data[0];
			}
			else{
				minkey.bn_key = 0x7FFFFFFFF;
			}
		}
		if(bt_key_cmp(maxkey, pinter->bn_entry[i].bn_key) > 0)
			return BTREE_FAIL;
		if(bt_key_cmp(minkey, pinter->bn_entry[i].bn_key) <= 0){
			return BTREE_FAIL;
		}
	}
	return BTREE_OK;
}

int bt_tree_check(bptree_t * ptree){
	aque_t queue;
	btnode_t * pthis = NULL;
	btnode_t * pnext = NULL;
	aque_init(&queue);
	aq_push(&queue,ptree->bpt_root);
	while(queue.len>0){
		pthis = aq_pop(&queue);
		if(TYPE_INT == pthis->bn_type){
			pnext = aq_head(&queue);
			if(NULL != pnext){
				if(bt_key_cmp(pthis->bn_inter.bn_entry[pthis->bn_inter.bn_num-1].bn_key, pnext->bn_inter.bn_entry[0].bn_key) >= 0)
					return BTREE_FAIL;
			}
			if(bt_branch_check(&pthis->bn_inter, pthis->bn_level) == BTREE_FAIL)
				return BTREE_FAIL;
			else{
				int i;
				for(i=0; i<pthis->bn_inter.bn_num; i++){
					aq_push(&queue, pthis->bn_inter.bn_entry[i].bn_ptr.bn_ptr);
				}
			}
		}
		else{
			if(bt_leaf_check(pthis) == BTREE_FAIL)
				return BTREE_FAIL;
		}
	}
	return BTREE_OK;
}

void bt_init(bptree_t * ptree){
	ptree->bpt_leaf_nums = LEAF_KEY_NUM;
	ptree->bpt_branch_nums = INTER_KEY_NUM;
	ptree->bpt_level = 1;
	ptree->bpt_root = bt_new_node(TYPE_LEAF);
	ptree->bpt_root->bn_level = BT_LVL_ROOT;
	ptree->bpt_root->bn_type = TYPE_LEAF;
	ptree->bpt_root->bn_leaf.bn_num = 0;
}

#if 1

int get_tree_leaf_num(btnode_t * proot){
	btleaf_t * pleaf = NULL;
	btnode_t * pNode = proot;
	while(pNode->bn_type != TYPE_LEAF){
		pNode = (btnode_t *)pNode->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
	}
	pleaf = &pNode->bn_leaf;
	struct list_head * plnode = &pleaf->lnode;
	int num = pleaf->bn_num;
	while(plnode->next != NULL){
		plnode = plnode->next;
		pleaf = (btleaf_t *)plnode;
		num += pleaf->bn_num;
	}
	return num;
}

void print_btree_info(bptree_t * ptree){
	log_debug("B+ Tree level num:%d\n", ptree->bpt_level);
	log_debug("B+ Tree branch node key num:%d\n", ptree->bpt_branch_nums);
	log_debug("B+ Tree leaf node key num:%d\n", ptree->bpt_leaf_nums);
	log_debug("B+ Tree have %d datas\n", get_tree_leaf_num(ptree->bpt_root));
}

void print_leaf(btleaf_t * pleaf, char * prefix){
	if(pleaf->bn_num <= 0){
		log_debug("There is no data, please insert!\n");
		return;
	}
	//log_debug("leaf entry num:%d!\n", pleaf->bn_num);
	int i = 0;
	for(i=0; i<pleaf->bn_num-1; i++){
		printf("%s├────%ld\r\n",prefix, pleaf->bn_data[i].bn_key);
	}
	printf("%s└────%ld\r\n",prefix, pleaf->bn_data[i].bn_key);
}

void print_btree(bptree_t * ptree)
{
	if(NULL == ptree)
	{
		return;
	}

	btpath_t path[BT_LVL_ROOT] = {0};
	int cusor = 0;
	//压栈
	path[cusor].pmid = ptree->bpt_root;
	path[cusor].idx = 0;
	cusor++;
	btnode_t * tmpNode = NULL;
	btp_t ptr;
	char prefix[100] = {0};
	int pre_idx = 0;
	printf("Root\n");
	while(cusor){
		//出栈
		tmpNode = path[cusor-1].pmid;
		if(TYPE_LEAF != tmpNode->bn_type){
			if(path[cusor-1].idx < tmpNode->bn_inter.bn_num){
				if(path[cusor-1].idx == tmpNode->bn_inter.bn_num-1){
					printf("%s└────%ld\n", prefix, tmpNode->bn_inter.bn_entry[path[cusor-1].idx].bn_key.bn_key);
					sprintf(prefix,"%s          ", prefix);
				}
				else{
					printf("%s├────%ld\n", prefix, tmpNode->bn_inter.bn_entry[path[cusor-1].idx].bn_key.bn_key);
					sprintf(prefix,"%s│         ", prefix);
				}
				//入栈
				path[cusor].pmid = (btnode_t *)tmpNode->bn_inter.bn_entry[path[cusor-1].idx].bn_ptr.bn_ptr;
				path[cusor-1].idx ++;
				path[cusor].idx = 0;
				cusor ++;
				pre_idx += 8;
				//printf("pre_idx:%d\n", pre_idx);
				prefix[pre_idx] = 0;
			}
			else{
				cusor--;
				pre_idx -= 8;
				prefix[pre_idx] = 0;
			}
		}
		else{
			print_leaf(&tmpNode->bn_leaf, prefix);
			cusor --;
			pre_idx -= 8;
			prefix[pre_idx] = 0;
		}
	}
}
#endif

void write_node(btnode_t * pNode, int fd, int num){
	if(NULL == pNode)
		return;
	char buf[1024] = {0};
	int i = 0;
	if(pNode->bn_type == TYPE_INT){
		sprintf(buf, "\n%d ### INT %d :", num, pNode->bn_level);
		//printf("%s\n",buf);
		for(i=0; i<pNode->bn_inter.bn_num; i++){
			sprintf(buf, "%s - [ %ld : %ld ]", buf, (unsigned long)(pNode->bn_inter.bn_entry[i].bn_ptr.bn_ptr), pNode->bn_inter.bn_entry[i].bn_key.bn_key);
			//printf("%s\n",buf);
		}
		//sprintf(buf,"\n",buf);
	}
	else{
		sprintf(buf, "\n%d ### LEAF %d :", num, pNode->bn_level);
		for(i=0; i<pNode->bn_leaf.bn_num; i++){
			sprintf(buf, "%s - %ld", buf, pNode->bn_leaf.bn_data[i].bn_key);
		}
		//sprintf(buf,"\n",buf);
	}
	printf("%s\n", buf);
	write(fd, buf, strlen(buf));
}

void dump_tree(bptree_t * ptree, int fd){
	btnode_t * pNode = NULL;
	char buf[NODE_SIZE];
	btnode_t * tmpNode = (btnode_t *)buf;
	aque_t queue;
	unsigned long long offset = 0;
	int num = 0;
	sprintf(buf,"\nData count:%d", get_tree_leaf_num(ptree->bpt_root));
	write(fd, buf, strlen(buf));
	aque_init(&queue);
	aq_push(&queue, ptree->bpt_root);
	while(queue.len > 0){
		pNode = aq_pop(&queue);
		if(pNode->bn_type == TYPE_INT){
			memcpy(buf, pNode, NODE_SIZE);
			int i;
			for(i=0; i<pNode->bn_inter.bn_num; i++){
				offset += 1;
				aq_push(&queue, pNode->bn_inter.bn_entry[i].bn_ptr.bn_ptr);
				tmpNode->bn_inter.bn_entry[i].bn_ptr.bn_ptr = (void *)offset;
			}
			pNode = tmpNode;
		}
		write_node(pNode, fd, num++);
	}
}

void load_tree(bptree_t * ptree, char * tree_name){
	char line[200] = {0};
	char type[5] = {0};
	int level = 0;
	if(ptree->bpt_root != NULL){
		return;
	}
	FILE * pfile = open(tree_name, "r");
	if(NULL == pfile){
		return;
	}
	btnode_t * tmpNode = NULL;
	while(NULL != fgets(line, 199, pfile)){
		sscanf(line, "*### %s %d :*", type, level);
		if(strcmp(type, "INT") == 0){
			if(level == BT_LVL_ROOT){
				tmpNode = bt_new_node(TYPE_INT);
				tmpNode->bn_level = BT_LVL_ROOT;
				ptree->bpt_root = tmpNode;
			}
			else{
				tmpNode = bt_new_node(TYPE_INT);
				tmpNode->bn_level = BT_LVL_INT;
			}
			
		}
		else{//leaf line
			
		}
	}
}


void save_tree_data(bptree_t * ptree, int fd){
	char buf[12];
	btleaf_t * pleaf = NULL;
	btnode_t * pNode = ptree->bpt_root;
	while(pNode->bn_type != TYPE_LEAF){
		pNode = (btnode_t *)pNode->bn_inter.bn_entry[0].bn_ptr.bn_ptr;
	}
	pleaf = &pNode->bn_leaf;
	while(pleaf != NULL){
		int i;
		memset(buf, 0, 12);
		for(i=0; i<pleaf->bn_num; i++){
			sprintf(buf, "%ld\n", pleaf->bn_data[i].bn_key);
			write(fd, buf, strlen(buf));
		}
		pleaf = (btleaf_t *)pleaf->lnode.next;
	}
}

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
	int invalid = 0;
	unsigned long ikey = 0;
	bptree_t tree = {0};
	bt_init(&tree);
	print_prompt();
	print_eof();
	int idump = open("insert.txt", O_WRONLY|O_CREAT, S_IRWXU);
	int ddump = open("delete.txt", O_WRONLY|O_CREAT, S_IRWXU);
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
				dump_tree(&tree, idump);
				if(BTREE_OK == bt_insert(&tree,key))
					log_debug("Insert :%ld success!\n", key.bn_key);
				else
					log_debug("Insert:%ld failed!\n", key.bn_key);
				if(BTREE_FAIL == bt_tree_check(&tree)){
					if(ikey == 0){
						invalid = 1;
						ikey = key.bn_key;
						print_btree(&tree);
						log_debug("Insert:%ld lead to Tree invalid, please check!\n",ikey);
					}
				}
				print_eof();
				break;
			case 'd':
				//delete
				log_debug("\n please input node id which you want delete:");
				setbuf(stdin, NULL);
				scanf("%ld", &key.bn_key);
				setbuf(stdin, NULL);
				dump_tree(&tree, ddump);
				if(BTREE_OK == bt_del(&tree,key))
					log_debug("Delete :%ld success!\n", key.bn_key);
				else
					log_debug("Delete:%ld failed!\n", key.bn_key);
				if(BTREE_FAIL == bt_tree_check(&tree)){
					if(ikey == 0){
						ikey = key.bn_key;
						print_btree(&tree);
					}
					log_debug("Delete:%ld lead to Tree invalid, please check!\n",ikey);
				}
				print_eof();
				break;
			case 'q':
				//quit
				dump_tree(&tree, idump);
				close(idump);
				dump_tree(&tree, ddump);
				close(ddump);
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

			case 's':
				//show
				log_debug("***show B+ tree base info as follow*****\n");
				print_btree_info(&tree);
				if(1 == invalid)
					log_debug("Tree invalid!\n");
				print_eof();
				break;

			case 'f':
				//flush
				log_debug("***flush B+ tree to file*****\n");
				int treeinfo = open("tree_data.txt", O_RDONLY);
				if(treeinfo > 0){
					close(treeinfo);
					remove("tree_data.txt");
				}
				treeinfo = open("tree_data.txt", O_WRONLY|O_CREAT, S_IRWXU);
				save_tree_data(&tree, treeinfo);
				close(treeinfo);
				print_eof();
				break;

			default:
				break;
		};
	}
	return 0;
}
