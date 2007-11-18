/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * ������å��������饹��
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"

using namespace ipmsg;

/**
 * ���󥹥ȥ饯����
 */
RecievedMessage::RecievedMessage()
{
	IPMSG_FUNC_ENTER( "RecievedMessage::RecievedMessage()" );
	IPMSG_FUNC_EXIT;
}

/**
 * ���ԡ����󥹥ȥ饯����
 * @param other ���ԡ����Υ��֥�������
 */
RecievedMessage::RecievedMessage( const RecievedMessage& other )
{
	IPMSG_FUNC_ENTER( "RecievedMessage::RecievedMessage( const RecievedMessage& other )" );
	CopyFrom( other );
	IPMSG_FUNC_EXIT;
}

/**
 * �����黻�ҡ�
 * @param other ���ԡ����Υ��֥�������
 * @retval �����󥹥���
 */
RecievedMessage&
RecievedMessage::operator=( const RecievedMessage& other )
{
	IPMSG_FUNC_ENTER( "RecievedMessage& RecievedMessage::operator=( const RecievedMessage& other )" );
	CopyFrom( other );
	IPMSG_FUNC_RETURN( *this );
}

/**
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
 */
void
RecievedMessage::CopyFrom( const RecievedMessage& other )
{
	IPMSG_FUNC_ENTER( "void RecievedMessage::CopyFrom( const RecievedMessage& other )" );
	_MessagePacket = other. _MessagePacket;
	_Message = other. _Message;
	_Recieved = other. _Recieved;
	_IsConfirmed = other. _IsConfirmed;
	_IsSecret = other. _IsSecret;
	_IsNoLogging = other. _IsNoLogging;
	_IsCrypted = other. _IsCrypted;
	_Host = other. _Host;
	_IsPasswordLock = other. _IsPasswordLock;
	_IsBroadcast = other. _IsBroadcast;
	_IsMulticast = other. _IsMulticast;
	_HasAttachFile = other. _HasAttachFile;
	_Files = other._Files;
	IPMSG_FUNC_EXIT;
}

/**
 * ���󥹥ȥ饯����
 * <ul>
 * <li>�����ѥ�å������ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 */
RecievedMessageList::RecievedMessageList()
{
	IPMSG_FUNC_ENTER( "RecievedMessageList::RecievedMessageList()" );
	IpMsgMutexInit( "RecievedMessageList::RecievedMessageList()", &messagesMutex, NULL );
	IPMSG_FUNC_EXIT;
}

/**
 * ���ԡ����󥹥ȥ饯����
 * @param other ���ԡ����Υ��֥�������
 */
RecievedMessageList::RecievedMessageList( const RecievedMessageList& other )
{
	IPMSG_FUNC_ENTER( "RecievedMessageList::RecievedMessageList( const RecievedMessageList& other )" );
	IpMsgMutexInit( "RecievedMessageList::RecievedMessageList(RecievedMessageList&)", &messagesMutex, NULL );
	Lock( "RecievedMessageList::RecievedMessageList(RecievedMessageList&)" );
	CopyFrom( other );
	Unlock( "RecievedMessageList::RecievedMessageList(RecievedMessageList&)" );
	IPMSG_FUNC_EXIT;
}

/**
 * �ǥ��ȥ饯����
 * <ul>
 * <li>�����ѥ�å������ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå������˴���</li>
 * </ul>
 */
RecievedMessageList::~RecievedMessageList()
{
	IPMSG_FUNC_ENTER( "RecievedMessageList::~RecievedMessageList()" );
	IpMsgMutexDestroy( "RecievedMessageList::~RecievedMessageList()", &messagesMutex );
	IPMSG_FUNC_EXIT;
}

/**
 * �����黻�ҡ�
 * @param other ���ԡ����Υ��֥�������
 * @retval �����󥹥���
 */
RecievedMessageList&
RecievedMessageList::operator=( const RecievedMessageList& other )
{
	IPMSG_FUNC_ENTER( "RecievedMessageList& RecievedMessageList::operator=( const RecievedMessageList& other )" );
	IpMsgMutexInit( "RecievedMessageList::operator=(RecievedMessageList&)", &messagesMutex, NULL );
	Lock( "RecievedMessageList::operator=(RecievedMessageList&)" );
	CopyFrom( other );
	Unlock( "RecievedMessageList::operator=(RecievedMessageList&)" );
	IPMSG_FUNC_RETURN( *this );
}

