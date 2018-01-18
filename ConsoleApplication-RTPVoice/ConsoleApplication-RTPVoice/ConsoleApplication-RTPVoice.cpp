// ConsoleApplication-RTPVoice.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "mymaster.h"


int _tmain(int argc, _TCHAR* argv[])
{
	std::string charbuff;

	std::cout <<"Master Server Start..."<< endl;

	MyMasterServer *master_server = new MyMasterServer;

	master_server->Start();

	do
	{
		std::cin >> charbuff;

	} while (charbuff != "quit");


	delete master_server;
	std::cout << "Master Server Over!" << endl;
	Sleep(10000);
	return 0;
}

