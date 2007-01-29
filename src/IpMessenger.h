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
 * 読み込み専用プロパティ
 * IPMSG_READONLY_PROPERTY( SomeClass, PropName )は
 * -------------------------------------------------------------------
 *   private: SomeClass _PropName;
 *   public: SomeClass& PropName(){ return _PropName; };
 * -------------------------------------------------------------------
 * に展開されます。
 **/
#define IPMSG_READONLY_PROPERTY(t, name ) \
									private: \
										t _##name; \
									public: \
										inline t name() const { return _##name; };

/**
 * 読み書き両用プロパティ
 * IPMSG_PROPERTY( SomeClass, PropName )は
 * -------------------------------------------------------------------
 *   private: SomeClass _PropName;
 *   public: SomeClass PropName(){ return _PropName; };
 *           void setPropName( SomeClass val ){ _PropName = val; };
 * -------------------------------------------------------------------
 * に展開されます。
 **/
#define IPMSG_PROPERTY(t, name )	private: \
										t _##name; \
									public: \
										inline t name() const { return _##name; }; \
										inline void set##name( const t val ){ _##name = val; };

/**
 * 読み書き両用プロパティ(参照用)
 * IPMSG_PROPERTY_REF( SomeClass, PropName )は
 * -------------------------------------------------------------------
 *   private: SomeClass _PropName;
 *   public: SomeClass& PropName(){ return _PropName; };
 *           void setPropName( SomeClass& val ){ _PropName = val; };
 * -------------------------------------------------------------------
 * に展開されます。
 **/
#define IPMSG_PROPERTY_REF(t, name )	\
									private: \
										t _##name; \
									public: \
										inline t& name() { return _##name; }; \
										inline void set##name( const t& val ){ _##name = val; };
/**
 * パケットクラス
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
 * ネットワークインターフェースクラス
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
 * グループクラス
 **/
class GroupItem{
	public:
		IPMSG_PROPERTY( std::string, GroupName );
		IPMSG_PROPERTY( std::string, EncodingName );
};

/**
 * ホストクラス
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
 * ホスト一覧ソート用比較クラス
 *（各Applicationは必要に応じてこのクラスを継承して処理を実装してください。）
 **/
class HostListComparator{
	public:
		/**
		 * 比較。
		 * @param host1 ホスト情報1
		 * @param host2 ホスト情報2
		 * @retval -n:host1が大きい
		 * @retval 0:host1とhost2が等しい
		 * @retval +n:host2が大きい
		 */
		virtual int compare( std::vector<HostListItem>::iterator host1, std::vector<HostListItem>::iterator host2 )=0;
};

/**
 * ホスト一覧デフォルトソート用比較クラス
 **/
class HostListDefaultComparator: public HostListComparator{
	public:
		/**
		 * 比較。
		 * @param host1 ホスト情報1
		 * @param host2 ホスト情報2
		 * @retval -n:host1が大きい
		 * @retval 0:host1とhost2が等しい
		 * @retval +n:host2が大きい
		 */
		virtual int compare( std::vector<HostListItem>::iterator host1, std::vector<HostListItem>::iterator host2 ){
			return host1->Compare( *host2 );
		};
};

/**
 * ホスト一覧クラス
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
 * ファイル名コンバータクラス
 * （各Applicationは必要に応じてこのクラスを継承して処理を実装してください。）
 **/
class FileNameConverter {
	public:
		virtual std::string ConvertNetworkToLocal( std::string original_file_name ) = 0;
		virtual std::string ConvertLocalToNetwork( std::string original_file_name ) = 0;
};

/**
 * ファイル名無変換コンバータクラス
 **/
class NullFileNameConverter:public FileNameConverter {
	public:
		virtual std::string ConvertNetworkToLocal( std::string original_file_name ){ return original_file_name; };
		virtual std::string ConvertLocalToNetwork( std::string original_file_name ){ return original_file_name; };
};

/**
 * 添付ファイルクラス
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
 * ダウンロード情報クラス
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
 * 添付ファイル一覧クラス
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
 * 受信メッセージクラス
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
 * 受信メッセージ一覧クラス
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
 * 送信メッセージクラス
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
 * 送信メッセージ一覧クラス
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
 * 不在モードクラス
 **/
class AbsenceMode {
	public:
		IPMSG_PROPERTY( std::string, EncodingName );
		IPMSG_PROPERTY( std::string, AbsenceName );
		IPMSG_PROPERTY( std::string, AbsenceDescription );
};


/**
 * IP Messenger イベントクラス
 * （各Applicationはこのクラスを継承して処理を実装してください。）
 **/
class IpMessengerEvent {
	public:
		/**
		 * ホストリストリフレッシュ後イベント
		 * @param hostList ホストリスト
		 */
		virtual void RefreashHostListAfter( HostList& hostList )=0;
		/**
		 * ホストリスト更新後イベント
		 * @param hostList ホストリスト
		 */
		virtual void UpdateHostListAfter( HostList& hostList )=0;
		/**
		 * ホストリスト取得リトライエラーイベント
		 * @retval true:リトライする
		 * @retval false:リトライしない
		 */
		virtual bool GetHostListRetryError()=0;
		/**
		 * メッセージ受信後イベント。
		 * @param msg 受信メッセージ
		 * @retval true:処理してメッセージの保存が不要
		 * @retval false:メッセージを保存
		 */
		virtual bool RecieveAfter( RecievedMessage& msg )=0;
		/**
		 * メッセージ送信後イベント
		 * @param msg 送信メッセージ
		 */
		virtual void SendAfter( SentMessage& msg )=0;
		/**
		 * メッセージ送信リトライエラーイベント
		 * @param msg 送信メッセージ
		 * @retval true:リトライする
		 * @retval false:リトライしない
		 */
		virtual bool SendRetryError( SentMessage& msg )=0;
		/**
		 * 開封通知後イベント
		 * @param msg 送信メッセージ
		 */
		virtual void OpenAfter( SentMessage& msg )=0;
		/**
		 * ダウンロード開始イベント
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 */
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * ダウンロード処理中イベント
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 */
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * ダウンロード終了イベント
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 */
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * ダウンロードエラーイベント(リトライする場合はTRUEを返す。)
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 * @retval true:リトライする
		 * @retval false:リトライしない
		 */
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		/**
		 * ホストの参加通知後イベント
		 * @param host ホスト
		 */
		virtual void EntryAfter( HostListItem& host )=0;
		/**
		 * ホストの脱退通知後イベント
		 * @param host ホスト
		 */
		virtual void ExitAfter( HostListItem& host )=0;
		/**
		 * 不在モード更新後イベント
		 * @param hostList ホスト
		 */
		virtual void AbsenceModeChangeAfter( HostListItem& host )=0;
		/**
		 * バージョン情報受信後イベント
		 * @param host ホスト
		 * @param version バージョン
		 */
		virtual void VersionInfoRecieveAfter( HostListItem &host, std::string version )=0;
		/**
		 * 不在詳細情報受信後イベント
		 * @param host ホスト
		 * @param absenceDetail 不在詳細情報
		 */
		virtual void AbsenceDetailRecieveAfter( HostListItem& host, std::string absenceDetail )=0;
		/**
		 * デストラクタ
		 */
		virtual ~IpMessengerEvent()=0;
};

class IpMessengerAgentImpl;

/**
 * IP Messenger エージェントクラス
 **/
class IpMessengerAgent {
	public:
		/**
		 * インスタンスを取得
		 **/
		static IpMessengerAgent *GetInstance();

		/**
		 * インスタンスを解放
		 **/
		static void Release();

		/**
		 * NICの情報を取得
		 **/
		static void GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics );

		/**
		 * ブロードキャストアドレスのリストをクリア
		 **/
		void ClearBroadcastAddress();

		/**
		 * ブロードキャストアドレスのリストからアドレスを削除
		 **/
		void DeleteBroadcastAddress( std::string addr );

		/**
		 * ブロードキャストアドレスのリストにアドレスを追加
		 **/
		void AddBroadcastAddress( std::string addr );

		/**
		 * ログイン通知（参加通知）
		 **/
		void Login( std::string nickname, std::string groupName );

		/**
		 * ログアウト通知（脱退通知）
		 **/
		void Logout();

		/**
		 * ホストリストを取得
		 **/
		HostList& GetHostList();

		/**
		 * ホストリストを更新して取得
		 **/
		HostList& UpdateHostList();

		/**
		 * メッセージ送信（添付無し）
		 **/
		SentMessage SendMsg( HostListItem host, std::string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * メッセージ送信（一つ添付）
		 **/
		SentMessage SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFile& file, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * メッセージ送信（複数添付）
		 **/
		SentMessage SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFileList& files, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * 不在解除
		 **/
		void ResetAbsence();

		/**
		 * 不在設定
		 **/
		void SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes );

		/**
		 * グループ一覧取得
		 **/
		std::vector<GroupItem> GetGroupList();

		/**
		 * バージョン情報取得
		 **/
		std::string GetInfo( HostListItem& host );

		/**
		 * 不在情報取得
		 **/
		std::string GetAbsenceInfo( HostListItem& host );

		/**
		 * パケットの処理（ポーリング用）
		 **/
		int Process();

		/**
		 * 受信済メッセージの個数取得
		 **/
		int GetRecievedMessageCount();

		/**
		 * 受信メッセージの取り出し
		 **/
		RecievedMessage PopRecievedMessage();

		/**
		 * 送信元に開封通知を送信
		 **/
		void ConfirmMessage( RecievedMessage &msg );

		/**
		 * 送信元にメッセージ削除（廃棄）を送信
		 **/
		void DeleteNotify( RecievedMessage msg );

		/**
		 * 送信済メッセージに開封済であることをマーク
		 **/
		void AcceptConfirmNotify( SentMessage msg );

		/**
		 * 送信メッセージリストの取得
		 **/
		SentMessageList *GetSentMessages();

		/**
		 * 送信メッセージリストのコピーの取得
		 **/
		SentMessageList CloneSentMessages() const;

		/**
		 * ホストリスト比較オブジェクトの設定
		 **/
		void SetSortHostListComparator( const HostListComparator *comparator );

		/**
		 * ホストリスト比較オブジェクトの取得
		 **/
		HostListComparator *GetSortHostListComparator() const;

		/**
		 * イベントオブジェクトの設定
		 **/
		void SetEventObject( const IpMessengerEvent *evt );

		/**
		 * イベントオブジェクトの取得
		 **/
		IpMessengerEvent *GetEventObject() const;

		/**
		 * ファイルコンバーターの設定
		 **/
		void SetFileNameConverter( const FileNameConverter *conv );

		/**
		 * ファイルコンバーターの取得
		 **/
		FileNameConverter *GetFileNameConverter() const;

		/**
		 * 不在？
		 **/
		bool IsAbsence() const;

		/**
		 * ネットワークの起動（NIC指定）
		 **/
		void StartNetwork( const std::vector<NetworkInterface>& nics );

		/**
		 * ネットワークの起動(デフォルト)
		 **/
		void StartNetwork();

		/**
		 * ネットワークの終了
		 **/
		void StopNetwork();

		/**
		 * ネットワークの再起動（NIC指定）
		 **/
		void RestartNetwork( const std::vector<NetworkInterface>& nics );

		/**
		 * ネットワークの再起動(デフォルト)
		 **/
		void RestartNetwork();

		/**
		 * ログイン名
		 **/
		std::string LoginName() const;

		/**
		 * ホスト名
		 **/
		std::string HostName() const;

		/**
		 * デフォルトポート
		 **/
		int DefaultPortNo() const;
		void setDefaultPortNo( const int defaultPortNo );

		/**
		 * ダイヤルアップ
		 **/
		bool IsDialup() const;
		void setIsDialup( const bool isDialup );

		/**
		 * ファイルが変更されたらダウンロードを中断する
		 **/
		bool AbortDownloadAtFileChanged() const;
		void setAbortDownloadAtFileChanged( const bool isAbort );

		/**
		 * 送信メッセージを記憶する
		 **/
		bool SaveSentMessage() const;
		void setSaveSentMessage( const bool isSave );

		/**
		 * 受信メッセージを記憶する
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
