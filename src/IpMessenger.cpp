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
#endif
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#endif
using namespace std;

//selectシステムコールのタイムアウト時間
#if defined(DEBUG)
#include <ctype.h>
#define SELECT_TIMEOUT_SEC	1
#define SELECT_TIMEOUT_USEC	0
#else
#define SELECT_TIMEOUT_SEC	0
#define SELECT_TIMEOUT_USEC	5
#endif

#define SENDMSG_RETRY_MAX	5
#define GETLIST_RETRY_MAX	2
#define HOST_LIST_SEND_MAX_AT_ONCE	100
#define	PACKET_DELIMITER_CHAR	':'
#define	PACKET_DELIMITER_STRING	":"
#define	PACKET_FIELD_SEPERATOR_CHAR	'\a'
#define	IPMSG_AGENT_VERSION		"IpMessengerAgent for C++ Unix Version 1.0"

#define RSA_KEY_LENGTH_MINIMUM	512
#define RSA_KEY_LENGTH_MIDIUM	1024
#define RSA_KEY_LENGTH_MAXIMUM	2048
#define ENCRYPT_PRIME			65537

static IpMessengerAgent *myInstance = NULL;

void *GetFileDataThread( void *param );
void *GetDirFilesThread( void *param );
#ifdef HAVE_PTHREAD
pthread_mutex_t instanceMutex;
int mutex_init_result = pthread_mutex_init( &instanceMutex, NULL );
#endif

/**
 * IP メッセンジャエージェントクラスのインスタンスを取得する。
 * Singletonパターンを採用しているので、ホスト唯一のインスタンスでなければならない。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgent *
IpMessengerAgent::GetInstance()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgent();
	}
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif
	return myInstance;
}

/**
 * IP メッセンジャエージェントクラスのインスタンスを解放する。
 * このメソッドを使ってオブジェクトを解放しなければならない。
 * ライブラリを通じないで直接deleteされた場合はその後の動作について感知しない。
 * 注：このインスタンスはスレッドセーフでない。
 */
void
IpMessengerAgent::Release()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif
	if ( myInstance == NULL ) {
#ifdef HAVE_PTHREAD
		pthread_mutex_unlock( &instanceMutex );
#endif
		return;
	}
	delete myInstance;
	myInstance = NULL;
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif
}

/**
 * IP メッセンジャエージェントクラスのコンストラクタ。
 * ・暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の生成を行う。
 * ・パケットNoに使用する乱数シードを時刻で初期化する。
 * ・ファイル名コンバータを初期セットアップする。（変換を行わないNullConverterがデフォルト）
 * ・ネットワークの初期化。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgent::IpMessengerAgent()
{
	CryptoInit();
	ResetAbsence();
	srandom( time( NULL ) );
	converter = new NullFileNameConverter();
	_IsAbortDownloadAtFileChanged = false;
	_IsSaveSentMessage = true;
	_IsSaveRecievedMessage = true;
	NetworkInit();
}

/**
 * IP メッセンジャエージェントクラスのデストラクタ。
 * ・まず、ログアウト。
 * ・暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の破棄を行う。
 * ・割り当て済のファイル名コンバータを削除する。
 * ・ソケットのクローズ。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgent::~IpMessengerAgent()
{
	Logout();
	CryptoEnd();
	delete converter;
	close(tcp_sd);
	close(udp_sd);
}

/**
 * ファイル名コンバータのセッター。
 * ・割り当て済のファイル名コンバータを削除する。
 * ・新しいコンバータの割り当て。
 * 注：このメソッドはスレッドセーフでない。
 * @param conv コンバータのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetFileNameConverter( FileNameConverter *conv )
{
	if ( conv == NULL ){
		return;
	}
	delete converter;
	converter = conv;
}

/**
 * 暗号関連の初期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::CryptoInit()
{
#ifdef WITH_OPENSSL
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
#endif	//WITH_OPENSSL
}

/**
 * 暗号関連の終期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::CryptoEnd()
{
#ifdef WITH_OPENSSL
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
#endif	//WITH_OPENSSL
}

/**
 * ネットワーク関連の初期化。
 * ・環境変数からホスト名を取得。（出来なければlocalhost固定）
 * ・環境変数からユーザ名を取得。（出来なければuid）
 * ・使用するネットワークインターフェイスのIPアドレスを求める。（ TODO :現在は"eth0"固定）
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::NetworkInit()
{
	char buf[100];
	char *env;

	env = getenv( "HOSTNAME" );
	if ( env == NULL ){
		HostName = "localhost";
	} else {
		HostName = env;
	}
	env = getenv( "USERNAME" );
	if ( env == NULL ){
		LoginName = snprintf( buf, sizeof( buf ), "%d", getuid() );
	} else {
		LoginName = env;
	}
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;

	/* eth0のIPアドレスを取得したい */
