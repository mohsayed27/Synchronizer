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

key_t msgq_key_id, shm_key_id, sem_key_id;
int msgq_id, send_val, shm_id, rec_val, sem_id;
void *shmaddr;



int main()
{
    signal(SIGINT, handler);

    
    
    
    msgq_key_id = ftok("keyfile", 27);
    shm_key_id = ftok("keyfile", 7);
    sem_key_id = ftok("keyfile", 2);

    msgq_id = msgget(msgq_key_id, 0666 | IPC_CREAT);
    shm_id = shmget(shm_key_id, 4096, IPC_CREAT | 0666);
    sem_id = semget(sem_key_id, 1, 0666 | IPC_CREAT);

    if (msgq_id == -1 || shm_id == -1 || sem_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    printf("Message Queue ID = %d\n", msgq_id);
    printf("Shared Memory ID = %d\n", shm_id);
    printf("Semaphor ID = %d\n", sem_id);

    

    struct msgbuff message;
    message.mtype = 7;


    shmaddr = shmat(shm_id, (void *)0, 0);
    if (shmaddr == NULL)
    {
        perror("Error in attach in client");
        exit(-1);
    }
    else
    {
        printf("\nClient: Shared memory attached at address %x\n", shmaddr);
    }
    
    int item;
    int prod_cnt = 0;
    int count = 0;
    int *buffer = (int*)shmaddr;
    int i = 0;
    int idx;

    while(1){
        item = prod_cnt + 1;
        if(N == count){
            rec_val = msgrcv(msgq_id, &message, sizeof(message.mtext), message.mtype, !IPC_NOWAIT);
            if (rec_val == -1)
                perror("Error in receive");
        }
        down(sem_id);
        buffer[i] = item;
        idx = i + 1 == N ? 0 : i + 1;
        up(sem_id);

        if(1 == count){
            send_val = msgsnd(msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
            if (send_val == -1)
                perror("Errror in send");
        }

    }

    return 0;
}


void handler(int signum)
{
    printf("\nClient Detaching...");
    shmdt(shmaddr);
    exit(0);
}
