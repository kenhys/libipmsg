/**
 * IP メッセンジャライブラリ(Unix用)
 * IPメッセンジャエージェントクラス。
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define WINCOMPAT

#ifdef HAVE_OPENSSL
#ifdef WINCOMPAT
#define SUPPORT_RSA_512
#define SUPPORT_RSA_1024
#define SUPPORT_RC2_40
#define SUPPORT_BLOWFISH_128
#endif	// WINCOMPAT
#endif	// HAVE_OPENSSL

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif	// HAVE_PTHREAD
#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#endif	// HAVE_OPENSSL
using namespace std;

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

//NICの最大数
#define IFR_MAX 20

//メッセージ送信リトライ最大数
#define SENDMSG_RETRY_MAX	5
//ホストリスト取得リトライ最大数
#define GETLIST_RETRY_MAX	2
//一回のホストリスト取得最大数
#define HOST_LIST_SEND_MAX_AT_ONCE	100
//パケットのデリミタ文字
#define	PACKET_DELIMITER_CHAR	':'
//パケットのデリミタ文字列
#define	PACKET_DELIMITER_STRING	":"
//オプション部の項目区切り文字
#define	PACKET_FIELD_SEPERATOR_CHAR	'\a'
//バージョン文字列
#define	IPMSG_AGENT_VERSION		"IpMessengerAgent for C++ Unix Version 0.1.1"

//暗号化キー(RSA)のビット数(最弱)
#define RSA_KEY_LENGTH_MINIMUM	512
//暗号化キー(RSA)のビット数(まぁまぁ)
#define RSA_KEY_LENGTH_MIDIUM	1024
//暗号化キー(RSA)のビット数(最強)
#define RSA_KEY_LENGTH_MAXIMUM	2048

//RSAキー生成時に使用する素数
#define ENCRYPT_PRIME			65537

static IpMessengerAgentImpl *myInstance = NULL;

void *GetFileDataThread( void *param );
void *GetDirFilesThread( void *param );

#ifdef HAVE_PTHREAD
static pthread_mutex_t instanceMutex;
static int mutex_init_result = pthread_mutex_init( &instanceMutex, NULL );
#endif

class IpMessengerNullEvent: public IpMessengerEvent {
	public:
		virtual void UpdateHostListAfter( HostList& hostList ){ printf("UpdateHostListAfter\n"); };
		virtual void GetHostListRetryError(){ printf("GetHostListRetryError\n"); };
		virtual bool RecieveAfter( RecievedMessage& msg ){ printf("RecieveAfter\n");return false; };
		virtual void SendAfter( SentMessage& msg ){ printf("SendAfter\n"); };
		virtual void SendRetryError( SentMessage& msg ){ printf("SendRetryError\n"); };
		virtual void OpenAfter( SentMessage& msg ){ printf("OpenAfter\n"); };
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadStart\n"); };
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadProcessing\n"); };
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadEnd\n"); };
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadError\n"); };
		virtual void EntryAfter( HostList& hostList ){ printf("EntryAfter\n"); };
		virtual void ExitAfter( HostList& hostList ){ printf("ExitAfter\n"); };
};

/**
 * IP メッセンジャエージェントクラスのインスタンスを取得する。
 * Singletonパターンを採用しているので、ホスト唯一のインスタンスでなければならない。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgentImpl *
IpMessengerAgentImpl::GetInstance()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif	// HAVE_PTHREAD
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgentImpl();
	}
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif	// HAVE_PTHREAD
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
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif	// HAVE_PTHREAD
	if ( myInstance == NULL ) {
#ifdef HAVE_PTHREAD
		pthread_mutex_unlock( &instanceMutex );
#endif	// HAVE_PTHREAD
		return;
	}
	delete myInstance;
	myInstance = NULL;
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif	// HAVE_PTHREAD
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
	setAbortDownloadAtFileChanged( false );
	setSaveSentMessage( true );
	setSaveRecievedMessage( true );
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( NICs );
	NetworkInit();
	ResetAbsence();
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
	Logout();
	CryptoEnd();
	delete converter;
	delete event;
	NetworkEnd();
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * ・まず、ログアウト。
 * ・ネットワーク終期化。
 * ・ネットワーク初期化。
 * ・再度ログイン。
 * 注：このインスタンスはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::RestartNetwork()
{
	Logout();
	NetworkEnd();
	NetworkInit();
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
 * @param conv イベントオブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
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
 * 暗号関連の初期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::CryptoInit()
{
#ifdef HAVE_OPENSSL
	ERR_load_crypto_strings();
	char errbuf[1024];

	encryptionCapacity = 0UL;
	RsaMax = NULL;
#ifdef SUPPORT_RSA_2048
	RsaMax = RSA_generate_key( RSA_KEY_LENGTH_MAXIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMax == NULL ) {
		printf("in encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );	
	} else {
		encryptionCapacity |= IPMSG_RSA_2048;
		printf("encryption extention enabled.(RSA2048)\n");
	}
#endif	//SUPPORT_RSA_2048
	RsaMid = NULL;
#ifdef SUPPORT_RSA_1024
	RsaMid = RSA_generate_key( RSA_KEY_LENGTH_MIDIUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMid == NULL ) {
		printf("in encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );	
	} else {
		encryptionCapacity |= IPMSG_RSA_1024;
		printf("encryption extention enabled.(RSA1024)\n");
	}
#endif	//SUPPORT_RSA_1024
	RsaMin = NULL;
#ifdef SUPPORT_RSA_512
	RsaMin = RSA_generate_key( RSA_KEY_LENGTH_MINIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMin == NULL ) {
		printf("in encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );	
	} else {
		encryptionCapacity |= IPMSG_RSA_512;
		printf("encryption extention enabled.(RSA512)\n");
	}
#endif	//SUPPORT_RSA_512
	if ( encryptionCapacity == 0UL ) {
		//暗号化無効
		printf("encryption extention disabled.\n");
	}
#ifdef SUPPORT_RC2_40
	encryptionCapacity |= IPMSG_RC2_40;
#endif	//SUPPORT_RC2_40
#ifdef SUPPORT_RC2_128
	encryptionCapacity |= IPMSG_RC2_128;
#endif	//SUPPORT_RC2_128

#ifdef SUPPORT_RC2_256
	encryptionCapacity |= IPMSG_RC2_256;
#endif	//SUPPORT_RC2_256

#ifdef SUPPORT_BLOWFISH_128
	encryptionCapacity |= IPMSG_BLOWFISH_128;
#endif	//SUPPORT_BLOWFISH_128

#ifdef SUPPORT_BLOWFISH_256
	encryptionCapacity |= IPMSG_BLOWFISH_256;
#endif	//SUPPORT_BLOWFISH_256
#endif	//HAVE_OPENSSL
}

/**
 * 暗号関連の終期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::CryptoEnd()
{
#ifdef HAVE_OPENSSL
	if ( RsaMin != NULL ) {
		RSA_free( RsaMin );
	}
	if ( RsaMid != NULL ) {
		RSA_free( RsaMid );
	}
	if ( RsaMax != NULL ) {
		RSA_free( RsaMax );
	}
	ERR_free_strings();
#endif	//HAVE_OPENSSL
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
	struct ifconf ifc;

	struct ifreq ifr[IFR_MAX];
	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_ifcu.ifcu_buf = (char *)ifr;

	ioctl(fd, SIOCGIFCONF, &ifc);
	int nifs = ifc.ifc_len / sizeof(struct ifreq);

	/* ローカルループバックをのぞく全てのIPアドレスが対象 */
	/* 全てのNICを取得する */
	for( int i = 0; i < nifs; i++ ){
		if ( strcmp("127.0.0.1", inet_ntoa( ( (struct sockaddr_in *)&ifr[i].ifr_addr )->sin_addr ) ) == 0 ){
			continue;
		}
#if defined(DEBUG) || !defined(NDEBUG)
		printf( "dev=%s,ipaddress=%s\n", ifr[i].ifr_name, inet_ntoa( ( (struct sockaddr_in *)&ifr[i].ifr_addr )->sin_addr ) );
#endif
		NetworkInterface ni;
		ni.setDeviceName( ifr[i].ifr_name );
		ni.setPortNo( IPMSG_DEFAULT_PORT );
		ni.setIpAddress( inet_ntoa( ( (struct sockaddr_in *)&ifr[i].ifr_addr )->sin_addr ) );
		nics.push_back( ni );
#if defined(DEBUG) || !defined(NDEBUG)
		printf( "NIC device=%s[IpAddress=%s][Port=%d]\n",nics[nics.size() - 1].DeviceName().c_str(),nics[nics.size()-1].IpAddress().c_str(), nics[nics.size()-1].PortNo() );
		fflush(stdout);
#endif
	}

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
IpMessengerAgentImpl::NetworkInit()
{
	char buf[100];
	char *env;

	env = getenv( "HOSTNAME" );
	if ( env == NULL ){
		_HostName = "localhost";
	} else {
		_HostName = env;
	}
	env = getenv( "USERNAME" );
	if ( env == NULL ){
		_LoginName = snprintf( buf, sizeof( buf ), "%d", getuid() );
	} else {
		_LoginName = env;
	}

#ifdef HAVE_OPENSSL
	DecryptErrorMessage = "\r\n"\
						  " ==== AutoReply(DecryptErr) ====\r\n" \
						  "  My PubKey is updated, I can't\r\n" \
						  "  receive your message.\r\n" \
						  "  Please press refresh button.\r\n" \
						  " ==============================";
#endif	//HAVE_OPENSSL
	InitSend();
	InitRecv( NICs );
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
	
	IpMsgPrintBuf( "Login:sendBuf", sendBuf, MAX_UDPBUF );
#ifdef HAVE_OPENSSL
	if ( encryptionCapacity != 0UL ) {
		sendBufLen = CreateNewPacketBuffer( IPMSG_BR_ENTRY | IPMSG_FILEATTACHOPT | IPMSG_ENCRYPTOPT,
											  _LoginName, _HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
	} else {
#endif	//HAVE_OPENSSL
		sendBufLen = CreateNewPacketBuffer( IPMSG_BR_ENTRY | IPMSG_FILEATTACHOPT,
											  _LoginName, _HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
#ifdef HAVE_OPENSSL
	}
#endif	//HAVE_OPENSSL
	SendBroadcast( IPMSG_BR_ENTRY, sendBuf, sendBufLen );
	RecvPacket();
	//0.05秒まつ。
	usleep( 50000L );
	RecvPacket();

//	UpdateHostList();
//	RecvPacket();
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

	sendBufLen = CreateNewPacketBuffer( IPMSG_BR_EXIT,
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
IpMessengerAgentImpl::UpdateHostList()
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	HostList backup = hostList;

	hostList.clear();
	AddDefaultHost();

	sendBufLen = CreateNewPacketBuffer( IPMSG_BR_ISGETLIST2,
										  _LoginName, _HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ISGETLIST2, sendBuf, sendBufLen );
	int pcount = RecvPacket();
	//自分以外のホストが見付からないか５回リトライする間繰り返す
	for( int i = 0; i < 5; i++ ) {
		//0.01秒まつ。
		usleep( 10000L );
		pcount = RecvPacket();
	}

	//Error??? TODO
	//if ( event != NULL ) {
	//	event->GetHostListRetryError();
	//	hostList = backup;
	//	return hostList;
	//}
#if defined(DEBUG)
	printf("\n\n");
	printf("== M Y   H O S T L I S T ==============================>\n");
	vector<HostListItem>::iterator ix = hostList.begin();
	for( ; ix != hostList.end(); ix++ ){
		printf( "Version[%s]\n" \
				"AbsenceDescription[%s]\n" \
				"User[%s]\n" \
				"Host[%s]\n" \
				"CommandNo[%lu]\n" \
				"IpAddress[%s]\n" \
				"NickName[%s]\n" \
				"Group[%s]\n" \
				"Encoding[%s]\n" \
				"EncryptionCapacity[%lu]\n" \
				"PubKeyHex[%s]\n" \
				"EncryptMethodHex[%s]\n" \
				"PortNo[%lu]\n" \
				"##########################################################\n",
				ix->Version().c_str(),
				ix->AbsenceDescription().c_str(),
				ix->UserName().c_str(),
				ix->HostName().c_str(),
				ix->CommandNo(),
				ix->IpAddress().c_str(),
				ix->Nickname().c_str(),
				ix->GroupName().c_str(),
				ix->EncodingName().c_str(),
				ix->EncryptionCapacity(),
				ix->PubKeyHex().c_str(),
				ix->EncryptMethodHex().c_str(),
				ix->PortNo() );
	}
	printf("<= M Y   H O S T L I S T ===============================\n");
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
IpMessengerAgentImpl::SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	AttachFileList files;
	return SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, opt );
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
IpMessengerAgentImpl::SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	AttachFileList files;
	files.AddFile( file );
	return SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, opt );
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
IpMessengerAgentImpl::SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
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
	optBuf[optBufLen++] = '\0';
	IpMsgPrintBuf( "optBuf:", optBuf, optBufLen );

	for( vector<AttachFile>::iterator ixfile = files.begin(); ixfile != files.end(); ixfile++ ) {
		ixfile->GetLocalFileInfo();
		string filename = converter->ConvertLocalToNetwork( ixfile->FileName() );
		int wsize = snprintf( &optBuf[ optBufLen ], sizeof( optBuf ) - optBufLen - 1,
							"%d:%s:%llx:%lx:%lx\a",
							ixfile->FileId(), filename.c_str(), ixfile->FileSize(), ixfile->MTime(), ixfile->Attr() );
		optBufLen += wsize;
		optBuf[optBufLen] = '\0';
	}
	optBufLen++;
	optBuf[optBufLen ] = '\0';


	long packetNo = random();

	sendBufLen = CreateNewPacketBuffer( IPMSG_SENDMSG | IPMSG_SENDCHECKOPT |
#ifdef HAVE_OPENSSL
										  ( isEncrypted ? IPMSG_ENCRYPTOPT : 0UL ) |
#endif	//HAVE_OPENSSL
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
	message.setIsConfirmed( false );
	message.setIsPasswordLock( isLockPassword );
	message.setIsCrypted( isEncrypted );
	message.setIsConfirmAnswered( false );
	message.setIsSecret( isSecret );
	message.setFiles( files );

#if defined(DEBUG)
	printf( "UserName[%s]\n", message.Host().UserName().c_str() );
	printf( "HostName[%s]\n", message.Host().HostName().c_str() );
	printf( "Nickname[%s]\n", message.Host().Nickname().c_str() );
#endif
	if ( SaveSentMessage() ){
		sentMsgList.append( message );
	}

	RecvPacket();

	if ( event != NULL ){
		event->SendAfter( message );
	}

#if defined(DEBUG)
	printf("sentMsgList.append() size=%d\n", sentMsgList.size() );
	fflush(stdout);
#endif

	return message;
}

/**
 * メッセージ暗号化。
 * @param host 送信先ホスト
 * @param optBuf パケットオプション部のバッファのアドレス
 * @param optBufLen パケットオプション部のバッファの現在の有効データ長
 * @param enc_optBufLen 暗号化済のパケットオプション部のバッファの有効データ長のアドレス
 * @param opt_size パケットオプション部のバッファのサイズ
 * @retval true:暗号化OK、false:暗号化NG
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgentImpl::EncryptMsg( HostListItem host, unsigned char *optBuf, int optBufLen, int *enc_optBufLen, int opt_size )
{
#ifdef HAVE_OPENSSL
	unsigned long pubKeyMethod = 0UL;
	unsigned char iv[EVP_MAX_IV_LENGTH];
	char errbuf[200];

	//EVPのSeal系の公開鍵暗号の暗号化APIは使いにくいので、自分でEncrypt系、RSA系のAPIで実装します。
#ifndef WINCOMPAT
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_2048 && host.EncryptionCapacity() & IPMSG_RSA_2048 ) {
		pubKeyMethod = IPMSG_RSA_2048;
	}
#endif	//SUPPORT_RSA_2048
#endif	//WINCOMPAT

#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_1024 && host.EncryptionCapacity() & IPMSG_RSA_1024 ) {
		pubKeyMethod = IPMSG_RSA_1024;
	}
#endif	//SUPPORT_RSA_1024

#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_512  && host.EncryptionCapacity() & IPMSG_RSA_512 ) {
		pubKeyMethod = IPMSG_RSA_512;
	}
#endif	//SUPPORT_RSA_512
	//暗号化出来ないので、平文で送信。
	if ( pubKeyMethod == 0UL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("encryptionCapacity(%lx)\n", encryptionCapacity );
		printf("host.EncryptionCapacity()(%lx)\n", host.EncryptionCapacity() );
		printf("pubKeyMethod == 0UL\n");
#endif
		return false;
	}

	RSA *rsa = RSA_new();
	rsa->e = BN_new();
	if ( BN_hex2bn( &rsa->e, host.EncryptMethodHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		printf( "BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));
#endif
		RSA_free( rsa );
		return false;
	}
	rsa->n = BN_new();
	if ( BN_hex2bn( &rsa->n, host.PubKeyHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		printf( "BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));
#endif
		RSA_free( rsa );
		return false;
	}

	memset( iv, 0, sizeof( iv ) );
	
	unsigned char sharekey[EVP_MAX_KEY_LENGTH];
	int key_bytes_size = 0;
	unsigned long shareKeyMethod = 0UL;
#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_256 && host.EncryptionCapacity() & IPMSG_BLOWFISH_256 ) {
		shareKeyMethod = IPMSG_BLOWFISH_256;
		key_bytes_size = 256/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT

#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_128 && host.EncryptionCapacity() & IPMSG_BLOWFISH_128 && pubKeyMethod == IPMSG_RSA_1024 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_128 && host.EncryptionCapacity() & IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		shareKeyMethod = IPMSG_BLOWFISH_128;
		key_bytes_size = 128/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_256      && host.EncryptionCapacity() & IPMSG_RC2_256 ) {
		shareKeyMethod = IPMSG_RC2_256;
		key_bytes_size = 256/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_128      && host.EncryptionCapacity() & IPMSG_RC2_128 ) {
		shareKeyMethod = IPMSG_RC2_128;
		key_bytes_size = 128/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_40       && host.EncryptionCapacity() & IPMSG_RC2_40 && pubKeyMethod == IPMSG_RSA_512 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_40       && host.EncryptionCapacity() & IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		shareKeyMethod = IPMSG_RC2_40;
		key_bytes_size = 40/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_40
	//暗号化出来ないので、平文で送信。
	if ( shareKeyMethod == 0UL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("shareKeyMethod == 0UL\n");
#endif
		RSA_free( rsa );
		return false;
	}
	int enc_key_size = RSA_size( rsa );
	unsigned char *enc_key = (unsigned char *)calloc( enc_key_size + 1, 1 );
#if defined(INFO) || !defined(NDEBUG)
	printf( "enc_key_size(%d)\n", enc_key_size );
#endif
	if ( enc_key == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("enc_key == NULL\n");
#endif
		RSA_free( rsa );
		return false;
	}
	//共通鍵をRSA公開鍵で暗号化。
	int enc_key_len = RSA_public_encrypt( key_bytes_size, sharekey, enc_key, rsa, RSA_PKCS1_PADDING );
	if ( enc_key_len < 0 ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("enc_key_len < 0\n");
#endif
		RSA_free( rsa );
		free( enc_key );
		return false;
	}
	//共通鍵で本文を暗号化。
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init( &ctx );
	int seal_init_ret = 0;
#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 && pubKeyMethod == IPMSG_RSA_512 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_40_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_128
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 && pubKeyMethod == IPMSG_RSA_1024 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_bf_cbc(), NULL, NULL );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BLOWFISH_256 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_bf_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	char *enc_buf = (char *)calloc( optBufLen + key_bytes_size + 1, 1 );
	if ( enc_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("enc_buf == NULL\n");
#endif
		RSA_free( rsa );
		free( enc_key );
		return false;
	}
	int ol;
	int o_len = 0;
	int ret;
	// バッファが終わるまで繰り返す。
	for( int i = 0; i < optBufLen / key_bytes_size; i++ ){
		ret = EVP_EncryptUpdate( &ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], key_bytes_size );
		o_len += ol;
	}
	if( optBufLen % key_bytes_size != 0 ){
		ret = EVP_EncryptUpdate( &ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], optBufLen % key_bytes_size );
		o_len += ol;
	}
	ret = EVP_EncryptFinal( &ctx, (unsigned char*)&enc_buf[o_len], &ol );
	o_len += ol;

	int ob_len = 8 + 1 + ( enc_key_len * 2 ) + 1 + ( o_len * 2 ) + 1;
	char *out_buf = (char *)calloc( ob_len + 1, 1 );
	if ( out_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("out_buf == NULL\n");
#endif
		RSA_free( rsa );
		free( enc_key );
		free( enc_buf );
		return false;
	}
	snprintf( (char *)out_buf, ob_len, "%lx:", pubKeyMethod | shareKeyMethod );
	for( int i = 0; i < enc_key_len; i++ ) {
		char pout_hex[3];
		snprintf( pout_hex, 3, "%02x", (unsigned char)enc_key[i] );
		strcat( (char *)out_buf, pout_hex );
	}
	strcat( (char *)out_buf, PACKET_DELIMITER_STRING );
	for( int i = 0; i < o_len; i++ ) {
		char pout_hex[3];
		snprintf( pout_hex, 3, "%02x", (unsigned char)enc_buf[i] );
		strcat( (char *)out_buf, pout_hex );
	}
	*enc_optBufLen = strlen( (char *)out_buf );
	if ( opt_size > *enc_optBufLen ) {
		memset( optBuf, 0, *enc_optBufLen + 1 );
		memcpy( optBuf, out_buf, *enc_optBufLen );
	}
	RSA_free( rsa );
	free( enc_key );
	free( enc_buf );
	free( out_buf );
	if ( opt_size > *enc_optBufLen ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("TRUE!!\n");
#endif
		return true;
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("FALSE!!\n");
#endif
	return false;
#else	//HAVE_OPENSSL
	return false;
#endif	//HAVE_OPENSSL
}

/**
 * メッセージ復号化。
 * @param packet パケットオブジェクト（参照）
 * @retval true:復号化OK、false:復号化NG
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgentImpl::DecryptMsg( Packet &packet )
{
#ifdef HAVE_OPENSSL
	EVP_CIPHER_CTX ctx;
	unsigned char iv[EVP_MAX_IV_LENGTH];

	char *buf = (char *)calloc( packet.Option().size() + 1, 1);
	if ( buf == NULL ){
		return false;
	}
	memcpy( buf, packet.Option().c_str(), packet.Option().size());
	char *file_ptr = &buf[strlen( buf ) + 1];
	char *file_info = (char *)calloc( packet.Option().size(), 1 );
	int file_info_len = strlen( file_ptr );
	if ( file_info == NULL ) {
		free( buf );
		return false;
	}
	memcpy( file_info, file_ptr, file_info_len );
	IpMsgPrintBuf("file_ptr:", file_ptr, file_info_len);
	IpMsgPrintBuf("file_info:", file_info, file_info_len);

	char *token = buf;
	char *nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		return false;
	}
	char *dmyptr;
	long methods = strtoul( token, &dmyptr, 16 );

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		return false;
	}
	string ekey = token;

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		return false;
	}
	string emsg = token;

	string esign = "";
	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token != NULL ) {
		esign = token;
	}
	free( buf );
	RSA *rsa = NULL;
	int rsa_bits = 0;
	unsigned long pubKeyMethod = 0UL;
#ifndef WINCOMPAT
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_2048 ) {
		pubKeyMethod = IPMSG_RSA_2048;
		rsa_bits = 2048/8;
		rsa = RsaMax;
	}
#endif	//SUPPORT_RSA_2048
#endif	//WINCOMPAT

#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_1024 ) {
		pubKeyMethod = IPMSG_RSA_1024;
		rsa_bits = 1024/8;
		rsa = RsaMid;
	}
#endif	//SUPPORT_RSA_1024

#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_512 ) {
		pubKeyMethod = IPMSG_RSA_512;
		rsa_bits = 512/8;
		rsa = RsaMin;
	}
#endif	//SUPPORT_RSA_512
	//暗号化されていない？
	if ( pubKeyMethod == 0UL ) {
		return false;
	}
	//パディングを含むサイズ
	int ekey_len = ekey.length() / 2;
	if ( ekey_len % rsa_bits > 0 ) {
		ekey_len = ( ( ekey.length() / 2 ) / rsa_bits ) * ( rsa_bits + 1 );
	}
	unsigned char *ek = (unsigned char *)calloc( ekey_len + 1, 1 );
	if ( ek == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("calloc 1\n");
#endif
		perror("calloc");
		return false;
	}
	unsigned char *ekp = ek;
	for( unsigned int i = 0; i < ekey.length(); i += 2 ) {
		unsigned char ekc [3];
		ekc[0] = ekey.at( i );
		ekc[1] = ekey.at( i + 1 );
		ekc[2] = '\0';
		*ekp = (unsigned char)strtoul( (char *)ekc, &dmyptr, 16 );
		ekp++;
	}
	int ekl = ekey_len;
	unsigned long shareKeyMethod = 0UL;
	int key_bytes_size = 0;
#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_512 && shareKeyMethod == 0UL && methods & IPMSG_RC2_40 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		key_bytes_size = 40/8;
		shareKeyMethod = IPMSG_RC2_40;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_RC2_40\n");
#endif
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_128 ) {
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_RC2_128;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_RC2_128\n");
#endif
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_256 ) {
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_RC2_256;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_RC2_256\n");
#endif
	}
#endif	//SUPPORT_RC2_256

#endif	//WINCOMPAT
#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_1024 && shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_128 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_BLOWFISH_128;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_BF_128\n");
#endif
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_256 ){
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_BLOWFISH_256;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_BF_256\n");
#endif
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	//暗号化されていない？
	if ( shareKeyMethod == 0UL ) {
		free( file_info );
		free( ek );
		return false;
	}
	unsigned char *emsg_buf = (unsigned char *)calloc( emsg.length() + 1, 1 );
	if ( emsg_buf == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("calloc 2\n");
#endif
		perror("calloc");
		free( file_info );
		free( ek );
		return false;
	}
	int data_len = 0;
	for( unsigned int i = 0; i < emsg.length(); i += 2 ) {
		unsigned char emc [3];
		emc[0] = emsg.at( i );
		emc[1] = emsg.at( i + 1 );
		emc[2] = '\0';
#if defined(INFO) || !defined(NDEBUG)
		printf("%d:emc=[%s]", data_len, emc);
#endif
		emsg_buf[data_len] = (unsigned char)strtoul( (char *)emc, &dmyptr, 16 );
#if defined(INFO) || !defined(NDEBUG)
		printf("[%02x]\n", emsg_buf[data_len]);
#endif
		data_len++;
	}

	EVP_PKEY pubkey;
	EVP_PKEY_set1_RSA( &pubkey, rsa );
	int open_init_ret = 0;
	memset( iv, 0, sizeof( iv ) );
#ifdef SUPPORT_RC2_40
#ifndef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_512 && shareKeyMethod == IPMSG_RC2_40 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_40_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_64_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_64_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_1024 && shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( &ctx, EVP_bf_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BROWFISH_256 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_bf_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	if ( open_init_ret <= 0 ){
		free( file_info );
		free( ek );
		free( emsg_buf );
		return false;
	}
	int tmp_len = 0;
	int tmp;
	unsigned char *optBuf = (unsigned char *)calloc( data_len + key_bytes_size + 1 + file_info_len + 1, 1 );
	if ( optBuf == NULL ){
		perror("calloc");
		free( file_info );
		free( ek );
		free( emsg_buf );
		return false;
	}

	int ret;
	ret = EVP_OpenUpdate( &ctx, &optBuf[tmp_len], &tmp, &emsg_buf[tmp_len], data_len );
	tmp_len += tmp;
	ret = EVP_OpenFinal( &ctx, &optBuf[tmp_len], &tmp );
	tmp_len += tmp;
	free( ek );
	free( emsg_buf );
	
	if ( file_info_len > 0 ){
		IpMsgPrintBuf( "optBuf(1):", (char *)optBuf, tmp_len );
		IpMsgPrintBuf( "file_info:", (char *)file_info, file_info_len );
		memcpy( &optBuf[tmp_len+1], file_info, file_info_len );
		tmp_len += ( file_info_len + 1 );
	}
	packet.setOption( string( (char *)optBuf, tmp_len ) );
	IpMsgPrintBuf( "optBuf(2):", (char *)optBuf, tmp_len );
	free( optBuf );
	free( file_info );
	return true;
#else	//HAVE_OPENSSL
	return false;
#endif	//HAVE_OPENSSL
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
		printf( "Delete Broadcast Address from %s(%d)\n", inet_ntoa( net->sin_addr ), ntohs( net->sin_port ) );
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
	vector<struct sockaddr_in>::iterator net = FindBroadcastNetworkByAddress( addr );
	if ( net != broadcastAddr.end() ) {
		return;
	}
	struct sockaddr_in add_addr;
	add_addr.sin_family = AF_INET;
	add_addr.sin_port = htons(IPMSG_DEFAULT_PORT);
	add_addr.sin_addr.s_addr = inet_addr( addr.c_str() );
#if defined(DEBUG)
	printf( "Add Broadcast Address To %s(%d)\n", inet_ntoa( add_addr.sin_addr ), ntohs( add_addr.sin_port ) );
#endif
	broadcastAddr.push_back( add_addr );
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
 * ホストリストをホスト名で検索し、該当するHostListItemを返却する。
 * @param hostName ホスト名
 * @retval HostListItem
 * 注：このメソッドはスレッドセーフでない。
 */
vector<HostListItem>::iterator
IpMessengerAgentImpl::FindHostByHostName( string hostName )
{
	for( vector<HostListItem>::iterator ix = hostList.begin(); ix < hostList.end(); ix++ ){
		if ( ix->HostName() == hostName ) {
			return ix;
		}
	}
	return hostList.end();
}

/**
 * ホストリストをIPアドレスで検索し、該当するHostListItemを返却する。
 * @param addr IPアドレス文字列
 * @retval HostListItem
 * 注：このメソッドはスレッドセーフでない。
 */
vector<HostListItem>::iterator
IpMessengerAgentImpl::FindHostByAddress( string addr )
{
	for( vector<HostListItem>::iterator ix = hostList.begin(); ix < hostList.end(); ix++ ){
#if defined(DEBUG)
printf("HOST CHECK IpAddress=%s addr=%s\n", ix->IpAddress().c_str(), addr.c_str() );
#endif
		if ( ix->IpAddress() == addr ) {
#if defined(DEBUG)
printf("★★★★★★★★★★★★\n");
printf("HOST FOUND!!!\n");
printf("★★★★★★★★★★★★\n");
#endif
			return ix;
		}
	}
#if defined(DEBUG)
printf("★★★★★★★★★★★★\n");
printf("HOST NOT FOUND!!!\n");
printf("★★★★★★★★★★★★\n");
#endif
	return hostList.end();
}

/**
 * 対象ホストのバージョン情報を取得。
 * ・GETINFOパケットを送信。
 * ・他のメソッド（ANSINFO受信）にて取得するまで待機。（五回まで）
 * ・IPアドレスでマッチングしてANSINFOで更新されたバージョン情報を取得
 * @param host 対象のホスト
 * @retval 対象ホストのバージョン情報
 * 注：このメソッドはスレッドセーフでない。
 */
string
IpMessengerAgentImpl::GetInfo( HostListItem host )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen = sizeof( packetNoBuf );
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( host.PortNo() );
	addr.sin_addr.s_addr = inet_addr(host.IpAddress().c_str());

	RecvPacket();
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETINFO,
										  _LoginName, _HostName,
										  packetNoBuf, packetNoBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_GETINFO, sendBuf, sendBufLen, addr );
	int pcount = RecvPacket();
	for( int i = 0; i < 5; i++ ) {
		if ( pcount == 0 ) {
			break;
		}
		pcount = RecvPacket();
	}
	vector<HostListItem>::iterator hostIt = FindHostByAddress( host.IpAddress() );
	if ( hostIt != hostList.end() ) {
		return hostIt->Version();
	}
	return "";
}

