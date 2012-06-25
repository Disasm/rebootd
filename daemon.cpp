#include "daemon.h"
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PID_FILE "/var/run/rebootd.pid"

static bool isDaemon = false;

static void child_handler(int signum)
{
    switch(signum) {
        case SIGALRM: exit(1); break;
        case SIGUSR1: exit(0); break;
        case SIGCHLD: exit(1); break;
    }
}

// code from http://www-theorie.physik.unizh.ch/~dpotter/howto/daemonize
void daemon_start()
{
    // already a daemon
    if(getppid() == 1) return;
    
    // Trap signals that we expect to recieve
    signal(SIGCHLD,child_handler);
    signal(SIGUSR1,child_handler);
    signal(SIGALRM,child_handler);
    
    // Fork off the parent process
    pid_t pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "unable to fork daemon, code=%d (%s)\n", errno, strerror(errno));
        exit(1);
    }
    // If we got a good PID, then we can exit the parent process.
    if (pid > 0)
    {
        /* Wait for confirmation from the child via SIGTERM or SIGCHLD, or
        for two seconds to elapse (SIGALRM).  pause() should not return. */
        alarm(2);
        pause();
        
        exit(1);
    }
    
    // Create the pid file as the current user.
    int h = open(PID_FILE, O_RDWR|O_CREAT, 0640);
    if(h<0)
    {
        fprintf(stderr, "unable to create pid file %s, code=%d (%s)\n", PID_FILE, errno, strerror(errno));
        exit(1);
    }
    char buf[10];
    sprintf(buf, "%d", getpid());
    write(h, buf, strlen(buf));
    close(h);
    
    // At this point we are executing as the child process
    pid_t parent = getppid();
    
    // Cancel certain signals
    signal(SIGCHLD,SIG_DFL); /* A child process dies */
    signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
    signal(SIGTERM,SIG_DFL); /* Die on SIGTERM */
    
    /* Change the file mode mask */
    umask(0);
    
    /* Create a new SID for the child process */
    pid_t sid = setsid();
    if (sid < 0)
    {
        fprintf(stderr, "unable to create a new session, code %d (%s)\n", errno, strerror(errno));
        exit(1);
    }
    
    /* Change the current working directory.  This prevents the current
    directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0)
    {
        fprintf(stderr, "unable to change directory to %s, code %d (%s)\n", "/", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
    
    /* Tell the parent process that we are A-okay */
    kill(parent, SIGUSR1);
    
    isDaemon = true;
}


void daemon_kill()
{
    FILE* f = fopen(PID_FILE, "r");
    if(!f)
    {
        fprintf(stderr, "Can't found PID file, may be program is not running\n");
        return;
    }
    char buf[20];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    pid_t pid = atoi(buf);
    if(!pid)
    {
        fprintf(stderr, "Bad pid file\n");
        return;
    }
    fprintf(stderr, "killing pid %d...\n", pid);
    int res = kill(pid, SIGINT);
    if(res<0) fprintf(stderr, "unable to kill pid %d, code=%d (%s)\n", pid, errno, strerror(errno));
}

void daemon_terminate()
{
    if(isDaemon)
    {
        unlink(PID_FILE);
    }
}
