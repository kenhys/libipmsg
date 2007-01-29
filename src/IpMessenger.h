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
#include <map>
#include <vector>

namespace ipmsg {

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
		IPMSG_PROPERTY( std::string, HostName );
		IPMSG_PROPERTY( std::string, UserName );
		IPMSG_PROPERTY( std::string, Option );
		IPMSG_PROPERTY( struct sockaddr_in, Addr );
		IPMSG_PROPERTY( int, TcpSocket );
};

/**
 * �ͥåȥ�����󥿡��ե��������饹
 **/
class NetworkInterface {
	public:
		IPMSG_PROPERTY( std::string, DeviceName );
		IPMSG_READONLY_PROPERTY( std::string, IpAddress );
		IPMSG_READONLY_PROPERTY( std::string, NetMask );
		IPMSG_READONLY_PROPERTY( std::string, NetworkAddress );
		IPMSG_READONLY_PROPERTY( std::string, BroadcastAddress );
		IPMSG_READONLY_PROPERTY( in_addr_t, NativeIpAddress );
		IPMSG_READONLY_PROPERTY( in_addr_t, NativeNetMask );
		IPMSG_READONLY_PROPERTY( in_addr_t, NativeNetworkAddress );
		IPMSG_READONLY_PROPERTY( in_addr_t, NativeBroadcastAddress );
		IPMSG_PROPERTY( int, PortNo );
		NetworkInterface( std::string deviceName ):_DeviceName( deviceName ){};
		NetworkInterface(){};
	public:
		void setIpAddress( const std::string val );
		void setNetMask( const std::string val );
		void setNativeIpAddress( const in_addr_t val );
		void setNativeNetMask( const in_addr_t val );
	private:
		void recalc();
};

/**
 * ���롼�ץ��饹
 **/
class GroupItem{
	public:
		IPMSG_PROPERTY( std::string, GroupName );
		IPMSG_PROPERTY( std::string, EncodingName );
};

/**
 * �ۥ��ȥ��饹
 **/
