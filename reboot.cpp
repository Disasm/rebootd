#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "config.h"
#include "key.h"

int sock;

int loop()
{
    char buf[1024];
    
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(sock, &fd);
    FD_SET(0, &fd);
    
    struct timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;
    int res = select(sock+1, &fd, NULL, NULL, &tm);
    if(res<0)
    {
        return -1;
    }
    if(res>0)
    {
        if(FD_ISSET(sock, &fd))
        {
            res = recv(sock, buf, sizeof(buf)-1, 0);
            if(res<=0) return -1;
            buf[res] = 0;
            fwrite(buf, 1, res, stdout);
            fflush(stdout);
        }
        if(FD_ISSET(0, &fd))
        {
            if(!fgets(buf, sizeof(buf), stdin)) return -1;
            send(sock, buf, strlen(buf), 0);
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <keyfile> <server_ip>\n", argv[0]);
        return 1;
    }
    
    int res = readKey(argv[1]);
    if(res) return 2;

    char* ip = argv[2];

    sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(SERVER_PORT);
    sa.sin_addr.s_addr = inet_addr(ip);

    if(connect(sock, (struct sockaddr*)&sa, sizeof(sa))==-1)
    {
        perror("connect");
        return 3;
    }
    
    uint8_t challenge[CHALLENGE_SIZE];
    int len = recv(sock, challenge, sizeof(challenge), 0);
    if(len<=0)
    {
        perror("recv");
        return 4;
    }
    if(len!=sizeof(challenge))
    {
        fprintf(stderr, "short challenge\n");
        return 5;
    }
    
    uint8_t digest[20];
    getDigest(digest, challenge, sizeof(challenge));
    
    send(sock, digest, sizeof(digest), 0);
    
    while(!feof(stdin))
    {
        if(loop()<0) break;
    }
    
    return 0;
}
