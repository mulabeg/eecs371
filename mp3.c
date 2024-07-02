/*   EMir Mulabegovic();
     EECS 371

     MP3

Some notes:

-Compile program with:

make

(Makefile is supplied)

Files included:
----------------
Makefile
mp3.c
queues.c
mp3.h

*************************************************
Type:

'mp3 -help'

*************************************************

Most of the program is the same as mp2.c. mp3.c just adds signals to handle
IO executions. Look down in section marked as *Signals and handlers*.

Idea is as follows:

Whenever any of the processes becomes active in IO queues (there are 2 of them)
process (parent, server) forks a kid ... ahm I meant child. Child basicaly
just sits there and do nothing. To forks a child parent send SIGHUP to itself.

Upon completion of IO burst parent sends SIGTERM to a child in order to
terminate it. In this moment child was supposed to print it pid and parents
pid and theh exits. I decided not to do that since I'm using curses for
displaying data and allowing both children and parents to write on the same
screen would be messy. Instead child just exits and parent catches SIGCHLD
signal and then displays pid of the child that just terminated.

Note on displaying data:
All actions that involve signals (forks and terminations) are displayed on
the Console (lower-left conner of the screen). Number appended in the front
is time when action happened. Numbers in parenthesis are pids.

Note on report:
It's the same as in mp2.

I guess that's all .....

*/

#include "mp3.h"
#include <curses.h>
#include <signal.h>
#include <sys/types.h>
#include <termios.h>

/* Global simulation parameters */
int timer;        /* Clock - in ms */
int memory_avail; /* Available memory (in kb) */

int lts_alg = SJF; /* defaults */
int sts_alg = RR;

/* These are simulation control flags */

int run;
int step;
int finish;

/* Balanced IO algorithm parameters */
int io_busy[2]; /* Business indicators */

/* Round-Robin parameters */
int quantum = 5; /* Time slice parameter */
int slice = 0;   /* Elapsed time */

/* Predicted-Shortes-Job-First parameters */
#define T0 4

/* Queues declarations */

struct Q queue[7]; /* Queue pointers */
struct Q helpQ;
struct PCB *active[3]; /* pointers to active processes
                          on CPU, IO1 and IO2 */

/* Console control */

#define CON_SIZE 5

char **Console;
int *con_index;

void init_console() {
  int i;

  con_index = (int *)malloc(sizeof(int));

  Console = (char **)malloc(CON_SIZE * sizeof(char *));
  for (i = 0; i < CON_SIZE; i++)
    Console[i] = (char *)malloc(50);
}

void console(char *message) {
  strcpy(Console[*con_index], message);
  *con_index = ++*con_index % CON_SIZE;
}

/* ***************************************************************** */
/* ReadFile reads input data, sort them and put them into job queue. */
/* It is using function Insert_In_Order                              */
/* ***************************************************************** */

void ReadFile() {
  struct PCB *new, *aux;
  FILE *data;
  char buffer[100];
  char *buffpt;
  int i;

  data = fopen(DATA_FILE, "r");

  if (data == NULL) {
    perror("Cannot open input file !");
    exit(0);
  }

  while ((fgets(buffer, sizeof(buffer), data)) != NULL) {
    buffpt = buffer;
    while ((*buffpt == ' ') || (*buffpt == '\t'))
      buffpt++;
    if (strlen(buffpt) - 1) {
      new = malloc(sizeof(struct PCB));

      /* Set defaults ... */
      new->current_burst = 0;
      new->burst_elapsed = 1;
      new->predict = T0;

      sscanf(buffpt, "%d %d %d %d", &(new->proc_num), &(new->arrival_time),
             &(new->memory), &(new->io_dev));

      fgets(buffer, sizeof(buffer), data);
      buffpt = buffer;
      i = 0;
      do {

        sscanf(buffpt, "%d", &((new->burst)[i]));
        buffpt = (char *)strchr(buffpt, ' ');
        if (buffpt != NULL)
          while (*buffpt == ' ')
            buffpt++;

      } while ((new->burst)[i++] != -1);

      if (lts_alg == SJF)
        InsertInOrder(new, &queue[JOB], SIZE);
      else
        InsertInOrder(new, &queue[new->io_dev + 4], TIME);
    }
  }

  fclose(data);
}

