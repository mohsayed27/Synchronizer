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

key_t msgq_key_id, shm_key_id, sem_key_id;
int msgq_id, rec_val, shm_id, send_val, sem_id;
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

    struct msgbuff message;
    message.mtype = 7;
    
    int i = 0;
    int count = 0;
    int item;
    int *buffer = (int*)shmaddr;

    while(1)
    {
        if(0 == count){
            rec_val = msgrcv(msgq_id, &message, sizeof(message.mtext), message.mtype, !IPC_NOWAIT);
            if (rec_val == -1)
                perror("Error in receive");
        }
        down(sem_id);
        item = buffer[(i--)%N];
        up(sem_id);

        if(N == count){
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
