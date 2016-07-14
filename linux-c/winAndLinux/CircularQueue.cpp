/************************************************
*����CircularQueue.cpp
*���ܣ�ѭ������,�̰߳�ȫ
*��Ȩ�����ڽ�֤�Ƽ��ɷ����޹�˾
*ʱ�䣺2013.11.01
*���ߣ������
*�޸����ڣ�
*�޸����ݣ�
**************************************************/
#include <stdlib.h>
#include <errno.h>
#include "CircularQueue.h"

/*********************************************************************
 * ��������:CCircularQueue
 * ˵��:�๹�캯��
 * ��ڲ���:
 *      int   iMaxDepth = 1024,     ���������
 * ����ֵ:
 *	    ��
 * ����: YanYF
 * ʱ�� : 2013-11-01 09:50
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
	//��ʼ����
	InitLock(&m_lock, NULL);
#if defined(WIN32) || defined(WIN64)
	//�����˳��¼�
	m_hExitEvt = ::CreateEvent(NULL, TRUE, FALSE, NULL);
#endif

	//������Ӧ���ź���
	if (m_iMaxDepth > 0 && m_iMaxDepth < MAX_CIRCULAR_QUEUE_DEPTH)
	{
		Init();
	}
}

/*********************************************************************
 * ��������:~CCircularQueue
 * ˵��:����������
 * ��ڲ���:
 *      void
 * ����ֵ:
 *	    ��
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:00
*********************************************************************/
CCircularQueue::~CCircularQueue()
{
	Destroy();
	DestroyLock(&m_lock);
}

