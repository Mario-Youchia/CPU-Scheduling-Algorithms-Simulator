#include "headers.h"

int msqid;
void clearResources(int);
void Terminate();


pid_t clk_p ,sch_p;

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    
    if ((msqid = msgget(MSGBUF, IPC_CREAT | 0666 )) < 0) {
        perror("msgget");
        exit(1);
    }

    
    char CurrentAlgorithm[10];
    // Initialization.
    // 1. Read the input files.
    queue process_queue = q_new() ;
    char line[100];
    FILE *file = fopen("./processes.txt","rt");
    fgets(line,100,file);
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    while (fgets(line,100,file))
    {
        //char s1,s2,s3,s4;
        printf("%s\n",line);
        processData* pData= ( processData *) malloc(sizeof(processData));
        
        sscanf(line, "%d\t%d\t%d\t%d\n",& pData->id,& pData->arrivaltime,& pData->runningtime,& pData->priority);
        enqueue(process_queue,pData);
        /* printf("%d\n",pData->id);
        printf("queue has %d size and at %d\n",(int)process_queue->alloc,(int)process_queue->tail); */
        
        
    }

    /* for (size_t i = 0; i < 10; i++)
    {
        processData * x = (processData*)process_queue->buf[i];
        printf("hh %d\n",(int)(x->id));
    } */
    
    
    int choice;
    printf("Select scheduling algorithm:\n1-RoundR\n2-Highest Priority First\n3-Shortest remaining time first\n");
    scanf("%d",&choice);
    
    
    switch (choice)
    {
    case 1: strcpy(CurrentAlgorithm , "RR"); break;
    case 2: strcpy(CurrentAlgorithm , "HPF"); break;
    case 3: strcpy(CurrentAlgorithm , "SRTN"); break;
    
    default:
        printf("Ok...I am out!!\n");
        return -1;
        break;
    }
    printf("Opening clock and scheduler\n");
    clk_p = fork();
    if(clk_p ==-1)
        Terminate();
    else if (clk_p ==0)
    {
        if(execl("./clk.out","Clock",NULL) <0)
            perror("Error in opening clock");
        exit(-1);
    }
    
    sch_p = fork();
    if(sch_p ==-1)
        Terminate();
    else if (sch_p ==0)
    {
        if(execl("./scheduler.out","Scheduler",CurrentAlgorithm,NULL))
            perror("Error in opening scheduler");
        exit(-1);
    }

    
    printf("Opened successfully\n");
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    
    printf("current time is %d\n", getClk());
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.


    int current_proc_time ;   // this is the time we will get from the process on queue 
processMessage processMessage;
    while (!empty(process_queue))
    {
        processData * x = (processData*)Peak(process_queue);
        
        current_proc_time = x->arrivaltime;
        // printf("current time is %d\n", getClk());
        if(current_proc_time <= getClk()) {
DATA dummy;
            dequeue(process_queue, & dummy);
processMessage.arrivaltime=x->arrivaltime;
            processMessage.id=x->id;
            processMessage.priority=x->priority;
            processMessage.runningtime=x->runningtime;
            processMessage.mtype = 1;
            printf("sending\n");
            if(msgsnd(msqid,&processMessage,sizeof(processMessage)-sizeof(processMessage.mtype),!IPC_NOWAIT)<0)
                perror("msgsnd");
                ;
            printf("Sent with id %d at time %d\n",processMessage.id,getClk());

            free(x);

        }
        
        

    }
    // this is the end signal ... indicating no more processes are coming 
        processMessage.arrivaltime=-1;
        processMessage.id=-1;
        processMessage.priority=-1;
        processMessage.runningtime=-1;
        processMessage.mtype = 1;
        printf("Sending end signal\n");
        if(msgsnd(msqid,&processMessage,sizeof(processMessage)-sizeof(processMessage.mtype),IPC_NOWAIT)<0)
                perror("msgsnd");
    

    while (1)
    {
        /* code */
    }
    
    // 7. Clear clock resources
    destroyClk(true);
}

void Terminate() {
    //
    //
    printf("Error occured");
    clearResources(-1);
    exit(0);
}

void clearResources(int signum)
{
    kill(clk_p,SIGINT);
    kill(sch_p,SIGINT);
    msgctl(msqid, IPC_RMID, NULL);

    exit(0);

}


