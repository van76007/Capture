/*
   Class: TFTPClient

   Monitor file system
*/
#if !defined(AFX_TFTPCLIENT1_H__63376F6D_091B_43D9_9703_517424B6D6C2__INCLUDED_)
#define AFX_TFTPCLIENT1_H__63376F6D_091B_43D9_9703_517424B6D6C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Global.h"
#include <winsock.h>
#include <string>
#include <atlstr.h>
#include <assert.h>

#define TFTP_DEFAULT_PORT    69

#define TFTP_OPCODE_READ     1
#define TFTP_OPCODE_WRITE    2
#define TFTP_OPCODE_DATA     3
#define TFTP_OPCODE_ACK      4
#define TFTP_OPCODE_ERROR    5

#define TFTP_DEFAULT_MODE    "octet"

#define TFTP_DATA_PKT_SIZE   512

#define TFTP_ERROR_MSG_MAX	1024
#define TFTP_PACKET_DATA_MAX 1024

#define TFTP_RETRY     3


#define TFTP_RESULT_ERROR		1
#define TFTP_RESULT_CONTINUE    2
#define TFTP_RESULT_DONE		3

#define TFTP_STATUS_NOTHING		1
#define TFTP_STATUS_GET			2
#define TFTP_STATUS_PUT			3
#define TFTP_STATUS_ERROR		4

#define TFTP_TIMEOUT		5000 //ms

class TFTPClient
{

protected:

	class CPacket
	{	public:
			CPacket();
			~CPacket();
			bool AddByte(BYTE c);
			bool AddWord(WORD c);
			bool AddString(char* str);
			bool AddMem(char* mem,int len);
			unsigned char* GetData(int pos=0);
			WORD GetWord(int pos);
			BYTE GetByte(int pos);
			bool GetMem(int pos,char* mem,int len);
			int GetSize();
			void SetSize(int N);
			void Clear();
			bool IsValid();
			bool IsNull();
			bool IsACK(int blocknum);
			bool IsDATA(int blocknum);
			bool IsError();
			bool GetError(char* buf,unsigned int buflen);

			void MakeACK(int blocknum);
			void MakeDATA(int blocknum,char* data,int size);
			bool MakeWRQ(int opcode,char* filename);

		protected:
			unsigned char data[TFTP_PACKET_DATA_MAX];
			int size;
	};

public:
	TFTPClient(const std::wstring& sAddress, int p);
	~TFTPClient();
	
	int sendFile(const std::wstring& fileName, const std::wstring& remotefile);
	
	// From the MFC code
	
	//int Put(char* localfile, char* remotefile, char* host,int port=TFTP_DEFAULT_PORT);
	int Put(const std::wstring& localfile, const std::wstring& remotefile,char* host,int port);
	
	int Continue();
	void Close();

	int PacketsCount();
	int BytesCount();
	char* LastError();

	static bool Startup(); // void start(); 
	static void Cleanup(); // void stop();

private:
	
	int port;
	//CString localfile,remotefile,host;
	char* host;

	// char* localfile;
	//char* remotefile;
	
	// From the MFC code, this is protected:
	char m_lasterror[TFTP_ERROR_MSG_MAX];
	FILE* m_file; // Van: To hold the copy of the local_file: if( (m_file  = fopen( localfile, "wb" )) == NULL )
	unsigned int m_packetnum;
	unsigned int m_totalbytes;
	int m_status;
	CPacket m_lastack;
	SOCKET m_socket;
	sockaddr_in m_server;

	int ContinuePut();
	void Error(char* text);


	bool Send(TFTPClient::CPacket* p);
	bool WaitACK(int packetnum,int timeout = TFTP_TIMEOUT);
	bool WaitPacket(TFTPClient::CPacket* p,int timeout = TFTP_TIMEOUT);
	//
	int Open(char* host,int port);	// Connect to remote server here
	bool ProcessError(TFTPClient::CPacket* p);
	
};
#endif // !defined(AFX_TFTPCLIENT1_H__63376F6D_091B_43D9_9703_517424B6D6C2__INCLUDED_)