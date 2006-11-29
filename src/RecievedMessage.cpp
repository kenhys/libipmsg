/**
 * IP メッセンジャライブラリ(Unix用)
 * 受信メッセージクラス。
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
using namespace std;

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
		close( sock );
		return false;
	}
	fchmod( fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	char readbuf[MAX_UDPBUF];
	long long readSize = 0LL;
	long long wroteSize = 0LL;
	time_t startTime = time( NULL );
	int read_len = recv( sock, readbuf, file.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : file.FileSize() - readSize, 0 );
	if ( read_len < 0 ) {
		perror("recv");
		close( sock );
		close( fd );
		return false;
	}
	readSize += read_len;
	while( read_len > 0 ){
		int wrote_len = write( fd, readbuf, read_len );
		if ( wrote_len < 0 ) {
			perror("write");
			close( sock );
			close( fd );
			return false;
		}
		wroteSize += wrote_len;
		read_len = recv( sock, readbuf, file.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : file.FileSize() - readSize, 0 );
		if ( read_len < 0 ) {
			perror("recv");
			close( sock );
			close( fd );
			return false;
		}
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
IpMsgPrintBuf( "DownloadDir:readbuf", readbuf, read_len );
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
IpMsgPrintBuf( "DownloadDir:readbuf2", readbuf, read_len );
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
//IpMsgPrintBuf( "DownloadDir:readbuf3", readbuf, read_len );
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
//IpMsgPrintBuf( "DownloadDir:readbuf4", readbuf, read_len );
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