/* ******************* */
/* Signals & handlers  */
/* ******************* */

/* This is part that is specific to mp3.c */

pid_t child_pid[2]; /* We need 2 of them. For IO1 & IO2 */
int queue_index;

/* Clients (children) part */

static void terminate_me() {
  exit(1); /* If child needs to print anything on exit it
              would go in this function */
}

static void exec_child() {
  /* Captures SIGTERM and then waits forever */
  char buffer[50];

  if (signal(SIGTERM, terminate_me) == SIG_ERR) {
    perror("Can't capture SIGTERM !\n");
    exit(-1);
  }

  while (1)
    pause(); /* Wait until SIGTERM is received*/
}

/* Server part - parent */

static void spawn_child() {
  /* Forks a new child. Executes upon receiving SIGHUP */

  char buffer[50];

  if ((child_pid[queue_index] = fork()) < 0)
    console("Error forking child !");
  else if (child_pid[queue_index] == 0) /* Child */
    exec_child();
  else { /* Parent */
    sprintf(buffer, "%d: Parent (%d) forked - child pid: %d", timer, getpid(),
            child_pid[queue_index]);
    console(buffer);
  }

  return;
}

static void wait_child() {
  /* Executes whenever any of children terminates (upon receiving SIGCHLD) */
  char buffer[50];
  int status;
  int pid;

  pid = wait(&status);

  sprintf(buffer, "%d: Child (%d) terminated - parent pid %d", timer, pid,
          getpid());
  console(buffer);

  return;
}

void init_signals() {
  /* Catches signals */
  if (signal(SIGHUP, spawn_child) == SIG_ERR) {
    perror("Can't capture SIGHUP !\n");
    exit(-1);
  }
  if (signal(SIGCHLD, wait_child) == SIG_ERR) {
    perror("Can't capture SIGCHLD !\n");
    exit(-1);
  }
}

/* ********************************************************************* */
/* SHEDULING ROUTINES
/* ********************************************************************* */

/* lts_bal_io() is long term scheduler implementing BAL_IO algorithm */

int lts_bal_io() {
  int arrived_io1;
  int arrived_io2;
  int picked = 0;

  do {
    arrived_io1 = 0;
    arrived_io2 = 0;
    picked = 0;

    if (empty(&queue[LTS_IO1]) && empty(&queue[LTS_IO2]))
      return 0;

    if (!empty(&queue[LTS_IO1]))
      if ((queue[LTS_IO1].head->arrival_time <= timer) &&
          (queue[LTS_IO1].head->memory <= memory_avail))
        arrived_io1 = 1;

    if (!empty(&queue[LTS_IO2]))
      if ((queue[LTS_IO2].head->arrival_time <= timer) &&
          (queue[LTS_IO2].head->memory <= memory_avail))
        arrived_io2 = 1;

    if (arrived_io1 && arrived_io2)
      if (io_busy[0] <= io_busy[1])
        picked = 1;
      else
        picked = 2;
    else if (arrived_io1)
      picked = 1;
    else if (arrived_io2)
      picked = 2;

    if (picked) {
      memory_avail -= queue[picked + 4].head->memory;
      queue[picked + 4].head->lts_select = timer;
      queue[picked + 4].head->start_wait = timer;
      EnQueue(&queue[READY], DeQueue(&queue[picked + 4]));
      io_busy[picked - 1]++;
    }
  } while (picked);

  return 1;
}

/* lts_bal_io() is long term scheduler implementing SJF algorithm */

int lts_sjf() {
  struct PCB *helpt = NULL;

  if (empty(&queue[JOB]))
    return 0;
  do {
    if ((queue[JOB].head->arrival_time <= timer) &&
        (queue[JOB].head->memory <= memory_avail)) {

      memory_avail -= queue[JOB].head->memory;
      queue[JOB].head->lts_select = timer;
      queue[JOB].head->start_wait = timer;
      EnQueue(&queue[READY], DeQueue(&queue[JOB]));
    } else {
      if (helpt == NULL)
        helpt = queue[JOB].head;
      EnQueue(&queue[JOB], DeQueue(&queue[JOB]));
    }
  } while (helpt != queue[JOB].head);
}

