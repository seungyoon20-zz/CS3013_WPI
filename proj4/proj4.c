//Yixue Wang ywang20
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MAX_THREAD  15
char input[256];

int numOfThreads;
int currentNumThreads;
sem_t* updateCount;
sem_t* readNewFile;


int numBadFiles;
int numDir;
int numReg;
int numSpl;
int byteReg;
int numAllText;
int byteText;

struct queueNode* head;
struct queueNode* tail;
//struct queueNode* cursor;

struct queueNode{
    struct queueNode* prev; //points to the previous node
    struct queueNode* next; //points to the next node
    pthread_t threadID; //points to the undelivered msg
    
};

//helper function: add undelivered request to the end of the queue
void addToQueue(pthread_t id){
    //make a new node
    struct queueNode* newNode = malloc(sizeof (struct queueNode));
    newNode->threadID = id;
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

//return the thread id of the first thread in the queue
pthread_t popNode(){
    pthread_t result;
    //if the target node is in middle of a queue
    if(head == NULL){
        return 0;
    }
    else if(head == tail){
        result = head->threadID;
        head = NULL;
        tail = NULL;
    }
    else{
        result = head->threadID;
        head = head->next;
    }
    return result;
}


void process(char* fileName){
    struct stat fileInfo;
    char buffer;
    int fileDescriptor;
    int ifAllText = 1;
    //save the file infomation to fileInfo
    int result = stat(fileName, &fileInfo);
    //check if get fileInfo successfully
    if(result == -1){
        numBadFiles++; //if not, this is a bad file
    }
    else if(S_ISREG(fileInfo.st_mode)){//otherwise if this is a regular file
        numReg++;//if so, # of regular files ++
        byteReg += fileInfo.st_size;//add the size to the byte count
        if((fileDescriptor = open(fileName, O_RDONLY)) > 0){// if able to open the file
            while(read(fileDescriptor, &buffer, sizeof(char)) == 1){//read the content 1 char by 1 char
                if(!isprint(buffer) && !isspace(buffer)){//check if text
                    ifAllText = 0;
                    break;
                }
            }
            close(fileDescriptor);//close the file
        }
        
        if(ifAllText){//if all-text file, update the count
            byteText += fileInfo.st_size;
            numAllText ++;
        }
    }
    else if(S_ISDIR(fileInfo.st_mode)){
        numDir ++;
    }
    else{
        numSpl ++;//then the file is special file
    }
    //
}


void *adder(void* argv){
    sem_wait(updateCount);//start the critical region
    process((char*) argv);
    sem_post(updateCount);//end the critical region
    return 0;
}

void rmNewLine(char* someString){//remove the "\n" from each string
    size_t length = strlen(someString);
    if(length > 0){
        if(someString[length - 1] == '\n'){
            someString[length - 1] = '\0';
        }
    }
}

int main(int argc, char** argv){
    readNewFile = malloc(sizeof(sem_t));
    sem_init(readNewFile, 0, 1);
    
    
    //save the wall start time of the process
    struct timeval* startTime = malloc(sizeof(struct timeval));//get finish time for the child process
    gettimeofday(startTime, NULL);
    
    
    //if in serial architecture
    if(argc == 1){
        while(fgets(input, 256, stdin)){
            sem_wait(readNewFile);
            rmNewLine(input);
            //printf("%s", input);
            process(input);
            sem_post(readNewFile);
        }
        

    }
    //if in multi-threaded architecture
    else if(argc == 3 && !strcmp(argv[1], "thread")){
        
        
        numOfThreads = atoi(argv[2]);
        if(numOfThreads > MAX_THREAD){//if the given # of threads is too large
            numOfThreads = MAX_THREAD;
        }
        if(numOfThreads <= 0){
            printf("Please give integer no smaller than 1!\n");
            exit(1);
        }
        updateCount = malloc(sizeof(sem_t));
        sem_init(updateCount, 0, 1);

        
        while(fgets(input, 256, stdin)){//read file name from stdin
            sem_wait(readNewFile);
            rmNewLine(input);
            pthread_t* handler = malloc(sizeof (pthread_t));
            char* fileName = malloc((sizeof(char))* 256 );
            strcpy(fileName, input);
            
            
            if(currentNumThreads < numOfThreads){//if haven't yet reach the limit
                //printf("-----%s\n", fileName);
                pthread_create(handler, NULL, adder, (void*) fileName);
                currentNumThreads += 1;
            }
            else{//otherwise, wait for the first thread started to terminate
                //printf("-----%s\n", fileName);
                pthread_t target = popNode();
                pthread_join(target, NULL);
                pthread_create(handler, NULL, adder, (void*) fileName);
            }
            
            addToQueue(*handler);//add this thread's id to the queue
            sem_post(readNewFile);//allow the main function to read another fileName
            //fgets(input, 256, stdin);
        }
        
        pthread_t target = popNode();//after working on all the given files, wait for all runing threads to terminate
        while(target){
            pthread_join(target, NULL);
            target = popNode();
        }
        
     }
    else{
        printf("Wrong arguments!\n");
        exit(1);
    }
    
    //print the statistics
    printf("Number of bad files: %d\n", numBadFiles);
    printf("Number of directories: %d\n", numDir);
    printf("Number of regular files: %d\n", numReg);
    printf("Number of sepcial files: %d\n", numSpl);
    printf("Total number of bytes of regular files: %d\n", byteReg);
    printf("Number of all-text files: %d\n", numAllText);
    printf("total bytes of all-text files: %d\n", byteText);
    
    //print the CPU time
    struct timeval* endTime = malloc(sizeof(struct timeval));//get finish time for the child process
    gettimeofday(endTime, NULL);
    struct rusage* resources = malloc(sizeof (struct rusage));
    if(getrusage(RUSAGE_SELF, resources) == 0){
        double CPUTimeU =  resources->ru_utime.tv_sec*1000.0 + resources->ru_utime.tv_usec /1000.0;
        printf("--------------------------------------------\nCPU time used(user): %lf\n", CPUTimeU);
        double CPUTimeSys = resources->ru_stime.tv_sec*1000.0 +  resources->ru_stime.tv_usec/1000.0;
        printf("CPU time used(system): %lf\n", CPUTimeSys);
        double wallClockTime = -startTime->tv_sec*1000.0- startTime->tv_usec/1000.0 + endTime->tv_sec*1000.0 + endTime->tv_usec/1000.0;//transfer wall-clock time from timeval to a double value
        printf("Wall-clock time used: %lf\n", wallClockTime);
    }
    



}
