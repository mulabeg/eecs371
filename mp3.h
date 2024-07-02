#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define READY 0 /* Queue Identifiers */
#define CPU 0   /* e.g. queue[READY] is READY queue etc. */
#define IO1 1
#define IO2 2
#define JOB 3
#define FIN 4
#define LTS_IO1 5
#define LTS_IO2 6

#define DATA_FILE "./mp3.data"
#define REPORT_FILE "./mp3.report"

#define MEMORY 64 /* Amount of system memory (in kb) */

#define SJF 1 /* define LTS sheduling algorithms */
#define BAL_IO 2
#define RR 1 /* define STS sheduling algorithms */
#define PSJF 2

static char *ltsM[2] = {"SJF", "BAL_IO"};

static char *stsM[2] = {"RR", "PSJF"};

static char *Qmessage[7] = {"CPU_WAIT",    "IO-1_WAIT",  "IO-2_WAIT",
                            "JOB_WAIT",    "TERMINATED", "LTS_IO1_WAIT",
                            "LTS_IO2_WAIT"};
/* Messages associated with jobs
   waiting in various queues */

static char *ActiveM[3] = {"CPU_ACTIVE", "IO-1_ACTIVE", "IO-2_ACTIVE"};
/* Messages associated with active processes */

/* All this greatly simplifies displaying data. We can use one big loop */
struct PCB;

static char *messageT = "Welcome to the WHOLE_FAMILY scheduling simulation\n\
(featuring parents and children)\
\
Type 'mp3 -help' to see command line parameters.\
\
Report will be stored in mp3.report\n\n\
Note console in lower-left conner";

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
/* (in queues.c) */

void EnQueue(struct Q *queue, struct PCB *job);
struct PCB *DeQueue(struct Q *queue);
int empty(struct Q *queue);

/* Queue sorting criterias */
#define SIZE 1    /* SIZE = memory size of the program */
#define TIME 2    /* TIME = Arrival time */
#define PREDICT 3 /* PREDICT = predeicted execution time */

int InsertInOrder(struct PCB *newp, struct Q *queue, int order);

int predict();
int exec_proc(struct PCB *proc);
