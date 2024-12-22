#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 100
#define NUM_CLIENTS 5

typedef struct {
    char message[BUFFER_SIZE]; // 메시지를 저장할 버퍼
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int new_message; // 새로운 메시지가 있는지 여부
    int clients_ready; // 메시지를 처리한 클라이언트 수
} shared_data_t;

shared_data_t shared_data = { .new_message = 0, 
                               .clients_ready = 0, 
                               .mutex = PTHREAD_MUTEX_INITIALIZER, 
                               .cond = PTHREAD_COND_INITIALIZER };

void *client_thread(void *arg) {
    int client_id = (int)(long)arg; // 클라이언트 ID
    while (1) {
        pthread_mutex_lock(&shared_data.mutex);
        while (!shared_data.new_message) {
            pthread_cond_wait(&shared_data.cond, &shared_data.mutex);
        }

        // 클라이언트 ID와 함께 메시지 출력
        printf("클라이언트 %d: %s\n", client_id + 1, shared_data.message);
        
        shared_data.clients_ready++; // 메시지를 처리한 클라이언트 수 증가

        // 모든 클라이언트가 메시지를 처리했는지 확인
        if (shared_data.clients_ready == NUM_CLIENTS) {
            shared_data.new_message = 0; // 모든 클라이언트가 처리했으므로 플래그 초기화
            shared_data.clients_ready = 0; // 클라이언트 수 초기화
            pthread_cond_broadcast(&shared_data.cond); // 서버에게 신호
        }

        pthread_mutex_unlock(&shared_data.mutex);
    }
    return NULL;
}

void *server_thread(void *arg) {
    char input[BUFFER_SIZE];
    while (1) {
        printf("메시지를 입력하세요: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0; // 개행 문자 제거

        pthread_mutex_lock(&shared_data.mutex);
        strcpy(shared_data.message, input);
        shared_data.new_message = 1; // 새로운 메시지가 있음을 표시
        shared_data.clients_ready = 0; // 클라이언트 수 초기화
        pthread_cond_broadcast(&shared_data.cond); // 모든 클라이언트에게 신호
        pthread_mutex_unlock(&shared_data.mutex);
    }
    return NULL;
}

int main() {
    pthread_t clients[NUM_CLIENTS];
    pthread_t server_tid;

    // 클라이언트 쓰레드 생성
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_create(&clients[i], NULL, client_thread, (void*)(long)i);
    }

    // 서버 쓰레드 생성
    pthread_create(&server_tid, NULL, server_thread, NULL);

    // 서버 쓰레드가 종료될 때까지 대기
    pthread_join(server_tid, NULL);

    // 클라이언트 쓰레드 종료 대기
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_cancel(clients[i]); // 클라이언트 쓰레드 종료
    }

    return 0;
}
