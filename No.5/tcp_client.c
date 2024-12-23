#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define PORT 3490 /* 서버 포트 */
#define MAXDATASIZE 100
#define addr "127.0.0.1"
int main(int argc, char *argv[])
{
    int csock, numbytes;
    char buf[MAXDATASIZE];
    struct sockaddr_in serv_addr;
    int len;
  
    /* 클라이언트 소켓 생성 */
    if ((csock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    /* 서버 주소 설정 */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addr);
    serv_addr.sin_port = htons(PORT);
    /* 서버 연결 */
    if (connect(csock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }
    /* 메시지 송수신 */
    scanf("%s",buf);
    if ((numbytes = send(csock, buf, MAXDATASIZE, 0)) == -1)
    {
        perror("send");
        exit(1);
    }
    if ((numbytes = recv(csock, buf, MAXDATASIZE, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("Received: %s\n", buf);
    close(csock);
}
