/**
 * IP メッセンジャライブラリ(Unix用)
 * IPメッセンジャエージェントクラス。
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
#include <pthread.h>
using namespace std;

static IpMessengerAgentImpl *myInstance = NULL;

void *GetFileDataThread( void *param );
void *GetDirFilesThread( void *param );

static pthread_mutex_t instanceMutex;
static int mutex_init_result = IpMsgMutexInit( "IpMessengerImpl::Global", &instanceMutex, NULL );

class IpMessengerNullEvent: public IpMessengerEvent {
	public:
		virtual void UpdateHostListAfter( HostList& hostList ){ printf("UpdateHostListAfter\n"); };
		virtual bool GetHostListRetryError(){ printf("GetHostListRetryError\n");return false; };
		virtual bool RecieveAfter( RecievedMessage& msg ){ printf("RecieveAfter\n");return false; };
		virtual void SendAfter( SentMessage& msg ){ printf("SendAfter\n"); };
		virtual bool SendRetryError( SentMessage& msg ){ printf("SendRetryError\n");return false; };
		virtual void OpenAfter( SentMessage& msg ){ printf("OpenAfter\n"); };
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadStart\n"); };
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadProcessing\n"); };
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadEnd\n"); };
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadError\n"); return false; };
		virtual void EntryAfter( HostList& hostList ){ printf("EntryAfter\n"); };
		virtual void ExitAfter( HostList& hostList ){ printf("ExitAfter\n"); };
		virtual void AbsenceModeChangeAfter( HostList& hostList ){ printf("AbsenceModeChangeAfter\n"); };
		virtual void VersionInfoRecieveAfter( HostListItem &host, string version ){ printf("VersionInfoRecieveAfter\n"); };
		virtual void AbsenceDetailRecieveAfter( HostListItem &host, string absenceDetail ){ printf("AbsenceDetailRecieveAfter\n"); };
};

/**
 * IP メッセンジャエージェントクラスのインスタンスを取得する。
 * Singletonパターンを採用しているので、ホスト唯一のインスタンスでなければならない。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgentImpl *
IpMessengerAgentImpl::GetInstance()
{
	IpMsgMutexLock( "IpMessengerAgentImpl::GetInstance()", &instanceMutex );
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgentImpl();
	}
	IpMsgMutexUnlock( "IpMessengerAgentImpl::GetInstance()", &instanceMutex );
	return myInstance;
}

/**
 * IP メッセンジャエージェントクラスのインスタンスを解放する。
 * このメソッドを使ってオブジェクトを解放しなければならない。
 * ライブラリを通じないで直接deleteされた場合はその後の動作について感知しない。
 * 注：このインスタンスはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::Release()
{
	IpMsgMutexLock( "IpMessengerAgentImpl::Release()", &instanceMutex );
	if ( myInstance == NULL ) {
		IpMsgMutexUnlock( "IpMessengerAgentImpl::Release()", &instanceMutex );
		return;
	}
	delete myInstance;
	myInstance = NULL;
	IpMsgMutexUnlock( "IpMessengerAgentImpl::Release()", &instanceMutex );
}

/**
 * IP メッセンジャエージェントクラスのコンストラクタ。
 * ・暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の生成を行う。
 * ・パケットNoに使用する乱数シードを時刻で初期化する。
 * ・ファイル名コンバータを初期セットアップする。（変換を行わないNullConverterがデフォルト）
 * ・ネットワークの初期化。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgentImpl::IpMessengerAgentImpl()
{
	CryptoInit();
	srandom( time( NULL ) );
	converter = new NullFileNameConverter();
	compare = new HostListDefaultComparator();
	setAbortDownloadAtFileChanged( false );
	setSaveSentMessage( true );
	setSaveRecievedMessage( true );
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( NICs );
	event = new IpMessengerNullEvent();
}

/**
 * IP メッセンジャエージェントクラスのデストラクタ。
 * ・まず、ログアウト。
 * ・暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の破棄を行う。
 * ・割り当て済のファイル名コンバータを削除する。
 * ・ソケットのクローズ。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgentImpl::~IpMessengerAgentImpl()
{
	if ( networkStarted ){
		Logout();
		StopNetwork();
	}
	CryptoEnd();
	delete converter;
	delete compare;
	delete event;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを起動する。
 * ・NICを指定しないでネットワーク開始(全てのNIC、デフォルトポートで起動)。
 */
void
IpMessengerAgentImpl::StartNetwork()
{
	vector<NetworkInterface> nics;
	StartNetwork( nics );
}

/**
 * IP メッセンジャエージェントクラスのネットワークを起動する。
 * ・ネットワーク初期化。
 * ・不在を解除。
 * ・開始後ログインする必要があります。
 */
