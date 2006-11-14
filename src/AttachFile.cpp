/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * Windows��IP��å��󥸥㡼���̿�����饤�֥�ꡣ
 * ���Υ饤�֥��ϥ���åɥ����դǤʤ�����դ�ɬ�ס�
 * �ץ�ȥ�������������Τ�¾�θ���Ȥμ������̿��Ǥ���褦��
 * Shift_JIS����ˤϤʤäƤ��ʤ���
 * â�����ץ�ȥ������ʸ��ζ��ڤ�ʸ���Ȥ���'\0'��Ȥ����Ȥ�ͭ�뤿�ᡢ
 * UTF16��UCS���ͤʡ�ʸ�������ɤ�'\0'��ޤ�ʸ�������ɤˤ��б����Ƥ��ʤ���
 *
 * ���Υ饤�֥��ΥС������Ǥϡ��Ź�Ϥ�OpenSSL��crypto�饤�֥�����Ѥ��뤿�ᡢ
 * ��������RSA512,1024,2048 ���̸���RC2-40,RC2-128,RC2-256,BLOWFISH-128,BLOWFISH-256
 * �򥵥ݡ��ȤǤ��롣���̿���Υ��饤����ȤκǶ����٤ΰŹ��ͥ��������Ȥ��Ƽ�ư���򤹤롣��
 * â����Windows�Ǥϡʥ�������Υ����Ȥˤ��Сˡּ�ȴ���פμ����ʤΤǺǶ����٤�����
 * ���Ƥ��ޤ��ȡ�Windows�Ǥϼ��ʤ�ǽ�Ϥ�����������Ƥ���ΤǰŹ沽���줿�ѥ��åȤ�����
 * ����ʤ����Ȥ���ա�
 *
 * Windows�ǤǤ�RSA512+RC2-40, RSA1024+BLOWFISH128�������ݡ��Ȥ��Ƥ��ʤ���
 * �����WINCOMPAT�ޥ���ˤ�ꥵ�ݡ��Ȥ��롣��WINCOMPAT���������ȡ�Windows�Ǥμ���ǽ�Ϥε���������
 * ��碌�ưŹ沽��ˡ�����򤹤롣��
 *
 * ��������ɼ���ե�����̾���Ѵ��ϥե�����̾����С������֥������ȤǹԤ���
 * ������ե����륷���ƥ�Υ��󥳡��ǥ��󥰤ȥ�å������Υ��󥳡��ǥ��󥰤�����Ѵ����륪�֥������Ȥǡ�
 * �饤�֥��Ȥ��Ƥϲ��۴��쥯�饹��FileNameConverter���Ѵ���Ԥ�ʤ�NullFileNameConverter(�ǥե����)
 * ���󶡤��롣���ץꥱ��������ɬ�פ˱����ơ�FileNameConverter��Ѿ����ơ��ե�����̾�Ѵ����å�
 * ���Ȥ߹���ɬ�פ����롣��Windows�Ǥ�Shift_JIS(CP932)�����̤�Unix�Ǥ�eucJP or UTF-8�����顣��
 *
 * ���ޥ�ɥ��ץ����μ����䡢��ȥ饤���μ�����Ŭ���Ǥ���Τǻ��Ѥˤ���դ�ɬ�ס�
 * ���ϥ��ץ�¦�μ����Ȥ��������ϴ��Τ��ʤ���
 *
 * �ƥꥹ�ȡʥۥ��ȥꥹ�ȡ�������å������ꥹ�ȡ�������å������ꥹ�ȡ�ź�եե�����ꥹ�ȡˤ�
 * ����API��������������Ȼפ������������������롣
 *
 * automake/autoconf���⥪�������������롣
 *
 * ���Ĥ�
 * ������Ū�˥���åɥ����դǤʤ���
 * �����饹��ʬ�䤬���ޤ�����
 * �������å���Υ��󥹥��󥹤�¿�����롣��äȥҡ��׾�ˤȤ�٤���
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
		printf( "file_id  %d\n", file_id );
		printf( "ixfile->FileId %d\n", ixfile->FileId() );
		printf( "ixfile->FileName %s\n", ixfile->FileName().c_str() );
		fflush( stdout );
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
	printf("file_id before     == %d\n", file_id );
	_FileId = file_id++;
	printf("AttachFile::FileId == %d\n", FileId() );
	printf("file_id after      == %d\n", file_id );
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
	stat( FullPath().c_str(), &st );
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
			printf("retdir = %s\n", retdir.c_str());
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
