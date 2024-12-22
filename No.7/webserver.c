#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define PORT 3490 /* default port */
#define QLEN 10   /* request queue size */
#define BUF_SIZE 1024
void *handle_clnt(int sockfd);
void send_err(int sockfd);
void send_msg(int sockfd);

int main(int argc, char *argv[])
{
    int sockfd, new_fd;             /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in server_addr; /* structure to
    hold server's address */
    struct sockaddr_in client_addr; /* structure to hold
    client's address */
    int alen;                       /* length of address */
    fd_set readfds, activefds;      /* the set of read
         descriptors */
    int i, maxfd = 0, numbytes;
    char buf[100];
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(1);
    }
    /* Specify a size of request queue */
    if (listen(sockfd, QLEN) < 0)
    {
        fprintf(stderr, "listen failed\n");
        exit(1);
    }
    alen = sizeof(client_addr);
    /* Initialize the set of active sockets. */
    FD_ZERO(&activefds);
    FD_SET(sockfd, &activefds);
    maxfd = sockfd;
    /* Main server loop - accept and handle requests */
    fprintf(stderr, "Server up and running.\n");
    while (1)
    {
        printf("SERVER: Waiting for contact..., %d\n", maxfd);
        /* Block until input arrives on one or more active sockets. */
        readfds = activefds;
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
        /* Service all the sockets with input pending. */
        for (i = 0; i <= maxfd; i++)
        {
            if (FD_ISSET(i, &readfds))
            {
                if (i == sockfd)
                {
                    if ((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &alen)) < 0)
                    {
                        fprintf(stderr, "accept failed\n");
                        exit(1);
                    }
                    FD_SET(new_fd, &activefds); // add the new socket desc to our active connections set if (new_fd > maxfd)
                    maxfd = new_fd;
                }
                else
                {
                    printf("handle clnt\n");
                    handle_clnt(i);
                    close(i);
                    FD_CLR(i, &activefds);
                }
            }
        }
    }
    close(sockfd);
}

void *handle_clnt(int client_sock) {
    int str_len = 0;
    char msg[BUF_SIZE];
    char method[10];
    char uri[100];
    char *cgi_script = NULL;

    // 클라이언트로부터 요청 읽기
    if ((str_len = read(client_sock, msg, BUF_SIZE)) == -1) {
        perror("read() error");
        exit(1);
    }

    // 요청 메시지 파싱
    sscanf(msg, "%s %s", method, uri);

    if (strcmp(method, "GET") != 0) {
        send_err(client_sock);
    } else {
        // CGI 요청인지 확인
        if (strncmp(uri, "/CGI/", 5) == 0) { // 수정: 5로 변경
            // CGI 스크립트 경로 설정
            cgi_script = uri + 1; // '/'를 제거
            // CGI 스크립트 실행
            FILE *fp = popen(cgi_script, "r");
            if (fp == NULL) {
                send_err(client_sock);
                return NULL;
            }

            // CGI 스크립트의 출력을 클라이언트에게 전송
            char buffer[BUF_SIZE];
            while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                write(client_sock, buffer, strlen(buffer));
            }
            pclose(fp);
        } else {
            send_msg(client_sock); // 일반 메시지 전송
        }
    }
    return NULL;
}



void send_err(int client_sock) // send to all
{
    char protocol[] = "HTTP/1.1 400 Bad Request\r\n ";
    char server[] = "Server:NetscapeEnterprise/6.0\r\n";
    char contenttype[] = "ContentType:text/html\r\n";
    char html[] = "<html><head> BADConnection </head> <body><H1> Bad Request</H1></body></html>\r\n ";
    char end[] = "\r\n";
    printf("send err\n");
    write(client_sock, protocol, strlen(protocol));
    write(client_sock, server, strlen(server));
    write(client_sock, contenttype, strlen(contenttype));
    write(client_sock, end, strlen(end));
    write(client_sock, html, strlen(html));
    fflush(fdopen(client_sock, "w"));
}

void send_msg(int client_sock) // send to all
{
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    char contentlength[] = "Content-Length: 72\r\n";
    char contenttype[] = "Content-Type:text/html\r\n";
    char html[] = "<html><head>Hello World</head> <body><H1>Hello World</H1></body></html>\r\n ";
    char end[] = "\r\n";
    printf("send msg, len=%d\n", strlen(html));
    write(client_sock, protocol, strlen(protocol));
    write(client_sock, server, strlen(server));
    write(client_sock, contentlength, strlen(contentlength));
    write(client_sock, contenttype, strlen(contenttype));
    write(client_sock, end, strlen(end));
    write(client_sock, html, strlen(html));
    fflush(fdopen(client_sock, "w"));
}