#include "Global.h"
#include "TFTPClient.h"
#include <string>

TFTPClient::TFTPClient(const std::wstring& sAddress, int p)
{
	m_socket = INVALID_SOCKET;
	m_file = NULL;

	port = p;
	// Convert std::wstring serverAddress to CString host
	/*
	CString sAddressCString (sAddress.c_str());
	host = sAddressCString;
	*/
	printf("TFTPClient transfer files to sAddress %ls and port %d \n", sAddress.c_str(), p);

	// This conversion code is FUCK
	/*
	int length = static_cast<int>(sAddress.length());
	int mbsize = WideCharToMultiByte(CP_UTF8, 0, sAddress.c_str(), length, NULL, 0, NULL, NULL);
	if (mbsize > 0)
	{	
		host = (char*)malloc(mbsize);
		// WideCharToMultiByte         ( CP_UTF8, 0, lpwstr, -1, NULL, 0, NULL, NULL );
		int bytes = WideCharToMultiByte(CP_UTF8, 0, sAddress.c_str(), length, host, mbsize, NULL, NULL);
	}
	*/

	// Repl by this code
	int size = WideCharToMultiByte( CP_UTF8, 0, sAddress.c_str(), -1, NULL, 0, NULL, NULL );
	host = new char[ size+1];
	WideCharToMultiByte( CP_UTF8, 0, sAddress.c_str(), -1, host, size, NULL, NULL );
	
	// TBA
	printf("TFTPClient converts host %s\n", host);
}

TFTPClient::~TFTPClient()
{
	Close();
	free(host);
}
	
bool
TFTPClient::Startup()
{
	WSADATA w;
	return (::WSAStartup(0x0101, &w)!=0);
}

void 
TFTPClient::Cleanup()
{
	::WSACleanup();
}

int 
TFTPClient::sendFile(const std::wstring& fileName, const std::wstring& remotefile)
{
	printf("TFTPClient sending file %ls\n", fileName.c_str());

	// Convert std::wstring fileName to CString localfile,remotefile
	// Call the equivalent code in tftp_client.cpp
	int rc = TFTP_RESULT_CONTINUE;
	
	Put(fileName, remotefile, host, port);
	
	printf("Tranffering file %ls...\n", fileName.c_str());
	while (rc == TFTP_RESULT_CONTINUE)
	{
		rc = Continue();
		printf("\rPackets: %i , Bytes: %i", PacketsCount(), BytesCount());

	}
	printf("\nDone.\n");

	if (rc == TFTP_RESULT_DONE)
	{
		printf("Ok.\n");
		rc = TFTP_FILE_SENT;
	}

	if (rc == TFTP_RESULT_ERROR)
	{
		printf("Error: %s\n", LastError());
		rc = TFTP_FILE_NOT_SENT;
	}

	return rc;
}

// MFC code
//////////////////////////////////////////////////////////////////////
// Small
//////////////////////////////////////////////////////////////////////


int TFTPClient::PacketsCount()
{
	return m_packetnum;
}

int TFTPClient::BytesCount()
{
	return m_totalbytes;
}


void TFTPClient::Error(char* text)
{
	assert(text!=NULL);
	assert(strlen(text)<TFTP_ERROR_MSG_MAX-1);

	//strcpy(m_lasterror,text);
	strcpy_s(m_lasterror,text);
}


char* TFTPClient::LastError()
{
	return m_lasterror;
}




//////////////////////////////////////////////////////////////////////
// TFTP packet class functions
//////////////////////////////////////////////////////////////////////



TFTPClient::CPacket::CPacket()
{
	Clear();
}
TFTPClient::CPacket::~CPacket()
{
	
}

unsigned char* TFTPClient::CPacket::GetData(int pos)
{
	return &(data[pos]);
}


int TFTPClient::CPacket::GetSize()
{
	return size;
}


void TFTPClient::CPacket::Clear()
{
	size=0;
	memset(data,0,TFTP_PACKET_DATA_MAX);
}


bool TFTPClient::CPacket::IsValid()
{
	return (size!=-1);
}

