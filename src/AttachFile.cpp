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

static int file_id = 0;

/**
 * 添付ファイル一覧からフルパスで検索
 * @param fullParh 検索対象のフルパス
 * @retval 見付かったAttachFileのイテレータ。見付からない場合end();
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
 * 添付ファイルオブジェクトが自分と一致するかを返す。
 * @param item 添付ファイル
 * @retval 添付ファイルオブジェクトへのイテレータ
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
 * 添付ファイルコンストラクタ
 * ・file_id+1でファイルidを初期化。
 */
AttachFile::AttachFile()
{
	printf("file_id before     == %d\n", file_id );
	_FileId = file_id++;
	printf("AttachFile::FileId == %d\n", FileId() );
	printf("file_id after      == %d\n", file_id );
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
 * 添付ファイルが一般ファイルかどうかを判定。
 * @retval 一般ファイル:true／一般ファイルでない:false
 */
bool
AttachFile::IsRegularFile()
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_REGULAR;
}

/**
 * 添付ファイルがディレクトリかどうかを判定。
 * @retval ディレクトリ:true／ディレクトリでない:false
 */
bool
AttachFile::IsDirectory()
{
	return GET_FILETYPE( Attr() ) == IPMSG_FILE_DIR;
}

/**
 * ディレクトリスタックからフルパスを生成する。
 * @param dirstack ディレクトリスタック
 * @retval フルパス
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
