/*
 * FifoQueue.c
 *
 * Created: 2016/12/19
 * Author: EDWARDS
 */ 
#include "stdafx.h"
#include "fifoqueue.h"

FifoQueue::FifoQueue()
:queuelock(NULL)
, queuesem(NULL)
,fifo_counter(0)
{
	
	//m_hSemaphore = CreateSemaphore(NULL, 0, 100, NULL);
	//InitializeCriticalSection(&cs);
#ifdef WIN32
	queuelock = new CriSection();
#else
	queuelock = new Mutex((const char *)"queuelocker");
#endif

	queuesem = new MySynSem();
	for (int i = 0; i < 20; i++){
		memset(&fifobuff[i][0], 0x00, 512);
	}

}
FifoQueue::~FifoQueue()
{
	if (queuelock != NULL)
	{
		delete queuelock;
		queuelock = NULL;
	}
	if (queuesem != NULL)
	{
		delete queuesem;
		queuesem = NULL;
	}

	//CloseHandle(m_hSemaphore);
	//DeleteCriticalSection(&cs);
	
}

void FifoQueue::ClearQueue()
{
	//EnterCriticalSection(&cs);
	queuelock->Lock();
	m_list.clear();
	queuelock->Unlock();
	///LeaveCriticalSection(&cs);

}


bool  FifoQueue::QueueIsEmpty()
{
	return(m_list.empty());
}
bool FifoQueue::PushToQueue(void *packet, int len)
{
	int err = 0;
	static int counter = 0;

	//EnterCriticalSection(&cs);
	queuelock->Lock();

	memcpy(&fifobuff[fifo_counter][0], packet, len);
	m_list.push_back(&fifobuff[fifo_counter][0]);//将一个数据包地址插入链表尾
	fifo_counter++;
	if (fifo_counter > 19){
		fifo_counter = 0;
	}

	queuelock->Unlock();

	queuesem->SemPost();
	//LeaveCriticalSection(&cs);

	//ReleaseSemaphore(m_hSemaphore, 1, NULL);//触发信号量
	return true;

}

int32_t FifoQueue::TakeFromQueue(void *packet, int& len, int waittime)
{

	char* sBuffer = NULL;

	//DWORD ret = WaitForSingleObject(m_hSemaphore, waittime);//等待信号量触发
	//if (ret != WAIT_OBJECT_0)
	//{
	//	return ret;
	//}

	int ret = queuesem->SemWait(waittime);
	if ((ret < 0) || (ret >= 1))
	{
		return ret;//timeout or failed
	}
	if (!m_list.empty())
	{
			//EnterCriticalSection(&cs);
			queuelock->Lock();

			sBuffer = m_list.front();//返回链表第一个数据包地址给sbuffer
			m_list.pop_front();//并删除链表第一个数据包地址
			memcpy(packet, sBuffer, 512);

			queuelock->Unlock();
			//LeaveCriticalSection(&cs);
			len = 512;
			return ret;
	}
	else
	{
		return -2;//no happen but must check

	}


}



