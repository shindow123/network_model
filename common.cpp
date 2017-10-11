#include "common.h"

#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <signal.h>

ssize_t readn(int fd, void* vptr, size_t n)
{
	ssize_t nread;
	void* ptr = vptr;
	size_t nleft = n;
	while(nleft > 0)
	{
		if((nread = read(fd, ptr, nleft)) < 0)
		{
			if(errno == EINTR)
				nread = 0;
			else
				return -1;
		}
		else if(nread == 0)
		{
			break;
		}

		nleft -= nread;
		ptr += nread;
	}

	return (n-nleft);
}

ssize_t writen(int fd, const void* vptr, size_t n)
{
	size_t nleft = n;
	ssize_t nwritten;
	const void* ptr = vptr;
	while(nleft > 0)
	{
		if((nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if(nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

ssize_t readline(int fd, void* vptr, size_t maxlen)
{
	ssize_t n, rc;
	char* ptr = (char*)vptr;
	char c;
	for(n = 1; n < maxlen; n++)
	{
again:
		if((rc = read(fd, &c, 1)) == 1)
		{
			*ptr++ = c;
			if(c == '\n')
				break;
		}
		else if(rc == 0)
		{
			*ptr = 0;
			return (n-1);
		}
		else
		{
			if(errno == EINTR)
				goto again;
			return -1;
		}
	}
	*ptr = 0;
	return n;
}

void web_child(int sockfd)
{
	int nwrite;
	ssize_t nread;
	char line[MAXLINE];
	char result[MAXN];
	prepare_msg(result);

	while(true)
	{
		if((nread = read(sockfd, line, MAXLINE)) == 0)
		{
			return;
		}
		nwrite = atoi(line);
		if(nwrite < 0 || nwrite > MAXN)
		{
			printf("client request %d bytes data\n", nwrite);
			exit(1);
		}
		if(nwrite == 0)
		{
			shutdown(sockfd, SHUT_WR);
			return;
		}
		writen(sockfd, result, nwrite);
	}
}

void prepare_msg(char* msg)
{
	memset(msg, 'a', MAXN);
	msg[MAXN-1] = '\0';
}

void sig_int(int signo)
{
	pr_cpu_time();
	exit(0);
}

void sig_child(int signo)
{
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{
		printf("child %d terminated\n", pid);
	}
	return;
}

void pr_cpu_time()
{
	double user, sys;
	struct rusage myusage, childusage;
	if(getrusage(RUSAGE_SELF, &myusage) < 0)
	{
		printf("getrusage error\n");
		exit(1);
	}
	if(getrusage(RUSAGE_CHILDREN, &childusage) < 0)
	{
		printf("getrusage error\n");
		exit(1);
	}

	user = (double)myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec / 1000000.0;
	user += (double)childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec / 1000000.0;
	sys = (double)myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec / 1000000.0;
	sys += (double)childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec / 1000000.0;

	printf("\nuser time = %g, sys time = %g\n", user, sys);
}

void keep_alive(int sockfd, int nbytes, int nloops)
{
	char request[MAXLINE], reply[MAXN];
	snprintf(request, sizeof(request), "%d\n", nbytes);
    ssize_t nwrite, nread;
    for (int i = 0; i < nloops; i++)
    {
        nwrite = write(sockfd, request, strlen(request));
        if((nread = readn(sockfd, reply, nbytes)) != nbytes)
        {
            printf("read %d bytes from server! error!\n", nread);
            close(sockfd);
            exit(1);
        } else 
        {
            printf("read %d bytes from server!\n", nread);
        }
        sleep(1);
    }
    close(sockfd);
}














