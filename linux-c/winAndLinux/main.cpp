# include <iostream>
# include "CircularQueue.h"
# include "WrapperLog.h"

int main()
{
	LogInit(20000);
	char *pFunId = "1267567";
	char *pMessage = "succ log..........";
	for (int i=0; i<50000; i++)
	{
		PutLog(500010,0,pFunId,pMessage);
	}

	LogQuit();
	return 0;
}