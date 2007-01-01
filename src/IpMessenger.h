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
 * 読み込み専用プロパティ
 * IPMSG_READONLY_PROPERTY( SomeClass, PropName )は
 * -------------------------------------------------------------------
 *   private: SomeClass _PropName;
 *   public: SomeClass& PropName(){ return _PropName; };
 * -------------------------------------------------------------------
 * に展開されます。
 **/
#define IPMSG_READONLY_PROPERTY(t, name ) private: t _##name; \
								public: t name() const { return _##name; };

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
#define IPMSG_PROPERTY(t, name ) private: t _##name; \
								public: t name() const { return _##name; };\
								void set##name( const t val ){ _##name = val; };

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

class GroupItem{
	public:
		IPMSG_PROPERTY( string, GroupName );
		IPMSG_PROPERTY( string, EncodingName );
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
		bool IsLocalHost();
		bool IsFileAttachSupport();
		bool IsEncryptSupport();
		bool IsAbsence();
		bool Equals( const HostListItem& item );
		int Compare( const HostListItem& item );
		void QueryVersionInfo();
		void QueryAbsenceInfo();
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
		void AddHost( const HostListItem& host );
		void Delete( vector<HostListItem>::iterator &it );
		void DeleteHost( string hostname );
		vector<HostListItem>::iterator FindHostByAddress( string addr );
		vector<HostListItem>::iterator FindHostByHostName( string hostName );
		static HostListItem CreateHostListItemFromPacket( const Packet& packet );
		vector<HostListItem>::iterator begin();
		vector<HostListItem>::iterator end();
		int size();
		void clear();
		string ToString( int start );
		void sort( HostListComparator *comparator );
		HostList();
		~HostList();
	private:
		void qsort( HostListComparator *comparator, int left, int right );
		vector<HostListItem>items;
		pthread_mutex_t hostListMutex;
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
		IPMSG_PROPERTY( long long, TransSize );
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
		static string CreateDirFullPath( const vector<string>& dirstack );
};

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
		AttachFileList();
		~AttachFileList();
	private:
		vector<AttachFile> files;
		pthread_mutex_t filesMutex;
};

class IpMessengerEvent;

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
		vector<RecievedMessage>::iterator begin();
		vector<RecievedMessage>::iterator end();
		vector<RecievedMessage>::iterator erase( vector<RecievedMessage>::iterator item );
		void append( const RecievedMessage &item );
		int size();
		void clear();
		RecievedMessageList();
		~RecievedMessageList();
	private:
		vector<RecievedMessage> messages;
		pthread_mutex_t messagesMutex;
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
		IPMSG_PROPERTY( bool, IsNoLogging );
		IPMSG_PROPERTY( int, HostCountAtSameTime );
		IPMSG_PROPERTY( unsigned long, Opt );
		IPMSG_PROPERTY_REF( AttachFileList, Files );
		bool isRetryMaxOver();
		bool needSendRetry( time_t tryNow );
		vector<AttachFile>::iterator FindAttachFileByPacket( const Packet &packet );
};

class SentMessageList {
	public:
		vector<SentMessage>::iterator begin();
		vector<SentMessage>::iterator end();
		vector<SentMessage>::iterator erase( vector<SentMessage>::iterator item );
		vector<SentMessage>::iterator FindSentMessageByPacketNo( unsigned long PacketNo );
		vector<SentMessage>::iterator FindSentMessageByPacket( Packet packet );
		void append( const SentMessage &item );
		int size();
		void clear();
		vector<SentMessage> *GetMessageList();
		SentMessageList();
		~SentMessageList();

	private:
		vector<SentMessage> messages;
		pthread_mutex_t messagesMutex;
};

class AbsenceMode {
	public:
		IPMSG_PROPERTY( string, EncodingName );
		IPMSG_PROPERTY( string, AbsenceName );
		IPMSG_PROPERTY( string, AbsenceDescription );
};


/**
 * IP Messenger イベントオブジェクト
 * （各Applicationはこのオブジェクトを継承して処理を実装してください。）
 **/