//	strncpy(ifr.ifr_name, "vmnet8", IFNAMSIZ-1);
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

#ifdef WITH_OPENSSL
	DecryptErrorMessage = "\r\n"\
						  " ==== AutoReply(DecryptErr) ====\r\n" \
						  "  My PubKey is updated, I can't\r\n" \
						  "  receive your message.\r\n" \
						  "  Please press refresh button.\r\n" \
						  " ==============================";
#endif	//WITH_OPENSSL
	HostAddress = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	InitSend();
	InitRecv();
}

/**
 * ログイン（サービス参加通知）。
 * ・NOOPERATIONパケットを送信しネットワークが使用可能かどうかを確認した上でホストリストを取得。
 * ・BR_ENTRYをブロードキャスト。
 * ・パケットを受信した上で、ホストリストを再度取得。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::Login( string nickname, string groupName )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;

	SendNoOperation();
	UpdateHostList();

#if defined(DEBUG) || !defined(NDEBUG)
	memset( sendBuf, 0, MAX_UDPBUF );
#endif
	if ( nickname != "" ) {
		Nickname = nickname;
	} else {
		Nickname = LoginName;
	}
	GroupName = groupName;
	optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s", Nickname.c_str() );
	optBuf[ optBufLen ] = '\0';
	optBufLen++;
	snprintf( &optBuf[ optBufLen ], sizeof( optBuf ) - optBufLen - 1, "%s", GroupName.c_str() );
	optBufLen += GroupName.size();
	optBuf[ optBufLen ] = '\0';
	
	IpMsgPrintBuf( "Login:sendBuf", sendBuf, MAX_UDPBUF );
#ifdef WITH_OPENSSL
	if ( encryptionCapacity != 0UL ) {
		sendBufLen = CreateNewPacketBuffer( IPMSG_BR_ENTRY | IPMSG_FILEATTACHOPT | IPMSG_ENCRYPTOPT,
											  LoginName, HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
	} else {
#endif	//WITH_OPENSSL
		sendBufLen = CreateNewPacketBuffer( IPMSG_BR_ENTRY | IPMSG_FILEATTACHOPT,
											  LoginName, HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
#ifdef WITH_OPENSSL
	}
#endif	//WITH_OPENSSL
	SendBroadcast( sendBuf, sendBufLen );
	RecvPacket();

	UpdateHostList();

}

/**
 * ログアウト（サービス脱退通知）。
 * ・BR_EXITをブロードキャスト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::Logout()
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( IPMSG_BR_EXIT,
										  LoginName, HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( sendBuf, sendBufLen );
	RecvPacket();
}

/**
 * ホストリスト取得。
 * @retval エージェントが保持しているHostListオブジェクト
 * @retval ホストリスト
 */
HostList&
IpMessengerAgent::GetHostList()
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
IpMessengerAgent::UpdateHostList()
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	hostList.clear();
	sendBufLen = CreateNewPacketBuffer( IPMSG_BR_ISGETLIST2,
										  LoginName, HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( sendBuf, sendBufLen );
	int pcount = RecvPacket();
	//自分以外のホストが見付からないか５回リトライする間繰り返す
	for( int i = 0; i < 5; i++ ) {
		if ( pcount == 0 ){
			break;
		}
		pcount == RecvPacket();
	}

#if defined(DEBUG)
	printf("\n\n");
	printf("== M Y   H O S T L I S T ==============================>\n");
	vector<HostListItem>::iterator ix = hostList.begin();
	for( ; ix != hostList.end(); ix++ ){
		printf("ver[%s]adesc[%s]user[%s]host[%s]cno[%lu]ipa[%s]nick[%s]gr[%s]enc[%s]port[%lu]\n",
				ix->Version().c_str(),
				ix->AbsenceDescription().c_str(),
				ix->UserName().c_str(),
				ix->HostName().c_str(),
				ix->CommandNo(),
				ix->IpAddress().c_str(),
				ix->Nickname().c_str(),
				ix->GroupName().c_str(),
				ix->EncodingName().c_str(),
				ix->PortNo());
	}
	printf("<= M Y   H O S T L I S T ===============================\n");
#endif
	return hostList;
}

