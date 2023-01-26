#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define N 10
#define K 7

struct library_user
{
    int books_to_read[K];
    int role;
};
typedef struct library_user library_user;

struct book 
{
    int id;
    int to_read;
    int author;
    int state;
};
typedef struct book book;

static struct sembuf buf;

void up(int semid, int semnum, int value){
   buf.sem_num = semnum;
   buf.sem_op = value;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Semaphore can not go up");
      exit(1);
   }
}

void down(int semid, int semnum, int value){
   buf.sem_num = semnum;
   buf.sem_op = -value;
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
    int id;
    int user_id;
    library_user *user_addr;
    int book_id;
    book *book_addr;
    int shr_vars_id, *shr_vars_addr;
    int sem_id;
    struct buf_elem msg_buf;
    int msg_id[N];

    user_id = shmget(0x5042, N * sizeof(library_user),IPC_CREAT|0600);
    if(user_id == -1)
    {
        perror("failed to create library users sheard memory");
        return 1;
    }
    user_addr = (library_user*)shmat(user_id, NULL, 0);
    if(user_addr == NULL)
    {
        perror("failed to mount library users sheard memory");
        return 1;
    }

    book_id = shmget(0x5043, K * sizeof(book),IPC_CREAT|0600);
    if(book_id == -1)
    {
        perror("failed to create books sheard memory");
        return 1;
    }
    book_addr = (book*)shmat(book_id, NULL, 0);
    if(book_addr == NULL)
    {
        perror("failed to mount books sheard memory");
        return 1;
    }

    shr_vars_id = shmget(0x5044, 3 * sizeof(int),IPC_CREAT|0600);
    if(shr_vars_id == -1)
    {
        perror("failed to create variables in sheard memory");
        return 1;
    }
    shr_vars_addr = (int*)shmat(shr_vars_id, NULL, 0);
    if(shr_vars_addr == NULL)
    {
        perror("failed to mount variables from sheard memory");
        return 1;
    }

    sem_id = semget(0x5042, 3, IPC_CREAT|0600);
    if(sem_id == -1)
    {
        perror("failed to create semaphore table");
        return 1;
    }
    if (semctl(sem_id, 0, SETVAL, (int)N) == -1)
    {
      perror("failed to set value of semaphore 0");
      return 1;
    }
    if (semctl(sem_id, 1, SETVAL, (int)K) == -1)
    {
      perror("failed to set value of semaphore 1");
      return 1;
    }
    if (semctl(sem_id, 2, SETVAL, (int)1) == -1)
    {
      perror("failed to set value of semaphore 2");
      return 1;
    }
    for(int i = 0;i < N;i++)
    {
        msg_id[i] = msgget(0x5042 + i, IPC_CREAT|0666);
        if(msg_id[i] == -1)
        {
            perror("failed to create mesege queue");
            return 1;
        }
    }
    

    for(int i = 0;i < N;i++)
    {
        if(i < 3)
        {
            user_addr[i].role = 1;
            shr_vars_addr[1]++;
        } 
        else
            user_addr[i].role = 0;
        for(int j = 0;j < K;j++)
            user_addr[i].books_to_read[j] = -1;
    }

    for(int i = 0;i < K;i++)
    {
        book_addr[i].state = 0;
        book_addr[i].to_read = 0;
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
    int read_check, read;
    srand(getpid());
    while(1)
    {
        if(user_addr[id].role == 0)
        {
            printf("Library user %d reader.\n", id);
            down(sem_id, 0, 1);
            read_check = msgrcv(msg_id[id], &msg_buf, sizeof(int), -shr_vars_addr[0], IPC_NOWAIT);
            if(read_check > 0)
            {
                read = msg_buf.mvalue;
                for(int i = 0;i < K;i++)
                {
                    if(book_addr[i].id == read)
                    {
                        down(sem_id, 2, 1);
                        book_addr[i].to_read--;
                        printf("Library user %d read %d by %d as a reader.\n", id, read, book_addr[i].author);
                        if(book_addr[i].to_read == 0)
                        {
                            book_addr[i].state = 0;
                            up(sem_id, 1, 1);
                            printf("%d read by everyone\n", i);
                        }
                        up(sem_id, 2, 1);
                        break;
                    }
                }
                for(int i = 0;i < K;i++)
                {
                    if(user_addr[id].books_to_read[i] == read)
                    {
                        user_addr[id].books_to_read[i] = -1;
                        break;
                    }
                }
            }
            up(sem_id, 0, 1);
        }
        else
        {
            printf("Library user %d writer.\n", id);
            down(sem_id, 0, N);
            read_check = msgrcv(msg_id[id], &msg_buf, sizeof(int), -shr_vars_addr[0], IPC_NOWAIT);
            printf("%d\n", read_check);
            if(read_check > 0)
            {
                read = msg_buf.mvalue;
                for(int i = 0;i < K;i++)
                {
                    if(book_addr[i].id == read)
                    {
                        down(sem_id, 2, 1);
                        book_addr[i].to_read--;
                        printf("Library user %d read %d by %d as a writer.\n", id, read, book_addr[i].author);
                        if(book_addr[i].to_read == 0)
                        {
                            book_addr[i].state = 0;
                            up(sem_id, 1, 1);
                            printf("%d read by everyone\n", i);
                        }
                        up(sem_id, 2, 1);
                        break;
                    }
                }
                for(int i = 0;i < K;i++)
                {
                    if(user_addr[id].books_to_read[i] == read)
                    {
                        user_addr[id].books_to_read[i] = -1;
                        break;
                    }
                }
            }
            
            up(sem_id, 0, N);
            int decision = 0;
            for(int i = 0;i < K;i++)
            {
                if(user_addr[id].books_to_read[i] != -1)
                {
                    for(int j = 0;j < K;j++)
                    {
                        if(book_addr[j].id == user_addr[id].books_to_read[i] && book_addr[j].to_read <= shr_vars_addr[1])
                        {
                            decision = 1;
                            break;
                        }
                        if(decision)
                            break;
                    }
                }
            }
            if(decision == 0)
            {
                down(sem_id, 1, 1);
                down(sem_id, 0, N);
                msg_buf.mvalue = shr_vars_addr[0];
                msg_buf.mtype = shr_vars_addr[0] + 1;
                for(int i = 0;i < K;i++)
                {
                    if(book_addr[i].state == 0)
                    {
                        book_addr[i].state = 1;
                        book_addr[i].author = id;
                        book_addr[i].id = shr_vars_addr[0];
                        shr_vars_addr[0]++;
                        read = i;
                        printf("Library user %d writen %d.\n", id, book_addr[i].id);
                        break;
                    }
                }
                for(int i = 0;i < N;i++)
                {
                    down(sem_id, 2, 1);
                    if(user_addr[i].role == 0)
                    {
                        if(msgsnd(msg_id[i], &msg_buf, sizeof(int), 0) == -1)
                        {
                            perror("Failed to send message");
                            return 1;
                        }
                        printf("send to %d\n", i);
                        book_addr[read].to_read++;
                        for(int j = 0;j < K;j++)
                        {
                            if(user_addr[i].books_to_read[j] == -1)
                            {
                                user_addr[i].books_to_read[j] = msg_buf.mvalue;
                                break;
                            }
                        }
                    }
                    up(sem_id, 2, 1);
                }
                if(book_addr[read].to_read == 0)
                {
                    book_addr[read].state = 0;
                    up(sem_id, 1, 1);
                    printf("%d read by everyone\n", read);
                }
                up(sem_id, 0, N);
            }
            
        }
        usleep(10);
        change = rand() % 10;
        usleep(20);
        if(change > 7 && user_addr[id].role == 1)
        {
            down(sem_id, 2, 1);
            user_addr[id].role = 0;
            shr_vars_addr[1]--;
            up(sem_id, 2, 1);
            printf("Library user %d relaxed, and is a reader now.\n", id);
        }
        else if(change > 7 && user_addr[id].role == 0)
        {
            down(sem_id, 2, 1);
            user_addr[id].role = 1;
            shr_vars_addr[1]++;
            up(sem_id, 2, 1);
            printf("Library user %d relaxed, and is a writer now.\n", id);
        }
        else
            printf("Library user %d relaxed.\n", id);
        usleep(5);
    }

    return 0;
}