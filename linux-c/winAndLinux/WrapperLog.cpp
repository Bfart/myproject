#include "WrapperLog.h"


//日志队列
CCircularQueue * g_CirticalQueueLog = NULL;
//消息丢弃时记录结构指针
tagLog * g_pstError;
//日志线程指针
CThreadWork *pThreadWork = NULL;

//线程互斥
CRITICAL_SECTION g_Critical_LogWarning;
int LogInit(int nLimit)
{
	int nParam = 0;
	DWORD dwThreadID = 0;

	InitializeCriticalSection(&g_Critical_LogWarning);

	if (g_CirticalQueueLog == NULL)
	{
		g_CirticalQueueLog = new CCircularQueue(nLimit);
		if (g_CirticalQueueLog == NULL)
		{
			return FALSE;
		}
	}

	try
	{
		pThreadWork = new CThreadWork;
		pThreadWork->Start();
	}
	catch (...)
	{
		OutputDebugString("HST2.Log:初始化日志线程失败");
		return FALSE;	
	}

	return TRUE;

}
int LogQuit()
{
	pThreadWork->Stop();
	if (pThreadWork != NULL)
	{
		delete pThreadWork;
		pThreadWork = NULL;
	}
	
	if (g_CirticalQueueLog != NULL)
	{
		delete g_CirticalQueueLog;
		g_CirticalQueueLog = NULL;
	}
	DeleteCriticalSection(&g_Critical_LogWarning);

	return TRUE;
}
//写日志 
int WritePacketLog(int nMsgID,int nMSGLeave,char* pFuncId,void *p_pBuffer,int p_iBufferLen)
{
    tagLog* pstLog = NULL;
	LARGE_INTEGER lgClock, lgFreq;
	QueryPerformanceCounter(&lgClock);
	QueryPerformanceFrequency(&lgFreq);
	
	try {
		pstLog = new tagLog;
		memset(pstLog,  0x00, sizeof(tagLog));
        char szTime[128]={0};
		char* pLog = (char*) malloc(p_iBufferLen + 1 ); 
		memcpy(pLog,p_pBuffer,p_iBufferLen);
		SYSTEMTIME localTime;	
		GetLocalTime(&localTime);

		//日志打到DEBUGVIEW.EXE上
		//OutputDebugString((char*)p_pBuffer);

		//组织日志结构
		_snprintf(pstLog->szTime, sizeof(pstLog->szTime)-1, "%04d%02d%02d %02d:%02d:%02d",
			localTime.wYear, localTime.wMonth, localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond);
		pstLog->dClockValue = (double)lgClock.QuadPart/lgFreq.QuadPart;
		pstLog->dClockValue = 1000*1000*pstLog->dClockValue;
		pstLog->lThreadID   = GetCurrentThreadId();
		pstLog->pLogMsg		= pLog;
		pstLog->nLogID		= nMsgID;
		pstLog->nLogType	= nMSGLeave;
		strncpy(pstLog->szFuncId, pFuncId, sizeof(pstLog->szFuncId));
	
		int nRet = g_CirticalQueueLog->Put(pstLog,sizeof(tagLog),5000);

		if (nRet != QUEUE_OPERATE_SUCCESS)
		{
			//放入队列失败，将记录在日志中。
			EnterCriticalSection(&g_Critical_LogWarning);
			if (g_pstError != NULL)
			{
				if (g_pstError->pLogMsg != NULL)
				{
					delete g_pstError->pLogMsg;
				}
				memset(g_pstError, 0x00, sizeof(tagLog));
			}
			else
			{
				g_pstError = new tagLog;
			}
			g_pstError->dClockValue = pstLog->dClockValue;
			g_pstError->lThreadID	= pstLog->lThreadID;
			g_pstError->nLogID		= pstLog->nLogID;
			g_pstError->nLogType	= MSG_WARNING;
			g_pstError->pLogMsg		=  (char*) malloc(64 + 1 );
			strncpy(g_pstError->szFuncId,	pstLog->szFuncId,	sizeof(g_pstError->szFuncId));
			strncpy(g_pstError->szTime,		pstLog->szTime,		sizeof(g_pstError->szTime));
			_snprintf(g_pstError->pLogMsg, sizeof(64 + 1), "日志队列已满");
			LeaveCriticalSection(&g_Critical_LogWarning);
			//清理未成功放入队列的结构
			if (pstLog != NULL)
			{
				if (pstLog->pLogMsg != NULL)
				{
					delete pstLog->pLogMsg;
					pstLog->pLogMsg = NULL;
				}
				
				delete pstLog;
				pstLog = NULL;
			}
			return -1;
			
		}
		
    }
    catch(...) {
        return -2;
    }
    return 0;
}

int PutLog(int nMsgID,int nMSGLeave,char* pFuncId,char *p_pszErrorMessageFMT, ...)
{
    
	va_list arg_ptr;
    char szMessage[8192]={0};
	
	va_start(arg_ptr, p_pszErrorMessageFMT);
	_vsnprintf(szMessage, sizeof(szMessage)-1, p_pszErrorMessageFMT, arg_ptr);
	va_end(arg_ptr);
#ifdef _DEBUG
	OutputDebugString(szMessage);
	OutputDebugString("\n");
#endif 
	return WritePacketLog(nMsgID, nMSGLeave, pFuncId, szMessage, sizeof(szMessage));	
	
	return 1;
}




