#include <string.h>
#include "vector.h"

void aque_init(aque_t * queue){
    memset(queue, 0, sizeof(aque_t));
    queue->f = 0;
    queue->len = 0;
}

void * aq_head(aque_t * queue){
	if(queue->len > 0)
		return queue->pque[queue->f];
	return NULL;
}

void * aq_pop(aque_t * queue){
    int idx = queue->f;
    if(queue->len == 0)
        return NULL;
    queue->f = (queue->f+1)>(STK_QUE_LEN-1)?0:(queue->f+1);
    queue->len --;
    return queue->pque[idx];
}

void aq_push(aque_t * queue, void * ptr){
    int idx = queue->f + queue->len;
    if(queue->len < STK_QUE_LEN)
        queue->len ++;
    else
        queue->f = (queue->f-1)>0?(queue->f-1):(STK_QUE_LEN-1);
    queue->pque[idx] = ptr;
}

void lque_init(lque_t * queue){
	if(NULL != queue)
		queue->head = NULL;
}

void * lq_pop(lque_t * queue){
	struct list_head * pNode = NULL;
	if(queue->head == NULL)
		return NULL;
	else if(queue->head->next == queue->head){
		pNode = queue->head;
		queue->head = NULL;
	}
	list_del(pNode);
	return (void *)pNode;
}

void lq_push(lque_t * queue, void * ptr){
    struct list_head * pNode = (struct list_head *)ptr;
	list_init(pNode);
	if(NULL == queue->head)
		queue->head = pNode;
	else
		list_add(pNode, queue->head->prev);
}

void astack_init(astk_t * stack){
    memset(stack, 0, sizeof(astk_t));
    stack->button = 0;
    stack->top = -1;
}

void * as_pop(astk_t * stack){
    int idx = stack->top;
    if(0 > idx)
        return NULL;
    --stack->top;
    return stack->pstack[idx];
}

void as_push(astk_t * stack, void * ptr){
    if(STK_QUE_LEN-1 <= stack->top)
        return;
    ++stack->top;
    stack->pstack[stack->top] = ptr;
}

void * as_top(astk_t * stack){
    return stack->pstack[stack->top];
}

void lstack_init(lstack_t * stack){
	if(NULL != stack)
		stack->top = NULL;
}

void * ls_pop(lstack_t * stack){
	if(NULL == stack->top)
		return NULL;
	struct list_head * pNode = stack->top;
	stack->top = pNode->next;
	stack->top->prev = NULL;
	return (void *)pNode;
}

void ls_push(lstack_t * stack, void * ptr){
	struct list_head * pNode = (struct list_head *)ptr;
	stack->top->prev = pNode;
	pNode->prev = NULL;
	pNode->next = stack->top;
	stack->top = pNode;
}

void * ls_top(lstack_t * stack){
    return (void *)stack->top;
}

void * list_data(struct list_head * pNode){
	return (void *)((char *)pNode+sizeof(struct list_head));
}