void
IpMessengerAgentImpl::StartNetwork( const vector<NetworkInterface>& nics )
{
	NetworkInit( nics );
	Logout();
	// TODO 受信スレッド開始
	networkStarted = true;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを終了する。
 * ・ネットワーク終期化。
 */
void
IpMessengerAgentImpl::StopNetwork()
{
	// TODO 受信スレッド終了。＆待ち合わせ。
	NetworkEnd();
	networkStarted = false;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * ・NICを指定しないでネットワーク再起動(全てのNIC、デフォルトポート再起動)。
 */
void
IpMessengerAgentImpl::RestartNetwork()
{
	vector<NetworkInterface> nics;
	RestartNetwork( nics );
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * ・まず、ログアウト。
 * ・ネットワーク終期化。
 * ・ネットワーク初期化。
 * ・再度ログイン。
 */
void
IpMessengerAgentImpl::RestartNetwork( const vector<NetworkInterface>& nics )
{
	Logout();
	StopNetwork();
	StartNetwork( nics );
	Login( Nickname, GroupName );
}

/**
 * ファイル名コンバータのゲッター。
 * 注：このメソッドはスレッドセーフでない。
 * @retval コンバータのアドレス。
 */
FileNameConverter *
IpMessengerAgentImpl::GetFileNameConverter()
{
	return converter;
}

/**
 * ファイル名コンバータのセッター。
 * ・割り当て済のファイル名コンバータを削除する。
 * ・新しいコンバータの割り当て。
 * 注：このメソッドはスレッドセーフでない。
 * @param conv コンバータのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgentImpl::SetFileNameConverter( FileNameConverter *conv )
{
	if ( conv == NULL ){
		return;
	}
	delete converter;
	converter = conv;
}

/**
 * ホストリスト比較オブジェクトのゲッター。
 * 注：このメソッドはスレッドセーフでない。
 * @retval ホストリスト比較オブジェクトのアドレス。
 */
HostListComparator *
IpMessengerAgentImpl::GetSortHostListComparator()
{
	return compare;
}; 

/**
 * ホストリスト比較オブジェクトのセッター。
 * ・割り当て済のホストリスト比較オブジェクトを削除する。
 * ・新しいホストリスト比較オブジェクトの割り当て。
 * 注：このメソッドはスレッドセーフでない。
 * @param comparator ホストリスト比較オブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgentImpl::SetSortHostListComparator( HostListComparator *comparator )
{
	if ( comparator == NULL ){
		return;
	}
	delete compare;
	compare = comparator;
}

/**
 * イベントオブジェクトのゲッター。
 * 注：このメソッドはスレッドセーフでない。
 * @retval イベントオブジェクトのアドレス。
 */
IpMessengerEvent *
IpMessengerAgentImpl::GetEventObject()
{
	return event;
}; 

/**
 * イベントオブジェクトのセッター。
 * ・割り当て済のイベントオブジェクトを削除する。
 * ・新しいイベントオブジェクトの割り当て。
 * 注：このメソッドはスレッドセーフでない。
 * @param evt イベントオブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgentImpl::SetEventObject( IpMessengerEvent *evt )
{
	if ( evt == NULL ){
		return;
	}
	delete event;
	event = evt;
}

/**
 * NICの情報を取得する。
 * ・使用するネットワークインターフェイスのIPアドレスを求める。（ローカルループバックをのぞく全てのNIC）
 * @param nics ネットワークインターフェースの一覧
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::GetNetworkInterfaceInfo( vector<NetworkInterface>& nics )
{
	//情報取得のためのソケットを作成
	int fd;
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* ローカルループバックをのぞく全てのIPアドレスが対象 */
	/* 全てのNICを取得する(より移植性が高い方法) */
	struct if_nameindex *p0 = NULL, *p = NULL;
	for( p0 = p = if_nameindex(); p->if_index > 0; p++ ){
		struct ifreq ifr;
		memset( &ifr, 0, sizeof( ifr ) );

		ifr.ifr_addr.sa_family = AF_INET;
		strncpy(ifr.ifr_name, p->if_name, IFNAMSIZ-1);
		ioctl(fd, SIOCGIFADDR, &ifr);

		char ipAddrBuf[100];
		in_addr_t ipAddr = ( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr.s_addr;
		inet_ntoa_r( ipAddr, ipAddrBuf, sizeof( ipAddrBuf ) );
		if ( strcmp( "127.0.0.1", ipAddrBuf ) == 0 ||
			 strcmp( "0.0.0.0", ipAddrBuf ) == 0 ||
			 strcmp( "255.255.255.255", ipAddrBuf ) == 0 ){
			continue;
		}

		ioctl(fd, SIOCGIFNETMASK, &ifr);

		in_addr_t netMask = ( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr.s_addr;
		char netMaskBuf[100];
		inet_ntoa_r( ( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr.s_addr, netMaskBuf, sizeof( netMaskBuf ) );

		char networkAddrBuf[100];
		in_addr_t networkAddr = ipAddr & netMask;
		inet_ntoa_r( networkAddr, networkAddrBuf, sizeof( networkAddrBuf ) );

		char broadcastAddrBuf[100];
		in_addr_t broadcastAddr = networkAddr | ( 0xffffffff ^ netMask );
		inet_ntoa_r( broadcastAddr, broadcastAddrBuf, sizeof( broadcastAddrBuf ) );

//#if defined(DEBUG) || !defined(NDEBUG)
		printf( "dev=%s,ipaddress=%s,netmask=%s,network=%s,broadcast=%s\n",
				ifr.ifr_name, ipAddrBuf, netMaskBuf, networkAddrBuf, broadcastAddrBuf );fflush( stdout );
//#endif

		NetworkInterface ni;
		ni.setDeviceName( ifr.ifr_name );
		ni.setPortNo( IPMSG_DEFAULT_PORT );
		ni.setIpAddress( ipAddrBuf );
		ni.setNetMask( netMaskBuf );
		ni.setNetworkAddress( networkAddrBuf );
		ni.setBroadcastAddress( broadcastAddrBuf );
		ni.setNativeIpAddress( ipAddr );
		ni.setNativeNetMask( netMask );
		ni.setNativeNetworkAddress( networkAddr );
		ni.setNativeBroadcastAddress( broadcastAddr );
		nics.push_back( ni );
#if defined(DEBUG) || !defined(NDEBUG)
		printf( "NIC device=%s[IpAddress=%s][Port=%d][NetMask=%s][NetworkAddress=%s]\n",
						nics[nics.size() - 1].DeviceName().c_str(),
						nics[nics.size()-1].IpAddress().c_str(),
						nics[nics.size()-1].PortNo(),
						nics[nics.size()-1].NetMask().c_str(),
						nics[nics.size()-1].NetworkAddress().c_str());fflush( stdout );
#endif
	}
	if_freenameindex( p0 );
	//情報取得のためのソケットを閉じる。
	close(fd);
}

/**
 * ネットワーク関連の初期化。
 * ・環境変数からホスト名を取得。（出来なければlocalhost固定）
 * ・環境変数からユーザ名を取得。（出来なければuid）
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::NetworkInit( const vector<NetworkInterface>& nics )
{
	char buf[100];
	char *env;

	_HostName = "";
	env = getenv( "HOSTNAME" );
	if ( env != NULL ) {
		_HostName = env;
	}
	if ( _HostName == "" ) {
		env = getenv( "HOST" );
		if ( env != NULL ) {
			_HostName = env;
		}
	}
	if ( _HostName == "" ) {
		_HostName = "localhost";
	}
	env = getenv( "USERNAME" );
	if ( env != NULL ){
		_LoginName = env;
	}
	if ( _LoginName == "" ){
		env = getenv( "USER" );
		if ( env != NULL ) {
			_LoginName = env;
		}
	}
	if ( _LoginName == "" ){
		_LoginName = snprintf( buf, sizeof( buf ), "%d", getuid() );
	}

#ifdef HAVE_OPENSSL
	DecryptErrorMessage = "\r\n"\
						  " ==== AutoReply(DecryptErr) ====\r\n" \
						  "  My PubKey is updated, I can't\r\n" \
						  "  receive your message.\r\n" \
						  "  Please press refresh button.\r\n" \
						  " ==============================";
#endif	//HAVE_OPENSSL
	if ( nics.size() > 0 ){
		InitSend( nics );
		InitRecv( nics );
	} else {
		InitSend( NICs);
		InitRecv( NICs );
	}
}

/**
 * ネットワーク関連の初期化。
 * ・全てのソケットを閉じる。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::NetworkEnd()
{
	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		close(udp_sd[i]);
	}
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		close(tcp_sd[i]);
	}
	udp_sd.clear();
	tcp_sd.clear();
	sd_addr.clear();
}

/**
 * ログイン（サービス参加通知）。
 * ・NOOPERATIONパケットを送信しネットワークが使用可能かどうかを確認した上でホストリストを取得。
 * ・BR_ENTRYをブロードキャスト。
 * ・パケットを受信した上で、ホストリストを再度取得。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::Login( string nickname, string groupName )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;

	SendNoOperation();

#if defined(DEBUG) || !defined(NDEBUG)
	memset( sendBuf, 0, MAX_UDPBUF );
#endif
	if ( nickname != "" ) {
		Nickname = nickname;
	} else {
		Nickname = _LoginName;
	}
	GroupName = groupName;
	optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s", Nickname.c_str() );
	optBuf[ optBufLen ] = '\0';
	optBufLen++;
	snprintf( &optBuf[ optBufLen ], sizeof( optBuf ) - optBufLen - 1, "%s", GroupName.c_str() );
	optBufLen += GroupName.size();
	optBuf[ optBufLen ] = '\0';
	
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ENTRY ),
										_LoginName, _HostName,
										optBuf, optBufLen,
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ENTRY, sendBuf, sendBufLen );
	ResetAbsence();
	RecvPacket();
	//0.05秒まつ。
	usleep( 50000L );
	RecvPacket();

}

/**
 * ログアウト（サービス脱退通知）。
 * ・BR_EXITをブロードキャスト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::Logout()
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_EXIT ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_EXIT, sendBuf, sendBufLen );
	RecvPacket();
}

/**
 * ホストリスト取得。
 * @retval エージェントが保持しているHostListオブジェクト
 * @retval ホストリスト
 */
HostList&
IpMessengerAgentImpl::GetHostList()
{
	return hostList;
}

/**
 * ホストリスト更新取得。
 * ・BR_ISGETLIST2をブロードキャスト。
 * ・他のメソッド（ANSLIST受信）にて取得するまで待機。（五回まで）
 * 注：ホストリストの構築はANSLIST受信時に行われるので、このメソッドではひたすら待機。
 * 注：ホストリストはANSLIST受信時に追加／更新されることがあるので常に同じホストリストを返すとは限らない。
 * 注：このメソッドはスレッドセーフでない。
 * @retval 取得したHostListオブジェクト
 */
HostList&
IpMessengerAgentImpl::UpdateHostList( bool isRetry )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	if ( !isRetry && !hostList.IsAsking() ) {
#if defined(DEBUG)
		for(int i=0;i < 6;i++)
			printf("!! HOSTLIST CLEAR !! ");fflush( stdout );
		printf("\n");fflush(stdout);
#endif
		hostList.clear();
#if defined(DEBUG)
		for(int i=0;i < 6;i++)
			printf("!! HOSTLIST CLEARED !! ");fflush( stdout );
		printf("\n");fflush(stdout);
#endif
	}
	hostList.setIsAsking( true );
	if ( !isRetry ) {
		hostList.setAskStartTime( time( NULL ) );
		hostList.setPrevTry( hostList.AskStartTime() );
		hostList.setRetryCount( 0 );
	}
	AddDefaultHost();

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ISGETLIST2 ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ISGETLIST2, sendBuf, sendBufLen );
	//再入禁止(リトライ時はRecvPacketから呼ばれる)
	if ( !isRetry ) {
		int pcount = RecvPacket();
		//自分以外のホストが見付からないか５回リトライする間繰り返す
		for( int i = 0; i < 5; i++ ) {
			//0.01秒まつ。
			usleep( 10000L );
			pcount = RecvPacket();
		}
	}

#if defined(DEBUG)
	IpMsgDumpHostList( " M Y   H O S T L I S T ( BEFORE SORT ) ", hostList );
#endif
	if ( compare != NULL ) {
		hostList.sort( compare );
	}
#if defined(DEBUG)
	IpMsgDumpHostList( " M Y   H O S T L I S T ( AFTER SORT ) ", hostList );
#endif
	if ( event != NULL ) {
		event->UpdateHostListAfter( hostList );
	}
	return hostList;
}

/**
 * 不在モードかどうかを判定。
 * @retval 設定済の不在モードを返す。
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgentImpl::IsAbsence()
{
	return _IsAbsence;
}
/**
 * 不在モードをクリアする。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::ResetAbsence()
{
	_IsAbsence = false;
	localEncoding = "";
	vector<AbsenceMode> d;
	absenceModeList = d;
	SendAbsence();
}

/**
 * 不在モードを設定する。
 * @param encoding ローカルエンコーディング
 * @param absenceModes AbsenceModeオブジェクトのベクタ（自動応答時に複数エンコーディング対応するため）
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::SetAbsence( string encoding, vector<AbsenceMode> absenceModes )
{
	_IsAbsence = true;

	localEncoding = encoding;
	absenceModeList = absenceModes;
	SendAbsence();
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param opt 送信オプション
 * 注：このメソッドはスレッドセーフでない。
 */
SentMessage
IpMessengerAgentImpl::SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	AttachFileList files;
	return SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, IsNoLogging, opt, false, 0UL );
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param file 添付ファイル
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param opt 送信オプション
 * 注：このメソッドはスレッドセーフでない。
 */
SentMessage
IpMessengerAgentImpl::SendMsg( HostListItem host, string msg, bool isSecret, AttachFile& file, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	AttachFileList files;
	files.AddFile( file );
	return SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, IsNoLogging, opt, false, 0UL );
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param files 添付ファイル群
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param opt 送信オプション
 * 注：このメソッドはスレッドセーフでない。
 */
SentMessage
IpMessengerAgentImpl::SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList& files, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt, bool isRetry, unsigned long PrevPacketNo )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[GetMaxOptionBufferSize() + 1];
	int optBufLen = 0;
	struct sockaddr_in addr;
	bool isEncrypted = false;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( host.PortNo() );
	addr.sin_addr.s_addr = inet_addr(host.IpAddress().c_str());

	RecvPacket();

	optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s", msg.c_str() );
#ifdef HAVE_OPENSSL
	if ( isSecret && EncryptMsg( host, (unsigned char*)optBuf, optBufLen, &optBufLen, sizeof( optBuf ) ) ) {
		isEncrypted = true;
	} else {
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s", msg.c_str() );
	}
#endif	//HAVE_OPENSSL
	//切り捨てられた。
	if ( optBufLen >= (int)sizeof( optBuf ) - 1 ) {
		optBufLen = sizeof( optBuf ) - 1;
	}

	optBuf[optBufLen++] = '\0';
	IpMsgPrintBuf( "optBuf:", optBuf, optBufLen );

	int fileBufLen = 0;
	char fileBuf[MAX_UDPBUF];

	for( vector<AttachFile>::iterator ixfile = files.begin(); ixfile != files.end(); ixfile++ ) {
		ixfile->GetLocalFileInfo();
		string filename = converter->ConvertLocalToNetwork( ixfile->FileName() );
		int wsize = snprintf( &fileBuf[ fileBufLen ], sizeof( fileBuf ) - fileBufLen - 1,
							"%d:%s:%llx:%lx:%lx:\a",
							ixfile->FileId(), filename.c_str(), ixfile->FileSize(), (unsigned long)ixfile->MTime(), ixfile->Attr() );
		if ( optBufLen + fileBufLen + wsize - 1 > MAX_UDPBUF ) {
#if defined(DEBUG)
			printf( "break;\n" );fflush(stdout);
#endif
			break;
		}
		fileBufLen += wsize;
		fileBuf[fileBufLen] = '\0';
	}
#if defined(DEBUG)
	printf( "(1)optBufLen = %d, fileBufLen = %d", optBufLen, fileBufLen );fflush(stdout);
#endif
	memcpy( &optBuf[ optBufLen ], fileBuf, fileBufLen );
	optBufLen += fileBufLen;
#if defined(DEBUG)
	printf( "(2)optBufLen = %d, fileBufLen = %d", optBufLen, fileBufLen );fflush(stdout);
#endif
	IpMsgPrintBuf( "fileBuf2:", fileBuf, fileBufLen );
	if ( optBufLen >= (int)sizeof( optBuf ) - 1 ) {
		optBufLen = sizeof( optBuf ) - 1;
	}
	optBuf[ optBufLen ] = '\0';

	IpMsgPrintBuf( "optBuf2:", optBuf, optBufLen );

	unsigned long packetNo = (isRetry && PrevPacketNo != 0UL ? PrevPacketNo : random() );

	sendBufLen = CreateNewPacketBuffer( IPMSG_SENDMSG | IPMSG_SENDCHECKOPT |
#ifdef HAVE_OPENSSL
										  ( isEncrypted ? IPMSG_ENCRYPTOPT : 0UL ) |
#endif	//HAVE_OPENSSL
										  ( IsNoLogging ? IPMSG_NOLOGOPT : 0UL ) |
										  ( isSecret ? IPMSG_SECRETOPT : 0UL ) |
										  ( _IsAbsence ? IPMSG_AUTORETOPT : 0UL ) |
										  ( isLockPassword ? IPMSG_PASSWORDOPT : 0UL ) |
										  ( files.size() > 0 ? IPMSG_FILEATTACHOPT : 0UL ) | opt,
										  packetNo,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_SENDMSG, sendBuf, sendBufLen, addr );

	SentMessage message;
	message.setTo( addr );
	message.setHost( host );
	message.setPacketNo( packetNo );
	message.setMessage( msg );
	message.setSent( time( NULL ) );
	if ( !isRetry ) {
		message.setPrevTry( message.Sent() );
		message.setIsRetryMaxOver( false );
		message.setRetryCount( 0 );
	}
	message.setIsConfirmed( false );
	message.setIsPasswordLock( isLockPassword );
	message.setIsCrypted( isEncrypted );
	message.setIsConfirmAnswered( false );
	message.setHostCountAtSameTime( hostCountAtSameTime );
	message.setOpt( opt );
	message.setIsNoLogging( IsNoLogging );
	message.setIsSecret( isSecret );
	message.setFiles( files );
	message.setIsSent( false );

#if defined(DEBUG)
	printf( "UserName[%s]\n", message.Host().UserName().c_str() );fflush( stdout );
	printf( "HostName[%s]\n", message.Host().HostName().c_str() );fflush( stdout );
	printf( "Nickname[%s]\n", message.Host().Nickname().c_str() );fflush( stdout );
#endif
	if ( SaveSentMessage() && !isRetry ){
		sentMsgList.append( message );
	}

//	RecvPacket();

#if defined(DEBUG)
	printf("sentMsgList.append() size=%d\n", sentMsgList.size() );fflush( stdout );
#endif

	return message;
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::ClearBroadcastAddress()
{
	broadcastAddr.clear();
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::DeleteBroadcastAddress( string addr )
{
	vector<struct sockaddr_in>::iterator net = FindBroadcastNetworkByAddress( addr );
	if ( net != broadcastAddr.end() ) {
#if defined(DEBUG)
		char ipAddrBuf[100];
		printf( "Delete Broadcast Address from %s(%d)\n", inet_ntoa_r( net->sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), ntohs( net->sin_port ) );fflush( stdout );
#endif
		broadcastAddr.erase( net );
		return;
	}
}

/**
 * ブロードキャストアドレスを登録
 * @param addr 登録するブロードキャストアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::AddBroadcastAddress( string addr )
{
	struct addrinfo hints, *res;
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	int err = getaddrinfo( addr.c_str(), NULL, &hints, &res );
	if ( err != 0 ){
		fprintf(stderr, "getaddrinfo:%s\n", gai_strerror( err ) );
		return;
	}
	struct sockaddr_in addAddr;
	addAddr.sin_family = AF_INET;
	addAddr.sin_port = htons(IPMSG_DEFAULT_PORT);
	addAddr.sin_addr.s_addr = ((struct sockaddr_in *)(res->ai_addr) )->sin_addr.s_addr;
	freeaddrinfo( res );

	char ipAddrBuf[100];
	string broadIp = inet_ntoa_r( addAddr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	vector<struct sockaddr_in>::iterator net = FindBroadcastNetworkByAddress( broadIp );
	if ( net != broadcastAddr.end() ) {
		return;
	}
#if defined(DEBUG)
	printf( "Add Broadcast Address To %s(%d)\n", inet_ntoa_r( addAddr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), ntohs( addAddr.sin_port ) );fflush( stdout );
#endif
	broadcastAddr.push_back( addAddr );
}

/**
 * 登録済のブロードキャストアドレスを検索し、該当するsockaddr_in構造体を返却する。
 * @param addr ブロードキャストアドレス文字列
 * @retval sockaddr_in構造体
 * 注：このメソッドはスレッドセーフでない。
 */
vector<struct sockaddr_in>::iterator
IpMessengerAgentImpl::FindBroadcastNetworkByAddress( string addr )
{
	in_addr_t s_addr = inet_addr( addr.c_str() );
	for( vector<struct sockaddr_in>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
		if ( ixaddr->sin_addr.s_addr == s_addr ) {
			return ixaddr;
		}
	}
	return broadcastAddr.end();
}

/**
 * 対象ホストのバージョン情報を問い合わせる。
 * ・GETINFOパケットを送信。
 * @param host 対象のホスト
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::QueryVersionInfo( HostListItem& host )
{
	char sendBuf[MAX_UDPBUF]={0};
	int sendBufLen;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( host.PortNo() );
	addr.sin_addr.s_addr = inet_addr(host.IpAddress().c_str());

	sendBufLen = CreateNewPacketBuffer( IPMSG_GETINFO,
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_GETINFO, sendBuf, sendBufLen, addr );
}

/**
 * 対象ホストのバージョン情報を取得。
 * ・バージョン情報を問い合わせる。
 * ・他のメソッド（ANSINFO受信）にて取得するまで待機。（五回まで）
 * ・IPアドレスでマッチングしてANSINFOで更新されたバージョン情報を返す。
 * @param host 対象のホスト
 * @retval 対象ホストのバージョン情報
 * 注：このメソッドはスレッドセーフでない。
 */
string
IpMessengerAgentImpl::GetInfo( HostListItem& host )
{
	RecvPacket();
	for( int i = 0; i < 5; i++ ) {
		RecvPacket();
	}
	vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( host.IpAddress() );
	if ( hostIt != hostList.end() ) {
		return hostIt->Version();
	}
	return "";
}

/**
 * 対象ホストの不在説明文字列情報を問い合わせる。
 * ・GETABSENCEINFOパケットを送信。
 * @param host 対象のホスト
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::QueryAbsenceInfo( HostListItem& host )
{
	char sendBuf[MAX_UDPBUF]={0};
	int sendBufLen;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( host.PortNo() );
	addr.sin_addr.s_addr = inet_addr(host.IpAddress().c_str());

	sendBufLen = CreateNewPacketBuffer( IPMSG_GETABSENCEINFO,
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_GETABSENCEINFO, sendBuf, sendBufLen, addr );
}

/**
 * 対象ホストの不在説明文字列情報を取得。
 * ・不在説明文字列情報を問い合わせる。
 * ・他のメソッド（ANSABSENCEINFO受信）にて取得するまで待機。（五回まで）
 * ・IPアドレスでマッチングしてANSABSENCEINFOで更新された不在説明文字列情報を返す。
 * @param host 対象のホスト
 * @retval 対象ホストの不在説明文字列情報
 * 注：このメソッドはスレッドセーフでない。
 */
string
IpMessengerAgentImpl::GetAbsenceInfo( HostListItem& host )
{
	QueryAbsenceInfo( host );
	RecvPacket();
	for( int i = 0; i < 5; i++ ) {
		RecvPacket();
	}
	vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( host.IpAddress() );
	if ( hostIt != hostList.end() ) {
		return hostIt->AbsenceDescription();
	}
	return "";
}

/**
 * 保持中のホストリストからグループリストを取得する。
 * @retval グループリスト
 * 注：このメソッドはスレッドセーフでない。
 */
vector<GroupItem>
IpMessengerAgentImpl::GetGroupList()
{
	vector<GroupItem> ret;
	for( vector<HostListItem>::iterator ixhost = hostList.begin(); ixhost != hostList.end(); ixhost++ ) {
		bool is_found = false;
		for( vector<GroupItem>::iterator ixret = ret.begin(); ixret != ret.end(); ixret++){
			if ( ixhost->GroupName() == ixret->GroupName() && ixhost->EncodingName() == ixret->EncodingName() ) {
				is_found = true;
				break;
			}
		}
		if ( !is_found ){
			GroupItem item;
			item.setGroupName( ixhost->GroupName() );
			item.setEncodingName( ixhost->EncodingName() );
			ret.push_back( item );
		}
	}
	return ret;
}

/**
 * 送信元にメッセージを削除したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::DeleteNotify( RecievedMessage msg )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;
	char *dmyptr;
	unsigned long packetNo = strtoul( msg.MessagePacket().Option().c_str(), &dmyptr, 10 );

	optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lu", packetNo );
	optBuf[optBufLen++] = '\0';
	sendBufLen = CreateNewPacketBuffer( IPMSG_DELMSG,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_DELMSG, sendBuf, sendBufLen, msg.MessagePacket().Addr() );
	return;
}

/**
 * 送信元にメッセージを開封したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::ConfirmMessage( RecievedMessage &msg )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

	if ( ( IPMSG_SECRETOPT & msg.MessagePacket().CommandOption() ) && !msg.IsConfirmed() ) {
		packetNoBufLen = snprintf( packetNoBuf, sizeof( packetNoBuf ), "%ld", msg.MessagePacket().PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_READMSG,
											  _LoginName, _HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( IPMSG_READMSG, sendBuf, sendBufLen, msg.MessagePacket().Addr() );
	}
	msg.setIsConfirmed( true );
//	RecvPacket();
}

/**
 * 送信済メッセージリストに開封されたことをマークする。
 * @param msg 送信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::AcceptConfirmNotify( SentMessage msg )
{
	vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( msg.PacketNo() );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmAnswered( true );
	}
}

// private methods start here

/**
 * 送信初期化
 * ・ブロードキャストアドレス構造体の初期化＋リストに押し込む。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::InitSend( const vector<NetworkInterface>& nics )
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(IPMSG_DEFAULT_PORT);
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	broadcastAddr.push_back( addr );

	for( unsigned int i = 0; i < nics.size(); i++ ){
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( nics[i].PortNo() );
		addr.sin_addr.s_addr = nics[i].NativeNetworkAddress() | ( 0xffffffff ^ nics[i].NativeNetMask() );
		bool IsFound = false;
		for( vector<struct sockaddr_in>::iterator i = broadcastAddr.begin(); i != broadcastAddr.end(); ++i ){
			if ( addr.sin_port == i->sin_port && addr.sin_addr.s_addr == i->sin_addr.s_addr ) {
				IsFound = true;
				break;
			}
		}
		if ( !IsFound ) {
			broadcastAddr.push_back( addr );
		}
	}
}

/**
 * TCPパケットの送信を行う。
 * @param sd ソケットディスクリプタ
 * @param buf バッファ
 * @param size バッファサイズ
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::SendTcpPacket( int sd, char *buf, int size )
{
#if defined(DEBUG)
	printf("== S E N D   T C P ====================================>\n");fflush( stdout );
#endif
	IpMsgPrintBuf( "SendTcpPacket:SendTcpBufer", buf, size );
	int ret = 0;
	ret = send( sd, buf, size + 1, 0 );
	if ( ret <= 0 ) {
		perror("send");
#if defined(DEBUG)
		printf("S E N D   T C P   F A I L E D\n");fflush( stdout );
#endif
	}
#if defined(DEBUG)
	printf("<= S E N D   T C P======================================\n");fflush( stdout );
#endif
}

/**
 * UDPパケットの送信を行う。
 * @param buf バッファ
 * @param size バッファサイズ
 * @param to_addr 送信先のIPアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::SendPacket( const unsigned long cmd, char *buf, int size, struct sockaddr_in to_addr )
{
#if defined(DEBUG)
	printf("== S E N D ============================================>\n");fflush( stdout );
	printf( "Command[%s]\n", GetCommandString( cmd ).c_str() );fflush( stdout );
	char ipAddrBuf[100];
	printf( "Send  %s(%d)\n", inet_ntoa_r( to_addr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), ntohs( to_addr.sin_port ) );fflush( stdout );
#endif
	IpMsgPrintBuf( "SendUdpPacket:SendUdpBuffer", buf, size );
	int ret = 0;
	int sock = udp_sd[0];
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = to_addr.sin_port;
	addr.sin_addr.s_addr = to_addr.sin_addr.s_addr;
	for( map<int,NetworkInterface>::iterator i = sd_addr.begin(); i != sd_addr.end(); ++i ){
#if defined(DEBUG)
		printf( "Send Check Before %s(%d)\n",
				i->second.IpAddress().c_str(),
				i->second.PortNo() );fflush( stdout );
		printf( "addr=%lx net=%lx\n", (long)i->second.NativeNetworkAddress(), (unsigned long)( to_addr.sin_addr.s_addr & i->second.NativeNetMask() ) );fflush( stdout );
#endif
		//同一ネットワーク上ではこのソケットを使う。
		if ( i->second.NativeNetworkAddress() == to_addr.sin_addr.s_addr & i->second.NativeNetMask() ) {
			sock = i->first;
#if defined(DEBUG)
			printf( "Send Check Found %s(%d)\n",
					i->second.IpAddress().c_str(),
					i->second.PortNo() );fflush( stdout );
#endif
#if 0
			addr.sin_family = AF_INET;
			addr.sin_port = htons( i->second.PortNo() );
			addr.sin_addr.s_addr = i->second.NativeIpAddress();
#endif
			break;
		}
	}
	printf( "Send  %s(%d)\n", inet_ntoa_r( addr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), ntohs( addr.sin_port ) );fflush( stdout );
//	ret = sendto( sock, buf, size + 1, 0, ( struct sockaddr * )&addr, sizeof( addr ) );
	ret = sendto( sock, buf, size + 1, 0, ( struct sockaddr * )&to_addr, sizeof( to_addr ) );
//	ret = sendto( udp_sd[0], buf, size + 1, 0, ( struct sockaddr * )&to_addr, sizeof( to_addr ) );
	if ( ret <= 0 ) {
		perror("sendto unicast");
#if defined(DEBUG)
		printf("S E N D   F A I L E D ( ret = %d)\n", ret );fflush( stdout );
#endif
	}
#if defined(DEBUG)
	printf("<= S E N D =============================================\n");fflush( stdout );
#endif
}

/**
 * UDPパケットのブロードキャストを行う。
 * ・ブロードキャストアドレスリストに登録済のアドレスに全て送信する。
 * @param buf バッファ
 * @param size バッファサイズ
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::SendBroadcast( const unsigned long cmd, char *buf, int size )
{
#if defined(DEBUG)
	printf("== S E N D   B R O A D C A S T ========================>\n");fflush( stdout );
	printf( "Command[%s]\n", GetCommandString( cmd ).c_str() );fflush( stdout );
#endif
	IpMsgPrintBuf( "SendBroadcast:SendUdpBroadcastBuffer", buf, size );
	for( vector<struct sockaddr_in>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
#if defined(DEBUG)
		char ipAddrBuf[100];
		printf( "Send To %s(%d)\n", inet_ntoa_r( ixaddr->sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), ntohs( ixaddr->sin_port ) );fflush( stdout );
#endif
		int ret = 0;
		for( unsigned int i = 0; i < udp_sd.size(); i++ ){
			ret = sendto( udp_sd[i], buf, size + 1, 0, ( struct sockaddr * )&(*ixaddr), sizeof( struct sockaddr ) );
			if ( ret <= 0 ) {
				perror("sendto broadcast.");
#if defined(DEBUG)
				printf("S E N D   F A I L E D\n");fflush( stdout );
#endif
			}
		}
	}
	for( vector<HostListItem>::iterator ixhost = hostList.begin(); ixhost != hostList.end(); ixhost++ ){
		if ( ixhost->CommandNo() | IPMSG_DIALUPOPT ) {
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons( ixhost->PortNo() );
			addr.sin_addr.s_addr = inet_addr( ixhost->IpAddress().c_str() );
#if defined(DEBUG)
			char ipAddrBuf[100];
			printf( "Send To %s(%d)\n", inet_ntoa_r( addr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), ntohs( addr.sin_port ) );fflush( stdout );
#endif
			int ret = 0;
			for( unsigned int i = 0; i < udp_sd.size(); i++ ){
				ret = sendto( udp_sd[i], buf, size + 1, 0, ( struct sockaddr * )&addr, sizeof( struct sockaddr ) );
				if ( ret <= 0 ) {
					perror("sendto dialup host.");
#if defined(DEBUG)
					printf("S E N D   F A I L E D ( D I A L U P )\n");fflush( stdout );
#endif
				}
			}
		}
	}
	//念のため自分にも
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( IPMSG_DEFAULT_PORT );
	addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
#if defined(DEBUG)
	char ipAddrBuf[100];
	printf( "Send To %s(%d)\n", inet_ntoa_r( addr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), ntohs( addr.sin_port ) );fflush( stdout );
#endif
	int ret = sendto( udp_sd[0], buf, size + 1, 0, ( struct sockaddr * )&addr, sizeof( struct sockaddr ) );
	if ( ret <= 0 ) {
		perror("sendto myself.");
#if defined(DEBUG)
		printf("S E N D   F A I L E D ( D I A L U P )\n");fflush( stdout );
#endif
	}

#if defined(DEBUG)
	printf("<= S E N D   B R O A D C A S T =========================\n");fflush( stdout );
#endif
}

/**
 * 受信初期化。
 * ・ブロードキャストアドレスに関するUDP受信初期化。
 * ・指定のNICに対してUDPに関する受信初期化。
 * ・指定のNICに対してTCPに関する受信初期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::InitRecv( const vector<NetworkInterface>& nics )
{
	if ( nics.size() > 0 ) {
		HostAddress = nics[0].IpAddress();
	}

	udp_sd.clear();
	tcp_sd.clear();
	sd_addr.clear();
	for( unsigned int i = 0; i < nics.size(); i++ ){
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( nics[i].PortNo() );
		addr.sin_addr.s_addr = inet_addr( nics[i].IpAddress().c_str() );

		int sock = -1;

		sock = InitUdpRecv( addr );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			char ipAddrBuf[100];
			printf( "UDP_SD[%d][%s] = %d\n", udp_sd.size(), inet_ntoa_r( addr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), sock );fflush( stdout );
#endif
			udp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
		} else {
			printf( "UDP Error[%s]=%s\n", nics[i].DeviceName().c_str(), nics[i].IpAddress().c_str() );fflush( stdout );
		}
		sock = InitTcpRecv( addr );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			char ipAddrBuf[100];
			printf( "TCP_SD[%d][%s] = %d\n", tcp_sd.size(), inet_ntoa_r( addr.sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), sock );fflush( stdout );
#endif
			tcp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
		} else {
			printf( "TCP Error[%s]=%s\n", nics[i].DeviceName().c_str(), nics[i].IpAddress().c_str() );fflush( stdout );
		}
	}

	for( vector<struct sockaddr_in>::iterator addr = broadcastAddr.begin(); addr != broadcastAddr.end(); addr++ ){
		int sock = -1;

		sock = InitUdpRecv( *addr );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			char ipAddrBuf[100];
			printf( "BROADCAST UDP_SD[%d][%s] = %d\n", udp_sd.size(), inet_ntoa_r( addr->sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ), sock );fflush( stdout );
#endif
			udp_sd.push_back( sock );
		} else {
			char ipAddrBuf[100];
			printf( "UDP Error=%s\n", inet_ntoa_r( addr->sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ) );fflush( stdout );
		}
	}

	FD_ZERO( &rfds );

	max_sd = -1;

	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		FD_SET( udp_sd[i], &rfds );
		if ( max_sd < udp_sd[i] ){
			max_sd = udp_sd[i];
		}
	}
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		FD_SET( tcp_sd[i], &rfds );
		if ( max_sd < tcp_sd[i] ){
			max_sd = tcp_sd[i];
		}
	}
}

/**
 * UDPに関する受信初期化。
 * ・2425待ち受けのUDPソケットを準備する
 * ・UDPはbroadcast許可
 * 注：このメソッドはスレッドセーフでない。
 */
int
IpMessengerAgentImpl::InitUdpRecv( struct sockaddr_in addr )
{
	int sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0 ){
		perror("bind(udp)");
		close( sock );
		return -1;
	}
	int yes = 1;
	if ( setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof(yes)) != 0 ) {
		perror("setsockopt(broadcast)");
		close( sock );
		return -1;
	}
	int buf_size = MAX_SOCKBUF, buf_minsize = MAX_SOCKBUF / 2;
	if ( setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(sendbuf)");
		close( sock );
		return -1;
	}
	if ( setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(recvbuf)");
		close( sock );
		return -1;
	}

#if 0
	//マルチキャスト送信用のインターフェースの指定	
	in_addr_t my_if = addr.sin_addr.s_addr;
	if ( setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&my_if, sizeof(my_if) ) != 0 ){
		perror("setsockopt(multicast_if)");
		close( sock );
		return -1;
	}
	//マルチキャスト受信の準備	
	struct ip_mreq mreq;
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_interface.s_addr = INADDR_ANY;
	mreq.imr_interface.s_addr = addr.sin_addr.s_addr;
	mreq.imr_multiaddr.s_addr = addr.sin_addr.s_addr;
	mreq.imr_multiaddr.s_addr = inet_addr("192.168.163.255");
	if ( setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof( mreq ) ) != 0 ) {
		perror("setsockopt(add_membership)");
		close( sock );
		return -1;
	}
