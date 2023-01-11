/******************************************************************************
 *
 * File Name: queue.h
 *	      (c) 2010 AED
 * Author:    AED team
 * Revision:  v2.3 ACR
 *            v4.0 AED Team, Nov 2019
 *
 * NAME
 *     queue.h - prototypes
 *
 * SYNOPSIS
 *     #include <stdlib.h>
 *     #include <stdio.h>
 *     #include <string.h>
 *
 * DESCRIPTION
 *		Definitions useful to implement breath scan
 *
 * DIAGNOSTICS
 *
 *****************************************************************************/

#ifndef QUEUE
#define QUEUE

typedef struct _queue Queue;
typedef struct _element Element;


Queue *QueueNew();               /* creates empty list */
void *GetFirst(Queue* queue);           /* get first element of the queue */
void InsertLast(Queue* queue, void *this);   /* insertion at the end of queue */
int IsEmpty(Queue *q);
void freeQueue(Queue *q);
void InsertFirst(Queue* queue, void *this);

#endif

