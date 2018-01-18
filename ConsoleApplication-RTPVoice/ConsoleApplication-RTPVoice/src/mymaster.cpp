#include"stdafx.h"
#include "mymaster.h"

MyMasterServer *MyMasterServer::pThis = NULL;

MyMasterServer::MyMasterServer()
{
	pThis = this;
	fp = NULL;;
	channel1RTP = NULL;
	channel2RTP = NULL;
	mastergate = new JProtocol;
}

MyMasterServer::~MyMasterServer()
{
	if (channel1RTP != NULL)
	{
		delete channel1RTP;
		channel1RTP = NULL;
		if (fp != NULL)
		{
			fclose(fp);
			fp = NULL;
		}
	}
	if (channel2RTP != NULL)
	{
		delete channel2RTP;
		channel2RTP = NULL;
	}
	if (mastergate != NULL)
	{
		delete mastergate;
		mastergate = NULL;
	}

	std::cout << "exit class: MyMasterServer \n" << endl;

}

void MyMasterServer::Start()
{

	mastergate->SetCallBackFunc(MasterOnData);//设置回调函数

	mastergate->Start();//启动master

	int src = 0xffff;
	int dst = 0xabcd;
	std::string channel = "channel0";
	// TODO:  在此添加控件通知处理程序代码
	//if (mastergate != NULL)
	//{
	//	if (mastergate->IsMaterInitComplete())mastergate->CallStartNotify(0, src, dst, channel);

	//}

	//if (mastergate != NULL)
	//{
	//	if (mastergate->IsMaterInitComplete())mastergate->CallEndNotify(0, src, dst, channel);

	//}


}

void MyMasterServer::RTPChannel1OnData(ResponeRTPData data)
{
	if (pThis == NULL)exit(-1);
	pThis->RTPChannel1OnDataFunc(data);

}
void MyMasterServer::RTPChannel1OnDataFunc(ResponeRTPData data)
{
	static int count = 0;
	count++;
	fwrite(data.payloaddata, 1, data.payloaddatalength, fp);
	std::cout << "RTPChannel - 1: Receive %d packet of voice data \n" << count << endl;
	//TRACE(("RTPChannel-1: Receive %d packet of voice data\n"), count);

}

void MyMasterServer::RTPChannel2OnData(ResponeRTPData data)
{
	if (pThis == NULL)exit(-1);
	pThis->RTPChannel2OnDataFunc(data);

}
void MyMasterServer::RTPChannel2OnDataFunc(ResponeRTPData data)
{
	static int count = 0;
	count++;
	std::cout << "RTPChannel - 2: Receive %d packet of voice data \n" << count << endl;
	//TRACE(_T("RTPChannel-2: Receive %d packet of voice data\n"), count);

}


void MyMasterServer::MasterOnData(int command, ResponeData data)
{

	if (pThis == NULL)exit(-1);
	pThis->MasterOnDataFunc(command, data);

}
void MyMasterServer::MasterOnDataFunc(int command, ResponeData data)
{
	std::string c_status = "success";
	//char *sendVoiceBuff = {"abcdefghiojklmnopqrstuvwxyz"};
	char sendVoiceBuff[50] = "abcdefghiojklmnopqrstuvwxyz";

	PROTOCOL_Names pro_name = (PROTOCOL_Names)command;
	switch (pro_name)
	{
	case CONNECT:
		mastergate->ConnectReply(data.socket_fd, "success", "fine!");
		if (1)/*PTT Notice Enable*/
		{

		}
		break;


	case LISTENING:
		if (data.channel1_group_id != 0)
		{
			if (channel1RTP == NULL){
				channel1RTP = new MyRTP;
				channel1RTP->RtpParamsInit(CHANNEL1RTPBASEPORT, CHANNEL1RTPDESTPORT, CHANNEL1RTPSSRC);//是否隐式设置？
				channel1RTP->SetCallBackFunc(RTPChannel1OnData);

				fp = fopen("E:\\CloudMusic\\master_recv_voice.pcm", "wb+");
				if (fp == NULL){
					printf("fp  fail\n");
				}
				fseek(fp, 0, SEEK_SET);

			}
			//TRACE(("set channel1 is:%d\n"), data.channel1_group_id);
			std::cout << "set channel1 is:"<<dec<< data.channel1_group_id << endl;

		}

		if (data.channel2_group_id != 0)
		{
			if (channel2RTP == NULL){
				channel2RTP = new MyRTP;
				channel2RTP->RtpParamsInit(CHANNEL2RTPBASEPORT, CHANNEL2RTPDESTPORT, CHANNEL2RTPSSRC);
				channel2RTP->SetCallBackFunc(RTPChannel2OnData);
			}
			//TRACE(("set channel2 is:%d\n"), data.channel2_group_id);
			std::cout << "set channel2 is:" << dec << data.channel2_group_id << endl;
		}
		if (data.channel1_group_id == 0 && data.channel2_group_id == 0)
		{
			//TRACE(_T("No channel is set\n"));
			std::cout << "No channel is set\n" << endl;
		}
		mastergate->ConfigReply(data.socket_fd, data.channel1_group_id, data.channel2_group_id);
		break;

	case QUERY:
		mastergate->QueryReply(data.socket_fd, data.channel1_group_id, data.channel2_group_id);
		break;


	case CALLREQUEST:
		mastergate->CallRequestReply(data.socket_fd, "fail", "Unconnect");
		if (c_status == "success")
		{
			//TRACE(_T("Start to send RTP Voice\n"));
			std::cout << "Start to send RTP Voice\n" << endl;
			mastergate->CallStartNotify(data.socket_fd, data.src_id, data.dst_id, data.channel_id);

			if ((data.channel_id == "channel1") && channel1RTP != NULL)
				channel1RTP->SendRTPPayloadData(sendVoiceBuff, 24);

			if ((data.channel_id == "channel2") && channel2RTP != NULL)
				channel2RTP->SendRTPPayloadData(sendVoiceBuff, 24);

		}
		break;

	case CALLRELEASE:
		mastergate->CallReleaseReply(data.socket_fd, "fail", "Unconnect");
		if (c_status == "success")
		{
			//TRACE(_T("Stop to send RTP Voice\n"));
			std::cout << "Stop to send RTP Voice\n" << endl;
			mastergate->CallEndNotify(data.socket_fd, data.src_id, data.dst_id, data.channel_id);
		}
		break;
	default:

		break;


	}

	//TRACE(_T("MasterOnDataFunc finished\n"));
	std::cout << "MasterOnDataFunc finished\n" << endl;

}