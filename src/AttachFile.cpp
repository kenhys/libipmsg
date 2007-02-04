/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * ź�եե����륯�饹��
 */
 
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"

using namespace ipmsg;

static int file_id = 0;

/**
 * ���󥹥ȥ饯����
 * <ul>
 * <li>�ե�����ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 */
AttachFileList::AttachFileList()
{
	IpMsgMutexInit( "AttachFileList::AttachFileList()", &filesMutex, NULL );
}

/**
 * ���ԡ����󥹥ȥ饯����
 * <ul>
 * <li>�ե�����ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 * @param other ���ԡ����Υ��֥�������
 */
AttachFileList::AttachFileList( const AttachFileList& other )
{
	IpMsgMutexInit( "AttachFileList::AttachFileList(AttachFileList&)", &filesMutex, NULL );
	other.Lock( "AttachFileList::AttachFileList(AttachFileList&)" );
	CopyFrom( other );
	other.Unlock( "AttachFileList::AttachFileList(AttachFileList&)" );
}

/**
 * �����黻�ҡ�
 * <ul>
 * <li>�ե�����ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 * @param other ���ԡ����Υ��֥�������
 * @retval �����֥������ȤΥ��󥹥���
 */
AttachFileList&
AttachFileList::operator=( const AttachFileList& other )
{
	IpMsgMutexInit( "AttachFileList::operator=(AttachFileList&)", &filesMutex, NULL );
	other.Lock( "AttachFileList::operator=(AttachFileList&)" );
	CopyFrom( other );
	other.Unlock( "AttachFileList::operator=(AttachFileList&)" );
	return *this;
}

/**
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
 */
void
AttachFileList::CopyFrom( const AttachFileList& other )
{
	files = other.files;
}

/**
 * �ǥ��ȥ饯����
 * <ul>
 * <li>�ե�����ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå������˴���</li>
 * </ul>
 */
AttachFileList::~AttachFileList()
{
	IpMsgMutexDestroy( "AttachFileList::~AttachFileList()", &filesMutex );
}

/**
 * �ե�����ꥹ�Ȥ��å�
 * @param pos ��å����Ƥ�����֤򼨤�ʸ����
 */
void
AttachFileList::Lock( const char *pos ) const
{
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &filesMutex ) );
}

/**
 * �ե�����ꥹ�Ȥ򥢥��å�
 * @param pos �����å����Ƥ�����֤򼨤�ʸ����
 */
void
AttachFileList::Unlock( const char *pos ) const
{
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &filesMutex ) );
}

/**
 * �ե������������ɲä��ޤ���
 * @param file �ɲä���ź�եե����륪�֥�������
 */
void
AttachFileList::AddFile( const AttachFile& file )
{
	Lock( "AttachFileList::AddFile()" );
	files.push_back( file );
	Unlock( "AttachFileList::AddFile()" );
}

/**
 * �ե������������Ƭ��ؤ����ƥ졼�����֤��ޤ���
 */
std::vector<AttachFile>::iterator
AttachFileList::begin()
{
	return files.begin();
}

/**
 * �ե���������ν�ü�ΰ�ĸ�����ؤ����ƥ졼�����֤��ޤ���
 */
std::vector<AttachFile>::iterator
AttachFileList::end()
{
	return files.end();
}

/**
 * �ե�����������¸�ߤ���ź�եե�����θĿ����֤��ޤ���
 * @retval ź�եե����륪�֥������ȤθĿ���
 */
int
AttachFileList::size() const
{
	Lock( "AttachFileList::size()" );
	int ret = files.size();
	Unlock( "AttachFileList::size()" );
	return ret;
}

/**
 * �ե����������򥯥ꥢ���ޤ���
 */
void
AttachFileList::clear()
{
	Lock( "AttachFileList::clear()" );
	files.clear();
	Unlock( "AttachFileList::clear()" );
}

/**
 * �ե�������������ź�եե�����������ޤ���
 * @param item �������ź�եե����륪�֥�������
 */
