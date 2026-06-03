#include "headers.h"

/* Modify this file as needed*/
int remainingtime;


int main(int agrc, char * argv[])
{
    initClk();
    
    remainingtime = atoi(argv[1]) ;
    int lastClock  =getClk();
    printf("Process started with remaining time: %d\n",remainingtime);
    
    while (remainingtime > 0)
    {
        
        if (lastClock != getClk())
        {
            remainingtime --; 
            lastClock = getClk(); 
            printf("remaining time is now: %d\n",remainingtime);
        }
        
        
    }
    printf("Process finished peacefully \n");
    destroyClk(false);
    
    return 12;
}
