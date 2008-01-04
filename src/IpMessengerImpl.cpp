/**
 * IP メッセンジャライブラリ(Unix用)<br/>
 * IPメッセンジャエージェントクラス。
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
#include <pthread.h>
using namespace ipmsg;

static IpMessengerAgentImpl *myInstance = NULL;

static pthread_mutex_t instanceMutex;
static int mutex_init_result = IpMsgMutexInit( "IpMessengerImpl::Global", &instanceMutex, NULL );

/**
 * IP メッセンジャイベントクラスのデフォルト実装。
 */
class IpMessengerNullEvent: public IpMessengerEvent {
	public:
		/**
		 * 通知イベント開始前イベント(GUIスレッドのロック等を実装してください)
		 */
		virtual void EventBefore(){};
		/**
		 * 通知イベント終了後イベント(GUIスレッドのアンロック等を実装してください)
		 */
		virtual void EventAfter(){};
		/**
		 * ホストリストリフレッシュ後イベント
		 * @param hostList ホストリスト
		 */
		virtual void RefreshHostListAfter( HostList& hostList ){ printf("UpdateHostListAfter\n"); };
		/**
		 * ホストリスト更新後イベント。イベントが発生したことを示すためprintを行う。
		 * @param hostList ホストリスト
		 */
		virtual void UpdateHostListAfter( HostList& hostList ){ printf("UpdateHostListAfter\n"); };
		/**
		 * ホストリスト取得リトライエラーイベント。イベントが発生したことを示すためprintを行う。
		 * @retval true:リトライする
		 * @retval false:リトライしない
		 */
		virtual bool GetHostListRetryError(){ printf("GetHostListRetryError\n");return false; };
		/**
		 * メッセージ受信後イベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 受信メッセージ
		 * @retval true:処理してメッセージの保存が不要
		 * @retval false:メッセージを保存
		 */
		virtual bool RecieveAfter( RecievedMessage& msg ){ printf("RecieveAfter\n");return false; };
		/**
		 * メッセージ送信後イベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 送信メッセージ
		 */
		virtual void SendAfter( SentMessage& msg ){ printf("SendAfter\n"); };
		/**
		 * メッセージ送信リトライエラーイベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 送信メッセージ
		 * @retval true:リトライする
		 * @retval false:リトライしない
		 */
		virtual bool SendRetryError( SentMessage& msg ){ printf("SendRetryError\n");return false; };
		/**
		 * メッセージ暗号化失敗通知イベント。
		 */
		virtual void NotifySendEncryptionFail( HostListItem& host ){ printf("NotifySendEncryptionFail\n"); };
		/**
		 * メッセージ暗号化失敗イベント。
		 * @retval true:暗号化せずに送信する
		 * @retval false:失敗させる
		 */
		virtual bool IsSendContinueOnEncryptionFail( HostListItem& host ){ printf("IsSendContinueOnEncryptionFail\n"); return false; };
		/**
		 * 開封通知後イベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 送信メッセージ
		 */
		virtual void OpenAfter( SentMessage& msg ){ printf("OpenAfter\n"); };
		/**
		 * ダウンロード開始イベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 */
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadStart\n"); };
		/**
		 * ダウンロード処理中イベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 */
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadProcessing\n"); };
		/**
		 * ダウンロード終了イベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 */
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadEnd\n"); };
		/**
		 * ダウンロードエラーイベント。イベントが発生したことを示すためprintを行う。
		 * @param msg 受信メッセージ
		 * @param file 添付ファイル
		 * @param info ダウンロード情報
		 * @param data DownloadFile、DownloadDirで指定した任意データへのポインタ
		 * @retval true:リトライする
		 * @retval false:リトライしない
		 */
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadError\n"); return false; };
		/**
		 * ホストの参加通知後イベント。イベントが発生したことを示すためprintを行う。
		 * @param hostList ホスト
		 */
		virtual void EntryAfter( HostListItem& host ){ printf("EntryAfter\n"); };
		/**
		 * ホストの脱退通知後イベント。イベントが発生したことを示すためprintを行う。
		 * @param hostList ホスト
		 */
		virtual void ExitAfter( HostListItem& host ){ printf("ExitAfter\n"); };
		/**
		 * 不在モード更新後イベント。イベントが発生したことを示すためprintを行う。
		 * @param hostList ホスト
		 */
		virtual void AbsenceModeChangeAfter( HostListItem& host ){ printf("AbsenceModeChangeAfter\n"); };
		/**
		 * バージョン情報受信後イベント。イベントが発生したことを示すためprintを行う。
		 * @param host ホスト
		 * @param version バージョン
		 */
		virtual void VersionInfoRecieveAfter( HostListItem &host, std::string version ){ printf("VersionInfoRecieveAfter\n"); };
		/**
		 * 不在詳細情報受信後イベント。イベントが発生したことを示すためprintを行う。
		 * @param host ホスト
		 * @param absenceDetail 不在詳細情報
		 */
		virtual void AbsenceDetailRecieveAfter( HostListItem &host, std::string absenceDetail ){ printf("AbsenceDetailRecieveAfter\n"); };
};

/**
 * IP メッセンジャエージェントクラスのインスタンスを取得する。
 * <ul>
 * <li>Singletonパターンを採用しているので、ホスト唯一のインスタンスでなければならない。</li>
 * </ul>
 */
IpMessengerAgentImpl *
IpMessengerAgentImpl::GetInstance()
{
	IPMSG_FUNC_ENTER("IpMessengerAgentImpl *IpMessengerAgentImpl::GetInstance()");
	mutex_init_result = 0; //fix warnings, but no effect.
	IpMsgMutexLock( "IpMessengerAgentImpl::GetInstance()", &instanceMutex );
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgentImpl();
	}
	IpMsgMutexUnlock( "IpMessengerAgentImpl::GetInstance()", &instanceMutex );
	IPMSG_FUNC_RETURN( myInstance );
}

/**
 * IP メッセンジャエージェントクラスのインスタンスを解放する。
 * <ul>
 * <li>このメソッドを使ってオブジェクトを解放しなければならない。</li>
 * <li>ライブラリを通じないで直接deleteされた場合はその後の動作について関知しない。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::Release()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::Release()");
	IpMsgMutexLock( "IpMessengerAgentImpl::Release()", &instanceMutex );
	if ( myInstance == NULL ) {
		IpMsgMutexUnlock( "IpMessengerAgentImpl::Release()", &instanceMutex );
		IPMSG_FUNC_EXIT;
	}
	delete myInstance;
	myInstance = NULL;
	IpMsgMutexUnlock( "IpMessengerAgentImpl::Release()", &instanceMutex );
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのコンストラクタ。<br/>
 * <ul>
 * <li>暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の生成を行う。</li>
 * <li>パケットNoに使用する乱数シードを時刻で初期化する。</li>
 * <li>ファイル名コンバータを初期セットアップする。（変換を行わないNullConverterがデフォルト）</li>
 * <li>ネットワークの初期化。
 * </ul>
 */
IpMessengerAgentImpl::IpMessengerAgentImpl()
		:_DefaultPortNo( IPMSG_DEFAULT_PORT )
{
	IPMSG_FUNC_ENTER("IpMessengerAgentImpl::IpMessengerAgentImpl()");
	CryptoInit();
	srandom( time( NULL ) );
	converter = new NullFileNameConverter();
	compare = new HostListDefaultComparator();
	setAbortDownloadAtFileChanged( false );
	setSaveSentMessage( true );
	setSaveRecievedMessage( true );
	event = new IpMessengerNullEvent();
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのデストラクタ。
 * <ul>
 * <li>まず、ログアウト。</li>
 * <li>暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の破棄を行う。</li>
 * <li>割り当て済のファイル名コンバータを削除する。</li>
 * <li>ソケットのクローズ。
 * </ul>
 */
IpMessengerAgentImpl::~IpMessengerAgentImpl()
{
	IPMSG_FUNC_ENTER("IpMessengerAgentImpl::~IpMessengerAgentImpl()");
	if ( IsNetworkStarted() ){
		Logout();
		StopNetwork();
	}
	CryptoEnd();
	delete converter;
	delete compare;
	delete event;
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを起動する。
 * <ul>
 * <li>NICを指定しないでネットワーク開始(全てのNIC、デフォルトポートで起動)。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::StartNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::StartNetwork()");
	std::vector<NetworkInterface> nics;
	StartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを起動する。
 * <ul>
 * <li>ネットワーク初期化。</li>
 * <li>ネットワークにごみとしてホスト情報が残っていることを考慮して一旦ログアウト。</li>
 * <li>開始後ログインする必要があります。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::StartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::StartNetwork( const std::vector<NetworkInterface>& nics )");
	NICs.clear();
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( NICs, UseIPv6(), DefaultPortNo() );
	NetworkInit( nics );
	Logout();
	// TODO 受信スレッド開始
	pthread_t t_id;

//	printf( "Thread create\n" );fflush(stdout);
	if ( pthread_create( &t_id, NULL, ProcessPacketThread, NULL ) != 0 ){
		perror("StartNetwork:pthread_create");
		IPMSG_FUNC_EXIT;
	}
	_IsNetworkStarted = true;
//	printf( "Thread detach\n" );fflush(stdout);
	if ( pthread_detach( t_id ) != 0 ){
		perror("StartNetwork:pthread_detach");
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

void *
ipmsg::ProcessPacketThread( void *param )
{
#ifdef DEBUG
	long p = 0;
#endif
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	while( agent->IsNetworkStarted() ) {
#ifdef DEBUG
		printf( "ProcessPacketThread(p=%ld)\n", ++p );fflush(stdout);
#endif
		agent->Process();
		if ( usleep( 500000L ) != 0 ) {
			printf( "usleep fail\n" );fflush(stdout);
		}
	}
#ifdef DEBUG
	printf( "ProcessPacketThread END.\n" );fflush(stdout);
#endif
	return NULL;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを終了する。
 * <ul>
 * <li>ネットワーク終期化。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::StopNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::StopNetwork()");
	// TODO 受信スレッド終了。＆待ち合わせ。
	_IsNetworkStarted = false;
	usleep( 1000000L );
	NetworkEnd();
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * <ul>
 * <li>NICを指定しないでネットワーク再起動(全てのNIC、デフォルトポート再起動)。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::RestartNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::RestartNetwork()");
	std::vector<NetworkInterface> nics;
	RestartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * <ul>
 * <li>まず、ログアウト。</li>
 * <li>ネットワーク終期化。</li>
 * <li>ネットワーク初期化。</li>
 * <li>再度ログイン。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::RestartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::RestartNetwork( const std::vector<NetworkInterface>& nics )");
	if ( IsNetworkStarted() ) {
		Logout();
		StopNetwork();
	}
	StartNetwork( nics );
	Login( Nickname, GroupName );
	IPMSG_FUNC_EXIT;
}

/**
 * ファイル名コンバータのゲッター。<br>
 * @retval コンバータのアドレス。
 */
FileNameConverter *
IpMessengerAgentImpl::GetFileNameConverter() const
{
	IPMSG_FUNC_ENTER("FileNameConverter *IpMessengerAgentImpl::GetFileNameConverter() const");
	IPMSG_FUNC_RETURN( converter );
}

/**
 * ファイル名コンバータのセッター。<br>
 * <ul>
 * <li>割り当て済のファイル名コンバータを削除する。</li>
 * <li>新しいコンバータの割り当て。</li>
 * </ul>
 * @param conv コンバータのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgentImpl::SetFileNameConverter( const FileNameConverter *conv )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetFileNameConverter( const FileNameConverter *conv )");
	if ( conv == NULL ){
		IPMSG_FUNC_EXIT;
	}
	//自己代入を考慮
	if ( conv == converter ){
		IPMSG_FUNC_EXIT;
	}
	delete converter;
	converter = const_cast<FileNameConverter *>( conv );
	IPMSG_FUNC_EXIT;
}

/**
 * ホストリスト比較オブジェクトのゲッター。
 * @retval ホストリスト比較オブジェクトのアドレス。
 */
HostListComparator *
IpMessengerAgentImpl::GetSortHostListComparator() const
{
	IPMSG_FUNC_ENTER("HostListComparator *IpMessengerAgentImpl::GetSortHostListComparator() const");
	IPMSG_FUNC_RETURN( compare );
}; 

/**
 * ホストリスト比較オブジェクトのセッター。
 * <ul>
 * <li>割り当て済のホストリスト比較オブジェクトを削除する。</li>
 * <li>新しいホストリスト比較オブジェクトの割り当て。</li>
 * </ul>
 * @param comparator ホストリスト比較オブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgentImpl::SetSortHostListComparator( const HostListComparator *comparator )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetSortHostListComparator( const HostListComparator *comparator )");
	if ( comparator == NULL ){
		IPMSG_FUNC_EXIT;
	}
	//自己代入を考慮
	if ( comparator == compare ){
		IPMSG_FUNC_EXIT;
	}
	delete compare;
	compare = const_cast<HostListComparator *>( comparator );
	IPMSG_FUNC_EXIT;
}

/**
 * イベントオブジェクトのゲッター。
 * @retval イベントオブジェクトのアドレス。
 */
IpMessengerEvent *
IpMessengerAgentImpl::GetEventObject() const
{
	IPMSG_FUNC_ENTER("IpMessengerEvent *IpMessengerAgentImpl::GetEventObject() const");
	IPMSG_FUNC_RETURN( event );
}; 

/**
 * イベントオブジェクトのセッター。
 * <ul>
 * <li>割り当て済のイベントオブジェクトを削除する。</li>
 * <li>新しいイベントオブジェクトの割り当て。</li>
 * </ul>
 * @param evt イベントオブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgentImpl::SetEventObject( const IpMessengerEvent *evt )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetEventObject( const IpMessengerEvent *evt )");
	if ( evt == NULL ){
		IPMSG_FUNC_EXIT;
	}
	//自己代入を考慮
	if ( evt == event ){
		IPMSG_FUNC_EXIT;
	}
	delete event;
	event = const_cast<IpMessengerEvent *>( evt );
	IPMSG_FUNC_EXIT;
}

/**
 * NICの情報を取得する。
 * <ul>
 * <li>使用するネットワークインターフェイスのIPアドレスを求める。（ローカルループバックをのぞく全てのNIC）</li>
 * </ul>
 * @param nics ネットワークインターフェースの一覧
 */
void
IpMessengerAgentImpl::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )");
#ifdef DEBUG
printf("IpMessengerAgentImpl::GetNetworkInterfaceInfo useIPv6=%s\n", useIPv6 ? "true" : "false" );fflush(stdout);
#endif
	ipmsg::getNetworkInterfaceInfo( nics, useIPv6, defaultPortNo );
	IPMSG_FUNC_EXIT;
}

/**
 * ネットワーク関連の初期化。
 * <ul>
 * <li>環境変数からホスト名を取得。（出来なければlocalhost固定）</li>
 * <li>環境変数からユーザ名を取得。（出来なければuid）</li>
 * </ul>
 */
void
IpMessengerAgentImpl::NetworkInit( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::NetworkInit( const std::vector<NetworkInterface>& nics )");
	haveIPv4Nic = false;
	haveIPv6Nic = false;

	if ( nics.size() > 0 ){
		for( unsigned int i = 0; i < nics.size(); i++ ){
#ifdef ENABLE_IPV4
			if ( nics[i].AddressFamily() == AF_INET ) {
				haveIPv4Nic = true;
			}
#endif
#ifdef ENABLE_IPV6
		 	if ( nics[i].AddressFamily() == AF_INET6 ) {
				haveIPv6Nic = true;
			}
#endif
		}
	} else {
		for( unsigned int i = 0; i < NICs.size(); i++ ){
#ifdef ENABLE_IPV4
			if ( NICs[i].AddressFamily() == AF_INET ) {
				haveIPv4Nic = true;
			}
#endif
#ifdef ENABLE_IPV6
		 	if ( NICs[i].AddressFamily() == AF_INET6 ) {
				haveIPv6Nic = true;
			}
#endif
		}
	}

	_HostName = IpMsgGetHostName();
	if ( _HostName == "" ) {
		_HostName = "localhost";
	}

	uid_t uid = getuid();
	_LoginName = IpMsgGetLoginName( uid );
	if ( _LoginName == "" ){
		char buf[100];
		IpMsgIntToString( buf, sizeof( buf ), uid );
		_LoginName = buf;
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
		if ( NICs.size() > 0 ) {
			InitSend( NICs);
			InitRecv( NICs );
		}
	}
	printf( "%s network service started.\n", IPMSG_AGENT_VERSION );
	fflush( stdout );
	IPMSG_FUNC_EXIT;
}

/**
 * ネットワーク関連の初期化。
 * <ul>
 * <li>全てのソケットを閉じる。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::NetworkEnd()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::NetworkEnd()");
	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		close(udp_sd[i]);
	}
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		close(tcp_sd[i]);
	}
	udp_sd.clear();
	tcp_sd.clear();
	sd_addr.clear();
	sd_address_family.clear();
	IPMSG_FUNC_EXIT;
}

/**
 * ログイン（サービス参加通知）。
 * <ul>
 * <li>NOOPERATIONパケットを送信しネットワークが使用可能かどうかを確認した上でホストリストを取得。</li>
 * <li>BR_ENTRYをブロードキャスト。</li>
 * <li>パケットを受信した上で、ホストリストを再度取得。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::Login( std::string nickname, std::string groupName )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::Login( std::string nickname, std::string groupName )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	SendNoOperation();

	_IsAbsence = false;
	Logout();
#if defined(DEBUG) || !defined(NDEBUG)
	memset( sendBuf, 0, MAX_UDPBUF );
#endif
	if ( nickname != "" ) {
		Nickname = nickname;
	} else {
		Nickname = _LoginName;
	}
	GroupName = groupName;
	std::string optBuf = Nickname + '\0' + GroupName +'\0';
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ENTRY ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ENTRY, sendBuf, sendBufLen );
	ResetAbsence();

//	skulkHostList.Lock("IpMessengerAfentImpl::Login");
	std::vector<HostListItem>::iterator hi = skulkHostList.begin();
	for( ; hi != skulkHostList.end(); hi++ ) {
		struct sockaddr_storage addr;
		if ( createSockAddrIn( &addr, hi->IpAddress(), hi->PortNo() ) != NULL ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::Login HideFromAddr\n");fflush( stdout );
#endif
			HideFromAddr( addr );
		}
	}
//	skulkHostList.Unlock("IpMessengerAfentImpl::Login");
//	RecvPacket( false );
	//0.05秒まつ。
	usleep( 50000L );
//	RecvPacket( false );
	//0.1秒まつ。
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * ログアウト（サービス脱退通知）。
 * <ul>
 * <li>BR_EXITをブロードキャスト。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::Logout()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::Logout()");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_EXIT ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_EXIT, sendBuf, sendBufLen );
//	RecvPacket( false );
	//0.1秒まつ。
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * ログイン（特定のホスト向けサービス参加通知）。
 * <ul>
 * <li>NOOPERATIONパケットを送信しネットワークが使用可能かどうかを確認した上でホストリストを取得。</li>
 * <li>BR_ENTRYを特定のホストに送る。</li>
 * <li>パケットを受信した上で、ホストリストを再度取得。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::VisibleToAddr( struct sockaddr_storage &addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::VisibleToAddr( struct sockaddr_storage &addr )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	if ( !IsNetworkStarted() ) {
		IPMSG_FUNC_EXIT;
	}

#if defined(DEBUG) || !defined(NDEBUG)
	memset( sendBuf, 0, MAX_UDPBUF );
#endif
	std::string optBuf = Nickname + '\0' + GroupName +'\0';
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ENTRY ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_BR_ENTRY, sendBuf, sendBufLen, addr );
	//0.1秒まつ。
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * ログアウト（特定のホスト向けに隠れるためのサービス脱退通知）。
 * <ul>
 * <li>BR_EXITを特定のホストに送る。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::HideFromAddr( struct sockaddr_storage &addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::HideFromAddr( struct sockaddr_storage &addr )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	if ( !IsNetworkStarted() ) {
		IPMSG_FUNC_EXIT;
	}

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_EXIT ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_BR_EXIT, sendBuf, sendBufLen, addr );
	//0.1秒まつ。
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * ホストリスト取得。
 * @retval エージェントが保持しているHostListオブジェクト
 */
HostList&
IpMessengerAgentImpl::GetHostList()
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgentImpl::GetHostList()");
	IPMSG_FUNC_RETURN( appearanceHostList );
}

