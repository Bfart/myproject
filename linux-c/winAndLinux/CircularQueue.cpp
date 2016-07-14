/************************************************
*程序：CircularQueue.cpp
*功能：循环队列,线程安全
*版权：深圳金证科技股份有限公司
*时间：2013.11.01
*作者：闫燕飞
*修改日期：
*修改内容：
**************************************************/
#include <stdlib.h>
#include <errno.h>
#include "CircularQueue.h"

/*********************************************************************
 * 函数名称:CCircularQueue
 * 说明:类构造函数
 * 入口参数:
 *      int   iMaxDepth = 1024,     最大队列深度
 * 返回值:
 *	    无
 * 作者: YanYF
 * 时间 : 2013-11-01 09:50
*********************************************************************/
CCircularQueue::CCircularQueue(int   iMaxDepth /*= 0*/)
    :m_pQueueSpace(NULL)
	,m_iMaxDepth(iMaxDepth)
	,m_iQueueHead(0)
	,m_iQueueTail(0)
#if defined(WIN32) || defined(WIN64)
    ,m_hSemEmpty(NULL)
	,m_hSemStore(NULL)
	,m_hExitEvt(NULL)
#endif
{
	//初始化锁
	InitLock(&m_lock, NULL);
#if defined(WIN32) || defined(WIN64)
	//创建退出事件
	m_hExitEvt = ::CreateEvent(NULL, TRUE, FALSE, NULL);
#endif

	//创建相应的信号量
	if (m_iMaxDepth > 0 && m_iMaxDepth < MAX_CIRCULAR_QUEUE_DEPTH)
	{
		Init();
	}
}

/*********************************************************************
 * 函数名称:~CCircularQueue
 * 说明:类析构函数
 * 入口参数:
 *      void
 * 返回值:
 *	    无
 * 作者: YanYF
 * 时间 : 2013-11-01 10:00
*********************************************************************/
CCircularQueue::~CCircularQueue()
{
	Destroy();
	DestroyLock(&m_lock);
}

/*********************************************************************
 * 函数名称:Create
 * 说明:创建队列
 * 入口参数:
 *      int   iMaxDepth,       需要创建的队列深度
 * 返回值:
 *	    int    0成功, -1失败
 * 作者: YanYF
 * 时间 : 2013-11-01 10:03
*********************************************************************/
int   CCircularQueue::Create(int   iMaxDepth)
{
	if (iMaxDepth <= 0 || iMaxDepth > MAX_CIRCULAR_QUEUE_DEPTH)
	{
		return -1;
	}

	Lock(&m_lock);
	//如果已经Create了，直接返回
	if(m_iMaxDepth)
	{
		UnLock(&m_lock);
		return 0;
	}
	m_iMaxDepth = iMaxDepth;
	//初始化
	if (-1 == Init())
	{
		UnLock(&m_lock);
		return -1;
	}
	UnLock(&m_lock);

	EmptyQ();

	return 0;
}

/*********************************************************************
 * 函数名称:Init
 * 说明:初始化信号量及队列空间空间
 * 入口参数:
 *      void
 * 返回值:
 *	    int    0成功, -1失败
 * 作者: YanYF
 * 时间 : 2013-11-01 10:08
*********************************************************************/
int     CCircularQueue::Init()
{
#if defined(WIN32) || defined(WIN64)
	m_hSemEmpty = ::CreateSemaphore(NULL, m_iMaxDepth, m_iMaxDepth, NULL);
	if (NULL == m_hSemEmpty)
	{
		return -1;
	}
	m_hSemStore = ::CreateSemaphore(NULL, 0, m_iMaxDepth, NULL);
	if (NULL == m_hSemStore)
	{
		::CloseHandle(m_hSemEmpty);
		m_hSemEmpty = NULL;
		return -1;
	}

#else
	if (sem_init(&m_hSemEmpty, 0, m_iMaxDepth))
	{
		return -1;
	}

	if (sem_init(&m_hSemStore, 0, 0))
	{
		sem_destroy(&m_hSemEmpty);
		return -1;
	}
#endif

	/*分配空间*/
	/*循环队列要多分配出一个空间*/
	++m_iMaxDepth;
	m_pQueueSpace = (LPQUEUE_ELEMENT)malloc(m_iMaxDepth * sizeof(QUEUE_ELEMENT));
	if (NULL == m_pQueueSpace)
	{
		Destroy();
		return -1;
	}
	memset(m_pQueueSpace, 0, m_iMaxDepth * sizeof(QUEUE_ELEMENT));
	m_iQueueHead = 0;
	m_iQueueTail = 0;

	return 0;
}