class IpMessengerEvent {
	public:
		//ホストリスト更新後
		virtual void UpdateHostListAfter( HostList& hostList )=0;
		//ホストリスト取得リトライエラー
		virtual bool GetHostListRetryError()=0;
		//メッセージ受信後(処理してメッセージの保存が不要ならTRUEを返す)
		virtual bool RecieveAfter( RecievedMessage& msg )=0;
		//メッセージ送信後
		virtual void SendAfter( SentMessage& msg )=0;
		//メッセージ送信リトライエラー
		virtual bool SendRetryError( SentMessage& msg )=0;
		//開封通知後
		virtual void OpenAfter( SentMessage& msg )=0;
		//ダウンロード開始
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//ダウンロード処理中
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//ダウンロード終了
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//ダウンロードエラー(リトライする場合はTRUEを返す。)
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data )=0;
		//ホストの参加通知後
		virtual void EntryAfter( HostList& hostList )=0;
		//ホストの脱退通知後
		virtual void ExitAfter( HostList& hostList )=0;
		//不在モード更新後
		virtual void AbsenceModeChangeAfter( HostList& hostList )=0;
		//バージョン情報受信後
		virtual void VersionInfoRecieveAfter( HostListItem &host, string version )=0;
		//不在詳細情報受信後
		virtual void AbsenceDetailRecieveAfter( HostListItem& host, string absenceDetail )=0;
		virtual ~IpMessengerEvent()=0;
};

class IpMessengerAgentImpl;

/**
 * IP Messenger エージェント
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
		static void GetNetworkInterfaceInfo( vector<NetworkInterface>& nics );

		/**
		 * ブロードキャストアドレスのリストをクリア
		 **/
		void ClearBroadcastAddress();

		/**
		 * ブロードキャストアドレスのリストからアドレスを削除
		 **/
		void DeleteBroadcastAddress( string addr );

		/**
		 * ブロードキャストアドレスのリストにアドレスを追加
		 **/
		void AddBroadcastAddress( string addr );

		/**
		 * ログイン通知（参加通知）
		 **/
		void Login( string nickname, string groupName );

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
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * メッセージ送信（一つ添付）
		 **/
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * メッセージ送信（複数添付）
		 **/
		SentMessage SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword=false, int hostCountAtSameTime=1, bool IsNoLogging=false, unsigned long opt=0UL );

		/**
		 * 不在解除
		 **/
		void ResetAbsence();

		/**
		 * 不在設定
		 **/
		void SetAbsence( string encoding, vector<AbsenceMode> absenceModes );

		/**
		 * グループ一覧取得
		 **/
		vector<GroupItem> GetGroupList();

		/**
		 * バージョン情報取得
		 **/
		string GetInfo( HostListItem& host );

		/**
		 * 不在情報取得
		 **/
		string GetAbsenceInfo( HostListItem& host );

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
		SentMessageList CloneSentMessages();

		/**
		 * ホストリスト比較オブジェクトの設定
		 **/
		void SetSortHostListComparator( HostListComparator *comparator );

		/**
		 * ホストリスト比較オブジェクトの取得
		 **/
		HostListComparator *GetSortHostListComparator();

		/**
		 * イベントオブジェクトの設定
		 **/
		void SetEventObject( IpMessengerEvent *evt );

		/**
		 * イベントオブジェクトの取得
		 **/
		IpMessengerEvent *GetEventObject();

		/**
		 * ファイルコンバーターの設定
		 **/
		void SetFileNameConverter( FileNameConverter *conv );

		/**
		 * ファイルコンバーターの取得
		 **/
		FileNameConverter *GetFileNameConverter();

		/**
		 * 不在？
		 **/
		bool IsAbsence();

		/**
		 * ネットワークの再起動
		 **/
		void RestartNetwork( const vector<NetworkInterface>& nics );

		/**
		 * ログイン名
		 **/
		string LoginName();

		/**
		 * ホスト名
		 **/
		string HostName();

		/**
		 * ダイヤルアップ
		 **/
		bool IsDialup();
		void setIsDialup( bool isDialup );

		/**
		 * ファイルが変更されたらダウンロードを中断する
		 **/
		bool AbortDownloadAtFileChanged();
		void setAbortDownloadAtFileChanged( bool isAbort );
		/* 後方互換性の為(0.1.0) */
		bool GetAbortDownloadAtFileChanged(){ return AbortDownloadAtFileChanged(); };
		void SetAbortDownloadAtFileChanged( bool isAbort ){ setAbortDownloadAtFileChanged( isAbort ); };

		/**
		 * 送信メッセージを記憶する
		 **/
		bool SaveSentMessage();
		void setSaveSentMessage( bool isSave );

		/**
		 * 受信メッセージを記憶する
		 **/
		bool SaveRecievedMessage();
		void setSaveRecievedMessage( bool isSave );

	private:
		IpMessengerAgent();
		~IpMessengerAgent();
		IpMessengerAgentImpl *ipmsgImpl;
};

#endif