/**
 * ホストリスト更新取得。<br>
 * <ul>
 * <li>BR_ISGETLIST2をブロードキャスト。</li>
 * <li>他のメソッド（ANSLIST受信）にて取得するまで待機。（五回まで）</li>
 * </ul>
 * <ul>
 * <li>ホストリストの構築はANSLIST受信時に行われるので、このメソッドではひたすら待機。</li>
 * <li>ホストリストはANSLIST受信時に追加／更新されることがあるので常に同じホストリストを返すとは限らない。</li>
 * </ul>
 * @retval 取得したHostListオブジェクト
 */
HostList&
IpMessengerAgentImpl::UpdateHostList( bool isRetry )
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgentImpl::UpdateHostList( bool isRetry )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	//リトライ中かどうか？
	if ( !isRetry && !hostList.IsAsking() ) {
#if defined(DEBUG)
		printf("IpMessengerAgentImpl::UpdateHost HostList backup now.\n");
		fflush( stdout );
#endif
		hostListBackup = hostList;
		hostList.clear();
#if defined(DEBUG)
		printf("IpMessengerAgentImpl::UpdateHost HostList cleared.\n");
		fflush( stdout );
#endif
	}
	//問合せ中に状態を設定
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
//		int pcount = RecvPacket( false );
		//自分以外のホストが見付からないか５回リトライする間繰り返す
		for( int i = 0; i < 5; i++ ) {
			//0.01秒まつ。
			usleep( 10000L );
//			pcount = RecvPacket( false );
		}
	}

#if defined(DEBUG)
	IpMsgDumpHostList( " M Y   H O S T L I S T ( BEFORE SORT ) ", hostList );
#endif
	//ソート
	if ( compare != NULL ) {
		hostList.sort( compare );
		appearanceHostList.sort( compare );
	}
#if defined(DEBUG)
	IpMsgDumpHostList( " M Y   H O S T L I S T ( AFTER SORT ) ", hostList );
#endif
	//イベントを挙げる
	if ( event != NULL ) {
		event->EventBefore();
#if defined(DEBUG)
		printf("UpdateHostListAfter before\n");
#endif
		event->UpdateHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("UpdateHostListAfter after\n");
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( appearanceHostList );
}

/**
 * 不在モードかどうかを判定。
 * @retval 設定済の不在モードを返す。
 */
bool
IpMessengerAgentImpl::IsAbsence() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::IsAbsence() const");
	IPMSG_FUNC_RETURN( _IsAbsence );
}
/**
 * 不在モードをクリアする。
 */
void
IpMessengerAgentImpl::ResetAbsence()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ResetAbsence()");
	_IsAbsence = false;
	localEncoding = "";
	std::vector<AbsenceMode> d;
	absenceModeList = d;
	SendAbsence();
	IPMSG_FUNC_EXIT;
}

/**
 * 不在モードを設定する。
 * @param encoding ローカルエンコーディング
 * @param absenceModes AbsenceModeオブジェクトのベクタ（自動応答時に複数エンコーディング対応するため）
 */
void
IpMessengerAgentImpl::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )");
	_IsAbsence = true;

	localEncoding = encoding;
	absenceModeList = absenceModes;
	SendAbsence();
	IPMSG_FUNC_EXIT;
}

/**
 * メッセージ送信。
 * @param message 生成されたメッセージオブジェクト(Output)
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param IsNoLogging ログに記録しない（ことを推奨）
 * @param opt 送信オプション
 */
bool
IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )");
	AttachFileList files;
	IPMSG_FUNC_RETURN( SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, isLogging, opt, false, 0UL ) );
}

/**
 * メッセージ送信。
 * @param message 生成されたメッセージオブジェクト(Output)
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param file 添付ファイル
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param IsNoLogging ログに記録しない（ことを推奨）
 * @param opt 送信オプション
 */
bool
IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )");
	AttachFileList files;
	files.AddFile( file );
	IPMSG_FUNC_RETURN( SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, isLogging, opt, false, 0UL ) );
}

/**
 * メッセージ送信。
 * @param message 生成されたメッセージオブジェクト(Output)
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param files 添付ファイル群
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param IsNoLogging ログに記録しない（ことを推奨）
 * @param opt 送信オプション
 * @param isRetry 暗黙のリトライによる送信要求かを示すフラグ
 * @param PrevPacketNo リトライであれば前回のパケット番号
 */
bool
IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt, bool isRetry, unsigned long PrevPacketNo )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt, bool isRetry, unsigned long PrevPacketNo )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	size_t optBufSize = GetMaxOptionBufferSize() + 1;
	char *optBuf = (char *)calloc( optBufSize, 1 );
	if ( optBuf == NULL ) {
		perror("IpMessengerAgentImpl::SendMsg calloc()");
		exit(1);
	}
	int optBufLen = 0;
	struct sockaddr_storage addr;
	bool isEncrypted = false;
	if ( createSockAddrIn( &addr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_RETURN( false );
	}

//	RecvPacket( false );
	//0.1秒まつ。
	usleep( 100000L );

	optBufLen = optBufSize < msg.size() ? optBufSize : msg.size();
	memcpy( optBuf, msg.c_str(), optBufLen );
#ifdef HAVE_OPENSSL
	//OpenSSLサポートが有効なら、暗号化
	if ( isSecret.IsSecret() ) {
#if defined(DEBUG)
		printf( "IpMessengerAgentImpl::SendMsg Send message specified by Secret Mode.\n" );
		printf( "IpMessengerAgentImpl::SendMsg Target host's public key=[%s]\n", host.PubKeyHex().c_str() );
		printf( "IpMessengerAgentImpl::SendMsg Target host's encrypt method by hex=[%s]\n", host.EncryptMethodHex().c_str() );
#endif
		if ( EncryptMsg( host, (unsigned char*)optBuf, optBufLen, &optBufLen, optBufSize ) ) {
			isEncrypted = true;
		} else if ( NoSendMessageOnEncryptionFailed() ) {
			printf("IpMessengerAgentImpl::SendMsg Encryption failed.SendMsg was canceled.\n");
			fflush(stdout);
			if ( event != NULL && !isRetry ) {
				event->EventBefore();
#if defined(DEBUG)
				printf("NotifySendEncryotionFail before\n");
#endif
				event->NotifySendEncryptionFail( host );
#if defined(DEBUG)
				printf("NotifySendEncryotionFail after\n");
#endif
				event->EventAfter();
			}
			free( optBuf );
			IPMSG_FUNC_RETURN( false );
		} else {
			printf("IpMessengerAgentImpl::SendMsg Encryption failed.");
			fflush(stdout);
			if ( event != NULL && !isRetry ) {
				event->EventBefore();
#if defined(DEBUG)
				printf("IsSendContinueIbEncryptionFail before\n");
#endif
				if ( !event->IsSendContinueOnEncryptionFail( host ) ) {
					event->EventAfter();
#if defined(DEBUG)
					printf("IsSendContinueIbEncryptionFail after\n");
#endif
					printf("SendMsg was canceled.\n");
					fflush(stdout);
					free( optBuf );
					IPMSG_FUNC_RETURN( false );
				}
#if defined(DEBUG)
				printf("IsSendContinueIbEncryptionFail after\n");
#endif
				event->EventAfter();
			}
			printf("SendMsg try send no encryption message.\n");
			fflush(stdout);
			optBufLen = optBufSize < msg.size() ? optBufSize : msg.size();
			memcpy( optBuf, msg.c_str(), optBufLen );
		}
	} else {
		optBufLen = optBufSize < msg.size() ? optBufSize : msg.size();
		memcpy( optBuf, msg.c_str(), optBufLen );
	}
#endif	//HAVE_OPENSSL

	optBuf[optBufLen++] = '\0';
	IpMsgPrintBuf( "optBuf:", optBuf, optBufLen );

	int fileBufLen = 0;
	char fileBuf[MAX_UDPBUF];

	//ファイルを添付
	for( std::vector<AttachFile>::iterator ixfile = files.begin(); ixfile != files.end(); ixfile++ ) {
		ixfile->GetLocalFileInfo();
		std::string filename = converter->ConvertLocalToNetwork( ixfile->FileName() );
		size_t wsize = snprintf( &fileBuf[ fileBufLen ], sizeof( fileBuf ) - fileBufLen - 1,
							"%d:%s:%llx:%lx:%lx:\a",
							ixfile->FileId(), filename.c_str(), ixfile->FileSize(), (unsigned long)ixfile->MTime(), ixfile->Attr() );
		//添付ファイルの総数が送信バッファを超えたまたはsprintfが書ききらなかった。
		if ( optBufLen + fileBufLen + wsize - 1 > MAX_UDPBUF || wsize >= sizeof( fileBuf ) - fileBufLen - 1 ) {
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::SendMsg Attachment file's attribute buffer was overflow.\n" );
			fflush(stdout);
#endif
			break;
		}
		fileBufLen += wsize;
		fileBuf[fileBufLen] = '\0';
	}
	memcpy( &optBuf[ optBufLen ], fileBuf, fileBufLen );
	optBufLen += fileBufLen;
	IpMsgPrintBuf( "fileBuf2:", fileBuf, fileBufLen );
	if ( optBufLen >= (int) optBufSize - 1 ) {
		optBufLen =  optBufSize - 1;
	}
	optBuf[ optBufLen ] = '\0';

	IpMsgPrintBuf( "optBuf2:", optBuf, optBufLen );

	//リトライ時は以前のバケットNoを使用する。
	unsigned long packetNo = (isRetry && PrevPacketNo != 0UL ? PrevPacketNo : random() );

	sendBufLen = CreateNewPacketBuffer( IPMSG_SENDMSG | IPMSG_SENDCHECKOPT |
#ifdef HAVE_OPENSSL
										  ( isEncrypted ? IPMSG_ENCRYPTOPT : 0UL ) |
#endif	//HAVE_OPENSSL
										  ( !isLogging.IsLogging() ? IPMSG_NOLOGOPT : 0UL ) |
										  ( isSecret.IsSecret() ? IPMSG_SECRETOPT : 0UL ) |
										  ( _IsAbsence ? IPMSG_AUTORETOPT : 0UL ) |
										  ( isLockPassword.IsLockPassword() ? IPMSG_PASSWORDOPT : 0UL ) |
										  ( files.size() > 0 ? IPMSG_FILEATTACHOPT : 0UL ) | opt,
										  packetNo,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_SENDMSG, sendBuf, sendBufLen, addr );

	if ( !isRetry ) {
		SentMessage message;
		message.setTo( addr );
		message.setHost( host );
		message.setPacketNo( packetNo );
		message.setMessage( msg );
		message.setSent( time( NULL ) );
		message.setPrevTry( message.Sent() );
		message.setIsRetryMaxOver( false );
		message.setRetryCount( 0 );
		message.setIsConfirmed( false );
		message.setIsPasswordLock( isLockPassword.IsLockPassword() );
		message.setIsCrypted( isEncrypted );
		message.setIsConfirmAnswered( false );
		message.setHostCountAtSameTime( hostCountAtSameTime );
		message.setOpt( opt );
		message.setIsNoLogging( !isLogging.IsLogging() );
		message.setIsSecret( isSecret.IsSecret() );
		message.setFiles( files );
		message.setIsSent( false );
		if ( SaveSentMessage() ){
			sentMsgList.append( message );
		}
	}

	free( optBuf );
	IPMSG_FUNC_RETURN( true );
}