class HostListItem{
	public:
		IPMSG_PROPERTY( std::string, Version );
		IPMSG_PROPERTY( std::string, AbsenceDescription );
		IPMSG_PROPERTY( std::string, UserName );
		IPMSG_PROPERTY( std::string, HostName );
		IPMSG_PROPERTY( unsigned long, CommandNo );
		IPMSG_PROPERTY( std::string, IpAddress );
		IPMSG_PROPERTY( std::string, Nickname );
		IPMSG_PROPERTY( std::string, GroupName );
		IPMSG_PROPERTY( std::string, EncodingName );
		IPMSG_PROPERTY( std::string, Priority );
		IPMSG_PROPERTY( unsigned long, PortNo );
		IPMSG_PROPERTY( unsigned long, EncryptionCapacity );
		IPMSG_PROPERTY( std::string, PubKeyHex );
		IPMSG_PROPERTY( std::string, EncryptMethodHex );
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
		virtual int compare( std::vector<HostListItem>::iterator host1, std::vector<HostListItem>::iterator host2 )=0;
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
		virtual int compare( std::vector<HostListItem>::iterator host1, std::vector<HostListItem>::iterator host2 ){
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
		void Delete( std::vector<HostListItem>::iterator &it );
		void DeleteHostByAddress( std::string addr );
		std::vector<HostListItem>::iterator FindHostByAddress( std::string addr );
		std::vector<HostListItem>::iterator FindHostByHostName( std::string hostName );
		static HostListItem CreateHostListItemFromPacket( const Packet& packet );
		std::vector<HostListItem>::iterator begin();
		std::vector<HostListItem>::iterator end();
		int size() const;
		void clear();
		std::string ToString( int start, const struct sockaddr_in *addr );
		void sort( HostListComparator *comparator );
		HostList();
		HostList( const HostList& other );
		~HostList();
		HostList& operator=( const HostList& other );
		std::vector<GroupItem> GetGroupList();
	private:
		void qsort( HostListComparator *comparator, int left, int right );
		void Lock( const char *pos ) const;
		void Unlock( const char *pos ) const;
		void CopyFrom( const HostList& other );
		std::vector<HostListItem>items;
		pthread_mutex_t hostListMutex;
};

/**
 * �ե�����̾����С������饹
 * �ʳ�Application��ɬ�פ˱����Ƥ��Υ��饹��Ѿ����ƽ�����������Ƥ�����������
 **/
class FileNameConverter {
	public:
		virtual std::string ConvertNetworkToLocal( std::string original_file_name ) = 0;
		virtual std::string ConvertLocalToNetwork( std::string original_file_name ) = 0;
};

/**
 * �ե�����̵̾�Ѵ�����С������饹
 **/
class NullFileNameConverter:public FileNameConverter {
	public:
		virtual std::string ConvertNetworkToLocal( std::string original_file_name ){ return original_file_name; };
		virtual std::string ConvertLocalToNetwork( std::string original_file_name ){ return original_file_name; };
};

/**
 * ź�եե����륯�饹
 **/
class AttachFile{
	public:
		std::map<std::string, std::vector<unsigned long> >::iterator beginExtAttrs() { return _ExtAttrs.begin(); };
		std::map<std::string, std::vector<unsigned long> >::iterator endExtAttrs() { return _ExtAttrs.end(); };
		void addExtAttrs( std::string key, unsigned long val ){ _ExtAttrs[key].push_back( val ); };
		IPMSG_PROPERTY( int, FileId );
		IPMSG_PROPERTY( std::string, FullPath );
		IPMSG_PROPERTY( std::string, FileName );
		IPMSG_PROPERTY( std::string, Location );
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
		static int SendFileBuffer( int ifd, int sock, int size );
	private:
		std::map<std::string, std::vector<unsigned long> > _ExtAttrs;
	public:
		static AttachFile AnalyzeHeader( char *buf, FileNameConverter *conv );
		static std::string CreateDirFullPath( const std::vector<std::string>& dirstack );
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
		IPMSG_PROPERTY( std::string, LocalFileName );
		IPMSG_PROPERTY_REF( AttachFile, File );

