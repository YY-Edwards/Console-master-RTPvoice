#include "stdafx.h"
#include "socketwrap.h"

#define INVALIDSOCKHANDLE   INVALID_SOCKET  

#ifdef WIN32 
#include <windows.h>  
#define ISSOCKHANDLE(x)  (x!=INVALID_SOCKET)  
#define BLOCKREADWRITE      0  
#define NONBLOCKREADWRITE   0  
#define SENDNOSIGNAL        0  
#define ETRYAGAIN(x)     (x==WSAEWOULDBLOCK||x==WSAETIMEDOUT||x==EAGAIN||x==EINTR)  
//#define gxsprintf   sprintf

#else
#define ISSOCKHANDLE(x)    (x>0)  
#define BLOCKREADWRITE      MSG_WAITALL  
#define NONBLOCKREADWRITE   MSG_DONTWAIT  
#define SENDNOSIGNAL        MSG_NOSIGNAL  
#define ETRYAGAIN(x)        (x==EAGAIN||x==EWOULDBLOCK)  
#define gxsprintf           snprintf  

#endif  

void GetAddressFrom(sockaddr_in *addr, const char *ip, int port)
{
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;            /*地址类型为AF_INET*/
	if (ip)
	{
		addr->sin_addr.s_addr = inet_addr(ip);
	}
	else
	{
		/*网络地址为INADDR_ANY，这个宏表示本地的任意IP地址，因为服务器可能有多个网卡，每个网卡也可能绑定多个IP地址，
		这样设置可以在所有的IP地址上监听，直到与某个客户端建立了连接时才确定下来到底用哪个IP地址*/
		addr->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	addr->sin_port = htons(port);   /*端口号*/
}
void GetIpAddress(char *ip, sockaddr_in *addr)
{
	unsigned char *p = (unsigned char *)(&(addr->sin_addr));
	/*gxsprintf(ip, 17, "%u.%u.%u.%u", *p, *(p + 1), *(p + 2), *(p + 3));*/
	sprintf(ip, "%u.%u.%u.%u", *p, *(p + 1), *(p + 2), *(p + 3));
}

int GetLastSocketError()
{
#ifdef WIN32 
	return WSAGetLastError();
#else
	return errno;
#endif  
}

bool IsValidSocketHandle(HSocket handle)
{
	return ISSOCKHANDLE(handle);
}

void SocketClose(HSocket &handle)
{
	if (ISSOCKHANDLE(handle))
	{
#ifdef WIN32 
		closesocket(handle);
#else
		close(handle);
#endif  
		handle = INVALIDSOCKHANDLE;
	}
}

HSocket SocketOpen(int tcpudp)
{
	int protocol = 0;
	HSocket hs;
#ifdef WIN32 
	if (tcpudp == SOCK_STREAM) protocol = IPPROTO_TCP;
	else if (tcpudp == SOCK_DGRAM) protocol = IPPROTO_UDP;
#endif  
	hs = socket(AF_INET, tcpudp, protocol);
	return hs;
}
int SocketBind(HSocket hs, sockaddr_in *paddr)
{
	return bind(hs, (struct sockaddr *)paddr, sizeof(sockaddr_in));
}
int SocketListen(HSocket hs, int maxconn)
{
	return listen(hs, maxconn);
}
HSocket SocketAccept(HSocket hs, sockaddr_in *paddr)
{
#ifdef WIN32 
	int cliaddr_len = sizeof(sockaddr_in);
#else
	socklen_t cliaddr_len = sizeof(sockaddr_in);
#endif  
	return accept(hs, (struct sockaddr *)paddr, &cliaddr_len);
}

//  
// if timeout occurs, nbytes=-1, nresult=1  
// if socket error, nbyte=-1, nresult=-1  
// if the other side has disconnected in either block mode or nonblock mode, nbytes=0, nresult=-1  
// otherwise nbytes= the count of bytes sent , nresult=0  
void SocketSend(HSocket hs, const char *ptr, int nbytes, transresult_t &rt)
{
	rt.nbytes = 0;
	rt.nresult = 0;
	if (!ptr || nbytes<1) return;

	//Linux: flag can be MSG_DONTWAIT, MSG_WAITALL, 使用MSG_WAITALL的时候, socket 必须是处于阻塞模式下，否则WAITALL不能起作用  
	rt.nbytes = send(hs, ptr, nbytes, BLOCKREADWRITE | SENDNOSIGNAL);
	if (rt.nbytes>0)
	{
		rt.nresult = (rt.nbytes == nbytes) ? 0 : 1;
	}
	else if (rt.nbytes == 0)
	{
		rt.nresult = -1;
	}
	else
	{
		rt.nresult = GetLastSocketError();
		rt.nresult = ETRYAGAIN(rt.nresult) ? 1 : -1;
	}
}



// if timeout occurs, nbytes=-1, nresult=1  
// if socket error, nbyte=-1, nresult=-1  
// if the other side has disconnected in either block mode or nonblock mode, nbytes=0, nresult=-1  
void SocketRecv(HSocket hs, char *ptr, int nbytes, transresult_t &rt)
{
	rt.nbytes = 0;
	rt.nresult = 0;
	if (!ptr || nbytes<1) return;

	rt.nbytes = recv(hs, ptr, nbytes, BLOCKREADWRITE);
	if (rt.nbytes>0)
	{
		return;
	}
	else if (rt.nbytes == 0)
	{
		rt.nresult = -1;
	}
	else
	{
		rt.nresult = GetLastSocketError();
		rt.nresult = ETRYAGAIN(rt.nresult) ? 1 : -1;
	}

}



void SocketClearRecvBuffer(HSocket hs)
{
#ifdef WIN32 
	struct timeval tmOut;
	tmOut.tv_sec = 0;
	tmOut.tv_usec = 0;
	fd_set    fds;
	FD_ZERO(&fds);
	FD_SET(hs, &fds);
	int   nRet = 1;
	char tmp[100];
	//int rt;
	while (nRet>0)
	{
		//用select的超时特性，将超时时间设置为0，通过检测select的返回值，就可以判断缓冲是否被清空。通过这个技巧，使一个阻塞的socket成了‘非阻塞’socket。
		nRet = select(FD_SETSIZE, &fds, NULL, NULL, &tmOut);
		if (nRet>0)
		{
			nRet = recv(hs, tmp, 100, 0);
		}
	}
#else 
	char tmp[100];
	while (recv(hs, tmp, 100, NONBLOCKREADWRITE)> 0);
#endif  
}

int SocketBlock(HSocket hs, bool bblock)
{
	unsigned long mode;
	if (ISSOCKHANDLE(hs))
	{
#ifdef WIN32 
		mode = bblock ? 0 : 1;
		return ioctlsocket(hs, FIONBIO, &mode);
#else
		mode = fcntl(hs, F_GETFL, 0);                  //获取文件的flags值。  
		//设置成阻塞模式      非阻塞模式  
		return bblock ? fcntl(hs, F_SETFL, mode&~O_NONBLOCK) : fcntl(hs, F_SETFL, mode | O_NONBLOCK);
#endif  
	}
	return -1;
}
int SocketTimeOut(HSocket hs, int recvtimeout, int sendtimeout, int lingertimeout)   //in milliseconds  
{
	int rt = -1;
	if (ISSOCKHANDLE(hs))
	{
		rt = 0;
#ifdef WIN32 
		if (lingertimeout>-1)
		{
			struct linger  lin;
			lin.l_onoff = lingertimeout;
			lin.l_linger = lingertimeout;
			//残留数据超时设置
			rt = setsockopt(hs, SOL_SOCKET, SO_DONTLINGER, (const char*)&lin, sizeof(linger)) == 0 ? 0 : 0x1;
		}
		if (recvtimeout>0 && rt == 0)
		{
			rt = rt | (setsockopt(hs, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvtimeout, sizeof(int)) == 0 ? 0 : 0x2);
		}
		if (sendtimeout>0 && rt == 0)
		{
			rt = rt | (setsockopt(hs, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendtimeout, sizeof(int)) == 0 ? 0 : 0x4);
		}
#else 
		struct timeval timeout;
		if (lingertimeout>-1)
		{
			struct linger  lin;
			lin.l_onoff = lingertimeout>0 ? 1 : 0;
			lin.l_linger = lingertimeout / 1000;
			rt = setsockopt(hs, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(linger)) == 0 ? 0 : 0x1;
		}
		if (recvtimeout>0 && rt == 0)
		{
			timeout.tv_sec = recvtimeout / 1000;
			timeout.tv_usec = (recvtimeout % 1000) * 1000;
			rt = rt | (setsockopt(hs, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0 ? 0 : 0x2);
		}
		if (sendtimeout>0 && rt == 0)
		{
			timeout.tv_sec = sendtimeout / 1000;
			timeout.tv_usec = (sendtimeout % 1000) * 1000;
			rt = rt | (setsockopt(hs, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0 ? 0 : 0x4);
		}
#endif  
	}
	return rt;
}
int InitializeSocketEnvironment()
{
#ifdef WIN32   
	WSADATA  Ws;
	//Init Windows Socket  
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
	{
		return -1;
	}
#endif  
	return 0;
}
void FreeSocketEnvironment()
{
#ifdef WIN32 
	WSACleanup();
#endif  
}

CSockWrap::CSockWrap(int tcpudp)
{
	memset(&m_stAddr, 0, sizeof(sockaddr_in));
	m_tcpudp = tcpudp;
	m_hSocket = INVALIDSOCKHANDLE;
	Reopen(false);
}


CSockWrap::~CSockWrap()
{
	SocketClose(m_hSocket);
}
void CSockWrap::Close()
{
	SocketClose(m_hSocket);
	m_hSocket = INVALIDSOCKHANDLE;
}
void CSockWrap::Reopen(bool bForceClose)
{

	if (ISSOCKHANDLE(m_hSocket) && bForceClose) SocketClose(m_hSocket);
	if (!ISSOCKHANDLE(m_hSocket))
	{
		m_hSocket = SocketOpen(m_tcpudp);
	}

}
void CSockWrap::SetAddress(const char *ip, int port)
{
	GetAddressFrom(&m_stAddr, ip, port);
}
void CSockWrap::SetAddress(sockaddr_in *addr)
{
	memcpy(&m_stAddr, addr, sizeof(sockaddr_in));
}

int CSockWrap::SetTimeOut(int recvtimeout, int sendtimeout, int lingertimeout)   //in milliseconds  
{
	return SocketTimeOut(m_hSocket, recvtimeout, sendtimeout, lingertimeout);
}

int CSockWrap::SetBufferSize(int recvbuffersize, int sendbuffersize)   //in bytes  
{
	int rt = -1;
	if (ISSOCKHANDLE(m_hSocket))
	{
#ifdef WIN32 
		if (recvbuffersize>-1)
		{
			rt = setsockopt(m_hSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&recvbuffersize, sizeof(int));
		}
		if (sendbuffersize>-1)
		{
			rt = rt | (setsockopt(m_hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&sendbuffersize, sizeof(int)) == 0 ? 0 : 0x2);
		}
#endif  
	}
	return rt;
}

int CSockWrap::SetBlock(bool bblock)
{
	return SocketBlock(m_hSocket, bblock);
}
transresult_t CSockWrap::Send(void *ptr, int nbytes)
{
	transresult_t rt;
	SocketSend(m_hSocket, (const char *)ptr, nbytes, rt);
	return rt;
}
transresult_t CSockWrap::Recv(void *ptr, int nbytes)
{
	transresult_t rt;
	SocketRecv(m_hSocket, (char *)ptr, nbytes, rt);
	return rt;
}
//transresult_t CSockWrap::TrySend(void *ptr, int nbytes, int milliseconds)
//{
//	transresult_t rt;
//	SocketTrySend(m_hSocket, (const char *)ptr, nbytes, milliseconds, rt);
//	return rt;
//}
//transresult_t CSockWrap::TryRecv(void *ptr, int nbytes, int  milliseconds)
//{
//	transresult_t rt;
//	SocketTryRecv(m_hSocket, (char *)ptr, nbytes, milliseconds, rt);
//	return rt;
//}

void CSockWrap::ClearRecvBuffer()
{
	SocketClearRecvBuffer(m_hSocket);
}