/**
 * 不在モードかどうかを判定。
 * @retval 設定済の不在モードを返す。
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgent::IsAbsence()
{
	return _IsAbsence;
}
/**
 * 不在モードをクリアする。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::ResetAbsence()
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
IpMessengerAgent::SetAbsence( string encoding, vector<AbsenceMode> absenceModes )
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
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
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
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
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
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
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
#ifdef WITH_OPENSSL
	if ( isSecret && EncryptMsg( host, (unsigned char*)optBuf, optBufLen, &optBufLen, sizeof( optBuf ) ) ) {
		isEncrypted = true;
	} else {
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%s", msg.c_str() );
	}
#endif	//WITH_OPENSSL
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
#ifdef WITH_OPENSSL
										  ( isEncrypted ? IPMSG_ENCRYPTOPT : 0UL ) |
#endif	//WITH_OPENSSL
										  ( isSecret ? IPMSG_SECRETOPT : 0UL ) |
										  ( _IsAbsence ? IPMSG_AUTORETOPT : 0UL ) |
										  ( isLockPassword ? IPMSG_PASSWORDOPT : 0UL ) |
										  ( files.size() > 0 ? IPMSG_FILEATTACHOPT : 0UL ) | opt,
										  packetNo,
										  LoginName, HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, addr );

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

	if ( _IsSaveSentMessage ){
		sentMsgList.append( message );
	}
#if defined(DEBUG)
	printf("sentMsgList.append() size=%d\n", sentMsgList.size() );
	fflush(stdout);
#endif

	RecvPacket();

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
IpMessengerAgent::EncryptMsg( HostListItem host, unsigned char *optBuf, int optBufLen, int *enc_optBufLen, int opt_size )
{
#ifdef WITH_OPENSSL
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
		return false;
	}

	RSA *rsa = RSA_new();
	rsa->e = BN_new();
	if ( BN_hex2bn( &rsa->e, host.EncryptMethodHex().c_str() ) == 0 ){
		printf( "BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));
	}
	rsa->n = BN_new();
	if ( BN_hex2bn( &rsa->n, host.PubKeyHex().c_str() ) == 0 ){
		printf( "BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));
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
		return false;
	}
	int enc_key_size = RSA_size( rsa );
	unsigned char *enc_key = (unsigned char *)calloc( enc_key_size + 1, 1 );
	printf( "enc_key_size(%d)\n", enc_key_size );
	if ( enc_key == NULL ){
		return false;
	}
	//共通鍵をRSA公開鍵で暗号化。
	int enc_key_len = RSA_public_encrypt( key_bytes_size, sharekey, enc_key, rsa, RSA_PKCS1_PADDING );
	if ( enc_key_len < 0 ) {
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
		free( enc_key );
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
		free( enc_key );
		free( enc_buf );
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
	free( enc_key );
	free( enc_buf );
	free( out_buf );
	if ( opt_size > *enc_optBufLen ) {
		return true;
	}

	return false;
#else	//WITH_OPENSSL
	return false;
#endif	//WITH_OPENSSL
}

/**
 * メッセージ復号化。
 * @param packet パケットオブジェクト（参照）
 * @retval true:復号化OK、false:復号化NG
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgent::DecryptMsg( Packet &packet )
{
#ifdef WITH_OPENSSL
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
		printf("calloc 1\n");
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
		printf("IPMSG_RC2_40\n");
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_128 ) {
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_RC2_128;
		printf("IPMSG_RC2_128\n");
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_256 ) {
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_RC2_256;
		printf("IPMSG_RC2_256\n");
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
		printf("IPMSG_BF_128\n");
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_256 ){
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_BLOWFISH_256;
		printf("IPMSG_BF_256\n");
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
		printf("calloc 2\n");
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
		printf("%d:emc=[%s]", data_len, emc);
		emsg_buf[data_len] = (unsigned char)strtoul( (char *)emc, &dmyptr, 16 );
		printf("[%02x]\n", emsg_buf[data_len]);
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
#else	//WITH_OPENSSL
	return false;
#endif	//WITH_OPENSSL
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::DeleteBroadcastAddress( string addr )
{
	vector<struct sockaddr_in>::iterator net = FindBroadcastNetworkByAddress( addr );
	if ( net != broadcastAddr.end() ) {
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
IpMessengerAgent::AddBroadcastAddress( string addr )
{
	vector<struct sockaddr_in>::iterator net = FindBroadcastNetworkByAddress( addr );
	if ( net != broadcastAddr.end() ) {
		return;
	}
	struct sockaddr_in add_addr;
	add_addr.sin_family = AF_INET;
	add_addr.sin_port = htons(IPMSG_DEFAULT_PORT);
	add_addr.sin_addr.s_addr = inet_addr( addr.c_str() );
	broadcastAddr.push_back( add_addr );
}

/**
 * 登録済のブロードキャストアドレスを検索し、該当するsockaddr_in構造体を返却する。
 * @param addr ブロードキャストアドレス文字列
 * @retval sockaddr_in構造体
 * 注：このメソッドはスレッドセーフでない。
 */
