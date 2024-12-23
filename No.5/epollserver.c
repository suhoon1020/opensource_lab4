#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define PORT 3490        /* 서버가 사용하는 포트 번호 */
#define QLEN 6           /* 대기 연결 요청 큐 크기 */
#define EPOLL_SIZE 128   /* epoll 이벤트 배열 크기 */
#define BUF_SIZE 128     /* 메시지 버퍼 크기 */
#define MAX_CLNT 4       /* 최대 클라이언트 수 */

void *handle_clnt(int epfd, int sockfd); /* 클라이언트 처리 함수 */
void send_msg(char *msg, int len);       /* 모든 클라이언트에게 메시지 전송 */

int client_count = 0;             /* 현재 접속한 클라이언트 수 */
int client_socks[MAX_CLNT];       /* 클라이언트 소켓 저장 배열 */

int main(int argc, char *argv[])
{
    int i;
    int server_sock, client_sock;                  /* 서버 소켓, 클라이언트 소켓 */
    struct sockaddr_in server_addr, client_addr;   /* 서버, 클라이언트 주소 구조체 */
    int alen;                                      /* 주소 길이 */
    struct epoll_event event;                      /* epoll 이벤트 구조체 */
    struct epoll_event *ep_events;                /* epoll 이벤트 배열 */
    int epfd;                                      /* epoll 파일 디스크립터 */
    int event_cnt = 0;                             /* 발생한 이벤트 수 */
    int option = 1;                                /* 소켓 옵션 값 */
    socklen_t optlen;

    /* 서버 소켓 생성 */
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        exit(1);
    }

    /* 서버 주소 설정 */
    memset(&server_addr, 0, sizeof(server_addr));  /* 주소 초기화 */
    server_addr.sin_family = AF_INET;             /* IPv4 사용 */
    server_addr.sin_port = htons(PORT);           /* 포트 설정 */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* 모든 인터페이스에서 접속 허용 */

    /* 서버 소켓과 주소 바인딩 */
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(1);
    }

    /* 클라이언트 연결 요청 대기 큐 설정 */
    if (listen(server_sock, QLEN) < 0)
    {
        fprintf(stderr, "listen failed\n");
        exit(1);
    }

    alen = sizeof(client_addr);

    /* epoll 생성 */
    epfd = epoll_create(EPOLL_SIZE);
    ep_events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    /* 서버 소켓을 epoll에 등록 */
    event.events = EPOLLIN;   /* 읽기 이벤트 감지 */
    event.data.fd = server_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &event);

    fprintf(stderr, "Server up and running.\n");

    /* 서버의 메인 루프 */
    while (1)
    {
        /* 이벤트 대기 */
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if (event_cnt == -1)
            fprintf(stderr, "epoll_wait() error!\n");

        for (i = 0; i < event_cnt; i++)
        {
            /* 새로운 클라이언트 연결 처리 */
            if (ep_events[i].data.fd == server_sock)
            {
                if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &alen)) < 0)
                {
                    fprintf(stderr, "accept failed\n");
                    exit(1);
                }
                /* 새로운 클라이언트 소켓을 배열에 추가 */
                client_socks[client_count++] = client_sock;

                /* 클라이언트 소켓 옵션 설정 (주소 재사용) */
                option = 1;
                optlen = sizeof(option);
                setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

                /* 클라이언트 소켓을 epoll에 등록 */
                event.events = EPOLLIN;
                event.data.fd = client_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
                printf("connected client: %d\n", client_sock);
            }
            else
            {
                /* 기존 클라이언트의 데이터 처리 */
                handle_clnt(epfd, ep_events[i].data.fd);
            }
        }
    }

    /* 자원 해제 및 종료 */
    close(server_sock);
    close(epfd);
    return 0;
}

/* 클라이언트 요청 처리 */
void *handle_clnt(int epoll_fd, int client_sock)
{
    int i;
    int recv = 0, str_len = 0; /* 수신한 바이트 수 */
    char msg[BUF_SIZE];        /* 메시지 버퍼 */

    /* 클라이언트로부터 데이터 읽기 */
    if ((str_len = read(client_sock, &msg[recv], BUF_SIZE)) == -1)
    {
        printf("read() error!\n");
        exit(1);
    }
    recv += str_len;

    /* 수신한 메시지를 모든 클라이언트에게 전송 */
    send_msg(msg, recv);

    /* 메시지 버퍼 초기화 */
    memset(msg, 0, BUF_SIZE);

    /* 클라이언트 연결 종료 처리 */
    close(client_sock);
    for (i = 0; i < client_count; i++)
    {
        if (client_sock == client_socks[i])
        {
            /* 클라이언트 소켓 배열에서 제거 */
            while (i++ < client_count - 1)
            {
                client_socks[i] = client_socks[i + 1];
            }
            break;
        }
    }
    client_count--;

    /* epoll에서 클라이언트 소켓 제거 */
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_sock, NULL);
    return NULL;
}

/* 메시지를 모든 클라이언트에게 전송 */
void send_msg(char *msg, int len)
{
    int i;
    for (i = 0; i < client_count; i++)
    {
        write(client_socks[i], msg, BUF_SIZE); /* 각 클라이언트 소켓에 메시지 전송 */
    }
}
