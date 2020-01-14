#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <sys/shm.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <math.h>
#include "entry.h"
#define SIZEOFSHAREDOBJECT (NENTRIES*sizeof(entry))
#define lamda 0.2
#define rand0to1() ((double)rand() / (double)RAND_MAX)

int main(int argc, char *argv[]) {
    if (argc < 5){
        printf("Too few arguments. Try:\n<NumOfPeers> <NumOfEntries> <NumOfReps> <PercentOfReads>\n");
        return -1;
    }
    int NPEERS = atoi(argv[1]), NENTRIES = atoi(argv[2]), NREPS = atoi(argv[3]), PERCENT = atoi(argv[4]);
    srand(time(NULL));
    clock_t startWr, endWr, startRd, endRd;
    double cpuTimeUsedWr = 0, cpuTimeUsedRd = 0;
    
    /*Creating shared memory file*/
    int shm_fd; /* shared memory file descriptor */
    entry *db; /* pointer to shared memory obect */
    shm_fd = shm_open("MyShared", O_CREAT | O_RDWR, 0666); /* create the shared memory object */
    if (shm_fd == -1) {
        perror("Failed to open shared memory file");
        return -1;
    }
    if (ftruncate(shm_fd, SIZEOFSHAREDOBJECT) == -1) { /* configure the size of the shared memory object */
        perror("Failed to configure the size of the shared memory object");
        return -1;
    }
    db = mmap(0, SIZEOFSHAREDOBJECT, PROT_WRITE, MAP_SHARED, shm_fd, 0); /* memory map the shared memory object */
    if(db == MAP_FAILED){
        perror("Failed to memory map the shared memory object");
        return -1;
    }
    
    for(int i = 0; i<NENTRIES; i++){ /*initialize entries*/
        db[i].read = 0;
        db[i].written = 0;
        strcpy(db[i].data,"AAAAAAA");
    }
    
    /*Creating sems*/
    sem_t *rw_mutex[NENTRIES];// semaphores are set at 1
    sem_t *mutex[NENTRIES];//
    for (int nameOfSemInInt = 1; nameOfSemInInt<=NENTRIES; nameOfSemInInt++){
        char nameOfSemInStrRW[13];
        char nameOfSemInStrM[13];
        sprintf(nameOfSemInStrRW, "%d", nameOfSemInInt);
        sprintf(nameOfSemInStrM, "%d", nameOfSemInInt*(-1));
        rw_mutex[nameOfSemInInt-1] = sem_open(nameOfSemInStrRW, O_CREAT, 0644, 1);
        if (rw_mutex[nameOfSemInInt-1] == SEM_FAILED) {
            perror("Failed to open semphore");
            return -1;
        }
        mutex[nameOfSemInInt-1] = sem_open(nameOfSemInStrM, O_CREAT, 0644, 1);
        if (mutex[nameOfSemInInt-1] == SEM_FAILED) {
            perror("Failed to open semphore");
            return -1;
        }
    }
    
    /*Creating shared memory file for read_count*/
    int shm_fd2; /* shared memory file descriptor */
    int *read_count; /* pointer to shared memory obect */
    shm_fd2 = shm_open("readCount", O_CREAT | O_RDWR, 0666); /* create the shared memory object */
    if (ftruncate(shm_fd2, sizeof(int)*NENTRIES) == -1) { /* configure the size of the shared memory object */
        perror("Failed to configure the size of the shared memory object");
        return -1;
    }
    read_count = mmap(0, sizeof(int)*NENTRIES, PROT_WRITE, MAP_SHARED, shm_fd2, 0); /* memory map the shared memory object */
    if(read_count == MAP_FAILED){
        perror("Failed to memory map the shared memory object");
        return -1;
    }
    for (int i=0; i<NENTRIES; i++)
        read_count[i] = 0;
    
    /*Creating semaphore for printing*/
    sem_t *print_sem = sem_open("p_sem", O_CREAT, 0644, 1);
    if (print_sem == SEM_FAILED) {
        perror("Failed to open semphore");
        return -1;
    }
    
    for(int i=0;i<NPEERS;i++) {
        if(fork() == 0) {
            srand(getpid());
            int numReads = 0, numWrites = 0; //counters for individual peers
            float totalWaitTime = 0;
            for(int k=0;k<NREPS;k++){
                int selectedEntry = rand() % NENTRIES; //randomly select entry
                if(rand()%100 <= PERCENT){ //if is a reader
                    numReads++;
                    
                    sem_wait(mutex[selectedEntry]);
                    read_count[selectedEntry]++;
                    startRd = clock();
                    if (read_count[selectedEntry] == 1)
                        sem_wait(rw_mutex[selectedEntry]);
                    endRd = clock();
                    cpuTimeUsedRd += ((double) (endRd - startRd)) / CLOCKS_PER_SEC;
                    db[selectedEntry].read++;
                    sem_post(mutex[selectedEntry]);
                    
                    sleep((int) -log(rand0to1())/lamda);
                    
                    sem_wait(mutex[selectedEntry]);
                    read_count[selectedEntry]--;
                    if (read_count[selectedEntry] == 0)
                        sem_post(rw_mutex[selectedEntry]);
                    sem_post(mutex[selectedEntry]);
                }
                else{  // if writer
                    numWrites++;
                    
                    startWr = clock();
                    sem_wait(rw_mutex[selectedEntry]);
                    endWr = clock();
                    cpuTimeUsedWr += ((double) (endWr - startWr)) / CLOCKS_PER_SEC;
                    
                    sleep((int) -log(rand0to1())/lamda);
                    db[selectedEntry].written++;
                    db[selectedEntry].data[rand() % 7]++; // the actual writting taking place;
                    
                    sem_post(rw_mutex[selectedEntry]);
                }
            }
            cpuTimeUsedRd = numReads ? cpuTimeUsedRd / numReads : 0;
            cpuTimeUsedWr = numWrites ? cpuTimeUsedWr / numWrites : 0;
            sem_wait(print_sem);
            printf("\nPeer is done. Stats:\nReads: %d Writes: %d\n",numReads,numWrites);
            printf("Average read time: %f Average write time: %f\n",cpuTimeUsedRd,cpuTimeUsedWr);
            sem_post(print_sem);
            return 0;
        }
    }
    for(int i=0;i<=NPEERS;i++)
        wait(NULL);
    for(int i = 0; i<NENTRIES; i++) //print entries
        printf("Times read: %d  Times Wirtten: %d  Data: %s\n",db[i].read,db[i].written,db[i].data);
    if(shm_unlink("MyShared") == -1){
        perror("Failed to unlink the shared memory object");
        return -1;
    }
    if(shm_unlink("readCount") == -1){
        perror("Failed to unlink the shared memory object");
        return -1;
    }
    for (int i = 0; i<NENTRIES; i++){
        if (sem_destroy(rw_mutex[i]) == -1) {
            perror("Failed to destroy semphore");
            return -1;
        }
        if (sem_destroy(mutex[i]) == -1) {
            perror("Failed to destroy semphore");
            return -1;
        }
    }
    if (sem_destroy(print_sem) == -1) {
        perror("Failed to destroy semphore");
        return -1;
    }
    return 0;
}
