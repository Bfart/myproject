/************************************************
*程序：CircularQueue.h
*功能：循环队列,线程安全
*版权：深圳金证科技股份有限公司
*时间：2013.11.01
*作者：闫燕飞
*修改日期：
*修改内容：
**************************************************/

#ifndef CIRCULAR_QUEUE_H_H
#define CIRCULAR_QUEUE_H_H
#include <stdio.h>
#include <string.h>

#if defined(WIN32) || defined(WIN64)
	#include <windows.h>

	/*定义一些宏函数*/
	#ifndef snprintf
	    #define snprintf        _snprintf
	#endif

	#ifndef strcasecmp
	    #define strcasecmp   stricmp
	#endif

	/*定义锁相关的宏*/
	#ifndef InitLock
	    #define InitLock(a, b)                 InitializeCriticalSection(a)
	#endif

	#ifndef Lock
	    #define Lock                             EnterCriticalSection
	#endif

	#ifndef UnLock
	    #define UnLock                         LeaveCriticalSection
	#endif

	#ifndef DestroyLock
	    #define DestroyLock                  DeleteCriticalSection
	#endif
	typedef CRITICAL_SECTION  lock_t;
#else
	#include <pthread.h>
	#include <time.h>
	#include <semaphore.h>
	#include <unistd.h>

	/*定义锁相关的宏*/
	#ifndef InitLock
	    #define InitLock(a, b)                pthread_mutex_init(a, b)
	#endif

	#ifndef Lock
	    #define Lock                             pthread_mutex_lock
	#endif

	#ifndef UnLock
	    #define UnLock                         pthread_mutex_unlock
	#endif

	#ifndef DestroyLock
	    #define DestroyLock                  pthread_mutex_destroy
	#endif
	typedef pthread_mutex_t    lock_t;
#endif

#define  MAX_CIRCULAR_QUEUE_DEPTH        100000

typedef enum
{
	QUEUE_OPERATE_ERR_UNKOWN = -3,
	QUEUE_OPERATE_ERR_EXIT        = -2,
	QUEUE_OPERATE_ERR_TIME_OUT = -1,
    QUEUE_OPERATE_SUCCESS          = 0
}QUEUE_OPERATE_RET;

class CCircularQueue
{
public:
	CCircularQueue(int   iMaxDepth = 0);
	virtual ~CCircularQueue();

	/*创建队列*/
	int      Create(int   iMaxDepth);

	/*销毁队列*/
	void    Destroy();

	/*清空队列*/
	void   EmptyQ();

	/*不释放空间的清空队列*/
	void   EmptyQWithoutFree();

	/*获取队列深度*/
	int     GetDepth();

	/*向队列中添加数据*/
	QUEUE_OPERATE_RET    Put(const void *pData, int iDataLen, int iTimeOut);
	QUEUE_OPERATE_RET    PutM(const void *pData, int iDataLen, int iTimeOut);

	/*从队列中取数据*/
	QUEUE_OPERATE_RET    Get(void **pData, int *piDataLen, int iTimeOut);
	QUEUE_OPERATE_RET    GetM(void **pData, int *piDataLen, int iTimeOut);
#if defined(WIN32) || defined(WIN64)
	void   TellGetPutExit();     /*告知Get和Put退出事件*/
#endif

private:
	/*定义队列的元素*/
	typedef struct tagQUEUE_ELEMENT
	{
		void         *pBuffer;                     /*元素中真正存储的数据*/
		int             iDataLen;                  /*存储的数据长度*/
	}QUEUE_ELEMENT, *LPQUEUE_ELEMENT;

	LPQUEUE_ELEMENT                m_pQueueSpace;         /*存储数据元素*/

	int                                       m_iMaxDepth;             /*队列最大深度*/
	int                                       m_iQueueHead;           /*队列头部*/
	int                                       m_iQueueTail;              /*队列尾部*/

	lock_t                                  m_lock;                       /*临界区*/
#if defined(WIN32) || defined(WIN64)
	HANDLE      m_hSemEmpty;   /*可put的信号量*/
	HANDLE      m_hSemStore;    /*可get的信号量*/
	HANDLE      m_hExitEvt;       /*退出事件*/
#else
	sem_t      m_hSemEmpty;      //可put的信号量
	sem_t      m_hSemStore;      //可get的信号量
#endif


private:
	/*初始化信号量及队列空间空间*/
	int     Init();
	/*屏蔽拷贝构造函数*/
	CCircularQueue(const CCircularQueue &);
	CCircularQueue & operator=(const CCircularQueue &);
};

#endif