/**
 * 登録済のブロードキャストアドレスをクリア
 */
void
IpMessengerAgentImpl::ClearBroadcastAddress()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ClearBroadcastAddress()");
	broadcastAddr.clear();
	IPMSG_FUNC_EXIT;
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 */
void
IpMessengerAgentImpl::DeleteBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteBroadcastAddress( std::string addr )");
	std::vector<struct sockaddr_storage>::iterator net = FindBroadcastNetworkByAddress( addr );
	if ( net != broadcastAddr.end() ) {
#if defined(DEBUG)
		struct sockaddr_storage netaddr = *net;
		printf( "IpMessengerAgentImpl::DeleteBroadcastAddress Address=%s Port=%d\n", getSockAddrInRawAddress( netaddr ).c_str(), ntohs( getSockAddrInPortNo( netaddr ) ) );
		fflush( stdout );
#endif
		broadcastAddr.erase( net );
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

/**
 * ブロードキャストアドレスを登録
 * @param addr 登録するブロードキャストアドレス
 */
void
IpMessengerAgentImpl::AddBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddBroadcastAddress( std::string addr )");
	struct sockaddr_storage addAddr;
	if ( createSockAddrIn( &addAddr, addr, DefaultPortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}

	std::string broadIp = getSockAddrInRawAddress( addAddr );
	std::vector<struct sockaddr_storage>::iterator net = FindBroadcastNetworkByAddress( broadIp );
	if ( net != broadcastAddr.end() ) {
		IPMSG_FUNC_EXIT;
	}
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::AddBroadcastAddress Address=%s Port=%d\n", getSockAddrInRawAddress( &addAddr ).c_str(), ntohs( getSockAddrInPortNo( addAddr ) ) );fflush( stdout );
#endif
	broadcastAddr.push_back( addAddr );
	IPMSG_FUNC_EXIT;
}

/**
 * 登録済のブロードキャストアドレスを検索し、該当するsockaddr_in構造体を返却する。
 * @param addr ブロードキャストアドレス文字列
 * @retval sockaddr_in構造体
 */
std::vector<struct sockaddr_storage>::iterator
IpMessengerAgentImpl::FindBroadcastNetworkByAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("std::vector<struct sockaddr_storage>::iterator IpMessengerAgentImpl::FindBroadcastNetworkByAddress( std::string addr )");
	struct sockaddr_storage ss;
	if ( createSockAddrIn( &ss, addr, 0 ) == NULL ) {
		IPMSG_FUNC_RETURN( broadcastAddr.end() );
	}
	for( std::vector<struct sockaddr_storage>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
		if ( isSameSockAddrIn( ss, *ixaddr ) ){
			IPMSG_FUNC_RETURN( ixaddr );
		}
	}
	IPMSG_FUNC_RETURN( broadcastAddr.end() );
}

/**
 * 登録済の隠れるホストをクリア
 */
void
IpMessengerAgentImpl::ClearSkulkHost()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ClearSkulkHost()");
	skulkHostList.clear();
	if ( IsNetworkStarted() ) {
		Login( Nickname, GroupName );
	}
	IPMSG_FUNC_EXIT;
}

/**
 * 登録済の隠れるホストを削除
 * @param addr 登録済の隠れるホストのアドレス
 */