/*********************************************************************
 * 函数名称:Destroy
 * 说明:销毁队列
 * 入口参数:
 *      void
 * 返回值:
 *	     void
 * 作者: YanYF
 * 时间 : 2013-11-01 10:14
*********************************************************************/
void  CCircularQueue::Destroy()
{
#if defined(WIN32) || defined(WIN64)
	if (m_hExitEvt)
	{
		::SetEvent(m_hExitEvt);
		::CloseHandle(m_hExitEvt);
		m_hExitEvt = NULL;
	}
#endif
    /*清空队列*/
	EmptyQ();

	/*销毁信号量*/
	Lock(&m_lock);
#if defined(WIN32) || defined(WIN64)
	if (m_hSemEmpty)
	{
		::CloseHandle(m_hSemEmpty);
		m_hSemEmpty = NULL;
	}

	if (m_hSemStore)
	{
		::CloseHandle(m_hSemStore);
		m_hSemStore = NULL;
	}
#else
	if (m_iMaxDepth && m_pQueueSpace)
	{
		sem_destroy(&m_hSemEmpty);
		sem_destroy(&m_hSemStore);
	}

	if(m_pQueueSpace)
	{
	    free(m_pQueueSpace);
		m_pQueueSpace = NULL;
	}
	m_iMaxDepth = 0;
#endif
	UnLock(&m_lock);
}

/*********************************************************************
 * 函数名称:EmptyQ
 * 说明:清空队列
 * 入口参数:
 *      void
 * 返回值:
 *	     void
 * 作者: YanYF
 * 时间 : 2013-11-01 10:19
*********************************************************************/
void  CCircularQueue::EmptyQ()
{
	LPQUEUE_ELEMENT   pElem;
	Lock(&m_lock);
	while (m_iQueueHead != m_iQueueTail)
	{
		pElem = m_pQueueSpace + m_iQueueHead;
		if (pElem->pBuffer)
		{
            free(pElem->pBuffer);
			pElem->pBuffer = NULL;
			pElem->iDataLen = 0;
		}

		m_iQueueHead = (++m_iQueueHead) % m_iMaxDepth;
#if defined(WIN32) || defined(WIN64)
		::ReleaseSemaphore(m_hSemEmpty, 1, NULL);
#else
		sem_post(&m_hSemEmpty);
#endif
	}
	UnLock(&m_lock);
}

/*********************************************************************
 * 函数名称:GetDepth
 * 说明:获取队列深度
 * 入口参数:
 *      void
 * 返回值:
 *	     int
 * 作者: YanYF
 * 时间 : 2013-11-01 10:26
*********************************************************************/
int     CCircularQueue::GetDepth()
{
	return (m_iQueueTail - m_iQueueHead + m_iMaxDepth) %m_iMaxDepth;
}

#if defined(WIN32) || defined(WIN64)
#else
/*********************************************************************
 * 函数名称:TimedWait
 * 说明:等待指定时间成功获取信号量则返回0否则返回-1
 * 入口参数:
 *       sem_t *pSemaphore, 信号量
 *       int iMillSecond,等待时间
 * 返回值:
 *	  int
 * 作者: YanYF
 * 时间 : 2012-12-28 17:46
*********************************************************************/
static int TimedWait(sem_t *pSemaphore, int iMillSecond)
{
    struct timespec tv;
    int iRetVal = -1;
	if(iMillSecond < 0)
	{
	   while(((iRetVal = sem_wait(pSemaphore)) != 0) && (errno ==EINTR))
	   {
	       return iRetVal;
	   }
	}
	else if(0 == iMillSecond)
	{
	    iRetVal = sem_trywait(pSemaphore);
	}
	else
	{
	   //获取当前时间
	  clock_gettime(CLOCK_REALTIME, &tv);
		tv.tv_sec += (iMillSecond / 1000);
		tv.tv_nsec += (iMillSecond % 1000) * 1000 * 1000;
		if (tv.tv_nsec > 999999999)
		{
			tv.tv_nsec = 0;
		}
		
		iRetVal = sem_timedwait(pSemaphore, &tv);
	}

	if(0 == iRetVal)
	{
	    return 0;
	}

	return -1;
}
#endif

