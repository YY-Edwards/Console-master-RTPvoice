/*
* myprotocol.cpp
*
* Created: 2018/01/03
* Author: EDWARDS
*/


#include "stdafx.h"
#include "myprotocol.h"

JProtocol *JProtocol::pThis = NULL;

JProtocol::JProtocol()
{
	 InitProtocolData();
}

JProtocol::~JProtocol()
{
	CloseMater();
	SetThreadExitFlag();//通知线程退出

	if (recovery_thread_p != NULL)
	{
		delete recovery_thread_p;
		recovery_thread_p = NULL;
	}

	for (int i = 0; i < MAX_LISTENING_COUNT; i++)
	{
		if (listen_thread_p[i] != NULL)
		{
			delete listen_thread_p[i];
			listen_thread_p[i] = NULL;
		}

	}

	std::map<SocketParams_t, ClientObj *> ::iterator it;
	while (clientmap.size()>0)
	{
		it = clientmap.begin();
		if (it->second != NULL)
		{
			delete it->second;
			it->second = NULL;
		}
		clientmap.erase(it);
	}

	if (clientmap_locker != NULL)
	{
		delete clientmap_locker;
		clientmap_locker = NULL;
	}

	FreeSocketEnvironment();
	std::cout<<"Destory: JProtocol \n"<<std::endl;

}