/* STS module handles short term sheduling for queue[READY],
   queue[IO1] and queue[IO2] */

int sts()

{
  int i;

  /* CPU part - Round Robin*/
  if (sts_alg == RR) {
    if (slice == quantum && (active[CPU] != NULL)) {
      active[CPU]->start_wait = timer;
      EnQueue(&queue[READY], active[CPU]);
      active[CPU] = NULL;
    } else
      slice++;
    if (!empty(&queue[READY]))
      if (active[CPU] == NULL) {

        queue[READY].head->wait_total += timer - queue[READY].head->start_wait;
        active[CPU] = (struct PCB *)DeQueue(&queue[READY]);
        slice = 1;
      }
  }

  /* CPU part - PSJF */
  if (sts_alg == PSJF) {
    if (!empty(&queue[READY])) {
      predict();
      if (active[CPU] == NULL) {
        queue[READY].head->wait_total += timer - queue[READY].head->start_wait;
        active[CPU] = (struct PCB *)DeQueue(&queue[READY]);
      }
    }
  }

  /* IO1 & IO2 - using FCFS algorithm */

  for (i = 1; i <= 2; i++) {
    if (!empty(&queue[i]))
      if (active[i] == NULL) {
        active[i] = (struct PCB *)DeQueue(&queue[i]);

        queue_index = i - 1;
        kill(getpid(), SIGHUP); /*  Fork a new child */
      }
  }
}

/* Predict is function used by PSJF */

int predict() {
  struct PCB *helpt;

  while (!empty(&queue[READY])) {
    helpt = (struct PCB *)DeQueue(&queue[READY]);
    if (helpt->current_burst > 0)
      helpt->predict =
          (helpt->predict + helpt->burst[helpt->current_burst - 1]) / 2;
    InsertInOrder(helpt, &helpQ, PREDICT);
  }

  while (!empty(&helpQ))
    EnQueue(&queue[READY], DeQueue(&helpQ));
  return 1;
}

/* Kernel module is central part of simulation. It takes care that processes
   actually get "executed" and figures out when execution is completed. It
   also handles IO execution. */

/* Ahm .... actually most of the stuff is done in exec_proc below ... */

int kernel() {
  int i;
  /* Execute CPU and IO devices */

  for (i = 0; i <= 2; i++)
    exec_proc(active[i]);
}

int exec_proc(struct PCB *proc) {
  if (proc == NULL)
    return 0;

  if ((proc->burst)[proc->current_burst] > proc->burst_elapsed) {
    /* Process is still running */
    proc->burst_elapsed++;
  }

  else if ((proc->burst)[(proc->current_burst) + 1] == -1) {
    /* Process terminated */
    proc->terminate = timer + 1;
    memory_avail += proc->memory;
    if (lts_alg == BAL_IO)
      io_busy[proc->io_dev - 1]--;

    EnQueue(&queue[FIN], proc);
    active[CPU] = NULL;
  }

  else {
    proc->burst_elapsed = 1;
    /* Process requested IO or CPU */
    if (proc->current_burst++ % 2) {
      proc->start_wait = timer + 1;
      EnQueue(&queue[READY], proc);

      kill(child_pid[proc->io_dev - 1], SIGTERM); /* Terminate  Child */

      active[proc->io_dev] = NULL;
    } else {
      EnQueue(&queue[proc->io_dev], proc);
      active[CPU] = NULL;
    }
  }
  return 1;
}

/* Window identifiers */

static WINDOW *jobs;
static WINDOW *name;
static WINDOW *status;
static WINDOW *control;
static WINDOW *message;
static WINDOW *console_win;

/* ********************************************************************* */
/* FUNCTIONS FOR DISPLAYING DATA
/* ********************************************************************* */

void display_name() {
  wmove(name, 1, 10);
  wprintw(name, "Emir Mulabegovic - mulabego@mcs.anl.gov");
  wmove(name, 2, 10);
  wprintw(name, "EECS 371 - Fall 1997");

  wrefresh(name);
}