vector<struct sockaddr_in>::iterator
IpMessengerAgent::FindBroadcastNetworkByAddress( string addr )
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
 * ホストリストをIPアドレスで検索し、該当するHostListItemを返却する。
 * @param addr IPアドレス文字列
 * @retval HostListItem
 * 注：このメソッドはスレッドセーフでない。
 */
vector<HostListItem>::iterator
IpMessengerAgent::FindHostByAddress( string addr )
{
	for( vector<HostListItem>::iterator ix = hostList.begin(); ix < hostList.end(); ix++ ){
		if ( ix->IpAddress() == addr ) {
			return ix;
		}
	}
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
IpMessengerAgent::GetInfo( HostListItem host )
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
										  LoginName, HostName,
										  packetNoBuf, packetNoBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, addr );
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
IpMessengerAgent::GetAbsenceInfo( HostListItem host )
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
										  LoginName, HostName,
										  packetNoBuf, packetNoBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, addr );
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
IpMessengerAgent::GetGroupList()
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
IpMessengerAgent::DeleteNotify( RecievedMessage msg )
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
										  LoginName, HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, msg.MessagePacket().Addr() );
	return;
}

/**
 * 送信元にメッセージを開封したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

	if ( ( IPMSG_SECRETOPT & msg.MessagePacket().CommandOption() ) && !msg.IsConfirmed() ) {
		packetNoBufLen = snprintf( packetNoBuf, sizeof( packetNoBuf ), "%ld", msg.MessagePacket().PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_READMSG,
											  LoginName, HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( sendBuf, sendBufLen, msg.MessagePacket().Addr() );
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
IpMessengerAgent::AcceptConfirmNotify( SentMessage msg )
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
IpMessengerAgent::FindSentMessageByPacketNo( unsigned long PacketNo )
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
IpMessengerAgent::InitSend()
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
IpMessengerAgent::SendTcpPacket( int sd, char *buf, int size )
{
#if defined(DEBUG)
	printf("== S E N D   T C P ====================================>\n");
#endif
	IpMsgPrintBuf( "SendTcpPacket:SendTcpBufer", buf, size );
	int ret = 0;
	ret = send( sd, buf, size + 1, 0 );
#if defined(DEBUG)
	if ( ret <= 0 ) {
		printf("S E N D   T C P   F A I L E D\n");
	}
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
IpMessengerAgent::SendPacket( char *buf, int size, struct sockaddr_in to_addr )
{
#if defined(DEBUG)
	printf("== S E N D ============================================>\n");
	printf( "Send  %s(%d)\n", inet_ntoa( to_addr.sin_addr ), ntohs( to_addr.sin_port ) );
#endif
	IpMsgPrintBuf( "SendUdpPacket:SendUdpBuffer", buf, size );
	int ret = 0;
	ret = sendto( udp_sd, buf, size + 1, 0, ( struct sockaddr * )&to_addr, sizeof( to_addr ) );
#if defined(DEBUG)
	if ( ret <= 0 ) {
		printf("S E N D   F A I L E D\n");
	}
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
IpMessengerAgent::SendBroadcast( char *buf, int size )
{
#if defined(DEBUG)
	printf("== S E N D   B R O A D C A S T ========================>\n");
	for( vector<struct sockaddr_in>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
		printf( "Send To %s(%d)\n", inet_ntoa( ixaddr->sin_addr ), ntohs( ixaddr->sin_port ) );
	}
#endif
	IpMsgPrintBuf( "SendBroadcast:SendUdpBroadcastBuffer", buf, size );
	for( vector<struct sockaddr_in>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
		int ret = 0;
		ret = sendto( udp_sd, buf, size + 1, 0, ( struct sockaddr * )&(*ixaddr), sizeof( struct sockaddr ) );
		if ( ret <= 0 ) {
#if defined(DEBUG)
			printf("S E N D   F A I L E D\n");
#endif
		}
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
IpMessengerAgent::InitRecv()
{
	udp_sd = socket( AF_INET, SOCK_DGRAM, 0 );
	tcp_sd = socket( AF_INET, SOCK_STREAM, 0 );
	addr_recv.sin_family = AF_INET;
	addr_recv.sin_port = htons(IPMSG_DEFAULT_PORT);
	addr_recv.sin_addr.s_addr = INADDR_ANY;
	if ( bind(udp_sd, (struct sockaddr *)&addr_recv, sizeof(addr_recv)) != 0 ){
		perror("bind(udp)");
		close( tcp_sd );
		return;
	}
	if ( tcp_sd >= 0 && bind(tcp_sd, (struct sockaddr *)&addr_recv, sizeof(addr_recv)) != 0 ){
		perror("bind(tcp)");
		close( tcp_sd );
		return;
	}

	int yes = 1;
	if ( setsockopt(udp_sd, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof(yes)) != 0 ) {
		perror("setsockopt(broadcast)");
		return;
	}
	
	int buf_size = MAX_SOCKBUF, buf_minsize = MAX_SOCKBUF / 2;
	if ( setsockopt(udp_sd, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(udp_sd, SOL_SOCKET, SO_SNDBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(sendbuf)");
		return;
	}
	if ( setsockopt(udp_sd, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(udp_sd, SOL_SOCKET, SO_RCVBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(recvbuf)");
		return;
	}
	if ( tcp_sd >= 0 && setsockopt(tcp_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) != 0 ) {
		perror("setsockopt(reuseaddr)");
		return;
	}
	if ( tcp_sd >= 0 && listen(tcp_sd, 5 ) != 0 ) {
		perror("setsockopt(reuseaddr)");
		return;
	}
//	in_addr_t my_if = inet_addr( HostAddress.c_str() );
//	setsockopt(udp_sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&my_if, sizeof(my_if));

	FD_ZERO( &rfds );
	FD_SET( udp_sd, &rfds );
	FD_SET( tcp_sd, &rfds );
}

/**
 * 受信処理（ユーザ向け）。
 * 注：このメソッドはスレッドセーフでない。
 */
