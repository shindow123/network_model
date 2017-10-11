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

#include "common.h"

int main(int argc, char* argv[])
{
	int listenfd, connfd;
	struct sockaddr_in servaddr, cliaddr;
	char rcv_buf[MAXLINE];
	char snd_buf[MAXN];
	prepare_msg(snd_buf);

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

	signal(SIGINT, sig_int);

	while(true)
	{
		socklen_t socklen;
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &socklen);
		if(connfd < 0)
		{
			printf("accept fail\n");
			continue;
		}
		printf("connection from %s, port %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, rcv_buf, sizeof(rcv_buf)), ntohs(cliaddr.sin_port));
		web_child(connfd);
	}

	return 0;
}