int CThreadWork::Run()
{
	char szFileName[512] = {0};
	char szFileDir[512] = {0};
	SYSTEMTIME localTime;	
	//FILE *g_fpLogFile=NULL;
	HANDLE hFile = NULL;
	tagLog* pstLog = NULL;
	char szLogWrite[8192] = {0};
	char szTemp[32] = {0};
	int nIsError = 0 ;
	GetLocalTime(&localTime);

	_snprintf(szFileName, sizeof(szFileName)-1, ".\\log\\%04d%02d%02d\\wrapper_HST2_%04x.log", 
		localTime.wYear, localTime.wMonth, localTime.wDay,GetCurrentThreadId());
	_snprintf(szFileDir, sizeof(szFileDir)-1, ".\\log\\%04d%02d%02d", 
		localTime.wYear, localTime.wMonth, localTime.wDay);
	try
	{
		//创建文件夹
		//设置属性
		SECURITY_ATTRIBUTES attribute;
		attribute.nLength = sizeof(attribute);
		attribute.lpSecurityDescriptor = NULL;
		attribute.bInheritHandle = FALSE;
		//创建
		CreateDirectoryA(szFileDir,&attribute);
		//创建或打开文件
		HANDLE hFile = CreateFile(szFileName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ,NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_ARCHIVE,
			NULL);
		if (INVALID_HANDLE_VALUE == hFile) 
		{

			return -1;
		}

		SetFilePointer(hFile,0,NULL,FILE_END);
		while (1)
		{
			DWORD len = 0;
			tagLog stTemp;
			if (g_CirticalQueueLog == NULL)
			{
				return 0;
			}
			EnterCriticalSection(&g_Critical_LogWarning);
			if (g_pstError != NULL)
			{
				memset(&stTemp, 0x00, sizeof(stTemp));
				stTemp.dClockValue	= g_pstError->dClockValue;
				stTemp.lThreadID	= g_pstError->lThreadID;
				stTemp.nLogID		= g_pstError->nLogID;
				stTemp.nLogType		= g_pstError->nLogType;
				stTemp.pLogMsg		=  (char*) malloc(64 + 1 );
				strncpy(stTemp.szFuncId,	g_pstError->szFuncId,	sizeof(stTemp.szFuncId));
				strncpy(stTemp.szTime,		g_pstError->szTime,		sizeof(stTemp.szTime));
				strncpy(stTemp.pLogMsg,		g_pstError->pLogMsg,	(64 + 1 ));
				pstLog = &stTemp;
				nIsError = 1;
				//清理
				if (g_pstError->pLogMsg != NULL)
				{
					delete g_pstError->pLogMsg;
					g_pstError->pLogMsg = NULL;
				}
				delete g_pstError;
				g_pstError = NULL;
				LeaveCriticalSection(&g_Critical_LogWarning);
				goto __Process;
			}
			LeaveCriticalSection(&g_Critical_LogWarning);

			int iDataLen = sizeof(tagLog);
			void ** ppstLog = (void **)malloc(sizeof(tagLog));
			int flag = g_CirticalQueueLog->Get(ppstLog,&iDataLen,5000);
			pstLog = (tagLog *)*ppstLog;
			free(ppstLog);

			if (pstLog == NULL)
			{
				goto __Continue;
			}
__Process:
			//信息处理
			if (pstLog->nLogType >= MSG_WARNING)
			{

				if (strlen(pstLog->szFuncId) <= 0)
				{
					//不需要写日志
					goto __Continue;
				}
			}


			switch(pstLog->nLogType)
			{
			case MSG_ERROR:
				_snprintf(szTemp, sizeof(szTemp), "ERROR");
				break;
			case MSG_WARNING:
				_snprintf(szTemp, sizeof(szTemp), "WARNING");
				break;
			case MSG_ANSLOG:
				_snprintf(szTemp, sizeof(szTemp), "ANSLOG");
				break;
			default:
				_snprintf(szTemp, sizeof(szTemp), "NORMAL");
			}
			_snprintf(szLogWrite, sizeof(szLogWrite), "[%s] [%15.0f][%-7s][MSGID:%4d][%6x][REQ:%-8s]  %s\x0d\x0a",
				pstLog->szTime,
				pstLog->dClockValue,
				szTemp,
				pstLog->nLogID,
				pstLog->lThreadID,
				pstLog->szFuncId,
				pstLog->pLogMsg);
			//int n = fwrite("\r\n",2,1,g_fpLogFile);	
			//int n  = fwrite(szLogWrite,strlen(szLogWrite),1,g_fpLogFile);

			WriteFile(hFile,szLogWrite,strlen(szLogWrite),&len,NULL);
__Continue:
			if (pstLog != NULL&&nIsError == 0)
			{
				if (pstLog->pLogMsg != NULL)
				{
					delete pstLog->pLogMsg;
					pstLog->pLogMsg = NULL;
				}

				delete pstLog;
				pstLog = NULL;
			}
			nIsError = 0;

		}
		//fclose(g_fpLogFile);
		CloseHandle(hFile);
	}
	catch (...)
	{
		if (pstLog != NULL)
		{
			if (pstLog->pLogMsg != NULL)
			{
				delete pstLog->pLogMsg;
				pstLog->pLogMsg = NULL;
			}

			delete pstLog;
			pstLog = NULL;
		}
		return -1;
	}

	return 0;
}