void display_status() {
  char dummy[10];
  werase(status);
  wmove(status, 1, 10);
  box(status, '|', '-');
  wprintw(status, "Time: %d\tAvailable memory: %d\tLTS: %s\tSTS: %s %s", timer,
          memory_avail, ltsM[lts_alg - 1], stsM[sts_alg - 1], "");

#if 0
	  (sts_alg == RR) ? sprintf(dummy,"(q = %d)",quantum) : "");
#endif
  /* I know ... I know ... this looks insane .... but belive me .... it works */

  wrefresh(status);
}

void display_message() {
  wprintw(message, "%s", messageT);

  wrefresh(message);
}

void display_control() {
  werase(control);
  wmove(control, 1, 0);
  wprintw(control, "\t%s\n\t%s\n\t[F]inish", (run) ? "[P]ause" : "[G]o",
          (run) ? "" : "[S]tep");

  box(control, '|', '-');
  wrefresh(control);
}

void display_console() {
  int i;

  werase(console_win);
  for (i = 0; i < CON_SIZE; i++) {
    wmove(console_win, i + 1, 1);
    wprintw(console_win, "%s", Console[(i + *con_index) % CON_SIZE]);
  }
  box(console_win, '|', '-');
  wmove(console_win, 0, 1);
  wprintw(console_win, "* Console *");
  wrefresh(console_win);
}

void display_jobs() {
  int i = 1, j;
  struct PCB *flipt;

  werase(jobs);
  box(jobs, '|', '-');
  wmove(jobs, i++, 2);
  wprintw(jobs,
          "PID\tArrived\tMemory\tIO_dev\tLTS\tTermin.\tWait\tPredict\tStatus");
  wmove(jobs, i++, 2);
  wprintw(jobs, "--------------------------------------------------------------"
                "---------");

  for (j = 0; j <= 6; j++) {
    if ((active[j] != NULL) && (j != 3) && (j != 4) && (j != 5) && (j != 6)) {
      wmove(jobs, i++, 2);
      wprintw(jobs, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s", active[j]->proc_num,
              active[j]->arrival_time, active[j]->memory, active[j]->io_dev,
              active[j]->lts_select, (j == FIN) ? active[j]->terminate : -1,
              active[j]->wait_total,
              (sts_alg == PSJF) ? active[j]->predict : -1, ActiveM[j]);
    }

    flipt = queue[j].head;

    while (flipt != NULL) {
      if (!(((j == JOB) || (j == LTS_IO1) || (j == LTS_IO2)) &&
            flipt->arrival_time > timer)) {

        wmove(jobs, i++, 2);
        wprintw(jobs, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s", flipt->proc_num,
                flipt->arrival_time, flipt->memory, flipt->io_dev,
                flipt->lts_select, (j == FIN) ? flipt->terminate : -1,
                /*		(j == FIN) ? flipt->terminate-flipt->arrival_time
                   : -1,*/
                (j == CPU) ? flipt->wait_total + timer - flipt->start_wait
                           : flipt->wait_total,
                (sts_alg == PSJF) ? flipt->predict : -1, Qmessage[j]);
      }

      flipt = flipt->next;
    }
  }
  wrefresh(jobs);
}

/* ********************************************************************* */
/* Initialize windows + turn on non-cannonical input mode
/* ********************************************************************* */

void init_terminal() {
  static struct termios buf;

  printf("%d\n", initscr());

  tcgetattr(0, &buf);

  buf.c_lflag &= ~(ECHO | ICANON);
  buf.c_cc[VMIN] = 0;
  buf.c_cc[VTIME] = 4;
  tcsetattr(0, TCSAFLUSH, &buf);

  name = newwin(4, 80, 0, 0);
  box(name, '|', '-');
  status = newwin(3, 80, 3, 0);
  jobs = newwin(12, 80, 5, 0);
  control = newwin(7, 31, 16, 49);
  message = newwin(11, 70, 6, 10);
  console_win = newwin(7, 50, 16, 0);
}

/* ********************************************************************* */
/* Report creation function
/* ********************************************************************* */