/**
 * 対象ホストの不在説明文字列情報を取得。
 * ・GETABSENCEINFOパケットを送信。
 * ・他のメソッド（ANSABSENCEINFO受信）にて取得するまで待機。（五回まで）
 * ・IPアドレスでマッチングしてANSABSENCEINFOで更新されたバージョン情報を取得
 * @param host 対象のホスト
 * @retval 対象ホストの不在説明文字列情報
 * 注：このメソッドはスレッドセーフでない。
 */
string
IpMessengerAgentImpl::GetAbsenceInfo( HostListItem host )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen = sizeof( packetNoBuf );
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( host.PortNo() );
	addr.sin_addr.s_addr = inet_addr(host.IpAddress().c_str());

	RecvPacket();
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETABSENCEINFO,
										  _LoginName, _HostName,
										  packetNoBuf, packetNoBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( IPMSG_GETABSENCEINFO, sendBuf, sendBufLen, addr );
	int pcount = RecvPacket();
	for( int i = 0; i < 5; i++ ) {
		if ( pcount == 0 ) {
			break;
		}
		pcount = RecvPacket();
	}
	vector<HostListItem>::iterator hostIt = FindHostByAddress( host.IpAddress() );
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
vector<string>
IpMessengerAgentImpl::GetGroupList()
{
	vector<string> ret;
	for( vector<HostListItem>::iterator ixhost = hostList.begin(); ixhost != hostList.end(); ixhost++ ) {
		bool is_found = false;
		for( vector<string>::iterator ixret = ret.begin(); ixret != ret.end(); ixret++){
			if ( ixhost->GroupName() == *ixret ) {
				is_found = true;
				break;
			}
		}
		if ( !is_found ){
			ret.push_back( ixhost->GroupName() );
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
	RecvPacket();
}

/**
 * 送信済メッセージリストに開封されたことをマークする。
 * @param msg 送信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::AcceptConfirmNotify( SentMessage msg )
{
	vector<SentMessage>::iterator sentMsg = FindSentMessageByPacketNo( msg.PacketNo() );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmAnswered( true );
	}
}

/**
 * パケットNoで送信済メッセージリストから送信済メッセージのイテレータを取得する。
 * @param PacketNo パケットNo
 * @retval 送信済メッセージのイテレータ。（見付からない場合sentMsgList.end()）
 * 注：このメソッドはスレッドセーフでない。
 */
vector<SentMessage>::iterator
IpMessengerAgentImpl::FindSentMessageByPacketNo( unsigned long PacketNo )
{
	for( vector<SentMessage>::iterator ixmsg = sentMsgList.begin(); ixmsg != sentMsgList.end(); ixmsg++ ) {
		if ( PacketNo == ixmsg->PacketNo() ) {
			return ixmsg;
		}
	}
	return sentMsgList.end();
}
		
// private methods start here

/**
 * 送信初期化
 * ・ブロードキャストアドレス構造体の初期化＋リストに押し込む。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::InitSend()
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(IPMSG_DEFAULT_PORT);
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	broadcastAddr.push_back( addr );
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
	printf("== S E N D   T C P ====================================>\n");
#endif
	IpMsgPrintBuf( "SendTcpPacket:SendTcpBufer", buf, size );
	int ret = 0;
	ret = send( sd, buf, size + 1, 0 );
	if ( ret <= 0 ) {
		perror("send");
#if defined(DEBUG)
		printf("S E N D   T C P   F A I L E D\n");
#endif
	}
#if defined(DEBUG)
	printf("<= S E N D   T C P======================================\n");
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
IpMessengerAgentImpl::SendPacket( const long cmd, char *buf, int size, struct sockaddr_in to_addr )
{
#if defined(DEBUG)
	printf("== S E N D ============================================>\n");
	printf( "Command[%s]\n", GetCommandString( cmd ).c_str() );
	printf( "Send  %s(%d)\n", inet_ntoa( to_addr.sin_addr ), ntohs( to_addr.sin_port ) );
#endif
	IpMsgPrintBuf( "SendUdpPacket:SendUdpBuffer", buf, size );
	int ret = 0;
	ret = sendto( udp_sd[0], buf, size + 1, 0, ( struct sockaddr * )&to_addr, sizeof( to_addr ) );
	if ( ret <= 0 ) {
		perror("sendto unicast");
#if defined(DEBUG)
		printf("S E N D   F A I L E D ( ret = %d)\n", ret );
#endif
	}
#if defined(DEBUG)
	printf("<= S E N D =============================================\n");
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
IpMessengerAgentImpl::SendBroadcast( const long cmd, char *buf, int size )
{
#if defined(DEBUG)
	printf("== S E N D   B R O A D C A S T ========================>\n");
	printf( "Command[%s]\n", GetCommandString( cmd ).c_str() );
#endif
	IpMsgPrintBuf( "SendBroadcast:SendUdpBroadcastBuffer", buf, size );
	for( vector<struct sockaddr_in>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
#if defined(DEBUG)
		printf( "Send To %s(%d)\n", inet_ntoa( ixaddr->sin_addr ), ntohs( ixaddr->sin_port ) );
#endif
		int ret = 0;
		for( unsigned int i = 0; i < udp_sd.size(); i++ ){
			ret = sendto( udp_sd[i], buf, size + 1, 0, ( struct sockaddr * )&(*ixaddr), sizeof( struct sockaddr ) );
			if ( ret <= 0 ) {
				perror("sendto broadcast.");
#if defined(DEBUG)
				printf("S E N D   F A I L E D\n");
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
			printf( "Send To %s(%d)\n", inet_ntoa( addr.sin_addr ), ntohs( addr.sin_port ) );
#endif
			int ret = 0;
			for( unsigned int i = 0; i < udp_sd.size(); i++ ){
				ret = sendto( udp_sd[i], buf, size + 1, 0, ( struct sockaddr * )&addr, sizeof( struct sockaddr ) );
				if ( ret <= 0 ) {
					perror("sendto dialup host.");
#if defined(DEBUG)
					printf("S E N D   F A I L E D ( D I A L U P )\n");
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
	printf( "Send To %s(%d)\n", inet_ntoa( addr.sin_addr ), ntohs( addr.sin_port ) );
#endif
	int ret = sendto( udp_sd[0], buf, size + 1, 0, ( struct sockaddr * )&addr, sizeof( struct sockaddr ) );
	if ( ret <= 0 ) {
		perror("sendto myself.");
#if defined(DEBUG)
		printf("S E N D   F A I L E D ( D I A L U P )\n");
#endif
	}

#if defined(DEBUG)
	printf("<= S E N D   B R O A D C A S T =========================\n");
#endif
}

/**
 * 受信初期化。
 * ・2425待ち受けのUDP、TCPソケットを準備する
 * ・UDPはbroadcast許可
 * ・TCPはREUSEADDR
 * ・litsenは5ポート
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::InitRecv( vector<NetworkInterface> nics )
{
	if ( nics.size() > 0 ) {
		HostAddress = nics[0].IpAddress();
	}
	for( vector<struct sockaddr_in>::iterator addr = broadcastAddr.begin(); addr != broadcastAddr.end(); addr++ ){
		int sock = -1;

		sock = InitUdpRecv( *addr );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "UDP_SD[%d] = %d\n", udp_sd.size(),sock );
#endif
			udp_sd.push_back( sock );
		} else {
			printf( "UDP Error=%s\n", inet_ntoa( addr->sin_addr ) );
		}
	}
	for( unsigned int i = 0; i < nics.size(); i++ ){
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( nics[i].PortNo() );
		addr.sin_addr.s_addr = inet_addr( nics[i].IpAddress().c_str() );

		int sock = -1;

		sock = InitUdpRecv( addr );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "UDP_SD[%d] = %d\n", udp_sd.size(),sock );
#endif
			udp_sd.push_back( sock );
		} else {
			printf( "UDP Error[%s]=%s\n", nics[i].DeviceName().c_str(), nics[i].IpAddress().c_str() );
		}
		sock = InitTcpRecv( addr );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "TCP_SD[%d] = %d\n", tcp_sd.size(),sock );
#endif
			tcp_sd.push_back( sock );
		} else {
			printf( "TCP Error[%s]=%s\n", nics[i].DeviceName().c_str(), nics[i].IpAddress().c_str() );
		}
	}

//		addr.sin_addr.s_addr = inet_addr( "192.168.1.111" );
//	addr.sin_addr.s_addr = inet_addr( "192.168.163.1" );
//	sock = InitUdpRecv( addr );
//	if ( sock > 0 ) {
//		udp_sd.push_back( sock );
//	}
//	sock = InitTcpRecv( addr );
//	if ( sock > 0 ) {
//		tcp_sd.push_back( sock );
//	}

	FD_ZERO( &rfds );
	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		FD_SET( udp_sd[i], &rfds );
	}
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		FD_SET( tcp_sd[i], &rfds );
	}
}

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
//	//マルチキャスト送信用のインターフェースの指定	
//	in_addr_t my_if = addr.sin_addr.s_addr;
//	if ( setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&my_if, sizeof(my_if) ) != 0 ){
//		perror("setsockopt(multicast_if)");
//		close( sock );
//		return -1;
//	}
//	//マルチキャスト受信の準備	
//	struct ip_mreq mreq;
//	memset(&mreq, 0, sizeof(mreq));
//	mreq.imr_interface.s_addr = INADDR_ANY;
//	mreq.imr_interface.s_addr = addr.sin_addr.s_addr;
//	mreq.imr_multiaddr.s_addr = addr.sin_addr.s_addr;
//	mreq.imr_multiaddr.s_addr = inet_addr("192.168.163.255");
//	if ( setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof( mreq ) ) != 0 ) {
//		perror("setsockopt(add_membership)");
//		close( sock );
//		return -1;
//	}

	return sock;
}
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
 */
int
IpMessengerAgentImpl::RecvPacket()
{
	char buf[MAX_UDPBUF];
	int selret = 1;
	int ret = 0;
	int max_sd = -1;

	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		if ( max_sd < udp_sd[i] ){
			max_sd = udp_sd[i];
		}
	}
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		if ( max_sd < tcp_sd[i] ){
			max_sd = tcp_sd[i];
		}
	}

	queue<Packet> pack_que;

	while( selret > 0 ) {
		fd_set fds;
		memcpy( &fds, &rfds, sizeof( fd_set ) );

		memset( buf, 0, sizeof( buf ) );
		tv.tv_sec = SELECT_TIMEOUT_SEC;
		tv.tv_usec = SELECT_TIMEOUT_USEC;
		selret = select( max_sd + 1, &fds, NULL, NULL, &tv );
		if ( selret == -1 ) {
			perror( "select()" );
			break;
		} else if ( selret == 0 ){
#if defined(INFO) || !defined(NDEBUG)
			printf(".");
#endif
			break;
		} else {
			int tcp_socket = -1;
#if defined(DEBUG)
			printf("\n");
			printf( "select returns == %d\n\n", selret );
#endif
			struct sockaddr_in sender_addr;
			socklen_t sender_addr_len = 0;
			int sz = 0;
			//とりあえず
			bool recieved = false;
			//UDPでソケットに変化が有ったら受信
			for( unsigned int i = 0; i < udp_sd.size(); i++ ){
				if ( FD_ISSET( udp_sd[i], &fds ) ){
					memset( &sender_addr, 0, sizeof( struct sockaddr_in ) );
					sender_addr_len = sizeof( struct sockaddr_in );
					sz = recvfrom( udp_sd[i], buf, sizeof( buf ), 0, (struct sockaddr *)&sender_addr, &sender_addr_len );
					if ( sz < 0 ) {
						perror("recvfrom");
					}
//					if ( sz > 0 ){
//						Packet packet = DismantlePacketBuffer( buf, sz, sender_addr );
//						packet.setTcpSocket( -1 );
//						IpMsgDumpPacket( packet, sender_addr );
//						ret++;
//						pack_que.push( packet );
						
						recieved = true;
						break;
//					}
				}
			}
			//UDPでソケットに変化がない。
			tcp_socket = -1;
			if ( !recieved ) {
				//TCPでソケットに変化が有ったら受信
				for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
					if ( FD_ISSET( tcp_sd[i], &fds ) ){
						memset( &sender_addr, 0, sizeof( struct sockaddr_in ) );
						sender_addr_len = sizeof( struct sockaddr_in );
						tcp_socket = accept( tcp_sd[i], (struct sockaddr *)&sender_addr, &sender_addr_len );
						if ( tcp_socket < 0 ) {
							perror("accept");
						}
						sz = recv( tcp_socket, buf, sizeof( buf ), 0 );
						if ( sz < 0 ) {
							perror("recv");
						}
#if defined(INFO) || !defined(NDEBUG)
						printf("recv buf[%s]\n", buf );
#endif
//						if ( sz > 0 ){
//							Packet packet = DismantlePacketBuffer( buf, sz, sender_addr );
//							packet.setTcpSocket( tcp_socket );
//							IpMsgDumpPacket( packet, sender_addr );
//							ret++;
//							pack_que.push( packet );
							recieved = true;
							break;
//						}
					}
				}
				//UDP,TCPでソケットに変化がない。
				if ( !recieved ) {
					continue;
				}
			}
			Packet packet = DismantlePacketBuffer( buf, sz, sender_addr );
			IpMsgDumpPacket( packet, packet.Addr() );
			packet.setTcpSocket( tcp_socket );
			ret++;
			pack_que.push( packet );
		}
	}
	while( !pack_que.empty() ) {
		DoRecvCommand( pack_que.front() );
		pack_que.pop();
	}

