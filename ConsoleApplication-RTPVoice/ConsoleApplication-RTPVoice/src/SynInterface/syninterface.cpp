/*
* SynInterface.cpp
*
* Created: 2017/12/19
* Author: EDWARDS
*/
#include "stdafx.h"
#include "syninterface.h"


CriSection::CriSection()
{
#ifdef WIN32
	InitializeCriticalSection(&m_critclSection);

#else
	m_critclSection = NULL;
	pthread_mutex_init(&m_critclSection, NULL);

#endif

}
CriSection::~CriSection()
{
	
#ifdef WIN32
	DeleteCriticalSection(&m_critclSection);

#else

	pthread_mutex_destroy(&m_critclSection);

#endif

}
void CriSection::Lock()const
{

#ifdef WIN32
	EnterCriticalSection((LPCRITICAL_SECTION)&m_critclSection);

#else

	pthread_mutex_lock(&m_critclSection);

#endif

}
void CriSection::Unlock()const
{

#ifdef WIN32
	LeaveCriticalSection((LPCRITICAL_SECTION)&m_critclSection);

#else

	pthread_mutex_unlock(&m_critclSection);

#endif

}



Mutex::Mutex(const char * lockUniName)
:m_mutex(NULL)
{

#ifdef WIN32
	//lpName第三个参数用来设置互斥量的名称，在多个进程中的线程就是通过名称来确保它们访问的是同一个互斥量
	m_mutex = CreateMutex(nullptr, FALSE, (LPCTSTR)lockUniName);
#else
	pthread_mutex_init(&m_mutex, NULL);

#endif

}
Mutex::~Mutex()
{
#ifdef WIN32
	CloseHandle(m_mutex);

#else

	pthread_mutex_destroy(&m_mutex);

#endif
}
void Mutex::Lock()const
{
#ifdef WIN32
	WaitForSingleObject(m_mutex, INFINITE);

#else

	pthread_mutex_lock(&m_mutex);

#endif

}
void Mutex::Unlock()const
{
#ifdef WIN32
	ReleaseMutex(m_mutex);

#else

	pthread_mutex_unlock(&m_mutex);

#endif

}

MySynSem::MySynSem()
:m_sem(NULL)
{
#ifdef WIN32
	m_sem = CreateSemaphore(NULL, 0, 100, NULL);
#else
	sem_init(&m_sem, 0, 0);

#endif

}
MySynSem::~MySynSem()
{
#ifdef WIN32
	CloseHandle(m_sem);
#else
	sem_destroy(&m_sem);

#endif	

}
int MySynSem::SemWait(int waittime)const
{
	int ret = 0;

#ifdef WIN32
	ret = WaitForSingleObject(m_sem, waittime);//等待信号量触发,waittime:/ms
	if ((ret == WAIT_ABANDONED) || (ret == WAIT_FAILED))
	{
		ret = -1;
	}

#else
	struct timespec ts;
	ts.tv_nsec = waittime*1000*1000;
	ts.tv_sec   = 0;
	ret=sem_timedwait(&m_sem, &ts);
	if ((ret < 0) && (errno == ETIMEDOUT))
	{
		ret = 1;
	}
	else if (ret < 0)
	{
		ret = -1;
	}
	else
#endif	

	return ret;
}
void MySynSem::SemPost()const
{
#ifdef WIN32
	ReleaseSemaphore(m_sem, 1, NULL);//触发信号量
#else
	sem_post(&m_sem);

#endif	


}

MySynCond::MySynCond()
:m_condlock(NULL)
, m_cond(NULL)
{
	m_condlock = new Mutex("condlock");

#ifdef WIN32
	m_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
#else

	pthread_cond_init(&m_cond, NULL);

#endif



}
MySynCond::~MySynCond()
{
	if (m_condlock != NULL)
	{
		delete m_condlock;
		m_condlock = NULL;
	}

#ifdef WIN32
		
	CloseHandle(m_cond);
#else
	pthread_cond_destroy(%m_cond);

#endif


}
int MySynCond::CondWait(int waittime)const
{
	int ret = 0;

#ifdef WIN32
	ret = WaitForSingleObject(m_cond, waittime);//等待信号量触发,waittime:/ms
	if ((ret == WAIT_ABANDONED) || (ret == WAIT_FAILED))
	{
		ret = -1;
	}

#else

	m_condlock->Lock();
	ret = pthread_cond_wait(&m_cond, &(m_condlock->GetMutex()));//会意外苏醒，注意考虑此情况的处理机制
	m_condlock->Unlock();

#endif

	return ret;

}
void MySynCond::CondTrigger(bool isbroadcast)const
{

#ifdef WIN32

	SetEvent(m_cond);

#else

	m_condlock->Lock();
	if (isbroadcast)
		pthread_cond_broadcast(&m_cond);
	else
	{
		pthread_cond_signal(&m_cond);
	}
	m_condlock->Unlock();

#endif



}

#ifdef WIN32
MyCreateThread::MyCreateThread(void *func, void *ptr)
:thread_handle(NULL)
{
	thread_handle = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void *))func, ptr, 0, NULL);
	if (thread_handle == NULL)
	{
		std::cout << "create thread failed" << std::endl;
		system("pause");
	}
}
#else

MyCreateThread::MyCreateThread(pthread_t  *thread, void *func, void *ptr)
:thread_handle(NULL)
{
	int err =0;
	err = pthread_create(&thread_handle, NULL, func, ptr);
	if (err != 0){
		fprintf(stderr, "func create fail...\n");
	}
	//pthread_detach(thread_handle);//分离创建的线程，线程退出后资源自动回收
}

#endif

MyCreateThread::~MyCreateThread()
{

#ifdef WIN32
	WaitForSingleObject(thread_handle, INFINITE);

	CloseHandle(thread_handle);
#else
	pthread_join(thread_handle);//等待回收线程

#endif

	thread_handle = NULL;


}