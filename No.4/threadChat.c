#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAXBUFSIZE 127
#define NUM_THREAD 5

typedef struct data{
        char buff[MAXBUFSIZE];
        int is_ready;
        int count;
        int processed[NUM_THREAD];
        pthread_mutex_t buff_mutex;
        pthread_cond_t ready_to_read;
        pthread_cond_t ready_to_write;
} MESSAGE;

MESSAGE msg ={{0}, 0,0,{0}, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

void *recv_message(void *arg){
        int thread_id = (int)arg;
        // client 쓰레드
        // 1. 뮤텍스를 획득
        // 2. 조건변수 확인
        // 3. 메시지를 읽음 == 출력
        // 4. 뮤텍스 해제
        while(1){
                pthread_mutex_lock(&msg.buff_mutex);
                while(msg.is_ready == 0 || msg.processed[thread_id]){
                        pthread_cond_wait(&msg.ready_to_read, &msg.buff_mutex);
                }

                printf("%d : %s\n",thread_id, msg.buff);
                msg.processed[thread_id] = 1;
                msg.count++;
                if (msg.count == NUM_THREAD) {
                        msg.is_ready = 0;
                        msg.count = 0;
                        memset(msg.processed, 0, sizeof(msg.processed));
                        pthread_cond_signal(&msg.ready_to_write);
                }
                pthread_mutex_unlock(&msg.buff_mutex);
        }

        return NULL;
}

void *send_message(void *arg){
        while(1){
                // main 쓰레드
                // 1. 뮤텍스를 획득
                // 2. 조건변수 확인
                // 3. 메시지를 작성 => broadcast로 신호줘야겠다.
                // 4. 뮤텍스 해제

                pthread_mutex_lock(&msg.buff_mutex);

                while(msg.is_ready == 1){
                        pthread_cond_wait(&msg.ready_to_write, &msg.buff_mutex);
                }

                printf("server : ");
                scanf("%s", msg.buff);

                msg.is_ready = 1;
                pthread_cond_broadcast(&msg.ready_to_read);

                pthread_mutex_unlock(&msg.buff_mutex);
        }
        return NULL;
}

int main(void){
        int i;

        pthread_t clients[NUM_THREAD];
        pthread_t server;

        pthread_create(&server, NULL, send_message, NULL);

        for(i = 0; i < NUM_THREAD; i++){
                pthread_create(&clients[i], NULL, recv_message, (void *)i);
        }

        for(i = 0; i < NUM_THREAD; i++){
                pthread_join(clients[i], NULL);
        }

        pthread_join(server, NULL);

        return 0;
}