#if defined(DEBUG) || defined(INFO) || !defined(NDEBUG)
	printf("sentMsgList.size=%d\n", sentMsgList.size() );
	fflush(stdout);
#endif
	time_t tryNow = time( NULL );
	for( vector<SentMessage>::iterator ixmsg = sentMsgList.begin(); ixmsg != sentMsgList.end(); ixmsg++ ) {
		if ( needSendRetry( *ixmsg, tryNow ) ) {
			//再送信
			ixmsg->setRetryCount( ixmsg->RetryCount() + 1 );
			ixmsg->setPrevTry( tryNow );
		}
		if ( ixmsg->RetryCount() > 5 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("Retry Max Over\n");
#endif
			if ( event != NULL ){
				event->SendRetryError( *ixmsg );
			}
			ixmsg->setRetryCount( 0 );
			ixmsg->setIsRetryMaxOver( true );
		}
	}
	return ret;
}

bool
IpMessengerAgentImpl::isRetryMaxOver( SentMessage msg, int retryCount )
{
	if ( msg.RetryCount() > SENDMSG_RETRY_MAX ) {
		return true;
	}
	return false;
}
bool
IpMessengerAgentImpl::needSendRetry( SentMessage msg, time_t tryNow )
{
	if ( !msg.IsSent() && msg.PrevTry() != tryNow && !msg.IsRetryMaxOver() ) {
		return true;
	}
	return false;
}
/**
 * パケットのコマンドモードで受信イベントを振り分ける。
 * @param packet パケットオブジェクト
 */