bool TFTPClient::CPacket::IsNull()
{
	return (size == 0);
}

void TFTPClient::CPacket::SetSize(int N)
{
	if (N>TFTP_PACKET_DATA_MAX) 
	{	size =-1;
	} else
	{	size =N;
	}

}

bool TFTPClient::CPacket::AddByte(BYTE c)
{
	if (!IsValid()) return false;
	if (size>=TFTP_PACKET_DATA_MAX)
	{
		size=-1;
		return false;
	}
	data[size] = (unsigned char)c;
	size++;
	return true;
}

bool TFTPClient::CPacket::AddMem(char* mem,int len)
{
	if (!IsValid()) return false;
	if (size+len>=TFTP_PACKET_DATA_MAX)
	{	size=-1;
		return false;
	}
	memcpy(&(data[size]),mem,len);
	size+=len;
	return true;
}

bool TFTPClient::CPacket::AddWord(WORD c)
{
	if (!AddByte(  *(((BYTE*)&c)+1) )) return false; 
	return (!AddByte(  *((BYTE*)&c) ));
}


bool TFTPClient::CPacket::AddString(char* str)
{
	if ((str==NULL)||(strlen(str)==0)) return true;
	int n=strlen(str);
	for (int i=0;i<n;i++) if (!AddByte(*(str+i))) return false;
	return true;//AddByte(0);
}


bool TFTPClient::CPacket::GetMem(int pos,char* mem,int len)
{
	if (pos>size) return false;
	if (len<size-pos) return false;

	memcpy(mem,&(data[pos]),size-pos);

	return true;

}

WORD TFTPClient::CPacket::GetWord(int pos)
{
	WORD hi = GetByte(pos);
	WORD lo = GetByte(pos+1);
	return ((hi<<8)|lo);
}

BYTE TFTPClient::CPacket::GetByte(int pos)
{
	return  (BYTE)data[pos];
}


bool TFTPClient::CPacket::IsACK(int blocknum)
{
	if (GetSize()!=4) return false;
	WORD opcode =  GetWord(0);
	WORD block = GetWord(2);
	if (opcode != TFTP_OPCODE_ACK) return false;
	if (blocknum != block)  return false;
	
	return true;
}

bool TFTPClient::CPacket::IsDATA(int blocknum)
{
	if (GetSize()<4) return false;
	WORD opcode =  GetWord(0);
	WORD block = GetWord(2);
	if (opcode != TFTP_OPCODE_DATA) return false;
	if (blocknum != block)  return false;
	
	return true;
}


bool TFTPClient::CPacket::IsError()
{
	if (GetSize()<4) return false;
	WORD opcode =  GetWord(0);
	return  (opcode == TFTP_OPCODE_ERROR);
}

bool TFTPClient::CPacket::GetError(char* buf,unsigned int buflen)
{
	if (!IsError()) return false;
	WORD errcode = GetWord(2);
	char* errmsg = (char*)GetData(4);
	if (buf==NULL) return false;
	if (strlen(errmsg)+10>buflen) return false;
	//sprintf(buf,"#%i:%s",errcode,errmsg);
	sprintf_s(buf, 1024, "#%i:%s", errcode,errmsg);

	return true;
}


void TFTPClient::CPacket::MakeACK(int blocknum)
{
	Clear();
	AddWord(TFTP_OPCODE_ACK);
	AddWord(blocknum);
}


bool TFTPClient::CPacket::MakeWRQ(int opcode,char* filename)
{
	assert(filename!=NULL);
	if (strlen(filename)>TFTP_DATA_PKT_SIZE) return false;
	Clear();
	AddWord(opcode);
	AddString(filename);
	AddByte(0);
	AddString(TFTP_DEFAULT_MODE);
	AddByte(0);
	return true;
}

void TFTPClient::CPacket::MakeDATA(int blocknum,char* data,int size)
{
	assert(data!=NULL);
	assert(size<=TFTP_DATA_PKT_SIZE);
	Clear();
	AddWord(TFTP_OPCODE_DATA);
	AddWord(blocknum);
	AddMem(data,size);

}




