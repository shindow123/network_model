#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <event.h>

#include "common.h"

char sendbuf[MAXN];

void accept_cb(int fd, short events, void* arg);
void socket_read_cb(int fd, short events, void *arg);

int tcp_server_init(int port, int listen_num);

int main(int argc, char** argv)
{
  memset(sendbuf, 'a', MAXN); sendbuf[MAXN-1] = '\0';

  int listener = tcp_server_init(atoi(argv[1]), 100);
  if( listener == -1 )
  {
    perror(" tcp_server_init error ");
    return -1;
  }

  struct event_base* base = event_base_new();

  //添加监听客户端请求连接事件
  struct event* ev_listen = event_new(base, listener, EV_READ | EV_PERSIST,
                    accept_cb, base);
  event_add(ev_listen, NULL);


  event_base_dispatch(base);

  return 0;
}



void accept_cb(int fd, short events, void* arg)
{
  evutil_socket_t sockfd;

  struct sockaddr_in client;
  socklen_t len;

  sockfd = ::accept(fd, (struct sockaddr*)&client, &len );
  evutil_make_socket_nonblocking(sockfd);

  printf("accept a client %d\n", sockfd);

  struct event_base* base = (event_base*)arg;

  //仅仅是为了动态创建一个event结构体
  struct event *ev = event_new(NULL, -1, 0, NULL, NULL);
  //将动态创建的结构体作为event的回调参数
  event_assign(ev, base, sockfd, EV_READ | EV_PERSIST, socket_read_cb, (void*)ev);

  event_add(ev, NULL);
}


void socket_read_cb(int fd, short events, void *arg)
{
  char msg[4096];
  struct event *ev = (struct event*)arg;
  int len = read(fd, msg, sizeof(msg)-1);
  if( len <= 0 )
  {
    printf("read error!! len:%d, fd:%d %d %s\n", len, fd, errno, strerror(errno));

    close(event_get_fd(ev));
    event_free(ev);
    return ;
  } else {
    printf("read success! fd:%d\n", fd);
  }

  msg[len] = '\0';
  int datalen = atoi(msg);
  write(fd, sendbuf, datalen);
}



typedef struct sockaddr SA;
int tcp_server_init(int port, int listen_num)
{
  int errno_save;
  evutil_socket_t listener;

  listener = ::socket(AF_INET, SOCK_STREAM, 0);
  if( listener == -1 )
    return -1;

  //允许多次绑定同一个地址。要用在socket和bind之间
  evutil_make_listen_socket_reuseable(listener);

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(port);

  if( ::bind(listener, (SA*)&sin, sizeof(sin)) < 0 )
    goto error;

  if( ::listen(listener, listen_num) < 0)
    goto error;


  //跨平台统一接口，将套接字设置为非阻塞状态
  evutil_make_socket_nonblocking(listener);

  return listener;

  error:
    errno_save = errno;
    evutil_closesocket(listener);
    errno = errno_save;

    return -1;
}
