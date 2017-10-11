#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "common.h"



int main(int argc, char* argv[])
{
	int nchildren, nloops, nbytes;
	pid_t pid;
	ssize_t n;
	timeval t1, t2;
	char request[MAXLINE], reply[MAXN];

	if(argc != 6)
	{
		printf("usage: client <ip> <port> <#children> <#loops/child> <#bytes/request>\n");
		exit(1);
	}

	nchildren = atoi(argv[3]);
	nloops = atoi(argv[4]);
	nbytes = atoi(argv[5]);
	snprintf(request, sizeof(request), "%d\n", nbytes);

	gettimeofday(&t1,NULL);
	for(int i = 0; i < nchildren; ++i)
	{
		pid = fork();
		if(pid == 0) // 子进程
		{
			for(int j = 0; j < nloops; ++j)
			{
				int sockfd = socket(AF_INET, SOCK_STREAM, 0);
				struct sockaddr_in servaddr;
				bzero(&servaddr, sizeof(servaddr));
				servaddr.sin_family = AF_INET;
				inet_pton(AF_INET, argv[1], &servaddr.sin_addr);	// ip
				servaddr.sin_port = htons(atoi(argv[2]));			// 端口
				if(connect(sockfd, (const struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
				{
					close(sockfd);
					printf("connect fail\n");
					exit(1);
				}

                keep_alive(sockfd, nbytes, nloops);
				//ssize_t nwrite = write(sockfd, request, strlen(request));
				//ssize_t nread;
				//if((nread = readn(sockfd, reply, nbytes)) != nbytes)
				//{
				//	printf("read %d bytes from server!\n", nread);
				//	close(sockfd);
				//	exit(1);
				//}
				//close(sockfd);
			}
			printf("child %d done\n", i);
			exit(0);
		}
	}

	while(wait(NULL) > 0)
		;
	if(errno != ECHILD)
	{
		printf("wait error %d!\n", errno);
		exit(1);
	}

	gettimeofday(&t2, NULL);
	double seconds = (t2.tv_sec-t1.tv_sec) * 1.0 + (t2.tv_usec-t1.tv_usec) * 1.0 / 1000000;
	double qps = nchildren * nloops / seconds;
	printf("time: %fs, qps: %f\n", seconds, qps);

	return 0;
}