std::vector<AttachFile>::iterator
AttachFileList::erase( std::vector<AttachFile>::iterator item )
{
	Lock( "AttachFileList::erase(std::vector<AttachFile>::iterator)" );
	std::vector<AttachFile>::iterator ret = files.erase( item );
	Unlock( "AttachFileList::erase(std::vector<AttachFile>::iterator)" );
	return ret;
}

/**
 * �ե�������������ź�եե�����������ޤ���
 * @param item �������ź�եե����륪�֥�������
 */
std::vector<AttachFile>::iterator
AttachFileList::erase( const AttachFile& item )
{
	std::vector<AttachFile>::iterator it = FindByFileId( item.FileId() );
	Lock( "AttachFileList::erase(AttachFile&)" );
	std::vector<AttachFile>::iterator ret = files.erase( it );
	Unlock( "AttachFileList::erase(AttachFile&)" );
	return ret;
}

/**
 * ź�եե������������ե�ѥ��Ǹ���
 * @param fullPath �����оݤΥե�ѥ�
 * @retval ���դ��ä�AttachFile�Υ��ƥ졼��
 * @retval ���դ���ʤ����end()
 */
std::vector<AttachFile>::iterator
AttachFileList::FindByFullPath( const std::string& fullPath )
{
	Lock( "AttachFileList::FindByFullPath()" );
	std::vector<AttachFile>::iterator ret = end();
	for( std::vector<AttachFile>::iterator i = begin(); i != end(); i++ ) {
		if ( i->FullPath() == fullPath ) {
			ret = i;
			break;
		}
	}
	Unlock( "AttachFileList::FindByFullPath()" );
	return ret;
}

/**
 * ź�եե������������ե�����ID���˸��������פ��륪�֥������ȤΥ��ƥ졼�����֤���
 * @param file_id ź�եե�����Υե�����ID
 * @retval ź�եե����륪�֥������ȤؤΥ��ƥ졼��
 * @retval ¸�ߤ��ʤ���硢end()
 */
std::vector<AttachFile>::iterator
AttachFileList::FindByFileId( int file_id )
{
	Lock( "AttachFileList::FindByFileId()" );
	std::vector<AttachFile>::iterator ret = end();
	for( std::vector<AttachFile>::iterator ixfile = begin(); ixfile != end(); ixfile++ ) {
#ifdef DEBUG
		printf( "file_id  %d\n", file_id );fflush(stdout);
		printf( "ixfile->FileId %d\n", ixfile->FileId() );fflush(stdout);
		printf( "ixfile->FileName %s\n", ixfile->FileName().c_str() );fflush(stdout);
#endif
		if ( file_id == ixfile->FileId() ) {
			ret = ixfile;
			break;
		}
	}
	Unlock( "AttachFileList::FindByFileId()" );
	return ret;
}

/**
 * ź�եե����륳�󥹥ȥ饯��
 * <ul>
 * <li>file_id+1�ǥե�����id��������</li>
 * </ul>
 */
AttachFile::AttachFile()
{
#ifdef DEBUG
	printf("file_id before     == %d\n", file_id );fflush(stdout);
#endif
	_FileId = file_id++;
#ifdef DEBUG
	printf("AttachFile::FileId == %d\n", FileId() );fflush(stdout);
	printf("file_id after      == %d\n", file_id );fflush(stdout);
#endif
}

/**
 * ź�եե�����������
 */
void
AttachFile::GetLocalFileInfo()
{
	struct stat st;
	unsigned int loc = FullPath().find_last_of( '/' );
	std::string filename, location;
	if ( loc == std::string::npos ) {
		filename = FullPath();
	} else {
		location = FullPath().substr( 0, loc );
		filename = FullPath().substr( loc + 1 );
	}
	setFileName( filename );
	setLocation( location );
	lstat( FullPath().c_str(), &st );
	setAttr( 0 );
	if ( S_ISDIR( st.st_mode ) ) {
		setAttr( IPMSG_FILE_DIR );
		st.st_size = 0;
	} else {
		setAttr( IPMSG_FILE_REGULAR );
	}
	setMTime( st.st_mtime );
	setIsDownloaded( false );
	setIsDownloading( false );
	setFileSize( st.st_size );
}

