/*************************************************************************
	> File Name: msgrecv.c
	> Author: Bfart
	> Mail: ziliu1991@163.com 
	> Created Time: 2016年07月08日 星期五 10时24分25秒
 ************************************************************************/

# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/msg.h>
# include <stdio.h>
# include <stdlib.h>
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
		perror("ftok");
		exit(1);
	}

	if ((queueId = msgget(key,IPC_CREAT|0666)) == -1)
	{
		perror("msgget");
		exit(1);
	}

	printf("open queue %d\n",queueId);

	do
	{
		memset(msg.msgText, 0 , BUFFSIZE);
		if (msgrcv(queueId,(void *)&msg, BUFFSIZE, 0, 0) < 0)
		{
			perror("msgrcv");
			exit(1);
		}
		
		printf("the message from process %ld: %s", msg.msgType, msg.msgText);
	}while(strncmp(msg.msgText,"quit",4));

	if ((msgctl(queueId, IPC_RMID, NULL)) < 0)
	{
		perror("msgctl");
		exit(1);
	}

	exit(0);

}
