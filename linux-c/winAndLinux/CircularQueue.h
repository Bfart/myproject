/************************************************
*����CircularQueue.h
*���ܣ�ѭ������,�̰߳�ȫ
*��Ȩ�����ڽ�֤�Ƽ��ɷ����޹�˾
*ʱ�䣺2013.11.01
*���ߣ������
*�޸����ڣ�
*�޸����ݣ�
**************************************************/

#ifndef CIRCULAR_QUEUE_H_H
#define CIRCULAR_QUEUE_H_H
#include <stdio.h>
#include <string.h>

#if defined(WIN32) || defined(WIN64)
	#include <windows.h>

	/*����һЩ�꺯��*/
	#ifndef snprintf
	    #define snprintf        _snprintf
	#endif

	#ifndef strcasecmp
	    #define strcasecmp   stricmp
	#endif

	/*��������صĺ�*/
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

	/*��������صĺ�*/
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

	/*��������*/
	int      Create(int   iMaxDepth);

	/*���ٶ���*/
	void    Destroy();

	/*��ն���*/
	void   EmptyQ();

	/*���ͷſռ����ն���*/
	void   EmptyQWithoutFree();

	/*��ȡ�������*/
	int     GetDepth();

	/*��������������*/
	QUEUE_OPERATE_RET    Put(const void *pData, int iDataLen, int iTimeOut);
	QUEUE_OPERATE_RET    PutM(const void *pData, int iDataLen, int iTimeOut);

	/*�Ӷ�����ȡ����*/
	QUEUE_OPERATE_RET    Get(void **pData, int *piDataLen, int iTimeOut);
	QUEUE_OPERATE_RET    GetM(void **pData, int *piDataLen, int iTimeOut);
#if defined(WIN32) || defined(WIN64)
	void   TellGetPutExit();     /*��֪Get��Put�˳��¼�*/
#endif

private:
	/*������е�Ԫ��*/
	typedef struct tagQUEUE_ELEMENT
	{
		void         *pBuffer;                     /*Ԫ���������洢������*/
		int             iDataLen;                  /*�洢�����ݳ���*/
	}QUEUE_ELEMENT, *LPQUEUE_ELEMENT;

	LPQUEUE_ELEMENT                m_pQueueSpace;         /*�洢����Ԫ��*/

	int                                       m_iMaxDepth;             /*����������*/
	int                                       m_iQueueHead;           /*����ͷ��*/
	int                                       m_iQueueTail;              /*����β��*/

	lock_t                                  m_lock;                       /*�ٽ���*/
#if defined(WIN32) || defined(WIN64)
	HANDLE      m_hSemEmpty;   /*��put���ź���*/
	HANDLE      m_hSemStore;    /*��get���ź���*/
	HANDLE      m_hExitEvt;       /*�˳��¼�*/
#else
	sem_t      m_hSemEmpty;      //��put���ź���
	sem_t      m_hSemStore;      //��get���ź���
#endif


private:
	/*��ʼ���ź��������пռ�ռ�*/
	int     Init();
	/*���ο������캯��*/
	CCircularQueue(const CCircularQueue &);
	CCircularQueue & operator=(const CCircularQueue &);
};

#endif