int
IpMessengerAgent::Process()
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
IpMessengerAgent::RecvPacket()
{
	char buf[MAX_UDPBUF];
	int selret = 1;
	int ret = 0;
	int max_sd = udp_sd;
	if ( udp_sd < tcp_sd ){
		max_sd = tcp_sd;
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
			printf( "== R E C V ============================================>\n");
			printf( "selret == %d\n", selret );
#endif
			struct sockaddr_in sender_addr;
			socklen_t sender_addr_len = sizeof( sender_addr );
			int sz = 0;
			if ( FD_ISSET( udp_sd, &fds ) ){
				sz = recvfrom( udp_sd, buf, sizeof( buf ), 0, (struct sockaddr *)&sender_addr, &sender_addr_len );
			} else if ( FD_ISSET( tcp_sd, &fds ) ){
				tcp_socket = accept( tcp_sd, (struct sockaddr *)&sender_addr, &sender_addr_len );
				if ( tcp_socket < 0 ) {
					perror("accept");
				}
				sz = recv( tcp_socket, buf, sizeof( buf ), 0 );
#if defined(INFO) || !defined(NDEBUG)
				printf("recv buf[%s]\n", buf );
#endif
			} else {
				continue;
			}
			Packet packet = DismantlePacketBuffer( buf, sz, sender_addr );
#if defined(DEBUG)
			printf( "send from %s(%d)\n", inet_ntoa( sender_addr.sin_addr ), ntohs( sender_addr.sin_port ) );
			printf( "VersionNo    [%ld]\n", packet.VersionNo() );
			printf( "PacketNo     [%ld]\n", packet.PacketNo() );
			printf( "CommandMode  [%ld][%s]\n", packet.CommandMode(), GetCommandString( packet.CommandMode() ).c_str() );
			printf( "CommandOption[%ld]\n", packet.CommandOption() );
			printf( "HostName     [%s]\n", packet.HostName().c_str() );
			printf( "UserName     [%s]\n", packet.UserName().c_str() );
			printf( "<= R E C V =============================================\n");
#endif
			packet.setTcpSocket( tcp_socket );
			ret++;
			pack_que.push( packet );
		}
	}
	while( !pack_que.empty() ) {
		DoRecvCommand( pack_que.front() );
		pack_que.pop();
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("sentMsgList.size=%d\n", sentMsgList.size() );
	fflush(stdout);
#endif
	time_t tryNow = time( NULL );
	for( vector<SentMessage>::iterator ixmsg = sentMsgList.begin(); ixmsg != sentMsgList.end(); ixmsg++ ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("ixmsg=%p\n", &ixmsg );
		fflush(stdout);
#endif
		if ( needSendRetry( *ixmsg, tryNow ) ) {
			//再送信
			ixmsg->setRetryCount( ixmsg->RetryCount() + 1 );
			ixmsg->setPrevTry( tryNow );
		}
		if ( ixmsg->RetryCount() > 5 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("Retry Max Over\n");
#endif
			ixmsg->setRetryCount( 0 );
			ixmsg->setIsRetryMaxOver( true );
		}
	}
	return ret;
}