/**
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
 */
void
RecievedMessageList::CopyFrom( const RecievedMessageList& other )
{
	IPMSG_FUNC_ENTER( "void RecievedMessageList::CopyFrom( const RecievedMessageList& other )" );
	messages = other.messages;
	IPMSG_FUNC_EXIT;
}

/**
 * ������å������ꥹ�Ȥ��å�
 * @param pos ��å����Ƥ�����֤򼨤�ʸ����
 */
void
RecievedMessageList::Lock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void RecievedMessageList::Lock( const char *pos ) const" );
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &messagesMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * ������å������ꥹ�Ȥ򥢥��å�
 * @param pos �����å����Ƥ�����֤򼨤�ʸ����
 */
void
RecievedMessageList::Unlock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void RecievedMessageList::Unlock( const char *pos ) const" );
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &messagesMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ѥ�å������ꥹ�Ȥ���Ƭ�򼨤����ƥ졼�����֤���
 * @retval �����ѥ�å������ꥹ�Ȥ���Ƭ�򼨤����ƥ졼����
 */
std::vector<RecievedMessage>::iterator
RecievedMessageList::begin()
{
	IPMSG_FUNC_ENTER( "std::vector<RecievedMessage>::iterator RecievedMessageList::begin()" );
	IPMSG_FUNC_RETURN( messages.begin() );
}

/**
 * �����ѥ�å������ꥹ�Ȥ������ܣ��򼨤����ƥ졼�����֤���
 * @retval �����ѥ�å������ꥹ�Ȥ������ܣ��򼨤����ƥ졼����
 */
std::vector<RecievedMessage>::iterator
RecievedMessageList::end()
{
	IPMSG_FUNC_ENTER( "std::vector<RecievedMessage>::iterator RecievedMessageList::end()" );
	IPMSG_FUNC_RETURN( messages.end() );
}

/**
 * ���ꤵ�줿���ƥ졼���Ǽ����ѥ�å�����������ѥ�å������ꥹ�Ȥ��������롣
 * @param item ����оݤμ����ѥ�å������򼨤����ƥ졼����
 * @retval ������줿�����ѥ�å������μ������Ǥ򼨤����ƥ졼����
 */
