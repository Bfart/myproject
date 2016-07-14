/*************************************************************************
	> File Name: msgsend.c
	> Author: Bfart
	> Mail: ziliu1991@163.com 
	> Created Time: 2016年07月08日 星期五 10时12分17秒
 ************************************************************************/

# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/msg.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <unistd.h>

# define BUFFSIZE 512

struct message
{
	long msgType;
	char msgText[BUFFSIZE];
};

int main()
{
	int queueId;
	key_t key;
	struct message msg;

	if ((key = ftok(".",'a')) == -1)
	{
		perror("ftok error");
		exit(1);
	}

	if (queueId = msgget(key, IPC_CREAT|0666))
	{
		perror("msgget error");
		exit(1);
	}

	printf("open queue %d\n",queueId);

	while(1)
	{
		printf("enter some message to the queue:\n");
		if ((fgets(msg.msgText, BUFFSIZE, stdin)) == NULL)
		{
			puts("no message");
			exit(1);
		}

		msg.msgType = getpid();

		if ((msgsnd(queueId, &msg, strlen(msg.msgText), 0)) < 0)
		{
			perror("message posted");
			exit(1);
		}
	
		if (strncmp(msg.msgText, "quit", 4) == 0)
		{
			break;
		}

	}

	exit(0);
}