bool
IpMessengerAgent::isRetryMaxOver( SentMessage msg, int retryCount )
{
	if ( msg.RetryCount() > SENDMSG_RETRY_MAX ) {
		return true;
	}
	return false;
}
bool
IpMessengerAgent::needSendRetry( SentMessage msg, time_t tryNow )
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
IpMessengerAgent::DoRecvCommand( Packet packet )
{
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
IpMessengerAgent::UdpRecvEventNoOperation( Packet packet )
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
IpMessengerAgent::SendNoOperation()
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( IPMSG_NOOPERATION,
										  LoginName, HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( sendBuf, sendBufLen );
	return 0;
}

/**
 * 電文受信イベント：BR_ENTRY
 * ・送信元にANSENTRYを送信する。
 * ・不在モードの場合、不在として送信。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventBrEntry( Packet packet )
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
										  LoginName, HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}


/**
 * 電文送信（BR_ABSENCE）
 * ・不在通知／不在解除電文を送信する。
 */
int
IpMessengerAgent::SendAbsence()
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
										  LoginName, HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( sendBuf, sendBufLen );
	return 0;
}

/**
 * 電文受信イベント：BR_ABSENCE
 * ・自分のホストリストを更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventBrAbsence( Packet packet )
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
IpMessengerAgent::UdpRecvEventBrExit( Packet packet )
{
#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrExit\n");
#endif
	hostList.DeleteHost( packet.HostName() );
	return 0;
}

/**
 * 電文受信イベント：BR_RECVMSG
 * ・自分の送信済メッセージリストの該当メッセージに送信済フラグを立てる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventRecvMsg( Packet packet )
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
IpMessengerAgent::UdpRecvEventReadMsg( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;
	
	if ( packet.CommandOption() & IPMSG_READCHECKOPT ) {
		packetNoBufLen = snprintf( packetNoBuf, sizeof( packetNoBuf ), "%ld", packet.PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_ANSREADMSG,
											  LoginName, HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( sendBuf, sendBufLen, packet.Addr() );
	}

	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	vector<SentMessage>::iterator sentMsg = FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmed( true );
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
IpMessengerAgent::UdpRecvEventDelMsg( Packet packet )
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
IpMessengerAgent::UdpRecvEventAnsReadMsg( Packet packet )
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
IpMessengerAgent::UdpRecvEventSendMsg( Packet packet )
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
												  LoginName, HostName,
												  packetNoBuf, packetNoBufLen,
												  sendBuf, sizeof( sendBuf ) );
#if defined(INFO) || !defined(NDEBUG)
printf("Send(%s)\n", sendBuf);
#endif
			SendPacket( sendBuf, sendBufLen, packet.Addr() );
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
	message.setFiles( files );
	if ( _IsSaveRecievedMessage ){
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
IpMessengerAgent::UdpRecvEventBrIsGetList( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrIsGetList\n");
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_OKGETLIST,
										  LoginName, HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_ISGETLIST2
 * ・OKGETLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventBrIsGetList2( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvBrIsGetList2\n");
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_OKGETLIST,
										  LoginName, HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_GETLIST
 * ・ANSLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventGetList( Packet packet )
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
										  LoginName, HostName,
										  hosts.c_str(), hosts.length(),
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_OKGETLIST
 * ・GETLISTを投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventOkGetList( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvOkGetList[%s]\n", packet.Option().c_str());
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETLIST,
										  LoginName, HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_ANSENTRY
 * ・何もしない。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventAnsEntry( Packet packet )
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
//	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_ANSLIST
 * ・要求に応じたホストリストの部分をGETLISTに詰めて投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventAnsList( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char nextbuf[1024];

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvAnsList\n");
#endif
	if ( hostList.size() == 0 ) {
		HostListItem myHost;
		myHost.setUserName( LoginName );
		myHost.setHostName( HostName );
		myHost.setCommandNo( packet.CommandMode() | packet.CommandOption() );
		myHost.setIpAddress( HostAddress );
		myHost.setNickname( Nickname );
		myHost.setGroupName( GroupName );
		myHost.setPortNo( IPMSG_DEFAULT_PORT );
		hostList.AddHost( myHost );
	}
	int nextstart = CreateHostList( packet.Option().c_str(), packet.Option().length() );
	if ( nextstart > 0 ) {
		int nextbuf_len = snprintf( nextbuf, sizeof( nextbuf ), "%d", hostList.size() + 1 );
#if defined(INFO) || !defined(NDEBUG)
		printf("nextbuf_len = %d\n", nextbuf_len );
#endif
		sendBufLen = CreateNewPacketBuffer( IPMSG_GETLIST,
											  LoginName, HostName,
											  nextbuf, nextbuf_len,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( sendBuf, sendBufLen, packet.Addr() );
	}
	return 0;
}

/**
 * 電文受信イベント：BR_GETINFO
 * ・バージョン情報をSENDINFOに詰めて投げる。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventGetInfo( Packet packet )
{
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	string version = IPMSG_AGENT_VERSION;

#if defined(INFO) || !defined(NDEBUG)
	printf("UdpRecvGetInfo[%s]\n", packet.Option().c_str());
#endif
	sendBufLen = CreateNewPacketBuffer( IPMSG_SENDINFO,
										  LoginName, HostName,
										  version.c_str(), version.length(),
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_SENDINFO
 * ・取得したバージョン情報をホストリストに更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventSendInfo( Packet packet )
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
IpMessengerAgent::UdpRecvEventGetAbsenceInfo( Packet packet )
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
										  LoginName, HostName,
										  AbsenceDescription.c_str(), AbsenceDescription.length(),
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( sendBuf, sendBufLen, packet.Addr() );
	return 0;
}

/**
 * 電文受信イベント：BR_SENDABSENCEINFO
 * ・取得した不在詳細情報をホストリストに更新する。
 * @param packet パケットオブジェクト
 */
