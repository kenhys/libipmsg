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

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * ホスト情報をホストリストに追加する。
 * @param host ホスト情報
 */
void HostList::AddHost( HostListItem host )
{
	bool is_found = false;
	for( unsigned int i = 0; i < items.size(); i++ ){
		if ( host.Equals( items.at( i ) ) ) {
			is_found = true;
			break;
		}
	}
	if ( !is_found ) {
		items.push_back( host );
	}
}

/**
 * ホスト情報をホストリストから削除する。
 * @param ホスト名
 */
void HostList::DeleteHost( string hostname )
{
	for( vector<HostListItem>::iterator ix = items.begin(); ix < items.end(); ix++ ){
		if ( ix->HostName() == hostname ) {
			items.erase( ix );
			break;
		}
	}
}

/**
 * ホストリスト送信用文字列を作成する。
 * @param start 開始位置
 */
string HostList::ToString( int start )
{
	char buf[MAX_UDPBUF];
	string ret;
	unsigned int max;

	max = start + HOST_LIST_SEND_MAX_AT_ONCE - 1;
	if ( max > items.size() ) {
		max = items.size();
	}
	snprintf( buf, sizeof( buf ), "%-5d\a%-5d\a", start , max - start < 0 ? 0 : max - start );
	ret = buf;
	for( unsigned int i = start ; i < max; i++ ){
		HostListItem item = items.at( i );
		sprintf( buf, "%s\a%s\a%ld\a%s\a%d\a%s\a%s\a",
						item.UserName() == "" ? "\b" : item.UserName().c_str(),
						item.HostName() == "" ? "\b" : item.HostName().c_str(),
						item.CommandNo(),
						item.IpAddress() == "" ? "\b" : item.IpAddress().c_str(),
						htons( item.PortNo() ),
						item.Nickname() == "" ? "\b" : item.Nickname().c_str(),
						item.GroupName() == "" ? "\b" : item.GroupName().c_str() );
		ret = ret + buf;
	}
	return ret;
}

/**
 * パケットオブジェクトからホストリストアイテムを生成する。
 * @param packet パケットオブジェクト
 * @retval ホストリストアイテム
 */
HostListItem HostList::CreateHostListItemFromPacket( Packet packet )
{
	HostListItem ret;
	ret.setHostName( packet.HostName() );
	ret.setUserName( packet.UserName() );
	ret.setCommandNo( packet.CommandMode() | packet.CommandOption() );
	ret.setIpAddress( inet_ntoa( packet.Addr().sin_addr ) );
#if defined(INFO) || !defined(NDEBUG)
	printf( "CreateHostListItemFromPacket port %d\n", packet.Addr().sin_port );
#endif
	ret.setPortNo( packet.Addr().sin_port );
	unsigned int loc = packet.Option().find_first_of( '\0' );
	if ( loc == string::npos ) {
		ret.setNickname( packet.Option() );
		ret.setGroupName( "" );
	} else {
		ret.setNickname( packet.Option().substr( 0, loc ) );
		ret.setGroupName( packet.Option().substr( loc + 1 ) );
	}
	return ret;
}

/**
 * ホストリストアイテムオブジェクトが自分と一致するかを返す。
 * @param item ホストリストアイテム
 * @retval 一致：true／一致しない：false
 */
bool HostListItem::Equals( HostListItem item )
{
	return	item.UserName() == UserName() &&
			item.HostName() == HostName() &&
			item.IpAddress() == IpAddress();
//			item.Nickname() == Nickname() &&
//			item.GroupName() == GroupName() &&
//			item.PortNo() == PortNo();
}

//end of source
