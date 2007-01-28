/**
 * IP メッセンジャライブラリ(Unix用)
 * 添付ファイルクラス。
 */
 
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"

#if defined(__linux__)
    #include <linux/version.h>
    #if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
        #define SUPPORT_SENDFILE_LINUX_STYLE
        #include <sys/sendfile.h>
    #endif // LINUX_VERSION_CODE
#endif // __linux__
#if defined(__FreeBSD__) || defined(__DragonFly__)
    #define SUPPORT_SENDFILE_BSD_STYLE
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/uio.h>
#endif // __FreeBSD__,__DragonFly__
#if defined(__solaris__)
    #define SUPPORT_SENDFILE_SOLARIS_STYLE
#endif // __solaris__
#if defined(__HPUX__)
    #define SUPPORT_SENDFILE_HPUX_STYLE
#endif // __HPUX__

using namespace std;
using namespace ipmsg;

static int file_id = 0;

/**
 * コンストラクタ。
 * <ul>
 * <li>ファイルリストをロックするためのミューテックスを生成。</li>
 * </ul>
 */
AttachFileList::AttachFileList()
{
	IpMsgMutexInit( "AttachFileList::AttachFileList()", &filesMutex, NULL );
}

/**
 * コピーコンストラクタ。
 * <ul>
 * <li>ファイルリストをロックするためのミューテックスを生成。</li>
 * </ul>
 * @param other コピー元のオブジェクト
 */
AttachFileList::AttachFileList( const AttachFileList& other )
{
	IpMsgMutexInit( "AttachFileList::AttachFileList(AttachFileList&)", &filesMutex, NULL );
	other.Lock( "AttachFileList::AttachFileList(AttachFileList&)" );
	CopyFrom( other );
	other.Unlock( "AttachFileList::AttachFileList(AttachFileList&)" );
}

/**
 * 代入演算子。
 * <ul>
 * <li>ファイルリストをロックするためのミューテックスを生成。</li>
 * </ul>
 * @param other コピー元のオブジェクト
 * @retval 自オブジェクトのインスタンス
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
 * コピーメソッド。
 * @param other コピー元のオブジェクト
 */
void
AttachFileList::CopyFrom( const AttachFileList& other )
{
	files = other.files;
}

/**
 * デストラクタ。
 * <ul>
 * <li>ファイルリストをロックするためのミューテックスを破棄。</li>
 * </ul>
 */
AttachFileList::~AttachFileList()
{
	IpMsgMutexDestroy( "AttachFileList::~AttachFileList()", &filesMutex );
}

/**
 * ファイルリストをロック
 * @param pos ロックしている位置を示す文字列。
 */
void
AttachFileList::Lock( const char *pos ) const
{
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &filesMutex ) );
}

/**
 * ファイルリストをアンロック
 * @param pos アンロックしている位置を示す文字列。
 */
void
AttachFileList::Unlock( const char *pos ) const
{
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &filesMutex ) );
}

/**
 * ファイルを一覧に追加します。
 * @param file 追加する添付ファイルオブジェクト
 */
void
AttachFileList::AddFile( const AttachFile& file )
{
	Lock( "AttachFileList::AddFile()" );
	files.push_back( file );
	Unlock( "AttachFileList::AddFile()" );
}

/**
 * ファイル一覧の先頭を指すイテレータを返します。
 */
vector<AttachFile>::iterator
AttachFileList::begin()
{
	return files.begin();
}

/**
 * ファイル一覧の終端の一つ後方を指すイテレータを返します。
 */
vector<AttachFile>::iterator
AttachFileList::end()
{
	return files.end();
}

/**
 * ファイルを一覧に存在する添付ファイルの個数を返します。
 * @retval 添付ファイルオブジェクトの個数。
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
 * ファイルを一覧をクリアします。
 */
void
AttachFileList::clear()
{
	Lock( "AttachFileList::clear()" );
	files.clear();
	Unlock( "AttachFileList::clear()" );
}

/**
 * ファイルを一覧から添付ファイルを削除します。
 * @param item 削除する添付ファイルオブジェクト
 */
vector<AttachFile>::iterator
AttachFileList::erase( vector<AttachFile>::iterator item )
{
	Lock( "AttachFileList::erase(vector<AttachFile>::iterator)" );
	vector<AttachFile>::iterator ret = files.erase( item );
	Unlock( "AttachFileList::erase(vector<AttachFile>::iterator)" );
	return ret;
}

/**
 * ファイルを一覧から添付ファイルを削除します。
 * @param item 削除する添付ファイルオブジェクト
 */
