#include "mailbox.h"
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "queueNode.h"

#define MAXTHREAD 10

//list of mailboxes
struct mailbox* mailboxs;

//list of send-semaphores and receivesemaphores
sem_t* sendSem;
sem_t* recvSem;

//if no-block mode
int ifNB = 0;
//queue for undiliverd requests if in NB mode
struct queueNode* head;
struct queueNode* tail;
struct queueNode* cursor;

//helper function: add undelivered request to the end of the queue
void addToQueue(int recipient, struct msg* msg){
    //make a new node
    struct queueNode* newNode = malloc(sizeof (struct queueNode));
    newNode->recipient = recipient;
    *newNode->msg = *msg;
    //if its the first node in queue, set it head and tail
    if(!head){
        head = newNode;
        tail = newNode;
    }
    else{//otherwise add it to the tail
        tail->next = newNode;
        newNode->prev = tail;
        tail = newNode;
    }
}

//if one request is deliverd, remove it from the queue
void rmNode(struct queueNode* target){
    //if the target node is in middle of a queue
    if(target->prev && target->next){
        target->prev->next = target->next;
        target->next->prev = target->prev;
    //if the target is the tail
    }else if(target->prev){
        target->prev->next = NULL;
        tail = target->prev;
    //if the target is the head
    }else if(target->next){
        target->next->prev = NULL;
        head = target->next;
    //if the target is the only node in the queue
    }else{
        head = NULL;
        tail = NULL;
    }
}

/*  iFrom   -the mailbox tp retrieve from
    pMsg    -message structure to fill in with received message
*/
void RecvMsg(int iFrom, struct msg *pMsg){
    sem_wait(&recvSem[iFrom]);//make recvSem[iFrom] 0
    *pMsg = mailboxs[iFrom].message;
    sem_post(&sendSem[iFrom]);//make sendSem[iFrom] 1, able to get another msg in the box
}

/*  iTo -mailbox to send to
    pMsg -message to be sent
 */
void SendMsg(int iTo, struct msg *msg){
    sem_wait(&sendSem[iTo]);//make sendSem[iTo] 0, no other msg can be sent to this mailbox
    mailboxs[iTo].message = *msg;
    sem_post(&recvSem[iTo]);//make recvSem[iTo] 1, allow thread to receice this msg
}

/*  iTo -mailbox to send to
 pMsg -message to be sent
 */
int NBSendMsg(int iTo, struct msg *msg){
    //try send msg to the target mailbox
    if(sem_trywait(&sendSem[iTo]) == 0){ //if no msg in the mailbox, successfully aquired the semaphore
        mailboxs[iTo].message = *msg;//put in the msg
        sem_post(&recvSem[iTo]);//allow thread to receive this message
        return 0;
    }
    //if there is already a msg in the box, return -1
    return -1;
}

void *adder(void* argv){
    int index =  (long) argv;//cast the argument to integer


    struct msg helper;
    int value = 0;
    int cnt = 0;
    time_t start = time(NULL);//record the start time of the thread
    

    while(1){
        //try to receive msg from its mailbox
        RecvMsg(index, &helper);
        
        //if positive value
        if(helper.value >= 0){
            value += helper.value;  //add value
            cnt += 1;               //add count
            sleep(1);               //sleep for 1 second
        }
        else{
            //if get the terminate message, send a message back to main thread
            struct msg* toZero = malloc(sizeof (struct msg));
            toZero->iFrom = index;
            toZero->value = value;
            toZero->cnt = cnt;
            toZero->tot = time(NULL) - start;
            SendMsg(0, toZero);
            return 0;
        }
    }
}

int main(int argc, char** argv){
    int numOfThreads;
    int recipient, value;
    long i;
    pthread_t* handler;
    int result;
    char line[256];
    int tryResult;
    struct msg* message = malloc(sizeof(struct msg));
    
    //if didn't provide the number of thread
    if(argc < 2){
        printf("Give at least 2 arguments\n");
        exit(1);
    }
    
    //cast the number of thread to integer
    numOfThreads = atoi(argv[1]);
    //the number need to be a positive value
    if(numOfThreads < 0){
        printf("Wrong number of threads!\n");
        exit(1);
    }
    
    //if the given number is larger than the maximum value, use the maximum value instead
    if(numOfThreads > MAXTHREAD){
        numOfThreads = MAXTHREAD;
    }

    //check if not-block
    if((argc == 3) && !strcmp(argv[2], "nb")){
        printf("In NB mode...\n");
        ifNB = 1;
    }
    
    //allocate space for the semaphore lists
    sendSem = malloc((sizeof (sem_t))*(numOfThreads + 1));
    recvSem = malloc((sizeof (sem_t))*(numOfThreads + 1));
    
    //allocate space for mailboxes
    mailboxs = malloc((sizeof (struct mailbox))* (numOfThreads + 1));
    //initiate semaphore for the main thread
    sem_init(&sendSem[0], 0, 1);
    sem_init(&recvSem[0], 0, 0);
    //allocate space for the list of thread id
    handler = malloc((sizeof(pthread_t)) * numOfThreads);
    
    
    //for each thread, initiate the semaphores and create thread
    for(i = 0 ; i < numOfThreads ; i ++){
        sem_init(&sendSem[i + 1], 0, 1);
        sem_init(&recvSem[i + 1], 0, 0);
        if(pthread_create(&handler[i], NULL, adder,(void *) i+1) != 0){
            printf("Error creating thread!\n");
            exit(1);
        }
       
    }
    
    //wait for the standard input
    fgets(line, 256, stdin);
    result = sscanf(line, "%d %d", &value, &recipient);
    while(result == 2){//if we can get 2 integers
        if((recipient < (numOfThreads+1)) && (recipient >= 1)){//if the recipient index is correct
            if(value < 0){
                printf("Please give positive value!\n");
            }
            else{
                message->iFrom = 0;
                message->value = value;
                message->cnt = 0;
                message->tot = 0;
                if(!ifNB){//if not not-block mode
                    SendMsg(recipient, message);//use SendMsg
                }
                else{//if not-block mode
                    //if the msg is not delived
                    if((tryResult = NBSendMsg(recipient, message)) == -1){
                        //add the undilivered message and recipient to queue
                        addToQueue(recipient, message);
                    }
                }
                //printf("Message with value %d sent\n", message->value);
            }
            
        }
        else{
            printf("Wrong mailbox number!\n");
        }
        
        //try to deliver each of the undilivered msgs in the queue
        cursor = head;
        while(cursor){
            //if sent successfully to the mailbox
            if((tryResult = NBSendMsg(cursor->recipient, cursor->msg)) == 0){
                //remove the msg from the queue
                rmNode(cursor);
            }
            cursor = cursor->next;
        }
        
        fgets(line, 256, stdin);
        result = sscanf(line, "%d %d", &value, &recipient);
   }
    
    //after get something not 2 integers from the stadard input
    message->iFrom = 0;
    message->value = -1;
    message->cnt = 0;
    message->tot = 0;
    for(i = 0; i < numOfThreads; i ++){
        SendMsg(i+1, message);
        //send terminate message to each child thread
    }
  
    //wait for the message from each child
    for(i = 0; i < numOfThreads; i ++){
        RecvMsg(0, message);
        //print the statistics
        printf("The result from thread %d is %d from %d operations during %d seconds\n", message->iFrom, message->value, message->cnt, message->tot);
    }
    
    //join the children threads
    for(i = 0; i < numOfThreads; i++){
        pthread_join(handler[i], NULL);
    }
    
    return 0;
}















