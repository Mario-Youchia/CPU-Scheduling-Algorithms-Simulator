#include "headers.h"
#include <math.h>
#define MAXPROC 200


void handler();
void chld_handler();
void save_pref();
int RunProc(PCB* p);
void stop_proc(PCB* p);
void cont_proc(PCB* p);
bool enSignal = false;
bool cont = true;
bool q_empty = false;
FILE * pFile;

PCB* allprocesses[MAXPROC];
SchAlgo currentAlgorithm;
int ProcessesCounter =0;
int finish_num=1000;
int process_in_counter=0;
int pid;
int quant=1;
PCB* currentProcess;


int main(int argc, char * argv[])
{
    signal(SIGINT,handler);
    
    int msqid;
    if ((msqid = msgget(MSGBUF, 0666 )) < 0) {
        perror("msgget");
        exit(1);
    }
    initClk();
    pFile = fopen("scheduler.log","w");
    fprintf(pFile, "#At\ttime\tx\tprocess y\tstate\t\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    if ((char)argv[1][0] == 'R')
    {
        printf("RR detected\n");
        currentAlgorithm = RR;
    }
    else if ((char)argv[1][0] =='H')
    {
        printf("HPF detected\n");
        
        currentAlgorithm = HPF;
    }
    else
    {
        printf("SRTN detected\n");
        currentAlgorithm = SRTN;
    }
    
    
    
    
    processMessage processMessage;
    processMessage.mtype= 1; 
    
    // Scheduler implementation.
    
    
        
    heap_t* h = (heap_t*)malloc(sizeof(heap_t));
    
    msgrcv(msqid, &processMessage, sizeof(processMessage)-sizeof(processMessage.mtype), 1, !IPC_NOWAIT);
    printf("Received process with Id:%d and priority:%d\n", processMessage.id, processMessage.priority);
    if (processMessage.id==-1)
    {
        //clear resources first
        printf("received end signal\n");
        return 0;
    }
    process_in_counter++;
    
    PCB* pData= ( PCB *) malloc(sizeof(PCB));
    pData->processData = (processData *) malloc(sizeof(processData));

    pData->finished=false;       
    pData->processData->id=processMessage.id;
    pData->processData->arrivaltime=processMessage.arrivaltime;
    pData->processData->priority=processMessage.priority;
    pData->processData->runningtime=processMessage.runningtime;
    pData->finished=false;
    pData->finishTime=-1;
    pData->remainingTime=processMessage.runningtime;
    pData->running=false;
    pData->startTime=0;
    pData->pid=-1;

    switch (currentAlgorithm)
        {

            case HPF: 
            push(h,pData->processData->priority,pData);

            break;
            case RR: push(h,0,pData);
            break;
            default:
            
            push(h,pData->remainingTime,pData);
            break;
        }
        
        int lastTime = getClk();
        int lastRt = getClk();
        while(cont) {             
            signal(SIGCHLD,chld_handler);
            switch (currentAlgorithm)
            {
            case HPF:  
            {          
                if (ProcessesCounter == 0  || (! currentProcess->running && h->len >0))
                {
                    currentProcess = (PCB*)pop(h);
                    allprocesses[ProcessesCounter++]=currentProcess;
                    RunProc(currentProcess);
                    

                }
                else if(lastTime != getClk())
                {
                    lastTime = getClk();
                    currentProcess->remainingTime --;
                    q_empty = h->len ==0;
                    
                }
                
                

                break;
            }
            case RR:
                
                if(lastTime != getClk() && ProcessesCounter != 0 && currentProcess->running )   // track running-process time
                {
                    
                    lastTime = getClk();
                    currentProcess->remainingTime --;
                    q_empty = h->len ==0;          
                }

                //
                if ( ProcessesCounter == 0  || (! currentProcess->running && h->len > 0))    // no running process now
                {
                    
                    currentProcess = (PCB*)pop(h);
                    printf("process with id %d started\n",currentProcess->processData->id);
                    
                    if(currentProcess->pid ==-1)
                    {
                        RunProc(currentProcess);
                        allprocesses[ProcessesCounter++]=currentProcess;
                    }    
                    else
                    {
                        
                        cont_proc(currentProcess);
                        signal(SIGCHLD,chld_handler);
                        
                    }
                        
                }
                else if (lastRt +quant == getClk())    // quant time... time to swap ... and the current time quantum has expired
                {
                    //signal(SIGCHLD,SIG_IGN);
                    lastRt = getClk();
                    
                    if (!q_empty)
                    {
                        
                        stop_proc(currentProcess);
                        
                        printf("process with id %d stopped \n",currentProcess->processData->id);
                        
                        push(h,0,currentProcess);
                        currentProcess = pop(h);
                        
                        if (currentProcess->pid ==-1)   // if new create
                        {
                            printf("process with id %d started\n",currentProcess->processData->id);
                            
                            RunProc(currentProcess);
                            allprocesses[ProcessesCounter++]=currentProcess;
                        }
                        else     //else resume  
                        {
                            
                            cont_proc(currentProcess);
                            
                            printf("process with id %d resumed\n",currentProcess->processData->id);
                            
                        }
                        signal(SIGCHLD,chld_handler);
                    }    
                }
                
                break;
            default:      //case srtn
            {
            
            if(((PCB*)top(h)) !=NULL  &&  currentProcess == NULL) //first process
            {
                /* currentProcess = (PCB*) malloc(sizeof(PCB));
                currentProcess->processData = (processData*) malloc(sizeof(processData)); */
                currentProcess= pop(h); 
                currentProcess->startTime=getClk();
                currentProcess->running=1;

                RunProc(currentProcess);
                    
                
                    
                
                        
            }
            else if( ((PCB*)top(h)) !=NULL  && ((currentProcess->remainingTime > ((PCB*)top(h))->remainingTime && !currentProcess->finished) || currentProcess->finished)) // the process at the top is shorter or the current is finished
            {
                if(((PCB*)top(h))->pid != -1 && !currentProcess->finished) //the process is initialized once 
                {
                    printf("process with id %d stopped and process with id %d continued\n",currentProcess->processData->id,((PCB*)top(h))->processData->id);
                    if(currentProcess->pid >100)
                    stop_proc(currentProcess);
                    push(h,currentProcess->remainingTime,currentProcess);
                    currentProcess=pop(h);
                    currentProcess->running=1;
                    if(currentProcess->pid >100)
                    cont_proc(currentProcess);
                    signal(SIGCHLD,chld_handler);
                }
                else if (((PCB*)top(h))->pid == -1 ) //new process
                {

                    printf("process with id %d stopped and process with id %d started\n",currentProcess->processData->id,((PCB*)top(h))->processData->id);

                    pid = RunProc((PCB*)top(h));

                    
                       
                    ((PCB*)top(h))->pid= pid;
                    ((PCB*)top(h))->startTime=getClk();
                    if(!currentProcess->finished)
                    {
                        if(currentProcess->pid >100)
                        stop_proc(currentProcess);
                        push(h,currentProcess->remainingTime,currentProcess);
                    }
                    currentProcess=pop(h);
                    currentProcess->running=1;
                    signal(SIGCHLD,chld_handler);   
                }
                else
                {

                    printf("process with id %d started\n",((PCB*)top(h))->processData->id);
                    currentProcess=pop(h);
                    cont_proc(currentProcess);
                }
                signal(SIGCHLD,chld_handler);


            }
            if(lastTime != getClk() )
                {
                    lastTime = getClk();
                    currentProcess->remainingTime --;
                    if(currentProcess->remainingTime==0)
                    {
                        currentProcess->finished=1;
                        currentProcess->finishTime=lastTime;
                        currentProcess->running=0;
                        allprocesses[++ProcessesCounter]=currentProcess;

                    }

                    q_empty = (h->len ==0 && currentProcess->finished);

                }
            //signal(SIGCHLD,chld_handler);
            break;
            }
            }

            if (! enSignal &&  ! (msgrcv(msqid, &processMessage, sizeof(processMessage)-sizeof(processMessage.mtype), 1, IPC_NOWAIT) < 0)) 
            {   
                
                   
                printf("Received process with Id:%d and priority:%d at time %d\n",processMessage.id,processMessage.priority,getClk());
            
                if (processMessage.id==-1)
                {
                    //clear resources first
                    //clear resources first
                    printf("received end signal\n");
                    finish_num=process_in_counter;
                    enSignal = true;
                }
                else 
                {
                    PCB* pData= ( PCB *) malloc(sizeof(PCB));
                    pData->processData = (processData *) malloc(sizeof(processData));

                    pData->finished=false;       
                    pData->processData->id=processMessage.id;
                    pData->processData->arrivaltime=processMessage.arrivaltime;
                    pData->processData->priority=processMessage.priority;
                    pData->processData->runningtime=processMessage.runningtime;
                    pData->finished=false;
                    pData->finishTime=-1;
                    pData->remainingTime=processMessage.runningtime;
                    pData->running=false;
                    pData->startTime=0;
                    pData->pid=-1;
                    switch (currentAlgorithm)
                    {
                    case HPF:
                        push(h,pData->processData->priority,pData);
                        break;
                    case SRTN :
                        if( processMessage.runningtime >0)
                        {
                            push(h,pData->processData->runningtime,pData);
                        }        
                        if(enSignal && q_empty && finish_num==ProcessesCounter)
                            cont=false;
                        break;
                    default:
                        
                        push(h,0,pData);

                        break;
                    }
                    
                    
                }
                  
            }

        }

        fclose(pFile);
        save_pref();
        
        
       
        
    
    //upon termination release the clock resources
    
    destroyClk(true);
}

void handler() {
    printf("Scheduler terminating\n");
    fclose(pFile);

    exit(-1);
}
int RunProc(PCB* p) {
    PCB* temp = p;
    p->running = true;
    p->startTime = getClk();
    p->remainingTime = p->processData->runningtime;
    printf("At\ttime\t%d\tprocess\t%d\tstarted\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",getClk() ,temp->processData->id ,temp->processData->arrivaltime,temp->processData->runningtime,temp->remainingTime,((int)temp->startTime - (int)temp->processData->arrivaltime));

    fprintf(pFile,"At\ttime\t%d\tprocess\t%d\tstarted\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",getClk() ,temp->processData->id ,temp->processData->arrivaltime,temp->processData->runningtime,temp->remainingTime,((int)temp->startTime - (int)temp->processData->arrivaltime));
char str[5];
    sprintf(str,"%d",p->remainingTime);
    
    int pid =fork();
    if (pid == 0)
    {
        execl("./process.out","Process",str,NULL);
        exit(0);
    }
    else if( pid > 0)
    {
        p->pid = pid;
return pid;
    } 
    
    
    
}

void chld_handler() {    //received signal from child as it finished
    PCB* temp;
    /* if (currentAlgorithm == SRTN)
    {
        temp   = allprocesses[ProcessesCounter];
    }
    else
        temp= allprocesses[ProcessesCounter-1]; */

    temp = currentProcess;

    int status;

    waitpid(0,&status,WNOHANG |WUNTRACED );
    
    if(WIFEXITED(status) &&  WEXITSTATUS(status) == 12 ) 
    {
        
 
        temp->finished = true;
        temp->finishTime = getClk();
        temp->running = false;
fprintf(pFile,"At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%0.2f\n",getClk() ,temp->processData->id ,temp->processData->arrivaltime,temp->processData->runningtime,temp->remainingTime,((int)temp->startTime - (int)temp->processData->arrivaltime),temp->finishTime-temp->processData->arrivaltime,((float)(temp->finishTime-temp->processData->arrivaltime)/(float)temp->processData->runningtime));
        printf("Process with ID: %d and priority: %d finished at %d \n",temp->processData->id,temp->processData->priority,temp->finishTime); 
        if (enSignal && q_empty  )
        {
            printf("?\n");
            cont = false;
        }
    }
    
    
}
 
void stop_proc(PCB* p) {
    signal(SIGCHLD,SIG_IGN);
    p->running = false;
    
    kill(p->pid,SIGSTOP);
    
    printf("At\ttime\t%d\tprocess\t%d\tStopped\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",getClk() ,p->processData->id ,p->processData->arrivaltime,p->processData->runningtime,p->remainingTime,((int)p->startTime - (int)p->processData->arrivaltime));
    fprintf(pFile,"At\ttime\t%d\tprocess\t%d\tStopped\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",getClk() ,p->processData->id ,p->processData->arrivaltime,p->processData->runningtime,p->remainingTime,((int)p->startTime - (int)p->processData->arrivaltime));
    

}
void cont_proc(PCB* p) {
    //signal(SIGCHLD,SIG_IGN);
    if(p->pid ==-1)
        printf("Unexpected uninitialized process state\n");
    p->running = true;
    
    kill(p->pid,SIGCONT);
    
    printf("At\ttime\t%d\tprocess\t%d\tresumed\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",getClk() ,p->processData->id ,p->processData->arrivaltime,p->processData->runningtime,p->remainingTime,((int)p->startTime - (int)p->processData->arrivaltime));
    fprintf(pFile,"At\ttime\t%d\tprocess\t%d\tresumed\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",getClk() ,p->processData->id ,p->processData->arrivaltime,p->processData->runningtime,p->remainingTime,((int)p->startTime - (int)p->processData->arrivaltime));  
    
}

void save_pref() {
    FILE * perfFile;
    perfFile = fopen("./scheduler.perf", "w");
    int TotalRuntime = 0, TotalWaitingTime = 0;
    float TotalWTA = 0.0f, TotalSD = 0.0f, CPU_utilization = 0.0f;
    int i = 0;
    while(allprocesses[i] != NULL)
    {
        TotalRuntime += allprocesses[i]->processData->runningtime;
        TotalWTA += (float)(allprocesses[i]->finishTime - allprocesses[i]->processData->arrivaltime)/(float)(allprocesses[i]->processData->runningtime);
        TotalWaitingTime += allprocesses[i]->startTime - allprocesses[i]->processData->arrivaltime;
        i++;
    }
    if (allprocesses[0] != NULL) {
        CPU_utilization = (float)(TotalRuntime + allprocesses[0]->processData->arrivaltime)/ (float)getClk() * 100.0f;
    }
    float AvgWTA = TotalWTA / (float)ProcessesCounter;
    float AvgWaiting = (float)TotalWaitingTime / (float)ProcessesCounter;

    i = 0;
    while (allprocesses[i] != NULL)
    {
        TotalSD += pow((((float)(allprocesses[i]->finishTime - allprocesses[i]->processData->arrivaltime)/(float)(allprocesses[i]->processData->runningtime)) - AvgWTA),2);
        i++;
    }

    float StdWTA = sqrt((float)TotalSD/(float)ProcessesCounter);

    fprintf(perfFile, "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f", CPU_utilization, AvgWTA, AvgWaiting, StdWTA);
    fclose(perfFile);
}


    