//////////////////////////////////////////////////////////////////////
// Others
//////////////////////////////////////////////////////////////////////



bool TFTPClient::Send(TFTPClient::CPacket* p)
{
	if (m_socket == INVALID_SOCKET)
	{
		Error("Not connected.");
		return false;
	}	
	if (!p->IsValid())
	{
		Error("Invalid packet.");
		return false;
	}
	if (p->IsNull()) return true;

	int N =  sendto(	m_socket, 
					(char*)p->GetData(), 
					p->GetSize(), 
					0, 
					(SOCKADDR *) &m_server, 
					sizeof(sockaddr_in)	);

	return ( N == p->GetSize() );
}






bool TFTPClient::WaitPacket(TFTPClient::CPacket* p,int timeout)
{
	if (m_socket == INVALID_SOCKET)
	{
		Error("Not connected.");
		return false;
	}

	assert(p!=NULL);
	p->Clear();

	FD_SET Reader;
	timeval t;
	t.tv_sec = timeout/1000;
	t.tv_usec = 0;
	FD_ZERO(&Reader);
	FD_SET(m_socket, &Reader);
	int rc = select(0,&Reader,NULL,&Reader,&t);
	if ( rc == SOCKET_ERROR)
	{
		Error("Select error.");
		return false;
	}
	if (rc == 0) 
	{
		Error("Read wait timeout.");
		return false;
	}

	int N;
		
	int  SenderAddrSize = sizeof(sockaddr_in);

	N =  recvfrom(	m_socket, 

					(char*)p->GetData(), 
					TFTP_PACKET_DATA_MAX, 
					0, 
					(SOCKADDR *)&m_server, 
					&SenderAddrSize);



    if ( ( N == 0) || (N == WSAECONNRESET ) ) 
	{
		Error("Connection Closed.");
		return false;
    }
	if ( N == SOCKET_ERROR)
	{
		Error("recv error.");
		return false;
	}
	p->SetSize(N);


	return true;

}


bool TFTPClient::ProcessError(TFTPClient::CPacket* p)
{
	if (!p->IsError()) return false;

	char errmsg[TFTP_ERROR_MSG_MAX];
	if (!p->GetError(errmsg,TFTP_ERROR_MSG_MAX))
	{
		Error("Can't get error.");
		return false;
	}
	Error(errmsg);
	return true;
}



bool TFTPClient::WaitACK(int packetnum,int timeout)
{
	CPacket p;

	
	
	if (!WaitPacket(&p,timeout)) return false;

	if (p.IsACK(packetnum)) return true;

	if (ProcessError(&p)) return false;

	Error("No ACK found.");
	return false;
}


int TFTPClient::Open(char* host,int port)
{
	// create socket
	m_socket = socket(AF_INET, SOCK_DGRAM,0);
	if (m_socket == INVALID_SOCKET)
	{
		Error("Could not create socket.");
		return TFTP_RESULT_ERROR;
	}
	// prepare address structure
	memset(&m_server,0,sizeof(sockaddr_in));
	m_server.sin_family = AF_INET;
	m_server.sin_port =  htons(port);
	m_server.sin_addr.s_addr = inet_addr(host);

	return TFTP_RESULT_CONTINUE;
}


int TFTPClient::ContinuePut()
{
	if (m_socket==INVALID_SOCKET)
	{
		Error("Not connected.");
		return TFTP_RESULT_ERROR;
	}
	if (m_file==NULL)
	{
		Error("File not opened.");
		return TFTP_RESULT_ERROR;
	}


	CPacket p;
	
	bool islastpacket=false;
	unsigned long total=0;
	int count=0;
	char buffer[TFTP_DATA_PKT_SIZE];
	int retrycount=0;
	
	
	if (feof(m_file)) 
	{	count=0;
	} else
	{	count = fread( buffer, 1, TFTP_DATA_PKT_SIZE, m_file );

		if( ferror( m_file ) )      
		{	 Error("File read error");
			 return TFTP_RESULT_ERROR;
		}

	}
	if (count<TFTP_DATA_PKT_SIZE) islastpacket=true;
	p.MakeDATA(m_packetnum,buffer,count);

	retrycount=0;
	while (true)
	{
		if (!Send(&p)) return TFTP_RESULT_ERROR;
		if (WaitACK(m_packetnum)) break;
		retrycount++;
		if (retrycount>TFTP_RETRY) 
		{	
			Error("No answer.");
			return TFTP_RESULT_ERROR;
		}
	}
	m_packetnum++;
	m_totalbytes+=count;

	if (islastpacket)
	{
		fclose(m_file);
		m_file =  NULL;
		m_status = TFTP_STATUS_NOTHING;
	}

	return  ( (islastpacket) ? TFTP_RESULT_DONE : TFTP_RESULT_CONTINUE );

}

