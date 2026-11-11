#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>

//this program uses shared memory to simulate a bank account
//between a parent and child
//semaphores are used to prevent processes changing account at same time
void  ClientProcess(int [], sem_t *);

int main(void)
{
     int    ShmID;
     int    *ShmPTR;
     pid_t  pid;
     int    status;

     int localBalance = 0;
     int amount;
     sem_t *mutex;
    

     ShmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
     if (ShmID < 0) {
          printf("*** shmget error (server) ***\n");
          exit(1);
     }

     ShmPTR = (int *) shmat(ShmID, NULL, 0);
     if (ShmPTR == (void *)-1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
     }
    //bankaccount at 0
     ShmPTR[0]= 0;

     //semaphore with parent and child 
     mutex = mmap(NULL, sizeof(sem_t),
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (mutex == MAP_FAILED){
          perror("mmap error");
          exit(1);
    }
    //only one semaphore need for both processes
    if (sem_init(mutex, 1, 1) == -1){
        perror("sem_init error");
        exit(1);
    }
    printf("Server is about to fork a child process...\n");

    pid = fork();
    if (pid<0){
        printf("** fork error (server)***\n");
        exit(1);
    }
    else if (pid == 0) {
     ClientProcess(ShmPTR, mutex);
     exit(0);
    
    }   
    //Parent process - Dear old dad
    srand(time(NULL) ^ getpid());

    while(1) {
        sleep(rand() % 6);

        printf("Dear Old Dad: Attempting to Check Balance\n");

        //decide if Dad will try to deposit or check balance
        if (rand() % 2 ==0){

            if (localBalance < 100) {
                sem_wait(mutex);
                //get current balance from shared memory
                localBalance = ShmPTR[0];

                amount = rand() % 101;

                if (rand() % 2 == 0){
                      localBalance += amount;
                      printf("Dear old Dad: Deposits $%d/ Balance = $%d\n",
                            amount, localBalance);
    
                }
                else{
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }

                ShmPTR[0] = localBalance;

                //unlock shared bank account
                sem_post(mutex);
            }
            else{
                printf("Dear old Dad: Thinks Student has enough Cash($%d)\n",
                      localBalance);
            
                }
            }
            fflush(stdout);

        }
        wait(&status);
        shmdt((void*) ShmPTR);
        shmctl(ShmID, IPC_RMID, NULL);
        exit(0);
    }

    void ClientProcess(int SharedMem[], sem_t *mutex)
    {
        int localBalance = 0;
        int need;

        srand(time(NULL) ^ getpid());

        while (1) {
              sleep(rand() % 6);

              printf("Poor Student: Attempting to Check Balance\n");
              if (rand() % 2 == 0){

                //lock shared bank account
                sem_wait(mutex);

                localBalance = SharedMem[0];
                need = rand() % 51;

                printf("Poor student needs $%d\n", need);

                if (need <= localBalance){
                    localBalance -= need;

                    printf("Poor student: Withdraws $%d / Balance = $%d\n",
                          need, localBalance);
                }

                else{
                    printf("Poor student: Not Enough Cash ($%d)\n",
                          localBalance);
                }

                SharedMem[0] = localBalance;

                //unlock shared bank account
                sem_post(mutex);
              
              }
              else {
                   printf("Poor Student: Last Checking Balance = $%d\n",
                          localBalance);
              }

              fflush(stdout);
        }




    }