std::vector<RecievedMessage>::iterator
RecievedMessageList::erase( std::vector<RecievedMessage>::iterator item )
{
	IPMSG_FUNC_ENTER( "std::vector<RecievedMessage>::iterator RecievedMessageList::erase( std::vector<RecievedMessage>::iterator item )" );
	Lock( "RecievedMessageList::erase()" );
	std::vector<RecievedMessage>::iterator ret = messages.erase( item );
	Unlock( "RecievedMessageList::erase()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �����ѥ�å������ꥹ�Ȥ˥�å��������ɲä��롣
 * @param item �����ѥ�å�������
 */
void
RecievedMessageList::append( const RecievedMessage &item )
{
	IPMSG_FUNC_ENTER( "void RecievedMessageList::append( const RecievedMessage &item )" );
	Lock( "RecievedMessageList::append()" );
	messages.push_back( item );
	Unlock( "RecievedMessageList::append()" );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ѥ�å������ꥹ�ȤθĿ����֤���
 * @retval �����ѥ�å������ꥹ�ȤθĿ���
 */
int
RecievedMessageList::size() const
{
	IPMSG_FUNC_ENTER( "int RecievedMessageList::size() const" );
	Lock( "RecievedMessageList::size()" );
	int ret = messages.size();
	Unlock( "RecievedMessageList::size()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �����ѥ�å������ꥹ�Ȥ򥯥ꥢ���롣
 */
void
RecievedMessageList::clear()
{
	IPMSG_FUNC_ENTER( "void RecievedMessageList::clear()" );
	Lock( "RecievedMessageList::clear()" );
	messages.clear();
	Unlock( "RecievedMessageList::clear()" );
	IPMSG_FUNC_EXIT;
}

/**
 * �ե��������������
 * <ul>
 * <li>�����Ф˥ե���������׵�ѥ��åȤ����������ե������������롣</li>
 * </ul>
 */
bool
RecievedMessage::DownloadFile( AttachFile &file, std::string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv, void *data )
{
	IPMSG_FUNC_ENTER( "bool RecievedMessage::DownloadFile( AttachFile &file, std::string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv, void *data )" );
#if defined(DEBUG)
	printf("RecievedMessage::DownloadFile\n" );fflush(stdout);
#endif
	bool ret = true;
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	IpMessengerEvent *event = agent->GetEventObject();
	FileNameConverter *converter = conv;
	if ( conv == NULL ){
		converter = new NullFileNameConverter();
	}

	info.setProcessing( true );
	info.setFile( file );
	info.setLocalFileName( saveFileNameFullPath );
	if ( event == NULL ) {
		ret = DownloadFilePrivate( NULL, file, saveFileNameFullPath, info, converter, data );
	} else {
		while( ret ) {
			event->DownloadStart( *this, file, info, data );
			if ( DownloadFilePrivate( event, file, saveFileNameFullPath, info, converter, data ) ) {
				event->DownloadEnd( *this, file, info, data );
				ret = true;
				break;
			} else {
				ret = event->DownloadError( *this, file, info, data );
			}
		}
	}
	if ( conv == NULL ){
		delete converter;
	}
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ե�����������������������
 * <ul>
 * <li>�����Ф˥ե���������׵�ѥ��åȤ����������ե������������롣</li>
 * </ul>
 */
bool
RecievedMessage::DownloadFilePrivate( IpMessengerEvent *event, AttachFile &file, std::string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv, void *data )
{
	IPMSG_FUNC_ENTER( "bool RecievedMessage::DownloadFilePrivate( IpMessengerEvent *event, AttachFile &file, std::string saveFileNameFullPath, DownloadInfo& info, FileNameConverter *conv, void *data )" );
	struct sockaddr_storage svr_addr;
	svr_addr = MessagePacket().Addr();
	int sock = socket( svr_addr.ss_family, SOCK_STREAM, 0 );
#if defined(DEBUG)
printf("RecievedMessage::DownloadFilePrivate saveFileNameFullPath[%s]\n", saveFileNameFullPath.c_str() );fflush(stdout);
#endif
	if ( connect( sock, (struct sockaddr *)&svr_addr, sizeof( svr_addr ) ) != 0 ){
		perror("connect");
		info.setProcessing( false );
		IPMSG_FUNC_RETURN( false );
	}

	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	size_t optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx:%x:0", MessagePacket().PacketNo(), file.FileId() );
	if ( optBufLen > sizeof( optBuf ) ) {
		IPMSG_FUNC_RETURN( false );
	}
	sendBufLen = agent->CreateNewPacketBuffer( IPMSG_GETFILEDATA,
												 agent->LoginName(), agent->HostName(),
												 optBuf, optBufLen,
												 sendBuf, sizeof( sendBuf ) );
	agent->SendTcpPacket( sock, sendBuf, sendBufLen );
	file.setIsDownloading( true );
	int fd = open( saveFileNameFullPath.c_str(), O_WRONLY | O_CREAT );
	if ( fd < 0 ){
		perror("open");
		close( sock );
		info.setProcessing( false );
		IPMSG_FUNC_RETURN( false );
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
		info.setProcessing( false );
		IPMSG_FUNC_RETURN( false );
	}
	readSize += read_len;
	while( read_len > 0 ){
		int wrote_len = write( fd, readbuf, read_len );
		if ( wrote_len < 0 ) {
			perror("write");
			close( sock );
			close( fd );
			info.setProcessing( false );
			IPMSG_FUNC_RETURN( false );
		}
		wroteSize += wrote_len;
		info.setSize( readSize );
		info.setTime( time( NULL ) - startTime );
		if ( event != NULL ) {
			event->DownloadProcessing( *this, file, info, data );
		}
#if defined(DEBUG)
		printf( "RecievedMessage::DownloadFilePrivate read_len = %d\n", read_len );fflush(stdout);
		printf( "RecievedMessage::DownloadFilePrivate readSize = %lld\n", readSize );fflush(stdout);
		printf( "RecievedMessage::DownloadFilePrivate file.FileSize() = %lld\n", file.FileSize() );fflush(stdout);
#endif
		if ( file.FileSize() == readSize ) {
			break;
		}
		read_len = recv( sock, readbuf, file.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : file.FileSize() - readSize, 0 );
		if ( read_len < 0 ) {
			perror("recv");
			close( sock );
			close( fd );
			info.setProcessing( false );
			IPMSG_FUNC_RETURN( false );
		}
		readSize += read_len;
	}
#if defined(DEBUG)
	printf("RecievedMessage::DownloadFilePrivate close");fflush(stdout);
#endif
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
	info.setProcessing( false );
	if ( event != NULL ) {
		event->DownloadProcessing( *this, file, info, data );
	}
	IPMSG_FUNC_RETURN( true );
}

/**
 * �ǥ��쥯�ȥ���������ʥե�����̾����С������ץ�����դ��ˡ�
 * <ul>
 * <li>�����Ф˥ǥ��쥯�ȥ�����׵�ѥ��åȤ����������ǥ��쥯�ȥ��������롣</li>
 * </ul>
 */
bool
RecievedMessage::DownloadDir( AttachFile &file, std::string saveName, std::string saveBaseDir, DownloadInfo& info, FileNameConverter *conv, void *data )
{
	IPMSG_FUNC_ENTER( "bool RecievedMessage::DownloadDir( AttachFile &file, std::string saveName, std::string saveBaseDir, DownloadInfo& info, FileNameConverter *conv, void *data )" );
#if defined(DEBUG)
	printf("RecievedMessage::DownloadDir\n" );fflush(stdout);
#endif
	bool ret = true;
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	IpMessengerEvent *event = agent->GetEventObject();
	FileNameConverter *converter = conv;
	if ( conv == NULL ){
		converter = new NullFileNameConverter();
	}
	
	info.setFile( file );
	info.setLocalFileName( GetSaveDir( saveName, saveBaseDir ) );
	info.setProcessing( true );
	if ( event == NULL ) {
		ret = DownloadDirPrivate( NULL, file, saveName, saveBaseDir, info, converter, data );
	} else {
		while( ret ) {
			event->DownloadStart( *this, file, info, data );
			if ( DownloadDirPrivate( event, file, saveName, saveBaseDir, info, converter, data ) ) {
				event->DownloadEnd( *this, file, info, data );
				ret = true;
				break;
			} else {
				ret = event->DownloadError( *this, file, info, data );
			}
		}
	}
	if ( conv == NULL ){
		delete converter;
	}
	IPMSG_FUNC_RETURN( ret );
}

/**
 * ���������줿�ǥ��쥯�ȥ�̾��������롣
 * @param dirName ���������Υǥ��쥯�ȥ�̾
 * @retval ��������Υǥ��쥯�ȥ�̾
 */
std::string
RecievedMessage::GetFormalDir( std::string dirName )
{
	IPMSG_FUNC_ENTER( "std::string RecievedMessage::GetFormalDir( std::string dirName )" );
	if ( dirName.at( dirName.length() - 1 ) != '/' ) {
		IPMSG_FUNC_RETURN( dirName + "/" );
	}
	IPMSG_FUNC_RETURN( dirName );
}

/**
 * ��¸����ե�����̾�ʤޤ��ϥǥ��쥯�ȥ�̾�ˤ�������롣
 * <ul>
 * <li>��¸��Υǥ��쥯�ȥꡢ�ե�����ʤޤ��ϥǥ��쥯�ȥ��̾
 * ���ʤ���ϥץ�ȥ���塢�ե�����̾�Τߤ�����Ǥ���١ˤ��Ȥ߹�碌�ƥե�ѥ������롣</li>
 * </ul>
 * @param saveName ��¸�ե�����ʤޤ��ϥǥ��쥯�ȥ�̾��̾
 * @param saveBaseDir ��¸��ǥ��쥯�ȥ�̾
 * @retval ��¸����ե�����ʤޤ��ϥǥ��쥯�ȥ�̾��
 */
std::string
RecievedMessage::GetSaveDir( std::string saveName, std::string saveBaseDir )
{
	IPMSG_FUNC_ENTER( "std::string RecievedMessage::GetSaveDir( std::string saveName, std::string saveBaseDir )" );
	IPMSG_FUNC_RETURN( GetFormalDir( saveBaseDir ) + saveName + "/" );
}

/**
 * �ǥ��쥯�ȥ���������ʥե�����̾����С������ץ�����դ��ˡ����������
 * <ul>
 * <li>�����Ф˥ǥ��쥯�ȥ�����׵�ѥ��åȤ����������ǥ��쥯�ȥ��������롣</li>
 * </ul>
 */
bool
RecievedMessage::DownloadDirPrivate( IpMessengerEvent *event, AttachFile &file, std::string saveName, std::string saveBaseDir, DownloadInfo& info, FileNameConverter *conv, void *data )
{
	IPMSG_FUNC_ENTER( "bool RecievedMessage::DownloadDirPrivate( IpMessengerEvent *event, AttachFile &file, std::string saveName, std::string saveBaseDir, DownloadInfo& info, FileNameConverter *conv, void *data )" );
	if ( conv == NULL ) {
		info.setProcessing( false );
		IPMSG_FUNC_RETURN( false );
	}
	struct stat st;
	std::string saveBaseDirFormal = GetFormalDir( saveBaseDir );
	std::string saveDir = GetSaveDir( saveName, saveBaseDir );

#if defined(DEBUG)
printf("RecievedMessage::DownloadDir saveName[%s]\n", saveName.c_str() );fflush(stdout);
printf("RecievedMessage::DownloadDir saveBaseDir[%s]\n", saveBaseDir.c_str() );fflush(stdout);
printf("RecievedMessage::DownloadDir saveDir[%s]\n", saveDir.c_str() );fflush(stdout);
printf("RecievedMessage::DownloadDir saveBaseDirFormal[%s]\n", saveBaseDirFormal.c_str() );fflush(stdout);
#endif
	if ( stat( saveBaseDir.c_str(), &st ) != 0 ) {
		perror("stat");
#if defined(DEBUG)
		printf("RecievedMessage::DownloadDir saveBaseDir == [%s]\n", saveBaseDir.c_str());fflush(stdout);
#endif
		info.setProcessing( false );
		IPMSG_FUNC_RETURN( false );
	}
	if ( mkdir( saveDir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 ) {
		perror("RecievedMessage::DownloadDir mkdir(1)");
#if defined(DEBUG)
		printf("RecievedMessage::DownloadDir saveDir == [%s]\n", saveDir.c_str());fflush(stdout);
#endif
		info.setProcessing( false );
		IPMSG_FUNC_RETURN( false );
	}

	struct sockaddr_storage svr_addr;
	svr_addr = MessagePacket().Addr();

	int sock = socket( svr_addr.ss_family, SOCK_STREAM, 0 );
	if ( connect( sock, (struct sockaddr *)&svr_addr, sizeof( svr_addr ) ) != 0 ){
		perror("RecievedMessage::DownloadDir connect");
		info.setProcessing( false );
		IPMSG_FUNC_RETURN( false );
	}

	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();

	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	size_t optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx:%x", MessagePacket().PacketNo(), file.FileId() );
	if ( optBufLen <= sizeof( optBuf ) ) {
		IPMSG_FUNC_RETURN( false );
	}
	sendBufLen = agent->CreateNewPacketBuffer( IPMSG_GETDIRFILES,
												 agent->LoginName(), agent->HostName(),
												 optBuf, optBufLen,
												 sendBuf, sizeof( sendBuf ) );
	agent->SendTcpPacket( sock, sendBuf, sendBufLen );

	char readbuf[MAX_UDPBUF];
	bool isEob = false;
	bool isTopDir = true;
	std::vector<std::string> dir;
	dir.push_back( saveBaseDir );
	dir.push_back( saveName );

	file.setIsDownloading( true );
	long long totalReadSize = 0LL;
	long totalFileCount = 0L;
	time_t startTime = time( NULL );
#define HEADER_LENGTH_LEN	5
#if defined(DEBUG)
	printf("RecievedMessage::DownloadDir start DIR [%s]\n", AttachFile::CreateDirFullPath( dir ).c_str() );fflush(stdout);
#endif
	while( !isEob ){
#if defined(DEBUG)
		printf("RecievedMessage::DownloadDir saveBaseDirFormal[%s]\n", saveBaseDirFormal.c_str() );fflush(stdout);
		printf("RecievedMessage::DownloadDir AttachFile::CreateDirFullPath( dir )[%s]\n", AttachFile::CreateDirFullPath( dir ).c_str() );fflush(stdout);
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
printf("RecievedMessage::DownloadDir header length=%d\n", read_len );fflush(stdout);
IpMsgPrintBuf( "RecievedMessage::DownloadDir readbuf", readbuf, read_len );
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
printf("RecievedMessage::DownloadDir Next recv length=%d\n", header_len - HEADER_LENGTH_LEN);fflush(stdout);
IpMsgPrintBuf( "RecievedMessage::DownloadDir DownloadDir:readbuf2", readbuf, read_len );
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
			std::string FullPath = AttachFile::CreateDirFullPath( dir ) + f.FileName();
			int fd = open( FullPath.c_str(), O_WRONLY | O_CREAT );
			if ( fd < 0 ){
				perror("open");
				info.setProcessing( false );
				IPMSG_FUNC_RETURN( false );
			}
			fchmod( fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
			memset( readbuf, 0, sizeof( readbuf ) );
#if defined(DEBUG)
printf("RecievedMessage::DownloadDir f.FileSize=%lld\n", f.FileSize() );fflush(stdout);
#endif
			int read_len = recv( sock, readbuf, f.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : f.FileSize() - readSize, 0 );
#if defined(DEBUG)
IpMsgPrintBuf( "RecievedMessage::DownloadDir readbuf3", readbuf, read_len );
#endif
			readSize += read_len;
			while( read_len > 0 ){
				if ( read_len < 0 ) {
					perror("recv");
					isEob = true;
					break;
				}
				int wrote_len = write( fd, readbuf, read_len );
				if ( wrote_len < 0 ) {
					perror("write");
					close( sock );
					close( fd );
					info.setProcessing( false );
					IPMSG_FUNC_RETURN( false );
				}
				wroteSize += wrote_len;
				info.setSize( totalReadSize );
				info.setTime( time( NULL ) - startTime );
				if ( event != NULL ) {
					event->DownloadProcessing( *this, file, info, data );
				}
				memset( readbuf, 0, sizeof( readbuf ) );
				read_len = recv( sock, readbuf, f.FileSize() - readSize > sizeof( readbuf ) ? sizeof( readbuf ) : f.FileSize() - readSize, 0 );
#if defined(DEBUG)
//IpMsgPrintBuf( "RecievedMessage::DownloadDir readbuf4", readbuf, read_len );
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
			printf("RecievedMessage::DownloadDir read=%lld,wrote=%lld\n", readSize, wroteSize);fflush(stdout);
#endif
		} else if ( GET_FILETYPE( f.Attr() ) == IPMSG_FILE_DIR ) {
			if ( isTopDir ) {
				isTopDir = false;
				continue;
			}
			dir.push_back( f.FileName().c_str() );
			std::string FullPath = f.CreateDirFullPath( dir );
			if ( mkdir( FullPath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 ) {
				perror("mkdir(2)");
				isEob = true;
				break;
			}
			info.setTime( time( NULL ) - startTime );
			info.setFileCount( ++totalFileCount );
			if ( event != NULL ) {
				event->DownloadProcessing( *this, file, info, data );
			}
		} else if ( GET_FILETYPE( f.Attr() ) == IPMSG_FILE_RETPARENT ) {
			dir.pop_back();
		}
	}
#if defined(DEBUG)
	printf("RecievedMessage::DownloadDir close socket");fflush(stdout);
#endif
	close( sock );
	file.setIsDownloading( false );
	file.setIsDownloaded( true );
	info.setSize( totalReadSize );
	info.setTime( time( NULL ) - startTime );
	info.setFileCount( totalFileCount );
	info.setProcessing( false );
	if ( event != NULL ) {
		event->DownloadProcessing( *this, file, info, data );
	}
	IPMSG_FUNC_RETURN( true );
}
