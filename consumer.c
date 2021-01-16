#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

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

key_t msgq_key_id, shm_key_id, mutex_sem_key_id, cnt_shm_key_id, idx_shm_key_id;
int msgq_id, send_val, shm_id, rec_val, mutex_sem_id, cnt_shm_id, cns_idx_shm_id;
int *shmaddr;
int* cnt_ptr;
int* cns_idx_ptr;


int main()
{
    signal(SIGINT, handler);
    
    msgq_key_id = ftok("keyfile", 27);
    shm_key_id = ftok("keyfile", 7);
    mutex_sem_key_id = ftok("keyfile", 2);
    cnt_shm_key_id = ftok("keyfile", 10);
    idx_shm_key_id = ftok("keyfile", 4);

    msgq_id = msgget(msgq_key_id, 0666 | IPC_CREAT);
    shm_id = shmget(shm_key_id, N * sizeof(int), IPC_CREAT | 0666);
    mutex_sem_id = semget(mutex_sem_key_id, 1, 0666 | IPC_CREAT);
    cnt_shm_id = shmget(cnt_shm_key_id, sizeof(int), IPC_CREAT | 0666);
    cns_idx_shm_id = shmget(idx_shm_key_id, sizeof(int), IPC_CREAT | 0666);


    if (msgq_id == -1 || shm_id == -1 || mutex_sem_id == -1 || cnt_shm_id == -1 || cns_idx_shm_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    printf("Message Queue ID = %d\n", msgq_id);
    printf("Shared Memory ID = %d\n", shm_id);
    printf("Semaphor ID = %d\n", mutex_sem_id);

    
    union Semun semun;
    struct msgbuff message;
    message.mtype = 7;

    
    shmaddr = shmat(shm_id, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("Error in attach in Server");
        exit(-1);
    }
    else
    {
        printf("\nServer: Shared memory attached at address %x\n", shmaddr);
    }

    cnt_ptr = (int*)shmat(cnt_shm_id, (void *)0, 0);
    if (shmaddr == NULL)
    {
        perror("Error in attach in client");
        exit(-1);
    }
    else
    {
        printf("\nClient: Shared memory attached at address %x\n", shmaddr);
    }

    cns_idx_ptr = shmat(cns_idx_shm_id, (void *)0, 0);
    if (shmaddr == NULL)
    {
        perror("Error in attach in client");
        exit(-1);
    }
    else
    {
        printf("\nClient: Shared memory attached at address %x\n", shmaddr);
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
        }
        down(mutex_sem_id);
        i = *cns_idx_ptr;
        item = buffer[*cns_idx_ptr];
        *cns_idx_ptr = i - 1 == -1 ? N - 1 : i - 1;
        up(mutex_sem_id);

        if(N == *cnt_ptr){
            send_val = msgsnd(msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
            if (send_val == -1)
                perror("Errror in send");
        }


            
    }

    return 0;
}


void handler(int signum)
{
    printf("\nAbout to destroy the shared memory area !\n");
    shmctl(shm_id, IPC_RMID, (struct shmid_ds *)0);
    exit(0);
}