void
IpMessengerAgentImpl::DeleteSkulkHostAddress( const std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteSkulkHostAddress( std::string addr )");
	//すべてのホストアドレスから探す
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( addr );
	if ( hostIt != hostList.end() ) {
		//発見
		DeleteSkulkHost( *hostIt );
		IPMSG_FUNC_EXIT;
	}
	HostListItem host;
	host.setIpAddress( addr );
	host.setPortNo( DefaultPortNo() );
	DeleteSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * 登録済の隠れるホストを削除
 * @param host 登録済の隠れるホスト
 */
void
IpMessengerAgentImpl::DeleteSkulkHost( const HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteSkulkHost( HostListItem &host )");
	std::vector<HostListItem>::iterator hi = FindSkulkHostByAddress( host.IpAddress() );
	if ( hi != skulkHostList.end() ) {
		if ( IsNetworkStarted() ) {
			struct sockaddr_storage addr;
			if ( createSockAddrIn( &addr, hi->IpAddress(), hi->PortNo() ) == NULL ) {
				IPMSG_FUNC_EXIT;
			}
			VisibleToAddr( addr );
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::DeleteSkulkHost Address=%s Port=%d\n", getSockAddrInRawAddress( addr ).c_str(), ntohs( getSockAddrInPortNo( addr ) ) );
			fflush( stdout );
#endif
		}
		skulkHostList.DeleteHostByAddress( host.IpAddress() );
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

/**
 * 隠れるホストを登録
 * @param addr 登録するホストのアドレス
 */
void
IpMessengerAgentImpl::AddSkulkHostAddress( const std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddSkulkHostAddress( std::string addr )");
	//すべてのホストアドレスから探す
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( addr );
	if ( hostIt != hostList.end() ) {
		//発見
		AddSkulkHost( *hostIt );
		IPMSG_FUNC_EXIT;
	}
	HostListItem host;
	host.setIpAddress( addr );
	host.setPortNo( DefaultPortNo() );
	AddSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * 隠れるホストを登録
 * @param host 登録するホスト
 */
void
IpMessengerAgentImpl::AddSkulkHost( const HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddSkulkHost( HostListItem host )");
	struct sockaddr_storage addAddr;
	if ( createSockAddrIn( &addAddr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}

	std::string hideIp = getSockAddrInRawAddress( addAddr );
	std::vector<HostListItem>::iterator hi = FindSkulkHostByAddress( hideIp );
	if ( hi != skulkHostList.end() ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::AddSkulkHost HideFromAddr\n");fflush( stdout );
#endif
		HideFromAddr( addAddr );
		IPMSG_FUNC_EXIT;
	}
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::AddSkulkHost Address=%s Port=%d\n", getSockAddrInRawAddress( &addAddr ).c_str(), ntohs( getSockAddrInPortNo( addAddr ) ) );fflush( stdout );
#endif
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddSkulkHost HideFromAddr\n");fflush( stdout );
#endif
	HideFromAddr( addAddr );
	skulkHostList.AddHost( host, true );
	IPMSG_FUNC_EXIT;
}

/**
 * 登録済の隠れるホストのアドレスを検索し、該当するsockaddr_in構造体を返却する。
 * @param addr 隠れるホストのアドレス文字列
 * @retval sockaddr_in構造体
 */
std::vector<HostListItem>::iterator
IpMessengerAgentImpl::FindSkulkHostByAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("std::vector<struct sockaddr_storage>::iterator IpMessengerAgentImpl::FindSkulkHostByAddress( std::string addr )");
	struct sockaddr_storage ss;
	if ( createSockAddrIn( &ss, addr, 0 ) == NULL ) {
		IPMSG_FUNC_RETURN( skulkHostList.end() );
	}
	for( std::vector<HostListItem>::iterator ixaddr = skulkHostList.begin(); ixaddr != skulkHostList.end(); ixaddr++ ){
		if ( addr == ixaddr->IpAddress() ){
			IPMSG_FUNC_RETURN( ixaddr );
		}
	}
	IPMSG_FUNC_RETURN( skulkHostList.end() );
}

/**
 * 登録済の隠れるホストのホストリストを返却する。
 * @retval 隠れるホストリスト
 */
HostList
IpMessengerAgentImpl::GetSkulkHost()
{
	IPMSG_FUNC_ENTER("HostList IpMessengerAgent::GetSkulkHost()");
	HostList ret = skulkHostList;
	IPMSG_FUNC_RETURN( ret );
}

/**
 * 対象ホストのバージョン情報を問い合わせる。
 * <ul>
 * <li>GETINFOパケットを送信。</li>
 * </ul>
 * @param host 対象のホスト
 */
void
IpMessengerAgentImpl::QueryVersionInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::QueryVersionInfo( HostListItem& host )");
	char sendBuf[MAX_UDPBUF]={0};
	int sendBufLen;
	struct sockaddr_storage addr;
	if ( createSockAddrIn( &addr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETINFO,
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_GETINFO, sendBuf, sendBufLen, addr );
	IPMSG_FUNC_EXIT;
}

/**
 * 対象ホストのバージョン情報を取得。
 * <ul>
 * <li>バージョン情報を問い合わせる。</li>
 * <li>他のメソッド（ANSINFO受信）にて取得するまで待機。（五回まで）</li>
 * <li>IPアドレスでマッチングしてANSINFOで更新されたバージョン情報を返す。</li>
 * <li>待ち合わせを行います。時間がかかる場合があります。Query系のAPIでイベントを拾うことを推奨。このメソッドからも同じイベントを発行します。</li>
 * </ul>
 * @param host 対象のホスト
 * @retval 対象ホストのバージョン情報
 */
std::string
IpMessengerAgentImpl::GetInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgentImpl::GetInfo( HostListItem& host )");
	//0.05秒まつ。
	usleep( 50000L );
//	RecvPacket( false );
	for( int i = 0; i < 5; i++ ) {
		//0.05秒まつ。
		usleep( 50000L );
//		RecvPacket( false );
	}
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( host.IpAddress() );
	if ( hostIt != hostList.end() ) {
		IPMSG_FUNC_RETURN( hostIt->Version() );
	}
	IPMSG_FUNC_RETURN( "" );
}

/**
 * 対象ホストの不在説明文字列情報を問い合わせる。
 * <ul>
 * <li>GETABSENCEINFOパケットを送信。</li>
 * </ul>
 * @param host 対象のホスト
 */
void
IpMessengerAgentImpl::QueryAbsenceInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::QueryAbsenceInfo( HostListItem& host )");
	char sendBuf[MAX_UDPBUF]={0};
	int sendBufLen;
	struct sockaddr_storage addr;

	if ( createSockAddrIn( &addr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}

	sendBufLen = CreateNewPacketBuffer( IPMSG_GETABSENCEINFO,
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_GETABSENCEINFO, sendBuf, sendBufLen, addr );
	IPMSG_FUNC_EXIT;
}

/**
 * 対象ホストの不在説明文字列情報を取得。
 * <ul>
 * <li>不在説明文字列情報を問い合わせる。</li>
 * <li>他のメソッド（ANSABSENCEINFO受信）にて取得するまで待機。（五回まで）</li>
 * <li>IPアドレスでマッチングしてANSABSENCEINFOで更新された不在説明文字列情報を返す。</li>
 * <li>待ち合わせを行います。時間がかかる場合があります。Query系のAPIでイベントを拾うことを推奨。このメソッドからも同じイベントを発行します。</li>
 * </ul>
 * @param host 対象のホスト
 * @retval 対象ホストの不在説明文字列情報
 */
std::string
IpMessengerAgentImpl::GetAbsenceInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgentImpl::GetAbsenceInfo( HostListItem& host )");
	QueryAbsenceInfo( host );
	//0.05秒まつ。
	usleep( 50000L );
//	RecvPacket( false );
	for( int i = 0; i < 5; i++ ) {
		//0.05秒まつ。
		usleep( 50000L );
//		RecvPacket( false );
	}
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( host.IpAddress() );
	if ( hostIt != hostList.end() ) {
		IPMSG_FUNC_RETURN( hostIt->AbsenceDescription() );
	}
	IPMSG_FUNC_RETURN( "" );
}

/**
 * 保持中のホストリストからグループリストを取得する。
 * @retval グループリスト
 */
std::vector<GroupItem>
IpMessengerAgentImpl::GetGroupList()
{
	IPMSG_FUNC_ENTER("std::vector<GroupItem> IpMessengerAgentImpl::GetGroupList()");
	IPMSG_FUNC_RETURN( hostList.GetGroupList() );
}

/**
 * 送信元にメッセージを削除したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 */
void
IpMessengerAgentImpl::DeleteNotify( RecievedMessage msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteNotify( RecievedMessage msg )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;
	char *dmyptr;
	unsigned long packetNo = strtoul( msg.MessagePacket().Option().c_str(), &dmyptr, 10 );

	optBufLen = IpMsgULongToString( optBuf, sizeof( optBuf ), packetNo );
	sendBufLen = CreateNewPacketBuffer( IPMSG_DELMSG,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_DELMSG, sendBuf, sendBufLen, msg.MessagePacket().Addr() );
	IPMSG_FUNC_EXIT;
}

/**
 * 送信元にメッセージを開封したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 */
void
IpMessengerAgentImpl::ConfirmMessage( RecievedMessage &msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ConfirmMessage( RecievedMessage &msg )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

	if ( ( IPMSG_SECRETOPT & msg.MessagePacket().CommandOption() ) && !msg.IsConfirmed() ) {
		packetNoBufLen = IpMsgULongToString( packetNoBuf, sizeof( packetNoBuf ), msg.MessagePacket().PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_READMSG,
											  _LoginName, _HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( -1, IPMSG_READMSG, sendBuf, sendBufLen, msg.MessagePacket().Addr() );
	}
	msg.setIsConfirmed( true );
//	RecvPacket( false );
	IPMSG_FUNC_EXIT;
}

/**
 * 送信済メッセージリストに開封されたことをマークする。
 * @param msg 送信メッセージオブジェクト。
 */
void
IpMessengerAgentImpl::AcceptConfirmNotify( SentMessage msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AcceptConfirmNotify( SentMessage msg )");
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( msg.PacketNo() );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmAnswered( true );
	}
	IPMSG_FUNC_EXIT;
}

// private methods start here

/**
 * 送信初期化
 * <ul>
 * <li>ブロードキャストアドレス構造体の初期化＋リストに押し込む。</li>
 * </ul>
 */
void
IpMessengerAgentImpl::InitSend( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::InitSend( const std::vector<NetworkInterface>& nics )");
	struct sockaddr_storage addr;
#ifdef ENABLE_IPV4
	if ( haveIPv4Nic ) {
		if ( createSockAddrIn( &addr, "255.255.255.255", DefaultPortNo() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}
		broadcastAddr.push_back( addr );
	}
#endif	
#ifdef ENABLE_IPV6
	if ( _UseIPv6 && haveIPv6Nic ) {
		for( unsigned int i = 0; i < nics.size(); i++ ){
			if ( nics[i].AddressFamily() == AF_INET6 ) {
				if ( createSockAddrIn( &addr, "ff02::1", DefaultPortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
					IPMSG_FUNC_EXIT;
				}
			}
		}
		broadcastAddr.push_back( addr );
	}
#endif	
	for( unsigned int i = 0; i < nics.size(); i++ ){
		struct sockaddr_storage addr;
		if ( createSockAddrIn( &addr, getBroadcastAddress( nics[i].AddressFamily(), nics[i].NetworkAddress(), nics[i].NetMask() ), DefaultPortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}
		bool IsFound = false;
		for( std::vector<struct sockaddr_storage>::iterator i = broadcastAddr.begin(); i != broadcastAddr.end(); ++i ){
#ifdef DEBUG
			std::string check =getSockAddrInRawAddress( &( *i ) );
			std::string base = getSockAddrInRawAddress( &addr );
			int checkp = getSockAddrInPortNo( &( *i ) );
			int basep = getSockAddrInPortNo( &addr );
			printf("IpMessengerAgentImpl::InitSend check[%s][%d] == base[%s][%d]\n", check.c_str(), checkp, base.c_str(), basep );
#endif	
			if ( getSockAddrInRawAddress( &( *i ) ) == getSockAddrInRawAddress( &addr ) &&
				 getSockAddrInPortNo( &( *i ) ) == getSockAddrInPortNo( &addr ) ){
				IsFound = true;
#ifdef DEBUG
				printf("IpMessengerAgentImpl::InitSend Broadcast address was found.\n" );
#endif	
				break;
			}
		}
		if ( !IsFound ) {
#ifdef DEBUG
			printf("IpMessengerAgentImpl::InitSend Broadcast address was not found.\n" );
#endif	
			broadcastAddr.push_back( addr );
		}
	}
#ifdef DEBUG
	for( std::vector<struct sockaddr_storage>::iterator i = broadcastAddr.begin(); i != broadcastAddr.end(); ++i ){
		printf( "IpMessengerAgentImpl::InitSend broadcastAddr=[%s]\n", getSockAddrInRawAddress( &( *i ) ).c_str() );
	}
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * TCPパケットの送信を行う。
 * @param sd ソケットディスクリプタ
 * @param buf バッファ
 * @param size バッファサイズ
 */
void
IpMessengerAgentImpl::SendTcpPacket( int sd, char *buf, int size )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SendTcpPacket( int sd, char *buf, int size )");
#if defined(DEBUG)
	printf( "\n" );
	printf("IpMessengerImpl::SendTcpPacket == SEND TCP START ==============================>\n");fflush( stdout );
#endif
	IpMsgPrintBuf( "IpMessengerImpl::SendTcpPacket Send TCP Buffer", buf, size );
	int ret = 0;
	ret = send( sd, buf, size + 1, 0 );
	if ( ret <= 0 ) {
		perror("send");
#if defined(DEBUG)
		printf("IpMessengerImpl::SendTcpPacket SEND TCP FAILED\n");fflush( stdout );
#endif
	}
#if defined(DEBUG)
	printf("IpMessengerImpl::SendTcpPacket <= SEND TCP END =================================\n\n");fflush( stdout );
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * UDPパケットの送信を行う。
 * @param send_socket 送信ソケット。
 * @param buf バッファ
 * @param size バッファサイズ
 * @param to_addr 送信先のIPアドレス
 */
void
IpMessengerAgentImpl::SendPacket( const int send_socket, const unsigned long cmd, char *buf, int size, struct sockaddr_storage to_addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SendPacket( const unsigned long cmd, char *buf, int size, struct sockaddr_storage to_addr )");
#if defined(DEBUG)
	printf( "\n" );
	printf( "IpMessengerAgentImpl::SendPacket == SEND  START =====================================>\n");fflush( stdout );
	printf( "IpMessengerAgentImpl::SendPacket Packet Command i[%s]To Address=[%s][Sock %d]\n", GetCommandString( cmd ).c_str(), getSockAddrInRawAddress( to_addr ).c_str(), send_socket );fflush( stdout );
#endif
	IpMsgPrintBuf( "IpMessengerImpl::SendUdpPacket Send UDP Buffer", buf, size );

	UdpSendto( send_socket, &to_addr, buf, size );

#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::SendPacket <= SEND  END ========================================\n\n");fflush( stdout );
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * UDPパケットのブロードキャストを行う。
 * <ul>
 * <li>ブロードキャストアドレスリストに登録済のアドレスに全て送信する。</li>
 * </ul>
 * @param buf バッファ
 * @param size バッファサイズ
 */
void
IpMessengerAgentImpl::SendBroadcast( const unsigned long cmd, char *buf, int size )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SendBroadcast( const unsigned long cmd, char *buf, int size )");
#if defined(DEBUG)
	printf( "\n" );
	printf("IpMessengerAgentImpl::SendBroadcast == SEND BROADCAST START ==================>\n");fflush( stdout );
	printf( "IpMessengerAgentImpl::SendBroadcast Command[%s]\n", GetCommandString( cmd ).c_str() );fflush( stdout );
#endif
	IpMsgPrintBuf( "SendBroadcast:SendUdpBroadcastBuffer", buf, size );
	for( std::vector<struct sockaddr_storage>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
#if defined(DEBUG)
		printf( "IpMessengerAgentImpl::SendBroadcast Sendto specified broadcast addresses.Packet Command=[%s]To Address=[%s]\n", GetCommandString( cmd ).c_str(), getSockAddrInRawAddress( *ixaddr ).c_str() );fflush( stdout );
#endif
		UdpSendto( -1, &(*ixaddr), buf, size );
	}
	for( std::vector<HostListItem>::iterator ixhost = hostList.begin(); ixhost != hostList.end(); ixhost++ ){
		if ( ixhost->CommandNo() | IPMSG_DIALUPOPT ) {
			struct sockaddr_storage addr;
			if ( createSockAddrIn( &addr, ixhost->IpAddress(), ixhost->PortNo() ) == NULL ) {
				IPMSG_FUNC_EXIT;
			}
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::SendBroadcast Sendto dialup hosts.Packet Command=[%s]To Address=[%s]\n", GetCommandString( cmd ).c_str(), getSockAddrInRawAddress( addr ).c_str() );fflush( stdout );
#endif
			UdpSendto( -1, &addr, buf, size );
		}
	}
#if defined(DEBUG)
	printf("IpMessengerAgentImpl::SendBroadcast <= SEND BROADCAST END =====================\n\n");fflush( stdout );
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * ソケットにバインドされたネットワークに対して同一ネットワークに属していれば、そのNICから電文送信を行うことを保証するsendto。
 * @param send_socket 送信ソケット。0未満はブロードキャスト。（ソケットは自動選択）。
 * @param addr 送信先のアドレス。
 * @param buf バッファ
 * @param size バッファサイズ
 */
void
IpMessengerAgentImpl::UdpSendto( const int send_socket, struct sockaddr_storage *addr, char *buf, int size )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::UdpSendto( const struct sockaddr_storage *addr, char *buf, int size )");
	//送信ソケットを指定された場合。
	if ( send_socket >= 0 ){
		int ret = sendToSockAddrIn( send_socket, buf, size + 1, addr );
		if ( ret <= 0 ) {
			fprintf( stderr, "IpMessengerAgentImpl::UdpSendto Address=[%s] Port=(%d)@Sock=%d(manual specified for unicasting) errno=(%d):", getSockAddrInRawAddress( addr ).c_str(), ntohs( getSockAddrInPortNo( addr ) ), send_socket, errno );fflush( stdout );
			perror("sendto 1.");
#if defined(DEBUG)
			printf("IpMessengerAgentImpl::UdpSendto SEND FAILED\n");fflush( stdout );
#endif
		}
		IPMSG_FUNC_EXIT;
	}
	//send_socket < 0 はブロードキャスト
	if ( udp_sd.size() == 0 ) {
		IPMSG_FUNC_EXIT;
	}
	int sock = -1;
	int same_family_sock = -1;
	int scope = if_nametoindex( sd_addr.begin()->second.DeviceName().c_str() );

#if defined(DEBUG)
	std::string from_addr = sd_addr.begin()->second.IpAddress();

	printf( "IpMessengerAgentImpl::UdpSendto SendTo Address [%s](%d)\n",
				getSockAddrInRawAddress( addr ).c_str(),
				ntohs( getSockAddrInPortNo( addr ) ) );
	fflush( stdout );
#endif
	for( std::map<int,NetworkInterface>::iterator i = sd_addr.begin(); i != sd_addr.end(); ++i ){
#if defined(DEBUG)
		printf( "IpMessengerAgentImpl::UdpSendto Searching using NIC Address=[%s](%d) Network Address=[%s] Netmask=[%s]\n",
				i->second.IpAddress().c_str(),
				i->second.PortNo(),
				i->second.NetworkAddress().c_str(),
				i->second.NetMask().c_str() );fflush( stdout );
#endif
		if ( isSameNetwork( addr, i->second.NetworkAddress() ,i->second.NetMask() ) ) {
			int scope_id = if_nametoindex( i->second.DeviceName().c_str() );
#ifdef ENABLE_IPV6
			if ( addr->ss_family == AF_INET6 && scope_id != getScopeId( addr ) ){
				continue;
			}
#endif
			sock = i->first;
			scope = scope_id;
#if defined(DEBUG)
			from_addr = i->second.IpAddress();
			printf( "IpMessengerAgentImpl::UdpSendto NIC was found [%s][%s](%d)\n",
					i->second.DeviceName().c_str(),
					i->second.IpAddress().c_str(),
					i->second.PortNo() );fflush( stdout );
#endif
			break;
		}
		//同じアドレスファミリのソケットをデフォルトとして用いる。
		if ( same_family_sock < 0 && addr->ss_family == i->second.AddressFamily() ) {
			same_family_sock = i->first;
			scope = if_nametoindex( i->second.DeviceName().c_str() );
#if defined(DEBUG)
			from_addr = i->second.IpAddress();
#endif
		}
	}
	//送信ソケットが未確定の場合で、
	if ( sock < 0 ) {
		//同じアドレスファミリのデフォルトのソケットが見付からない場合、
		if ( same_family_sock < 0 ) {
			//ソケットの先頭のソケットを用いる。（エラーになる可能性有り）
			sock = udp_sd[0];
#if defined(DEBUG)
			from_addr = sd_addr.begin()->second.IpAddress();
#endif
		} else {
			//同じアドレスファミリのソケットの先頭のソケットを用いる。（エラーにはならないが到達しないかも）
			sock = same_family_sock;
		}
	}
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::UdpSendto Send Packet [%s]->[%s] scope (%d) port(%d)@Sock %d\n",
				from_addr.c_str(),
				getSockAddrInRawAddress( addr ).c_str(),
				getScopeId( addr ),
				ntohs( getSockAddrInPortNo( addr ) ),
				sock );
	fflush( stdout );
#endif
	int ret = sendToSockAddrIn( sock, buf, size + 1, addr );
	if ( ret <= 0 ) {
		fprintf( stderr, "IpMessengerAgentImpl::UdpSendto Address=[%s] Port=(%d)@Sock=%d(automatic specified for broadcasting) errno=(%d):", getSockAddrInRawAddress( addr ).c_str(), ntohs( getSockAddrInPortNo( addr ) ), sock, errno );fflush( stdout );
		perror("sendto 2.");
#if defined(DEBUG)
		printf("IpMessengerAgentImpl::UdpSendto SEND FAILED\n");fflush( stdout );
#endif
	}
	IPMSG_FUNC_EXIT;
}

/**
 * 受信初期化。
 * <ul>
 * <li>ブロードキャストアドレスに関するUDP受信初期化。</li>
 * <li>指定のNICに対してUDPに関する受信初期化。</li>
 * <li>指定のNICに対してTCPに関する受信初期化。</li>
 * </ul>
 * @param nics ネットワークインターフェースのリスト。先頭がデフォルトカードになります。 
 */
void
IpMessengerAgentImpl::InitRecv( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::InitRecv( const std::vector<NetworkInterface>& nics )");
	if ( nics.size() == 0 ) {
		IPMSG_FUNC_EXIT;
	}
	HostAddress = getLocalhostAddress( _UseIPv6, nics );
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::InitRecv localhost IP address=%s\n", HostAddress.c_str() );
#endif
	udp_sd.clear();
	tcp_sd.clear();
	sd_addr.clear();
	sd_address_family.clear();
	for( unsigned int i = 0; i < nics.size(); i++ ){
		struct sockaddr_storage addr;

		if ( createSockAddrIn( &addr, nics[i].IpAddress(), nics[i].PortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}

		int sock = -1;

		sock = InitUdpRecv( addr, nics[i].DeviceName().c_str() );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessgenrAgentImpl::InitRecv UDP Socket Descriptor for unicasting[%d][%s:%s] = %d\n",
							udp_sd.size(),
							getSockAddrInRawAddress( &addr ).c_str(),
							getSockAddrInAddressFamilyString( addr ).c_str(),
							sock );
			fflush( stdout );
#endif
			udp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
			sd_address_family[sock] = addr.ss_family;
		} else {
			printf( "IpMessgenrAgentImpl::InitRecv UDP Error[%s:%s]=%s\n",
							nics[i].DeviceName().c_str(),
							getAddressFamilyString( nics[i].AddressFamily() ).c_str(),
							nics[i].IpAddress().c_str() );
			fflush( stdout );
		}
		sock = InitTcpRecv( addr, nics[i].DeviceName().c_str() );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessgenrAgentImpl::InitRecv TCP Socket Descriptor[%d][%s:%s] = %d\n",
							udp_sd.size(),
							getSockAddrInRawAddress( &addr ).c_str(),
							getSockAddrInAddressFamilyString( addr ).c_str(),
							sock );
			fflush( stdout );
#endif
			tcp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
			sd_address_family[sock] = addr.ss_family;
		} else {
			printf( "IpMessgenrAgentImpl::InitRecv TCP Error[%s:%s]=%s\n",
							nics[i].DeviceName().c_str(),
							getAddressFamilyString( nics[i].AddressFamily() ).c_str(),
							nics[i].IpAddress().c_str() );
			fflush( stdout );
		}

		if ( createSockAddrIn( &addr, nics[i].BroadcastAddress(), nics[i].PortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}
		sock = InitUdpRecv( addr, nics[i].DeviceName().c_str() );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessgenrAgentImpl::InitRecv UDP Socket Descriptor for broadcasting[%d][%s:%s] = %d\n",
							udp_sd.size(),
							getSockAddrInRawAddress( &addr ).c_str(),
							getSockAddrInAddressFamilyString( addr ).c_str(),
							sock );
			fflush( stdout );
#endif
			udp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
			sd_address_family[sock] = addr.ss_family;
		} else {
			printf( "IpMessgenrAgentImpl::InitRecv UDP Error[%s:%s]=%s\n",
							nics[i].DeviceName().c_str(),
							getAddressFamilyString( nics[i].AddressFamily() ).c_str(),
							nics[i].IpAddress().c_str() );
			fflush( stdout );
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
	IPMSG_FUNC_EXIT;
}

/**
 * UDPに関する受信初期化。
 * <ul>
 * <li>2425待ち受けのUDPソケットを準備する</li>
 * <li>UDPはbroadcast許可</li>
 * </ul>
 * @param addr 初期化するアドレス情報。
 * @retval 初期化済みのソケット
 */
int
IpMessengerAgentImpl::InitUdpRecv( struct sockaddr_storage addr, const char *devname )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::InitUdpRecv( struct sockaddr_storage addr, const char *devname )");
	int sock = bindSocket( SOCK_DGRAM, addr, devname );
	if ( sock < 0 ) {
		IPMSG_FUNC_RETURN( -1 );
	}

	int buf_size = MAX_SOCKBUF, buf_minsize = MAX_SOCKBUF / 2;
	if ( setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(sendbuf)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(recvbuf)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}

	IPMSG_FUNC_RETURN( sock );
}

/**
 * TCPに関する受信初期化。
 * <ul>
 * <li>2425待ち受けのTCPソケットを準備する</li>
 * <li>TCPはREUSEADDR</li>
 * <li>litsenは5ポート</li>
 * </ul>
 * @param addr 初期化するアドレス情報。
 * @retval 初期化済みのソケット
 */
int
IpMessengerAgentImpl::InitTcpRecv( struct sockaddr_storage addr, const char *devname )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::InitTcpRecv( struct sockaddr_storage addr, const char *devname )");
	int sock = bindSocket( SOCK_STREAM, addr, devname );
	if ( sock < 0 ) {
		IPMSG_FUNC_RETURN( -1 );
	}

	int yes = 1;
	if ( sock >= 0 && setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) != 0 ) {
		perror("setsockopt(reuseaddr)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( sock >= 0 && listen(sock, 5 ) != 0 ) {
		perror("setsockopt(reuseaddr)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
	IPMSG_FUNC_RETURN( sock );
}

/**
 * 受信処理（ユーザ向け）。
 * @retval 受信パケット数
 */
int
IpMessengerAgentImpl::Process()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::Process()");
	IPMSG_FUNC_RETURN( RecvPacket( true ) );
}

/**
 * 受信処理（内部）。
 * <ul>
 * <li>select(タイムアウト付き)にて受信待ち。</li>
 * <li>受信処理を行い、パケットをキューにため込む。</li>
 * <li>受信が終了したら、キューの中身を処理する。（各イベントを呼び出す。）</li>
 * <li>一定時間以前のパケットと重複判定を行い、重複している場合はパケットを破棄します。</li>
 * </ul>
 * @retval 受信パケット数
 */
int
IpMessengerAgentImpl::RecvPacket( bool isBlock )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::RecvPacket()");
	char buf[MAX_UDPBUF];
	int selret = 1;
	int ret = 0;
	time_t nowTime = time( NULL );

	std::vector<Packet> pack_que;

	while( selret > 0 ) {
		fd_set fds;
		memcpy( &fds, &rfds, sizeof( fd_set ) );
		memset( buf, 0, sizeof( buf ) );

		tv.tv_sec = SELECT_TIMEOUT_SEC;
		tv.tv_usec = SELECT_TIMEOUT_USEC;
//		if ( isBlock ) {
//			selret = select( max_sd + 1, &fds, NULL, NULL, NULL );
//		} else {
			selret = select( max_sd + 1, &fds, NULL, NULL, &tv );
//		}
		if ( selret == -1 ) {
			//selectが割り込み発生で戻った場合は、無視します。
			if ( errno == EINTR ){
				continue;
			}
			perror( "select()" );
			break;
		} else if ( selret == 0 ){
#if defined(INFO) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::RecvPacket Waiting for next packet.\n");fflush( stdout );
#endif
			break;
		} else {
			int udp_socket = -1;
			int tcp_socket = -1;
#if defined(DEBUG)
			printf("\n");fflush( stdout );
			printf( "IpMessengerAgentImpl::RecvPacket select returns == %d\n\n", selret );fflush( stdout );
#endif
			struct sockaddr_storage sender_addr;
			int sz = sizeof( buf );
			bool recieved = RecvUdp( &fds, &sender_addr, &sz, buf, &udp_socket );
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
			Packet packet = DismantlePacketBuffer( udp_socket >= 0 ? udp_socket : tcp_socket, buf, sz, sender_addr, nowTime );
			packet.setUdpSocket( udp_socket );
			packet.setTcpSocket( tcp_socket );
			//同一セッション内だけパケットの一意性をチェック。重複パケットは無視
			if ( !FindDuplicatePacket( packet ) ) {
#if defined(INFO) || !defined(NDEBUG)
				struct sockaddr_storage tempAddr = packet.Addr();	
				IpMsgDumpPacket( packet, &tempAddr );
#endif
				pack_que.push_back( packet );
				PacketsForChecking.push_back( packet );
				ret++;
			}
		}
	}
	// TODO pack_que,PacketsForCheckingはdequeのほうが。。。？
	//パケットを処理する。
//	printf("start RecvDoCommand\n");
	while( !pack_que.empty() ) {
//		printf("do RecvDoCommand\n");
		DoRecvCommand( pack_que.front() );
		pack_que.erase( pack_que.begin() );
	}
//	printf("end RecvDoCommand\n");

	//一定以上前のチェック用のパケットベクタを消す。
	PurgePacket( nowTime );

	//メッセージ送信リトライのチェック
	CheckSendMsgRetry( nowTime );

	//ホストリスト取得のリトライチェック
	CheckGetHostListRetry( nowTime );

	IPMSG_FUNC_RETURN( ret );
}

void
IpMessengerAgentImpl::SkulkFromHost( const Packet &packet )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SkulkHost( Packet packet )");
	//隠れるホストにエントリーされていたら無視。
	struct sockaddr_storage pAddr = packet.Addr();
	std::string hideIp = getSockAddrInRawAddress( pAddr );
	std::vector<HostListItem>::iterator hi = FindSkulkHostByAddress( hideIp );
	if ( hi != skulkHostList.end() ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::UdpRecvEventGetPubKey HideFromAddr\n");fflush( stdout );
#endif
		HideFromAddr( pAddr );
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

/**
 * TCPパケットを受信し、パケットオブジェクトを生成する。
 * @param fds FD_SET構造体
 * @param sender_addr IPアドレスのアドレス
 * @param sz 受信バッファのサイズのアドレス
 * @param buf 受信バッファのアドレス
 * @param tcp_socket 受信したUDPソケット
 */
bool
IpMessengerAgentImpl::RecvUdp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *udp_socket )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::RecvUdp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf )");
	socklen_t sender_addr_len = 0;
	bool recieved = false;
	int size = *sz;

	//UDPでソケットに変化が有ったら受信
	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		if ( FD_ISSET( udp_sd[i], fds ) ){
			memset( sender_addr, 0, sizeof( struct sockaddr_storage ) );
			sender_addr_len = sizeof( struct sockaddr_storage );
			size = recvfrom( udp_sd[i], buf, size, 0, (struct sockaddr *)sender_addr, &sender_addr_len );
			if ( size < 0 ) {
				perror("recvfrom");
			}
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::RecvUdp Recieved UDP_SD[%d] == %d[%s]\n",
						i, udp_sd[i], getSockAddrInRawAddress( sender_addr ).c_str() );
			fflush( stdout );