#endif

	return sock;
}

/**
 * TCPに関する受信初期化。
 * ・2425待ち受けのTCPソケットを準備する
 * ・TCPはREUSEADDR
 * ・litsenは5ポート
 * 注：このメソッドはスレッドセーフでない。
 */
int
IpMessengerAgentImpl::InitTcpRecv( struct sockaddr_in addr )
{
	int sock = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sock >= 0 && bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0 ){
		perror("bind(tcp)");
		close( sock );
		return -1;
	}
	int yes = 1;
	if ( sock >= 0 && setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) != 0 ) {
		perror("setsockopt(reuseaddr)");
		close( sock );
		return -1;
	}
	if ( sock >= 0 && listen(sock, 5 ) != 0 ) {
		perror("setsockopt(reuseaddr)");
		close( sock );
		return -1;
	}
	return sock;
}

/**
 * 受信処理（ユーザ向け）。
 * 注：このメソッドはスレッドセーフでない。
 * @retval 受信パケット数
 */
int
IpMessengerAgentImpl::Process()
{
	return RecvPacket();
}

/**
 * 受信処理（内部）。
 * ・select(タイムアウト付き)にて受信待ち。
 * ・受信処理を行い、パケットをキューにため込む。
 * ・受信が終了したら、キューの中身を処理する。（各イベントを呼び出す。）
 * 注：このメソッドはスレッドセーフでない。
 * @retval 受信パケット数
 */
