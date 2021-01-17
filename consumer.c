#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <ctype.h>
#include <errno.h>

#define N 7

struct msgbuff
{
    long mtype;
    char mtext[256];
};

union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void handler(int signum);

void down(int sem_id)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem_id, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem_id)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem_id, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}

key_t mty_sem_key_id, full_sem_key_id, shm_key_id, mutex_sem_key_id, prc_sem_key_id, cnt_shm_key_id, idx_shm_key_id, cons_sem_key_id, prod_sem_key_id;
int mty_sem_id, full_sem_id, send_val, shm_id, rec_val, mutex_sem_id, prc_sem_id, cnt_shm_id, cons_idx_shm_id, cons_sem_id, prod_sem_id;
int *shmaddr;
int* cnt_ptr;
int* cons_idx_ptr;
int init_cnt = 0, init_cons_idx = 0, cons = 0, prod = 0;
union Semun semun;
struct msgbuff message;
int nProcs, nProd, nCons;
    
int main()
{
    signal(SIGINT, handler);
    
    message.mtype = 7;
    cons = 1;

    mty_sem_key_id = ftok("keyfile", 27);
    full_sem_key_id = ftok("keyfile", 72);
    prc_sem_key_id = ftok("keyfile", 104);
    shm_key_id = ftok("keyfile", 7);
    mutex_sem_key_id = ftok("keyfile", 2);
    cnt_shm_key_id = ftok("keyfile", 10);
    idx_shm_key_id = ftok("keyfile", 4);
    prod_sem_key_id = ftok("keyfile", 140);
    cons_sem_key_id = ftok("keyfile", 14);

    prc_sem_id = semget(prc_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(prc_sem_id == -1){
        if(errno == EEXIST){
            printf("\nProcess Sem Already exists");
            prc_sem_id = semget(prc_sem_key_id, 1, 0666 | IPC_CREAT);
            if(prc_sem_id == -1){
                perror("Error in create");
                exit(-1);
            }
            up(prc_sem_id);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(prc_sem_id, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        printf("\nProcess Sem was created and initliazed");
    }

    mty_sem_id = semget(mty_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(mty_sem_id == -1){
        if(errno == EEXIST){
            printf("\nEmpty Sem Already exists");
            mty_sem_id = semget(mty_sem_key_id, 1, 0666 | IPC_CREAT);
            if(mty_sem_id == -1){
                perror("Error in create");
                exit(-1);
            }
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = N; /* initial value of the semaphore, Binary semaphore */
        if (semctl(mty_sem_id, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        printf("\nEmptySem was created and initliazed");
    }

    full_sem_id = semget(full_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(full_sem_id == -1){
        if(errno == EEXIST){
            printf("\nFull Sem Already exists");
            full_sem_id = semget(full_sem_key_id, 1, 0666 | IPC_CREAT);
            if(full_sem_id == -1){
                perror("Error in create");
                exit(-1);
            }
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(full_sem_id, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        printf("\nFull Sem was created and initliazed");
    }

    prod_sem_id = semget(prod_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(prod_sem_id == -1){
        if(errno == EEXIST){
            printf("\nProducer Sem Already exists");
            prod_sem_id = semget(prod_sem_key_id, 1, 0666 | IPC_CREAT);
            if(prod_sem_id == -1){
                perror("Error in create");
                exit(-1);
            }
            if(prod) up(prod_sem_id);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(prod_sem_id, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        if(prod) up(prod_sem_id);
        printf("\nProducer Sem was created and initliazed");
    }

    cons_sem_id = semget(cons_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(cons_sem_id == -1){
        if(errno == EEXIST){
            printf("\nConsumer Sem Already exists");
            cons_sem_id = semget(cons_sem_key_id, 1, 0666 | IPC_CREAT);
            if(cons_sem_id == -1){
                perror("Error in create");
                exit(-1);
            }
            if(cons) up(cons_sem_id);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(cons_sem_id, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        if(cons) up(cons_sem_id);
        printf("\nConsumer Sem was created and initliazed");
    }
    
    shm_id = shmget(shm_key_id, N * sizeof(int), IPC_CREAT | 0666 | IPC_EXCL);
    if(shm_id == -1){
        if(errno == EEXIST){
            printf("\nShared Memory Already exists");
            shm_id = shmget(shm_key_id, N * sizeof(int), IPC_CREAT | 0666);
            if(shm_id == -1){
                perror("Error in create");
                exit(-1);
            }
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }


    mutex_sem_id = semget(mutex_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(mutex_sem_id == -1){
        if(errno == EEXIST){
            printf("\nConsumer mutex sem Already exists");
            mutex_sem_id = semget(mutex_sem_key_id, 1, 0666 | IPC_CREAT);
            if(mutex_sem_id == -1){
                perror("Error in create");
                exit(-1);
            }
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(mutex_sem_id, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        printf("\nConsumer mutex sem created and init");
    }

    cnt_shm_id = shmget(cnt_shm_key_id, sizeof(int), IPC_CREAT | 0666 | IPC_EXCL);
    if(cnt_shm_id == -1){
        if(errno == EEXIST){
            cnt_shm_id = shmget(cnt_shm_key_id, sizeof(int), IPC_CREAT | 0666);
            if(cnt_shm_id == -1){
                perror("Error in create");
                exit(-1);
            }
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        printf("\nConsumer count memory created and will be init");
        init_cnt = 1;
    }

    cons_idx_shm_id = shmget(idx_shm_key_id, sizeof(int), IPC_CREAT | 0666 | IPC_EXCL);
    if(cons_idx_shm_id == -1){
        if(errno == EEXIST){
            printf("\nConsumer index mem Already Exists");
            cons_idx_shm_id = shmget(idx_shm_key_id, sizeof(int), IPC_CREAT | 0666);
            if(cons_idx_shm_id == -1){
                perror("Error in create");
                exit(-1);
            }
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        printf("\nConsumer index memory created and will be init");
        init_cons_idx = 1;
    }

    printf("Shared Memory ID = %d\n", shm_id);
    printf("Cnt Shared Memory ID = %d\n", cnt_shm_id);
    printf("Consumer Index Shared Memory ID = %d\n", cons_idx_shm_id);
    printf("Semaphor ID = %d", mutex_sem_id); 
    
    shmaddr = shmat(shm_id, (void *)0, 0);
    if (shmaddr == NULL)
    {
        perror("Error in attach in Server");
        exit(-1);
    }
    else
    {
        printf("\nServer: Shared memory attached at address %ls\n", shmaddr);
    }

    cnt_ptr = (int*)shmat(cnt_shm_id, (void *)0, 0);
    if (cnt_ptr == NULL)
    {
        perror("Error in attach in client");
        exit(-1);
    }
    else
    {
        printf("\nClient: Shared memory attached at address %ls\n", shmaddr);
    }
    if(init_cnt){
        *cnt_ptr = 0;
    }

    cons_idx_ptr = shmat(cons_idx_shm_id, (void *)0, 0);
    if (cons_idx_ptr == NULL)
    {
        perror("Error in attach in client");
        exit(-1);
    }
    else
    {
        printf("\nClient: Shared memory attached at address %ls\n", shmaddr);
    }
    if(init_cons_idx){
        *cons_idx_ptr = 0;
    }
    
    int i;
    int item;
    int *buffer = (int*)shmaddr;
    


    while(1){   

        down(full_sem_id);
        down(mutex_sem_id);
        i = *cons_idx_ptr;
        item = buffer[i];
        printf("\nConsumed item %d at pos i = %d", item, i);
        *cons_idx_ptr = i + 1 == N ? 0 : i + 1;
        *cnt_ptr -= 1;
        up(mutex_sem_id);
        up(mty_sem_id);
        //sleep(1);
    }
    return 0;
}


void handler(int signum)
{
    down(prc_sem_id);
    nProcs = semctl(prc_sem_id, 0, GETVAL, semun);
    if(nProcs == -1){
        perror("Error in prc semctl");
        exit(-1);
    }
    else{
        if(nProcs == 0){
            shmdt(shmaddr);
            shmdt(cnt_ptr);
            shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
            shmctl(cnt_shm_id, IPC_RMID, (struct shmid_ds *)0);
            semctl(prc_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
            semctl(mutex_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
            semctl(mty_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
            semctl(full_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
        }
        else{
            shmdt(shmaddr);
            shmdt(cons_idx_ptr);
            shmdt(cnt_ptr);
        }
    }

    down(cons_sem_id);
    nCons = semctl(cons_sem_id, 0, GETVAL, semun);
    if(nCons == -1){
        perror("\nError in semctl, Consumer -> Cons_sem");
        exit(-1);
    }
    else{
        if(nCons == 0){
            semctl(cons_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
            shmdt(cons_idx_ptr);
            shmctl(cons_idx_shm_id, IPC_RMID, (struct shmid_ds *)0);
        }
    }

    nProd = semctl(prod_sem_id, 0, GETVAL, semun);
    if(nProd == -1){
        if(errno != EINVAL){
            perror("Error in semctl");
            exit(-1);
        }
    }
    else{
        if(nProd == 0){
            semctl(prod_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
        }
    }
    exit(0);
}
