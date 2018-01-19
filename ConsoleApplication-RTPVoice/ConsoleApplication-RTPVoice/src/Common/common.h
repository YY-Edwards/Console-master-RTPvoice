#ifndef COMMON_H
#define COMMON_H

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h> 
#include <errno.h> 
#include <fcntl.h>

#else
#include <winsock2.h>
#include <process.h>
#endif // WIN32
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <stdbool.h>
#include <stdint.h>
#include <sstream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

#define                     WIN_RUNNING_PLATFORM 
//#define                     LINUX_RUNNING_PLATFORM 

#define						CHANNEL1RTPBASEPORT						55500
#define						CHANNEL1RTPDESTPORT						57400
#define						CHANNEL1RTPSSRC							1973

#define						CHANNEL2RTPBASEPORT						55600
#define						CHANNEL2RTPDESTPORT						57500
#define						CHANNEL2RTPSSRC							2012

#define						TCPPORT									57000
#define						BUFLENGTH								2048

#define						PROTOCOL_HEAD							0x01
#define						PROTOCOL_PACKAGE_LENGTH_SIZE			4

#define						MAX_LISTENING_COUNT						65535
#define						TIMEOUT_VALUE							10*1000

#ifdef WIN32
typedef  CRITICAL_SECTION GOCRITICAL_SECTION;
typedef  HANDLE GOMUTEX_T;
typedef  HANDLE GOSEM_T;
typedef  HANDLE GOCOND_T;
typedef  HANDLE GOTHREAD_T;
typedef  SOCKET HSocket;
//#define GOTHREADCREATE(x, y, z, q, w, e) _beginthreadex((x), (y), (z), (q), (w), (e))


#else
//#define GOTHREADCREATE(x, y, z, q, w, e) pthread_create((x), (y), (z), (q))
typedef  int GOTHREAD_T;
typedef  pthread_mutex_t	GOMUTEX_T;
typedef  sem_t				GOSEM_T;
typedef  pthread_cond_t		GOCOND_T;
typedef int HSocket;
#define SOCKET_ERROR  (-1)  
#define INVALID_SOCKET  0  

#endif


#pragma pack(1)


typedef struct SocketParams_t{

	HSocket			socket_fd;
	sockaddr_in		remote_addr;

	bool operator<(SocketParams_t const&_A) const
	{
		//这个函数指定排序策略，按socket_fd排序，如果socket_fd相等的话，按remote_addr.sin_addr排序
		if (socket_fd < _A.socket_fd)return true;
		if (socket_fd == _A.socket_fd)
		{
			if (remote_addr.sin_port < _A.remote_addr.sin_port)
				return true;
			else
				return false;
		}
		return false;


	}

}SocketParams_t;


struct ResponeData
{
	HSocket socket_fd;
	std::string identifier;
	//std::string type;
	//std::string name;
	std::string key;

	std::string channel_id;
	int16_t channel1_group_id;
	int16_t channel2_group_id;

	int32_t src_id;
	int32_t dst_id;
	
	std::string  status;
	std::string  reason;

};

typedef struct{

	void(*RequestCallBackFunc)(int, ResponeData);
	void(*ExitNotifyCallBackFunc)(SocketParams_t);

}MultiCallBackFuncs_t;

struct ResponeRTPData
{
	int32_t payloaddatalength;
	uint32_t ssrc;
	uint8_t *payloaddata;
};


//PROTOCOL State Master.
typedef enum {
	PROTOCOL_UNCONNECTEDWAITINGSTATUS,
	PROTOCOL_UNCONNECTEDWAITINGSETLISTENING,
	PROTOCOL_CONNECTED
} MASTER_States;


//PROTOCOL Type Master.
typedef enum {
	REQUEST,
	REPLY,
	NOTIFY
} PROTOCOL_Types;


//PROTOCOL name Master.
typedef enum {
	CONNECT,
	LISTENING,
	QUERY,
	CALLREQUEST,
	CALLRELEASE,
	CALLSTART,
	CALLEND

} PROTOCOL_Names;


