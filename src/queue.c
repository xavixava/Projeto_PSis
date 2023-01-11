/******************************************************************************
* (c) 2010-2020 AED Team

* Last mod: abl 2020-05-26
*
* DESCRIPTION
*	Auxiliary functions for Tree lab - QUEUE
*
******************************************************************************/
#include<stdio.h>
#include<stdlib.h>

#include "queue.h"

struct _element {
  void *this;
  struct _element *next;
};

struct _queue {
  Element *first;
  Element *last;
};


/******************************************************************************
 * QueueNew()
 *
 * Arguments:
 *
 * Returns: a pointer to Queue
 * Side-Effects: none
 *
 * Description: creates an empty queue
 *
 *****************************************************************************/

Queue *QueueNew()
{
  Queue *q = (Queue *) malloc(sizeof(Queue));
  q->first = NULL;
  q->last = NULL;
  return q;
}


/******************************************************************************
 * InsertLast()
 *
 * Arguments: queue - a queue
 *            this - a "node" to insert last in the queue
 *
 * Returns: void
 * Side-Effects: none
 *
 * Description: inserts an element at the end of a queue
 *
 *****************************************************************************/

void InsertLast(Queue* queue, void *this)
{
  Element *elem;

  if (queue == NULL || this == NULL)
  // if (queue == NULL || this <= 0)
     return;

  elem = malloc(sizeof(Element));
  elem->this = this;
  elem->next = NULL;
  if (queue == NULL)
     return;
  if (queue->last != NULL) queue->last->next = elem;
	queue->last = elem;
  if (queue->first == NULL)
     queue->first = elem;

  return;
}

int IsEmpty(Queue *q){
	if (q->first==NULL){
		return 0;
	}
	return 1;
}

void freeQueue(Queue *q){
	/*Element *e = q->last;
	free(e);*/
	free (q);
	return;
}

/******************************************************************************
 * GetFirst()
 *
 * Arguments: queue - a queue
 *
 * Returns: pointer to first node in queue
 * Side-Effects: none
 *
 * Description: prints the tree in the Breathfirst format
 *
 *****************************************************************************/

void *GetFirst(Queue* queue)
{
  Element *q = queue->first;
  void *this;
  this = NULL;

  if (queue->first != NULL) {
	 if(queue->first==queue->last)queue->last=NULL;
     queue->first = queue->first->next;

     this = q->this;
     free(q);
  }

  return this;
}


void InsertFirst(Queue* queue, void *this)
{
  Element *elem;

  if (queue == NULL)
     return;

  elem = malloc(sizeof(Element));
  elem->this = this;
  elem->next = NULL;
  if (queue == NULL)
     return;
  if (queue->first != NULL)
     elem->next = queue->first;
  queue->first = elem;
  if (queue->last == NULL)
     queue->last = elem;

  return;
}
