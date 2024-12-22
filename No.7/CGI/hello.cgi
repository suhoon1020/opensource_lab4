#include <stdio.h>

int main(void) {
    // HTTP 헤더 출력
    printf("Content-Type: text/html\r\n\r\n");
    // HTML 콘텐츠 출력
    printf("<html><head><title>Hello CGI</title></head><body>");
    printf("<h1>Hello from CGI!</h1>");
    printf("</body></html>");
    return 0;
}