/*connect struct*/
//request
typedef struct{

	std::string key;

}Connect_Request_Params_t;
//reply
typedef struct{

	std::string status;
	std::string reason;

}Connect_Reply_Params_t;


/*listening struct*/
//request
typedef struct{

	uint32_t channel1_group_id;
	uint32_t channel2_group_id;

}Listening_Channels_Groups_t;
typedef struct{

	Listening_Channels_Groups_t Listening_Channels_Group;

}Listening_Request_Params_t;

//reply
typedef struct{

	std::string status;
	std::string reason;
	uint32_t listening_group_id;

}Listening_Reply_Params_Channels_Params_t;

typedef struct{

	Listening_Reply_Params_Channels_Params_t channel1;
	Listening_Reply_Params_Channels_Params_t channel2;

}Listening_Reply_Params_Channels_t;

typedef struct{

	Listening_Reply_Params_Channels_t Listening_Reply_Params_Channels;
}Listening_Reply_Params_t;



/*query struct*/
//reply
typedef struct{

	std::string status;
	std::string reason;
	Listening_Channels_Groups_t Listening_Channels_Groups;

}Query_Request_Params_t;


/*callrequest struct*/
//request
typedef struct{

	uint32_t src;
	uint32_t dst;
	std::string channel_id;

}CallRequest_Request_Params_t;
//reply
typedef struct{

	std::string status;
	std::string reason;

}CallRequest_Reply_Params_t;

/*callrelease struct*/
//request
typedef struct{

	uint32_t src;
	uint32_t dst;
	std::string channel_id;

}CallRelease_Request_Params_t;

//reply
typedef struct{

	std::string status;
	std::string reason;

}CallRelease_Reply_Params_t;



/*callstart struct*/
//Notify
typedef struct{

	uint32_t src;
	uint32_t dst;
	std::string channel_id;

}CallStart_Notify_Params_t;



/*callend struct*/
//Notify
typedef struct{

	uint32_t src;
	uint32_t dst;
	std::string channel_id;

}CallEnd_Notify_Params_t;



typedef struct{

	Listening_Reply_Params_Channels_Params_t channel1;
	Listening_Reply_Params_Channels_Params_t channel2;

}Listening_Params_Channels_Params_t;

//PROTOCOL Params(All.)
typedef struct{

	std::string key;
	std::string status;
	std::string reason;
	uint32_t src;
	uint32_t dst;
	std::string channel; 
	Listening_Channels_Groups_t Listening_Channels_Group;
	Listening_Params_Channels_Params_t Listening_Params_Channels_Params;

}PROTOCOL_Params;


typedef struct{

	std::string identifier;
	std::string type;
	std::string name;

}PROTOCOL_Fixed_Header_t;

typedef struct{

	HSocket clientfd;//接收数据包的socket描述符，以便于socket发送时时使
	PROTOCOL_Names MASTER_State;
	//MASTER_States MASTER_State;
	PROTOCOL_Fixed_Header_t PROTOCOL_Fixed_Header;
	//PROTOCOL_Types PROTOCOL_Type;
	//PROTOCOL_Names PROTOCOL_Name;
	PROTOCOL_Params PROTOCOL_params;
		 
}PROTOCOL_Ctrlr;


typedef struct{

	char head;
	char payload_len[4];
	char json_payload[1019];

}transport_protocol_t;


typedef union 
{
	transport_protocol_t transport_protocol_fragment;
	char fragment_element[1024];

}phy_fragment_t;


#pragma pack()

/**
#pragma pack(1)
typedef struct{

	MASTER_States MASTER_State;
	char identifier[20];
	char type[20];
	char name[20];

	char key[128];
	char status[10];
	char reason[128];
	uint32_t src;
	uint32_t dst;
	char channel[10];

	Listening_Channels Listening_channles;

	char channel1_param_status[10];
	char channel1_param_reason[128];
	uint32_t channel1_param_value;

	char channel2_param_status[10];
	char channel2_param_reason[128];
	uint32_t channel2_param_value;

}DATA_Ctrlr;
#pragma pack()
**/


#endif