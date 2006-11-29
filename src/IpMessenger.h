#ifndef _IPMESSENGER_H_
#define _IPMESSENGER_H_
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

#define IPMSG_PROPERTY(t, name ) private: t _##name; \
								public: t name() {return _##name; };\
								void set##name(t val){ _##name = val; };

#define IPMSG_PROPERTY_REF(t, name ) private: t _##name; \
									public: t& name() {return _##name; };\
									void set##name(t& val){ _##name = val; };
class Packet{
	public:
		IPMSG_PROPERTY( unsigned long, VersionNo );
		IPMSG_PROPERTY( unsigned long, PacketNo );
		IPMSG_PROPERTY( unsigned long, CommandMode );
		IPMSG_PROPERTY( unsigned long, CommandOption );
		IPMSG_PROPERTY( string, HostName );
		IPMSG_PROPERTY( string, UserName );
		IPMSG_PROPERTY( string, Option );
		IPMSG_PROPERTY( struct sockaddr_in, Addr );
		IPMSG_PROPERTY( int, TcpSocket );

};

class NetworkInterface {
	public:
		NetworkInterface( string deviceName, string ipAddress){
			setDeviceName( deviceName );
			setDeviceName( ipAddress );
		};
		IPMSG_PROPERTY( string, DeviceName );
		IPMSG_PROPERTY( string, IpAddress );
};

class HostListItem{
	public:
		IPMSG_PROPERTY( string, Version );
		IPMSG_PROPERTY( string, AbsenceDescription );
		IPMSG_PROPERTY( string, UserName );
		IPMSG_PROPERTY( string, HostName );
		IPMSG_PROPERTY( unsigned long, CommandNo );
		IPMSG_PROPERTY( string, IpAddress );
		IPMSG_PROPERTY( string, Nickname );
		IPMSG_PROPERTY( string, GroupName );
		IPMSG_PROPERTY( string, EncodingName );
		IPMSG_PROPERTY( string, Priority );
		IPMSG_PROPERTY( unsigned long, PortNo );
		IPMSG_PROPERTY( unsigned long, EncryptionCapacity );
		IPMSG_PROPERTY( string, PubKeyHex );
		IPMSG_PROPERTY( string, EncryptMethodHex );
		bool Equals( HostListItem item );
};

class HostList{
	public:
		void AddHost( HostListItem host );
		void DeleteHost( string hostname );
		static HostListItem CreateHostListItemFromPacket( Packet packet );
		vector<HostListItem>::iterator begin(){ return items.begin(); };
		vector<HostListItem>::iterator end(){ return items.end(); };
		int size(){ return items.size(); };
		void clear(){ return items.clear(); };
		string ToString( int start );
	private:
		vector<HostListItem>items;
};

class FileNameConverter {
	public:
		virtual string ConvertNetworkToLocal( string original_file_name ) = 0;
		virtual string ConvertLocalToNetwork( string original_file_name ) = 0;
};

class NullFileNameConverter:public FileNameConverter {
	public:
		virtual string ConvertNetworkToLocal( string original_file_name ){ return original_file_name; };
		virtual string ConvertLocalToNetwork( string original_file_name ){ return original_file_name; };
};

class AttachFile{
	public:
		map<string, vector<unsigned long> >::iterator beginExtAttrs(){ return _ExtAttrs.begin(); };
		map<string, vector<unsigned long> >::iterator endExtAttrs(){ return _ExtAttrs.end(); };
		void addExtAttrs( string key, unsigned long val ){ _ExtAttrs[key].push_back( val ); };
		IPMSG_PROPERTY( int, FileId );
		IPMSG_PROPERTY( string, FullPath );
		IPMSG_PROPERTY( string, FileName );
		IPMSG_PROPERTY( string, Location );
		IPMSG_PROPERTY( long long, FileSize );
		IPMSG_PROPERTY( bool, IsDownloaded );
		IPMSG_PROPERTY( bool, IsDownloading );
		IPMSG_PROPERTY( time_t, MTime );
		IPMSG_PROPERTY( unsigned long, Attr );

		AttachFile();
		bool IsRegularFile();
		bool IsDirectory();
		void GetLocalFileInfo();
	private:
		map<string, vector<unsigned long> > _ExtAttrs;
	public:
		static AttachFile AnalyzeHeader( char *buf, FileNameConverter *conv );
		static string CreateDirFullPath( vector<string> dirstack );
};

#define IPMSG_SIZE_B	(long double)(1)
#define IPMSG_SIZE_KB	(long double)(1024 * IPMSG_SIZE_B)
#define IPMSG_SIZE_MB	(long double)(1024 * IPMSG_SIZE_KB)
#define IPMSG_SIZE_GB	(long double)(1024 * IPMSG_SIZE_MB)
#define IPMSG_SIZE_TB	(long double)(1024 * IPMSG_SIZE_GB)
class DownloadInfo{
	public:
		IPMSG_PROPERTY( long long, Size );
		IPMSG_PROPERTY( time_t, Time );
		IPMSG_PROPERTY( long, FileCount );
		DownloadInfo(){
			setSize( 0LL );
			setTime( 0 );
			setFileCount( 0L );
		};
		long double getSpeed(){ return Time() == 0 ? (long double)0 : ( ( long double )Size() / ( long double )Time() ); };
		string getSpeedString() { return getUnitSizeString( ( long long )getSpeed() ) + "/sec"; };
		string getSizeString() { return getUnitSizeString( Size() ); };
	private:
		string getUnitSizeString( long long size )
		{
			long double dsize = (long double)size;
			char buf[100];
			if ( dsize >= IPMSG_SIZE_TB ) {
				snprintf( buf, sizeof( buf ), "%.2Lf TB", (dsize / IPMSG_SIZE_TB) );
				return buf;
			} else if ( dsize >= IPMSG_SIZE_GB ) {
				snprintf( buf, sizeof( buf ), "%.2Lf GB", (dsize / IPMSG_SIZE_GB) );
				return buf;
			} else if ( dsize >= IPMSG_SIZE_MB ) {
				snprintf( buf, sizeof( buf ), "%.2Lf MB", (dsize / IPMSG_SIZE_MB) );
				return buf;
			} else if ( dsize >= IPMSG_SIZE_KB ) {
				snprintf( buf, sizeof( buf ), "%.2Lf KB", (dsize / IPMSG_SIZE_KB) );
				return buf;
			}
			snprintf( buf, sizeof( buf ), "%.2Lf B", dsize );
			return buf;
		};
};

class AttachFileList{
	public:
		inline void AddFile( AttachFile& file ){
			files.push_back( file );
		};
		vector<AttachFile>::iterator begin() { return files.begin(); };
		vector<AttachFile>::iterator end() { return files.end(); };
		int size() { return files.size(); };
		void clear() { return files.clear(); };
		vector<AttachFile>::iterator erase(vector<AttachFile>::iterator item) { return files.erase( item ); };
		vector<AttachFile>::iterator FindByFullPath( string fullPath );
		vector<AttachFile>::iterator FindByFileId( int file_id );
	private:
		vector<AttachFile> files;
};

class RecievedMessage{
	public:
		IPMSG_PROPERTY( Packet, MessagePacket );
		IPMSG_PROPERTY( string, Message );
		IPMSG_PROPERTY( time_t, Recieved );
		IPMSG_PROPERTY( bool, IsConfirmed );
		IPMSG_PROPERTY( bool, IsSecret );
		IPMSG_PROPERTY( bool, IsCrypted );
		IPMSG_PROPERTY( HostListItem, Host );
		IPMSG_PROPERTY( bool, IsPasswordLock );
		IPMSG_PROPERTY( bool, IsBroadcast );
		IPMSG_PROPERTY( bool, IsMulticast );
		IPMSG_PROPERTY( bool, HasAttachFile );
		IPMSG_PROPERTY_REF( AttachFileList, Files );
		bool DownloadFile( AttachFile &file, string saveFileNameFullPath, DownloadInfo& info );
		bool DownloadDir( AttachFile &file, string saveDirName, string saveBaseDir, DownloadInfo& info );
		bool DownloadDir( AttachFile &file, string saveDirName, string saveBaseDir, DownloadInfo& info, FileNameConverter *conv );
};

class RecievedMessageList {
	public:
		vector<RecievedMessage>::iterator begin(){ return messages.begin(); };
		vector<RecievedMessage>::iterator end(){ return messages.end(); };
		vector<RecievedMessage>::iterator erase( vector<RecievedMessage>::iterator item ){ return messages.erase( item ); };
		void append( RecievedMessage item ){ messages.push_back( item ); };
		int size(){ return messages.size(); };
		void clear(){ return messages.clear(); };
	private:
		vector<RecievedMessage> messages;
};

class SentMessage{
	public:
		IPMSG_PROPERTY( struct sockaddr_in, To );
		IPMSG_PROPERTY( HostListItem, Host );
		IPMSG_PROPERTY( unsigned long, PacketNo );
		IPMSG_PROPERTY( string, Message );
		IPMSG_PROPERTY( time_t, Sent );
		IPMSG_PROPERTY( time_t, PrevTry );
		IPMSG_PROPERTY( bool, IsRetryMaxOver );
		IPMSG_PROPERTY( bool, IsSent );
		IPMSG_PROPERTY( bool, IsPasswordLock );
		IPMSG_PROPERTY( bool, IsCrypted );
		IPMSG_PROPERTY( int, RetryCount );
		IPMSG_PROPERTY( bool, IsConfirmed );
		IPMSG_PROPERTY( bool, IsConfirmAnswered );
		IPMSG_PROPERTY( bool, IsSecret );
		IPMSG_PROPERTY_REF( AttachFileList, Files );
		vector<AttachFile>::iterator FindAttachFileByPacket( Packet packet );
};

class SentMessageList {
	public:
		vector<SentMessage>::iterator begin(){ return messages.begin(); };
		vector<SentMessage>::iterator end(){ return messages.end(); };
		vector<SentMessage>::iterator erase( vector<SentMessage>::iterator item ){ return messages.erase( item ); };
		void append( SentMessage item ){ messages.push_back( item ); };
		int size(){ return messages.size(); };
		void clear(){ return messages.clear(); };
		vector<SentMessage> *GetMessageList(){ return &messages; };
	private:
		vector<SentMessage> messages;
};

class AbsenceMode {
	public:
		IPMSG_PROPERTY( string, EncodingName );
		IPMSG_PROPERTY( string, AbsenceName );
		IPMSG_PROPERTY( string, AbsenceDescription );
};

class IpMessengerAgent {
	public:
		friend class RecievedMessage;
		friend class SentMessage;
		friend void *GetFileDataThread( void *param );
		friend void *GetDirFilesThread( void *param );

		static IpMessengerAgent *GetInstance();
		static void Release();
		void ClearBroadcastAddress();
		void DeleteBroadcastAddress( string addr );
		void AddBroadcastAddress( string addr );
		void Login( string nickname, string groupName );
		void Logout();
		void SetAbortDownloadAtFileChanged( bool isAbort ){
			_IsAbortDownloadAtFileChanged = isAbort;
		}
		bool GetAbortDownloadAtFileChanged(){
			return _IsAbortDownloadAtFileChanged;
		}
		void SetSaveSentMessage( bool isSave ){
			_IsSaveSentMessage = isSave;
		}
		bool GetSaveSentMessage(){
			return _IsSaveSentMessage;
		}
		void SetSaveRecievedMessage( bool isSave ){
			_IsSaveRecievedMessage = isSave;
		}
		bool GetSaveRecievedMessage(){
			return _IsSaveRecievedMessage;
		}
		HostList& GetHostList();
		HostList& UpdateHostList();
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, unsigned long opt=0UL );
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword=false, int hostCountAtSameTime=1, unsigned long opt=0UL );
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword=false, int hostCountAtSameTime=1, unsigned long opt=0UL );
		void ResetAbsence();
		void SetAbsence( string encoding, vector<AbsenceMode> absenceModes );
		vector<string> GetGroupList();
		string GetInfo( HostListItem host );
		string GetAbsenceInfo( HostListItem host );
		int Process();
		int GetRecievedMessageCount();
		RecievedMessage PopRecievedMessage();
		void ConfirmMessage( RecievedMessage &msg );
		void DeleteNotify( RecievedMessage msg );
		void AcceptConfirmNotify( SentMessage msg );
		vector<SentMessage> *GetSentMessages();
		vector<SentMessage> CloneSentMessages();
		void SetFileNameConverter( FileNameConverter *conv );
		FileNameConverter *GetFileNameConverter(){ return converter; };
		string GetLoginName(){return LoginName;};
		string GetHostName(){return HostName;};
		bool IsAbsence();

	private:
#ifdef HAVE_OPENSSL
		RSA *RsaMax;
		RSA *RsaMid;
		RSA *RsaMin;
		unsigned long encryptionCapacity;
#endif
		SentMessageList sentMsgList;
		RecievedMessageList recvMsgList;
		bool _IsAbsence;
		bool _IsAbortDownloadAtFileChanged;
		bool _IsSaveSentMessage;
		bool _IsSaveRecievedMessage;
		FileNameConverter *converter;
		vector<AbsenceMode> absenceModeList;
		string DecryptErrorMessage;
		string Nickname;
		string GroupName;
		string LoginName;
		string HostAddress;
		string HostName;

		vector<int> tcp_sd;
		vector<int> udp_sd;
		vector<struct sockaddr_in> addr_recv;
		struct timeval tv;
		fd_set rfds;
		vector<struct sockaddr_in> broadcastAddr;
		HostList hostList;
		string localEncoding;

		IpMessengerAgent();
		~IpMessengerAgent();
		bool isRetryMaxOver( SentMessage msg, int retryCount );
		bool needSendRetry( SentMessage msg, time_t tryNow );
		void CryptoInit();
		void CryptoEnd();
		void NetworkInit();
		void InitSend();
		void InitRecv( vector<NetworkInterface> nics );
		int InitUdpRecv( struct sockaddr_in addr );
		int InitTcpRecv( struct sockaddr_in addr );
		int RecvPacket();
		void SendPacket( char *buf, int size, struct sockaddr_in toAddr );
		void SendBroadcast( char *buf, int size );
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
		int CreateHostList( const char *hostListBuf, int bufLen );
		string GetCommandString(long cmd );
		Packet DismantlePacketBuffer( char *packetBuf, int size, struct sockaddr_in sender );
		int CreateAttachedFileList( const char *option, AttachFileList &files );
		bool EncryptMsg( HostListItem host, unsigned char *optBuf, int optBufLen, int *encOptBufLen, int optSize );
		bool DecryptMsg( Packet &packet );
		vector<struct sockaddr_in>::iterator FindBroadcastNetworkByAddress( string addr );
		vector<HostListItem>::iterator FindHostByAddress( string addr );
		vector<SentMessage>::iterator FindSentMessageByPacketNo( unsigned long PacketNo );

		//Library Use Only
		int CreateNewPacketBuffer(long cmd, long packet_no, string user, string host, const char *opt, int optLen, char *buf, int size );
		int CreateNewPacketBuffer(long cmd, string user, string host, const char *opt, int optLen, char *buf, int size );
		vector<SentMessage>::iterator FindSentMessageByPacket( Packet packet );
		vector<SentMessage>::iterator SentMessageListEnd() { return sentMsgList.end(); };
		void SendTcpPacket( int sd, char *buf, int size );
		bool SendFile( int sock, string FileName, off_t offset=0 );
		bool SendDirData( int sock, string cd, string dir, vector<string> &files );
};

#if defined(DEBUG) || defined(INFO)
void IpMsgPrintBuf( char* bufname, char *buf, int size );
#else
#define IpMsgPrintBuf( bufname, buf,size )
#endif

#endif
