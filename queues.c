#include "mp3.h"
#include <stdio.h>
#include <stdlib.h>

/* *********************************************** */
/* QUEUE MANIPULATION ROUTINES (obvious functions) */
/* *********************************************** */

void EnQueue(struct Q *queue, struct PCB *job)

{
  if (queue == NULL) {
    perror("EnQueue error !");
    exit(0);
  }

  if (queue->tail == NULL) {
    (queue->tail) = job;
    (queue->head) = job;
  } else {
    (queue->tail)->next = job;
    (queue->tail) = job;
  }
}

struct PCB *DeQueue(struct Q *queue) {
  struct PCB *tmp;

  tmp = queue->head;

  if (tmp != NULL) {
    queue->head = (queue->head)->next;
  }
  if ((queue->head) == NULL)
    queue->tail = NULL;
  (tmp->next) = NULL;
  return tmp;
}

/* ********************************************************************* */
/* Insert_In_Order inserts into queue by certain criteria */
/* ********************************************************************* */

int InsertInOrder(struct PCB *new, struct Q *queue, int order) {
  struct PCB *curpt, *prevpt;

  new->next = NULL;

  if (queue->head == NULL) {
    queue->head = new;
    queue->tail = new;
    return 1;
  }

  curpt = queue->head;
  prevpt = queue->head;

  while (curpt != NULL) {
    if (((new->arrival_time > curpt->arrival_time) && order == TIME) ||
        ((new->memory > curpt->memory) && order == SIZE) ||
        ((new->predict > curpt->predict) && order == PREDICT)) {
      prevpt = curpt;
      curpt = curpt->next;
    } else {
      new->next = curpt;
      if (curpt == queue->head)
        queue->head = new;
      else
        prevpt->next = new;
      return 1;
    }
  }
  prevpt->next = new;
  queue->tail = new;
  return 1;
}

/* Checks is queue is empty */

int empty(struct Q *queue) {
  if (queue->head == NULL)
    return 1;
  else
    return 0;
}