/*********************************************************************
 * 函数名称:Put
 * 说明:向队列中添加数据
 * 入口参数:
 *      const void *pData,            需要添加的数据
 *      int     iDataLen,                需要添加的数据长度
 *      int     iTimeOut,                等待超时时间
 * 返回值:
 *	     QUEUE_OPERATE_RET
 * 作者: YanYF
 * 时间 : 2013-11-01 10:29
*********************************************************************/
QUEUE_OPERATE_RET    CCircularQueue::Put(const void *pData, int iDataLen, int iTimeOut)
{
	void *pTempData = NULL;
#if defined(WIN32) || defined(WIN64)
	HANDLE   hEvent[2] = {m_hSemEmpty, m_hExitEvt};
	switch(::WaitForMultipleObjects(2, hEvent, FALSE, iTimeOut))
	{
	case WAIT_FAILED:
		/*调用WaitForMultipleObjects失败*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//如果等待失败则返回
	if(-1 == TimedWait(&m_hSemEmpty, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif

	//分配空间用来存储数据
	if (iDataLen)
	{
		pTempData = (void *)malloc(iDataLen);
		if (NULL == pTempData)
		{
#if defined(WIN32) || defined(WIN64)
			::ReleaseSemaphore(m_hSemEmpty, 1, NULL);
#else
			sem_post(&m_hSemEmpty);
#endif
			return QUEUE_OPERATE_ERR_UNKOWN;
		}
		/*拷贝数据*/
		memcpy(pTempData, pData, iDataLen);
	}

	/*进入队列*/
	Lock(&m_lock);
	m_pQueueSpace[m_iQueueTail].iDataLen = iDataLen;
	m_pQueueSpace[m_iQueueTail].pBuffer = pTempData;
	m_iQueueTail = (++m_iQueueTail) % m_iMaxDepth;
	UnLock(&m_lock);

	/*发送有数据信号*/
#if defined(WIN32) || defined(WIN64)
	::ReleaseSemaphore(m_hSemStore, 1, NULL);
#else
	sem_post(&m_hSemStore);
#endif

	return QUEUE_OPERATE_SUCCESS;
}