#endif
			IpMsgPrintBuf( "IpMessengerImpl::RecvUdp recvfrom buf", buf, size );
			*udp_socket = udp_sd[i];
			recieved = true;
			break;
		}
	}
	IPMSG_FUNC_RETURN( recieved );
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
IpMessengerAgentImpl::RecvTcp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *tcp_socket )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::RecvTcp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *tcp_socket )");
	socklen_t sender_addr_len = 0;
	bool recieved = false;
	int size = *sz;

	//TCPでソケットに変化が有ったら受信
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		if ( FD_ISSET( tcp_sd[i], fds ) ){
			memset( sender_addr, 0, sizeof( struct sockaddr_storage ) );
			sender_addr_len = sizeof( struct sockaddr_storage );
			*tcp_socket = accept( tcp_sd[i], (struct sockaddr *)sender_addr, &sender_addr_len );
			if ( *tcp_socket < 0 ) {
				perror("accept");
			}
			size = recv( *tcp_socket, buf, size, 0 );
			if ( size < 0 ) {
				perror("recv");
			}
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessengerAgentImpl::RecvTcp Recieved TCP_SD[%d] == %d\n", i, tcp_sd[i] );fflush( stdout );
#endif
			IpMsgPrintBuf( "recv buf", buf, size );
			recieved = true;
			break;
		}
	}
	IPMSG_FUNC_RETURN( recieved );
}

/**
 * 過去のパケットから重複パケットを検索する。
 * @param packet パケット
 * @retval true:存在
 * @retval false:存在しない
 */
bool
IpMessengerAgentImpl::FindDuplicatePacket( const Packet &packet )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::FindDuplicatePacket( const Packet &packet )");
	//直近のパケットから探す。
	for( int i = (int)PacketsForChecking.size() - 1; i >= 0; i-- ){
		if ( PacketsForChecking[i].PacketNo()             == packet.PacketNo() &&
			 isSameSockAddrIn( PacketsForChecking[i].Addr(), packet.Addr() ) ) {
			IPMSG_FUNC_RETURN( true );
		}
	}
	IPMSG_FUNC_RETURN( false );
}

/**
 * 過去のパケットから必要ない（古い）パケットを削除する。
 * @param nowTime システム時刻
 */
void
IpMessengerAgentImpl::PurgePacket( time_t nowTime )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::PurgePacket( time_t nowTime )");
	for( std::vector<Packet>::iterator pack = PacketsForChecking.begin(); pack != PacketsForChecking.end(); pack++ ){
		if ( nowTime > pack->Recieved() + PACKET_CHECK_FOR_SAVING_INTERVAL ) {
			pack = PacketsForChecking.erase( pack ) - 1;
		} else {
			break;
		}
	}
	IPMSG_FUNC_EXIT;
}

/**
 * メッセージ送信のリトライの必要性をチェックし、必要ならリトライを行う。
 * @param nowTime システム時刻
 */
void
IpMessengerAgentImpl::CheckSendMsgRetry( time_t nowTime )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::CheckSendMsgRetry( time_t nowTime )");
	for( std::vector<SentMessage>::iterator ixmsg = sentMsgList.begin(); ixmsg != sentMsgList.end(); ixmsg++ ) {
		if ( ixmsg->needSendRetry( nowTime ) ) {
			//再送信
			ixmsg->setRetryCount( ixmsg->RetryCount() + 1 );
			ixmsg->setPrevTry( nowTime );
			SendMsg( ixmsg->Host(),
					 ixmsg->Message(),
					 Secret( ixmsg->IsSecret() ),
					 ixmsg->Files(),
					 LockPassword( ixmsg->IsPasswordLock() ),
					 ixmsg->HostCountAtSameTime(),
					 Logging( !ixmsg->IsNoLogging() ),
					 ixmsg->Opt(),
					 true,
					 ixmsg->PacketNo() );
		}
		if ( ixmsg->isRetryMaxOver() ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::CheckSendMsgRetry Retry Max Over\n");fflush( stdout );
#endif
			ixmsg->setRetryCount( 0 );
			ixmsg->setIsRetryMaxOver( true );
			if ( event != NULL ){
				//リトライを続ける場合はTrueをセット。続けない場合はFalseをセット。
				//（RetryMaxOver(メッセージはエラー)状態にすれば、継続しません）
				//イベントの戻り値はtrue:継続、false:中断になります。
				event->EventBefore();
#if defined(DEBUG)
				printf("SendRetryError before\n");
#endif
				ixmsg->setIsRetryMaxOver( !event->SendRetryError( *ixmsg ) );
#if defined(DEBUG)
				printf("SendRetryError after\n");
#endif
				event->EventAfter();
			}
			//イベントで継続を設定しない場合はリトライマックスオーバーしたらやめる。
		}
	}
	IPMSG_FUNC_EXIT;
}

/**
 * ホストリスト取得のリトライの必要性をチェックし、必要ならリトライを行う。
 * @param nowTime システム時刻
 */
void
IpMessengerAgentImpl::CheckGetHostListRetry( time_t nowTime )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::CheckGetHostListRetry( time_t nowTime )");

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
					event->EventBefore();
#if defined(DEBUG)
					printf("GetHostListRetryError before\n");
#endif
					hostList.setIsAsking( event->GetHostListRetryError() );
#if defined(DEBUG)
					printf("GetHostListRetryError after\n");
#endif
					event->EventAfter();
				}
			}
		}
	}
	IPMSG_FUNC_EXIT;
}

// Protocol event processor start here.
/**
 * パケットのコマンドモードで受信イベントを振り分ける。
 * @param packet パケットオブジェクト
 */
void
IpMessengerAgentImpl::DoRecvCommand( const Packet& packet )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DoRecvCommand( const Packet& packet )");
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::DoRecvCommand [Command %s][From %s][Sock %d]\n", GetCommandString( packet.CommandMode() ).c_str(), getSockAddrInRawAddress( packet.Addr() ).c_str(), packet.UdpSocket() );
	fflush( stdout );
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
		default:
			fprintf(stderr, "PROTOCOL COMMAND MISS!!(CommandMode = 0x%08lx)\n", packet.CommandMode() );fflush(stderr);
	}
	SkulkFromHost( packet );
	IPMSG_FUNC_EXIT;
}

/**
 * 電文受信イベント：NOOPERATION
 * <ul>
 * <li>何もしない</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventNoOperation( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventNoOperation( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventNoOperation\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文送信(NOOPERATION)
 * <ul>
 * <li>NOOPERATIONを送信。</li>
 * </ul>
 */
int
IpMessengerAgentImpl::SendNoOperation()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::SendNoOperation()");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( IPMSG_NOOPERATION,
										  _LoginName, _HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_NOOPERATION, sendBuf, sendBufLen );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：BR_ENTRY
 * <ul>
 * <li>送信元にANSENTRYを送信する。</li>
 * <li>不在モードの場合、不在として送信。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrEntry( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventBrEntry( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string optBuf;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrEntry\n");fflush( stdout );
#endif
	if ( _IsAbsence ) {
		std::string AbsenceName = "";
		for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceName = i->AbsenceName();
				break;
			}
		}
		optBuf = Nickname + "[" + AbsenceName + "]";
	} else {
		optBuf = Nickname;
	}
	optBuf += '\0' + GroupName;
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_ANSENTRY ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1/*packet.UdpSocket()*/, IPMSG_ANSENTRY, sendBuf, sendBufLen, packet.Addr() );
#ifdef HAVE_OPENSSL
	GetPubKey( packet.Addr() );
