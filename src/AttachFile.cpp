/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * ź�եե����륯�饹��
 */
 
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
using namespace std;

static int file_id = 0;

/**
 * ź�եե������������ե�ѥ��Ǹ���
 * @param fullParh �����оݤΥե�ѥ�
 * @retval ���դ��ä�AttachFile�Υ��ƥ졼�������դ���ʤ����end();
 */
vector<AttachFile>::iterator
AttachFileList::FindByFullPath( string fullPath )
{
	for( vector<AttachFile>::iterator i = begin(); i != end(); i++ ) {
		if ( i->FullPath() == fullPath ) {
			return i;
		}
	}
	return end();
}

/**
 * ź�եե����륪�֥������Ȥ���ʬ�Ȱ��פ��뤫���֤���
 * @param item ź�եե�����
 * @retval ź�եե����륪�֥������ȤؤΥ��ƥ졼��
 */
vector<AttachFile>::iterator
AttachFileList::FindByFileId( int file_id )
{
	for( vector<AttachFile>::iterator ixfile = begin(); ixfile != end(); ixfile++ ) {
#ifdef DEBUG
		printf( "file_id  %d\n", file_id );fflush(stdout);
		printf( "ixfile->FileId %d\n", ixfile->FileId() );fflush(stdout);
		printf( "ixfile->FileName %s\n", ixfile->FileName().c_str() );fflush(stdout);
#endif
		if ( file_id == ixfile->FileId() ) {
			return ixfile;
		}
	}
	return end();
}

/**
 * ź�եե����륳�󥹥ȥ饯��
 * ��file_id+1�ǥե�����id��������
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
	string filename, location;
	if ( loc == string::npos ) {
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
 * @retval ���̥ե�����:true�����̥ե�����Ǥʤ�:false
 */
bool
AttachFile::IsRegularFile()
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_REGULAR;
}

/**
 * ź�եե����뤬�ǥ��쥯�ȥ꤫�ɤ�����Ƚ�ꡣ
 * @retval �ǥ��쥯�ȥ�:true���ǥ��쥯�ȥ�Ǥʤ�:false
 */
bool
AttachFile::IsDirectory()
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_DIR;
}

/**
 * �ǥ��쥯�ȥꥹ���å�����ե�ѥ����������롣
 * @param dirstack �ǥ��쥯�ȥꥹ���å�
 * @retval �ե�ѥ�
 */
string
AttachFile::CreateDirFullPath( vector<string> dirstack )
{
	string retdir = "";
	for( vector<string>::iterator d = dirstack.begin(); d != dirstack.end(); d++ ){
		if ( *d != "" ) {
			retdir += *d + ( d->at(d->size() - 1) == '/' ? "" : "/" );
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
	string size = "";
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

	string fattr = "";
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
		string fextattr = "";
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
