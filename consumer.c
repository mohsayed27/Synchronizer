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

key_t msgq_key_id, shm_key_id, mutex_sem_key_id, prc_sem_key_id, cnt_shm_key_id, idx_shm_key_id, cons_sem_key_id;
int msgq_id, send_val, shm_id, rec_val, mutex_sem_id, prc_sem_id, cnt_shm_id, cons_idx_shm_id, cons_sem_id;
int *shmaddr;
int* cnt_ptr;
int* cons_idx_ptr;
int init_cnt = 0, init_cons_idx = 0;
union Semun semun;
struct msgbuff message;
    
int main()
{
    signal(SIGINT, handler);
    
    message.mtype = 7;

    prc_sem_key_id = ftok("keyfile", 104);
    msgq_key_id = ftok("keyfile", 27);
    shm_key_id = ftok("keyfile", 7);
    mutex_sem_key_id = ftok("keyfile", 2);
    cnt_shm_key_id = ftok("keyfile", 10);
    idx_shm_key_id = ftok("keyfile", 4);
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

    cons_sem_id = semget(cons_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(cons_sem_id == -1){
        if(errno == EEXIST){
            printf("\nConsumer Sem Already exists");
            cons_sem_id = semget(cons_sem_key_id, 1, 0666 | IPC_CREAT);
            if(cons_sem_id == -1){
                perror("Error in create");
                exit(-1);
            }
            up(cons_sem_id);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(cons_sem_id, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        printf("\nConsumer Sem was created and initliazed");
    }
    
    msgq_id = msgget(msgq_key_id, 0666 | IPC_CREAT | IPC_EXCL);
    if(msgq_id == -1){
        if(errno == EEXIST){
            printf("\nMess Queue Already exists");
            msgq_id = msgget(msgq_key_id, 0666 | IPC_CREAT);
            if(msgq_id == -1){
                perror("Error in create");
                exit(-1);
            }
        }
        else{
            perror("Error in create");
            exit(-1);
        }
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

    
    printf("\nMessage Queue ID = %d\n", msgq_id);
    printf("Shared Memory ID = %d\n", shm_id);
    printf("Cnt Shared Memory ID = %d\n", cnt_shm_id);
    printf("Consumer Index Shared Memory ID = %d\n", shm_id);
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

    while(1)
    {   
        if(0 == *cnt_ptr){
            printf("\nBuffer is empty");
            rec_val = msgrcv(msgq_id, &message, sizeof(message.mtext), message.mtype, !IPC_NOWAIT);
            if (rec_val == -1)
                perror("Error in receive");
            else 
                printf("\nmessage received from producer: %s", message.mtext);
        }
        down(mutex_sem_id);
        i = *cons_idx_ptr;
        item = buffer[i];
        printf("\nConsumed item %d at pos i = %d", item, i);
        sleep(1);
        *cons_idx_ptr = i + 1 == N ? 0 : i + 1;
        *cnt_ptr -= 1;
        up(mutex_sem_id);

        if(N - 1 == *cnt_ptr){
            strcpy(message.mtext, "I have consumed");
            send_val = msgsnd(msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
            if (send_val == -1)
                perror("Errror in send");
            else
                printf("\nmessage sent from consumer: %s", message.mtext);
            
        }
    }
    return 0;
}


void handler(int signum)
{
    down(prc_sem_id);
    rec_val = semctl(prc_sem_id, 0, GETVAL, semun);
    if(rec_val == -1){
        perror("Error in semctl");
        exit(-1);
    }
    else{
        if(rec_val == 0){
            shmdt(shmaddr);
            shmdt(cnt_ptr);
            msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
            shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
            shmctl(cnt_shm_id, IPC_RMID, (struct shmid_ds *)0);
            semctl(prc_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
            semctl(mutex_sem_id, 0, IPC_RMID, (struct semid_ds *)0);
        }
        else{
            shmdt(shmaddr);
            shmdt(cons_idx_ptr);
            shmdt(cnt_ptr);
        }
    }
    down(cons_sem_id);
    rec_val = semctl(prc_sem_id, 0, GETVAL, semun);
    if(rec_val == -1){
        perror("Error in semctl");
        exit(-1);
    }
    else{
        if(rec_val == 0){
            shmdt(cons_idx_ptr);
            shmctl(cons_idx_shm_id, IPC_RMID, (struct shmid_ds *)0);
        }
    }
    exit(0);
}
