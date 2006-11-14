/**
 * IP メッセンジャライブラリ(Unix用)
 * WindowsのIPメッセンジャーと通信するライブラリ。
 * このライブラリはスレッドセーフでない。注意が必要。
 * プロトコルを実装したもので他の言語との実装と通信できるように
 * Shift_JIS限定にはなっていない。
 * 但し、プロトコルの電文中の区切り文字として'\0'を使うことが有るため、
 * UTF16やUCSの様な、文字コードに'\0'を含む文字コードには対応していない。
 *
 * このライブラリのバージョンでは、暗号系はOpenSSLのcryptoライブラリを使用するため、
 * 公開鍵はRSA512,1024,2048 共通鍵はRC2-40,RC2-128,RC2-256,BLOWFISH-128,BLOWFISH-256
 * をサポートできる。（通信先のクライアントの最強強度の暗号をネゴシエートして自動選択する。）
 * 但し、Windows版は（ソース中のコメントによれば）「手抜き」の実装なので最強強度で選択
 * してしまうと、Windows版は自己の能力を虚偽申請しているので暗号化されたパケットを復号
 * 出来ないことに注意。
 *
 * Windows版ではRSA512+RC2-40, RSA1024+BLOWFISH128しかサポートしていないが
 * これはWINCOMPATマクロによりサポートする。（WINCOMPATを定義すると、Windows版の自己能力の虚偽申請に
 * 合わせて暗号化方法を選択する。）
 *
 * ダウンロード周りファイル名の変換はファイル名コンバータオブジェクトで行う。
 * ローカルファイルシステムのエンコーディングとメッセージのエンコーディングの相互変換するオブジェクトで、
 * ライブラリとしては仮想基底クラスのFileNameConverterと変換を行わないNullFileNameConverter(デフォルト)
 * を提供する。アプリケーションは必要に応じて、FileNameConverterを継承して、ファイル名変換ロジック
 * を組み込む必要がある。（Windows版はShift_JIS(CP932)、一般のUnixではeucJP or UTF-8だから。）
 *
 * コマンドオプションの実装や、リトライ等の実装は適当であるので使用には注意が必要。
 * ログはアプリ側の実装とし、当方は関知しない。
 *
 * 各リスト（ホストリスト、受信メッセージリスト、送信メッセージリスト、添付ファイルリスト）は
 * 便利APIがあると便利だと思うがオイオイ実装する。
 *
 * automake/autoconf化もオイオイ実装する。
 *
 * 心残り
 * ・全体的にスレッドセーフでない。
 * ・クラスの分割がいまいち。
 * ・スタック上のインスタンスが多すぎる。もっとヒープ上にとるべき。
 * 
 * @version 0.1.0
 * @author kuninobu niki(nikikuni@yahoo.co.jp)
 * @created 2006.8.7(proto-type implimentation started)
 * @updated 2006.9.5(rc1,encrypt-decrypt function completed)
 * @target Linux(Main test environment), and Unix(no test), and more Unix clones.
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
using namespace std;

#if defined(DEBUG) || defined(INFO)
inline void PrintBuf( char* bufname, char *buf, int size );
#else
#define PrintBuf( bufname, buf,size )
#endif

/**
 * ファイル受信処理。
 * ・サーバにファイル受信要求パケットを送信し、ファイルを受信する。
 * 注：このメソッドはスレッドセーフでない。
 */
