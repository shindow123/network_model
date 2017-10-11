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
#include <pthread.h>
#include <vector>

#include "common.h"

void* func(void*)
{
}

int main(int argc, char* argv[])
{
	timeval t1, t2;
	gettimeofday(&t1,NULL);
	const int ChildNum = 10000;
	std::vector<pthread_t> threads(ChildNum);

	for(int i = 0; i < ChildNum; ++i)
	{
		pthread_create(&threads[i], NULL, &func, NULL);
	}

	for(int i = 0; i < ChildNum; ++i)
	{
		pthread_join(threads[i], NULL);
	}

	gettimeofday(&t2, NULL);
	int64_t useconds = (t2.tv_sec-t1.tv_sec) * 1000000 + (t2.tv_usec-t1.tv_usec);
	double per = useconds * 1.0 / ChildNum;
	printf("pthread_create + pthread_join time: %f us\n", per);

	return 0;
}
