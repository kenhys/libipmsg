#ifndef _IPMESSENGER_IMPL_H_
#define _IPMESSENGER_IMPL_H_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <dirent.h>
#include <netdb.h>
#include <utime.h>
#include <errno.h>

#ifdef HAVE_OPENSSL
#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/blowfish.h>
#include <openssl/rc2.h>
#endif

#include <string>
#include <queue>
#include <map>
#include <vector>
using namespace std;

const int IPMSG_DEFAULT_PORT=0x0979;

class IpMessengerAgentImpl {
	public:
		friend class IpMessengerAgent;
		friend class RecievedMessage;
		friend class SentMessage;
		friend class HostList;
		friend class HostListItem;
		friend void *GetFileDataThread( void *param );
		friend void *GetDirFilesThread( void *param );

		IPMSG_READONLY_PROPERTY( string, LoginName );
		IPMSG_READONLY_PROPERTY( string, HostName );
		IPMSG_PROPERTY( bool, IsDialup );
		IPMSG_PROPERTY( bool, AbortDownloadAtFileChanged );
		IPMSG_PROPERTY( bool, SaveSentMessage );
		IPMSG_PROPERTY( bool, SaveRecievedMessage );

		static IpMessengerAgentImpl *GetInstance();
		static void Release();
		static void GetNetworkInterfaceInfo( vector<NetworkInterface>& nics );
		void ClearBroadcastAddress();
		void DeleteBroadcastAddress( string addr );
		void AddBroadcastAddress( string addr );
		void Login( string nickname, string groupName );
		void Logout();
		HostList& GetHostList();
		HostList& UpdateHostList( bool isRetry=false );
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, bool noLogging=false, unsigned long opt=0UL );
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword=false, int hostCountAtSameTime=1, bool noLogging=false, unsigned long opt=0UL );
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword=false, int hostCountAtSameTime=1, bool noLogging=false, unsigned long opt=0UL, bool isRetry = false, unsigned long PrevPacketNo = 0UL );
		void ResetAbsence();
		void SetAbsence( string encoding, vector<AbsenceMode> absenceModes );
		vector<GroupItem> GetGroupList();
		string GetInfo( HostListItem& host );
		string GetAbsenceInfo( HostListItem& host );
		int Process();
		int GetRecievedMessageCount();
		RecievedMessage PopRecievedMessage();
		void ConfirmMessage( RecievedMessage &msg );
		void DeleteNotify( RecievedMessage msg );
		void AcceptConfirmNotify( SentMessage msg );
		SentMessageList *GetSentMessages();
		SentMessageList CloneSentMessages();
		void SetSortHostListComparator( HostListComparator *comparator );
		HostListComparator *GetSortHostListComparator();
		void SetEventObject( IpMessengerEvent *evt );
		IpMessengerEvent *GetEventObject();
		void SetFileNameConverter( FileNameConverter *conv );
		FileNameConverter *GetFileNameConverter();
		bool IsAbsence();
		void RestartNetwork();
		void QueryVersionInfo( HostListItem& host );
		void QueryAbsenceInfo( HostListItem& host );

	private:
#ifdef HAVE_OPENSSL
		RSA *RsaMax;
		RSA *RsaMid;
		RSA *RsaMin;
		RSA *GetOptimizedRsa( unsigned long cap );
		unsigned long encryptionCapacity;