/**
 * ź�եե����뤬���̥ե����뤫�ɤ�����Ƚ�ꡣ
 * @retval true:���̥ե�����
 * @retval false:���̥ե�����Ǥʤ�
 */
bool
AttachFile::IsRegularFile() const
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_REGULAR;
}

/**
 * ź�եե����뤬�ǥ��쥯�ȥ꤫�ɤ�����Ƚ�ꡣ
 * @retval true:�ǥ��쥯�ȥ�
 * @retval false:�ǥ��쥯�ȥ�Ǥʤ�
 */
bool
AttachFile::IsDirectory() const
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_DIR;
}

/**
 * �ǥ��쥯�ȥꥹ���å�����ե�ѥ����������롣
 * @param dirstack �ǥ��쥯�ȥꥹ���å�
 * @retval �ե�ѥ�
 */
std::string
AttachFile::CreateDirFullPath( const std::vector<std::string>& dirstack )
{
	std::string retdir = "";
	for( int i = 0; i < (int)dirstack.size(); i++ ){
		if ( dirstack[i] != "" ) {
			retdir += dirstack[i] + ( dirstack[i].at(dirstack[i].size() - 1) == '/' ? "" : "/" );
#ifdef DEBUG
			printf("retdir = %s\n", retdir.c_str());fflush(stdout);
#endif
		}
	}
	return retdir;
}

/**
 * �ǥ��쥯�ȥ��׵���ʸ�α�������ǥ��쥯�ȥ깽¤�إå�����Ϥ���ź�եե����륪�֥������Ȥ��������롣
 * @param buf �Хåե�
 * @param conv �ե�����̾����С���
 * @retval ź�եե����륪�֥�������
 */
AttachFile
AttachFile::AnalyzeHeader( char *buf, FileNameConverter *conv )
{
	char tmpbuf[100];
	int len = strlen( buf );
	int j = 0;
	int pos = 0;
	AttachFile f;
	for( int i=0; i < len; i++ ) {
		if ( buf[i] == ':' ) {
			if ( buf[i+1] != ':' ) {
				tmpbuf[j] = 0;
				pos = ++i;
				f.setFileName( conv->ConvertNetworkToLocal( tmpbuf ) );
				break;
			} else {
				i++;
			}
		}
		tmpbuf[j] = buf[i];
		j++;
	}
	char *dmyptr;
	std::string size = "";
	j = 0;
	for( int i=pos; i < len; i++ ){
		if ( buf[i] == ':' ) {
			tmpbuf[j] = 0;
			size = tmpbuf;
			pos = ++i;
			break;
		}
		tmpbuf[j] = buf[i];
		j++;
	}
	f.setFileSize( strtoull( size.c_str(), &dmyptr, 16 ) );

	std::string fattr = "";
	j = 0;
	for( int i=pos; i < len; i++ ){
		if ( buf[i] == ':' ) {
			tmpbuf[j] = 0;
			fattr = tmpbuf;
			pos = ++i;
			break;
		}
		tmpbuf[j] = buf[i];
		j++;
	}
	f.setAttr( strtoull( fattr.c_str(), &dmyptr, 16 ) );

	while( buf[pos] != '\0' ) {
		j = 0;
		std::string fextattr = "";
		for( int i=pos; i < len; i++ ){
			if ( buf[i] == ':' ) {
				tmpbuf[j] = 0;
				fextattr = tmpbuf;
				int eqpos = -1;
				for( int k = 0; tmpbuf[k] != '\0'; k++ ){ 
					if ( tmpbuf[k] == '=' ) {
						tmpbuf[k] = '\0';
						eqpos = k + 1;
						break;
					}
				}
				if ( eqpos >= 0 ) {
					dmyptr = tmpbuf;
					char *topchar = dmyptr;
					while( *dmyptr != '\0' ) {
						f.addExtAttrs( tmpbuf, strtoul( topchar, &dmyptr, 16 ) );
						topchar = ++dmyptr;
					}
					pos = ++i;
					break;
				}
			}
			tmpbuf[j] = buf[i];
			j++;
		}
	}

	return f;
}