void JProtocol::InitProtocolData()
{
	pThis = this;

	InitializeSocketEnvironment();

	mytcp_server = NULL;
	//socketoption.recvtimeout = TIMEOUT_VALUE;//10s
	//socketoption.sendtimeout = TIMEOUT_VALUE;//10s
	//socketoption.lingertimeout = TIMEOUT_VALUE;
	//socketoption.block = TRUE;

	startfunc_is_finished = false;
	set_thread_exit_flag = false;

	socketopen = false;
	serversoc = INVALID_SOCKET;
	listen_numb = 0;
	recovery_thread_p = NULL;
	memset(listen_thread_p, 0, MAX_LISTENING_COUNT*sizeof(MyCreateThread *));//注意大小问题
	//memset(clientobjarray, 0, MAX_LISTENING_COUNT*sizeof(ClientObj *));//注意大小问题
	//clientobj_p = NULL;
	//parse_thread_p = NULL;

	multicallbackfuncs.RequestCallBackFunc = NULL;
	multicallbackfuncs.ExitNotifyCallBackFunc = NotifyRecoveryClienObj;//通知，回调接口
	clientmap_locker = new CriSection();

	//ondata_locker = new CriSection();
	//stickdismantle_locker = new CriSection();

	////inset states
	//statemap.insert(std::pair<string, int>("Connect", CONNECT));
	//statemap.insert(std::pair<string, int>("Listening", LISTENING));
	//statemap.insert(std::pair<string, int>("Query", QUERY));
	//statemap.insert(std::pair<string, int>("CallRequest", CALLREQUEST));
	//statemap.insert(std::pair<string, int>("CallRelease", CALLRELEASE));
	//statemap.insert(std::pair<string, int>("CallStart", CALLSTART));
	//statemap.insert(std::pair<string, int>("CallEND", CALLEND));

	////init default protocol data
	//thePROTOCOL_Ctrlr.clientfd = INVALID_SOCKET;
	//thePROTOCOL_Ctrlr.MASTER_State = CONNECT;
	//thePROTOCOL_Ctrlr.PROTOCOL_Fixed_Header.identifier = "";
	//thePROTOCOL_Ctrlr.PROTOCOL_Fixed_Header.type = "Request";
	//thePROTOCOL_Ctrlr.PROTOCOL_Fixed_Header.name = "Connect";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.key = "";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.status = "fail";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.reason = "";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Channels_Group.channel1_group_id = 0;
	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Channels_Group.channel2_group_id = 0;


	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Params_Channels_Params.channel1.reason = "timeout";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Params_Channels_Params.channel1.status = "fail";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Params_Channels_Params.channel1.listening_group_id = 0;
	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Params_Channels_Params.channel2.reason = "timeout";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Params_Channels_Params.channel2.status = "fail";
	//thePROTOCOL_Ctrlr.PROTOCOL_params.Listening_Params_Channels_Params.channel2.listening_group_id = 0;

	//thePROTOCOL_Ctrlr.PROTOCOL_params.src = 9;
	//thePROTOCOL_Ctrlr.PROTOCOL_params.dst = 65536;
	//thePROTOCOL_Ctrlr.PROTOCOL_params.channel = "";

}
void JProtocol::CloseMater()
{
	if (serversoc != INVALID_SOCKET)
	{
		serversoc = INVALID_SOCKET;
		delete mytcp_server;
		mytcp_server = NULL;
	}

	std::cout<<"Close Server\n"<<std::endl;

}
void JProtocol::SetCallBackFunc(void(*callBackFunc)(int, ResponeData))
{
	//RequestCallBackFunc = callBackFunc;//回调设置
	multicallbackfuncs.RequestCallBackFunc = callBackFunc;
}
void JProtocol::Start()
{
	bool status = false;
	status = InitSocket();
	if (status != true)
	{
		std::cout<<"InitSocket fail...\n"<<std::endl;
	}
	else
	{
		startfunc_is_finished = status;
	}

}
bool JProtocol::InitSocket()
{
	if (mytcp_server != NULL)
	{
		mytcp_server->Reopen(TRUE);
	}
	else
	{
		mytcp_server = new CSockWrap(SOCK_STREAM);
	}
	serversoc = mytcp_server->GetHandle();//获取服务端描述符
	GetAddressFrom(&my_addr, 0, TCPPORT);//本地任意IP
	if (SocketBind(serversoc, &my_addr) == SOCKET_ERROR)
	{
		delete mytcp_server;
		mytcp_server = NULL;
		serversoc = INVALID_SOCKET;
		return false;
	}
	if (SocketListen(serversoc, 100) != 0)
	{
		delete mytcp_server;
		mytcp_server = NULL;
		serversoc = INVALID_SOCKET;
		return false;
	}


	/*
	if (socketopen)
	{
		CloseSocket(serversoc);
	}

	if ((serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0) //create a tcp socket  
	{
		std::cout<<"Create socket fail!\n"<<std::endl;
		//printf("Create socket fail!\n");
		return false;
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(TCPPORT);
	my_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	//将本地地址绑定到所创建的套接字上
	if (bind(serversoc, (LPSOCKADDR)&my_addr, sizeof(my_addr)) == SOCKET_ERROR)
	{
		CloseSocket(serversoc);
		return false;
	}

	//start listen
	if (listen(serversoc, 5) != 0)
	{
		CloseSocket(serversoc);
		return false;
	}
	socketopen = true;

	*/

	//CreatDataProcessThread();
	//CreatProtocolParseThread();
	CreateListenThread();
	CreateRecoveryClientObjThread();
	//if (recovery_thread_p == NULL)
	//	CreateRecoveryClientThread();

	/*socketopen = true;*/

	return true;

}
void JProtocol::CreateRecoveryClientObjThread()
{
	recovery_thread_p = new MyCreateThread(RecoveryClientObjThread, this);

}
void JProtocol::CreateListenThread()
{
	if (listen_numb < MAX_LISTENING_COUNT)
	{
		listen_thread_p[listen_numb++] = new MyCreateThread(ListenThread, this);
	}

}
int JProtocol::ListenThread(void* p)
{
	JProtocol *arg = (JProtocol*)p;
	int return_value = 0;
	if (arg != NULL)
	{
		return_value = arg->ListenThreadFunc();
	}
	return return_value;
}
int JProtocol::ListenThreadFunc()
{
	int return_value = 0;
	SocketParams_t socketparam;
	socketparam.socket_fd = INVALID_SOCKET;

	std::cout<<"The server is waiting for a newest connection \n"<<std::endl;
	socketparam.socket_fd = SocketAccept(serversoc, (sockaddr_in*)&(socketparam.remote_addr));

	if (serversoc != INVALID_SOCKET)CreateListenThread();//服务器运行中则继续监听

	if (socketparam.socket_fd == INVALID_SOCKET)
	{
		return_value = -1;
		std::cout << "Accept fail! and exit ListenThreadFunc: 0x" << hex << GetCurrentThreadId ()<< std::endl;
		//TRACE(("Accept fail! and exit listenthread: 0x%x\n"), GetCurrentThreadId());
	}
	else
	{

		if (listen_numb < MAX_LISTENING_COUNT)
		{
			ClientObj *clientobj_p = new ClientObj(socketparam, multicallbackfuncs);

			clientmap_locker->Lock();
			clientmap[socketparam] = clientobj_p;
			//clientmap.insert(std::pair<SocketParams_t, ClientObj *>(socketparam, clientobj_p));//save client info(socketfd,ip,port...) to map
			clientmap_locker->Unlock();
		}
	
		std::cout << "Create ClientObj finished and exit ListenThreadFunc: 0x"<< hex << GetCurrentThreadId() << std::endl;

	}

	return return_value;

}
int JProtocol::RecoveryClientObjThread(void* p)
{
	JProtocol *arg = (JProtocol*)p;
	int return_value = 0;
	if (arg != NULL)
	{
		return_value = arg->RecoveryClientObjThreadFunc();
	}
	return return_value;
}
int JProtocol::RecoveryClientObjThreadFunc()
{
	int return_value = 0;
	int len = 0;
	char temp[512];
	memset(temp, 0x00, 512);
	SocketParams_t clientsocket_temp;
	std::map<SocketParams_t, ClientObj *> ::iterator it;
	int ret = 0;

	while ((ret = clientqueue.TakeFromQueue(temp, len, 200)) >= 0)
	{
		if (set_thread_exit_flag)break;
		if (ret == SYN_OBJECT_o)
		{
			memcpy(&clientsocket_temp, temp, sizeof(SocketParams_t));
			clientmap_locker->Lock();

			it = clientmap.find(clientsocket_temp);
			if (it != clientmap.end())
			{
				delete it->second;
				it->second = NULL;
				it = clientmap.erase(it);
				std::cout << "#clientmap.erase a client# \n" << std::endl;
			}
			else
			{
				std::cout << "#clientmap no find# \n" << std::endl;
			}
			memset(temp, 0x00, 512);
			clientmap_locker->Unlock();
		}
		else
		{
			//timeout
		}
	}

	std::cout << "exit RecoveryClientThreadFunc: 0x" << hex << GetCurrentThreadId() << std::endl;
	return return_value;

}
void JProtocol::NotifyRecoveryClienObj(SocketParams_t clientsocket)
{
	if (pThis == NULL)exit(-1);
	pThis->NotifyDeleleClienObjFunc(clientsocket);

}
void JProtocol::NotifyDeleleClienObjFunc(SocketParams_t clientsocket)
{

	clientqueue.PushToQueue(&clientsocket, sizeof(SocketParams_t));

	std::cout << "#push clientqueue# \n" << std::endl;
}
bool JProtocol::CloseSocket(HSocket sockfd)
{
	closesocket(sockfd);
	return true;

}
int JProtocol::PhySocketSendData(HSocket Objsoc, char *buff, int send_len)
{
	int count = 0;
	transresult_t rt; 
	rt.nbytes = 0;
	rt.nresult = 0;

	do
	{
		SocketSend(Objsoc, &buff[count], (send_len - count), rt);
		if ((rt.nbytes > 0))
		{
			//TRACE(("send length is %d\n"), rt.nbytes);
			count += rt.nbytes;
		}
		else if ((rt.nbytes == -1) && (rt.nresult == 1))
		{
			std::cout<<"SocketSend Timeout\n"<<std::endl;

		}
		else if ((rt.nbytes == -1) && (rt.nresult == -1))
		{
			std::cout<<"Client close socket\n"<<std::endl;
			count = -1;
			return count;
		}


	} while ((send_len - count) != 0);

	count = 0;
	return count;


}
int JProtocol::SendDataToTheThirdParty(HSocket fd, std::string buff)
{
	int return_value = 0;
	int send_len = 0;
	HSocket Objsoc = 0;
	//std::map<HSocket, struct sockaddr_in> ::iterator it;
	std::map<SocketParams_t, ClientObj *> ::iterator it;

	stringstream ss;
	ss<< buff.size();
	Objsoc = fd;

	phy_fragment_t phy_fragment;
	memset(phy_fragment.fragment_element, 0x00, sizeof(phy_fragment));

	phy_fragment.transport_protocol_fragment.head = PROTOCOL_HEAD;

	if (buff.size() < 1000)
	{
		phy_fragment.transport_protocol_fragment.payload_len[0] = '0';
		memcpy(&(phy_fragment.transport_protocol_fragment.payload_len[1]), ss.str().c_str(), PROTOCOL_PACKAGE_LENGTH_SIZE - 1);
		//sprintf_s如下操作即可
		//sprintf_s(&(phy_fragment.transport_protocol_fragment.payload_len[1]), 10,  "%d", buff.size());
	}
	else
	{
		memcpy(&(phy_fragment.transport_protocol_fragment.payload_len[0]), ss.str().c_str(), PROTOCOL_PACKAGE_LENGTH_SIZE);
		//sprintf_s(&(phy_fragment.transport_protocol_fragment.payload_len[0]), 10,  "%d", buff.size());

	}
	
	memcpy(phy_fragment.transport_protocol_fragment.json_payload, buff.c_str(), buff.size());
	send_len = strlen(phy_fragment.fragment_element);

	if (Objsoc != INVALID_SOCKET)
	{
		if (Objsoc == 0)
		{
			clientmap_locker->Lock();
			for (it = clientmap.begin(); it != clientmap.end(); ++it)
			{
				Objsoc = (it->first).socket_fd;
				return_value = PhySocketSendData(Objsoc, phy_fragment.fragment_element, send_len);
			}
			clientmap_locker->Unlock();

		}
		else
		{
			return_value = PhySocketSendData(Objsoc, phy_fragment.fragment_element, send_len);
		}

	}
	else
	{
		std::cout<<"socket is empty!!!\n"<<std::endl;
	}

	Objsoc = INVALID_SOCKET;
	return return_value;
}
void JProtocol::ConnectReply(HSocket dst_fd, std::string status, std::string reason)
{
	
	Json::Value send_root;
	Json::Value send_arrayObj;
	Json::Value send_item;
	Json::StyledWriter style_write;

	send_item["status"] = status;
	send_item["reason"] = reason;
	send_arrayObj.append(send_item);

	send_root["identifier"] = CreateGuid();//"2017010915420322";
	send_root["type"] = "Reply";
	send_root["name"] = "Connect";

	send_root["param"] = send_arrayObj;

	send_root.toStyledString();//build json data

	std::string SendBuf = style_write.write(send_root);

	SendDataToTheThirdParty(dst_fd, SendBuf);
	//SendBuf.clear();
	//send_root.clear();
	//send_arrayObj.clear();
	//send_item.clear();
	std::cout<<"Send ConnectReply\n"<<std::endl;

}
void JProtocol::ConfigReply(HSocket dst_fd, int channel1_value, int channel2_value)
{
	Json::Value send_root;
	Json::Value send_arrayObj1;
	Json::Value send_arrayObj2;
	Json::Value send_arrayObj3;
	Json::Value send_item1;
	Json::Value send_item2;
	Json::Value send_item3;
	Json::StyledWriter style_write;

	if (channel1_value != 0)
	{
		send_item1["status"] = "success";
		send_item1["reason"] = "";
	}
	else
	{
		send_item1["status"] = "fail";
		send_item1["reason"] = "unset";
	}
	send_item1["value"] = channel1_value;
	send_arrayObj1.append(send_item1);

	if (channel2_value != 0)
	{
		send_item2["status"] = "success";
		send_item2["reason"] = "";
	}
	else
	{
		send_item2["status"] = "fail";
		send_item2["reason"] = "unset";
	}
	send_item2["value"] = channel2_value;
	send_arrayObj2.append(send_item2);

	send_item3["channel1"] = send_arrayObj1;
	send_item3["channel2"] = send_arrayObj2;
	send_arrayObj3.append(send_item3);


	send_root["identifier"] = CreateGuid();
	send_root["type"] = "Reply";
	send_root["name"] = "Listening";
	send_root["param"] = send_arrayObj3;

	send_root.toStyledString();//build json data

	std::string SendBuf = style_write.write(send_root);

	SendDataToTheThirdParty(dst_fd, SendBuf);

	std::cout<<"Send ConfigReply\n"<<std::endl;


}
void JProtocol::QueryReply(HSocket dst_fd, int channel1_value, int channel2_value)
{
	Json::Value send_root;
	Json::Value send_arrayObj1;
	Json::Value send_arrayObj2;
	Json::Value send_item1;
	Json::Value send_item2;
	Json::StyledWriter style_write;


	send_item2["channel1"] = channel1_value;
	send_item2["channel2"] = channel2_value;
	send_arrayObj2.append(send_item2);
	send_item1["listening"] = send_arrayObj2;
	if ((channel1_value != 0) && (channel2_value != 0))
	{
		send_item1["status"] = "success";
		send_item1["reason"] = "";
	}
	else
	{
		send_item1["status"] = "fail";
		if (channel1_value ==0 && channel2_value ==0)
			send_item1["reason"] = "channel1,2 is unset";
		else if (channel1_value == 0 && channel2_value != 0)
			send_item1["reason"] = "channel1 is unset";
		else
			send_item1["reason"] = "channel2 is unset";
	}

	send_arrayObj1.append(send_item1);

	send_root["identifier"] = CreateGuid();
	send_root["type"] = "Reply";
	send_root["name"] = "Query";

	send_root["param"] = send_arrayObj1;

	send_root.toStyledString();//build json data

	std::string SendBuf = style_write.write(send_root);

	SendDataToTheThirdParty(dst_fd, SendBuf);
	std::cout<<"Send QueryReply\n"<<std::endl;

}
void JProtocol::CallRequestReply(HSocket dst_fd, std::string status, std::string reason)
{
	Json::Value send_root;
	Json::Value send_arrayObj;
	Json::Value send_item;
	Json::StyledWriter style_write;

	send_item["status"] = status;
	send_item["reason"] = reason;
	send_arrayObj.append(send_item);

	send_root["identifier"] = CreateGuid();//"2017010915420322";
	send_root["type"] = "Reply";
	send_root["name"] = "CallRequest";

	send_root["param"] = send_arrayObj;

	send_root.toStyledString();//build json data

	std::string SendBuf = style_write.write(send_root);

	SendDataToTheThirdParty(dst_fd, SendBuf);
	std::cout<<"Send CallRequestReply\n"<<std::endl;

}
void JProtocol::CallReleaseReply(HSocket dst_fd, std::string status, std::string reason)
{
	Json::Value send_root;
	Json::Value send_arrayObj;
	Json::Value send_item;
	Json::StyledWriter style_write;

	send_item["status"] = status;
	send_item["reason"] = reason;
	send_arrayObj.append(send_item);

	send_root["identifier"] = CreateGuid();//"2017010915420322";
	send_root["type"] = "Reply";
	send_root["name"] = "CallRelease";

	send_root["param"] = send_arrayObj;

	send_root.toStyledString();//build json data

	std::string SendBuf = style_write.write(send_root);

	SendDataToTheThirdParty(dst_fd, SendBuf);
	std::cout<<"Send CallReleaseReply\n"<<std::endl;

}
void JProtocol::CallStartNotify(HSocket dst_fd, int src, int dst, std::string channel)
{
	Json::Value send_root;
	Json::Value send_arrayObj;
	Json::Value send_item;
	Json::StyledWriter style_write;

	send_item["src"] = src;
	send_item["dst"] = dst;
	send_item["channel"] = channel;
	send_arrayObj.append(send_item);

	send_root["identifier"] = CreateGuid();//"2017010915420322";
	send_root["type"] = "Noitify";
	send_root["name"] = "CallStart";

	send_root["param"] = send_arrayObj;

	send_root.toStyledString();//build json data

	std::string SendBuf = style_write.write(send_root);

	SendDataToTheThirdParty(dst_fd, SendBuf);

	std::cout<<"Send CallStartNotify\n"<<std::endl;

}
void JProtocol::CallEndNotify(HSocket dst_fd, int src, int dst, std::string channel)
{
	Json::Value send_root;
	Json::Value send_arrayObj;
	Json::Value send_item;
	Json::StyledWriter style_write;

	send_item["src"] = src;
	send_item["dst"] = dst;
	send_item["channel"] = channel;
	send_arrayObj.append(send_item);

	send_root["identifier"] = CreateGuid();//"2017010915420322";
	send_root["type"] = "Noitify";
	send_root["name"] = "CallEnd";

	send_root["param"] = send_arrayObj;

	send_root.toStyledString();//build json data

	std::string SendBuf = style_write.write(send_root);

	SendDataToTheThirdParty(dst_fd, SendBuf);
	std::cout<<"Send CallEndNotify\n"<<std::endl;

}
std::string JProtocol::CreateGuid()
{
	std::string strGuid = "", strValue;
	srand((unsigned)clock()); /*播种子*/
	for (int i = 0; i < 32; i++)
	{
		float Num = (float)(rand() % 16);
		int nValue = (int)floor(Num);
		/*GOSPRINTF((char *)strValue.c_str(), 2, "%0x", nValue);*/
		sprintf((char *)strValue.c_str(),"%0x", nValue);
		strGuid.insert(i, strValue.c_str());
	}
	return strGuid;
	

}