#endif
		IpMessengerEvent *event;
		HostListComparator *compare;
		SentMessageList sentMsgList;
		RecievedMessageList recvMsgList;
		bool _IsAbsence;
		FileNameConverter *converter;
		vector<AbsenceMode> absenceModeList;
		string DecryptErrorMessage;
		string Nickname;
		string GroupName;
		string HostAddress;

		vector<int> tcp_sd;
		vector<int> udp_sd;
		int max_sd;
		vector<struct sockaddr_in> addr_recv;
		struct timeval tv;
		fd_set rfds;
		vector<struct sockaddr_in> broadcastAddr;
		vector<Packet> PacketsForChecking;
		HostList hostList;
		vector<NetworkInterface> NICs;
		string localEncoding;

		IpMessengerAgentImpl();
		~IpMessengerAgentImpl();
		void CryptoInit();
		void CryptoEnd();
		void NetworkInit();
		void NetworkEnd();
		void InitSend();
		void InitRecv( vector<NetworkInterface> nics );
		int InitUdpRecv( struct sockaddr_in addr );
		int InitTcpRecv( struct sockaddr_in addr );
		int RecvPacket();
		bool RecvUdp( fd_set *fds, struct sockaddr_in *sender_addr, int *sz, char *buf );
		bool RecvTcp( fd_set *fds, struct sockaddr_in *sender_addr, int *sz, char *buf, int *tcp_socket );
		bool FindDuplicatePacket( const Packet &packet );
		void PurgePacket( time_t nowTime );
		void CheckSendMsgRetry( time_t nowTime );
		void CheckGetHostListRetry( time_t nowTime );
		void SendPacket( const unsigned long cmd, char *buf, int size, struct sockaddr_in toAddr );
		void SendBroadcast( const unsigned long cmd, char *buf, int size );
		void DoRecvCommand( Packet packet );
		int SendNoOperation();
		int SendAbsence();
		int UdpRecvEventNoOperation( Packet packet );
		int UdpRecvEventBrEntry( Packet packet );
		int UdpRecvEventBrExit( Packet packet );
		int UdpRecvEventBrAbsence( Packet packet );
		int UdpRecvEventBrIsGetList( Packet packet );
		int UdpRecvEventBrIsGetList2( Packet packet );
		int UdpRecvEventOkGetList( Packet packet );
		int UdpRecvEventGetList( Packet packet );
		int UdpRecvEventAnsEntry( Packet packet );
		int UdpRecvEventAnsList( Packet packet );
		int UdpRecvEventSendMsg( Packet packet );
		int UdpRecvEventRecvMsg( Packet packet );
		int UdpRecvEventDelMsg( Packet packet );
		int UdpRecvEventReadMsg( Packet packet );
		int UdpRecvEventAnsReadMsg( Packet packet );
		int UdpRecvEventGetInfo( Packet packet );
		int UdpRecvEventSendInfo( Packet packet );
		int UdpRecvEventGetAbsenceInfo( Packet packet );
		int UdpRecvEventSendAbsenceInfo( Packet packet );
		int UdpRecvEventGetPubKey( Packet packet );
		int UdpRecvEventAnsPubKey( Packet packet );
		int TcpRecvEventGetFileData( Packet packet );
		int UdpRecvEventReleaseFiles( Packet packet );
		int TcpRecvEventGetDirFiles( Packet packet );
		int AddDefaultHost();
		int CreateHostList( const char *packetIpAddress, const char *packetHostName, const char *hostListBuf, int bufLen );
		Packet DismantlePacketBuffer( char *packetBuf, int size, struct sockaddr_in sender, time_t nowTime );
		int CreateAttachedFileList( const char *option, AttachFileList &files );
		bool EncryptMsg( HostListItem host, unsigned char *optBuf, int optBufLen, int *encOptBufLen, int optSize );
		bool DecryptMsg( Packet &packet );
		vector<struct sockaddr_in>::iterator FindBroadcastNetworkByAddress( string addr );
		unsigned long AddCommonCommandOption( const unsigned long cmd );
		bool IsFileChanged( time_t mtime, unsigned long long size, struct stat statInit, struct stat statProgress );

		//Library Use Only
		void AddHostListFromPacket( const Packet& packet );
		int CreateNewPacketBuffer( unsigned long cmd, unsigned long packet_no, string user, string host, const char *opt, int optLen, char *buf, int size );
		int CreateNewPacketBuffer( unsigned long cmd, string user, string host, const char *opt, int optLen, char *buf, int size );
		void SendTcpPacket( int sd, char *buf, int size );
		bool SendFile( int sock, string FileName, time_t mtime, unsigned long long size, AttachFile *trans, off_t offset=0 );
		bool SendDirData( int sock, string cd, string dir, vector<string> &files );
		int GetMaxOptionBufferSize();
};

//selectシステムコールのタイムアウト時間
#if defined(DEBUG)
//0.1秒
#include <ctype.h>
#define SELECT_TIMEOUT_SEC	0
#define SELECT_TIMEOUT_USEC	100000
#else	//DEBUG
//0.05秒
#define SELECT_TIMEOUT_SEC	0
#define SELECT_TIMEOUT_USEC	50000
#endif	//DEBUG

//パケットの一意性チェック用のパケット保存期間(秒)
#define	PACKET_CHECK_FOR_SAVING_INTERVAL	20

//NICの最大数
#define IFR_MAX 20

//ホストリスト取得リトライ間隔
#define GETLIST_RETRY_INTERVAL	2
//ホストリスト取得リトライ最大数
#define GETLIST_RETRY_MAX		2

//一回のホストリスト取得最大数
#define HOST_LIST_SEND_MAX_AT_ONCE	100
//パケットのデリミタ文字
#define	PACKET_DELIMITER_CHAR	':'
//パケットのデリミタ文字列
#define	PACKET_DELIMITER_STRING	":"
//オプション部の項目区切り文字
#define	PACKET_FIELD_SEPERATOR_CHAR	'\a'
//バージョン文字列
#define	IPMSG_AGENT_VERSION		"IpMessengerAgent for C++ Unix Version " VERSION

char *inet_ntoa_r( in_addr_t in, char *ret, int size );

#if defined(DEBUG) || defined(INFO)
void IpMsgPrintBuf( const char* bufname, const char *buf, const int size );
void IpMsgDumpPacket( Packet packet, struct sockaddr_in sender_addr );
string GetCommandString( unsigned  long cmd );
void IpMsgDumpHostList( const char *s, HostList& hostList );
#else
#define IpMsgPrintBuf( bufname, buf,size )
#define IpMsgDumpPacket( packet, sender_addr )
#define GetCommandString( cmd )
#define IpMsgDumpHostList( s, hostList );
#endif

#endif