bool
RecievedMessage::DownloadFile( AttachFile &file, string saveFileNameFullPath, DownloadInfo& info )
{
	struct sockaddr_in svr_addr;
	int sock = socket( AF_INET, SOCK_STREAM, 0 );

	svr_addr = MessagePacket().Addr();
#if defined(DEBUG)
printf("IP[%s]\n", inet_ntoa( svr_addr.sin_addr ) );
fflush(stdout);
#endif
	if ( connect( sock, (struct sockaddr *)&svr_addr, sizeof( svr_addr ) ) != 0 ){
#if defined(DEBUG)
		printf("errno=[%s][%d]\n", strerror(errno), errno);
#endif
		herror("connect");
		return false;
	}

	IpMessengerAgent *agent = IpMessengerAgent::GetInstance();

	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx:%x:0", MessagePacket().PacketNo(), file.FileId() );
	sendBufLen = agent->CreateNewPacketBuffer( IPMSG_GETFILEDATA,
												 agent->GetLoginName(), agent->GetHostName(),
												 optBuf, optBufLen,
												 sendBuf, sizeof( sendBuf ) );
	agent->SendTcpPacket( sock, sendBuf, sendBufLen );
	file.setIsDownloading( true );
	int fd = open( saveFileNameFullPath.c_str(), O_WRONLY | O_CREAT );
	if ( fd < 0 ){
		perror("open");
		return false;
	}
	fchmod( fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	char readbuf[MAX_UDPBUF];
	long long readSize = 0LL;
	long long wroteSize = 0LL;
	time_t startTime = time( NULL );
	int read_len = recv( sock, readbuf, file.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : file.FileSize() - readSize, 0 );
	readSize += read_len;
	while( read_len > 0 ){
		int wrote_len = write( fd, readbuf, read_len );
		wroteSize += wrote_len;
		read_len = recv( sock, readbuf, file.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : file.FileSize() - readSize, 0 );
		readSize += read_len;
	}
	printf("close");
	close( fd );
	close( sock );
	struct utimbuf ubuf;
	ubuf.actime = ubuf.modtime = file.MTime();
	utime( saveFileNameFullPath.c_str(), &ubuf );
	file.setIsDownloading( false );
	file.setIsDownloaded( true );
	info.setSize( readSize );
	info.setTime( time( NULL ) - startTime );
	info.setFileCount( 1L );
	return true;
}

/**
 * ディレクトリ受信処理。
 * ・サーバにディレクトリ受信要求パケットを送信し、ディレクトリを受信する。
 * 注：このメソッドはスレッドセーフでない。
 */
bool
RecievedMessage::DownloadDir( AttachFile &file, string saveName, string saveBaseDir, DownloadInfo& info )
{
	NullFileNameConverter *codec = new NullFileNameConverter();
	bool ret = DownloadDir( file, saveName, saveBaseDir, info, codec );
	delete codec;
	return ret;
}

/**
 * ディレクトリ受信処理（ファイル名コンバータオプション付き）。
 * ・サーバにディレクトリ受信要求パケットを送信し、ディレクトリを受信する。
 * 注：このメソッドはスレッドセーフでない。
 */
bool
RecievedMessage::DownloadDir( AttachFile &file, string saveName, string saveBaseDir, DownloadInfo& info, FileNameConverter *conv )
{
	if ( conv == NULL ) {
		return false;
	}
	struct stat st;
	string saveBaseDirFormal = saveBaseDir;
	if ( saveBaseDirFormal.at( saveBaseDirFormal.length() - 1 ) != '/' ) {
		saveBaseDirFormal = saveBaseDir + "/";
	}
	string saveDir = saveBaseDirFormal + saveName + "/";

	if ( stat( saveBaseDir.c_str(), &st ) != 0 ) {
		perror("stat");
		printf("saveBaseDir == [%s]\n", saveBaseDir.c_str());
		return false;
	}
	if ( mkdir( saveDir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 ) {
		perror("mkdir");
		printf("saveDir == [%s]\n", saveDir.c_str());
		return false;
	}

	struct sockaddr_in svr_addr;
	int sock = socket( AF_INET, SOCK_STREAM, 0 );

	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = IPMSG_DEFAULT_PORT;
	svr_addr.sin_addr = MessagePacket().Addr().sin_addr;
	svr_addr = MessagePacket().Addr();
	if ( connect( sock, (struct sockaddr *)&svr_addr, sizeof( svr_addr ) ) != 0 ){
		perror("connect");
		return false;
	}

	IpMessengerAgent *agent = IpMessengerAgent::GetInstance();

	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx:%x", MessagePacket().PacketNo(), file.FileId() );
	sendBufLen = agent->CreateNewPacketBuffer( IPMSG_GETDIRFILES,
												 agent->GetLoginName(), agent->GetHostName(),
												 optBuf, optBufLen,
												 sendBuf, sizeof( sendBuf ) );
	agent->SendTcpPacket( sock, sendBuf, sendBufLen );

	char readbuf[MAX_UDPBUF];
	bool isEob = false;
	bool isTopDir = true;
	vector<string> dir;
	dir.push_back( saveBaseDir );
	dir.push_back( saveName );

	file.setIsDownloading( true );
	long long totalReadSize = 0LL;
	long totalFileCount = 0L;
	time_t startTime = time( NULL );
#define HEADER_LENGTH_LEN	5
#if defined(DEBUG)
	printf("start DIR [%s]\n", AttachFile::CreateDirFullPath( dir ).c_str() );
#endif
	while( !isEob ){
#if defined(DEBUG)
		fflush(stdout);
		printf("saveBaseDirFormal[%s]\n", saveBaseDirFormal.c_str() );
		printf("AttachFile::CreateDirFullPath( dir )[%s]\n", AttachFile::CreateDirFullPath( dir ).c_str() );
#endif
		if ( saveBaseDirFormal == AttachFile::CreateDirFullPath( dir ) ) {
			isEob = true;
			break;
		}
		memset( readbuf, 0, sizeof( readbuf ) );
		int read_len = recv( sock, readbuf, HEADER_LENGTH_LEN, 0 );
		if ( read_len < 0 ) {
			perror("recv");
			isEob = true;
			break;
		}
#if defined(DEBUG)
printf("HEADER=%d\n", read_len );
PrintBuf( "DownloadDir:readbuf", readbuf, read_len );
#endif
		if ( read_len < HEADER_LENGTH_LEN ) {
			isEob = true;
			break;
		}
		readbuf[read_len] = 0;
		char *dmyptr;
		int header_len = strtoul( readbuf, &dmyptr, 16 );
		memset( readbuf, 0, sizeof( readbuf ) );
		read_len = recv( sock, readbuf, header_len - HEADER_LENGTH_LEN, 0 );
#if defined(DEBUG)
printf("NEXT RECV=%d\n", header_len - HEADER_LENGTH_LEN);
PrintBuf( "DownloadDir:readbuf2", readbuf, read_len );
#endif
		if ( read_len < 0 ) {
			perror("recv");
			isEob = true;
			break;
		}
		if ( read_len < header_len - HEADER_LENGTH_LEN ) {
			isEob = true;
			break;
		}
		readbuf[read_len] = 0;
		AttachFile f = AttachFile::AnalyzeHeader( readbuf, conv );
		if ( GET_FILETYPE( f.Attr() ) == IPMSG_FILE_REGULAR ) {
			long long readSize = 0LL;
			long long wroteSize = 0LL;
			string FullPath = AttachFile::CreateDirFullPath( dir ) + f.FileName();
			int fd = open( FullPath.c_str(), O_WRONLY | O_CREAT );
			if ( fd < 0 ){
				perror("open");
				return false;
			}
			fchmod( fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
			memset( readbuf, 0, sizeof( readbuf ) );
#if defined(DEBUG)
printf("FileSize=%lld\n", f.FileSize() );
fflush(stdout);
#endif
			int read_len = recv( sock, readbuf, f.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : f.FileSize() - readSize, 0 );
#if defined(DEBUG)
//PrintBuf( "DownloadDir:readbuf3", readbuf, read_len );
#endif
			readSize += read_len;
			while( read_len > 0 ){
				if ( read_len < 0 ) {
					perror("recv");
					isEob = true;
					break;
				}

				wroteSize += write( fd, readbuf, read_len );
				memset( readbuf, 0, sizeof( readbuf ) );
				read_len = recv( sock, readbuf, f.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : f.FileSize() - readSize, 0 );
#if defined(DEBUG)
//PrintBuf( "DownloadDir:readbuf4", readbuf, read_len );
#endif
				readSize += read_len;
				totalReadSize += read_len;
			}
			close( fd );
			struct utimbuf ubuf;
			ubuf.actime = ubuf.modtime = f.MTime();
			utime( FullPath.c_str(), &ubuf );
			info.setSize( totalReadSize );
			info.setTime( time( NULL ) - startTime );
			info.setFileCount( ++totalFileCount );
#if defined(DEBUG)
			printf("read=%lld,wrote=%lld\n", readSize, wroteSize);
#endif
		} else if ( GET_FILETYPE( f.Attr() ) == IPMSG_FILE_DIR ) {
			if ( isTopDir ) {
				isTopDir = false;
				continue;
			}
			dir.push_back( f.FileName().c_str() );
			string FullPath = f.CreateDirFullPath( dir );
			if ( mkdir( FullPath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 ) {
				perror("mkdir");
				isEob = true;
				break;
			}
			info.setTime( time( NULL ) - startTime );
			info.setFileCount( ++totalFileCount );
		} else if ( GET_FILETYPE( f.Attr() ) == IPMSG_FILE_RETPARENT ) {
			dir.pop_back();
		}
	}
#if defined(DEBUG)
	printf("close socket");
	fflush(stdout);
#endif
	close( sock );
	file.setIsDownloading( false );
	file.setIsDownloaded( true );
	info.setSize( totalReadSize );
	info.setTime( time( NULL ) - startTime );
	info.setFileCount( totalFileCount );
	return true;
}
