//
//  doit.c
//  
//
//  Yixue Wang-ywang20
//
//



/*Differences between a real shell and this shell:
        -For cd command, can not use directory like ~ and title including *
        -Can not use pipe
        -Can not use '>', like cat > , cat >> to add contents to a certern file
        -can not switch to su
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>


//since getrusage get all children processes, need to record all finished CPU time and fault
double terminatedCPUTimeU = 0;
double terminatedCPUTimeSys = 0;
int terminatedMajor = 0;
int terminatedMinor = 0;

//print the statistics of the child process(finished), using the startTime and endTime recorded
void printStatistics(char* name, struct timeval* startTime, struct timeval* endTime){
    struct rusage* resources =  malloc(sizeof(struct rusage));//allocate space for saving the rusage
    double wallClockTime = -startTime->tv_sec*1000.0- startTime->tv_usec/1000.0 + endTime->tv_sec*1000.0 + endTime->tv_usec/1000.0;//transfer wall-clock time from timeval to a double value
    if (getrusage(RUSAGE_CHILDREN, resources) == 0) { // if successfully get the rusage, print all the data
        
        
        //calculate the CPU time of desired process by minus the CPU time of terminated process from the overall CPU time
        double CPUTimeU =  resources->ru_utime.tv_sec*1000.0 + resources->ru_utime.tv_usec /1000.0-terminatedCPUTimeU;
        printf("CPU time used(user): %lf\n", CPUTimeU);
        double CPUTimeSys = resources->ru_stime.tv_sec*1000.0 +  resources->ru_stime.tv_usec/1000.0-terminatedCPUTimeSys;
        printf("CPU time used(system): %lf\n", CPUTimeSys);
        printf("Wall-clock time used: %lf\n", wallClockTime);
        printf("Involuntary: %ld\n", resources->ru_nivcsw);
        printf("Voluntary: %ld\n", resources->ru_nvcsw);
        printf("Number of major page fault: %ld\n", resources->ru_majflt - terminatedMajor);
        printf("Number of minor page faults: %ld\n\n\n", resources->ru_minflt - terminatedMinor);
        
        
        //update the CPU time and faults of terminated processes
        terminatedCPUTimeU = resources->ru_utime.tv_sec*1000.0 + resources->ru_utime.tv_usec /1000.0;
        terminatedCPUTimeSys = resources->ru_stime.tv_sec*1000.0 +  resources->ru_stime.tv_usec/1000.0;
        terminatedMinor = resources->ru_minflt;
        terminatedMajor = resources->ru_majflt;
    }
}


//when running background processes, a linked-list to record the information of running bg processes is needed
struct processNode{
    int pid;//pid of the bg process
    char* name;
    struct timeval* startTime;//wall-clock time
    struct processNode* previousProcess;
    struct processNode* nextProcess;//pointer to the next process, in order to form a linkedlist
};



//linkedlist to record all the running background processes
struct processNode* head;//points to the first background process. when no background process running, point to null
struct processNode* tail;//point to the most recently added background process
int size = 0;//record how many background processes are running

void addNewProcess(struct processNode* newProcess){//add a new process to the linkedlist
    if(!head){//when there is no background process running, make it head
        head = newProcess;
        tail = newProcess;
    }
    else{//otherwise add the new process to the tail
        tail->nextProcess = newProcess;
        newProcess->previousProcess = tail;
        tail = tail->nextProcess;
    }
    size++;//size plus one
}


//remove one process from the linkedlist
void removeProcess(struct processNode* target){
    if(target->previousProcess && target->nextProcess){//if the target process is in the middle
        target->previousProcess->nextProcess = target->nextProcess;
        target->nextProcess->previousProcess = target->previousProcess;
        return;
    }else if(target->previousProcess){//if the target is at the tail
        target->previousProcess->nextProcess = NULL;
        tail = target->previousProcess;
    }else if (target->nextProcess){//if the target is at the head
        target->nextProcess->previousProcess = NULL;
        head = target->nextProcess;
    }
    else{//if the target is the only process in the linked list
        head = NULL;
        tail = NULL;
    }
    
}


//consider a pid and find the process of same pid, remove the process from linkedlist and print the "Completed Message" and required statiscs
int checkProcessStatus(int pid){
    struct processNode* iterator = head;
    while(iterator){
        if(pid == iterator->pid){
            //if ended
            removeProcess(iterator);
            printf("\n[%d]\t%s\t%d\tCompleted\n",size, iterator->name, iterator->pid);
            size--;
            struct timeval* current = malloc(sizeof(struct timeval));
            gettimeofday(current, NULL);
            printStatistics(iterator->name, iterator->startTime, current);
            return 0;
        }
        iterator = iterator-> nextProcess;
    }
    return 1;
}



//considering 2 arguments: the array of argv, and if the giving command should be run background
void myExec(char* argv[], int ifBack){
    int pid;
    struct timeval* startTime =  malloc(sizeof(struct timeval));
    gettimeofday(startTime, NULL);//store the starttime
    if((pid = fork())< 0){//if the fork failed
        printf("Fork error\n");
        exit(1);
    }
    else if (pid == 0){//if child process
        if(argv[0]){
            int result;
            if((result = execvp(argv[0], argv)) < 0){//execute the given command, if failed, print execution error and exit
                printf("Exec error\n");
                exit(1);
            }
        }
        
    }
    else{//if parent process
        //checkProcessStatus();
        if(!ifBack){ //if the argv asks the shell to run a foreground process
            int getPid = wait(NULL); //wait for running processes to be finished
            checkProcessStatus(getPid);//check if the finished process is a backround process
            wait(NULL);//wait for the child process to be finished
            printf("\n");
            struct timeval* endTime = malloc(sizeof(struct timeval));//get finish time for the child process
            gettimeofday(endTime, NULL);
            printStatistics(argv[0], startTime, endTime);//print the statistics of the child process
    
            
            
        }else{//if the argv asks the shell to run a background process
            struct processNode* newProcess = malloc(sizeof(struct processNode));//create a process node waiting to be added to the linkedlis
            newProcess->pid = pid;//set all the neccessary component
            newProcess->name = malloc(sizeof argv[0]);
            strcpy(newProcess->name, argv[0]);
            newProcess->startTime = startTime;
            newProcess->previousProcess = NULL;
            newProcess->nextProcess = NULL;
            addNewProcess(newProcess);//add the new process to the linkedlist
            printf("\n[%d]%s\t%d\n", size,argv[0], pid);//tell the user that the underground process is now running
            return;
        }
        
    }
    
}

//parse the string to arguments can directly be used by execvp
void parse(char* input, char* target[32]){
    char* token;
    token = strtok(input, " \n");//remove the \n from the string first
    int i = 0;
    while(token != NULL){//if there is any charactor besides the \n, continue
        target[i] = token;
        i++;
        token = strtok(NULL, " \n");
    }//read the string word by word and store them as arguments
    target[i] = NULL;//set the argument after the last one as NULL to let the user know where is the last arguments
}

int main(int argc, char* argv[32]){
    char* command = malloc(sizeof(char)*128);
    char* prompt = "==>";//set the default prompt as ==>
    if(argc > 1){//if the command are given ,run it
        if(strcmp(argv[1], "exit") == 0){//if the given command is to exit, exit
            printf("quit the shell...\n");
            exit(0);
        }
        else{
            myExec(argv+1, 0);//otherwise just run the command
        }
    
    }
    else{
        printf("Shell:%s", prompt);//if there is no argument give, enter the shell
        
        while(1){//waiting for the user to enter command
            fgets(command, 128, stdin);//read the input of the user
            command = strtok(command, "\n");//remove the enter from the command
            if(command == NULL){//if it is the end of the file, just exit
                exit(0);
            }
            parse(command, argv);//otherwise parse the input
            if(strcmp(argv[0], "exit") == 0){//if input equals exit, exit
                printf("quit the shell...\n");
                exit(0);
            }
            else if(strcmp(argv[0], "cd") == 0 ){//if the input ask to change directory, go
                if(argv[1]){
                    chdir(argv[1]);
                }
            }
            else if(strcmp(argv[0], "jobs") == 0){//if the input ask to list all running background processes,
                struct processNode* current = head;
                while (current) {//loop through the linked list and print the processes
                    printf("[%d] %d   %s\n", size, current->pid, current->name );
                    current = current->nextProcess;
                }
            }
            //if the user want to change the prompt, reset it
            else if(!strcmp(argv[0], "set") && argv[1] && argv[2] && argv[3] && !strcmp(argv[1], "prompt") && !strcmp(argv[2], "="))
            {
                prompt = argv[3];
            }
            //if the user want to user common commands
            else{
                int i = 0;
                while(argv[i]){
                    i++;
                }//get to the last argument
                if(!strcmp(argv[i-1], "&")){//if it is background process
                    argv[i-1] = NULL;
                    myExec(argv, 1);
                }
                else{//if it is foreground process
                    myExec(argv, 0);
                }
                
            }
            printf("%s", prompt);//print the prompt for next command
        }
        
    }
}