int
IpMessengerAgentImpl::RecvPacket()
{
	char buf[MAX_UDPBUF];
	int selret = 1;
	int ret = 0;
	time_t nowTime = time( NULL );

	vector<Packet> pack_que;

	while( selret > 0 ) {
		fd_set fds;
		memcpy( &fds, &rfds, sizeof( fd_set ) );

		memset( buf, 0, sizeof( buf ) );
		tv.tv_sec = SELECT_TIMEOUT_SEC;
		tv.tv_usec = SELECT_TIMEOUT_USEC;
		selret = select( max_sd + 1, &fds, NULL, NULL, &tv );
		if ( selret == -1 ) {
			if ( errno == EINTR ){
				continue;
			}
			perror( "select()" );
			break;
		} else if ( selret == 0 ){
#if defined(INFO) || !defined(NDEBUG)
			printf(".");fflush( stdout );
#endif
			break;
		} else {
			int tcp_socket = -1;
#if defined(DEBUG)
			printf("\n");fflush( stdout );
			printf( "select returns == %d\n\n", selret );fflush( stdout );
#endif
			struct sockaddr_in sender_addr;
			int sz = sizeof( buf );
			bool recieved = RecvUdp( &fds, &sender_addr, &sz, buf );
			//UDPでソケットに変化がない。
			tcp_socket = -1;
			if ( !recieved ) {
				sz = sizeof( buf );
				recieved = RecvTcp( &fds, &sender_addr, &sz, buf, &tcp_socket );
				//UDP,TCPでソケットに変化がない。
				if ( !recieved ) {
					continue;
				}
			}
			Packet packet = DismantlePacketBuffer( buf, sz, sender_addr, nowTime );
#if defined(INFO) || !defined(NDEBUG)
			printf("recv from[%s]\n", packet.HostName().c_str() );fflush( stdout );
#endif
			packet.setTcpSocket( tcp_socket );
			//同一セッション内だけパケットの一意性をチェック。重複パケットは無視
			if ( !FindDuplicatePacket( packet ) ) {
				IpMsgDumpPacket( packet, packet.Addr() );
				pack_que.push_back( packet );
				PacketsForChecking.push_back( packet );
				ret++;
			}
		}
	}
	//パケットを処理する。
	while( !pack_que.empty() ) {
		DoRecvCommand( pack_que.front() );
		pack_que.erase( pack_que.begin() );
	}

	//一定以上前のチェック用のパケットベクタを消す。
	PurgePacket( nowTime );

	//メッセージ送信リトライのチェック
	CheckSendMsgRetry( nowTime );

	//ホストリスト取得のリトライチェック
	CheckGetHostListRetry( nowTime );

	return ret;
}

/**
 * TCPパケットを受信し、パケットオブジェクトを生成する。
 * @param fds FD_SET構造体
 * @param sender_addr IPアドレスのアドレス
 * @param sz 受信バッファのサイズのアドレス
 * @param buf 受信バッファのアドレス
 */
bool
IpMessengerAgentImpl::RecvUdp( fd_set *fds, struct sockaddr_in *sender_addr, int *sz, char *buf )
{
	socklen_t sender_addr_len = 0;
	bool recieved = false;
	int size = *sz;

	//UDPでソケットに変化が有ったら受信
	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		if ( FD_ISSET( udp_sd[i], fds ) ){
			memset( sender_addr, 0, sizeof( struct sockaddr_in ) );
			sender_addr_len = sizeof( struct sockaddr_in );
			size = recvfrom( udp_sd[i], buf, size, 0, (struct sockaddr *)sender_addr, &sender_addr_len );
			if ( size < 0 ) {
				perror("recvfrom");
			}
#if defined(DEBUG)
			printf( "Recieved UDP_SD[%d] == %d\n", i, udp_sd[i] );fflush( stdout );
			printf( "  IpAddr == 0x%08lx\n", (unsigned long)sender_addr->sin_addr.s_addr );fflush( stdout );
			printf( "  PortNo == %d\n", sender_addr->sin_port );fflush( stdout );
#endif
			IpMsgPrintBuf( "recvfrom buf", buf, size );
			recieved = true;
			break;
		}
	}
	return recieved;
}

/**
 * TCPパケットを受信し、パケットオブジェクトを生成する。
 * @param fds FD_SET構造体
 * @param sender_addr IPアドレスのアドレス
 * @param sz 受信バッファのサイズのアドレス
 * @param buf 受信バッファのアドレス
 * @param tcp_socket acceptしたTCPソケット
 */
bool
IpMessengerAgentImpl::RecvTcp( fd_set *fds, struct sockaddr_in *sender_addr, int *sz, char *buf, int *tcp_socket )
{
	socklen_t sender_addr_len = 0;
	bool recieved = false;
	int size = *sz;

	//TCPでソケットに変化が有ったら受信
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		if ( FD_ISSET( tcp_sd[i], fds ) ){
			memset( sender_addr, 0, sizeof( struct sockaddr_in ) );
			sender_addr_len = sizeof( struct sockaddr_in );
			*tcp_socket = accept( tcp_sd[i], (struct sockaddr *)sender_addr, &sender_addr_len );
			if ( *tcp_socket < 0 ) {
				perror("accept");
			}
			size = recv( *tcp_socket, buf, size, 0 );
			if ( size < 0 ) {
				perror("recv");
			}
#if defined(INFO) || !defined(NDEBUG)
			printf( "Recieved UDP_SD[%d] == %d\n", i, tcp_sd[i] );fflush( stdout );
#endif
			IpMsgPrintBuf( "recv buf", buf, size );
			recieved = true;
			break;
		}
	}
	return recieved;
}

/**
 * 過去のパケットから重複パケットを検索する。
 * @param packet パケット
 * @retval true:存在、false:存在しない。
 */
bool
IpMessengerAgentImpl::FindDuplicatePacket( const Packet &packet )
{
	for( int i = (int)PacketsForChecking.size() - 1; i >= 0; i-- ){
		if ( PacketsForChecking[i].PacketNo()             == packet.PacketNo() &&
			 PacketsForChecking[i].Addr().sin_addr.s_addr == packet.Addr().sin_addr.s_addr &&
			 PacketsForChecking[i].Addr().sin_port        == packet.Addr().sin_port ) {
			return true;
		}
	}
	return false;
}

/**
 * 過去のパケットから必要ない（古い）パケットを削除する。
 * @param nowTime システム時刻
 */
void
IpMessengerAgentImpl::PurgePacket( time_t nowTime )
{
	for( vector<Packet>::iterator pack = PacketsForChecking.begin(); pack != PacketsForChecking.end(); pack++ ){
		if ( nowTime > pack->Recieved() + PACKET_CHECK_FOR_SAVING_INTERVAL ) {
			pack = PacketsForChecking.erase( pack ) - 1;
		} else {
			break;
		}
	}
}

/**
 * メッセージ送信のリトライの必要性をチェックし、必要ならリトライを行う。
 * @param nowTime システム時刻
 */
