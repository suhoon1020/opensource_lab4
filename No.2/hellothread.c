#include <pthread.h>
void *hello_thread(void *arg)
{
    printf("Thread: Hello, World %d!\n",arg);
    return arg;
}

int main()
{
    pthread_t tid;
    int status;
    int number=42;

    /* 쓰레드 생성 */
    status = pthread_create(&tid, NULL, hello_thread, (void*)number);

    if (status != 0) // 호출 성공공
        perror("Create thread");

    pthread_exit(NULL); // 스레드 종료
}