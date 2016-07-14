#ifndef __WRAPPERLOG_h__
#define __WRAPPERLOG_h__

#include "stdio.h"
#include "CircularQueue.h"
# include "thread.h"


#define MSG_ERROR		-1
#define MSG_WARNING		0
#define MSG_NORMAL		1
#define MSG_ANSLOG	  2
typedef struct 
{
	int  nLogID;										//消息ID
	int  nLogType;										//消息类型
	double dClockValue;									//精确TIK
	char szTime[128];									//粗略时间
	long lThreadID;										//线程号
	char szFuncId[16];									//功能号
	char* pLogMsg;										//消息体

}tagLog;


typedef struct
{
	long reqno;
	char szop_user[24];
	char szop_site[64];
	char szop_branch[24];
	char szchannel[24];
	char szop_role[24];
}TransPlugin;

class CThreadWork: public CThread
{
public:
	int Run();

private:

};


int LogInit(int nLimit = 20000);
int LogQuit();
int WritePacketLog(int nMsgID,int nMSGLeave,char* pFuncId,void *p_pBuffer,int p_iBufferLen);
DWORD WINAPI RunLogThread(LPVOID p);
int PutLog(int nMsgID,int nMSGLeave,char* pFuncId,char *p_pszErrorMessageFMT, ...);

#endif