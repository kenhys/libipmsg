#ifndef _IPMESSENGER_IMPL_H_
#define _IPMESSENGER_IMPL_H_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef __sun
#include <sys/sockio.h>
#endif
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
#include <deque>
#include <map>
#include <vector>

const int IPMSG_DEFAULT_PORT=0x0979;

namespace ipmsg {

extern "C" void *GetFileDataThread( void *param );
extern "C" void *GetDirFilesThread( void *param );

class MacAddressTableItem {
	public:
		IPMSG_PROPERTY( std::string, MacAddress );
		IPMSG_PROPERTY( std::string, IpAddress );
};

/**
 * IP Messenger エージェント実装クラス。(ライブラリ内部使用)
 */
class IpMessengerAgentImpl {
	public:
		friend class IpMessengerAgent;
		friend class RecievedMessage;
		friend class SentMessage;
		friend class HostList;
		friend class HostListItem;
		friend void *ipmsg::GetFileDataThread( void *param );
		friend void *ipmsg::GetDirFilesThread( void *param );

		IPMSG_READONLY_PROPERTY( std::string, LoginName );
		IPMSG_READONLY_PROPERTY( std::string, HostName );
		IPMSG_PROPERTY( bool, IsDialup );
		IPMSG_PROPERTY( bool, UseIPv6 );
		IPMSG_PROPERTY( bool, AbortDownloadAtFileChanged );
		IPMSG_PROPERTY( bool, SaveSentMessage );
		IPMSG_PROPERTY( bool, SaveRecievedMessage );
		IPMSG_PROPERTY( bool, NoSendMessageOnEncryptionFailed );
		IPMSG_PROPERTY( int, DefaultPortNo );

		static IpMessengerAgentImpl *GetInstance();
		static void Release();
		static void GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo=IPMSG_DEFAULT_PORT );
		void ClearBroadcastAddress();
		void DeleteBroadcastAddress( std::string addr );
		void AddBroadcastAddress( std::string addr );
		void Login( std::string nickname, std::string groupName );
		void Logout();
		HostList& GetHostList();
		HostList& UpdateHostList( bool isRetry=false );
		bool SendMsg( HostListItem host, std::string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, bool noLogging=false, unsigned long opt=0UL );
		bool SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFile& file, bool isLockPassword=false, int hostCountAtSameTime=1, bool noLogging=false, unsigned long opt=0UL );
		bool SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFileList& files, bool isLockPassword=false, int hostCountAtSameTime=1, bool noLogging=false, unsigned long opt=0UL, bool isRetry = false, unsigned long PrevPacketNo = 0UL );
		void ResetAbsence();
		void SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes );
		std::vector<GroupItem> GetGroupList();
		std::string GetInfo( HostListItem& host );
		std::string GetAbsenceInfo( HostListItem& host );
		int Process();
		int GetRecievedMessageCount();
		RecievedMessage PopRecievedMessage();
		void ConfirmMessage( RecievedMessage &msg );
		void DeleteNotify( RecievedMessage msg );
		void AcceptConfirmNotify( SentMessage msg );
		SentMessageList *GetSentMessages();
		SentMessageList CloneSentMessages() const;
		void SetSortHostListComparator( const HostListComparator *comparator );
		HostListComparator *GetSortHostListComparator() const;
		void SetEventObject( const IpMessengerEvent *evt );
		IpMessengerEvent *GetEventObject() const;
		void SetFileNameConverter( const FileNameConverter *conv );
		FileNameConverter *GetFileNameConverter() const;
		bool IsAbsence() const;
		void StartNetwork( const std::vector<NetworkInterface>& nics );
		void StartNetwork();
		void StopNetwork();
		void RestartNetwork( const std::vector<NetworkInterface>& nics );
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
		void GetPubKey( const struct sockaddr_storage &address );
