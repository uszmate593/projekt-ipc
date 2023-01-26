#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define N 10
#define K 3

struct library_user
{
    int id;
    int role;
};
typedef struct library_user library_user;

struct book 
{
    int id;
    int to_read[N];
    int author;
    int state;
};
typedef struct book book;

static struct sembuf buf;

void up(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = 1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Semaphore can not go up");
      exit(1);
   }
}

void down(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = -1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Semaphore can not go down");
      exit(1);
   }
}

struct buf_elem {
   long mtype;
   int mvalue;
};

int main()
{
    
    int next_book = 0;
    int user_id;
    library_user *user_addr;
    int book_id;
    book *book_addr;
    int shr_vars_id, *shr_vars_addr;
    int sem_id;
    struct buf_elem msg_buf;
    int msg_id;

    user_id = shmget(5042, N * sizeof(library_user),IPC_CREAT|0600);
    if(user_id == -1)
    {
        perror("failed to create library users sheard memory");
        return 1;
    }
    user_addr = (library_user*)shmat(user_id, NULL, 0)
    if(user_addr == NULL)
    {
        perror("failed to mount library users sheard memory");
        return 1;
    }

    book_id = shmget(5043, K * sizeof(book),IPC_CREAT|0600);
    if(book_id == -1)
    {
        perror("failed to create books sheard memory");
        return 1;
    }
    book_addr = (book*)shmat(book_id, NULL, 0)
    if(book_addr == NULL)
    {
        perror("failed to mount books sheard memory");
        return 1;
    }

    shr_vars_id = shmget(5044, 3 * sizeof(int),IPC_CREAT|0600);
    if(shr_vars_id == -1)
    {
        perror("failed to create variables in sheard memory");
        return 1;
    }
    shr_vars_addr = (int*)shmat(shr_vars_id, NULL, 0)
    if(shr_vars_addr == NULL)
    {
        perror("failed to mount variables from sheard memory");
        return 1;
    }

    sem_id = semget(5042, 4, IPC_CREAT|0600);
    if(sem_id == -1)
    {
        perror("failed to create semaphore table");
        return 1;
    }
    if (semctl(sem_id, 0, SETVAL, (int)1) == -1)
    {
      perror("failed to set value of semaphore 0");
      return 1;
    }
    if (semctl(sem_id, 1, SETVAL, (int)1) == -1)
    {
      perror("failed to set value of semaphore 1");
      return 1;
    }
    if (semctl(sem_id, 2, SETVAL, (int)K) == -1)
    {
      perror("failed to set value of semaphore 2");
      return 1;
    }

    msg_id = msgget(5042, IPC_CREAT);
    if(msg_id == -1)
    {
        perror("failed to create mesege queue");
        return 1;
    }

    for(int i = 0;i < N;i++)
    {
        user_addr[i].id = i;
        if(i < 4)
            user_addr[i].role = 1;
        else
            user_addr[i].role = 0;
    }

    for(int i = 0;i < K;i++)
    {
        book_addr[i].state = 0;
        for(int j = 0;j < N;j++)
            book_addr[i].to_read[j] = 0;
    }

    shr_vars_addr[0] = 0;
    shr_vars_addr[1] = 0;

    int fork_check;
    int change;
    for(int i = 1;i < N;i++)
    {
        fork_check = fork();
        if(fork_check != 0)
            break;
        id = i;
    }

    srand(getpid());
    while(1)
    {
        if(user_addr[id].role == 0)
        {

        }
        else
        {
            
        }
        usleep(1);
        change = rand() % 10;
        usleep(2);
        if(change < 5)
        {
            user_addr[id].role = 0;
            printf("Library user %d relaxed, and is a reader now.\n", id);
        }
        else
        {
            user_addr[id].role = 0;
            printf("Library user %d relaxed, and is a writer now.\n", id);
        }
        usleep(1);
    }

    return 0;
}