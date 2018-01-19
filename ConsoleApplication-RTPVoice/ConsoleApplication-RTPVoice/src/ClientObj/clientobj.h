
/*
* myprotocol.h
*
* Created: 2018/01/03
* Author: EDWARDS
*/


#ifndef CLIENTOBJ_H
#define CLIENTOBJ_H

#include "common.h"
#include "myrtp.h"
#include "json.h"
#include "fifoqueue.h"
#include "socketwrap.h"
#include "syninterface.h"
#include <time.h>

#pragma pack(push, 1)
typedef struct{

	int32_t		bytes_remained;
	uint32_t	count;
	int32_t		pro_length;
	char		data[BUFLENGTH];

}StickDismantleOptions_t;

#pragma pack(pop)



class ClientObj
{
public:
	ClientObj(SocketParams_t socketparam, MultiCallBackFuncs_t callbacks);
	~ClientObj();

private:

	/*
	��ʼ����Ա����
	*/
	void InitProtocolData();

	void DataProcessFunc();

	//�ص��ӿ�
	void(*RequestCallBackFunc)(int, ResponeData);//������ص�
	void(*NotifyDeleteCallBackFunc)(SocketParams_t);//����ݻ�֪ͨ��ص�
	void onData(void(*func)(int, ResponeData), int command, ResponeData data);
	void deOBJ(void(*func)(SocketParams_t), SocketParams_t clientsocket);
	
	
	/*
	Э������ճ��/�������
	*/
	int StickDismantleProtocol(HSocket fd, char *buff, int len, StickDismantleOptions_t &ptr);

	/*
	���Э������
	*/
	int PushRecvBuffToQueue(HSocket clientfd, char *buff, int buff_len);

	/*
	�����߳��˳���־
	*/
	void SetThreadExitFlag()   { set_thread_exit_flag = true; }
	bool set_thread_exit_flag;


	void CreateProcessClientReqThread();
	static int ProcessClientReqThread(void* p);
	int ProcessClientReqThreadFunc();

	void CreatProtocolParseThread();
	static int ProtocolParseThread(void *p);
	int ProtocolParseThreadFunc();


	FifoQueue jqueue;//JSON data queue
	//socket��ʼ�������ṹ��
	socketoption_t socketoption;

	PROTOCOL_Ctrlr thePROTOCOL_Ctrlr;//Э��ṹ
	std::map <std::string, int>  statemap;//״̬��
	ILock *ondata_locker;
	ILock *deobj_locker;
	MyCreateThread * process_client_thread_p;
	MyCreateThread * parse_thread_p;
	SocketParams_t clientsocket;

	char recvbuff[BUFLENGTH];
	transresult_t rt;
	StickDismantleOptions_t temp_option;


};




#endif