void
IpMessengerAgentImpl::CheckSendMsgRetry( time_t nowTime )
{
	for( vector<SentMessage>::iterator ixmsg = sentMsgList.begin(); ixmsg != sentMsgList.end(); ixmsg++ ) {
		if ( ixmsg->needSendRetry( nowTime ) ) {
			//再送信
			ixmsg->setRetryCount( ixmsg->RetryCount() + 1 );
			ixmsg->setPrevTry( nowTime );
			SendMsg( ixmsg->Host(),
					 ixmsg->Message(),
					 ixmsg->IsSecret(),
					 ixmsg->Files(),
					 ixmsg->IsPasswordLock(),
					 ixmsg->HostCountAtSameTime(),
					 ixmsg->IsNoLogging(),
					 ixmsg->Opt(),
					 true,
					 ixmsg->PacketNo() );
		}
		if ( ixmsg->isRetryMaxOver() ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("Retry Max Over\n");fflush( stdout );
#endif
			ixmsg->setRetryCount( 0 );
			ixmsg->setIsRetryMaxOver( true );
			if ( event != NULL ){
				//リトライを続ける場合はTrueをセット。続けない場合はFalseをセット。
				//（RetryMaxOver(メッセージはエラー)状態にすれば、継続しません）
				//イベントの戻り値はtrue:継続、false:中断になります。
				ixmsg->setIsRetryMaxOver( !event->SendRetryError( *ixmsg ) );
			}
			//イベントで継続を設定しない場合はリトライマックスオーバーしたらやめる。
		}
	}
}

/**
 * ホストリスト取得のリトライの必要性をチェックし、必要ならリトライを行う。
 * @param nowTime システム時刻
 */
void
IpMessengerAgentImpl::CheckGetHostListRetry( time_t nowTime )
{
	if ( hostList.IsAsking() ){
		hostList.setPrevTry( time( NULL ) );
		if ( hostList.PrevTry() - hostList.AskStartTime() > GETLIST_RETRY_INTERVAL ) {
			hostList.setAskStartTime( time( NULL ) );
			hostList.setPrevTry( hostList.AskStartTime() );
			hostList.setRetryCount( hostList.RetryCount() + 1 );
			if ( hostList.RetryCount() < GETLIST_RETRY_MAX ) {
				UpdateHostList( true );
			} else {
				hostList.setAskStartTime( 0L );
				hostList.setPrevTry( 0L );
				hostList.setRetryCount( 0 );
				hostList.setIsAsking( false );
				if ( event != NULL ) {
					//リトライを続ける場合はTrueをセット。続けない場合はFalseをセット。
					//イベントの戻り値はtrue:継続、false:中断になります。
					hostList.setIsAsking( event->GetHostListRetryError() );
				}
			}
		}
	}
}

/**
 * パケットのコマンドモードで受信イベントを振り分ける。
 * @param packet パケットオブジェクト
 */
void
IpMessengerAgentImpl::DoRecvCommand( const Packet& packet )
{
#if defined(DEBUG)
	printf( "PACKET.COMMAND=[%s]\n", GetCommandString( packet.CommandMode() ).c_str() );fflush( stdout );
#endif
	switch( packet.CommandMode() ) {
		case IPMSG_NOOPERATION:     UdpRecvEventNoOperation( packet ); break;
		case IPMSG_BR_ENTRY:        UdpRecvEventBrEntry( packet ); break;
		case IPMSG_BR_EXIT:         UdpRecvEventBrExit( packet ); break;
		case IPMSG_ANSENTRY:        UdpRecvEventAnsEntry( packet ); break;
		case IPMSG_BR_ABSENCE:      UdpRecvEventBrAbsence( packet );break;
		case IPMSG_BR_ISGETLIST:    UdpRecvEventBrIsGetList( packet ); break;
		case IPMSG_OKGETLIST:       UdpRecvEventOkGetList( packet ); break;
		case IPMSG_GETLIST:         UdpRecvEventGetList( packet ); break;
		case IPMSG_ANSLIST:         UdpRecvEventAnsList( packet ); break;
		case IPMSG_BR_ISGETLIST2:   UdpRecvEventBrIsGetList2( packet ); break;
		case IPMSG_SENDMSG:         UdpRecvEventSendMsg( packet ); break;
		case IPMSG_RECVMSG:         UdpRecvEventRecvMsg( packet ); break;
		case IPMSG_READMSG:         UdpRecvEventReadMsg( packet); break;
		case IPMSG_DELMSG:          UdpRecvEventDelMsg( packet); break;
		case IPMSG_ANSREADMSG:      UdpRecvEventAnsReadMsg( packet ); break;
		case IPMSG_GETINFO:         UdpRecvEventGetInfo( packet ); break;
		case IPMSG_SENDINFO:        UdpRecvEventSendInfo( packet ); break;
		case IPMSG_GETABSENCEINFO:  UdpRecvEventGetAbsenceInfo( packet ); break;
		case IPMSG_SENDABSENCEINFO: UdpRecvEventSendAbsenceInfo( packet ); break;
		case IPMSG_GETFILEDATA:     TcpRecvEventGetFileData( packet); break;
		case IPMSG_RELEASEFILES:    UdpRecvEventReleaseFiles( packet ); break;
		case IPMSG_GETDIRFILES:     TcpRecvEventGetDirFiles( packet ); break;
		case IPMSG_GETPUBKEY:       UdpRecvEventGetPubKey( packet ); break;
		case IPMSG_ANSPUBKEY:       UdpRecvEventAnsPubKey( packet ); break;
	}
}

/**
 * 電文受信イベント：NOOPERATION
 * ・何もしない
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventNoOperation( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvNoOperation\n");fflush( stdout );
#endif
	return 0;
}

/**
 * 電文送信(NOOPERATION)
 * ・NOOPERATIONを送信。
 */
int
IpMessengerAgentImpl::SendNoOperation()
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( IPMSG_NOOPERATION,
										  _LoginName, _HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_NOOPERATION, sendBuf, sendBufLen );
	return 0;
}

/**
 * 電文受信イベント：BR_ENTRY
 * ・送信元にANSENTRYを送信する。
 * ・不在モードの場合、不在として送信。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrEntry( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrEntry\n");fflush( stdout );
#endif
	if ( _IsAbsence ) {
		string AbsenceName = "";
		for( vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceName = i->AbsenceName();
				break;
			}
		}
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s[%s]", Nickname.c_str(), AbsenceName.c_str() );
	} else {
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s", Nickname.c_str() );
	}
	optBuf[optBufLen] = '\0';
	optBufLen++;
	snprintf( &optBuf[ optBufLen ], sizeof( optBuf ) - optBufLen - 1, "%s", GroupName.c_str() );
	optBufLen += GroupName.size();
	optBuf[optBufLen ] = '\0';
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_ANSENTRY ),
										_LoginName, _HostName,
										optBuf, optBufLen,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_ANSENTRY, sendBuf, sendBufLen, packet.Addr() );
	// ホストリストに追加
	AddHostListFromPacket( packet ); 
	if ( event != NULL ) {
		event->EntryAfter( hostList );
	}
	return 0;
}

/**
 * 電文送信（BR_ABSENCE）
 * ・不在通知／不在解除電文を送信する。
 */
int
IpMessengerAgentImpl::SendAbsence()
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;

#if defined(INFO) || !defined(NDEBUG)
	printf("SendBrAbsence\n");fflush( stdout );
#endif
	if ( _IsAbsence ) {
		string AbsenceName = "";
		for( vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceName = i->AbsenceName();
				break;
			}
		}
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s[%s]", Nickname.c_str(), AbsenceName.c_str() );
	} else {
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s", Nickname.c_str() );
	}
	optBuf[optBufLen] = '\0';
	optBufLen++;
	snprintf( &optBuf[ optBufLen ], sizeof( optBuf ) - optBufLen - 1, "%s", GroupName.c_str() );
	optBufLen += GroupName.size();
	optBuf[optBufLen ] = '\0';

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ABSENCE ),
										_LoginName, _HostName,
										optBuf, optBufLen,
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ABSENCE, sendBuf, sendBufLen );
	return 0;
}

