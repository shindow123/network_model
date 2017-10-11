#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "common.h"

void sig_interrupt(int);
pid_t child_make(int, int, int);
void child_main(int, int, int);

int nchildren;
static pid_t* pids;

int main(int argc, char* argv[])
{
	int listenfd, connfd;
	struct sockaddr_in servaddr, cliaddr;

	if(argc != 3)
	{
		printf("./server <port> <childnum>\n");
		exit(0);
	}

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));
	if(bind(listenfd, (const struct sockaddr*)&servaddr, (socklen_t)sizeof(servaddr)) < 0)
	{
		close(listenfd);
		printf("bind fail\n");
		exit(1);
	}

	if(listen(listenfd, 1000) < 0)
	{
		close(listenfd);
		printf("listen on %d fail\n", listenfd);
		exit(1);
	}

	signal(SIGINT, sig_interrupt);
	nchildren = atoi(argv[2]);
	pids = (pid_t*)calloc(nchildren, sizeof(pid_t));
	assert(pids != NULL);
	for(int i = 0; i < nchildren; ++i)
	{
		pids[i] = child_make(i, listenfd, sizeof(struct sockaddr));
	}

	while(true)
	{
		pause();
	}

	return 0;
}

void sig_interrupt(int signo)
{
	for(int i = 0; i < nchildren; ++i)
	{
		kill(pids[i], SIGTERM);
	}
	while(wait(NULL) > 0)
		;
	if(errno != ECHILD)
	{
		printf("wait error, errno [%d] [%s]\n", errno, strerror(errno));
		exit(1);
	}
	pr_cpu_time();
	exit(0);
}

pid_t child_make(int i, int listenfd, int addrlen)
{
	pid_t pid = fork();
	if(pid > 0) // 父进程
		return pid;
	child_main(i, listenfd, addrlen);
	exit(0);
}

void child_main(int i, int listenfd, int addrlen)
{
	socklen_t clilen;
	struct sockaddr cliaddr;
	printf("[%03d] child %ld starting...\n", i, (long)getpid());
	while(true)
	{
		clilen = addrlen;
		int connfd = accept(listenfd, &cliaddr, &clilen);
		if(connfd < 0)
		{
			printf("accept fail! errno [%d] [%s]\n", errno, strerror(errno));
			exit(1);
		}
		web_child(connfd);
		close(connfd);
	}
}
