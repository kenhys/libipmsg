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
	IPMSG_FUNC_ENTER( "AttachFileList::AttachFileList()" );
	IpMsgMutexInit( "AttachFileList::AttachFileList()", &filesMutex, NULL );
	IPMSG_FUNC_EXIT;
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
	IPMSG_FUNC_ENTER( "AttachFileList::AttachFileList( const AttachFileList& other )" );
	IpMsgMutexInit( "AttachFileList::AttachFileList(AttachFileList&)", &filesMutex, NULL );
	other.Lock( "AttachFileList::AttachFileList(AttachFileList&)" );
	CopyFrom( other );
	other.Unlock( "AttachFileList::AttachFileList(AttachFileList&)" );
	IPMSG_FUNC_EXIT;
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
	IPMSG_FUNC_ENTER( "AttachFileList& AttachFileList::operator=( const AttachFileList& other )" );
	IpMsgMutexInit( "AttachFileList::operator=(AttachFileList&)", &filesMutex, NULL );
	other.Lock( "AttachFileList::operator=(AttachFileList&)" );
	CopyFrom( other );
	other.Unlock( "AttachFileList::operator=(AttachFileList&)" );
	IPMSG_FUNC_RETURN( *this );
}

/**
 * コピーメソッド。
 * @param other コピー元のオブジェクト
 */
void
AttachFileList::CopyFrom( const AttachFileList& other )
{
	IPMSG_FUNC_ENTER( "void AttachFileList::CopyFrom( const AttachFileList& other )" );
	files = other.files;
	IPMSG_FUNC_EXIT;
}

/**
 * デストラクタ。
 * <ul>
 * <li>ファイルリストをロックするためのミューテックスを破棄。</li>
 * </ul>
 */
AttachFileList::~AttachFileList()
{
	IPMSG_FUNC_ENTER( "AttachFileList::~AttachFileList()" );
	IpMsgMutexDestroy( "AttachFileList::~AttachFileList()", &filesMutex );
	IPMSG_FUNC_EXIT;
}

/**
 * ファイルリストをロック
 * @param pos ロックしている位置を示す文字列。
 */
void
AttachFileList::Lock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void AttachFileList::Lock( const char *pos ) const" );
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &filesMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * ファイルリストをアンロック
 * @param pos アンロックしている位置を示す文字列。
 */
void
AttachFileList::Unlock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void AttachFileList::Unlock( const char *pos ) const" );
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &filesMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * ファイルを一覧に追加します。
 * @param file 追加する添付ファイルオブジェクト
 */
void
AttachFileList::AddFile( const AttachFile& file )
{
	IPMSG_FUNC_ENTER( "void AttachFileList::AddFile( const AttachFile& file )" );
	Lock( "AttachFileList::AddFile()" );
	files.push_back( file );
	Unlock( "AttachFileList::AddFile()" );
	IPMSG_FUNC_EXIT;
}

/**
 * ファイル一覧の先頭を指すイテレータを返します。
 */
std::vector<AttachFile>::iterator
AttachFileList::begin()
{
	IPMSG_FUNC_ENTER( "std::vector<AttachFile>::iterator AttachFileList::begin()" );
	IPMSG_FUNC_RETURN( files.begin() );
}

/**
 * ファイル一覧の終端の一つ後方を指すイテレータを返します。
 */
std::vector<AttachFile>::iterator
AttachFileList::end()
{
	IPMSG_FUNC_ENTER( "std::vector<AttachFile>::iterator AttachFileList::end()" );
	IPMSG_FUNC_RETURN( files.end() );
}

/**
 * ファイルを一覧に存在する添付ファイルの個数を返します。
 * @retval 添付ファイルオブジェクトの個数。
 */