/**
 * 電文受信イベント：BR_ABSENCE
 * ・自分のホストリストを更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrAbsence( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrAbsence\n");fflush( stdout );
#endif
	hostList.DeleteHost( packet.HostName() );
	hostList.AddHost( HostList::CreateHostListItemFromPacket( packet ) );
	if ( event != NULL ){
		event->AbsenceModeChangeAfter( hostList );
	}
	return 0;
}

/**
 * 電文受信イベント：BR_EXIT
 * ・自分のホストリストからホストを削除する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrExit( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrExit\n");fflush( stdout );
#endif
	hostList.DeleteHost( packet.HostName() );
	if ( event != NULL ) {
		event->ExitAfter( hostList );
	}
	return 0;
}

/**
 * 電文受信イベント：BR_RECVMSG
 * ・自分の送信済メッセージリストの該当メッセージに送信済フラグを立てる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventRecvMsg( const Packet& packet )
{
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packetNo );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsSent( true );
		sentMsg->setRetryCount( 0 );
		sentMsg->setIsRetryMaxOver( true );
		if ( event != NULL ){
			event->SendAfter( *sentMsg );
		}
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvRecvMsg\n");fflush( stdout );
#endif
	return 0;
}

/**
 * 電文受信イベント：BR_READMSG
 * ・READCHECKOPTが付いている場合、ANSREADMSGを投げる。
 * ・自分の送信済メッセージリストの該当メッセージに既読フラグを立てる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventReadMsg( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;
	
	if ( packet.CommandOption() & IPMSG_READCHECKOPT ) {
		packetNoBufLen = snprintf( packetNoBuf, sizeof( packetNoBuf ), "%ld", packet.PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_ANSREADMSG,
											  _LoginName, _HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( IPMSG_ANSREADMSG, sendBuf, sendBufLen, packet.Addr() );
	}

	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmed( true );
		if ( event != NULL ) {
			event->OpenAfter( *sentMsg );
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvReadMsg\n");fflush( stdout );
#endif
	return 0;
}

/**
 * 電文受信イベント：BR_DELMSG
 * ・自分の送信済メッセージリストの該当メッセージを削除。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )
{
	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsgList.erase(sentMsg);
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvDelMsg\n");fflush( stdout );
#endif
	return 0;
}

/**
 * 電文受信イベント：BR_ANSREADMSG
 * ・何もしない。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsReadMsg( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsReadMsg\n");fflush( stdout );
#endif
	return 0;
}

/**
 * 電文受信イベント：BR_SENDMSG
 * ・BROADCASTOPT or AUTORETOPTなら自動応答しない。
 * ・SENDCHECKOPT付きならRECVMSGを投げる。
 * ・自分が不在なら不在応答をする。
 * ・暗号化メッセージなら復号。
 * ・受信済メッセージに追加。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendMsg( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

#if defined(DEBUG) || !defined(NDEBUG)
	printf("受信済メッセージ件数(%d)\n", recvMsgList.size() );fflush( stdout );
#endif
	for( vector<RecievedMessage>::iterator ixmsg = recvMsgList.begin(); ixmsg != recvMsgList.end(); ixmsg++ ) {
#if defined(DEBUG) || !defined(NDEBUG)
			printf("受信メッセージ検索中...現在処理中のパケット(%ld) 検索中のメッセージのパケット(%ld)\n",
					packet.PacketNo(), ixmsg->MessagePacket().PacketNo() );fflush( stdout );
#endif
		if ( packet.PacketNo() == ixmsg->MessagePacket().PacketNo() ) {
#if defined(DEBUG) || !defined(NDEBUG)
			printf("すでに追加済み\n");fflush( stdout );
#endif
			return 0;
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvSendMsg[Packet = %lu]\n", packet.PacketNo() );fflush( stdout );
#endif
	bool noRaiseEvent = false;
	if ( packet.CommandOption() & IPMSG_BROADCASTOPT ||  packet.CommandOption() & IPMSG_AUTORETOPT ) {
		;
	} else {
		if ( packet.CommandOption() & IPMSG_SENDCHECKOPT ) {
			packetNoBufLen = snprintf( packetNoBuf, sizeof( packetNoBuf ), "%ld", packet.PacketNo() );
			sendBufLen = CreateNewPacketBuffer( IPMSG_RECVMSG,
												  _LoginName, _HostName,
												  packetNoBuf, packetNoBufLen,
												  sendBuf, sizeof( sendBuf ) );
			SendPacket( IPMSG_RECVMSG, sendBuf, sendBufLen, packet.Addr() );
		}
		if ( _IsAbsence ) {
			HostListItem host;
			char ipAddrBuf[100];
			host.setIpAddress( inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ) );
			host.setPortNo( ntohs( packet.Addr().sin_port ) );
			host.setEncodingName( localEncoding );
			vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( host.IpAddress() );
			if ( hostIt != hostList.end() ) {
				host.setEncodingName( hostIt->EncodingName() );
			}
			string AbsenceDescription = "";
			for( vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
				if ( i->EncodingName() == localEncoding ) {
					AbsenceDescription = i->AbsenceDescription();
					break;
				}
			}
			SendMsg( host, AbsenceDescription.c_str(), false, 1, true );
		}
	}

	string optionMessage = packet.Option();
	if ( packet.CommandOption() & IPMSG_ENCRYPTOPT ){
		if ( !DecryptMsg( packet, optionMessage ) ) {
			HostListItem host;
			char ipAddrBuf[100];
			host.setIpAddress( inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ) );
			host.setPortNo( ntohs( packet.Addr().sin_port ) );
			SendMsg( host, DecryptErrorMessage.c_str(), false, 1, true, IPMSG_AUTORETOPT );
			optionMessage = "";
			//暗号解除失敗による自動応答時はイベントを起こさない。
			noRaiseEvent = true;
		}
	}
	RecievedMessage message;
	message.setMessagePacket( packet );
	message.setMessage( optionMessage );
	message.setRecieved( time( NULL ) );
	message.setIsNoLogging( IPMSG_NOLOGOPT & packet.CommandOption() );
	message.setIsSecret( IPMSG_SECRETOPT & packet.CommandOption() );
	message.setIsCrypted( IPMSG_ENCRYPTOPT & packet.CommandOption() );
	message.setIsPasswordLock( IPMSG_PASSWORDOPT & packet.CommandOption() );
	message.setIsMulticast( IPMSG_MULTICASTOPT & packet.CommandOption() );
	message.setIsBroadcast( IPMSG_BROADCASTOPT & packet.CommandOption() );
	message.setIsConfirmed( false );
	for( vector<HostListItem>::iterator ixhost = hostList.begin(); ixhost != hostList.end(); ixhost++ ) {
		if ( ixhost->UserName() == packet.UserName() && ixhost->HostName() == packet.HostName() ) {
			message.setHost( *ixhost );
			break;
		}
	}

	message.setHasAttachFile( false );
	AttachFileList files = message.Files();
	if ( CreateAttachedFileList( packet.Option().c_str(), files ) != 0 ) {
		message.setHasAttachFile( true );
	}
	bool eventRet = false;
	message.setFiles( files );
	if ( !noRaiseEvent && event != NULL ) {
		eventRet = event->RecieveAfter( message );
	}
	if ( SaveRecievedMessage() && !eventRet ){
		recvMsgList.append( message );
	}
	return 0;
}

/**
 * 電文受信イベント：BR_ISGETLIST
 * ・OKGETLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrIsGetList( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrIsGetList\n");fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_OKGETLIST ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_OKGETLIST, sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_ISGETLIST2
 * ・OKGETLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrIsGetList2( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrIsGetList2\n");fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_OKGETLIST ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_OKGETLIST, sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_GETLIST
 * ・ANSLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetList( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	int start = 0;
	char *dmy;
	string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetList[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	start = strtoul( packet.Option().c_str(), &dmy, 10 );
	hosts = hostList.ToString( start );
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_ANSLIST ),
										_LoginName, _HostName,
										hosts.c_str(), hosts.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_ANSLIST, sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_OKGETLIST
 * ・GETLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventOkGetList( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvOkGetList[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_GETLIST ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_GETLIST, sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_ANSENTRY
 * ・何もしない。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsEntry( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsEntry\n");fflush( stdout );
#endif
	// ホストリストに追加
	AddHostListFromPacket( packet ); 
	if ( event != NULL ) {
		event->EntryAfter( hostList );
	}
	return 0;
}

/**
 * 電文受信イベント：BR_ANSLIST
 * ・要求に応じたホストリストの部分をGETLISTに詰めて投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsList( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char nextBuf[1024];

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsList\n");fflush( stdout );
#endif
	AddDefaultHost();
	char ipAddrBuf[100];
	int nextstart = CreateHostList( inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ),
									packet.HostName().c_str(),
									packet.Option().c_str(),
									packet.Option().length() );
	if ( nextstart > 0 ) {
		int nextBufLen = snprintf( nextBuf, sizeof( nextBuf ), "%d", hostList.size() + 1 );
#if defined(INFO) || !defined(NDEBUG)
		printf("nextBufLen = %d\n", nextBufLen );fflush( stdout );
#endif
		sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_GETLIST ),
											_LoginName, _HostName,
											nextBuf, nextBufLen,
											sendBuf, sizeof( sendBuf ) );
		SendPacket( IPMSG_GETLIST, sendBuf, sendBufLen, packet.Addr() );
	}
	string packetIpAddress = inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	for( unsigned int i = 0; i < NICs.size(); i++ ){
		if ( packetIpAddress == NICs[i].IpAddress() ){
			return 0;
		}
	}
	//自分以外からのホストリスト通知があれば、リトライ関連変数をクリア。
	hostList.setIsAsking( false );
	hostList.setAskStartTime( 0L );
	hostList.setPrevTry( 0L );
	hostList.setRetryCount( 0 );
	return 0;
}

/**
 * 電文受信イベント：BR_GETINFO
 * ・バージョン情報をSENDINFOに詰めて投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetInfo( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	string version = IPMSG_AGENT_VERSION;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetInfo[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_SENDINFO ),
										_LoginName, _HostName,
										version.c_str(), version.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_SENDINFO, sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_SENDINFO
 * ・取得したバージョン情報をホストリストに更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendInfo( const Packet& packet )
{
	char ipAddrBuf[100];
	string pIpAddress = inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
		hostIt->setVersion( packet.Option() );
		if ( event != NULL ){
			event->VersionInfoRecieveAfter( *hostIt, packet.Option() );
		}
	}
	return 0;
}

/**
 * 電文受信イベント：BR_GETABSENCEINFO
 * ・不在詳細情報をSENDINFOに詰めて投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo( const Packet& packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetAbsenceInfo[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	string AbsenceDescription = "";
	if ( _IsAbsence  ){
		char ipAddrBuf[100];
		string IpAddress = inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
		string EncodingName = localEncoding;
		vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( IpAddress );
		if ( hostIt != hostList.end() ) {
			EncodingName = hostIt->EncodingName();
		}
		for( vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceDescription = i->AbsenceDescription();
				break;
			}
		}
	} else {
		AbsenceDescription = "Not Absence mode";
	}
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_SENDABSENCEINFO ),
										_LoginName, _HostName,
										AbsenceDescription.c_str(), AbsenceDescription.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_SENDABSENCEINFO, sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_SENDABSENCEINFO
 * ・取得した不在詳細情報をホストリストに更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendAbsenceInfo( const Packet& packet )
{
	char ipAddrBuf[100];
	string pIpAddress = inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
		hostIt->setAbsenceDescription( packet.Option() );
		if ( event != NULL ){
			event->AbsenceDetailRecieveAfter( *hostIt, packet.Option() );
		}
	}
	return 0;
}

/**
 * パケットからオフセットを取得します。
 * ・パケットからオフセットを抽出し返します。
 * @param packet パケットオブジェクト
 * @retval ファイルオフセット。
 */
static unsigned long
GetSendFileOffsetInPacket( const Packet& packet )
{
	char *dmyptr;
	char *startptr;
	strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;
	strtoul( startptr, &dmyptr, 16 );
	startptr = ++dmyptr;
	unsigned long offset = strtoul( startptr, &dmyptr, 16 );

	return offset;
}

/**
 * 電文受信イベント：BR_GETFILEDATA
 * ・ファイル情報をTCPソケットにのせて送信し、その後ファイルをダウンロードさせる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::TcpRecvEventGetFileData( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "TcpRecvEventGetFileData\n" );fflush( stdout );
#endif

	pthread_t t_id;

	Packet *packetClone = new Packet( packet );

	if ( pthread_create( &t_id, NULL, GetFileDataThread, (void *)packetClone ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_create");
		return -1;
	}
	if ( pthread_detach( t_id ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_detach");
		return -1;
	}
	return 0;
}

/**
 * ファイルダウンロードスレッド
 * ・ファイルをダウンロードさせる。
 * @param param パケットオブジェクト(void*)
 */
void *
GetFileDataThread( void *param )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "GetFileDataThread\n" );fflush( stdout );
#endif

	Packet *packet = (Packet *)param;

	vector<SentMessage>::iterator msg = IpMessengerAgentImpl::GetInstance()->GetSentMessages()->FindSentMessageByPacket( *packet );
	if ( msg == IpMessengerAgentImpl::GetInstance()->GetSentMessages()->end() ){
		close( packet->TcpSocket() );
		delete packet;
		return 0;
	}
	vector<AttachFile>::iterator FoundFile = msg->FindAttachFileByPacket( *packet );
	if ( FoundFile == msg->Files().end() ){
		close( packet->TcpSocket() );
		delete packet;
		return 0;
	}

	FoundFile->setIsDownloading( true );
	IpMessengerAgentImpl::GetInstance()->SendFile( packet->TcpSocket(),
												   FoundFile->FullPath(),
												   FoundFile->MTime(),
												   FoundFile->FileSize(),
												   &(*FoundFile),
												   GetSendFileOffsetInPacket( *packet ) );
	FoundFile->setIsDownloading( false );
	FoundFile->setIsDownloaded( true );
	close( packet->TcpSocket() );
	delete packet;
	return NULL;
}

/**
 * TODO 何するの？
 * 電文受信イベント：BR_RELEASEFILES
 * ・ TODO 何するの？
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventReleaseFiles( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "TcpRecvEventReleaseFiles\n" );fflush( stdout );
#endif
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packetNo );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsgList.erase(sentMsg);
	}
	return 0;
}

/**
 * 電文受信イベント：BR_GETPUBKEY
 * ・RSA公開鍵をANSPUBKEYにのせて送信する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetPubKey( const Packet& packet )
{
#ifdef HAVE_OPENSSL
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetPubKey[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	char *dmyptr;
	unsigned long cap = strtoul( packet.Option().c_str(), &dmyptr, 16 );
	RSA *rsa = GetOptimizedRsa( cap );
	if ( rsa != NULL ){
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx:%s-%s", encryptionCapacity, BN_bn2hex(rsa->e), BN_bn2hex(rsa->n) );
		sendBufLen = CreateNewPacketBuffer( IPMSG_ANSPUBKEY,
											  _LoginName, _HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( IPMSG_ANSPUBKEY, sendBuf, sendBufLen, packet.Addr() );
	}
#endif
	return 0;
}

/**
 * 電文受信イベント：BR_ANSPUBKEY
 * ・取得したRSA公開鍵をホストリストに更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsPubKey( const Packet& packet )
{
#ifdef HAVE_OPENSSL
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsPubKey[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	//OptionはHex表現で
	//XXXXX:EEEEE-NNNNN
	//XXXXX=能力フラグのHEX表現
	//EEEEE=RSA公開キー（指数）
	//NNNNN=RSAモジュール
	char *opt = (char *)calloc( packet.Option().length() + 1, 1 );
	if ( opt == NULL ){
		return 0;
	}
	memcpy( opt, packet.Option().c_str(), packet.Option().length() );
	opt[packet.Option().length()] = 0;
	char *nextpos;
	char *token = strtok_r( opt,PACKET_DELIMITER_STRING, &nextpos );
	unsigned long cap = 0UL;
	if ( token != NULL ){
		char *dmyptr;
		cap = strtoul( opt, &dmyptr, 16 );
	} else {
		free( opt );
		return 0;
	}
	token = nextpos;
	token = strtok_r( token, "-", &nextpos );
	string meth;
	if ( nextpos != NULL ) {
		meth = token;
	} else {
		free( opt );
		return 0;
	}
	string pkey;
	if ( token != NULL ) {
		pkey = nextpos;
	} else {
		free( opt );
		return 0;
	}
	free( opt );
	char ipAddrBuf[100];
	string pIpAddress = inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
		hostIt->setEncryptionCapacity( cap );
		hostIt->setPubKeyHex( pkey );
		hostIt->setEncryptMethodHex( meth );
	}
#endif
	return 0;
}

/**
 * 電文受信イベント：GETDIRFILES
 * ・パケットで指定されたディレクトリを送信する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::TcpRecvEventGetDirFiles( const Packet& packet )
{
	pthread_t t_id;
	Packet *packetClone = new Packet( packet );

	if ( pthread_create( &t_id, NULL, GetDirFilesThread, (void *)packetClone ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_create");
		return -1;
	}
	if ( pthread_detach( t_id ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_detach");
		return -1;
	}

	return 0;
}

/**
 * ディレクトリダウンロードスレッド
 * ・ディレクトリをダウンロードさせる。
 * @param param パケットオブジェクト(void*)
 */
