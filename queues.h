#include <termios.h>
#include <curses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


#define READY 0  /* Queue Identifiers */
#define CPU   0  /* e.g. queue[READY] is READY queue etc. */
#define IO1   1 
#define IO2   2
#define JOB   3
#define FIN   4
#define LTS_IO1 5
#define LTS_IO2 6

#define DATA_FILE "./mp2.data"
#define REPORT_FILE "./mp2.report"

#define MEMORY 64            /* Amount of system memory (in kb) */


static char *Qmessage[7] = { "CPU_WAIT", "IO-1_WAIT",
		      "IO-2_WAIT", "JOB_WAIT", "TERMINATED", 
		      "LTS_IO1_WAIT", "LTS_IO2_WAIT" };
                          /* Messages associated with jobs 
			     waiting in various queues */


static char *ActiveM[3] = { "CPU_ACTIVE", "IO-1_ACTIVE", "IO-2_ACTIVE" };
                          /* Messages associated with active processes */
       
/* All this greatly simplifies displaying data. We can use one big loop */
struct PCB;


/* Main data structure */

struct PCB {
  int proc_num;
  int memory;
  int io_dev;
  int arrival_time;
  int burst[100];
  int predict;
  int current_burst;
  int burst_elapsed;
  int lts_select;
  int terminate;
  int start_wait;
  int wait_total;
  struct PCB *next;
};

struct Q {
  struct PCB *head;
  struct PCB *tail;
};


/* Some generic queue manipulation functions */
/* (in queues.c)

void EnQueue(struct Q *queue, struct PCB *job);
struct PCB *DeQueue(struct Q *queue);
int empty(struct Q *queue);

/* Queue sorting criterias */
#define SIZE 1          /* SIZE = memory size of the program */
#define TIME 2          /* TIME = Arrival time */
#define PREDICT 3       /* PREDICT = predeicted execution time */

int InsertInOrder(struct PCB *new, struct Q *queue, int order);