int
AttachFileList::size() const
{
	IPMSG_FUNC_ENTER( "int AttachFileList::size() const" );
	Lock( "AttachFileList::size()" );
	int ret = files.size();
	Unlock( "AttachFileList::size()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * ファイルを一覧をクリアします。
 */
void
AttachFileList::clear()
{
	IPMSG_FUNC_ENTER( "void AttachFileList::clear()" );
	Lock( "AttachFileList::clear()" );
	files.clear();
	Unlock( "AttachFileList::clear()" );
	IPMSG_FUNC_EXIT;
}

/**
 * ファイルを一覧から添付ファイルを削除します。
 * @param item 削除する添付ファイルオブジェクト
 */
std::vector<AttachFile>::iterator
AttachFileList::erase( std::vector<AttachFile>::iterator item )
{
	IPMSG_FUNC_ENTER( "std::vector<AttachFile>::iterator AttachFileList::erase( std::vector<AttachFile>::iterator item )" );
	Lock( "AttachFileList::erase(std::vector<AttachFile>::iterator)" );
	std::vector<AttachFile>::iterator ret = files.erase( item );
	Unlock( "AttachFileList::erase(std::vector<AttachFile>::iterator)" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * ファイルを一覧から添付ファイルを削除します。
 * @param item 削除する添付ファイルオブジェクト
 */
std::vector<AttachFile>::iterator
AttachFileList::erase( const AttachFile& item )
{
	IPMSG_FUNC_ENTER( "std::vector<AttachFile>::iterator AttachFileList::erase( const AttachFile& item )" );
	std::vector<AttachFile>::iterator it = FindByFileId( item.FileId() );
	Lock( "AttachFileList::erase(AttachFile&)" );
	std::vector<AttachFile>::iterator ret = files.erase( it );
	Unlock( "AttachFileList::erase(AttachFile&)" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * 添付ファイル一覧からフルパスで検索
 * @param fullPath 検索対象のフルパス
 * @retval 見付かったAttachFileのイテレータ
 * @retval 見付からない場合end()
 */
std::vector<AttachFile>::iterator
AttachFileList::FindByFullPath( const std::string& fullPath )
{
	IPMSG_FUNC_ENTER( "std::vector<AttachFile>::iterator AttachFileList::FindByFullPath( const std::string& fullPath )" );
	Lock( "AttachFileList::FindByFullPath()" );
	std::vector<AttachFile>::iterator ret = end();
	for( std::vector<AttachFile>::iterator i = begin(); i != end(); i++ ) {
		if ( i->FullPath() == fullPath ) {
			ret = i;
			break;
		}
	}
	Unlock( "AttachFileList::FindByFullPath()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * 添付ファイル一覧からファイルIDを基に検索し一致するオブジェクトのイテレータを返す。
 * @param file_id 添付ファイルのファイルID
 * @retval 添付ファイルオブジェクトへのイテレータ
 * @retval 存在しない場合、end()
 */
std::vector<AttachFile>::iterator
AttachFileList::FindByFileId( int file_id )
{
	IPMSG_FUNC_ENTER( "std::vector<AttachFile>::iterator AttachFileList::FindByFileId( int file_id )" );
	Lock( "AttachFileList::FindByFileId()" );
	std::vector<AttachFile>::iterator ret = end();
#ifdef DEBUG
	IpMsgPrintLogTime(stdout);
	printf( "AttachFileList::FindByFileId file_id=%d\n", file_id );
	fflush(stdout);
#endif
	for( std::vector<AttachFile>::iterator ixfile = begin(); ixfile != end(); ixfile++ ) {
#ifdef DEBUG
		IpMsgPrintLogTime(stdout);
		printf( "AttachFileList::FindByFileId Searching attach file list.\n" );
		IpMsgPrintLogTime(stdout);
		printf( "AttachFileList::FindByFileId FileId %d\n", ixfile->FileId() );
		IpMsgPrintLogTime(stdout);
		printf( "AttachFileList::FindByFileId FileName %s\n", ixfile->FileName().c_str() );
		fflush(stdout);
#endif
		if ( file_id == ixfile->FileId() ) {
#ifdef DEBUG
			IpMsgPrintLogTime(stdout);
			printf( "AttachFileList::FindByFileId File id was found\n" );
			fflush(stdout);
#endif
			ret = ixfile;
			break;
		}
	}
	Unlock( "AttachFileList::FindByFileId()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * 添付ファイルコンストラクタ
 * <ul>
 * <li>file_id+1でファイルidを初期化。</li>
 * </ul>
 */
AttachFile::AttachFile()
{
	IPMSG_FUNC_ENTER( "AttachFile::AttachFile()" );
	_FileId = file_id++;
	IPMSG_FUNC_EXIT;
}

/**
 * 添付ファイル情報取得
 */
void
AttachFile::GetLocalFileInfo()
{
	IPMSG_FUNC_ENTER( "void AttachFile::GetLocalFileInfo()" );
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
	IPMSG_FUNC_EXIT;
}

/**
 * 添付ファイルが一般ファイルかどうかを判定。
 * @retval true:一般ファイル
 * @retval false:一般ファイルでない
 */
bool
AttachFile::IsRegularFile() const
{
	IPMSG_FUNC_ENTER( "bool AttachFile::IsRegularFile() const" );
	IPMSG_FUNC_RETURN( GET_FILETYPE( Attr() ) == IPMSG_FILE_REGULAR );
}

/**
 * 添付ファイルがディレクトリかどうかを判定。
 * @retval true:ディレクトリ
 * @retval false:ディレクトリでない
 */
bool
AttachFile::IsDirectory() const
{
	IPMSG_FUNC_ENTER( "bool AttachFile::IsDirectory() const" );
	IPMSG_FUNC_RETURN( GET_FILETYPE( Attr() ) == IPMSG_FILE_DIR );
}

/**
 * ディレクトリスタックからフルパスを生成する。
 * @param dirstack ディレクトリスタック
 * @retval フルパス
 */
std::string
AttachFile::CreateDirFullPath( const std::vector<std::string>& dirstack )
{
	IPMSG_FUNC_ENTER( "std::string AttachFile::CreateDirFullPath( const std::vector<std::string>& dirstack )" );
	std::string retdir = "";
	for( int i = 0; i < (int)dirstack.size(); i++ ){
		if ( dirstack[i] != "" ) {
			retdir += dirstack[i] + ( dirstack[i].at(dirstack[i].size() - 1) == '/' ? "" : "/" );
#ifdef DEBUG
			IpMsgPrintLogTime(stdout);
			printf("AttachFile::CreateDirFullPath retdir = %s\n", retdir.c_str());fflush(stdout);
#endif
		}
	}
	IPMSG_FUNC_RETURN( retdir );
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
	IPMSG_FUNC_ENTER( "AttachFile AttachFile::AnalyzeHeader( char *buf, FileNameConverter *conv )" );
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

	IPMSG_FUNC_RETURN( f );
}