void *
GetDirFilesThread( void *param )
{
	Packet *packet = (Packet *)param;
#if defined(INFO) || !defined(NDEBUG)
	printf( "TcpRecvEventGetDirFiles\n" );fflush( stdout );
#endif
	vector<SentMessage>::iterator msg = myInstance->GetSentMessages()->FindSentMessageByPacket( *packet );
	if ( msg == myInstance->GetSentMessages()->end() ){
		close( packet->TcpSocket() );
		delete packet;
		return 0;
	}
	vector<AttachFile>::iterator FoundFile = msg->FindAttachFileByPacket( *packet );
	if ( FoundFile == msg->Files().end() ){
		close( packet->TcpSocket() );
		delete packet;
		return 0;
	}

	vector<string> DownloadFileList;
	FoundFile->setIsDownloading( true );
	myInstance->SendDirData( packet->TcpSocket(), FoundFile->FileName(), FoundFile->FullPath(), DownloadFileList );
	FoundFile->setIsDownloading( false );
	FoundFile->setIsDownloaded( true );
	close( packet->TcpSocket() );
	delete packet;

	return NULL;
}

/**
 * ディレクトリ送信。
 * @param sock TCPソケット
 * @param cd 今指しているディレクトリ名
 * @param dir 親ディレクトリのフルパス
 * @param files ファイル一覧
 */
bool
IpMessengerAgentImpl::SendDirData( int sock, string cd, string dir, vector<string> &files )
{
	DIR *d= opendir( dir.c_str() );
	struct dirent *dent;
	struct stat st;
	char headBuf[8192];

	if ( d == NULL ) {
		return false;
	}

	stat( cd.c_str(), &st );
	int headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
														converter->ConvertLocalToNetwork( cd.c_str() ).c_str(),
														(unsigned long long)st.st_size,
														IPMSG_FILE_DIR,
														IPMSG_FILE_MTIME, (long)st.st_mtime,
														IPMSG_FILE_CREATETIME, (long)st.st_ctime );
	headBuf[ snprintf( headBuf, sizeof(headBuf),"%04x", headBufLen) ] = ':';
	send( sock, headBuf, headBufLen, 0 );

	dent = readdir( d );
	while( dent != NULL ) {
		if ( strcmp(dent->d_name, "." ) != 0 && strcmp(dent->d_name, ".." ) != 0 ) {
			string dir_name = dir + "/" + dent->d_name;
#if defined(INFO) || !defined(NDEBUG)
			printf( "dir[%s]", dir_name.c_str() );fflush( stdout );
#endif
			stat( dir_name.c_str(), &st );
			files.push_back( dir_name );
			if ( S_ISDIR( st.st_mode ) ){
#if defined(INFO) || !defined(NDEBUG)
				printf( "DIR\n" );fflush( stdout );
#endif
				if ( !SendDirData( sock, dent->d_name, dir_name, files ) ){
					closedir( d );
					return false;
				}
			} else {
#if defined(INFO) || !defined(NDEBUG)
				printf( "FILE\n" );fflush( stdout );
#endif
				int headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
																	converter->ConvertLocalToNetwork( dent->d_name ).c_str(),
																	(unsigned long long)st.st_size,
																	IPMSG_FILE_REGULAR,
																	IPMSG_FILE_MTIME, (long)st.st_mtime,
																	IPMSG_FILE_CREATETIME, (long)st.st_ctime );
				headBuf[ snprintf( headBuf, sizeof(headBuf),"%04x", headBufLen) ] = ':';
				send( sock, headBuf, headBufLen, 0 );

				if ( !SendFile( sock, dir_name, st.st_mtime, st.st_size, NULL , 0 ) ){
					closedir( d );
					return false;
				}
			}
		}
		dent = readdir( d );
	}
	headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:.:0:%lx:", IPMSG_FILE_RETPARENT );
	headBuf[ snprintf( headBuf, sizeof(headBuf),"%04x", headBufLen) ] = ':';
	send( sock, headBuf, headBufLen, 0 );
	closedir( d );
	return true;
}

/**
 * ファイル送信。
 * @param sock TCPソケット
 * @param FileName ファイルのフルパス
 * @param offset オフセット
 * @retval true:成功、false:失敗
 */
bool
IpMessengerAgentImpl::SendFile( int sock, string FileName, time_t mtime, unsigned long long size, AttachFile *file, off_t offset )
{
	string localFileName = converter->ConvertNetworkToLocal( FileName.c_str() );
	char readbuf[8192];
	struct stat statInit;
	int readSize;
	unsigned long long transSize = 0LL;
	int fd = open( localFileName.c_str(), O_RDONLY );

	if ( file != NULL ) file->setTransSize( offset );

	if ( fd < 0 ) {
		perror( "open" );
#ifdef DEBUG
		printf("FileName.c_str() [%s]\n", FileName.c_str() );fflush(stdout);
#endif
		return false;
	}
	int rc = fstat( fd, &statInit );
	if ( rc != 0 ){
		close( fd );
		return false;
	}
	lseek( fd, offset, SEEK_SET );
	readSize = read( fd, readbuf, sizeof( readbuf ) );
	while( readSize > 0 ){
		if ( AbortDownloadAtFileChanged() ){
			struct stat statProgress;
			if ( stat( localFileName.c_str(), &statProgress ) != 0 ){
#ifdef DEBUG
				printf("FileName.c_str() [%s]\nFile Changed.\n", FileName.c_str() );fflush(stdout);
#endif
				close( fd );
				return false;
			}
			if ( IsFileChanged( mtime, size, statInit, statProgress ) ){
#ifdef DEBUG
				printf("FileName.c_str() [%s]\nFile Changed.\n", FileName.c_str() );fflush(stdout);
#endif
				close( fd );
				return false;
			}
#ifdef DEBUG
			printf("FileName.c_str() [%s]\nFile Unchanged.\n", FileName.c_str() );fflush(stdout);
#endif
		}
		send( sock, readbuf, readSize, 0 );
		transSize += readSize;
		if ( file != NULL ) file->setTransSize( transSize );
		readSize = read( fd, readbuf, sizeof( readbuf ) );
	}
	close( fd );
	return true;
}

/**
 * ファイルが更新されたか。
 * @param mtime 更新時刻
 * @param size ファイルサイズ
 * @param statInit ファイル属性初期状態
 * @param statProgress ファイル属性現在状態
 * @retval true:更新された、false:更新されていない。
 */
bool
IpMessengerAgentImpl::IsFileChanged( time_t mtime, unsigned long long size, struct stat statInit, struct stat statProgress )
{
	return mtime             != statProgress.st_mtime ||
		   statInit.st_ctime != statProgress.st_ctime ||
		   statInit.st_uid   != statProgress.st_uid   ||
		   statInit.st_gid   != statProgress.st_gid   ||
		   size              != (unsigned long long)statProgress.st_size;
}

/**
 * メッセージ受信時、パケットメッセージ本文の末尾の'\0'以降からファイル一覧情報を生成する。
 * @param option パケットオプション部
 * @param files 添付ファイルの一覧
 */
int
IpMessengerAgentImpl::CreateAttachedFileList( const char *option, AttachFileList &files )
{
	files.clear();
	int filelist_startpos = strlen( option ) + 1;
	int alloc_size = strlen( &option[filelist_startpos] );
	if ( alloc_size == 0 ) {
		return 0;
	}
	alloc_size++;

	char *file_list_tmp_ptr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *file_list_tmp_buf = (char *)calloc( alloc_size, 1 );
	if ( file_list_tmp_buf == NULL ) {
		return 0;
	}
	memset( file_list_tmp_buf, 0, alloc_size );
	memcpy( file_list_tmp_buf,  &option[filelist_startpos] , alloc_size - 1 );
#if defined(INFO) || !defined(NDEBUG)
	printf("File List Buffer = [%s]\n", file_list_tmp_buf);fflush( stdout );
#endif

	IpMsgPrintBuf("CreateAttachedFileList:file_list_tmp_buf",  file_list_tmp_buf, alloc_size );

	// USER NAME(1st)
	file_list_tmp_ptr = file_list_tmp_buf;
	token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	IpMsgPrintBuf("CreateAttachedFileList:file_list_tmp_ptr",  file_list_tmp_ptr, alloc_size );
	IpMsgPrintBuf("CreateAttachedFileList:token",  token, alloc_size );

	while( token != NULL ) {
		bool eob = false;
		while( 1 ) {
			AttachFile file;
#if defined(DEBUG) || !defined(NDEBUG)
			printf("AttachFile(-1)\n" );fflush(stdout);
#endif
			// FILE ID
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileId( strtoul( token, &ptrdmy, 10 ) );
#if defined(DEBUG) || !defined(NDEBUG)
			printf( "file.FileId() %d token [%s]\n", file.FileId(), token );fflush(stdout);
#endif
			// FILE NAME
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileName( token );
			// FILE SIZE
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileSize( strtoul( token, &ptrdmy, 16 ) );
			// MTIME
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setMTime( strtoul( token, &ptrdmy, 16 ) );
			// ATTR
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setAttr( strtoul( token, &ptrdmy, 16 ) );
			while( token != NULL && *token != '\a' ) {
				file_list_tmp_ptr = nextpos;
				token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
				if ( token != NULL && *token == '\a' ) eob = true;
				if ( token == NULL || *token == '\a' ) break;
				int pos = -1;
				for( int i = 0; token[i] != '\0'; i++ ){ 
					if ( token[i] == '=' ) {
						token[i] = '\0';
						pos = i + 1;
						break;
					}
				}
				if ( pos >= 0 ) {
					ptrdmy = &token[pos];
					char *topchar = ptrdmy;
					while( *ptrdmy != '\0' ) {
						file.addExtAttrs( token, strtoul( topchar, &ptrdmy, 16 ) );
						topchar = ++ptrdmy;
					}
				}
			}
#if defined(DEBUG) || !defined(NDEBUG)
			printf("\n\n");fflush(stdout);
			printf("== FILE  ==============================>\n");fflush( stdout );
			printf("FILE ID[%d]\n", file.FileId());fflush( stdout );
			printf("FILE NAME[%s]\n", file.FileName().c_str());fflush( stdout );
			printf("FILE SIZE[%lld]\n", file.FileSize());fflush( stdout );
			time_t tt = file.MTime();
			printf("MTIME[%s]\n", ctime( &tt ) );fflush( stdout );
			printf("ATTR[%lu]\n", file.Attr() );fflush( stdout );
			for( map<string, vector<unsigned long> >::iterator ixextattr = file.beginExtAttrs(); ixextattr != file.endExtAttrs(); ixextattr++){
				printf("EXT ATTR[%s]==", ixextattr->first.c_str() );fflush( stdout );
				for( vector<unsigned long>::iterator ixextattrv = ixextattr->second.begin(); ixextattrv != ixextattr->second.end(); ixextattrv++){
					printf("[%lu]", *ixextattrv );fflush( stdout );
				}
				printf("\n" );fflush( stdout );
			}
			printf("<= FILE  ===============================\n");fflush( stdout );
#endif
			// ADD FILELIST
#if defined(DEBUG) || !defined(NDEBUG)
			printf("AddFile()\n" );fflush( stdout );
#endif
			files.AddFile( file );
			break;
		}
		// FILE ID(not 1st)
		if ( token == NULL ){
#if defined(DEBUG) || !defined(NDEBUG)
			printf("File END,break;\n" );fflush( stdout );
#endif
			break;
		}
		if ( *token == '\a' ){
			token++;
		} else {
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
		}
	}
	free( file_list_tmp_buf );
	return files.size();
}

/**
 * ホストリスト受信時、パケットオプション部（バッファ）からホスト一覧情報を生成する。
 * @param hostListBuf バッファ
 * @param buf_len バッファの長さ
 */
