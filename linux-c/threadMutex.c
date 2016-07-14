/*************************************************************************
	> File Name: threadMutex.c
	> Author: Bfart
	> Mail: ziliu1991@163.com 
	> Created Time: 2016年07月08日 星期五 11时23分24秒
 ************************************************************************/

# include <stdio.h>
# include <stdlib.h>
# include <pthread.h>

# define THREAD_NUMBER 3
# define REPEART_NUMBER 3
# define DELAY_TIME_LEVELS 10.0

pthread_mutex_t mutex;

void *threadFunc(void *arg)
{
	int threadNum = (int)arg;
	int delayTime = 0;
	int count = 0;
	int res;

	res = pthread_mutex_lock(&mutex);
	
	if(res)
	{
		printf("Tread %d lock failed\n",threadNum);
		pthread_exit(NULL);
	}

	printf("Thread %d is starting \n", threadNum);
	for (count = 0; count < REPEART_NUMBER; count++)
	{
		delayTime = (int)(rand() * DELAY_TIME_LEVELS/(RAND_MAX)) + 1;
		sleep(delayTime);
		printf("\tThread %d: job %d delay = %d\n",)
	}
}