#endif
	// ホストリストに追加
	int ret = AddHostListFromPacket( packet );
	std::vector<HostListItem>::iterator it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	if ( event != NULL ) {
		event->EventBefore();
		if ( it != appearanceHostList.end() && !it->IsLocalHost() && ret > 0 ) {
#if defined(DEBUG)
			printf("EntryAfter before\n");
#endif
			event->EntryAfter( *it );
#if defined(DEBUG)
			printf("EntryAfter after\n");
#endif
		}
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文送信（BR_ABSENCE）
 * <ul>
 * <li>不在通知／不在解除電文を送信する。</li>
 * </ul>
 */
int
IpMessengerAgentImpl::SendAbsence()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::SendAbsence()");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string optBuf;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::SendBrAbsence\n");fflush( stdout );
#endif
	if ( _IsAbsence ) {
		std::string AbsenceName = "";
		for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceName = i->AbsenceName();
				break;
			}
		}
		optBuf = Nickname + "[" + AbsenceName + "]";
	} else {
		optBuf = Nickname;
	}
	optBuf += '\0' + GroupName;
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ABSENCE ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ABSENCE, sendBuf, sendBufLen );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：BR_ABSENCE
 * <ul>
 * <li>自分のホストリストを更新する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrAbsence( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventBrAbsence( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrAbsence\n");fflush( stdout );
#endif
	std::vector<HostListItem>::iterator it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	hostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	hostList.AddHost( HostList::CreateHostListItemFromPacket( packet ) );
	appearanceHostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	int ret = appearanceHostList.AddHost( HostList::CreateHostListItemFromPacket( packet ), false );
#ifdef HAVE_OPENSSL
	GetPubKey( packet.Addr() );
#endif
	if ( event != NULL ){
		it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
		event->EventBefore();
		if ( it != appearanceHostList.end() && ret > 0 ) {
#if defined(DEBUG)
			printf("AbsenceModeChangeAfter before\n");
#endif
			event->AbsenceModeChangeAfter( *it );
#if defined(DEBUG)
			printf("AbsenceModeChangeAfter after\n");
#endif
		}
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：BR_EXIT
 * <ul>
 * <li>自分のホストリストからホストを削除する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrExit( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventBrExit( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrExit\n");fflush( stdout );
#endif
	std::vector<HostListItem>::iterator it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	bool isFound = false;
	HostListItem host;
	if ( it != appearanceHostList.end() ) {
		isFound = true;
		host = *it;
	}
	appearanceHostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	hostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	if ( event != NULL ) {
		event->EventBefore();
		if ( isFound ) {
#if defined(DEBUG)
			printf("ExitAfter before\n");
#endif
			event->ExitAfter( host );
#if defined(DEBUG)
			printf("ExitAfter after\n");
#endif
		}
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：RECVMSG
 * <ul>
 * <li>自分の送信済メッセージリストの該当メッセージに送信済フラグを立てる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventRecvMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventRecvMsg( const Packet& packet )");
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packetNo );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsSent( true );
		sentMsg->setRetryCount( 0 );
		sentMsg->setIsRetryMaxOver( true );
		if ( event != NULL ){
			event->EventBefore();
#if defined(DEBUG)
			printf("SendAfter before\n");
#endif
			event->SendAfter( *sentMsg );
#if defined(DEBUG)
			printf("SendAfter after\n");
#endif
			event->EventAfter();
		}
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventRecvMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：READMSG
 * <ul>
 * <li>READCHECKOPTが付いている場合、ANSREADMSGを投げる。</li>
 * <li>自分の送信済メッセージリストの該当メッセージに既読フラグを立てる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventReadMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventReadMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;
	
	if ( packet.CommandOption() & IPMSG_READCHECKOPT ) {
		packetNoBufLen = IpMsgULongToString( packetNoBuf, sizeof( packetNoBuf ), packet.PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_ANSREADMSG,
											  _LoginName, _HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( packet.UdpSocket(), IPMSG_ANSREADMSG, sendBuf, sendBufLen, packet.Addr() );
	}

	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmed( true );
		if ( event != NULL ) {
			event->EventBefore();
#if defined(DEBUG)
			printf("OpenAfter before\n");
#endif
			event->OpenAfter( *sentMsg );
#if defined(DEBUG)
			printf("OpenAfter after\n");
#endif
			event->EventAfter();
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventReadMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：DELMSG
 * <ul>
 * <li>自分の送信済メッセージリストの該当メッセージを削除。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsgList.erase(sentMsg);
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventDelMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：ANSREADMSG
 * <ul>
 * <li>何もしない。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsReadMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsReadMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：SENDMSG
 * <ul>
 * <li>BROADCASTOPT or AUTORETOPTなら自動応答しない。</li>
 * <li>SENDCHECKOPT付きならRECVMSGを投げる。</li>
 * <li>自分が不在なら不在応答をする。</li>
 * <li>暗号化メッセージなら復号。</li>
 * <li>受信済メッセージに追加。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

#if defined(DEBUG) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventSendMsg Recieved Message List Count(%d)\n", recvMsgList.size() );fflush( stdout );
#endif
	for( std::vector<RecievedMessage>::iterator ixmsg = recvMsgList.begin(); ixmsg != recvMsgList.end(); ixmsg++ ) {
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::UdpRecvEventSendMsg Searching Recieved Message List..." \
					" Now processing packet numbero = (%ld) packet number in list(%ld)\n",
					packet.PacketNo(), ixmsg->MessagePacket().PacketNo() );fflush( stdout );
#endif
		if ( packet.PacketNo() == ixmsg->MessagePacket().PacketNo() ) {
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::UdpRecvEventSendMsg Message was already added.\n");fflush( stdout );
#endif
			IPMSG_FUNC_RETURN( 0 );
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvSendMsg[Packet = %lu]\n", packet.PacketNo() );fflush( stdout );
#endif
	bool noRaiseEvent = false;
	if ( packet.CommandOption() & IPMSG_BROADCASTOPT ||  packet.CommandOption() & IPMSG_AUTORETOPT ) {
		;
	} else {
		if ( packet.CommandOption() & IPMSG_SENDCHECKOPT ) {
			packetNoBufLen = IpMsgULongToString( packetNoBuf, sizeof( packetNoBuf ), packet.PacketNo() );
			sendBufLen = CreateNewPacketBuffer( IPMSG_RECVMSG,
												  _LoginName, _HostName,
												  packetNoBuf, packetNoBufLen,
												  sendBuf, sizeof( sendBuf ) );
			SendPacket( packet.UdpSocket(), IPMSG_RECVMSG, sendBuf, sendBufLen, packet.Addr() );
		}
		if ( _IsAbsence ) {
			HostListItem host;
			host.setIpAddress( getSockAddrInRawAddress( packet.Addr() ) );
			host.setPortNo( ntohs( getSockAddrInPortNo( packet.Addr() ) ) );
			host.setEncodingName( localEncoding );
			std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( host.IpAddress() );
			if ( hostIt != appearanceHostList.end() ) {
				host = *hostIt;
//				host.setEncodingName( hostIt->EncodingName() );
			}
			std::string AbsenceDescription = "";
			for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
				if ( i->EncodingName() == localEncoding ) {
					AbsenceDescription = i->AbsenceDescription();
					break;
				}
			}
			SendMsg( host, AbsenceDescription.c_str(), Secret::Off(), LockPassword::Off(), 1, Logging::Off() );
		}
	}

	std::string optionMessage = packet.Option();
	if ( packet.CommandOption() & IPMSG_ENCRYPTOPT ){
		if ( !DecryptMsg( packet, optionMessage ) ) {
			HostListItem host;
			host.setIpAddress( getSockAddrInRawAddress( packet.Addr() ) );
			host.setPortNo( ntohs( getSockAddrInPortNo( packet.Addr() ) ) );
			SendMsg( host, DecryptErrorMessage.c_str(), Secret::Off(), LockPassword::Off(), 1, Logging::Off(), IPMSG_AUTORETOPT );
			optionMessage = "";
			//暗号解除失敗による自動応答時はイベントを起こさない。
			noRaiseEvent = true;
		}
	}
	RecievedMessage message;
	message.setMessagePacket( packet );
	message.setMessage( optionMessage.c_str() );
	message.setRecieved( time( NULL ) );
	message.setIsNoLogging( IPMSG_NOLOGOPT & packet.CommandOption() );
	message.setIsSecret( IPMSG_SECRETOPT & packet.CommandOption() );
	message.setIsCrypted( IPMSG_ENCRYPTOPT & packet.CommandOption() );
	message.setIsPasswordLock( IPMSG_PASSWORDOPT & packet.CommandOption() );
	message.setIsMulticast( IPMSG_MULTICASTOPT & packet.CommandOption() );
	message.setIsBroadcast( IPMSG_BROADCASTOPT & packet.CommandOption() );
	message.setIsConfirmed( false );
	for( std::vector<HostListItem>::iterator ixhost = appearanceHostList.begin(); ixhost != appearanceHostList.end(); ixhost++ ) {
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
		event->EventBefore();
#if defined(DEBUG)
		printf("RecieveAfter before\n");
#endif
		eventRet = event->RecieveAfter( message );
#if defined(DEBUG)
		printf("RecieveAfter after\n");
#endif
		event->EventAfter();
	}
	if ( SaveRecievedMessage() && !eventRet ){
		recvMsgList.append( message );
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：BR_ISGETLIST
 * <ul>
 * <li>OKGETLISTを投げる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrIsGetList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrIsGetList\n");fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_OKGETLIST ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_OKGETLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：BR_ISGETLIST2
 * <ul>
 * <li>OKGETLISTを投げる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrIsGetList2( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrIsGetList2\n");fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_OKGETLIST ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_OKGETLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：GETLIST
 * <ul>
 * <li>ANSLISTを投げる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	int start = 0;
	char *dmy;
	std::string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetList[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	start = strtoul( packet.Option().c_str(), &dmy, 10 );
	struct sockaddr_storage addr = packet.Addr();
	hosts = hostListBackup.ToString( start, &addr );
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_ANSLIST ),
										_LoginName, _HostName,
										hosts.c_str(), hosts.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_ANSLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：OKGETLIST
 * <ul>
 * <li>GETLISTを投げる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventOkGetList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventOkGetList[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_GETLIST ),
										_LoginName, _HostName,
										"0", 1,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_GETLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：ANSENTRY
 * <ul>
 * <li>パケットからホストリストにホストの情報を追加する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsEntry( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsEntry\n");fflush( stdout );
#endif
	// ホストリストに追加
	AddHostListFromPacket( packet ); 
#ifdef HAVE_OPENSSL
	GetPubKey( packet.Addr() );
#endif
	if ( event != NULL ) {
		event->EventBefore();
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：ANSLIST
 * <ul>
 * <li>要求に応じたホストリストの部分をGETLISTに詰めて投げる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char nextBuf[1024];

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsList\n");fflush( stdout );
#endif
	AddDefaultHost();
	int nextstart = CreateHostList( getSockAddrInRawAddress( packet.Addr() ).c_str(),
									packet.HostName().c_str(),
									packet.Option().c_str(),
									packet.Option().length() );
	if ( nextstart > 0 ) {
		int nextBufLen = IpMsgIntToString( nextBuf, sizeof( nextBuf ), nextstart + 1 );
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::UdpRecvEventDelMsg nextBufLen = %d\n", nextBufLen );fflush( stdout );
#endif
		sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_GETLIST ),
											_LoginName, _HostName,
											nextBuf, nextBufLen,
											sendBuf, sizeof( sendBuf ) );
		SendPacket( packet.UdpSocket(), IPMSG_GETLIST, sendBuf, sendBufLen, packet.Addr() );
	}
	std::string packetIpAddress = getSockAddrInRawAddress( packet.Addr() );
	for( unsigned int i = 0; i < NICs.size(); i++ ){
		if ( packetIpAddress == NICs[i].IpAddress() ){
			IPMSG_FUNC_RETURN( 0 );
		}
	}
	//自分以外からのホストリスト通知があれば、リトライ関連変数をクリア。
	hostList.setIsAsking( false );
	hostList.setAskStartTime( 0L );
	hostList.setPrevTry( 0L );
	hostList.setRetryCount( 0 );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：GETINFO
 * <ul>
 * <li>バージョン情報をSENDINFOに詰めて投げる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventGetInfo( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string version = IPMSG_AGENT_VERSION;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetInfo[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_SENDINFO ),
										_LoginName, _HostName,
										version.c_str(), version.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_SENDINFO, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：SENDINFO
 * <ul>
 * <li>取得したバージョン情報をホストリストに更新する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventSendInfo( const Packet& packet )");
	std::string pIpAddress = getSockAddrInRawAddress( packet.Addr() );
	std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( pIpAddress );
	if ( hostIt != appearanceHostList.end() ) {
		hostIt->setVersion( packet.Option() );
		if ( event != NULL ){
			event->EventBefore();
#if defined(DEBUG)
			printf("VersionInfoRecieveAfter before\n");
#endif
			event->VersionInfoRecieveAfter( *hostIt, packet.Option() );
#if defined(DEBUG)
			printf("VersionInfoRecieveAfter after\n");
#endif
			event->EventAfter();
		}
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：GETABSENCEINFO
 * <ul>
 * <li>不在詳細情報をSENDINFOに詰めて投げる。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	std::string AbsenceDescription = "";
	if ( _IsAbsence  ){
		std::string IpAddress = getSockAddrInRawAddress( packet.Addr() );
		std::string EncodingName = localEncoding;
		std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( IpAddress );
		if ( hostIt != appearanceHostList.end() ) {
			EncodingName = hostIt->EncodingName();
		}
		for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
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
	SendPacket( packet.UdpSocket(), IPMSG_SENDABSENCEINFO, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：SENDABSENCEINFO
 * <ul>
 * <li>取得した不在詳細情報をホストリストに更新する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendAbsenceInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventSendAbsenceInfo( const Packet& packet )");
	std::string pIpAddress = getSockAddrInRawAddress( packet.Addr() );
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
		hostIt->setAbsenceDescription( packet.Option() );
		if ( event != NULL ){
			event->EventBefore();
#if defined(DEBUG)
			printf("AbsenceDetailRecieveAfter before\n");
#endif
			event->AbsenceDetailRecieveAfter( *hostIt, packet.Option() );
#if defined(DEBUG)
			printf("AbsenceDetailRecieveAfter after\n");
#endif
			event->EventAfter();
		}
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * パケットからオフセットを取得します。
 * <ul>
 * <li>パケットからオフセットを抽出し返します。</li>
 * </ul>
 * @param packet パケットオブジェクト
 * @retval ファイルオフセット。
 */
static unsigned long
GetSendFileOffsetInPacket( const Packet& packet )
{
	IPMSG_FUNC_ENTER("static unsigned long GetSendFileOffsetInPacket( const Packet& packet )");
	char *dmyptr;
	char *startptr;
	strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;
	strtoul( startptr, &dmyptr, 16 );
	startptr = ++dmyptr;
	unsigned long offset = strtoul( startptr, &dmyptr, 16 );

	IPMSG_FUNC_RETURN( offset );
}

/**
 * 電文受信イベント：BR_GETFILEDATA
 * <ul>
 * <li>ファイル情報をTCPソケットにのせて送信し、その後ファイルをダウンロードさせる。
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::TcpRecvEventGetFileData( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::TcpRecvEventGetFileData( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::TcpRecvEventGetFileData\n" );fflush( stdout );
#endif

	pthread_t t_id;

	Packet *packetClone = new Packet( packet );

	if ( pthread_create( &t_id, NULL, GetFileDataThread, (void *)packetClone ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_create");
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( pthread_detach( t_id ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_detach");
		IPMSG_FUNC_RETURN( -1 );
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * TODO 何するの？
 * 電文受信イベント：BR_RELEASEFILES
 * <ul>
 * <li> TODO 何するの？</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventReleaseFiles( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventReleaseFiles( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::UdpRecvEventReleaseFiles\n" );fflush( stdout );
#endif
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packetNo );
	if ( sentMsg != sentMsgList.end() ) {
		std::vector<AttachFile>::iterator FoundFile = sentMsg->FindAttachFileByPacket( packet );
		if ( FoundFile == sentMsg->Files().end() ){
			sentMsg->Files().erase( FoundFile );
		}
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：BR_GETPUBKEY
 * <ul>
 * <li>RSA公開鍵をANSPUBKEYにのせて送信する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetPubKey( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventGetPubKey( const Packet& packet )");
#ifdef HAVE_OPENSSL
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetPubKey[%s]\n", packet.Option().c_str());fflush( stdout );
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
		SendPacket( packet.UdpSocket(), IPMSG_ANSPUBKEY, sendBuf, sendBufLen, packet.Addr() );
#ifdef DEBUG
	} else {
		printf("IpMessengerAgentImpl::UdpRecvGetPubKey RSA is NULL[%lu]\n", cap );fflush( stdout );
#endif
	}
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：BR_ANSPUBKEY
 * <ul>
 * <li>取得したRSA公開鍵をホストリストに更新する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsPubKey( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventAnsPubKey( const Packet& packet )");
#ifdef HAVE_OPENSSL
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsPubKey[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	//OptionはHex表現で
	//XXXXX:EEEEE-NNNNN
	//XXXXX=能力フラグのHEX表現
	//EEEEE=RSA公開キー（指数）
	//NNNNN=RSAモジュール
	char *opt = (char *)calloc( packet.Option().length() + 1, 1 );
	if ( opt == NULL ){
		IPMSG_FUNC_RETURN( 0 );
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
		IPMSG_FUNC_RETURN( 0 );
	}
	token = nextpos;
	token = strtok_r( token, "-", &nextpos );
	std::string meth;
	if ( nextpos != NULL ) {
		meth = token;
	} else {
		free( opt );
		IPMSG_FUNC_RETURN( 0 );
	}
	std::string pkey;
	if ( token != NULL ) {
		pkey = nextpos;
	} else {
		free( opt );
		IPMSG_FUNC_RETURN( 0 );
	}
	free( opt );
	std::string pIpAddress = getSockAddrInRawAddress( packet.Addr() );
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
#ifdef DEBUG
printf( "IpMessengerAgentImpl::UdpRecvEventAnsPubKey Set key [%s][%s]\n", pkey.c_str(), meth.c_str() );
#endif
		hostIt->setEncryptionCapacity( cap );
		hostIt->setPubKeyHex( pkey );
		hostIt->setEncryptMethodHex( meth );
	}
	 hostIt = appearanceHostList.FindHostByAddress( pIpAddress );
	if ( hostIt != appearanceHostList.end() ) {
#ifdef DEBUG
printf( "IpMessengerAgentImpl::UdpRecvEventAnsPubKey appearanceHostList Set key [%s][%s]\n", pkey.c_str(), meth.c_str() );
#endif
		hostIt->setEncryptionCapacity( cap );
		hostIt->setPubKeyHex( pkey );
		hostIt->setEncryptMethodHex( meth );
	}
#endif
	//イベントを挙げる
	if ( event != NULL ) {
		event->EventBefore();
#if defined(DEBUG)
		printf("UpdateHostListAfter before\n");
#endif
		event->UpdateHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("UpdateHostListAfter after\n");
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}

	IPMSG_FUNC_RETURN( 0 );
}

/**
 * 電文受信イベント：GETDIRFILES
 * <ul>
 * <li>パケットで指定されたディレクトリを送信する。</li>
 * </ul>
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::TcpRecvEventGetDirFiles( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::TcpRecvEventGetDirFiles( const Packet& packet )");
	pthread_t t_id;
	Packet *packetClone = new Packet( packet );

	if ( pthread_create( &t_id, NULL, GetDirFilesThread, (void *)packetClone ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_create");
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( pthread_detach( t_id ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_detach");
		IPMSG_FUNC_RETURN( -1 );
	}

	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ファイルダウンロードスレッド
 * <ul>
 * <li>ファイルをダウンロードさせる。</li>
 * </ul>
 * @param param パケットオブジェクト(void*)
 */
void *
ipmsg::GetFileDataThread( void *param )
{
	IPMSG_FUNC_ENTER("void * ipmsg::GetFileDataThread( void *param )");
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::GetFileDataThread\n" );fflush( stdout );
#endif

	Packet *packet = (Packet *)param;

	std::vector<SentMessage>::iterator msg = IpMessengerAgentImpl::GetInstance()->GetSentMessages()->FindSentMessageByPacket( *packet );
	if ( msg == IpMessengerAgentImpl::GetInstance()->GetSentMessages()->end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}
	std::vector<AttachFile>::iterator FoundFile = msg->FindAttachFileByPacket( *packet );
	if ( FoundFile == msg->Files().end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}

	FoundFile->setIsDownloading( true );
	bool ret = IpMessengerAgentImpl::GetInstance()->SendFile( packet->TcpSocket(),
															  FoundFile->FullPath(),
															  FoundFile->MTime(),
															  FoundFile->FileSize(),
															  &(*FoundFile),
															  GetSendFileOffsetInPacket( *packet ) );
	FoundFile->setIsDownloading( false );
	FoundFile->setIsDownloaded( ret );
	close( packet->TcpSocket() );
	delete packet;
	IPMSG_FUNC_RETURN( NULL );
}

/**
 * ディレクトリダウンロードスレッド
 * <ul>
 * <li>ディレクトリをダウンロードさせる。</li>
 * </ul>
 * @param param パケットオブジェクト(void*)
 */
void *
ipmsg::GetDirFilesThread( void *param )
{
	IPMSG_FUNC_ENTER("void * ipmsg::GetDirFilesThread( void *param )");
	Packet *packet = (Packet *)param;
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::TcpRecvEventGetDirFiles\n" );fflush( stdout );
#endif
	std::vector<SentMessage>::iterator msg = myInstance->GetSentMessages()->FindSentMessageByPacket( *packet );
	if ( msg == myInstance->GetSentMessages()->end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}
	std::vector<AttachFile>::iterator FoundFile = msg->FindAttachFileByPacket( *packet );
	if ( FoundFile == msg->Files().end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}

	std::vector<std::string> DownloadFileList;
	FoundFile->setIsDownloading( true );
	bool ret = myInstance->SendDirData( packet->TcpSocket(), FoundFile->FileName(), FoundFile->FullPath(), DownloadFileList );
	FoundFile->setIsDownloading( false );
	FoundFile->setIsDownloaded( ret );
	close( packet->TcpSocket() );
	delete packet;

	IPMSG_FUNC_RETURN( NULL );
}

/**
 * ディレクトリ送信。
 * @param sock TCPソケット
 * @param cd 今指しているディレクトリ名
 * @param dir 親ディレクトリのフルパス
 * @param files ファイル一覧
 */
bool
IpMessengerAgentImpl::SendDirData( int sock, std::string cd, std::string dir, std::vector<std::string> &files )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendDirData( int sock, std::string cd, std::string dir, std::vector<std::string> &files )");
	DIR *d= opendir( dir.c_str() );
	struct stat st;
	char headBuf[8192];

	if ( d == NULL ) {
		IPMSG_FUNC_RETURN( false );
	}

	stat( cd.c_str(), &st );
	int headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
														converter->ConvertLocalToNetwork( cd.c_str() ).c_str(),
														(unsigned long long)st.st_size,
														IPMSG_FILE_DIR,
														IPMSG_FILE_MTIME, (long)st.st_mtime,
														IPMSG_FILE_CREATETIME, (long)st.st_ctime );
	snprintf( headBuf, sizeof( headBuf ),"%04x", headBufLen );
	headBuf[4] = ':';
	send( sock, headBuf, headBufLen, 0 );

	char *buf = (char *)calloc( offsetof( struct dirent, d_name ) + pathconf( dir.c_str(), _PC_NAME_MAX ) + 1 , 1 );
	struct dirent *bufdent = ( struct dirent * )buf;
	struct dirent *dent = NULL;
	while( readdir_r( d, bufdent, &dent ) == 0 && dent != NULL ) {
		if ( strcmp(dent->d_name, "." ) != 0 && strcmp(dent->d_name, ".." ) != 0 ) {
			std::string dir_name = dir + "/" + dent->d_name;
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessengerAgentImpl::SendDirData dir[%s]", dir_name.c_str() );fflush( stdout );
#endif
			stat( dir_name.c_str(), &st );
			files.push_back( dir_name );
			if ( S_ISDIR( st.st_mode ) ){
#if defined(INFO) || !defined(NDEBUG)
				printf( "IpMessengerAgentImpl::SendDirData DIR\n" );fflush( stdout );
#endif
				if ( !SendDirData( sock, dent->d_name, dir_name, files ) ){
					closedir( d );
					free( buf );
					IPMSG_FUNC_RETURN( false );
				}
			} else {
#if defined(INFO) || !defined(NDEBUG)
				printf( "IpMessengerAgentImpl::SendDirData FILE\n" );fflush( stdout );
#endif
				int headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
																	converter->ConvertLocalToNetwork( dent->d_name ).c_str(),
																	(unsigned long long)st.st_size,
																	IPMSG_FILE_REGULAR,
																	IPMSG_FILE_MTIME, (long)st.st_mtime,
																	IPMSG_FILE_CREATETIME, (long)st.st_ctime );
				snprintf( headBuf, sizeof(headBuf),"%04x", headBufLen);
				headBuf[4] = ':';
				send( sock, headBuf, headBufLen, 0 );

				if ( !SendFile( sock, dir_name, st.st_mtime, st.st_size, NULL , 0 ) ){
					closedir( d );
					free( buf );
					IPMSG_FUNC_RETURN( false );
				}
			}
		}
	}
	headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:.:0:%lx:", IPMSG_FILE_RETPARENT );
	snprintf( headBuf, sizeof(headBuf),"%04x", headBufLen);
	headBuf[4] = ':';
	send( sock, headBuf, headBufLen, 0 );
	closedir( d );
	free( buf );
	IPMSG_FUNC_RETURN( true );
}

/**
 * ファイル送信。
 * @param sock TCPソケット
 * @param FileName ファイルのフルパス
 * @param offset オフセット
 * @retval true:成功
 * @retval false:失敗
 */
bool
IpMessengerAgentImpl::SendFile( int sock, std::string FileName, time_t mtime, unsigned long long size, AttachFile *file, off_t offset )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendFile( int sock, std::string FileName, time_t mtime, unsigned long long size, AttachFile *file, off_t offset )");
	struct stat statInit;
	unsigned long long transSize = 0LL;
	char realPathName[PATH_MAX];

	memset( realPathName, 0, sizeof( realPathName ) );
	if ( realpath( FileName.c_str(), realPathName ) == NULL ) {
		IPMSG_FUNC_RETURN( false );
	}
	int fd = open( realPathName, O_RDONLY );

	if ( file != NULL ) file->setTransSize( offset );

	if ( fd < 0 ) {
		perror( "open" );
#ifdef DEBUG
		printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\n", FileName.c_str() );fflush(stdout);
#endif
		IPMSG_FUNC_RETURN( false );
	}
	int rc = fstat( fd, &statInit );
	if ( rc != 0 ){
		close( fd );
		IPMSG_FUNC_RETURN( false );
	}
	lseek( fd, offset, SEEK_SET );
	int readSize;
	while( ( readSize = IpMsgSendFileBuffer( fd, sock, 8192 ) ) > 0 ){
		if ( AbortDownloadAtFileChanged() ){
			struct stat statProgress;
			if ( stat( realPathName, &statProgress ) != 0 ){
#ifdef DEBUG
				printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\nFile Changed.\n", FileName.c_str() );fflush(stdout);
#endif
				close( fd );
				IPMSG_FUNC_RETURN( false );
			}
			if ( IsFileChanged( mtime, size, statInit, statProgress ) ){
#ifdef DEBUG
				printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\nFile Changed.\n", FileName.c_str() );fflush(stdout);
#endif
				close( fd );
				IPMSG_FUNC_RETURN( false );
			}
#ifdef DEBUG
			printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\nFile Unchanged.\n", FileName.c_str() );fflush(stdout);
#endif
		}
		transSize += readSize;
		if ( file != NULL ) file->setTransSize( transSize );
	}
	close( fd );
	IPMSG_FUNC_RETURN( true );
}

/**
 * ファイルが更新されたか。
 * @param mtime 更新時刻
 * @param size ファイルサイズ
 * @param statInit ファイル属性初期状態
 * @param statProgress ファイル属性現在状態
 * @retval true:更新された
 * @retval false:更新されていない
 */
bool
IpMessengerAgentImpl::IsFileChanged( time_t mtime, unsigned long long size, struct stat statInit, struct stat statProgress )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::IsFileChanged( time_t mtime, unsigned long long size, struct stat statInit, struct stat statProgress )");
	IPMSG_FUNC_RETURN( mtime             != statProgress.st_mtime ||
		   statInit.st_ctime != statProgress.st_ctime ||
		   statInit.st_uid   != statProgress.st_uid   ||
		   statInit.st_gid   != statProgress.st_gid   ||
		   size              != (unsigned long long)statProgress.st_size );
}

/**
 * メッセージ受信時、パケットメッセージ本文の末尾の'\0'以降からファイル一覧情報を生成する。
 * @param option パケットオプション部
 * @param files 添付ファイルの一覧
 */
int
IpMessengerAgentImpl::CreateAttachedFileList( const char *option, AttachFileList &files )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateAttachedFileList( const char *option, AttachFileList &files )");
	files.clear();
	int filelist_startpos = strlen( option ) + 1;
	int alloc_size = strlen( &option[filelist_startpos] );
	if ( alloc_size == 0 ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	alloc_size++;

	char *file_list_tmp_ptr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *file_list_tmp_buf = (char *)calloc( alloc_size, 1 );
	if ( file_list_tmp_buf == NULL ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	memset( file_list_tmp_buf, 0, alloc_size );
	memcpy( file_list_tmp_buf,  &option[filelist_startpos] , alloc_size - 1 );
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateAttachedFileList File List Buffer = [%s]\n", file_list_tmp_buf);fflush( stdout );
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
			printf("IpMessengerAgentImpl::CreateAttachedFileList AttachFile(-1)\n" );fflush(stdout);
#endif
			// FILE ID
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileId( strtoul( token, &ptrdmy, 10 ) );
#if defined(DEBUG) || !defined(NDEBUG)
			printf( "IpMessengerAgentImpl::CreateAttachedFileList file.FileId() %d token [%s]\n", file.FileId(), token );fflush(stdout);
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
			printf("IpMessengerAgentImpl::CreateAttachedFileList == FILE  ==============================>\n");fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList FILE ID[%d]\n", file.FileId());fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList FILE NAME[%s]\n", file.FileName().c_str());fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList FILE SIZE[%lld]\n", file.FileSize());fflush( stdout );
			time_t tt = file.MTime();
			char dmybuf[100];
			printf("IpMessengerAgentImpl::CreateAttachedFileList MTIME[%s]\n", ctime_r( &tt, dmybuf ) );fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList ATTR[%lu]\n", file.Attr() );fflush( stdout );
			for( std::map<std::string, std::vector<unsigned long> >::iterator ixextattr = file.beginExtAttrs(); ixextattr != file.endExtAttrs(); ixextattr++){
				printf("IpMessengerAgentImpl::CreateAttachedFileList EXT ATTR[%s]==", ixextattr->first.c_str() );fflush( stdout );
				for( std::vector<unsigned long>::iterator ixextattrv = ixextattr->second.begin(); ixextattrv != ixextattr->second.end(); ixextattrv++){
					printf("[%lu]", *ixextattrv );fflush( stdout );
				}
				printf("\n" );fflush( stdout );
			}
			printf("IpMessengerAgentImpl::CreateAttachedFileList <= FILE  ===============================\n");fflush( stdout );
#endif
			// ADD FILELIST
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::CreateAttachedFileList AddFile()\n" );fflush( stdout );
#endif
			files.AddFile( file );
			break;
		}
		// FILE ID(not 1st)
		if ( token == NULL ){
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::CreateAttachedFileList File END,break;\n" );fflush( stdout );
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
	IPMSG_FUNC_RETURN( files.size() );
}

/**
 * ホストリスト受信時、パケットオプション部（バッファ）からホスト一覧情報を生成する。
 * @param hostListBuf バッファ
 * @param buf_len バッファの長さ
 */
int
IpMessengerAgentImpl::CreateHostList( const char *packetIpAddress, const char *packetHostName, const char *hostListBuf, int buf_len )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateHostList( const char *packetIpAddress, const char *packetHostName, const char *hostListBuf, int buf_len )");
	int alloc_size = buf_len + 1;
	int add_count = 0;
	char *hostListTmpPtr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *hostListTmpBuf = (char *)calloc( alloc_size, 1 );

#if defined(DEBUG) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateHostList packetIpAddress=[%s] packetHostName[%s]\n", packetIpAddress, packetHostName );fflush(stdout);
	IpMsgPrintBuf( "hostListBuf", hostListBuf, buf_len );
#endif
	AddDefaultHost();
	if ( hostListTmpBuf == NULL ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	memset( hostListTmpBuf, 0, alloc_size );
	memcpy( hostListTmpBuf, hostListBuf, buf_len );
	hostListTmpPtr = hostListTmpBuf;
	// CONTINUE POSITION
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );
	if ( token == NULL ) {
		free( hostListTmpBuf );
		IPMSG_FUNC_RETURN( 0 );
	}
	// LIST COUNTS
	hostListTmpPtr = nextpos;
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );
	if ( token == NULL ) {
		free( hostListTmpBuf );
		IPMSG_FUNC_RETURN( 0 );
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
		hostList.DeleteHostByAddress( item.IpAddress() );
		hostList.AddHost( item );
		appearanceHostList.DeleteHostByAddress( item.IpAddress() );
		appearanceHostList.AddHost( item, false );

#ifdef HAVE_OPENSSL
		struct sockaddr_storage addr;
		if ( createSockAddrIn( &addr, item.IpAddress(), item.PortNo() ) == NULL ) {
			IPMSG_FUNC_RETURN( add_count );
		}
		GetPubKey( addr );
#endif
		//(A)最後のトークンは最後に判定する。(A)
		if ( token == NULL ) break;
		add_count++;
	}
	free( hostListTmpBuf );
	IPMSG_FUNC_RETURN( add_count );
}

#ifdef HAVE_OPENSSL
void
IpMessengerAgentImpl::GetPubKey( const struct sockaddr_storage &address )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::GetPubKey( const struct sockaddr_storage &address )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	size_t optBufLen;
	optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx", encryptionCapacity );
	if ( optBufLen >= sizeof(optBuf) ){
		optBufLen = sizeof(optBuf);
	}
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETPUBKEY,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_GETPUBKEY, sendBuf, sendBufLen, address );
	IPMSG_FUNC_EXIT;
}
#endif

/**
 * 受信メッセージの個数を取得する。
 * @retval 受信メッセージの個数
 */
int
IpMessengerAgentImpl::GetRecievedMessageCount()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::GetRecievedMessageCount()");
	IPMSG_FUNC_RETURN( recvMsgList.size() );
}

/**
 * 受信メッセージを一個取り出し、受信メッセージリストから削除する。
 * @retval 受信メッセージオブジェクト。
 */
RecievedMessage
IpMessengerAgentImpl::PopRecievedMessage()
{
	IPMSG_FUNC_ENTER("RecievedMessage IpMessengerAgentImpl::PopRecievedMessage()");
	RecievedMessage ret;
	for( std::vector<RecievedMessage>::iterator ix = recvMsgList.begin(); ix != recvMsgList.end(); ix++ ){
		ret = *ix;
		recvMsgList.erase( ix );
		break;
	}
	IPMSG_FUNC_RETURN( ret );
}

/**
 * 送信済メッセージリストのポインタを取得する。
 * @retval 送信済メッセージリストのポインタ。
 */
SentMessageList *
IpMessengerAgentImpl::GetSentMessages()
{
	IPMSG_FUNC_ENTER("SentMessageList *IpMessengerAgentImpl::GetSentMessages()");
	IPMSG_FUNC_RETURN( &sentMsgList );
}

/**
 * 送信済メッセージリストのコピーを取得する。
 * @retval 送信済メッセージリストのコピー。
 */
SentMessageList
IpMessengerAgentImpl::CloneSentMessages() const
{
	IPMSG_FUNC_ENTER("SentMessageList IpMessengerAgentImpl::CloneSentMessages() const");
	IPMSG_FUNC_RETURN( sentMsgList );
}

/**
 * オプション部の最大長を取得する。
 * @retval オプション部が許容するバッファの長さ
 */
int
IpMessengerAgentImpl::GetMaxOptionBufferSize()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::GetMaxOptionBufferSize()");
	char tmp[MAX_UDPBUF];
	int headSize = snprintf(tmp, sizeof(tmp), "%d:0000000000:%s:%s:0000000000:", IPMSG_VERSION, _LoginName.c_str(), _HostName.c_str() );
	int ret = MAX_UDPBUF - headSize;
	IPMSG_FUNC_RETURN( ret < 0 ? 0 : ret );
}

/**
 * 共通コマンドオプションを追加する。
 * @param 追加前のコマンドオプション
 * @retval 追加後のコマンドオプション
 */
unsigned long
IpMessengerAgentImpl::AddCommonCommandOption( const unsigned long cmd )
{
	IPMSG_FUNC_ENTER("unsigned long IpMessengerAgentImpl::AddCommonCommandOption( const unsigned long cmd )");
	unsigned long ret = cmd | IPMSG_FILEATTACHOPT
#ifdef HAVE_OPENSSL
							| ( encryptionCapacity != 0UL ?  IPMSG_ENCRYPTOPT : 0UL )
#endif	//HAVE_OPENSSL
							| ( IsAbsence() ? IPMSG_ABSENCEOPT : 0UL ) | ( IsDialup() ? IPMSG_DIALUPOPT : 0UL );
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::AddCommonCommandOption <<=================================================\n");fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption encryptionCapacity=%lu\n", encryptionCapacity );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption IsAbsence=%s\n", IsAbsence() ? "true" : "false" );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption IsDialup=%s\n", IsDialup() ? "true" : "false" );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption Option=%lu\n", ret );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption =================================================>>\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( ret );
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
IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, unsigned long packetNo, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, unsigned long packetNo, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateNewPacketBuffer()\n" );fflush(stdout);
#endif
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::CreateNewPacketBuffer Packet Command[%s]\n", GetCommandString( GET_MODE( cmd ) ).c_str() );fflush( stdout );
#endif
	memset( buf, 0, size );
	//Version:PacketNo:UserName:HostName:Command[:Option]
	int send_size = snprintf(buf, size, "%d:%ld:%s:%s:%ld:",
										IPMSG_VERSION,
										packetNo,
										user == "" ? "\b" : user.c_str(),
										host == "" ? "\b" : host.c_str(),
										cmd );
	if ( send_size > size ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	if ( send_size + optLen < size && optLen > 0 && opt != NULL ) {
		memcpy(&buf[send_size], opt, optLen );
	} else {
		optLen = 0;
	}
	IPMSG_FUNC_RETURN( send_size + optLen );
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
IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateNewPacketBuffer()\n" );fflush(stdout);
#endif
	unsigned long packetNo = random();
	IPMSG_FUNC_RETURN( CreateNewPacketBuffer(cmd, packetNo, user, host, opt, optLen, buf, size ) );
}

/**
 * 受信バッファからパケットオブジェクトを生成する。
 * @param packet_buf 受信バッファ
 * @param size 受信バッファのサイズ
 * @param sender 送信元アドレス
 * @retval パケットオブジェクト
 */
Packet
IpMessengerAgentImpl::DismantlePacketBuffer( int sock, char *packet_buf, int size, struct sockaddr_storage sender, time_t nowTime )
{
	IPMSG_FUNC_ENTER("Packet IpMessengerAgentImpl::DismantlePacketBuffer( char *packet_buf, int size, struct sockaddr_storage sender, time_t nowTime )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::DismantlePacketBuffer()\n" );fflush(stdout);
#endif
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
		IPMSG_FUNC_RETURN( ret );
	}
	memset( packet_tmp_buf, 0, alloc_size );
	memcpy( packet_tmp_buf, packet_buf, size );
	//VERSION NUMBER
	packet_tmp_ptr = packet_tmp_buf;
	token = strtok_r( packet_tmp_buf, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setVersionNo( strtoul( token, &ptrdmy, 10 ) );

	//PACKET NUMBER
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setPacketNo( strtoul( token, &ptrdmy, 10 ) );

	//USER
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setUserName( token );

	//HOST
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setHostName( token );

	//COMMAND
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	unsigned long command = strtoul( token, &ptrdmy, 10 ); 
	ret.setCommandMode( GET_MODE(command) );
	ret.setCommandOption( GET_OPT(command) );

	//OPTION
	int optLen = size - ( nextpos - packet_tmp_buf );
	ret.setOption( std::string( nextpos, optLen ) );
	free( packet_tmp_buf );

	//NAT環境でsenderアドレスは信用できないので。。。
	//まずは見てくれのホストリストから検索する。
	std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByHostName( ret.HostName(), sd_address_family[sock] );
	struct sockaddr_storage hostaddr;
	if ( hostIt != appearanceHostList.end() ) {
		if ( createSockAddrIn( &hostaddr, hostIt->IpAddress(), hostIt->PortNo() ) == NULL ) {
			IPMSG_FUNC_RETURN( ret );
		}
	} else {
		//無ければ実際ののホストリストから検索する。
		hostIt = hostList.FindHostByHostName( ret.HostName(), sd_address_family[sock] );
		if ( hostIt != hostList.end() ) {
			if ( createSockAddrIn( &hostaddr, hostIt->IpAddress(), hostIt->PortNo() ) == NULL ) {
				IPMSG_FUNC_RETURN( ret );
			}
		} else {
			hostaddr = sender;
#if 0
			if ( createSockAddrIn( &hostaddr, getSockAddrInRawAddress( sender ), ntohs( getSockAddrInPortNo( sender ) ) ) == NULL ) {
				IPMSG_FUNC_RETURN( ret );
			}
#endif
		}
	}
	ret.setAddr( hostaddr );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * パケットからホストリストに加える。
 * @param packet パケットオブジェクト
 * @retval 登録した件数。
 */
int
IpMessengerAgentImpl::AddHostListFromPacket( const Packet& packet )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddHostListFromPacket( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddHostListFromPacket()\n" );fflush(stdout);
#endif
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddHostListFromPacket ===================================\n");fflush( stdout );
	struct sockaddr_storage tempAddr = packet.Addr();
	IpMsgDumpPacket( packet, &tempAddr );
	printf("IpMessengerAgentImpl::AddHostListFromPacket ===================================\n");fflush( stdout );
#endif
	AddDefaultHost();
	// デフォルトのNIC(０番目)以外の自分自身のIPアドレスが登録依頼されたら無視。
	std::string packetIpAddress = getSockAddrInRawAddress( packet.Addr() );
	for( unsigned int i = 1; i < NICs.size(); i++ ){
		if ( packetIpAddress == NICs[i].IpAddress() ){
			AddDefaultHost();
			IPMSG_FUNC_RETURN( 0 );
		}
	}
	//デフォルトカード
	HostListItem item;
	item.setUserName( packet.UserName() );
	item.setHostName( packet.HostName() );
	item.setCommandNo( packet.CommandOption() );
	item.setIpAddress( getSockAddrInRawAddress( packet.Addr() ) );
	int NicknameLen = strlen( packet.Option().c_str() );
	item.setNickname( packet.Option().c_str() );
	item.setGroupName( packet.Option().c_str() + NicknameLen + 1 );
	item.setEncodingName( "" );
	item.setPriority( "" );
	item.setPortNo( ntohs( getSockAddrInPortNo( packet.Addr() ) ) );
	item.setEncryptionCapacity( 0UL );
	item.setPubKeyHex( "" );
	item.setEncryptMethodHex( "" );
	hostList.AddHost( item );
	int ret = appearanceHostList.AddHost( item, false );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * 念のためホストリストに自分を加えておく。
 * @retval 登録したホスト数。
 */
int
IpMessengerAgentImpl::AddDefaultHost()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::AddDefaultHost()");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddDefaultHost()\n" );fflush(stdout);
	printf("IpMessengerAgentImpl::AddDefaultHost Nickname=[%s]\n", Nickname.c_str() );fflush(stdout);
	printf("IpMessengerAgentImpl::AddDefaultHost GroupName=[%s]\n", GroupName.c_str() );fflush(stdout);
#endif
	std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( HostAddress );
	if ( hostIt == appearanceHostList.end() ) {
		HostListItem myHost;
		myHost.setUserName( _LoginName );
		myHost.setHostName( _HostName );
		myHost.setCommandNo( AddCommonCommandOption( 0UL ) );
		myHost.setIpAddress( HostAddress );
		myHost.setNickname( Nickname );
		myHost.setGroupName( GroupName );
		myHost.setPortNo( DefaultPortNo() );
		hostList.AddHost( myHost );
		appearanceHostList.AddHost( myHost, false );
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::AddDefaultHost MyHost[%s] Add.[%s][%s]\n", HostAddress.c_str(), myHost.UserName().c_str(), myHost.GroupName().c_str() );fflush( stdout );
#endif
		IPMSG_FUNC_RETURN( 1 );
	}
	IPMSG_FUNC_RETURN( 0 );
}
//end of source