int
IpMessengerAgentImpl::CreateHostList( const char *packetIpAddress, const char *packetHostName, const char *hostListBuf, int buf_len )
{
	int alloc_size = buf_len + 1;
	int add_count = 0;
	char *hostListTmpPtr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *hostListTmpBuf = (char *)calloc( alloc_size, 1 );

#if defined(DEBUG) || !defined(NDEBUG)
IpMsgPrintBuf( "hostListBuf", hostListBuf, buf_len );
#endif
	AddDefaultHost();
	if ( hostListTmpBuf == NULL ) {
		return 0;
	}
	memset( hostListTmpBuf, 0, alloc_size );
	memcpy( hostListTmpBuf, hostListBuf, buf_len );
	hostListTmpPtr = hostListTmpBuf;
	// CONTINUE POSITION
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );
	if ( token == NULL ) {
		free( hostListTmpBuf );
		return 0;
	}
	// LIST COUNTS
	hostListTmpPtr = nextpos;
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );
	if ( token == NULL ) {
		free( hostListTmpBuf );
		return 0;
	}
	// USER NAME(1st)
	hostListTmpPtr = nextpos;
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );

	while( token != NULL ) {
		HostListItem item;
		item.setVersion( "" );
		item.setAbsenceDescription( "" );
		item.setUserName( "" );
		item.setHostName( "" );
		item.setCommandNo( 0UL );
		item.setIpAddress( "" );
		item.setNickname( "" );
		item.setGroupName( "" );
		item.setEncodingName( "" );
		item.setPriority( "" );
		item.setPortNo( 0UL );
		item.setEncryptionCapacity( 0UL );
		item.setPubKeyHex( "" );
		item.setEncryptMethodHex( "" );
		// USER NAME
		if ( *token == '\b' ) {
			item.setUserName( "" );
			//'\b'と区切り文字'\a'の分を飛ばす。
			token += 2;
			nextpos = token;
		} else {
			item.setUserName( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// HOST NAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setHostName( "" );
			//'\b'と区切り文字'\a'の分を飛ばす。
			token += 2;
			nextpos = token;
		} else {
			item.setHostName( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// CommandNo
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setCommandNo( 0L );
			//'\b'と区切り文字'\a'の分を飛ばす。
			token += 2;
			nextpos = token;
		} else {
			item.setCommandNo( strtoul( token, &ptrdmy, 10 ) );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// IP ADDRESS
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setIpAddress( "" );
			//'\b'と区切り文字'\a'の分を飛ばす。
			token += 2;
			nextpos = token;
		} else {
			//ANSLISTで送られてくるホストリストのIPアドレスがループバックの場合が有る。（IPメッセンジャーのバグなのかな？）
			if ( strcmp( token, "127.0.0.1" ) == 0 ){
				//パケットを送信したホストのIPアドレスがループバックの場合はパケット送付元のIPアドレスを設定する。
				if ( item.HostName() == packetHostName ) {
					item.setIpAddress( packetIpAddress );
				} else {
					//そうでない場合はあきらめる。（AddHostメソッド内で無視されホストリストに追加されない。）
					item.setIpAddress( token );
				}
			} else {
				//ローカルループバックアドレスでは無い場合はそのまま設定する。
				item.setIpAddress( token );
			}
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// PORTNO
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setPortNo( 0L );
			//'\b'と区切り文字'\a'の分を飛ばす。
			token += 2;
			nextpos = token;
		} else {
			item.setPortNo( ntohs( strtoul( token, &ptrdmy, 10 ) ) );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// NICKNAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setNickname( "" );
			//'\b'と区切り文字'\a'の分を飛ばす。
			token += 2;
			nextpos = token;
		} else {
			item.setNickname( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// GROUPNAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setGroupName( "" );
			//'\b'と区切り文字'\a'の分を飛ばす。
			token += 2;
			nextpos = token;
		} else {
			item.setGroupName( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		//最後のトークンは最後に判定する。(A)部分。
		// ADD HOSTLIST
		hostList.DeleteHost( item.HostName() );
		hostList.AddHost( item );

#ifdef HAVE_OPENSSL
		char sendBuf[MAX_UDPBUF];
		int sendBufLen;
		char optBuf[MAX_UDPBUF];
		int optBufLen;
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx", encryptionCapacity );
		sendBufLen = CreateNewPacketBuffer( IPMSG_GETPUBKEY,
											  _LoginName, _HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( item.PortNo() );
		addr.sin_addr.s_addr = inet_addr( item.IpAddress().c_str() );
		SendPacket( IPMSG_GETPUBKEY, sendBuf, sendBufLen, addr );
#endif
		//(A)最後のトークンは最後に判定する。(A)
		if ( token == NULL ) break;
		add_count++;
	}
	free( hostListTmpBuf );
	return add_count;
}

/**
 * 受信メッセージの個数を取得する。
 * @retval 受信メッセージの個数
 */
int
IpMessengerAgentImpl::GetRecievedMessageCount()
{
	return recvMsgList.size();
}

/**
 * 受信メッセージを一個取り出し、受信メッセージリストから削除する。
 * @retval 受信メッセージオブジェクト。
 */
RecievedMessage
IpMessengerAgentImpl::PopRecievedMessage()
{
	RecievedMessage ret;
	for( vector<RecievedMessage>::iterator ix = recvMsgList.begin(); ix != recvMsgList.end(); ix++ ){
		ret = *ix;
		recvMsgList.erase( ix );
		break;
	}
	return ret;
}

/**
 * 送信済メッセージリストのポインタを取得する。
 * @retval 送信済メッセージリストのポインタ。
 */
SentMessageList *
IpMessengerAgentImpl::GetSentMessages()
{
	return &sentMsgList;
}

/**
 * 送信済メッセージリストのコピーを取得する。
 * @retval 送信済メッセージリストのコピー。
 */
SentMessageList
IpMessengerAgentImpl::CloneSentMessages()
{
	return sentMsgList;
}

/**
 * オプション部の最大長を取得する。
 * @retval オプション部が許容するバッファの長さ
 */
int
IpMessengerAgentImpl::GetMaxOptionBufferSize()
{
	char tmp[MAX_UDPBUF];
	int headSize = snprintf(tmp, sizeof(tmp), "%d:0000000000:%s:%s:0000000000:", IPMSG_VERSION, _LoginName.c_str(), _HostName.c_str() );
	int ret = MAX_UDPBUF - headSize;
	return ret < 0 ? 0 : ret;
}

/**
 * 共通コマンドオプションを追加する。
 * @param 追加前のコマンドオプション
 * @retval 追加後のコマンドオプション
 */
unsigned long
IpMessengerAgentImpl::AddCommonCommandOption( const unsigned long cmd )
{
	unsigned long ret = cmd | IPMSG_FILEATTACHOPT
#ifdef HAVE_OPENSSL
							| ( encryptionCapacity != 0UL ?  IPMSG_ENCRYPTOPT : 0UL )
#endif	//HAVE_OPENSSL
							| ( IsAbsence() ? IPMSG_ABSENCEOPT : 0UL ) | ( IsDialup() ? IPMSG_DIALUPOPT : 0UL );
#if defined(INFO) || !defined(NDEBUG)
	printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<AddCommonCommandOption<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");fflush( stdout );
	printf( "Option=%lu\n", ret );fflush( stdout );
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>AddCommonCommandOption>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");fflush( stdout );
#endif
	return ret;
}

/**
 * 送信用バッファを作成する。
 * @param cmd コマンド
 * @param packetNo パケット番号
 * @param user このホストのユーザ名
 * @param host このホストのホスト名
 * @param opt 連結するオプション文字列
 * @param optLen オプションの長さ
 * @param buf 送信バッファ
 * @param size 送信バッファの最大サイズ
 * @retval 送信バッファの長さ
 */
int
IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, unsigned long packetNo, string user, string host, const char *opt, int optLen, char *buf, int size )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "CMD[%s]\n", GetCommandString( GET_MODE( cmd ) ).c_str() );fflush( stdout );
#endif
	memset( buf, 0, size );
	//Version:PacketNo:UserName:HostName:Command[:Option]
	int send_size = snprintf(buf, size, "%d:%ld:%s:%s:%ld:",
										IPMSG_VERSION,
										packetNo,
										user == "" ? "\b" : user.c_str(),
										host == "" ? "\b" : host.c_str(),
										cmd );
	if ( optLen > 0 && opt != NULL) {
		memcpy(&buf[send_size], opt, optLen );
	} else {
		optLen = 0;
	}
	return send_size + optLen;
}

/**
 * 送信用バッファを作成する。(パケット番号自動生成版)
 * @param cmd コマンド
 * @param user このホストのユーザ名
 * @param host このホストのホスト名
 * @param opt 連結するオプション文字列
 * @param optLen オプションの長さ
 * @param buf 送信バッファ
 * @param size 送信バッファの最大サイズ
 * @retval 送信バッファの長さ
 */
int
IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, string user, string host, const char *opt, int optLen, char *buf, int size )
{
	unsigned long packetNo = random();
	return CreateNewPacketBuffer(cmd, packetNo, user, host, opt, optLen, buf, size );
}

/**
 * 受信バッファからパケットオブジェクトを生成する。
 * @param packet_buf 受信バッファ
 * @param size 受信バッファのサイズ
 * @param sender 送信元アドレス
 * @retval パケットオブジェクト
 */
Packet
IpMessengerAgentImpl::DismantlePacketBuffer( char *packet_buf, int size, struct sockaddr_in sender, time_t nowTime )
{
	Packet ret;
	int alloc_size = size + 1;
	char *packet_tmp_buf;
	char *packet_tmp_ptr;
	char *nextpos;
	char *token;
	char *ptrdmy;

	ret.setRecieved( nowTime );
	packet_tmp_buf = (char *)calloc( alloc_size, 1 );
	if ( packet_tmp_buf == NULL ) {
		return ret;
	}
	memset( packet_tmp_buf, 0, alloc_size );
	memcpy( packet_tmp_buf, packet_buf, size );
	//VERSION NUMBER
	packet_tmp_ptr = packet_tmp_buf;
	token = strtok_r( packet_tmp_buf, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		return ret;
	}
	ret.setVersionNo( strtoul( token, &ptrdmy, 10 ) );

	//PACKET NUMBER
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		return ret;
	}
	ret.setPacketNo( strtoul( token, &ptrdmy, 10 ) );

	//USER
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		return ret;
	}
	ret.setUserName( token );

	//HOST
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		return ret;
	}
	ret.setHostName( token );

	//COMMAND
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		return ret;
	}
	unsigned long command = strtoul( token, &ptrdmy, 10 ); 
	ret.setCommandMode( GET_MODE(command) );
	ret.setCommandOption( GET_OPT(command) );

	//OPTION
	int optLen = size - ( nextpos - packet_tmp_buf );
	ret.setOption( string( nextpos, optLen ) );
	free( packet_tmp_buf );

	vector<HostListItem>::iterator hostIt = hostList.FindHostByHostName( ret.HostName() );
	if ( hostIt != hostList.end() ) {
		struct sockaddr_in hostaddr;
		hostaddr.sin_family = AF_INET;
		hostaddr.sin_addr.s_addr = inet_addr( hostIt->IpAddress().c_str() );
		hostaddr.sin_port = htons( hostIt->PortNo() );
		ret.setAddr( hostaddr );
	} else {
		sender.sin_port = htons( IPMSG_DEFAULT_PORT );
		ret.setAddr( sender );
	}

	return ret;
}

/**
 * パケットからホストリストに加える。
 * @param packet パケットオブジェクト
 */
void
IpMessengerAgentImpl::AddHostListFromPacket( const Packet& packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("===================================\n");fflush( stdout );
	printf("AddHostListFromPacket\n");fflush( stdout );
	printf("===================================\n");fflush( stdout );
	IpMsgDumpPacket( packet, packet.Addr() );
	printf("===================================\n");fflush( stdout );
#endif
	AddDefaultHost();
	// デフォルトのNIC(０番目)以外の自分自身のIPアドレスが登録依頼されたら無視。
	char ipAddrBuf[100];
	string packetIpAddress = inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	for( unsigned int i = 1; i < NICs.size(); i++ ){
		if ( packetIpAddress == NICs[i].IpAddress() ){
			AddDefaultHost();
			return;
		}
	}
	//デフォルトカード
	HostListItem item;
	item.setUserName( packet.UserName() );
	item.setHostName( packet.HostName() );
	item.setCommandNo( packet.CommandOption() );
	item.setIpAddress( inet_ntoa_r( packet.Addr().sin_addr.s_addr, ipAddrBuf, sizeof( ipAddrBuf ) ) );
	int NicknameLen = strlen( packet.Option().c_str() );
	item.setNickname( packet.Option().c_str() );
	item.setGroupName( packet.Option().c_str() + NicknameLen + 1 );
	item.setEncodingName( "" );
	item.setPriority( "" );
	item.setPortNo( ntohs( packet.Addr().sin_port ) );
	item.setEncryptionCapacity( 0UL );
	item.setPubKeyHex( "" );
	item.setEncryptMethodHex( "" );
	hostList.AddHost( item );
}

/**
 * 念のためホストリストに自分を加えておく。
 * @retval 登録したホスト数。
 */
int
IpMessengerAgentImpl::AddDefaultHost()
{
	vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( HostAddress );
	if ( hostIt == hostList.end() ) {
		HostListItem myHost;
		myHost.setUserName( _LoginName );
		myHost.setHostName( _HostName );
		myHost.setCommandNo( AddCommonCommandOption( 0UL ) );
		myHost.setIpAddress( HostAddress );
		myHost.setNickname( Nickname );
		myHost.setGroupName( GroupName );
		myHost.setPortNo( IPMSG_DEFAULT_PORT );
		hostList.AddHost( myHost );
#if defined(INFO) || !defined(NDEBUG)
		printf("MyHost Add.[%s][%s]\n", myHost.UserName().c_str(), myHost.GroupName().c_str() );fflush( stdout );
#endif
		return 1;
	}
	return 0;
}
//end of source