		DownloadInfo():_Size( 0ULL ), _Time( 0 ), _FileCount( 0L ), _LocalFileName(""){};
		long double getSpeed();
		std::string getSpeedString();
		std::string getSizeString();
		static std::string getUnitSizeString( long long size );
};

/**
 * ź�եե�����������饹
 **/
class AttachFileList{
	public:
		void AddFile( const AttachFile& file );
		std::vector<AttachFile>::iterator begin();
		std::vector<AttachFile>::iterator end();
		int size() const;
		void clear();
		std::vector<AttachFile>::iterator erase( std::vector<AttachFile>::iterator item );
		std::vector<AttachFile>::iterator erase( const AttachFile& item );
		std::vector<AttachFile>::iterator FindByFullPath( const std::string& fullPath );
		std::vector<AttachFile>::iterator FindByFileId( int file_id );
		AttachFileList();
		AttachFileList( const AttachFileList& other );
		~AttachFileList();
		AttachFileList& operator=( const AttachFileList& other );
	private:
		void Lock( const char *pos ) const;
		void Unlock( const char *pos ) const;
		void CopyFrom( const AttachFileList& other );
		std::vector<AttachFile> files;
		pthread_mutex_t filesMutex;
};

class IpMessengerEvent;

/**
 * ������å��������饹
 **/
class RecievedMessage{
	public:
		IPMSG_PROPERTY( Packet, MessagePacket );
		IPMSG_PROPERTY( std::string, Message );
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
		bool DownloadFile( AttachFile &file, std::string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		bool DownloadDir( AttachFile &file, std::string saveDirName, std::string saveBaseDir, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
	private:
		void CopyFrom( const RecievedMessage& other );
		bool DownloadFilePrivate( IpMessengerEvent *event, AttachFile &file, std::string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		bool DownloadDirPrivate( IpMessengerEvent *event, AttachFile &file, std::string saveDirName, std::string saveBaseDir, DownloadInfo& info, FileNameConverter *conv=NULL, void *data=NULL );
		std::string GetFormalDir( std::string dirName );
		std::string GetSaveDir( std::string saveName, std::string saveBaseDir );
};

/**
 * ������å������������饹
 **/
class RecievedMessageList {
	public:
		std::vector<RecievedMessage>::iterator begin();
		std::vector<RecievedMessage>::iterator end();
		std::vector<RecievedMessage>::iterator erase( std::vector<RecievedMessage>::iterator item );
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
		std::vector<RecievedMessage> messages;
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
		IPMSG_PROPERTY( std::string, Message );
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
		std::vector<AttachFile>::iterator FindAttachFileByPacket( const Packet &packet );
	private:
		void CopyFrom( const SentMessage& other );
};

/**
 * ������å������������饹
 **/
class SentMessageList {
	public:
		std::vector<SentMessage>::iterator begin();
		std::vector<SentMessage>::iterator end();
		std::vector<SentMessage>::iterator erase( std::vector<SentMessage>::iterator item );
		std::vector<SentMessage>::iterator FindSentMessageByPacketNo( unsigned long PacketNo );
		std::vector<SentMessage>::iterator FindSentMessageByPacket( Packet packet );
		void append( const SentMessage &item );
		int size() const;
		void clear();
		std::vector<SentMessage> *GetMessageList();
		SentMessageList();
		SentMessageList( const SentMessageList& other );
		~SentMessageList();
		SentMessageList& operator=( const SentMessageList& other );

	private:
		void Lock( const char *pos ) const;
		void Unlock( const char *pos ) const;
		void CopyFrom( const SentMessageList& other );
		std::vector<SentMessage> messages;
		pthread_mutex_t messagesMutex;
};

/**
 * �Ժߥ⡼�ɥ��饹
 **/
class AbsenceMode {
	public:
		IPMSG_PROPERTY( std::string, EncodingName );
		IPMSG_PROPERTY( std::string, AbsenceName );
		IPMSG_PROPERTY( std::string, AbsenceDescription );
};


/**
 * IP Messenger ���٥�ȥ��饹
 * �ʳ�Application�Ϥ��Υ��饹��Ѿ����ƽ�����������Ƥ�����������
 **/
class IpMessengerEvent {
	public:
		/**
		 * �ۥ��ȥꥹ�ȥ�ե�å���奤�٥��
		 * @param hostList �ۥ��ȥꥹ��
		 */
		virtual void RefreashHostListAfter( HostList& hostList )=0;
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
		 * @param host �ۥ���
		 */
		virtual void EntryAfter( HostListItem& host )=0;
		/**
		 * �ۥ��Ȥ�æ�����θ奤�٥��
		 * @param host �ۥ���
		 */
		virtual void ExitAfter( HostListItem& host )=0;
		/**
		 * �Ժߥ⡼�ɹ����奤�٥��
		 * @param hostList �ۥ���
		 */
		virtual void AbsenceModeChangeAfter( HostListItem& host )=0;
		/**
		 * �С�������������奤�٥��
		 * @param host �ۥ���
		 * @param version �С������
		 */
		virtual void VersionInfoRecieveAfter( HostListItem &host, std::string version )=0;
		/**
		 * �Ժ߾ܺپ�������奤�٥��
		 * @param host �ۥ���
		 * @param absenceDetail �Ժ߾ܺپ���
		 */
		virtual void AbsenceDetailRecieveAfter( HostListItem& host, std::string absenceDetail )=0;
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
		static void GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics );

		/**
		 * �֥��ɥ��㥹�ȥ��ɥ쥹�Υꥹ�Ȥ򥯥ꥢ
		 **/
		void ClearBroadcastAddress();

		/**
		 * �֥��ɥ��㥹�ȥ��ɥ쥹�Υꥹ�Ȥ��饢�ɥ쥹����
		 **/
		void DeleteBroadcastAddress( std::string addr );

		/**
		 * �֥��ɥ��㥹�ȥ��ɥ쥹�Υꥹ�Ȥ˥��ɥ쥹���ɲ�
		 **/
		void AddBroadcastAddress( std::string addr );

		/**
		 * ���������Ρʻ������Ρ�
		 **/
		void Login( std::string nickname, std::string groupName );

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
		SentMessage SendMsg( HostListItem host, std::string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * ��å����������ʰ��ź�ա�
		 **/
		SentMessage SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFile& file, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * ��å�����������ʣ��ź�ա�
		 **/
		SentMessage SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFileList& files, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * �Ժ߲��
		 **/
		void ResetAbsence();

		/**
		 * �Ժ�����
		 **/
		void SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes );

		/**
		 * ���롼�װ�������
		 **/
		std::vector<GroupItem> GetGroupList();

		/**
		 * �С������������
		 **/
		std::string GetInfo( HostListItem& host );

		/**
		 * �Ժ߾������
		 **/
		std::string GetAbsenceInfo( HostListItem& host );

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
		SentMessageList CloneSentMessages() const;

		/**
		 * �ۥ��ȥꥹ����ӥ��֥������Ȥ�����
		 **/
		void SetSortHostListComparator( const HostListComparator *comparator );

		/**
		 * �ۥ��ȥꥹ����ӥ��֥������Ȥμ���
		 **/
		HostListComparator *GetSortHostListComparator() const;

		/**
		 * ���٥�ȥ��֥������Ȥ�����
		 **/
		void SetEventObject( const IpMessengerEvent *evt );

		/**
		 * ���٥�ȥ��֥������Ȥμ���
		 **/
		IpMessengerEvent *GetEventObject() const;

		/**
		 * �ե����륳��С�����������
		 **/
		void SetFileNameConverter( const FileNameConverter *conv );

		/**
		 * �ե����륳��С������μ���
		 **/
		FileNameConverter *GetFileNameConverter() const;

		/**
		 * �Ժߡ�
		 **/
		bool IsAbsence() const;

		/**
		 * �ͥåȥ���ε�ư��NIC�����
		 **/
		void StartNetwork( const std::vector<NetworkInterface>& nics );

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
		void RestartNetwork( const std::vector<NetworkInterface>& nics );

		/**
		 * �ͥåȥ���κƵ�ư(�ǥե����)
		 **/
		void RestartNetwork();

		/**
		 * ������̾
		 **/
		std::string LoginName() const;

		/**
		 * �ۥ���̾
		 **/
		std::string HostName() const;

		/**
		 * �ǥե���ȥݡ���
		 **/
		int DefaultPortNo() const;
		void setDefaultPortNo( const int defaultPortNo );

		/**
		 * ������륢�å�
		 **/
		bool IsDialup() const;
		void setIsDialup( const bool isDialup );

		/**
		 * �ե����뤬�ѹ����줿���������ɤ����Ǥ���
		 **/
		bool AbortDownloadAtFileChanged() const;
		void setAbortDownloadAtFileChanged( const bool isAbort );

		/**
		 * ������å������򵭲�����
		 **/
		bool SaveSentMessage() const;
		void setSaveSentMessage( const bool isSave );

		/**
		 * ������å������򵭲�����
		 **/
		bool SaveRecievedMessage() const;
		void setSaveRecievedMessage( const bool isSave );

	private:
		IpMessengerAgent();
		~IpMessengerAgent();
		IpMessengerAgentImpl *ipmsgImpl;
};

}; // namespace ipmsg
#endif
