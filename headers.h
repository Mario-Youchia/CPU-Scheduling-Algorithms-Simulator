#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <math.h>


typedef short bool;
typedef void* DATA;
#define true 1
#define false 0

#define SHKEY 300
#define MSGBUF 302
//#define INTERVAL 50   // test only 50 ms


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================


int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        //msleep(INTERVAL);
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

typedef enum SchAlgo{RR ,HPF  , SRTN} SchAlgo;


void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

typedef struct processData
{
    
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
} processData;

typedef struct ProcessControlBlock {
    processData* processData;
    int remainingTime;
    int startTime;
    int finishTime;
    int pid;
    bool running;
    bool finished;

} PCB;

typedef struct processMessage {
    long mtype;
    int arrivaltime;
    int priority;
    int runningtime;
    int id;

} processMessage;


// Data structures

//1- queue
typedef struct {
    DATA *buf;
    size_t head, tail, alloc;
} queue_t, *queue;
 
queue q_new()
{
    queue q = malloc(sizeof(queue_t));
    q->buf = malloc(sizeof(DATA) * (q->alloc = 4));
    //q->buf
    q->head = q->tail = 0;
    return q;
}
 
int empty(queue q)
{
    return q->tail == q->head;
}
 
void enqueue(queue q, DATA n)
{
    if (q->tail >= q->alloc) q->tail = 0;
    q->buf[q->tail++] = n;
 
    // Resize the circular buffer when it reaches capacity
    if (q->tail == q->alloc) {  /* needs more room */
        q->buf = realloc(q->buf, sizeof(DATA) * q->alloc * 2);
        if (q->head) {
            memcpy(q->buf + q->head + q->alloc, q->buf + q->head,
                sizeof(DATA) * (q->alloc - q->head));
            q->head += q->alloc;
        } else
            q->tail = q->alloc;
        q->alloc *= 2;
    }
}
 
int dequeue(queue q, DATA *n)
{
    if (q->head == q->tail) return 0;
    *n = q->buf[q->head++];
    if (q->head >= q->alloc) { /* reduce allocated storage no longer needed */
        q->head = 0;
        if (q->alloc >= 512 && q->tail < q->alloc / 2)
            q->buf = realloc(q->buf, sizeof(DATA) * (q->alloc/=2));
    }
    return 1;
}

DATA Peak(queue q) {
   if (q->head == q->tail) return NULL; 
   return q->buf[q->head];
} 

//2- p queue
typedef struct {
    int priority;
    // Stores scheduler data pointers.
    DATA data;
} node_t;
 
typedef struct {
    node_t *nodes;
    int len;
    int size;
} heap_t;
 
void push (heap_t *h, int priority, DATA data) {
    if (h->len + 1 >= h->size) {
        h->size = h->size ? h->size * 2 : 4;
        h->nodes = (node_t *)realloc(h->nodes, h->size * sizeof (node_t));
    }
    int i = h->len + 1;
    int j = i / 2;
    while (i > 1 && h->nodes[j].priority > priority) {
        h->nodes[i] = h->nodes[j];
        i = j;
        j = j / 2;
    }
    h->nodes[i].priority = priority;
    h->nodes[i].data = data;
    h->len++;
}
 
DATA pop (heap_t *h) {
    int i, j, k;
    if (!h->len) {
        return NULL;
    }
    DATA data = h->nodes[1].data;
 
    h->nodes[1] = h->nodes[h->len];
 
    h->len--;
 
    i = 1;
    while (i!=h->len+1) {
        k = h->len+1;
        j = 2 * i;
        if (j <= h->len && h->nodes[j].priority < h->nodes[k].priority) {
            k = j;
        }
        if (j + 1 <= h->len && h->nodes[j + 1].priority < h->nodes[k].priority) {
            k = j + 1;
        }
        h->nodes[i] = h->nodes[k];
        i = k;
    }
    return data;
}
DATA top(heap_t *h)
{
    if (h->len <= 0)
    {
        return NULL;
    }
    return h->nodes[1].data;
}


