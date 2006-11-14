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

/**
 * パケットから添付ファイルを検索します。
 * ・パケットからファイルIDを抽出しファイルIDを基に添付ファイルを検索し、AttachFileのイテレータを返します。
 * @param packet パケットオブジェクト
 * @retval AttachFileのイテレータ。見付からない場合、end()を返す。
 */
vector<AttachFile>::iterator
SentMessage::FindAttachFileByPacket( Packet packet )
{
	char *dmyptr;
	char *startptr;
	strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;
	int packet_file_id = strtoul( startptr, &dmyptr, 16 );
	startptr = ++dmyptr;

	vector<AttachFile>::iterator FoundFile;
	FoundFile = Files().FindByFileId( packet_file_id );
	return FoundFile;
}
