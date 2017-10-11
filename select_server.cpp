#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "common.h"

#define BUF_LEN 1024
#define SERV_PORT 6000
#define FD_SIZE 400
#define MAX_BACK 100

int main( int argc, char ** argv )
{
	int			listenfd, connfd, sockfd, maxfd, maxi, i;
	int			nready, client[FD_SIZE];		//!> 接收select返回值、保存客户端套接字
	int			lens;
	ssize_t		n;				//!> read字节数
	fd_set		rset, allset;	//!> 不要理解成就只能保存一个，其实fd_set有点像封装的数组
	char		buf[BUF_LEN];
	socklen_t	clilen;
	char		send_buf[MAXN];
	struct sockaddr_in servaddr, chiaddr;

	memset(send_buf, 'a', MAXN); send_buf[MAXN-1] = '\0';

	if( (listenfd = socket( AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf( "Create socket Error : [%d] [%s]\n", errno, strerror(errno) );
		exit( EXIT_FAILURE );
	}

	//!> 下面是接口信息
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr  =htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	//!> 绑定
	if(bind( listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		printf("Bind Error : [%d] [%s]\n", errno, strerror(errno));
		exit(EXIT_FAILURE  );
	}

	//!> 监听
	if(listen( listenfd, MAX_BACK) == -1)
	{
		printf("Listen Error : [%d] [%s]\n", errno, strerror(errno));
		exit( EXIT_FAILURE );
	}

	//!> 当前最大的感兴趣的套接字fd
	maxfd = listenfd;	//!> 当前可通知的最大的fd
	maxi = -1;			//!> 仅仅是为了client数组的好处理
	for(i = 0; i < FD_SIZE; i++)	//!> 首先置为全-1
	{
		client[i] = -1;		//!> 首先client的等待队列中是没有的，所以全部置为-1
	}

	FD_ZERO(&allset);			//!> 先将其置为0
	FD_SET(listenfd, &allset);	//!> 说明当前我对此套接字有兴趣，下次select的时候通知我！

	while(true)
	{
		rset = allset;//!> 由于allset可能每次一个循环之后都有变化，所以每次都赋值一次
		if( (nready = select( maxfd + 1, &rset, NULL, NULL, NULL)) == -1) // 出错
		{
			if(errno == EINTR)
			{
				continue;
			}
			printf("Select Erorr : %d %s\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}

		if( nready == 0 )			//!> if 所有的感兴趣的没有就接着回去select
		{
			continue;
		}

		if(FD_ISSET(listenfd, &rset))			//!> if 是监听接口上的“来电”
		{
			clilen = sizeof(chiaddr);
			if( (connfd = accept(listenfd, (struct sockaddr *)&chiaddr, &clilen)) == -1)
			{										//!> accept 返回的还是套接字
				printf("Accept Error : %d %s\n", errno, strerror(errno));
				continue;
			}

			// 将connfd挂在最小的槽上
			for(i = 0; i < FD_SIZE; i++)
			{
				if(client[i] < 0)
				{
					client[i] = connfd;			//!> 将client的请求连接保存
					break;
				}
			}

			if( i == FD_SIZE )				//!> The last one
			{
				printf("Too many connfd\n");
				close(connfd);				//!> if 满了那么就不连接你了，关闭吧
				continue;					//!> 返回
			}
											//!> listen的作用就是向数组中加入套接字！
			FD_SET(connfd, &allset);	//!> 说明现在对于这个连接也是感兴趣的！
											//!> 所以加入allset的阵容
			if(connfd > maxfd)			//!> 这个还是为了解决乱七八糟的数组模型的处理
			{
				maxfd = connfd;
			}

			if(i > maxi)					//!> 同上
			{
				maxi = i;
			}
		}

		//!> 下面就是处理数据函数( 其实说本质的select还是串行 )
		for(i = 0; i <= maxi; i++)		//!> 对所有的连接请求的处理
		{
			if((sockfd = client[i]) > 0)	//!> 还是为了不规整的数组
			{			//!> 也就说client数组不是连续的全正数或者-1，可能是锯齿状的
				if(FD_ISSET(sockfd, &rset))	//!> if 当前这个数据套接字有要读的
				 {
					memset(buf, 0, sizeof(buf));	//!> 此步重要，不要有时候出错
					n = read(sockfd, buf, BUF_LEN);
					if(n < 0)
					{
						if(errno == EINTR)
						{
							continue;
						}
						printf("connfd %d read error %d %s!\n", sockfd, errno, strerror(errno));
						close( sockfd );			//!> 说明在这个请求端口上出错了！
						FD_CLR(sockfd, &allset);
						client[i] = -1;
						exit(1);
					}
					if(n == 0)
					{
						close(sockfd);			//!> 说明在这个请求端口上读完了！
						FD_CLR(sockfd, &allset);
						client[i] = -1;
						continue;
					}

					int nwrite = atoi(buf);
					if(nwrite > MAXN)
					{
						printf("client request %d bytes data\n", nwrite);
						exit(1);
					}
					writen(sockfd, send_buf, nwrite);
				}
			}
		}

	}

	return 0;
}