void report() {
  struct PCB *flipt;
  FILE *out;
  int count;
  float wait_av, turn_av;

  out = fopen(REPORT_FILE, "w");

  flipt = queue[FIN].head;
  fprintf(out, "*** EMir Mulabegovic()\n*** EECS 371\n*** MP3\n\n");
  fprintf(out, "PID\tArrived\tLTS\tTerminate\tTurnaround\tWait\n");

  count = 0;
  wait_av = 0;
  turn_av = 0;
  while (flipt != NULL) {
    count++;
    turn_av += flipt->terminate - flipt->arrival_time;
    wait_av += flipt->wait_total;
    fprintf(out, "%d\t%d\t%d\t%d\t\t%d\t\t%d\n", flipt->proc_num,
            flipt->arrival_time, flipt->lts_select, flipt->terminate,
            flipt->terminate - flipt->arrival_time, flipt->wait_total);
    flipt = flipt->next;
  }

  fprintf(out, "\n\n");
  fprintf(out, "Average turnaround time: %.2f\n", turn_av / count);
  fprintf(out, "Average wait time: %.2f\n\n\n", wait_av / count);

  fprintf(out, "Legend:\nArrived\t\t - time when the process is submited to "
               "the system\n");
  fprintf(out, "LTS\t\t - Time when the process is picked up by the long term "
               "scheduler\n");
  fprintf(out, "Terminate\t - time when the process is terminated\n");
  fprintf(
      out,
      "Turnaround\t - time between submision and termination of the process\n");
  fprintf(out, "Wait\t\t - time the process spent in the READY queue\n\n\n");

  fprintf(out, "Have fun ... EM();\n");

  fclose(out);
}

/* ********************************************************************* */
/* Main program .... basically calls all functions above and checks when the
   simulation is over
/* ********************************************************************* */

int main(argc, argv) int argc;
char *argv[];
{
  int i = 1;
  char command;

  /* Argument parsing ..... */
  while (i < argc) {
    if (!strcmp(argv[i], "-lts")) {
      lts_alg = atoi(argv[++i]);
      i++;
    } else if (!strcmp(argv[i], "-sts")) {
      sts_alg = atoi(argv[++i]);
      if ((sts_alg == RR) && (i < argc - 1))
        quantum = atoi(argv[++i]);

      i++;
    }

    else {
      (void)fprintf(stderr,
                    "Usage: %s [-lts [1|2]] [-sts [1|2] [quantum]]\n\n-lts:\t1 "
                    "- Smallest Job First (default)\n\t2 - Balanced "
                    "I/O\n\n-sts:\t1 - Round-Robin (default); default quantum "
                    "= 5\n\t2 - Predict-Shortest-Job-First\n\n",
                    argv[0]);
      exit(-1);
    }
  }

  ReadFile();
  init_signals();
  init_terminal();
  init_console();

  timer = 0;
  memory_avail = MEMORY;
  display_name();
  display_status();
  display_control();
  display_message();

  run = 0;
  step = 0;
  finish = 0;

  while (!empty(&queue[READY]) || !empty(&queue[JOB]) ||
         !empty(&queue[LTS_IO1]) || !empty(&queue[LTS_IO2]) ||
         !empty(&queue[IO1]) || !empty(&queue[IO2]) || active[CPU] ||
         active[IO1] || active[IO2]) {

    display_control();
    if (run || step) {
      step = 0;
      timer++;
      if (lts_alg == SJF)
        lts_sjf();
      if (lts_alg == BAL_IO)
        lts_bal_io();
      sts();
      if (!finish) {
        display_status();
        display_jobs();
        display_console();
      }
      kernel();
    }

    if (!finish)
      read(0, &command, 1);
    if (command == 'g' || command == 'G')
      run = 1;
    if (command == 'p' || command == 'P')
      run = 0;
    if (command == 's' || command == 'S')
      step = 1;
    if (command == 'f' || command == 'F') {
      run = 1;
      finish = 1;
    }

    command = 0;
  }
  timer++;
  display_status();
  display_jobs();
  display_console();
  report();
  while (!command)
    read(0, &command, 1);

  endwin();
  system("/usr/ucb/clear");
  return(0);
}