#endif
		IpMessengerEvent *event;
		HostListComparator *compare;
		SentMessageList sentMsgList;
		RecievedMessageList recvMsgList;
		bool _IsAbsence;
		bool networkStarted;
		FileNameConverter *converter;
		std::vector<AbsenceMode> absenceModeList;
		std::string DecryptErrorMessage;
		std::string Nickname;
		std::string GroupName;
		std::string HostAddress;

		std::map<int, NetworkInterface> sd_addr;
		std::map<int, bool> sd_is_broadcast;
		std::map<int, bool> sd_address_family;
		std::vector<int> tcp_sd;
		std::vector<int> udp_sd;
		int max_sd;
		std::vector<struct sockaddr_in> addr_recv;
		struct timeval tv;
		fd_set rfds;
		std::vector<struct sockaddr_storage> broadcastAddr;
		std::vector<Packet> PacketsForChecking;
		HostList appearanceHostList;
		HostList hostList;
		HostList hostListBackup;
		std::vector<NetworkInterface> NICs;
		std::string localEncoding;
		bool haveIPv4Nic;
		bool haveIPv6Nic;

		IpMessengerAgentImpl();
		~IpMessengerAgentImpl();
		void CryptoInit();
		void CryptoEnd();
		void NetworkInit( const std::vector<NetworkInterface>& nics );
		void NetworkEnd();
		void InitSend( const std::vector<NetworkInterface>& nics );
		void InitRecv( const std::vector<NetworkInterface>& nics );
		int InitUdpRecv( struct sockaddr_storage addr, const char *devname );
		int InitTcpRecv( struct sockaddr_storage addr, const char *devname );
		int RecvPacket();
		bool RecvUdp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *udp_socket );
		bool RecvTcp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *tcp_socket );
		bool FindDuplicatePacket( const Packet &packet );
		void PurgePacket( time_t nowTime );
		void CheckSendMsgRetry( time_t nowTime );
		void CheckGetHostListRetry( time_t nowTime );
		void UdpSendto( const int send_socket, struct sockaddr_storage *addr, char *buf, int size );
		void SendPacket( const int send_socket, const unsigned long cmd, char *buf, int size, struct sockaddr_storage toAddr );
		void SendBroadcast( const unsigned long cmd, char *buf, int size );
		void DoRecvCommand( const Packet& packet );
		int SendNoOperation();
		int SendAbsence();
		int UdpRecvEventNoOperation( const Packet& packet );
		int UdpRecvEventBrEntry( const Packet& packet );
		int UdpRecvEventBrExit( const Packet& packet );
		int UdpRecvEventBrAbsence( const Packet& packet );
		int UdpRecvEventBrIsGetList( const Packet& packet );
		int UdpRecvEventBrIsGetList2( const Packet& packet );
		int UdpRecvEventOkGetList( const Packet& packet );
		int UdpRecvEventGetList( const Packet& packet );
		int UdpRecvEventAnsEntry( const Packet& packet );
		int UdpRecvEventAnsList( const Packet& packet );
		int UdpRecvEventSendMsg( const Packet& packet );
		int UdpRecvEventRecvMsg( const Packet& packet );
		int UdpRecvEventDelMsg( const Packet& packet );
		int UdpRecvEventReadMsg( const Packet& packet );
		int UdpRecvEventAnsReadMsg( const Packet& packet );
		int UdpRecvEventGetInfo( const Packet& packet );
		int UdpRecvEventSendInfo( const Packet& packet );
		int UdpRecvEventGetAbsenceInfo( const Packet& packet );
		int UdpRecvEventSendAbsenceInfo( const Packet& packet );
		int UdpRecvEventGetPubKey( const Packet& packet );
		int UdpRecvEventAnsPubKey( const Packet& packet );
		int TcpRecvEventGetFileData( const Packet& packet );
		int UdpRecvEventReleaseFiles( const Packet& packet );
		int TcpRecvEventGetDirFiles( const Packet& packet );
		int AddDefaultHost();
		int CreateHostList( const char *packetIpAddress, const char *packetHostName, const char *hostListBuf, int bufLen );
		Packet DismantlePacketBuffer( int sock, char *packetBuf, int size, struct sockaddr_storage sender, time_t nowTime );
		int CreateAttachedFileList( const char *option, AttachFileList &files );
		bool EncryptMsg( const HostListItem &host, unsigned char *optBuf, int optBufLen, int *encOptBufLen, int optSize );
		bool DecryptMsg( const Packet &packet, std::string& msg );
		std::vector<struct sockaddr_storage>::iterator FindBroadcastNetworkByAddress( std::string addr );
		unsigned long AddCommonCommandOption( const unsigned long cmd );
		bool IsFileChanged( time_t mtime, unsigned long long size, struct stat statInit, struct stat statProgress );

		//Library Use Only
		int AddHostListFromPacket( const Packet& packet );
		int CreateNewPacketBuffer( unsigned long cmd, unsigned long packet_no, std::string user, std::string host, const char *opt, int optLen, char *buf, int size );
		int CreateNewPacketBuffer( unsigned long cmd, std::string user, std::string host, const char *opt, int optLen, char *buf, int size );
		void SendTcpPacket( int sd, char *buf, int size );
		bool SendFile( int sock, std::string FileName, time_t mtime, unsigned long long size, AttachFile *trans, off_t offset=0 );
		bool SendDirData( int sock, std::string cd, std::string dir, std::vector<std::string> &files );
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