void TFTPClient::Close()
{
	if (m_file!=NULL)
	{	fclose(m_file);
		m_file=NULL;
	}
	if (m_socket!=INVALID_SOCKET) 
	{	closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	m_status = TFTP_STATUS_NOTHING;
}

// const std::wstring& fileName
int TFTPClient::Put(const std::wstring& localfile, const std::wstring& remotefile,char* host,int port)
{
	errno_t err;

	Close();
	
	int rc = Open(host,port);
	if (rc != TFTP_RESULT_CONTINUE) 
	{
		//Error("Error opening  host");
		return rc;
	}

	// open file
	
	if( (err  = _wfopen_s(&m_file, localfile.c_str(), L"rb" )) != 0 )
	{
		Error("Can't open file.");
		return TFTP_RESULT_ERROR;
	}
   

	// create and send write req
	CPacket p;
	
	// Must convert from wstring to char* remotefile

	/*
	char* remotefileChar;
	int length = static_cast<int>(remotefile.length());
	int mbsize = WideCharToMultiByte(CP_UTF8, 0, remotefile.c_str(), length, NULL, 0, NULL, NULL);
	*/

	// Repl by this code	
	char* remotefileChar;
	int mbsize = WideCharToMultiByte( CP_UTF8, 0, remotefile.c_str(), -1, NULL, 0, NULL, NULL );

	
	if (mbsize > 0)
	{	
		/*
		remotefileChar = (char*)malloc(mbsize);
		int bytes = WideCharToMultiByte(CP_UTF8, 0, remotefile.c_str(), length, remotefileChar, mbsize, NULL, NULL);
		printf("remotefileChar is %s\n", remotefileChar);
		*/

		remotefileChar = new char[ mbsize+1];
		WideCharToMultiByte( CP_UTF8, 0, remotefile.c_str(), -1, remotefileChar, mbsize, NULL, NULL );
		
		//
		if (!p.MakeWRQ(TFTP_OPCODE_WRITE,remotefileChar)) 
		{
			Error("Can't create write req.");
			//free(remotefileChar);
			delete [] remotefileChar;
			return TFTP_RESULT_ERROR;
		}
		//
		if (!Send(&p)) 
		{
			//free(remotefileChar);
			delete [] remotefileChar;
			return TFTP_RESULT_ERROR;
		}
		if (!WaitACK(0)) 
		{
			//free(remotefileChar);
			delete [] remotefileChar;
			return TFTP_RESULT_ERROR;
		}

		m_packetnum = 1;
		m_totalbytes = 0;
		m_status = TFTP_STATUS_PUT;

	} else {
		return TFTP_RESULT_ERROR;
	}

	/*
	if (!p.MakeWRQ(TFTP_OPCODE_WRITE,remotefileChar)) 
	{
		Error("Can't create write req.");
		return TFTP_RESULT_ERROR;
	}
	if (!Send(&p)) return TFTP_RESULT_ERROR;
	if (!WaitACK(0)) return TFTP_RESULT_ERROR;

	m_packetnum = 1;
	m_totalbytes = 0;
	m_status = TFTP_STATUS_PUT;
	*/

	return TFTP_RESULT_CONTINUE;

}

int TFTPClient::Continue()
{
	if (m_status == TFTP_STATUS_PUT) return ContinuePut();

	Error("Not started.");
	return TFTP_RESULT_ERROR;
}