vector<AttachFile>::iterator
AttachFileList::erase( const AttachFile& item )
{
	vector<AttachFile>::iterator it = FindByFileId( item.FileId() );
	Lock( "AttachFileList::erase(AttachFile&)" );
	vector<AttachFile>::iterator ret = files.erase( it );
	Unlock( "AttachFileList::erase(AttachFile&)" );
	return ret;
}

/**
 * 添付ファイル一覧からフルパスで検索
 * @param fullPath 検索対象のフルパス
 * @retval 見付かったAttachFileのイテレータ
 * @retval 見付からない場合end()
 */
vector<AttachFile>::iterator
AttachFileList::FindByFullPath( const string& fullPath )
{
	Lock( "AttachFileList::FindByFullPath()" );
	vector<AttachFile>::iterator ret = end();
	for( vector<AttachFile>::iterator i = begin(); i != end(); i++ ) {
		if ( i->FullPath() == fullPath ) {
			ret = i;
			break;
		}
	}
	Unlock( "AttachFileList::FindByFullPath()" );
	return ret;
}

/**
 * 添付ファイル一覧からファイルIDを基に検索し一致するオブジェクトのイテレータを返す。
 * @param file_id 添付ファイルのファイルID
 * @retval 添付ファイルオブジェクトへのイテレータ
 * @retval 存在しない場合、end()
 */
vector<AttachFile>::iterator
AttachFileList::FindByFileId( int file_id )
{
	Lock( "AttachFileList::FindByFileId()" );
	vector<AttachFile>::iterator ret = end();
	for( vector<AttachFile>::iterator ixfile = begin(); ixfile != end(); ixfile++ ) {
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
 * 添付ファイルコンストラクタ
 * <ul>
 * <li>file_id+1でファイルidを初期化。</li>
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
 * 添付ファイル情報取得
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
 * 添付ファイルが一般ファイルかどうかを判定。
 * @retval true:一般ファイル
 * @retval false:一般ファイルでない
 */
bool
AttachFile::IsRegularFile() const
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_REGULAR;
}

/**
 * 添付ファイルがディレクトリかどうかを判定。
 * @retval true:ディレクトリ
 * @retval false:ディレクトリでない
 */
bool
AttachFile::IsDirectory() const
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_DIR;
}

/**
 * ディレクトリスタックからフルパスを生成する。
 * @param dirstack ディレクトリスタック
 * @retval フルパス
 */
string
AttachFile::CreateDirFullPath( const vector<string>& dirstack )
{
	string retdir = "";
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
 * ディレクトリ要求電文の応答からディレクトリ構造ヘッダを解析し、添付ファイルオブジェクトを生成する。
 * @param buf バッファ
 * @param conv ファイル名コンバータ
 * @retval 添付ファイルオブジェクト
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
/**
 * ファイルバッファ送信メソッド。
 * <ul>
 * <li>sendfileがサポートされているプラットフォームではsendfileシステムコールを使って高速化。ケースバイケースだが実装によってはゼロコピーになる。<br>
 * でも、テストが出来ませんねぇ。とりあえず、Solarisとhp-uxはデフォルトのread/write実装を使うので遅い。Linuxでは4-5MBのファイルだと2倍違うこともある</li>
 * </ul>
 */
int
AttachFile::SendFileBuffer( int ifd, int sock, int size )
{
#if defined(SUPPORT_SENDFILE_LINUX_STYLE)
	//Linux用
	//printf("sendfile as sendfile syscall by linux\n");
	return sendfile( sock, ifd, NULL, size );
#elif defined(SUPPORT_SENDFILE_BSD_STYLE)
	//printf("sendfile as sendfile syscall by freebsd\n");
	//FreeBSD用
	return sendfile( sock, ifd, NULL, size, NULL, NULL, 0 );
/*
// TODO solaris support start from here.
#elif defined( SUPPORT_SENDFILE_SOLARIS_STYLE )
// TODO hp-ux support start from here.
#if 0
//#elif defined(SUPPORT_SENDFILE_HPUX_STYLE)
	printf("sendfile as sendfile syscall by hp-ux\n");
	//FreeBSD用
	return sendfile( sock, ifd, NULL, size, NULL, 0 );
#endif
// TODO hp-ux support end.
*/
#else
	//printf("sendfile as read write\n");
	//デフォルト実装
	char readbuf[8192];
	int readSize = read( ifd, readbuf, sizeof( readbuf ) );
	if ( readSize > 0 ){
		return send( sock, readbuf, readSize, 0 );
	}
	return -1;
#endif // SUPPORT_SENDFILE_LINUX, SUPPORT_SENDFILE_FREEBSD
}