#define	IP_ADDR_MAX_SIZE	INET6_ADDRSTRLEN + 1

//Network.cpp
int bindSocket( int proto, struct sockaddr_storage addr, const char *devname );
int sendToSockAddrIn( int sock, const char *buf, const int size, const struct sockaddr_storage *addr );
bool isSiteLocal( struct sockaddr_storage *addr );
bool isLinkLocal( struct sockaddr_storage *addr );
void setScopeId( struct sockaddr_storage *addr, int scope_id );
int getScopeId( struct sockaddr_storage *addr );
bool isLocalLoopbackAddress( struct sockaddr_storage *addr );
struct sockaddr_storage * createSockAddrIn( struct sockaddr_storage *addr, std::string rawAddress, int port, const char * devname=NULL );
std::string getNetworkInterfaceMacAddress( std::string deviceName );
char *convertMacAddressToBuffer( const unsigned char *mac, char *buf, int bufsize );
std::string convertIpAddressToMacAddress( std::string ipAddress, const std::vector<NetworkInterface>& nics );
int getSockAddrInPortNo( const struct sockaddr_storage *addr );
int getSockAddrInPortNo( const struct sockaddr_storage &addr );
std::string getSockAddrInRawAddress( const struct sockaddr_storage *addr );
std::string getSockAddrInRawAddress( const struct sockaddr_storage &addr );
std::string getSockAddrInAddressFamilyString( const struct sockaddr_storage &addr );
std::string getAddressFamilyString( int family );
std::string getLocalhostAddress( bool useIpv6, const std::vector<NetworkInterface>& nics );
bool isSameNetwork( const struct sockaddr_storage *addr, std::string ifnetaddr, std::string netmask );
bool isSameSockAddrIn( struct sockaddr_storage base, struct sockaddr_storage check );
void getNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo );
#ifndef HAVE_GETIFADDR
void getNetworkInterfaceInfoForIPv4( std::vector<NetworkInterface>& nics, int defaultPortNo );
void getNetworkInterfaceInfoForIPv6( std::vector<NetworkInterface>& nics, int defaultPortNo );
#endif
std::string getBroadcastAddress( int family, std::string netAddress, std::string netmask );
std::string getNetworkAddress( int family, std::string rawAddress, std::string netmask );

//PrivateUtils.cpp
int IpMsgSendFileBuffer( int ifd, int sock, int size );

#if defined(DEBUG) || defined(INFO)
void IpMsgPrintBuf( const char* bufname, const char *buf, const int size );
void IpMsgDumpPacket( ipmsg::Packet packet, struct sockaddr_storage *sender_addr );
std::string GetCommandString( unsigned  long cmd );
void IpMsgDumpHostList( const char *s, ipmsg::HostList& hostList );
void IpMsgDumpAddr( const struct sockaddr_storage *addr );
#else
#define IpMsgPrintBuf( bufname, buf,size )
#define IpMsgDumpPacket( packet, sender_addr )
#define GetCommandString( cmd )
#define IpMsgDumpHostList( s, hostList )
#define IpMsgDumpAddr( addr )
#endif
int IpMsgMutexInit( const char *pos, pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr );
int IpMsgMutexLock( const char *pos, pthread_mutex_t *mutex );
int IpMsgMutexUnlock( const char *pos, pthread_mutex_t *mutex );
int IpMsgMutexDestroy( const char *pos, pthread_mutex_t *mutex );
int IpMsgIntToString( char *buf, ssize_t bufsize, int val );
int IpMsgULongToString( char *buf, ssize_t bufsize, unsigned long val );
int IpMsgUCharToHexString( char buf[3], const unsigned char val );
std::string IpMsgPortToStr( int portNo );
std::string IpMsgGetLoginName( uid_t uid );
std::string IpMsgGetHostName();
#ifdef DEBUG_TRACE
void IpMsgCallTraceEnter( const char* funcname );
void IpMsgCallTraceExit( const char* funcname );
#endif
}; //namespace ipmsg

#ifdef DEBUG_TRACE
#define	IPMSG_FUNC_ENTER( __func__ )				const char *__func_name__ = __func__; \
													ipmsg::IpMsgCallTraceEnter( __func_name__ )
#define	IPMSG_FUNC_EXIT								ipmsg::IpMsgCallTraceExit( __func_name__ ); \
													return
#define	IPMSG_FUNC_RETURN( __val__ )				ipmsg::IpMsgCallTraceExit( __func_name__ ); \
													return __val__
#else
#define	IPMSG_FUNC_ENTER( __func__ )
#define	IPMSG_FUNC_EXIT					return
#define	IPMSG_FUNC_RETURN( __val__ )	return __val__
#endif

#endif