/*********************************************************************
 * 函数名称:PutM
 * 说明:向队列中添加数据
 * 入口参数:
 *      const void *pData,            需要添加的数据
 *      int     iDataLen,                需要添加的数据长度
 *      int     iTimeOut,                等待超时时间
 * 返回值:
 *	     int
 * 作者: YanYF
 * 时间 : 2013-11-01 10:40
*********************************************************************/
QUEUE_OPERATE_RET    CCircularQueue::PutM(const void *pData, int iDataLen, int iTimeOut)
{
	void *pTempData = NULL;
#if defined(WIN32) || defined(WIN64)
	HANDLE   hEvent[2] = {m_hSemEmpty, m_hExitEvt};
	switch(::WaitForMultipleObjects(2, hEvent, FALSE, iTimeOut))
	{
	case WAIT_FAILED:
		/*调用WaitForMultipleObjects失败*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//如果等待失败则返回
	if(-1 == TimedWait(&m_hSemEmpty, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif
	/*进入队列*/
	Lock(&m_lock);
	m_pQueueSpace[m_iQueueTail].iDataLen = iDataLen;
	m_pQueueSpace[m_iQueueTail].pBuffer = (void *)pData;
	m_iQueueTail = (++m_iQueueTail) % m_iMaxDepth;
	UnLock(&m_lock);

	/*发送有数据信号*/
#if defined(WIN32) || defined(WIN64)
	::ReleaseSemaphore(m_hSemStore, 1, NULL);
#else
	sem_post(&m_hSemStore);
#endif

	return QUEUE_OPERATE_SUCCESS;
}

/*********************************************************************
 * 函数名称:Get
 * 说明:从队列中取数据
 * 入口参数:
 *      void **pData,                   存放取到的数据
 *      int *piDataLen,                 存放取道的数据长度
 *      int     iTimeOut,                等待超时时间
 * 返回值:
 *	     int
 * 作者: YanYF
 * 时间 : 2013-11-01 10:53
*********************************************************************/
QUEUE_OPERATE_RET    CCircularQueue::Get(void **pData, int *piDataLen, int iTimeOut)
{
	void *pTempData = NULL;
	LPQUEUE_ELEMENT   pTempElem = NULL;
	if (!piDataLen)
	{
		return QUEUE_OPERATE_ERR_UNKOWN;
	}
#if defined(WIN32) || defined(WIN64)
	HANDLE   hEvent[2] = {m_hSemStore, m_hExitEvt};
	switch(::WaitForMultipleObjects(2, hEvent, FALSE, iTimeOut))
	{
	case WAIT_FAILED:
		/*调用WaitForMultipleObjects失败*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//如果等待失败则返回
	if(-1 == TimedWait(&m_hSemStore, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif

	/*出队列*/
	Lock(&m_lock);
	pTempElem = m_pQueueSpace + m_iQueueHead;
	pTempData = pTempElem->pBuffer;
	pTempElem->pBuffer = NULL;
	*piDataLen = pTempElem->iDataLen;
	pTempElem->iDataLen = 0;
	m_iQueueHead = (++m_iQueueHead) % m_iMaxDepth;
	UnLock(&m_lock);

	/*发送有空位的信号量*/
#if defined(WIN32) || defined(WIN64)
	::ReleaseSemaphore(m_hSemEmpty, 1, NULL);
#else
	sem_post(&m_hSemEmpty);
#endif

	if (*piDataLen)
	{
		*pData = malloc(*piDataLen);
		if (NULL == *pData)
		{
			free(pTempData);
			return QUEUE_OPERATE_ERR_UNKOWN;
		}
		memcpy(*pData, pTempData, *piDataLen);
	}

	return QUEUE_OPERATE_SUCCESS;
}

/*********************************************************************
 * 函数名称:GetM
 * 说明:从队列中取数据
 * 入口参数:
 *      void **pData,                   存放取到的数据
 *      int     iDataLen,                需要添加的数据长度
 *      int     iTimeOut,                等待超时时间
 * 返回值:
 *	     int
 * 作者: YanYF
 * 时间 : 2013-11-01 11:03
*********************************************************************/
QUEUE_OPERATE_RET  CCircularQueue::GetM(void **pData, int *piDataLen, int iTimeOut)
{
	LPQUEUE_ELEMENT   pTempElem = NULL;
	if (!piDataLen)
	{
		return QUEUE_OPERATE_ERR_UNKOWN;
	}
#if defined(WIN32) || defined(WIN64)
	HANDLE   hEvent[2] = {m_hSemStore, m_hExitEvt};
	switch(::WaitForMultipleObjects(2, hEvent, FALSE, iTimeOut))
	{
	case WAIT_FAILED:
		/*调用WaitForMultipleObjects失败*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//如果等待失败则返回
	if(-1 == TimedWait(&m_hSemStore, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif

	/*出队列*/
	Lock(&m_lock);
	pTempElem = m_pQueueSpace + m_iQueueHead;
	*pData = pTempElem->pBuffer;
	pTempElem->pBuffer = NULL;
	*piDataLen = pTempElem->iDataLen;
	pTempElem->iDataLen = 0;
	m_iQueueHead = (++m_iQueueHead) % m_iMaxDepth;
	UnLock(&m_lock);

	/*发送有空位的信号量*/
#if defined(WIN32) || defined(WIN64)
	::ReleaseSemaphore(m_hSemEmpty, 1, NULL);
#else
	sem_post(&m_hSemEmpty);
#endif
	return QUEUE_OPERATE_SUCCESS;
}

#if defined(WIN32) || defined(WIN64)
/*********************************************************************
 * 函数名称:TellGetPutExit
 * 说明:告知Get和Put退出事件
 * 入口参数:
 *     void
 * 返回值:
 *     void
 * 修改历史
*********************************************************************/
void   CCircularQueue::TellGetPutExit()
{
	if (m_hExitEvt)
	{
		::SetEvent(m_hExitEvt);
	}
}
#endif

/*********************************************************************
 * 函数名称:EmptyQWithoutFree
 * 说明:不释放空间的清空队列
 * 入口参数:
 *     void
 * 返回值:
 *     void
 * 时间 : 2014-02-28 17:13
 * 修改历史
*********************************************************************/
void   CCircularQueue::EmptyQWithoutFree()
{
	Lock(&m_lock);
	while (m_iQueueHead != m_iQueueTail)
	{
		m_iQueueHead = (++m_iQueueHead) % m_iMaxDepth;
#if defined(WIN32) || defined(WIN64)
		::ReleaseSemaphore(m_hSemEmpty, 1, NULL);
#else
		sem_post(&m_hSemEmpty);
#endif
	}
	UnLock(&m_lock);
}