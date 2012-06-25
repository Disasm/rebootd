#include "key.h"
#include "rng.h"
#include "daemon.h"
#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/klog.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

void sig_handler(int signum)
{
    daemon_terminate();
    exit(0);
}

int sock;

in_addr_t resolve(const char* host)
{
    struct in_addr ia;
    int res = inet_aton(host, &ia);
    if(res) return ia.s_addr;
    
    struct hostent* h = gethostbyname(host);
    if(!h)
    {
        fprintf(stderr, "can't resolve host\n");
        return 0;
    }
    
    return *((in_addr_t*)h->h_addr_list[0]);
}

void send2(int h, const char* str)
{
    send(h, str, strlen(str), 0);
}

void command_dmesg(int h, const char* cmd)
{
    int bufsize = 1024;
    
    char* cmd2 = (char*)malloc(strlen(cmd)+1);
    memcpy(cmd2, cmd, strlen(cmd)+1);
    char* tok = strtok(cmd2, " ");
    tok = strtok(NULL, " ");
    if(tok)
    {
        bufsize = atoi(tok);
    }
    free(cmd2);
    
    char* buf = (char *) malloc(bufsize);
    int n = klogctl(3, buf, bufsize);
    if(n < 0)
    {
        send2(h, "klogctl error\n");
        return;
    }
    send(h, buf, n, 0);
    free(buf);
}

void process_command(int h, const char* cmd)
{
    if(strcmp(cmd, "reboot")==0)
    {
        send2(h, "restarting...\n");
        reboot(LINUX_REBOOT_CMD_RESTART);
        return;
    }
    if(strncmp(cmd, "dmesg", 5)==0)
    {
        command_dmesg(h, cmd);
        return;
    }
    if(strcmp(cmd, "sync")==0)
    {
        send2(h, "syncing...\n");
        sync();
        return;
    }
    if(strcmp(cmd, "help")==0)
    {
        send2(h, "Commands: reboot, dmesg, sync, help\n");
        return;
    }
    if(strlen(cmd)!=0)
    {
        send2(h, "Unknown command: ");
        send(h, cmd, strlen(cmd), 0);
        send2(h, "\n");
    }
}

void process_connection(int h, uint8_t* nonce, size_t nonceSize)
{
    rngInit(nonce, nonceSize);
    
    uint8_t challenge[CHALLENGE_SIZE];
    rngGet(challenge, sizeof(challenge));
    
    uint8_t digest[20];
    getDigest(digest, challenge, sizeof(challenge));
    
    send(h, challenge, sizeof(challenge), 0);
    
    uint8_t digest2[20];
    int len = recv(h, digest2, sizeof(digest2), 0);
    if(len != 20) return;
    
    if(memcmp(digest2, digest, 20)!=0) return;
    
    char cmd[100];
    while(true)
    {
        send(h, "# ", 2, 0);
        
        len = recv(h, cmd, sizeof(cmd)-1, 0);
        if(len<=0) break;
        cmd[len] = 0;
        
        int i = len - 1;
        while(i>=0)
        {
            if((cmd[i]=='\n')||(cmd[i]=='\r')||(cmd[i]==' ')||(cmd[i]=='\t')) cmd[i] = 0;
            else break;
            i--;
        }
        
        process_command(h, cmd);
    }
}

int run_server()
{
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        fprintf(stderr, "can't create socket\n");
        return 1;
    }
    
    int opt = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(res == -1)
    {
        fprintf(stderr, "can't set socket option SO_REUSEADDR\n");
        return 1;
    }
    
    struct sockaddr_in sa;
    
    //sa.sin_len = sizeof(sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(SERVER_PORT);
    sa.sin_addr.s_addr = INADDR_ANY;
    
    res = bind(sock, (const struct sockaddr*)&sa, (socklen_t)sizeof(sa));
    if(res == -1)
    {
        fprintf(stderr, "can't bind socket\n");
        return 1;
    }
    
    res = listen(sock, 5);
    if(res == -1)
    {
        fprintf(stderr, "can't listen socket\n");
        return 1;
    }
    
    printf("server started at port %u\n", SERVER_PORT);
    
    while(true)
    {
        struct sockaddr_in sa;
        memset(&sa,0,sizeof(sa));
        
        int len = sizeof(sa);
        int h = accept(sock, (struct sockaddr*)&sa, (socklen_t *)&len);
        if(h==-1)
        {
            break;
        }
        
        uint8_t nonce[128];
        rngGet(nonce, sizeof(nonce));
        
        pid_t pid = fork();
        if(pid==-1) break;
        
        if(pid==0) // child process
        {
            process_connection(h, nonce, sizeof(nonce));
            shutdown(h, SHUT_RDWR);
            close(h);
            exit(0);
        }
    }
    
    return 0;
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "Usage: %s <keyfile>\n", argv[0]);
        return 1;
    }

    int res = readKey(argv[1]);
    if(res) return 1;

    res = rngInitUrandom(100);
    if(res)
    {
        fprintf(stderr, "Can't init RNG\n");
        return 1;
    }
    
    // daemon_start();
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    run_server();
    
    // daemon_terminate();
    
    return 0;
}