/*********************************************************************
 * ��������:Create
 * ˵��:��������
 * ��ڲ���:
 *      int   iMaxDepth,       ��Ҫ�����Ķ������
 * ����ֵ:
 *	    int    0�ɹ�, -1ʧ��
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:03
*********************************************************************/
int   CCircularQueue::Create(int   iMaxDepth)
{
	if (iMaxDepth <= 0 || iMaxDepth > MAX_CIRCULAR_QUEUE_DEPTH)
	{
		return -1;
	}

	Lock(&m_lock);
	//����Ѿ�Create�ˣ�ֱ�ӷ���
	if(m_iMaxDepth)
	{
		UnLock(&m_lock);
		return 0;
	}
	m_iMaxDepth = iMaxDepth;
	//��ʼ��
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
 * ��������:Init
 * ˵��:��ʼ���ź��������пռ�ռ�
 * ��ڲ���:
 *      void
 * ����ֵ:
 *	    int    0�ɹ�, -1ʧ��
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:08
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

	/*����ռ�*/
	/*ѭ������Ҫ������һ���ռ�*/
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
 * ��������:Destroy
 * ˵��:���ٶ���
 * ��ڲ���:
 *      void
 * ����ֵ:
 *	     void
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:14
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
    /*��ն���*/
	EmptyQ();

	/*�����ź���*/
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
 * ��������:EmptyQ
 * ˵��:��ն���
 * ��ڲ���:
 *      void
 * ����ֵ:
 *	     void
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:19
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
 * ��������:GetDepth
 * ˵��:��ȡ�������
 * ��ڲ���:
 *      void
 * ����ֵ:
 *	     int
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:26
*********************************************************************/
int     CCircularQueue::GetDepth()
{
	return (m_iQueueTail - m_iQueueHead + m_iMaxDepth) %m_iMaxDepth;
}

#if defined(WIN32) || defined(WIN64)
#else
/*********************************************************************
 * ��������:TimedWait
 * ˵��:�ȴ�ָ��ʱ��ɹ���ȡ�ź����򷵻�0���򷵻�-1
 * ��ڲ���:
 *       sem_t *pSemaphore, �ź���
 *       int iMillSecond,�ȴ�ʱ��
 * ����ֵ:
 *	  int
 * ����: YanYF
 * ʱ�� : 2012-12-28 17:46
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
	   //��ȡ��ǰʱ��
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
 * ��������:Put
 * ˵��:��������������
 * ��ڲ���:
 *      const void *pData,            ��Ҫ��ӵ�����
 *      int     iDataLen,                ��Ҫ��ӵ����ݳ���
 *      int     iTimeOut,                �ȴ���ʱʱ��
 * ����ֵ:
 *	     QUEUE_OPERATE_RET
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:29
*********************************************************************/
QUEUE_OPERATE_RET    CCircularQueue::Put(const void *pData, int iDataLen, int iTimeOut)
{
	void *pTempData = NULL;
#if defined(WIN32) || defined(WIN64)
	HANDLE   hEvent[2] = {m_hSemEmpty, m_hExitEvt};
	switch(::WaitForMultipleObjects(2, hEvent, FALSE, iTimeOut))
	{
	case WAIT_FAILED:
		/*����WaitForMultipleObjectsʧ��*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//����ȴ�ʧ���򷵻�
	if(-1 == TimedWait(&m_hSemEmpty, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif

	//����ռ������洢����
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
		/*��������*/
		memcpy(pTempData, pData, iDataLen);
	}

	/*�������*/
	Lock(&m_lock);
	m_pQueueSpace[m_iQueueTail].iDataLen = iDataLen;
	m_pQueueSpace[m_iQueueTail].pBuffer = pTempData;
	m_iQueueTail = (++m_iQueueTail) % m_iMaxDepth;
	UnLock(&m_lock);

	/*�����������ź�*/
#if defined(WIN32) || defined(WIN64)
	::ReleaseSemaphore(m_hSemStore, 1, NULL);
#else
	sem_post(&m_hSemStore);
#endif

	return QUEUE_OPERATE_SUCCESS;
}

/*********************************************************************
 * ��������:PutM
 * ˵��:��������������
 * ��ڲ���:
 *      const void *pData,            ��Ҫ��ӵ�����
 *      int     iDataLen,                ��Ҫ��ӵ����ݳ���
 *      int     iTimeOut,                �ȴ���ʱʱ��
 * ����ֵ:
 *	     int
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:40
*********************************************************************/
QUEUE_OPERATE_RET    CCircularQueue::PutM(const void *pData, int iDataLen, int iTimeOut)
{
	void *pTempData = NULL;
#if defined(WIN32) || defined(WIN64)
	HANDLE   hEvent[2] = {m_hSemEmpty, m_hExitEvt};
	switch(::WaitForMultipleObjects(2, hEvent, FALSE, iTimeOut))
	{
	case WAIT_FAILED:
		/*����WaitForMultipleObjectsʧ��*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//����ȴ�ʧ���򷵻�
	if(-1 == TimedWait(&m_hSemEmpty, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif
	/*�������*/
	Lock(&m_lock);
	m_pQueueSpace[m_iQueueTail].iDataLen = iDataLen;
	m_pQueueSpace[m_iQueueTail].pBuffer = (void *)pData;
	m_iQueueTail = (++m_iQueueTail) % m_iMaxDepth;
	UnLock(&m_lock);

	/*�����������ź�*/
#if defined(WIN32) || defined(WIN64)
	::ReleaseSemaphore(m_hSemStore, 1, NULL);
#else
	sem_post(&m_hSemStore);
#endif

	return QUEUE_OPERATE_SUCCESS;
}

/*********************************************************************
 * ��������:Get
 * ˵��:�Ӷ�����ȡ����
 * ��ڲ���:
 *      void **pData,                   ���ȡ��������
 *      int *piDataLen,                 ���ȡ�������ݳ���
 *      int     iTimeOut,                �ȴ���ʱʱ��
 * ����ֵ:
 *	     int
 * ����: YanYF
 * ʱ�� : 2013-11-01 10:53
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
		/*����WaitForMultipleObjectsʧ��*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//����ȴ�ʧ���򷵻�
	if(-1 == TimedWait(&m_hSemStore, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif

	/*������*/
	Lock(&m_lock);
	pTempElem = m_pQueueSpace + m_iQueueHead;
	pTempData = pTempElem->pBuffer;
	pTempElem->pBuffer = NULL;
	*piDataLen = pTempElem->iDataLen;
	pTempElem->iDataLen = 0;
	m_iQueueHead = (++m_iQueueHead) % m_iMaxDepth;
	UnLock(&m_lock);

	/*�����п�λ���ź���*/
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
 * ��������:GetM
 * ˵��:�Ӷ�����ȡ����
 * ��ڲ���:
 *      void **pData,                   ���ȡ��������
 *      int     iDataLen,                ��Ҫ��ӵ����ݳ���
 *      int     iTimeOut,                �ȴ���ʱʱ��
 * ����ֵ:
 *	     int
 * ����: YanYF
 * ʱ�� : 2013-11-01 11:03
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
		/*����WaitForMultipleObjectsʧ��*/
		return QUEUE_OPERATE_ERR_UNKOWN;
	case WAIT_TIMEOUT:
		return QUEUE_OPERATE_ERR_TIME_OUT;
	case (WAIT_OBJECT_0 + 1):
		return QUEUE_OPERATE_ERR_EXIT;
	default:
		break;
	}
#else
	//����ȴ�ʧ���򷵻�
	if(-1 == TimedWait(&m_hSemStore, iTimeOut))
	{
		return QUEUE_OPERATE_ERR_TIME_OUT;
	}
#endif

	/*������*/
	Lock(&m_lock);
	pTempElem = m_pQueueSpace + m_iQueueHead;
	*pData = pTempElem->pBuffer;
	pTempElem->pBuffer = NULL;
	*piDataLen = pTempElem->iDataLen;
	pTempElem->iDataLen = 0;
	m_iQueueHead = (++m_iQueueHead) % m_iMaxDepth;
	UnLock(&m_lock);

	/*�����п�λ���ź���*/
#if defined(WIN32) || defined(WIN64)
	::ReleaseSemaphore(m_hSemEmpty, 1, NULL);
#else
	sem_post(&m_hSemEmpty);
#endif
	return QUEUE_OPERATE_SUCCESS;
}

#if defined(WIN32) || defined(WIN64)
/*********************************************************************
 * ��������:TellGetPutExit
 * ˵��:��֪Get��Put�˳��¼�
 * ��ڲ���:
 *     void
 * ����ֵ:
 *     void
 * �޸���ʷ
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
 * ��������:EmptyQWithoutFree
 * ˵��:���ͷſռ����ն���
 * ��ڲ���:
 *     void
 * ����ֵ:
 *     void
 * ʱ�� : 2014-02-28 17:13
 * �޸���ʷ
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