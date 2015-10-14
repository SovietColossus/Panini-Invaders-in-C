/*
 * Filename: queue.c
 * Created by: Taylor Ball 
 * Created on: 04/03/2015
 * Last Modified by: Alexander Asereno
 * Last Modified on: 04/08/2015
 * Description: Implements a circular queue of a specified
 * buffer size... initialize, enqueue, dequeue, and the checks
 * with them
 */

#include <stdio.h>
#include "queue.h"

void init_queue(queue_t *buf) {
	buf->head = 0;
	buf->tail = 0;
}

int enqueue (queue_t *buf, int data) {
	if((buf->head)+1 % (QUEUE_SIZE) == (buf->tail))
		return 0;
	else{
		buf->buffer[buf->head] = data;
		buf->head = ((buf->head) + 1) % (QUEUE_SIZE);
		return 1;
	}
}

int dequeue (queue_t *buf) {
	if(buf->head==buf->tail)
		return 0;
	else{
		int temp = buf->tail;
		buf->tail = ((buf->tail) + 1) % (QUEUE_SIZE);
		return buf->buffer[temp];
	}
}

int queue_empty(queue_t *buf) {
	return (buf->head==buf->tail);
}
