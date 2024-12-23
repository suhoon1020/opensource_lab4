#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 3490
#define QLEN 10
#define BUF_SIZE 1024

void *handle_clnt(int sockfd);
void send_err(int sockfd);
void send_msg(int sockfd);
void handle_post(int client_sock, char *msg);

int main(int argc, char *argv[])
{
    int sockfd, new_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int alen;
    fd_set readfds, activefds;
    int i, maxfd = 0, numbytes;
    char buf[100];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("sock");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, QLEN) < 0)
    {
        fprintf(stderr, "listen failed\n");
        exit(1);
    }

    alen = sizeof(client_addr);
    FD_ZERO(&activefds);
    FD_SET(sockfd, &activefds);
    maxfd = sockfd;

    fprintf(stderr, "Server up and running.\n");

    while (1)
    {
        printf("SERVER : Waiting for contact..., %d\n", maxfd);

        readfds = activefds;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i <= maxfd; i++)
        {
            if (FD_ISSET(i, &readfds))
            {
                if (i == sockfd)
                {
                    new_fd = accept(sockfd, (struct sockaddr *)&server_addr, &alen);
                    if (new_fd < 0)
                    {
                        fprintf(stderr, "accept failed\n");
                        exit(1);
                    }
                    FD_SET(new_fd, &activefds);
                    if (new_fd > maxfd)
                        maxfd = new_fd;
                }
                else
                {
                    printf("handle clnt \n");
                    handle_clnt(i);
                    close(i);
                    FD_CLR(i, &activefds);
                }
            }
        }
    }
    close(sockfd);
}
void *handle_clnt(int client_sock)
{
    char msg[BUF_SIZE] = {0};
    char method[10] = {0};
    char path[BUF_SIZE] = {0};

    // 요청 읽기
    if (recv(client_sock, msg, BUF_SIZE, 0) <= 0)
    {
        printf("read() error!\n");
        return NULL;
    }

    // HTTP 메소드와 경로 파싱
    sscanf(msg, "%s %s", method, path);

    // CGI 경로 확인 및 실행
    if (strstr(path, "/cgi-bin/") == path)
    {
        char cgi_path[BUF_SIZE];
        snprintf(cgi_path, sizeof(cgi_path), ".%s", path); // CGI 프로그램 경로 설정
        handle_cgi(client_sock, cgi_path);
    }
    else if (strcmp(method, "POST") == 0)
    {
        handle_post(client_sock, msg);
    }
    else if (strcmp(method, "GET") == 0)
    {
        send_msg(client_sock);
    }
    else
    {
        send_err(client_sock);
    }

    return NULL;
}

void handle_post(int client_sock, char *msg)
{
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    char contenttype[] = "Content-Type:text/html\r\n";
    char end[] = "\r\n";
    char response_html[BUF_SIZE];

    // POST 데이터 파싱
    char *body = strstr(msg, "\r\n\r\n");
    if (body)
    {
        body += 4;
        printf("POST body: %s\n", body); // 디버깅용

        snprintf(response_html, BUF_SIZE,
                 "<html><head><title>POST Response</title></head>"
                 "<body><h1>Received POST Data</h1>"
                 "<p>Data: %s</p></body></html>\r\n",
                 body);
    }
    else
    {
        snprintf(response_html, BUF_SIZE,
                 "<html><head><title>POST Error</title></head>"
                 "<body><h1>Error Processing POST Data</h1></body></html>\r\n");
    }

    char content_length_header[64];
    snprintf(content_length_header, sizeof(content_length_header),
             "Content-Length: %lu\r\n", strlen(response_html));

    write(client_sock, protocol, strlen(protocol));
    write(client_sock, server, strlen(server));
    write(client_sock, content_length_header, strlen(content_length_header));
    write(client_sock, contenttype, strlen(contenttype));
    write(client_sock, end, strlen(end));
    write(client_sock, response_html, strlen(response_html));

    fflush(fdopen(client_sock, "w"));
}

void send_err(int client_sock) // send to all
{
    char protocol[] = "HTTP/1.1 400 Bad Request\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    char contenttype[] = "Content-Type:text/html\r\n";
    char html[] = "<html><head>BAD Connection</head><body><H1>Bad Request</H1></body></html>\r\n";
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
    char html[] = "<html><head>Hello World</head><body><H1>Hello World</H1></body></html>\r\n";
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

void handle_cgi(int client_sock, const char *path)
{
    int pipe_fd[2];
    pid_t pid;

    if (pipe(pipe_fd) == -1)
    {
        perror("pipe");
        send_err(client_sock);
        return;
    }

    if ((pid = fork()) < 0)
    {
        perror("fork");
        send_err(client_sock);
        return;
    }

    if (pid == 0)
    {                                    // Child process
        close(pipe_fd[0]);               // Close read end
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        dup2(pipe_fd[1], STDERR_FILENO); // Redirect stderr to pipe
        close(pipe_fd[1]);

        execl(path, path, NULL); // Execute CGI script
        perror("execl");         // execl failed
        exit(1);
    }
    else
    {                      // Parent process
        close(pipe_fd[1]); // Close write end

        char buffer[BUF_SIZE];
        ssize_t nread;

        // Read CGI output from pipe and send to client
        while ((nread = read(pipe_fd[0], buffer, BUF_SIZE)) > 0)
        {
            write(client_sock, buffer, nread);
        }

        close(pipe_fd[0]);
        waitpid(pid, NULL, 0); // Wait for child process to finish
    }
}
