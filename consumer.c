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

#define N 100

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

key_t msgq_key_id, shm_key_id, mutex_sem_key_id, prc_sem_key_id, cnt_shm_key_id, idx_shm_key_id;
int msgq_id, send_val, shm_id, rec_val, mutex_sem_id, prc_sem_id, cnt_shm_id, cons_idx_shm_id;
int *shmaddr;
int* cnt_ptr;
int* cons_idx_ptr;
int init_cnt = 0, init_cons_idx = 0;

int main()
{
    signal(SIGINT, handler);

    union Semun semun;
    struct msgbuff message;
    message.mtype = 7;
    
    prc_sem_key_id = ftok("keyfile", 104);
    msgq_key_id = ftok("keyfile", 27);
    shm_key_id = ftok("keyfile", 7);
    mutex_sem_key_id = ftok("keyfile", 2);
    cnt_shm_key_id = ftok("keyfile", 10);
    idx_shm_key_id = ftok("keyfile", 4);

    prc_sem_id = semget(prc_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(prc_sem_id == -1){
        if(errno == EEXIST){
            prc_sem_id = semget(prc_sem_key_id, 1, 0666 | IPC_CREAT);
            up(prc_sem_id);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(prc_sem_id, 1, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
    }


    msgq_id = msgget(msgq_key_id, 0666 | IPC_CREAT | IPC_EXCL);
    if(msgq_id == -1){
        if(errno == EEXIST){
            msgq_id = msgget(msgq_key_id, 0666 | IPC_CREAT);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }

    shm_id = shmget(shm_key_id, N * sizeof(int), IPC_CREAT | 0666 | IPC_EXCL);
    if(shm_id == -1){
        if(errno == EEXIST){
            shm_id = shmget(shm_key_id, N * sizeof(int), IPC_CREAT | 0666);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }


    mutex_sem_id = semget(mutex_sem_key_id, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(mutex_sem_id == -1){
        if(errno == EEXIST){
            mutex_sem_id = semget(mutex_sem_key_id, 1, 0666 | IPC_CREAT);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(mutex_sem_id, 1, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
    }

    cnt_shm_id = shmget(cnt_shm_key_id, sizeof(int), IPC_CREAT | 0666 | IPC_EXCL);
    if(cnt_shm_id == -1){
        if(errno == EEXIST){
            cnt_shm_id = shmget(cnt_shm_key_id, sizeof(int), IPC_CREAT | 0666);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        init_cnt = 1;
    }

    cons_idx_shm_id = shmget(idx_shm_key_id, sizeof(int), IPC_CREAT | 0666 | IPC_EXCL);
    if(cons_idx_shm_id == -1){
        if(errno == EEXIST){
            cons_idx_shm_id = shmget(idx_shm_key_id, sizeof(int), IPC_CREAT | 0666);
        }
        else{
            perror("Error in create");
            exit(-1);
        }
    }
    else{
        init_cons_idx = 1;
    }

    
    printf("Message Queue ID = %d\n", msgq_id);
    printf("Shared Memory ID = %d\n", shm_id);
    printf("Semaphor ID = %d\n", mutex_sem_id); 
    
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
    if (shmaddr == NULL)
    {
        perror("Error in attach in client");
        exit(-1);
    }
    else
    {
        printf("\nClient: Shared memory attached at address %ls\n", shmaddr);
    }

    cons_idx_ptr = shmat(cons_idx_shm_id, (void *)0, 0);
    if (shmaddr == NULL)
    {
        perror("Error in attach in client");
        exit(-1);
    }
    else
    {
        printf("\nClient: Shared memory attached at address %ls\n", shmaddr);
    }

    
    int i;
    int item;
    int *buffer = (int*)shmaddr;

    while(1)
    {   
        if(0 == *cnt_ptr){
            rec_val = msgrcv(msgq_id, &message, sizeof(message.mtext), message.mtype, !IPC_NOWAIT);
            if (rec_val == -1)
                perror("Error in receive");
            else 
                printf("\nmessage received from producer: %s", message.mtext);
        }
        down(mutex_sem_id);
        i = *cons_idx_ptr;
        item = buffer[i];
        *cons_idx_ptr = i - 1 == -1 ? N - 1 : i - 1;
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
    rec_val = semctl(prc_sem_id, 1, GETVAL);
    if(rec_val == -1){
        perror("Error in semctl");
        exit(-1);
    }
    else{
        if(rec_val == 0){
            shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
            shmctl(cons_idx_shm_id, IPC_RMID, (struct shmid_ds *)0);
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
    exit(0);
}
