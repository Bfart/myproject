/*************************************************************************
	> File Name: threadMutex.c
	> Author: Bfart
	> Mail: ziliu1991@163.com 
	> Created Time: 2016年07月08日 星期五 00时40分03秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

#define THREAD_NUMBER 3
#define REPEAT_NUMBER 3
#define DELAY_TIME_LEVELS 10.0

pthread_mutex_t mutex;

void *threadFunc(void *arg)
{
	int threadNum = (int)arg;
	int delayTime = 0;
	int count = 0;
	int res;

	res = pthread_mutex_lock(&mutex);
	if (res)
	{
		printf("thread %d lock failed\n",threadNum);
		exit(1);
	}
	printf("thread %d is starting\n",threadNum);
	for (count = 0; count < REPEAT_NUMBER; count++)
	{
		delayTime = (int)(rand() * DELAY_TIME_LEVELS/(RAND_MAX)) + 1;
		sleep(delayTime);
		printf("\tthread %d:job %d delay = %d\n",threadNum,count,delayTime);
	}
	printf("thread %d finished \n",threadNum);
	pthread_exit(NULL);
}


int main()
{
	pthread_t thread[THREAD_NUMBER];
	int no = 0;
	int res;
	void *threadRes;
	
	srand(time(NULL));
	pthread_mutex_init(&mutex,NULL);
	for (no = 0; no < THREAD_NUMBER; no++)
	{
		res = pthread_create(&thread[no], NULL, threadFunc, (void *)no);
		if (res != 0)
		{
			printf("create thread %d failed\n",no);
			exit(1);
		}

	}

	printf("create threads success \n waiting for threads to finish.....\n");
	for (no = 0; no < THREAD_NUMBER; no++)
	{
		res = pthread_join(thread[no], &threadRes);
		if (!res)
		{
			printf("thread %d joined\n", no);
		}
		else
		{
			printf("thread %d join failed\n", no);
		}
		pthread_mutex_unlock(&mutex);

	}
	pthread_mutex_destroy(&mutex);

	return 0;
}
