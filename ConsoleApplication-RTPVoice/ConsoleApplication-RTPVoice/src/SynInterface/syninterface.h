/*
* SynInterface.h
*
* Created: 2017/12/19
* Author: EDWARDS
*/
#ifndef _SYNINTERFACE_H
#define _SYNINTERFACE_H

#include "common.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h> 
#endif

//
typedef enum {
	SYN_FAILED		=-2,
	SYN_ABANDONED	=-1,
	SYN_OBJECT_o	=0,
	SYN_TIMEOUT		=1
} SynInterface_States;

//���ӿ���  
class ILock
{
public:
	virtual ~ILock() {}

	virtual void Lock() const = 0;
	virtual void Unlock() const = 0;
};

//�ź����ӿ��� 
class ISem
{
public:
	virtual ~ISem() {}

	virtual int SemWait(int waittime) const = 0;
	virtual void SemPost() const = 0;
};

//�¼�/�����ź����ӿ��� 
class ICond
{
public:
	virtual ~ICond() {}

	virtual int CondWait(int waittime) const = 0;
	virtual void CondTrigger(bool isbroadcast) const = 0;
};




//�ٽ�������:�ٶȿ죬������ԴС 
class CriSection : public ILock
{
public:
	CriSection();
	~CriSection();

	virtual void Lock() const;
	virtual void Unlock() const;

private:
	GOCRITICAL_SECTION m_critclSection;
};

//�������ࣺ����ʵ�ֲ�ͬ�������߳�֮�以��
class Mutex : public ILock
{
public:
	Mutex(const char * lockUniName);
	~Mutex();

	virtual void Lock() const;
	virtual void Unlock() const;

	GOMUTEX_T GetMutex(){ return m_mutex; }

private:
	GOMUTEX_T m_mutex;
};

//ͬ���ź���
class MySynSem : public ISem
{
public:
	MySynSem();
	~MySynSem();

	virtual int SemWait(int waittime) const;
	virtual void SemPost() const;

private:
	GOSEM_T m_sem;

};

//�����ź���/�¼�
class MySynCond : public ICond
{
public:
	MySynCond ();
	~MySynCond ();

	virtual int CondWait(int waittime) const;
	virtual void CondTrigger(bool isbroadcast) const;


private:
	Mutex *m_condlock;
	GOCOND_T m_cond;
	 

};

//�߳̽ӿ���
class MyCreateThread
{
public:

#ifdef WIN32
	/*
	�����߳�
	*/
	MyCreateThread(void *func, void *ptr);

	HANDLE GetThreadHandle(){ return thread_handle; }

#else

	MyCreateThread(pthread_t  *thread, void *func, void *ptr);
	pthread_t GetThreadHandle(){ return thread_handle; }

#endif
	/*
	�����߳�
	*/
	~MyCreateThread();



private:

#ifdef WIN32
	HANDLE thread_handle;
#else

	pthread_t thread_handle;

#endif



};




#endif