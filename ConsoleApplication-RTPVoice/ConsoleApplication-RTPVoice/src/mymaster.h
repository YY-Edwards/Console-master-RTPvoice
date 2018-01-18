#ifndef MYMASTER_H
#define MYMASTER_H

#include "myinclude.h"

class MyMasterServer
{
public:
	MyMasterServer();
	~MyMasterServer();

	void Start();

private:

	static MyMasterServer *pThis;

	MyRTP *channel1RTP;
	MyRTP *channel2RTP;
	JProtocol *mastergate;

	//protocol callbackfunc
	static void  MasterOnData(int command, ResponeData data);
	void  MasterOnDataFunc(int command, ResponeData data);

	//rtp callbackfunc
	static void  RTPChannel1OnData(ResponeRTPData data);
	void  RTPChannel1OnDataFunc(ResponeRTPData data);

	static void  RTPChannel2OnData(ResponeRTPData data);
	void  RTPChannel2OnDataFunc(ResponeRTPData data);

	FILE *fp;


};





#endif