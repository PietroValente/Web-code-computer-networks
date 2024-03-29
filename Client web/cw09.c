#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 60000


int main()
{
    struct sockaddr_in addr; // address
    int rl = 0; // total lenght of the message
    char request[BUF_SIZE], response[BUF_SIZE];
    int s = socket(AF_INET, SOCK_STREAM, 0); // socket
    char *rb = NULL; // response body
    int rbl; // response body length
    char * sl; // status line

    if (s == -1) {
        perror("Socket fail");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("142.250.186.132"); // Google server

    if (connect(s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
        perror("Connect fail");
        return 1;
    }

    sprintf(request, "GET / \r\n");

    //STAMPARE REQUEST
    printf("********************\n");
    printf("REQUEST: \n%s\n",request);
    printf("********************\n");

    if (write(s, request, strlen(request)) == -1) {
        perror("Write fail");
        return 1;
    }

    // read the response

    for(int j = 0; (j = read(s, response + rl, BUF_SIZE - rl - 1)) > 0; rl += j);

    response[rl] = 0;
    
    printf("********************\n");
    printf("RESPONSE: \n%s\n",response);
    printf("********************\n");


    printf("\nRead %d chars", rl);
}