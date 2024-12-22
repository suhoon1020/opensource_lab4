/* boundedbuffer.c */
/* bounded buffer example */
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h> // sleep 함수 사용을 위한 헤더
#define BUFFER_SIZE 20 // 버퍼의 크기
#define NUMITEMS 30    // 생산할 아이템의 수

// 버퍼 구조체 정의
typedef struct
{
    int item[BUFFER_SIZE]; // 버퍼를 저장할 배열
    int totalitems;        // 현재 버퍼에 저장된 아이템 수
    int in, out;           // 아이템을 추가할 인덱스와 제거할 인덱스
    pthread_mutex_t mutex; // 뮤텍스 객체
    pthread_cond_t full;   // 버퍼가 가득 찼을 때 대기하는 소비자 스레드에게 신호를 보내기 위한 조건 변수
    pthread_cond_t empty;  // 버퍼가 비었을 때 대기하는 생산자 스레드에게 신호를 보내기 위한 조건 변수
} buffer_t;

// 전역 버퍼 객체 초기화
buffer_t bb = {{0}, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};

// 아이템을 생성하는 함수
int produce_item()
{
    // 0에서 100 사이의 랜덤한 아이템 생성
    int item = (int)(100.0 * rand() / (RAND_MAX + 1.0));
    // 랜덤한 시간 동안 대기
    sleep((unsigned long)(5.0 * rand() / (RAND_MAX + 1.0)));
    printf("produce_item: item=%d\n", item); // 생성된 아이템 출력
    return item; // 생성된 아이템 반환
}

// 버퍼에 아이템을 추가하는 함수
int send_msg(int item)
{
    int status;
    // 뮤텍스 잠금
    status = pthread_mutex_lock(&bb.mutex);
    if (status != 0) // 잠금 실패 시
        return status;

    // 버퍼가 가득 차면 대기
    while (bb.totalitems >= BUFFER_SIZE) 
        pthread_cond_wait(&bb.empty, &bb.mutex);  // 조건변수에서 대기

    // 아이템을 버퍼에 추가
    bb.item[bb.in] = item;
    bb.in = (bb.in + 1) % BUFFER_SIZE; // 인덱스 순환
    bb.totalitems++; // 아이템 수 증가

    // 소비자에게 아이템 추가 알림
    pthread_cond_signal(&bb.full);
    return pthread_mutex_unlock(&bb.mutex); // 뮤텍스 잠금 해제
}

// 소비할 아이템을 처리하는 함수
void consume_item(int item)
{
    // 랜덤한 시간 동안 대기
    sleep((unsigned long)(5.0 * rand() / (RAND_MAX + 1.0)));
    printf("\t\tconsume_item: item=%d\n", item); // 소비된 아이템 출력
}

// 버퍼에서 아이템을 제거하는 함수
int remove_item(int *temp)
{
    int status;
    // 뮤텍스 잠금
    status = pthread_mutex_lock(&bb.mutex);
    if (status != 0)
        return status;

    // 버퍼가 비어 있으면 대기
    while (bb.totalitems <= 0) 
        pthread_cond_wait(&bb.full, &bb.mutex); // 조건변수에서 대기

    // 아이템을 버퍼에서 제거
    *temp = bb.item[bb.out];
    bb.out = (bb.out + 1) % BUFFER_SIZE; // 인덱스 순환
    bb.totalitems--; // 아이템 수 감소

    // 생산자에게 공간이 생겼다고 알림
    pthread_cond_signal(&bb.empty);
    return pthread_mutex_unlock(&bb.mutex); // 뮤텍스 잠금 해제
}

// 생산자 스레드 함수
void *producer(void *arg)
{
    int item;
    while (1)
    {
        item = produce_item(); // 아이템 생성
        send_msg(item);     // 생성된 아이템을 버퍼에 추가
    }
}

// 소비자 스레드 함수
void *consumer(void *arg)
{
    int item;
    while (1)
    {
        remove_item(&item);    // 버퍼에서 아이템 제거
        consume_item(item);    // 제거된 아이템 소비
    }
}

// 메인 함수
int main()
{
    int status;
    pthread_t producer_tid, consumer_tid; // 스레드 ID 선언

    // 생산자 스레드 생성
    status = pthread_create(&producer_tid, NULL, producer, NULL);
    if (status != 0)
        perror("Create producer thread");

    // 소비자 스레드 생성
    status = pthread_create(&consumer_tid, NULL, consumer, NULL);
    if (status != 0)
        perror("Create consumer thread");

    // 생산자 스레드 종료 대기
    status = pthread_join(producer_tid, NULL);
    if (status != 0)
        perror("Join producer thread");

    // 소비자 스레드 종료 대기
    status = pthread_join(consumer_tid, NULL);
    if (status != 0)
        perror("Join consumer thread");

    return 0; // 프로그램 종료
}
