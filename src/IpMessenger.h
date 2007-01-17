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
#define IPMSG_READONLY_PROPERTY(t, name ) \
									private: \
										t _##name; \
									public: \
										inline t name() const { return _##name; };

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
#define IPMSG_PROPERTY(t, name )	private: \
										t _##name; \
									public: \
										inline t name() const { return _##name; }; \
										inline void set##name( const t val ){ _##name = val; };

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
#define IPMSG_PROPERTY_REF(t, name )	\
									private: \
										t _##name; \
									public: \
										inline t& name() { return _##name; }; \
										inline void set##name( const t& val ){ _##name = val; };
/**
 * �ѥ��åȥ��饹
 **/
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

/**
 * �ͥåȥ�����󥿡��ե��������饹
 **/
class NetworkInterface {
	public:
		IPMSG_PROPERTY( string, DeviceName );
		IPMSG_PROPERTY( string, IpAddress );
		IPMSG_PROPERTY( string, NetMask );
		IPMSG_PROPERTY( string, NetworkAddress );
		IPMSG_PROPERTY( string, BroadcastAddress );
		IPMSG_PROPERTY( in_addr_t, NativeIpAddress );
		IPMSG_PROPERTY( in_addr_t, NativeNetMask );
		IPMSG_PROPERTY( in_addr_t, NativeNetworkAddress );
		IPMSG_PROPERTY( in_addr_t, NativeBroadcastAddress );
		IPMSG_PROPERTY( int, PortNo );
};

/**
 * ���롼�ץ��饹
 **/
class GroupItem{
	public:
		IPMSG_PROPERTY( string, GroupName );
		IPMSG_PROPERTY( string, EncodingName );
};

/**
 * �ۥ��ȥ��饹
 **/
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
		bool IsLocalHost() const;
		bool IsFileAttachSupport() const;
		bool IsEncryptSupport() const;
		bool IsAbsence() const;
		bool Equals( const HostListItem& item ) const;
		int Compare( const HostListItem& item ) const;
		void QueryVersionInfo();
		void QueryAbsenceInfo();
};

/**
 * �ۥ��Ȱ�������������ӥ��饹
 *�ʳ�Application��ɬ�פ˱����Ƥ��Υ��饹��Ѿ����ƽ�����������Ƥ�����������
 **/
class HostListComparator{
	public:
		/**
		 * ��ӡ�
		 * @param host1 �ۥ��Ⱦ���1
		 * @param host2 �ۥ��Ⱦ���2
		 * @retval -n:host1���礭��
		 * @retval 0:host1��host2��������
		 * @retval +n:host2���礭��
		 */
		virtual int compare( vector<HostListItem>::iterator host1, vector<HostListItem>::iterator host2 )=0;
};

/**
 * �ۥ��Ȱ����ǥե���ȥ���������ӥ��饹
 **/
class HostListDefaultComparator: public HostListComparator{
	public:
		/**
		 * ��ӡ�
		 * @param host1 �ۥ��Ⱦ���1
		 * @param host2 �ۥ��Ⱦ���2
		 * @retval -n:host1���礭��
		 * @retval 0:host1��host2��������
		 * @retval +n:host2���礭��
		 */
		virtual int compare( vector<HostListItem>::iterator host1, vector<HostListItem>::iterator host2 ){
			return host1->Compare( *host2 );
		};
};

/**
 * �ۥ��Ȱ������饹
 **/
class HostList{
	public:
		IPMSG_PROPERTY( bool, IsAsking );
		IPMSG_PROPERTY( time_t, AskStartTime );
		IPMSG_PROPERTY( time_t, PrevTry );
		IPMSG_PROPERTY( int, RetryCount );
		void AddHost( const HostListItem& host );
		void Delete( vector<HostListItem>::iterator &it );
		void DeleteHost( string hostname );
		vector<HostListItem>::iterator FindHostByAddress( string addr );
		vector<HostListItem>::iterator FindHostByHostName( string hostName );
		static HostListItem CreateHostListItemFromPacket( const Packet& packet );
		vector<HostListItem>::iterator begin();
		vector<HostListItem>::iterator end();
		int size() const;
		void clear();
		string ToString( int start, const struct sockaddr_in *addr );
		void sort( HostListComparator *comparator );
		HostList();
		HostList( const HostList& other );
		~HostList();
		HostList& operator=( const HostList& other );
	private:
		void qsort( HostListComparator *comparator, int left, int right );
		void Lock( const char *pos ) const;
		void Unlock( const char *pos ) const;
		void CopyFrom( const HostList& other );
		vector<HostListItem>items;
		pthread_mutex_t hostListMutex;
};

/**
 * �ե�����̾����С������饹
 * �ʳ�Application��ɬ�פ˱����Ƥ��Υ��饹��Ѿ����ƽ�����������Ƥ�����������
 **/
class FileNameConverter {
	public:
		virtual string ConvertNetworkToLocal( string original_file_name ) = 0;
		virtual string ConvertLocalToNetwork( string original_file_name ) = 0;
};

/**
 * �ե�����̵̾�Ѵ�����С������饹
 **/
class NullFileNameConverter:public FileNameConverter {
	public:
		virtual string ConvertNetworkToLocal( string original_file_name ){ return original_file_name; };
		virtual string ConvertLocalToNetwork( string original_file_name ){ return original_file_name; };
};

/**
 * ź�եե����륯�饹
 **/
class AttachFile{
	public:
		map<string, vector<unsigned long> >::iterator beginExtAttrs() { return _ExtAttrs.begin(); };
		map<string, vector<unsigned long> >::iterator endExtAttrs() { return _ExtAttrs.end(); };
		void addExtAttrs( string key, unsigned long val ){ _ExtAttrs[key].push_back( val ); };
		IPMSG_PROPERTY( int, FileId );
		IPMSG_PROPERTY( string, FullPath );
		IPMSG_PROPERTY( string, FileName );
		IPMSG_PROPERTY( string, Location );
		IPMSG_PROPERTY( long long, FileSize );
		IPMSG_PROPERTY( long long, TransSize );
		IPMSG_PROPERTY( bool, IsDownloaded );
		IPMSG_PROPERTY( bool, IsDownloading );
		IPMSG_PROPERTY( time_t, MTime );
		IPMSG_PROPERTY( unsigned long, Attr );

		AttachFile();
		bool IsRegularFile() const;
		bool IsDirectory() const;
		void GetLocalFileInfo();
	private:
		map<string, vector<unsigned long> > _ExtAttrs;
	public:
		static AttachFile AnalyzeHeader( char *buf, FileNameConverter *conv );
		static string CreateDirFullPath( const vector<string>& dirstack );
};

/**
 * ��������ɾ��󥯥饹
 **/
class DownloadInfo{
	public:
		IPMSG_PROPERTY( unsigned long long, Size );
		IPMSG_PROPERTY( time_t, Time );
		IPMSG_PROPERTY( long, FileCount );
		IPMSG_PROPERTY( bool, Processing );
		IPMSG_PROPERTY( string, LocalFileName );
		IPMSG_PROPERTY_REF( AttachFile, File );

		DownloadInfo():_Size( 0ULL ), _Time( 0 ), _FileCount( 0L ), _LocalFileName(""){};
		long double getSpeed();
		string getSpeedString();
		string getSizeString();
		static string getUnitSizeString( long long size );
};

/**
 * ź�եե�����������饹
 **/
class AttachFileList{
	public:
		void AddFile( const AttachFile& file );
		vector<AttachFile>::iterator begin();
		vector<AttachFile>::iterator end();
		int size() const;
		void clear();
		vector<AttachFile>::iterator erase( vector<AttachFile>::iterator item );
		vector<AttachFile>::iterator erase( const AttachFile& item );
		vector<AttachFile>::iterator FindByFullPath( const string& fullPath );
		vector<AttachFile>::iterator FindByFileId( int file_id );
		AttachFileList();
		AttachFileList( const AttachFileList& other );
		~AttachFileList();
		AttachFileList& operator=( const AttachFileList& other );
	private:
		void Lock( const char *pos ) const;
		void Unlock( const char *pos ) const;
		void CopyFrom( const AttachFileList& other );
		vector<AttachFile> files;
		pthread_mutex_t filesMutex;
};

class IpMessengerEvent;

/**
 * ������å��������饹
 **/
class RecievedMessage{
	public:
		IPMSG_PROPERTY( Packet, MessagePacket );
		IPMSG_PROPERTY( string, Message );
		IPMSG_PROPERTY( time_t, Recieved );
		IPMSG_PROPERTY( bool, IsConfirmed );
		IPMSG_PROPERTY( bool, IsSecret );
		IPMSG_PROPERTY( bool, IsNoLogging );
		IPMSG_PROPERTY( bool, IsCrypted );
		IPMSG_PROPERTY( HostListItem, Host );
		IPMSG_PROPERTY( bool, IsPasswordLock );
		IPMSG_PROPERTY( bool, IsBroadcast );
		IPMSG_PROPERTY( bool, IsMulticast );
		IPMSG_PROPERTY( bool, HasAttachFile );
		IPMSG_PROPERTY_REF( AttachFileList, Files );
		RecievedMessage();
		RecievedMessage( const RecievedMessage& other );
		RecievedMessage& operator=( const RecievedMessage& other );
		bool DownloadFile( AttachFile &file, string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		bool DownloadDir( AttachFile &file, string saveDirName, string saveBaseDir, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
	private:
		void CopyFrom( const RecievedMessage& other );
		bool DownloadFilePrivate( IpMessengerEvent *event, AttachFile &file, string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		bool DownloadDirPrivate( IpMessengerEvent *event, AttachFile &file, string saveDirName, string saveBaseDir, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		string GetFormalDir( string dirName );
		string GetSaveDir( string saveName, string saveBaseDir );
};

/**
 * ������å������������饹
 **/
class RecievedMessageList {
	public:
		vector<RecievedMessage>::iterator begin();
		vector<RecievedMessage>::iterator end();
		vector<RecievedMessage>::iterator erase( vector<RecievedMessage>::iterator item );
		void append( const RecievedMessage &item );
		int size() const;
		void clear();
		RecievedMessageList();
		RecievedMessageList( const RecievedMessageList& other );
		~RecievedMessageList();
		RecievedMessageList& operator=( const RecievedMessageList& other );
	private:
		void Lock( const char *pos ) const;
		void Unlock( const char *pos ) const;
		void CopyFrom( const RecievedMessageList& other );
		vector<RecievedMessage> messages;
		pthread_mutex_t messagesMutex;
};

/**
 * ������å��������饹
 **/
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
		IPMSG_PROPERTY( bool, IsNoLogging );
		IPMSG_PROPERTY( int, HostCountAtSameTime );
		IPMSG_PROPERTY( unsigned long, Opt );
		IPMSG_PROPERTY_REF( AttachFileList, Files );
		SentMessage();
		SentMessage( const SentMessage& other );
		SentMessage& operator=( const SentMessage& other );
		bool isRetryMaxOver() const;
		bool needSendRetry( time_t tryNow ) const;
		vector<AttachFile>::iterator FindAttachFileByPacket( const Packet &packet );
	private:
		void CopyFrom( const SentMessage& other );
};

/**
 * ������å������������饹
 **/
class SentMessageList {
	public:
		vector<SentMessage>::iterator begin();
		vector<SentMessage>::iterator end();
		vector<SentMessage>::iterator erase( vector<SentMessage>::iterator item );
		vector<SentMessage>::iterator FindSentMessageByPacketNo( unsigned long PacketNo );
		vector<SentMessage>::iterator FindSentMessageByPacket( Packet packet );
		void append( const SentMessage &item );
		int size() const;
		void clear();
		vector<SentMessage> *GetMessageList();
		SentMessageList();
		SentMessageList( const SentMessageList& other );
		~SentMessageList();
		SentMessageList& operator=( const SentMessageList& other );

	private:
		void Lock( const char *pos ) const;
		void Unlock( const char *pos ) const;
		void CopyFrom( const SentMessageList& other );
		vector<SentMessage> messages;
		pthread_mutex_t messagesMutex;
};

/**
 * �Ժߥ⡼�ɥ��饹
 **/
class AbsenceMode {
	public:
		IPMSG_PROPERTY( string, EncodingName );
		IPMSG_PROPERTY( string, AbsenceName );
		IPMSG_PROPERTY( string, AbsenceDescription );
};


/**
 * IP Messenger ���٥�ȥ��饹
 * �ʳ�Application�Ϥ��Υ��饹��Ѿ����ƽ�����������Ƥ�����������
 **/
class IpMessengerEvent {
	public:
		/**
		 * �ۥ��ȥꥹ�ȹ����奤�٥��
		 * @param hostList �ۥ��ȥꥹ��
		 */
		virtual void UpdateHostListAfter( HostList& hostList )=0;
		/**
		 * �ۥ��ȥꥹ�ȼ�����ȥ饤���顼���٥��
		 * @retval true:��ȥ饤����
		 * @retval false:��ȥ饤���ʤ�
		 */
		virtual bool GetHostListRetryError()=0;
		/**
		 * ��å����������奤�٥�ȡ�
		 * @param msg ������å�����
		 * @retval true:�������ƥ�å���������¸������
		 * @retval false:��å���������¸
		 */
		virtual bool RecieveAfter( RecievedMessage& msg )=0;
		/**
		 * ��å����������奤�٥��
		 * @param msg ������å�����
		 */
		virtual void SendAfter( SentMessage& msg )=0;
		/**
		 * ��å�����������ȥ饤���顼���٥��
		 * @param msg ������å�����
		 * @retval true:��ȥ饤����
		 * @retval false:��ȥ饤���ʤ�
		 */
		virtual bool SendRetryError( SentMessage& msg )=0;
		/**
		 * �������θ奤�٥��
		 * @param msg ������å�����
		 */
		virtual void OpenAfter( SentMessage& msg )=0;
		/**
		 * ��������ɳ��ϥ��٥��
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 */
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * ��������ɽ����楤�٥��
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 */
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * ��������ɽ�λ���٥��
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 */
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * ��������ɥ��顼���٥��(��ȥ饤�������TRUE���֤���)
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 * @retval true:��ȥ饤����
		 * @retval false:��ȥ饤���ʤ�
		 */
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * �ۥ��Ȥλ������θ奤�٥��
		 * @param hostList �ۥ��ȥꥹ��
		 */
		virtual void EntryAfter( HostList& hostList )=0;
		/**
		 * �ۥ��Ȥ�æ�����θ奤�٥��
		 * @param hostList �ۥ��ȥꥹ��
		 */
		virtual void ExitAfter( HostList& hostList )=0;
		/**
		 * �Ժߥ⡼�ɹ����奤�٥��
		 * @param hostList �ۥ��ȥꥹ��
		 */
		virtual void AbsenceModeChangeAfter( HostList& hostList )=0;
		/**
		 * �С�������������奤�٥��
		 * @param host �ۥ���
		 * @param version �С������
		 */
		virtual void VersionInfoRecieveAfter( HostListItem &host, string version )=0;
		/**
		 * �Ժ߾ܺپ�������奤�٥��
		 * @param host �ۥ���
		 * @param absenceDetail �Ժ߾ܺپ���
		 */
		virtual void AbsenceDetailRecieveAfter( HostListItem& host, string absenceDetail )=0;
		/**
		 * �ǥ��ȥ饯��
		 */
		virtual ~IpMessengerEvent()=0;
};

class IpMessengerAgentImpl;

/**
 * IP Messenger ����������ȥ��饹
 **/
class IpMessengerAgent {
	public:
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
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * ��å����������ʰ��ź�ա�
		 **/
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFile& file, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * ��å�����������ʣ��ź�ա�
		 **/
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList& files, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

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
		vector<GroupItem> GetGroupList();

		/**
		 * �С������������
		 **/
		string GetInfo( HostListItem& host );

		/**
		 * �Ժ߾������
		 **/
		string GetAbsenceInfo( HostListItem& host );

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
		 * �ͥåȥ���ε�ư��NIC�����
		 **/
		void StartNetwork( const vector<NetworkInterface>& nics );

		/**
		 * �ͥåȥ���ε�ư(�ǥե����)
		 **/
		void StartNetwork();

		/**
		 * �ͥåȥ���ν�λ
		 **/
		void StopNetwork();

		/**
		 * �ͥåȥ���κƵ�ư��NIC�����
		 **/
		void RestartNetwork( const vector<NetworkInterface>& nics );

		/**
		 * �ͥåȥ���κƵ�ư(�ǥե����)
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
