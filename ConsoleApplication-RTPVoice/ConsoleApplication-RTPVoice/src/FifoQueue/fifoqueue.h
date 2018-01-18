/*
 * FifoQueue.h
 *
 * Created: 2018/01/03
 * Author: EDWARDS
 */ 

#ifndef fifoqueue_h_
#define fifoqueue_h_
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <list>
#include <queue>
#include <stdint.h>
#include "syninterface.h"

class FifoQueue 
{
	
	public:
		FifoQueue();
		~FifoQueue();

	public:
		bool  			PushToQueue(void *packet, int len);
		int32_t 		TakeFromQueue(void *packet, int& len, int waittime =200);
		void			ClearQueue();
		bool 			QueueIsEmpty();

	private:
		//init mutex
		//GOSEM_T m_hSemaphore;
		//std::queue<char *>  m_queue;
		//GOCRITICAL_SECTION cs;
		std::list<char *>  	m_list;
		char fifobuff[20][512];
		volatile  uint32_t fifo_counter;
		ILock	*queuelock;
		ISem	*queuesem;

	
};



#endif