void
IpMessengerAgentImpl::DoRecvCommand( Packet packet )
{
#if defined(DEBUG)
	printf( "PACKET.COMMAND=[%s]\n", GetCommandString( packet.CommandMode() ).c_str() );
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
IpMessengerAgentImpl::UdpRecvEventNoOperation( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvNoOperation\n");
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
IpMessengerAgentImpl::UdpRecvEventBrEntry( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrEntry\n");
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
	sendBufLen = CreateNewPacketBuffer( IPMSG_ANSENTRY,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( AddCommonCommandOption( IPMSG_ANSENTRY ), sendBuf, sendBufLen, packet.Addr() );
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
	printf("SendBrAbsence\n");
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

	sendBufLen = CreateNewPacketBuffer( IPMSG_BR_ABSENCE | ( _IsAbsence ? IPMSG_ABSENCEOPT : 0UL),
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
IpMessengerAgentImpl::UdpRecvEventBrAbsence( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrAbsence\n");
#endif
	hostList.DeleteHost( packet.HostName() );
	hostList.AddHost( HostList::CreateHostListItemFromPacket( packet ) );
	return 0;
}

/**
 * 電文受信イベント：BR_EXIT
 * ・自分のホストリストからホストを削除する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrExit( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrExit\n");
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
IpMessengerAgentImpl::UdpRecvEventRecvMsg( Packet packet )
{
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = FindSentMessageByPacketNo( packetNo );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsSent( true );
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvRecvMsg\n");
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
IpMessengerAgentImpl::UdpRecvEventReadMsg( Packet packet )
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
	vector<SentMessage>::iterator sentMsg = FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmed( true );
	}
	if ( event != NULL ) {
		event->OpenAfter( *sentMsg );
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvReadMsg\n");
#endif
	return 0;
}

/**
 * 電文受信イベント：BR_DELMSG
 * ・自分の送信済メッセージリストの該当メッセージを削除。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventDelMsg( Packet packet )
{
	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsgList.erase(sentMsg);
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvDelMsg\n");
#endif
	return 0;
}

/**
 * 電文受信イベント：BR_ANSREADMSG
 * ・何もしない。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsReadMsg( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsReadMsg\n");
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
IpMessengerAgentImpl::UdpRecvEventSendMsg( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

	for( vector<RecievedMessage>::iterator ixmsg = recvMsgList.begin(); ixmsg != recvMsgList.end(); ixmsg++ ) {
		if ( packet.PacketNo() == ixmsg->MessagePacket().PacketNo() ) {
#if defined(DEBUG) || !defined(NDEBUG)
			printf("すでに追加済み\n");
#endif
			return 0;
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvSendMsg[Packet = %lu]\n", packet.PacketNo() );
#endif
	if ( packet.CommandOption() & IPMSG_BROADCASTOPT ||  packet.CommandOption() & IPMSG_AUTORETOPT ) {
		;
	} else {
		if ( packet.CommandOption() & IPMSG_SENDCHECKOPT ) {
			packetNoBufLen = snprintf( packetNoBuf, sizeof( packetNoBuf ), "%ld", packet.PacketNo() );
#if defined(INFO) || !defined(NDEBUG)
printf("SENDCHECKOPT(%s)\n", packetNoBuf);
#endif
			sendBufLen = CreateNewPacketBuffer( IPMSG_RECVMSG,
												  _LoginName, _HostName,
												  packetNoBuf, packetNoBufLen,
												  sendBuf, sizeof( sendBuf ) );
#if defined(INFO) || !defined(NDEBUG)
printf("Send(%s) -> IP[%s]\n", sendBuf, inet_ntoa( packet.Addr().sin_addr ) );
#endif
			SendPacket( IPMSG_RECVMSG, sendBuf, sendBufLen, packet.Addr() );
		}
		if ( _IsAbsence ) {
			HostListItem host;
			host.setIpAddress( inet_ntoa( packet.Addr().sin_addr ) );
			host.setPortNo( ntohs( packet.Addr().sin_port ) );
			host.setEncodingName( localEncoding );
			vector<HostListItem>::iterator hostIt = FindHostByAddress( host.IpAddress() );
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
			SendMsg( host, AbsenceDescription.c_str(), false );
		}
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("CHECK ENCRYPT[Packet = %lu]\n", packet.PacketNo() );
	printf("Decript Before Message[%s]\n", packet.Option().c_str() );
	fflush(stdout);
#endif
	if ( packet.CommandOption() & IPMSG_ENCRYPTOPT ){
#if defined(INFO) || !defined(NDEBUG)
	printf("ENCRYPT[Packet = %lu]\n", packet.PacketNo() );
#endif
		if ( !DecryptMsg( packet ) ) {
			HostListItem host;
			host.setIpAddress( inet_ntoa( packet.Addr().sin_addr ) );
			host.setPortNo( ntohs( packet.Addr().sin_port ) );
			SendMsg( host, DecryptErrorMessage.c_str(), false, IPMSG_AUTORETOPT );
			packet.setOption("");
		}
	}
	RecievedMessage message;
	message.setMessagePacket( packet );
	message.setMessage( packet.Option().c_str() );
#if defined(INFO) || !defined(NDEBUG)
	printf("Message[%s]\n", packet.Option().c_str() );
	fflush(stdout);
#endif
	message.setRecieved( time( NULL ) );
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
#if defined(DEBUG) || !defined(NDEBUG)
	printf( "UserName[%s]\n", packet.UserName().c_str() );
	printf( "HostName[%s]\n", packet.HostName().c_str() );
	printf( "UserName[%s]\n", message.Host().UserName().c_str() );
	printf( "HostName[%s]\n", message.Host().HostName().c_str() );
	printf( "Nickname[%s]\n", message.Host().Nickname().c_str() );
#endif

	message.setHasAttachFile( false );
	AttachFileList files = message.Files();
	if ( CreateAttachedFileList( packet.Option().c_str(), files ) != 0 ) {
		message.setHasAttachFile( true );
	}
	bool eventRet = false;
	message.setFiles( files );
	if ( event != NULL ) {
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
IpMessengerAgentImpl::UdpRecvEventBrIsGetList( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrIsGetList\n");
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_OKGETLIST,
										  _LoginName, _HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( AddCommonCommandOption( IPMSG_OKGETLIST ), sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_ISGETLIST2
 * ・OKGETLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventBrIsGetList2( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrIsGetList2\n");
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_OKGETLIST,
										  _LoginName, _HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( AddCommonCommandOption( IPMSG_OKGETLIST ), sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_GETLIST
 * ・ANSLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetList( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	int start = 0;
	char *dmy;
	string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetList[%s]\n", packet.Option().c_str());
#endif
	start = strtoul( packet.Option().c_str(), &dmy, 10 );
	hosts = hostList.ToString( start );
	sendBufLen = CreateNewPacketBuffer( IPMSG_ANSLIST,
										  _LoginName, _HostName,
										  hosts.c_str(), hosts.length(),
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( AddCommonCommandOption( IPMSG_ANSLIST ), sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_OKGETLIST
 * ・GETLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventOkGetList( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvOkGetList[%s]\n", packet.Option().c_str());
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETLIST,
										  _LoginName, _HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( AddCommonCommandOption( IPMSG_GETLIST ), sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_ANSENTRY
 * ・何もしない。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsEntry( Packet packet )
{
//	char sendBuf[MAX_UDPBUF];
//	int sendBufLen;
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsEntry\n");
#endif
//	sendBufLen = CreateNewPacketBuffer( IPMSG_ANSENTRY,
//										  LoginName, HostName,
//										  NULL, 0,
//										  sendBuf, sizeof( sendBuf ) );
//	SendPacket( IPMSG_ANSENTRY, sendBuf, sendBufLen, packet.Addr() );
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
IpMessengerAgentImpl::UdpRecvEventAnsList( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char nextbuf[1024];

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsList\n");
#endif
	AddDefaultHost();
	int nextstart = CreateHostList( packet.Option().c_str(), packet.Option().length() );
	if ( nextstart > 0 ) {
		int nextbuf_len = snprintf( nextbuf, sizeof( nextbuf ), "%d", hostList.size() + 1 );
#if defined(INFO) || !defined(NDEBUG)
		printf("nextbuf_len = %d\n", nextbuf_len );
#endif
		sendBufLen = CreateNewPacketBuffer( IPMSG_GETLIST,
											  _LoginName, _HostName,
											  nextbuf, nextbuf_len,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( AddCommonCommandOption( IPMSG_GETLIST ), sendBuf, sendBufLen, packet.Addr() );
	}
	return 0;
}

/**
 * 電文受信イベント：BR_GETINFO
 * ・バージョン情報をSENDINFOに詰めて投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetInfo( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	string version = IPMSG_AGENT_VERSION;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetInfo[%s]\n", packet.Option().c_str());
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_SENDINFO,
										  _LoginName, _HostName,
										  version.c_str(), version.length(),
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( AddCommonCommandOption( IPMSG_SENDINFO ), sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_SENDINFO
 * ・取得したバージョン情報をホストリストに更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendInfo( Packet packet )
{
	string pIpAddress = inet_ntoa( packet.Addr().sin_addr );
	vector<HostListItem>::iterator hostIt = FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
		hostIt->setVersion( packet.Option() );
	}
	return 0;
}

/**
 * 電文受信イベント：BR_GETABSENCEINFO
 * ・不在詳細情報をSENDINFOに詰めて投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetAbsenceInfo[%s]\n", packet.Option().c_str());
#endif
	string IpAddress = inet_ntoa( packet.Addr().sin_addr );
	string EncodingName = localEncoding;
	vector<HostListItem>::iterator hostIt = FindHostByAddress( IpAddress );
	if ( hostIt != hostList.end() ) {
		EncodingName = hostIt->EncodingName();
	}
	string AbsenceDescription = "";
	for( vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
		if ( i->EncodingName() == localEncoding ) {
			AbsenceDescription = i->AbsenceDescription();
			break;
		}
	}
	sendBufLen = CreateNewPacketBuffer( IPMSG_SENDABSENCEINFO,
										  _LoginName, _HostName,
										  AbsenceDescription.c_str(), AbsenceDescription.length(),
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( AddCommonCommandOption( IPMSG_SENDABSENCEINFO ), sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_SENDABSENCEINFO
 * ・取得した不在詳細情報をホストリストに更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::UdpRecvEventSendAbsenceInfo( Packet packet )
{
	string pIpAddress = inet_ntoa( packet.Addr().sin_addr );
	vector<HostListItem>::iterator hostIt = FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
		hostIt->setAbsenceDescription( packet.Option() );
	}
	return 0;
}

/**
 * パケットから送信済メッセージを検索します。
 * ・パケットからパケットNoを抽出しパケットNoを基に送信済メッセージを検索し、SentMessageのイテレータを返します。
 * @param packet パケットオブジェクト
 * @retval SentMessageのイテレータ。見付からない場合、end()を返す。
 */
vector<SentMessage>::iterator
IpMessengerAgentImpl::FindSentMessageByPacket( Packet packet )
{
	char *dmyptr;
	char *startptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;

	vector<AttachFile>::iterator FoundFile;
	for( vector<SentMessage>::iterator ixmsg = sentMsgList.begin(); ixmsg != sentMsgList.end(); ixmsg++ ) {
		if ( packetNo == ixmsg->PacketNo() ) {
			return ixmsg; 
		}
	}
	return sentMsgList.end(); 
}

/**
 * パケットからオフセットを取得します。
 * ・パケットからオフセットを抽出し返します。
 * @param packet パケットオブジェクト
 * @retval ファイルオフセット。
 */
static long
GetSendFileOffsetInPacket( Packet packet )
{
	char *dmyptr;
	char *startptr;
	strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;
	strtoul( startptr, &dmyptr, 16 );
	startptr = ++dmyptr;
	long offset = strtoul( startptr, &dmyptr, 16 );

	return offset;
}

/**
 * 電文受信イベント：BR_GETFILEDATA
 * ・ファイル情報をTCPソケットにのせて送信し、その後ファイルをダウンロードさせる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgentImpl::TcpRecvEventGetFileData( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "TcpRecvEventGetFileData\n" );
#endif

#ifdef HAVE_PTHREAD
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
#else	// HAVE_PTHREAD
	Packet *packetClone = new Packet( packet );
	GetFileDataThread( packetClone );
#endif	// HAVE_PTHREAD
}

/**
 * ファイルダウンロードスレッド
 * ・ファイルをダウンロードさせる。
 * @param param パケットオブジェクト(void*)
 */
void *
GetFileDataThread( void *param )
{
	Packet *packet = (Packet *)param;

	vector<SentMessage>::iterator msg = IpMessengerAgentImpl::GetInstance()->FindSentMessageByPacket( *packet );
	if ( msg == IpMessengerAgentImpl::GetInstance()->SentMessageListEnd() ){
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
	IpMessengerAgentImpl::GetInstance()->SendFile( packet->TcpSocket(), FoundFile->FullPath(), GetSendFileOffsetInPacket( *packet ) );
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
IpMessengerAgentImpl::UdpRecvEventReleaseFiles( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "TcpRecvEventReleaseFiles\n" );
#endif
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = FindSentMessageByPacketNo( packetNo );
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
IpMessengerAgentImpl::UdpRecvEventGetPubKey( Packet packet )
{
#ifdef HAVE_OPENSSL
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetPubKey[%s]\n", packet.Option().c_str());
#endif
	char *dmyptr;
	unsigned cap = strtoul( packet.Option().c_str(), &dmyptr, 16 );
	RSA *rsa = NULL;
	unsigned long pubKeyMethod = 0UL;
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_2048 && cap& IPMSG_RSA_2048 ) {
		pubKeyMethod |= IPMSG_RSA_2048;
		rsa = RsaMax != NULL ? RsaMax : NULL;
	}
#endif	// SUPPORT_RSA_2048
#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_1024 && cap & IPMSG_RSA_1024 ) {
		pubKeyMethod |= IPMSG_RSA_1024;
		rsa = RsaMid != NULL ? RsaMid : NULL;
	}
#endif	// SUPPORT_RSA_1024
#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_512  && cap & IPMSG_RSA_512 ) {
		pubKeyMethod |= IPMSG_RSA_512;
		rsa = RsaMin != NULL ? RsaMin : NULL;
	}
#endif	// SUPPORT_RSA_512
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
IpMessengerAgentImpl::UdpRecvEventAnsPubKey( Packet packet )
{
#ifdef HAVE_OPENSSL
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsPubKey[%s]\n", packet.Option().c_str());
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
	string pIpAddress = inet_ntoa( packet.Addr().sin_addr );
	vector<HostListItem>::iterator hostIt = FindHostByAddress( pIpAddress );
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
IpMessengerAgentImpl::TcpRecvEventGetDirFiles( Packet packet )
{
#ifdef HAVE_PTHREAD
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

#else	// HAVE_PTHREAD
	Packet *packetClone = new Packet( packet );
	GetDirFilesThread( (void *) packetClone );
#endif	// HAVE_PTHREAD
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
	printf( "TcpRecvEventGetDirFiles\n" );
#endif
	vector<SentMessage>::iterator msg = myInstance->FindSentMessageByPacket( *packet );
	if ( msg == myInstance->SentMessageListEnd() ){
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
	char headbuf[8192];

	if ( d == NULL ) {
		return false;
	}

	stat( cd.c_str(), &st );
	int head_len = snprintf( headbuf, sizeof( headbuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
														converter->ConvertLocalToNetwork( cd.c_str() ).c_str(), (long long)st.st_size,
														IPMSG_FILE_DIR,
														IPMSG_FILE_MTIME, st.st_mtime,
														IPMSG_FILE_CREATETIME, st.st_ctime );
	headbuf[ snprintf( headbuf, sizeof(headbuf),"%04x", head_len) ] = ':';
	send( sock, headbuf, head_len, 0 );

	dent = readdir( d );
	while( dent != NULL ) {
		if ( strcmp(dent->d_name, "." ) != 0 && strcmp(dent->d_name, ".." ) != 0 ) {
			string dir_name = dir + "/" + dent->d_name;
#if defined(INFO) || !defined(NDEBUG)
			printf( "dir[%s]", dir_name.c_str() );
#endif
			stat( dir_name.c_str(), &st );
			files.push_back( dir_name );
			if ( S_ISDIR( st.st_mode ) ){
#if defined(INFO) || !defined(NDEBUG)
				printf( "DIR\n" );
#endif
				if ( !SendDirData( sock, dent->d_name, dir_name, files ) ){
					closedir( d );
					return false;
				}
			} else {
#if defined(INFO) || !defined(NDEBUG)
				printf( "FILE\n" );
#endif
				int head_len = snprintf( headbuf, sizeof( headbuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
																	converter->ConvertLocalToNetwork( dent->d_name ).c_str(), (long long)st.st_size,
																	IPMSG_FILE_REGULAR,
																	IPMSG_FILE_MTIME, st.st_mtime,
																	IPMSG_FILE_CREATETIME, st.st_ctime );
				headbuf[ snprintf( headbuf, sizeof(headbuf),"%04x", head_len) ] = ':';
				send( sock, headbuf, head_len, 0 );

				if ( !SendFile( sock, dir_name, 0 ) ){
					closedir( d );
					return false;
				}
			}
		}
		dent = readdir( d );
	}
	head_len = snprintf( headbuf, sizeof( headbuf ), "0000:.:0:%lx:", IPMSG_FILE_RETPARENT );
	headbuf[ snprintf( headbuf, sizeof(headbuf),"%04x", head_len) ] = ':';
	send( sock, headbuf, head_len, 0 );
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
IpMessengerAgentImpl::SendFile( int sock, string FileName, off_t offset )
{
	string localFileName = converter->ConvertNetworkToLocal( FileName.c_str() );
	char readbuf[8192];
	struct stat st_init;
	int read_size;
	int fd = open( localFileName.c_str(), O_RDONLY );
	if ( fd < 0 ) {
		perror( "open" );
		printf("FileName.c_str() [%s]", FileName.c_str() );
		return false;
	}
	int rc = fstat( fd, &st_init );
	if ( rc != 0 ){
		close( fd );
		return false;
	}
	lseek( fd, offset, SEEK_SET );
	read_size = read( fd, readbuf, sizeof( readbuf ) );
	while( read_size > 0 ){
		if ( AbortDownloadAtFileChanged() ){
			struct stat st_progress;
			int rc = fstat( fd, &st_progress );
			if ( rc != 0 ){
				close( fd );
				return false;
			}
			if ( st_init.st_mtime != st_progress.st_mtime ||
				 st_init.st_ctime != st_progress.st_ctime ||
				 st_init.st_uid   != st_progress.st_uid   ||
				 st_init.st_gid   != st_progress.st_gid   ||
				 st_init.st_size  != st_progress.st_size ) {
				close( fd );
				return false;
			}
		}
		send( sock, readbuf, read_size, 0 );
		read_size = read( fd, readbuf, sizeof( readbuf ) );
	}
	close( fd );
	return true;
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
printf("File List Buffer = [%s]\n", file_list_tmp_buf);
fflush(stdout);
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
			printf("AttachFile(-1)\n" );
#endif
			// FILE ID
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileId( strtoul( token, &ptrdmy, 10 ) );
			printf( "file.FileId() %d token [%s]\n", file.FileId(), token );
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
			while( token != NULL && *token != 'a' ) {
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
					while( *ptrdmy != '0' ) {
						file.addExtAttrs( token, strtoul( topchar, &ptrdmy, 16 ) );
						topchar = ++ptrdmy;
					}
				}
			}
#if defined(DEBUG) || !defined(NDEBUG)
			printf("\n\n");
			printf("== FILE  ==============================>\n");
			printf("FILE ID[%d]\n", file.FileId());
			printf("FILE NAME[%s]\n", file.FileName().c_str());
			printf("FILE SIZE[%lld]\n", file.FileSize());
			time_t tt = file.MTime();
			printf("MTIME[%s]\n", ctime( &tt ) );
			printf("ATTR[%lu]\n", file.Attr() );
			for( map<string, vector<unsigned long> >::iterator ixextattr = file.beginExtAttrs(); ixextattr != file.endExtAttrs(); ixextattr++){
				printf("EXT ATTR[%s]==", ixextattr->first.c_str() );
				for( vector<unsigned long>::iterator ixextattrv = ixextattr->second.begin(); ixextattrv != ixextattr->second.end(); ixextattrv++){
					printf("[%lu]", *ixextattrv );
				}
				printf("\n" );
			}
			printf("<= FILE  ===============================\n");
#endif
			// ADD FILELIST
#if defined(DEBUG) || !defined(NDEBUG)
			printf("AddFile()\n" );
#endif
			files.AddFile( file );
			break;
		}
		// FILE ID(not 1st)
		if ( token == NULL ){
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
IpMessengerAgentImpl::CreateHostList( const char *hostListBuf, int buf_len )
{
	int alloc_size = buf_len + 1;
	int add_count = 0;
	char *hostListTmpPtr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *hostListTmpBuf = (char *)calloc( alloc_size, 1 );

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
			token++;
			nextpos = token;
		} else {
			item.setUserName( token );
			hostListTmpPtr = nextpos;
			token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		}
		if ( token == NULL ) break;
		// HOST NAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setHostName( "" );
			token++;
			nextpos = token;
		} else {
			item.setHostName( token );
			hostListTmpPtr = nextpos;
			token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		}
		if ( token == NULL ) break;
		// CommandNo
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setCommandNo( 0L );
			token++;
			nextpos = token;
		} else {
			item.setCommandNo( strtoul( token, &ptrdmy, 10 ) );
			hostListTmpPtr = nextpos;
			token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		}
		if ( token == NULL ) break;
		// IP ADDRESS
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setIpAddress( "" );
			token++;
			nextpos = token;
		} else {
			item.setIpAddress( token );
			hostListTmpPtr = nextpos;
			token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		}
		if ( token == NULL ) break;
		// PORTNO
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setPortNo( 0L );
			token++;
			nextpos = token;
		} else {
			item.setPortNo( ntohs( strtoul( token, &ptrdmy, 10 ) ) );
			hostListTmpPtr = nextpos;
			token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		}
		if ( token == NULL ) break;
		// NICKNAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setNickname( "" );
			token++;
			nextpos = token;
		} else {
			item.setNickname( token );
			hostListTmpPtr = nextpos;
			token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		}
		if ( token == NULL ) break;
		// GROUPNAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setGroupName( "" );
			token++;
			nextpos = token;
		} else {
			item.setGroupName( token );
			hostListTmpPtr = nextpos;
			token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		}
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
vector<SentMessage> *
IpMessengerAgentImpl::GetSentMessages()
{
	return sentMsgList.GetMessageList();
}

/**
 * 送信済メッセージリストのコピーを取得する。
 * @retval 送信済メッセージリストのコピー。
 */
vector<SentMessage>
IpMessengerAgentImpl::CloneSentMessages()
{
	vector<SentMessage> *msgList = sentMsgList.GetMessageList();
	return *msgList;
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
	return MAX_UDPBUF - headSize;
}

long
IpMessengerAgentImpl::AddCommonCommandOption( const long cmd )
{
	return cmd | ( IsAbsence() ? IPMSG_ABSENCEOPT : 0UL ) | ( IsDialup() ? IPMSG_DIALUPOPT : 0UL );
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
IpMessengerAgentImpl::CreateNewPacketBuffer(long cmd, long packetNo, string user, string host, const char *opt, int optLen, char *buf, int size )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "CMD[%s]\n", GetCommandString( GET_MODE( cmd ) ).c_str() );
#endif
	memset( buf, 0, size );
	cmd = AddCommonCommandOption( cmd );
	//Version:PacketNo:UserName:HostName:Command[:Option]
	int send_size = snprintf(buf, size, "%d:%ld:%s:%s:%ld:", IPMSG_VERSION, packetNo, user.c_str(), host.c_str(), cmd );
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
IpMessengerAgentImpl::CreateNewPacketBuffer(long cmd, string user, string host, const char *opt, int optLen, char *buf, int size )
{
	long packetNo = random();
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
IpMessengerAgentImpl::DismantlePacketBuffer( char *packet_buf, int size, struct sockaddr_in sender )
{
	Packet ret;
	int alloc_size = size + 1;
	char *packet_tmp_buf;
	char *packet_tmp_ptr;
	char *nextpos;
	char *token;
	char *ptrdmy;

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
	long command = strtoul( token, &ptrdmy, 10 ); 
	ret.setCommandMode( GET_MODE(command) );
	ret.setCommandOption( GET_OPT(command) );

	//OPTION
	int optLen = size - ( nextpos - packet_tmp_buf );
	ret.setOption( string( nextpos, optLen ) );
	free( packet_tmp_buf );

	vector<HostListItem>::iterator hostIt = FindHostByHostName( ret.HostName() );
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
 */
void
IpMessengerAgentImpl::AddHostListFromPacket( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("===================================\n");
	printf("AddHostListFromPacket\n");
	printf("===================================\n");
	IpMsgDumpPacket( packet, packet.Addr() );
	printf("===================================\n");
#endif
	AddDefaultHost();
	// デフォルトのNIC(０番目)以外の自分自身のIPアドレスが登録依頼されたら無視。
	string packetIpAddress = inet_ntoa( packet.Addr().sin_addr );
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
	item.setIpAddress( inet_ntoa( packet.Addr().sin_addr ) );
	int NicknameLen = strlen( packet.Option().c_str() );
	item.setNickname( packet.Option().c_str() );
	item.setGroupName( packet.Option().c_str() + NicknameLen );
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
	vector<HostListItem>::iterator hostIt = FindHostByAddress( HostAddress );
	if ( hostIt == hostList.end() ) {
		HostListItem myHost;
		myHost.setUserName( _LoginName );
		myHost.setHostName( _HostName );
		myHost.setCommandNo( AddCommonCommandOption( IPMSG_FILEATTACHOPT
#ifdef HAVE_OPENSSL
												   | IPMSG_ENCRYPTOPT
#endif	// HAVE_OPENSSL
																		) );
		myHost.setIpAddress( HostAddress );
		myHost.setNickname( Nickname );
		myHost.setGroupName( GroupName );
		myHost.setPortNo( IPMSG_DEFAULT_PORT );
		hostList.AddHost( myHost );
#if defined(INFO) || !defined(NDEBUG)
		printf("MyHost Add.[%s][%s]\n", myHost.UserName().c_str(), myHost.GroupName().c_str() );
#endif
		return 1;
	}
	return 0;
}
//end of source
