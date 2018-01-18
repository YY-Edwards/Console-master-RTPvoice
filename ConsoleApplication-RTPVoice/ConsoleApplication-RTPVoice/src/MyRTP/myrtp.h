/*
 * myrtp.h
 *
 * Created: 2018/01/03
 * Author: EDWARDS
 */ 
#ifndef myrtp_h_
#define myrtp_h_ 


#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtpsourcedata.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <stdbool.h>
#include <stdint.h>
#include <process.h>
#include "common.h"
#include "syninterface.h"

using namespace jrtplib;
using namespace std;

class MyRTP :public RTPSession
{

public:
		
		MyRTP();
		~MyRTP();
		/*
		RTPͨ���������ã�
			portbase�����ض˿ڣ�
			destport��Ŀ��˿ڣ�
			ssrc:Ԥ������������ݱ�ʶ
		*/
		void RtpParamsInit(uint16_t portbase, uint16_t destport, uint32_t ssrc);

		/*
		RTPͨ���߼����ã�
		*/
		void SetParamsForSender();

		/*
		RTPͨ���������ݣ�
		*/
		void SendRTPPayloadData(void* buff, uint32_t buff_length);

		/*
		�ص��ӿ�
		���ûص�����
		*/
		void SetCallBackFunc(void(*callBackFunc)(ResponeRTPData));


		uint8_t *GetRTPPayloadData();
		uint32_t GetRTPPayloadDataLength();
		uint32_t GetRTPSSRC();

protected:

		static int OnPollThread(void* p);
		void OnPollThreadFunc();
		void ProcessRTPPacket(const RTPSourceData &srcdat,const RTPPacket &rtppack);
		//�ص��ӿ�
		void(*RecvVoiceCallBackFunc)(ResponeRTPData);//�����������ݵĻص�

		void onData(void(*func)(ResponeRTPData), ResponeRTPData data);
	
	
	private:


		void SetThreadExitFlag()   { set_thread_exit_flag = true; }
		/*bool IsThreadHasExit() {
			if (rx_rtp_thread_p)return thread_exited_flag;
			else
				return true;

		}*/

		void CheckError(int rtperr);

		//HANDLE ondata_locker;
		ILock *ondatalock;
		bool poll_thread_isactive;
		bool set_thread_exit_flag;
		//bool thread_exited_flag;
		uint32_t ssrc;
		uint32_t payloaddatalength;
		uint8_t* payloaddata;
		//HANDLE rx_rtp_handle;
		MyCreateThread *rx_rtp_thread_p;
	
		
};








#endif