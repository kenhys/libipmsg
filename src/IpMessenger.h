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

/**
 * �ɤ߹������ѥץ�ѥƥ�
 * IPMSG_READONLY_PROPERTY( SomeClass, PropName )��
 * -------------------------------------------------------------------
 *   private: SomeClass _PropName;
 *   public: SomeClass& PropName(){ return _PropName; };
 * -------------------------------------------------------------------
 * ��Ÿ������ޤ���
 **/
#define IPMSG_READONLY_PROPERTY(t, name ) private: t _##name; \
								public: t name() const { return _##name; };

/**
 * �ɤ߽�ξ�ѥץ�ѥƥ�
 * IPMSG_PROPERTY( SomeClass, PropName )��
 * -------------------------------------------------------------------
 *   private: SomeClass _PropName;
 *   public: SomeClass PropName(){ return _PropName; };
 *           void setPropName( SomeClass val ){ _PropName = val; };
 * -------------------------------------------------------------------
 * ��Ÿ������ޤ���
 **/
#define IPMSG_PROPERTY(t, name ) private: t _##name; \
								public: t name() const { return _##name; };\
								void set##name( const t val ){ _##name = val; };

/**
 * �ɤ߽�ξ�ѥץ�ѥƥ�(������)
 * IPMSG_PROPERTY_REF( SomeClass, PropName )��
 * -------------------------------------------------------------------
 *   private: SomeClass _PropName;
 *   public: SomeClass& PropName(){ return _PropName; };
 *           void setPropName( SomeClass& val ){ _PropName = val; };
 * -------------------------------------------------------------------
 * ��Ÿ������ޤ���
 **/
#define IPMSG_PROPERTY_REF(t, name ) private: t _##name; \
									public: t& name() { return _##name; };\
									void set##name( t& val ){ _##name = val; };

class Packet{
	public:
		IPMSG_PROPERTY( unsigned long, VersionNo );
		IPMSG_PROPERTY( unsigned long, PacketNo );
		IPMSG_PROPERTY( unsigned long, CommandMode );
		IPMSG_PROPERTY( unsigned long, CommandOption );
		IPMSG_PROPERTY( time_t, Recieved );
		IPMSG_PROPERTY( string, HostName );
		IPMSG_PROPERTY( string, UserName );
		IPMSG_PROPERTY( string, Option );
		IPMSG_PROPERTY( struct sockaddr_in, Addr );
		IPMSG_PROPERTY( int, TcpSocket );
};

class NetworkInterface {
	public:
		IPMSG_PROPERTY( string, DeviceName );
		IPMSG_PROPERTY( string, IpAddress );
		IPMSG_PROPERTY( int, PortNo );
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
		bool IsFileAttachSupport();
		bool IsEncryptSupport();
		bool IsAbsence();
		bool Equals( HostListItem item );
		int Compare( HostListItem item );
};

class HostListComparator{
	public:
		virtual int compare( vector<HostListItem>::iterator host1, vector<HostListItem>::iterator host2 )=0;
};

class HostListDefaultComparator: public HostListComparator{
	public:
		virtual int compare( vector<HostListItem>::iterator host1, vector<HostListItem>::iterator host2 ){
			return host1->Compare( *host2 );
		};
};

class HostList{
	public:
		IPMSG_PROPERTY( bool, IsAsking );
		IPMSG_PROPERTY( time_t, AskStartTime );
		IPMSG_PROPERTY( time_t, PrevTry );
		IPMSG_PROPERTY( int, RetryCount );
		void AddHost( HostListItem host );
		void Delete( vector<HostListItem>::iterator &it );
		void DeleteHost( string hostname );
		vector<HostListItem>::iterator FindHostByAddress( string addr );
		vector<HostListItem>::iterator FindHostByHostName( string hostName );
		static HostListItem CreateHostListItemFromPacket( Packet packet );
		vector<HostListItem>::iterator begin(){ return items.begin(); };
		vector<HostListItem>::iterator end(){ return items.end(); };
		int size(){ return items.size(); };
		void clear(){ return items.clear(); };
		string ToString( int start );
		void sort( HostListComparator *comparator );
	private:
		void qsort( HostListComparator *comparator, int left, int right );
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
		IPMSG_PROPERTY( unsigned long long, Size );
		IPMSG_PROPERTY( time_t, Time );
		IPMSG_PROPERTY( long, FileCount );
		IPMSG_PROPERTY( string, LocalFileName );
		IPMSG_PROPERTY_REF( AttachFile, File );
		DownloadInfo():_Size( 0ULL ), _Time( 0 ), _FileCount( 0L ), _LocalFileName(""){};
#if 0
		DownloadInfo():_Size( 0ULL ), _Time( 0 ), _FileCount( 0L ), _LocalFileName(""){
			setSize( 0LL );
			setTime( 0 );
			setFileCount( 0L );
		};
#endif
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
		vector<AttachFile>::iterator erase( vector<AttachFile>::iterator item ) { return files.erase( item ); };
		vector<AttachFile>::iterator erase( AttachFile &item ) { return erase( FindByFileId( item.FileId() ) ); };
		vector<AttachFile>::iterator FindByFullPath( string fullPath );
		vector<AttachFile>::iterator FindByFileId( int file_id );
	private:
		vector<AttachFile> files;
};

class IpMessengerEvent;

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
		bool DownloadFile( AttachFile &file, string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		bool DownloadDir( AttachFile &file, string saveDirName, string saveBaseDir, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
	private:
		bool DownloadFilePrivate( IpMessengerEvent *event, AttachFile &file, string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		bool DownloadDirPrivate( IpMessengerEvent *event, AttachFile &file, string saveDirName, string saveBaseDir, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		string GetFormalDir( string dirName );
		string GetSaveDir( string saveName, string saveBaseDir );
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
		IPMSG_PROPERTY( int, HostCountAtSameTime );
		IPMSG_PROPERTY( unsigned long, Opt );
		IPMSG_PROPERTY_REF( AttachFileList, Files );
		bool isRetryMaxOver();
		bool needSendRetry( time_t tryNow );
		vector<AttachFile>::iterator FindAttachFileByPacket( Packet packet );
};

class SentMessageList {
	public:
		vector<SentMessage>::iterator begin(){ return messages.begin(); };
		vector<SentMessage>::iterator end(){ return messages.end(); };
		vector<SentMessage>::iterator erase( vector<SentMessage>::iterator item ){ return messages.erase( item ); };
		vector<SentMessage>::iterator FindSentMessageByPacketNo( unsigned long PacketNo );
		vector<SentMessage>::iterator FindSentMessageByPacket( Packet packet );
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


/**
 * IP Messenger ���٥�ȥ��֥�������
 * �ʳ�Application�Ϥ��Υ��֥������Ȥ�Ѿ����ƽ�����������Ƥ�����������
 **/
class IpMessengerEvent {
	public:
		//�ۥ��ȥꥹ�ȹ�����
		virtual void UpdateHostListAfter( HostList& hostList )=0;
		//�ۥ��ȥꥹ�ȼ�����ȥ饤���顼
		virtual bool GetHostListRetryError()=0;
		//��å�����������(�������ƥ�å���������¸�����פʤ�TRUE���֤�)
		virtual bool RecieveAfter( RecievedMessage& msg )=0;
		//��å�����������
		virtual void SendAfter( SentMessage& msg )=0;
		//��å�����������ȥ饤���顼
		virtual bool SendRetryError( SentMessage& msg )=0;
		//�������θ�
		virtual void OpenAfter( SentMessage& msg )=0;
		//��������ɳ���
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//��������ɽ�����
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//��������ɽ�λ
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//��������ɥ��顼(��ȥ饤�������TRUE���֤���)
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//�ۥ��Ȥλ������θ�
		virtual void EntryAfter( HostList& hostList )=0;
		//�ۥ��Ȥ�æ�����θ�
		virtual void ExitAfter( HostList& hostList )=0;
		virtual ~IpMessengerEvent()=0;
};

class IpMessengerAgentImpl;

/**
 * IP Messenger �����������
 **/
class IpMessengerAgent {
	public:
		friend class RecievedMessage;
		friend class SentMessage;
		friend class HostList;
		friend void *GetFileDataThread( void *param );
		friend void *GetDirFilesThread( void *param );

		/**
		 * ���󥹥��󥹤����
		 **/
		static IpMessengerAgent *GetInstance();

		/**
		 * ���󥹥��󥹤����
		 **/
		static void Release();

		/**
		 * NIC�ξ�������
		 **/
		static void GetNetworkInterfaceInfo( vector<NetworkInterface>& nics );

		/**
		 * �֥��ɥ��㥹�ȥ��ɥ쥹�Υꥹ�Ȥ򥯥ꥢ
		 **/
		void ClearBroadcastAddress();

		/**
		 * �֥��ɥ��㥹�ȥ��ɥ쥹�Υꥹ�Ȥ��饢�ɥ쥹����
		 **/
		void DeleteBroadcastAddress( string addr );

		/**
		 * �֥��ɥ��㥹�ȥ��ɥ쥹�Υꥹ�Ȥ˥��ɥ쥹���ɲ�
		 **/
		void AddBroadcastAddress( string addr );

		/**
		 * ���������Ρʻ������Ρ�
		 **/
		void Login( string nickname, string groupName );

		/**
		 * �����������Ρ�æ�����Ρ�
		 **/
		void Logout();

		/**
		 * �ۥ��ȥꥹ�Ȥ����
		 **/
		HostList& GetHostList();

		/**
		 * �ۥ��ȥꥹ�Ȥ򹹿����Ƽ���
		 **/
		HostList& UpdateHostList();

		/**
		 * ��å�����������ź��̵����
		 **/
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, unsigned long opt=0UL );

		/**
		 * ��å����������ʰ��ź�ա�
		 **/
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword=false, int hostCountAtSameTime=1, unsigned long opt=0UL );

		/**
		 * ��å�����������ʣ��ź�ա�
		 **/
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword=false, int hostCountAtSameTime=1, unsigned long opt=0UL );

		/**
		 * �Ժ߲��
		 **/
		void ResetAbsence();

		/**
		 * �Ժ�����
		 **/
		void SetAbsence( string encoding, vector<AbsenceMode> absenceModes );

		/**
		 * ���롼�װ�������
		 **/
		vector<string> GetGroupList();

		/**
		 * �С������������
		 **/
		string GetInfo( HostListItem host );

		/**
		 * �Ժ߾������
		 **/
		string GetAbsenceInfo( HostListItem host );

		/**
		 * �ѥ��åȤν����ʥݡ�����ѡ�
		 **/
		int Process();

		/**
		 * �����ѥ�å������θĿ�����
		 **/
		int GetRecievedMessageCount();

		/**
		 * ������å������μ��Ф�
		 **/
		RecievedMessage PopRecievedMessage();

		/**
		 * �������˳������Τ�����
		 **/
		void ConfirmMessage( RecievedMessage &msg );

		/**
		 * �������˥�å�����������Ѵ��ˤ�����
		 **/
		void DeleteNotify( RecievedMessage msg );

		/**
		 * �����ѥ�å������˳����ѤǤ��뤳�Ȥ�ޡ���
		 **/
		void AcceptConfirmNotify( SentMessage msg );

		/**
		 * ������å������ꥹ�Ȥμ���
		 **/
		SentMessageList *GetSentMessages();

		/**
		 * ������å������ꥹ�ȤΥ��ԡ��μ���
		 **/
		SentMessageList CloneSentMessages();

		/**
		 * �ۥ��ȥꥹ����ӥ��֥������Ȥ�����
		 **/
		void SetSortHostListComparator( HostListComparator *comparator );

		/**
		 * �ۥ��ȥꥹ����ӥ��֥������Ȥμ���
		 **/
		HostListComparator *GetSortHostListComparator();

		/**
		 * ���٥�ȥ��֥������Ȥ�����
		 **/
		void SetEventObject( IpMessengerEvent *evt );

		/**
		 * ���٥�ȥ��֥������Ȥμ���
		 **/
		IpMessengerEvent *GetEventObject();

		/**
		 * �ե����륳��С�����������
		 **/
		void SetFileNameConverter( FileNameConverter *conv );

		/**
		 * �ե����륳��С������μ���
		 **/
		FileNameConverter *GetFileNameConverter();

		/**
		 * �Ժߡ�
		 **/
		bool IsAbsence();

		/**
		 * �ͥåȥ���κƵ�ư
		 **/
		void RestartNetwork();

		/**
		 * ������̾
		 **/
		string LoginName();

		/**
		 * �ۥ���̾
		 **/
		string HostName();

		/**
		 * ������륢�å�
		 **/
		bool IsDialup();
		void setIsDialup( bool isDialup );

		/**
		 * �ե����뤬�ѹ����줿���������ɤ����Ǥ���
		 **/
		bool AbortDownloadAtFileChanged();
		void setAbortDownloadAtFileChanged( bool isAbort );
		/* �����ߴ����ΰ�(0.1.0) */
		bool GetAbortDownloadAtFileChanged(){ return AbortDownloadAtFileChanged(); };
		void SetAbortDownloadAtFileChanged( bool isAbort ){ setAbortDownloadAtFileChanged( isAbort ); };

		/**
		 * ������å������򵭲�����
		 **/
		bool SaveSentMessage();
		void setSaveSentMessage( bool isSave );

		/**
		 * ������å������򵭲�����
		 **/
		bool SaveRecievedMessage();
		void setSaveRecievedMessage( bool isSave );

	private:
		IpMessengerAgent();
		~IpMessengerAgent();
		IpMessengerAgentImpl *ipmsgImpl;
};

#endif