int
IpMessengerAgent::UdpRecvEventSendAbsenceInfo( Packet packet )
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
IpMessengerAgent::FindSentMessageByPacket( Packet packet )
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
IpMessengerAgent::TcpRecvEventGetFileData( Packet packet )
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

	vector<SentMessage>::iterator msg = IpMessengerAgent::GetInstance()->FindSentMessageByPacket( *packet );
	if ( msg == IpMessengerAgent::GetInstance()->SentMessageListEnd() ){
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
	IpMessengerAgent::GetInstance()->SendFile( packet->TcpSocket(), FoundFile->FullPath(), GetSendFileOffsetInPacket( *packet ) );
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
IpMessengerAgent::UdpRecvEventReleaseFiles( Packet packet )
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
IpMessengerAgent::UdpRecvEventGetPubKey( Packet packet )
{
#ifdef WITH_OPENSSL
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
											  LoginName, HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( sendBuf, sendBufLen, packet.Addr() );
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
IpMessengerAgent::UdpRecvEventAnsPubKey( Packet packet )
{
#ifdef WITH_OPENSSL
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
IpMessengerAgent::TcpRecvEventGetDirFiles( Packet packet )
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
IpMessengerAgent::SendDirData( int sock, string cd, string dir, vector<string> &files )
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
IpMessengerAgent::SendFile( int sock, string FileName, off_t offset )
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
		if ( _IsAbortDownloadAtFileChanged ){
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
IpMessengerAgent::CreateAttachedFileList( const char *option, AttachFileList &files )
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
IpMessengerAgent::CreateHostList( const char *hostListBuf, int buf_len )
{
	int alloc_size = buf_len + 1;
	int add_count = 0;
	char *hostListTmpPtr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *hostListTmpBuf = (char *)calloc( alloc_size, 1 );
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
		if ( token == NULL ) break;
		// ADD HOSTLIST
		hostList.DeleteHost( item.HostName() );
		hostList.AddHost( item );

#ifdef WITH_OPENSSL
		char sendBuf[MAX_UDPBUF];
		int sendBufLen;
		char optBuf[MAX_UDPBUF];
		int optBufLen;
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx", encryptionCapacity );
		sendBufLen = CreateNewPacketBuffer( IPMSG_GETPUBKEY,
											  LoginName, HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( item.PortNo() );
		addr.sin_addr.s_addr = inet_addr( item.IpAddress().c_str() );
		SendPacket( sendBuf, sendBufLen, addr );
#endif

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
IpMessengerAgent::GetRecievedMessageCount()
{
	return recvMsgList.size();
}

/**
 * 受信メッセージを一個取り出し、受信メッセージリストから削除する。
 * @retval 受信メッセージオブジェクト。
 */
RecievedMessage
IpMessengerAgent::PopRecievedMessage()
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
IpMessengerAgent::GetSentMessages()
{
	return sentMsgList.GetMessageList();
}

/**
 * 送信済メッセージリストのコピーを取得する。
 * @retval 送信済メッセージリストのコピー。
 */
vector<SentMessage>
IpMessengerAgent::CloneSentMessages()
{
	vector<SentMessage> *msgList = sentMsgList.GetMessageList();
	return *msgList;
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
IpMessengerAgent:: CreateNewPacketBuffer(long cmd, long packetNo, string user, string host, const char *opt, int optLen, char *buf, int size )
{
#if defined(INFO) || !defined(NDEBUG)
	printf( "CMD[%s]\n", GetCommandString( GET_MODE( cmd ) ).c_str() );
#endif
	memset( buf, 0, size );
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
IpMessengerAgent:: CreateNewPacketBuffer(long cmd, string user, string host, const char *opt, int optLen, char *buf, int size )
{
	long packetNo = random();
	return CreateNewPacketBuffer(cmd, packetNo, user, host, opt, optLen, buf, size );
}

/**
 * コマンド文字列を返す。
 * @param cmd コマンド
 * @retval コマンド文字列
 */
string
IpMessengerAgent::GetCommandString(long cmd )
{
	switch( cmd ){
		case IPMSG_NOOPERATION:     return "IPMSG_NOOPERATION";//
		case IPMSG_BR_ENTRY:        return "IPMSG_BR_ENTRY";//
		case IPMSG_BR_EXIT:         return "IPMSG_BR_EXIT";//
		case IPMSG_ANSENTRY:        return "IPMSG_ANSENTRY";//
		case IPMSG_BR_ABSENCE:      return "IPMSG_BR_ABSENCE";//
		case IPMSG_BR_ISGETLIST:    return "IPMSG_BR_ISGETLIST";//
		case IPMSG_OKGETLIST:       return "IPMSG_OKGETLIST";//
		case IPMSG_GETLIST:         return "IPMSG_GETLIST";//
		case IPMSG_ANSLIST:         return "IPMSG_ANSLIST";//
		case IPMSG_BR_ISGETLIST2:   return "IPMSG_BR_ISGETLIST2";//
		case IPMSG_SENDMSG:         return "IPMSG_SENDMSG";//
		case IPMSG_RECVMSG:         return "IPMSG_RECVMSG";
		case IPMSG_READMSG:         return "IPMSG_READMSG";
		case IPMSG_DELMSG:          return "IPMSG_DELMSG";
		case IPMSG_ANSREADMSG:      return "IPMSG_ANSREADMSG";
		case IPMSG_GETINFO:         return "IPMSG_GETINFO";
		case IPMSG_SENDINFO:        return "IPMSG_SENDINFO";
		case IPMSG_GETABSENCEINFO:  return "IPMSG_GETABSENCEINFO";
		case IPMSG_SENDABSENCEINFO: return "IPMSG_SENDABSENCEINFO";
		case IPMSG_GETFILEDATA:     return "IPMSG_GETFILEDATA";
		case IPMSG_RELEASEFILES:    return "IPMSG_RELEASEFILES";
		case IPMSG_GETDIRFILES:     return "IPMSG_GETDIRFILES";
		case IPMSG_GETPUBKEY:       return "IPMSG_GETPUBKEY";
		case IPMSG_ANSPUBKEY:       return "IPMSG_ANSPUBKEY";
	}
	return "no match";
}

/**
 * 受信バッファからパケットオブジェクトを生成する。
 * @param packet_buf 受信バッファ
 * @param size 受信バッファのサイズ
 * @param sender 送信元アドレス
 * @retval パケットオブジェクト
 */
Packet
IpMessengerAgent::DismantlePacketBuffer( char *packet_buf, int size, struct sockaddr_in sender )
{
	Packet ret;
	int alloc_size = size + 1;
	char *packet_tmp_buf;
	char *packet_tmp_ptr;
	char *nextpos;
	char *token;
	char *ptrdmy;

	ret.setAddr( sender );
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
	return ret;
}

//end of source
