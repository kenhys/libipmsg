/**
 * IP メッセンジャライブラリ(Unix用)
 * IPメッセンジャエージェントクラス。
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"

//NICの最大数
#define IFR_MAX 20

static IpMessengerAgent *myInstance = NULL;

#ifdef HAVE_PTHREAD
static pthread_mutex_t instanceMutex;
static int mutex_init_result = pthread_mutex_init( &instanceMutex, NULL );
#endif

IpMessengerEvent::~IpMessengerEvent(){};
/**
 * IP メッセンジャエージェントクラスのインスタンスを取得する。
 * Singletonパターンを採用しているので、ホスト唯一のインスタンスでなければならない。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgent *
IpMessengerAgent::GetInstance()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgent();
	}
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif
	return myInstance;
}

/**
 * IP メッセンジャエージェントクラスのインスタンスを解放する。
 * このメソッドを使ってオブジェクトを解放しなければならない。
 * ライブラリを通じないで直接deleteされた場合はその後の動作について感知しない。
 * 注：このインスタンスはスレッドセーフでない。
 */
void
IpMessengerAgent::Release()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif
	if ( myInstance == NULL ) {
#ifdef HAVE_PTHREAD
		pthread_mutex_unlock( &instanceMutex );
#endif
		return;
	}
	delete myInstance;
	myInstance = NULL;
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif
}

/**
 * IP メッセンジャエージェントクラスのコンストラクタ。
 * ・暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の生成を行う。
 * ・パケットNoに使用する乱数シードを時刻で初期化する。
 * ・ファイル名コンバータを初期セットアップする。（変換を行わないNullConverterがデフォルト）
 * ・ネットワークの初期化。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgent::IpMessengerAgent()
{
	ipmsgImpl = IpMessengerAgentImpl::GetInstance();
}

/**
 * IP メッセンジャエージェントクラスのデストラクタ。
 * ・まず、ログアウト。
 * ・暗号化サポートが有効な場合、ローカルホストのRSA公開鍵の破棄を行う。
 * ・割り当て済のファイル名コンバータを削除する。
 * ・ソケットのクローズ。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgent::~IpMessengerAgent()
{
	IpMessengerAgentImpl::Release();
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * ・まず、ログアウト。
 * ・ネットワーク終期化。
 * ・ネットワーク初期化。
 * ・再度ログイン。
 * 注：このインスタンスはスレッドセーフでない。
 */
void
IpMessengerAgent::RestartNetwork()
{
	ipmsgImpl->RestartNetwork();
}

/**
 * ファイル名コンバータのゲッター。
 * 注：このメソッドはスレッドセーフでない。
 * @retval コンバータのアドレス。
 */
FileNameConverter *
IpMessengerAgent::GetFileNameConverter()
{
	return ipmsgImpl->GetFileNameConverter();
}

/**
 * ファイル名コンバータのセッター。
 * ・割り当て済のファイル名コンバータを削除する。
 * ・新しいコンバータの割り当て。
 * 注：このメソッドはスレッドセーフでない。
 * @param conv コンバータのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetFileNameConverter( FileNameConverter *conv )
{
	ipmsgImpl->SetFileNameConverter( conv );
}

/**
 * イベントオブジェクトのゲッター。
 * 注：このメソッドはスレッドセーフでない。
 * @retval イベントオブジェクトのアドレス。
 */
HostListComparator *
IpMessengerAgent::GetSortHostListComparator()
{
	return ipmsgImpl->GetSortHostListComparator();
}; 

/**
 * ホストリスト比較オブジェクトのセッター。
 * ・割り当て済のホストリスト比較オブジェクトを削除する。
 * ・新しいホストリスト比較オブジェクトの割り当て。
 * 注：このメソッドはスレッドセーフでない。
 * @param comparator ホストリスト比較オブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetSortHostListComparator( HostListComparator *comparator )
{
	ipmsgImpl->SetSortHostListComparator( comparator );
}

/**
 * イベントオブジェクトのゲッター。
 * 注：このメソッドはスレッドセーフでない。
 * @retval イベントオブジェクトのアドレス。
 */
IpMessengerEvent *
IpMessengerAgent::GetEventObject()
{
	return ipmsgImpl->GetEventObject();
}; 

/**
 * イベントオブジェクトのセッター。
 * ・割り当て済のイベントオブジェクトを削除する。
 * ・新しいイベントオブジェクトの割り当て。
 * 注：このメソッドはスレッドセーフでない。
 * @param conv イベントオブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetEventObject( IpMessengerEvent *evt )
{
	ipmsgImpl->SetEventObject( evt );
}

/**
 * NICの情報を取得する。
 * ・使用するネットワークインターフェイスのIPアドレスを求める。（ローカルループバックをのぞく全てのNIC）
 * @param nics ネットワークインターフェースの一覧
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::GetNetworkInterfaceInfo( vector<NetworkInterface>& nics )
{
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( nics );
}

/**
 * ログイン（サービス参加通知）。
 * ・NOOPERATIONパケットを送信しネットワークが使用可能かどうかを確認した上でホストリストを取得。
 * ・BR_ENTRYをブロードキャスト。
 * ・パケットを受信した上で、ホストリストを再度取得。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::Login( string nickname, string groupName )
{
	ipmsgImpl->Login( nickname, groupName );
}

/**
 * ログアウト（サービス脱退通知）。
 * ・BR_EXITをブロードキャスト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::Logout()
{	
	ipmsgImpl->Logout();
}

/**
 * ホストリスト取得。
 * @retval エージェントが保持しているHostListオブジェクト
 * @retval ホストリスト
 */
HostList&
IpMessengerAgent::GetHostList()
{
	return ipmsgImpl->GetHostList();
}

/**
 * ホストリスト更新取得。
 * ・BR_ISGETLIST2をブロードキャスト。
 * ・他のメソッド（ANSLIST受信）にて取得するまで待機。（五回まで）
 * 注：ホストリストの構築はANSLIST受信時に行われるので、このメソッドではひたすら待機。
 * 注：ホストリストはANSLIST受信時に追加／更新されることがあるので常に同じホストリストを返すとは限らない。
 * 注：このメソッドはスレッドセーフでない。
 * @retval 取得したHostListオブジェクト
 */
HostList&
IpMessengerAgent::UpdateHostList()
{
	return ipmsgImpl->UpdateHostList();
}

/**
 * 不在モードかどうかを判定。
 * @retval 設定済の不在モードを返す。
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgent::IsAbsence()
{
	return ipmsgImpl->IsAbsence();
}
/**
 * 不在モードをクリアする。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::ResetAbsence()
{
	ipmsgImpl->ResetAbsence();
}

/**
 * 不在モードを設定する。
 * @param encoding ローカルエンコーディング
 * @param absenceModes AbsenceModeオブジェクトのベクタ（自動応答時に複数エンコーディング対応するため）
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::SetAbsence( string encoding, vector<AbsenceMode> absenceModes )
{
	ipmsgImpl->SetAbsence( encoding, absenceModes );
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param opt 送信オプション
 * 注：このメソッドはスレッドセーフでない。
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, isLockPassword, hostCountAtSameTime, opt );
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param file 添付ファイル
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param opt 送信オプション
 * 注：このメソッドはスレッドセーフでない。
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, file, isLockPassword, hostCountAtSameTime, opt );
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param files 添付ファイル群
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param opt 送信オプション
 * 注：このメソッドはスレッドセーフでない。
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, opt );
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::ClearBroadcastAddress()
{
	ipmsgImpl->ClearBroadcastAddress();
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::DeleteBroadcastAddress( string addr )
{
	ipmsgImpl->DeleteBroadcastAddress( addr );
}

/**
 * ブロードキャストアドレスを登録
 * @param addr 登録するブロードキャストアドレス
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::AddBroadcastAddress( string addr )
{
	ipmsgImpl->AddBroadcastAddress( addr );
}

/**
 * 対象ホストのバージョン情報を取得。
 * ・GETINFOパケットを送信。
 * ・他のメソッド（ANSINFO受信）にて取得するまで待機。（五回まで）
 * ・IPアドレスでマッチングしてANSINFOで更新されたバージョン情報を取得
 * @param host 対象のホスト
 * @retval 対象ホストのバージョン情報
 * 注：このメソッドはスレッドセーフでない。
 */
string
IpMessengerAgent::GetInfo( HostListItem& host )
{
	return ipmsgImpl->GetInfo( host );
#if 0
#endif
}

/**
 * 対象ホストの不在説明文字列情報を取得。
 * ・GETABSENCEINFOパケットを送信。
 * ・他のメソッド（ANSABSENCEINFO受信）にて取得するまで待機。（五回まで）
 * ・IPアドレスでマッチングしてANSABSENCEINFOで更新されたバージョン情報を取得
 * @param host 対象のホスト
 * @retval 対象ホストの不在説明文字列情報
 * 注：このメソッドはスレッドセーフでない。
 */
string
IpMessengerAgent::GetAbsenceInfo( HostListItem& host )
{
	return ipmsgImpl->GetAbsenceInfo( host );
}

/**
 * 保持中のホストリストからグループリストを取得する。
 * @retval グループリスト
 * 注：このメソッドはスレッドセーフでない。
 */
vector<string>
IpMessengerAgent::GetGroupList()
{
	return ipmsgImpl->GetGroupList();
}

/**
 * 送信元にメッセージを削除したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::DeleteNotify( RecievedMessage msg )
{
	ipmsgImpl->DeleteNotify( msg );
}

/**
 * 送信元にメッセージを開封したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )
{
	ipmsgImpl->ConfirmMessage( msg );
}

/**
 * 送信済メッセージリストに開封されたことをマークする。
 * @param msg 送信メッセージオブジェクト。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgent::AcceptConfirmNotify( SentMessage msg )
{
	ipmsgImpl->AcceptConfirmNotify( msg );
}
		
// private methods start here

/**
 * 受信処理（ユーザ向け）。
 * 注：このメソッドはスレッドセーフでない。
 */
int
IpMessengerAgent::Process()
{
	return ipmsgImpl->Process();
}

/**
 * 受信メッセージの個数を取得する。
 * @retval 受信メッセージの個数
 */
int
IpMessengerAgent::GetRecievedMessageCount()
{
	return ipmsgImpl->GetRecievedMessageCount();
}

/**
 * 受信メッセージを一個取り出し、受信メッセージリストから削除する。
 * @retval 受信メッセージオブジェクト。
 */
RecievedMessage
IpMessengerAgent::PopRecievedMessage()
{
	return ipmsgImpl->PopRecievedMessage();
}

/**
 * 送信済メッセージリストのポインタを取得する。
 * @retval 送信済メッセージリストのポインタ。
 */
SentMessageList *
IpMessengerAgent::GetSentMessages()
{
	return ipmsgImpl->GetSentMessages();
}

/**
 * 送信済メッセージリストのコピーを取得する。
 * @retval 送信済メッセージリストのコピー。
 */
SentMessageList
IpMessengerAgent::CloneSentMessages()
{
	return ipmsgImpl->CloneSentMessages();
}

/**
 * ログイン名のゲッター
 * @retval ログイン名
 */
string
IpMessengerAgent::LoginName()
{
	return ipmsgImpl->LoginName();
}

/**
 * ホスト名のゲッター
 * @retval ホスト名
 */
string
IpMessengerAgent::HostName()
{
	return ipmsgImpl->HostName();
}

/**
 * ダイヤルアップのゲッター
 * @retval ダイヤルアップ
 */
bool
IpMessengerAgent::IsDialup()
{
	return ipmsgImpl->IsDialup();
}

/**
 * ダイヤルアップのセッター
 * @param ダイヤルアップ
 */
void
IpMessengerAgent::setIsDialup( bool isDialup )
{
	ipmsgImpl->setIsDialup( isDialup );
}

/**
 * ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグのゲッター
 * @retval ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグ
 */
bool
IpMessengerAgent::AbortDownloadAtFileChanged()
{
	return ipmsgImpl->AbortDownloadAtFileChanged();
}

/**
 * ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグのセッター
 * @param ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグ
 */
void
IpMessengerAgent::setAbortDownloadAtFileChanged( bool isAbort )
{
	ipmsgImpl->setAbortDownloadAtFileChanged( isAbort );
}

/**
 * 送信メッセージを保存するかどうかのフラグのゲッター
 * @retval 送信メッセージを保存するかどうかのフラグ
 */
bool
IpMessengerAgent::SaveSentMessage()
{
	return ipmsgImpl->SaveSentMessage();
}

/**
 * 送信メッセージを保存するかどうかのフラグのセッター
 * @param 送信メッセージを保存するかどうかのフラグ
 */
void
IpMessengerAgent::setSaveSentMessage( bool isSave )
{
	ipmsgImpl->setSaveSentMessage( isSave );
}

/**
 * 受信メッセージを保存するかどうかのフラグのゲッター
 * @retval 受信メッセージを保存するかどうかのフラグ
 */
bool
IpMessengerAgent::SaveRecievedMessage()
{
	return ipmsgImpl->SaveRecievedMessage();
}

/**
 * 受信メッセージを保存するかどうかのフラグのセッター
 * @param 受信メッセージを保存するかどうかのフラグ
 */
void
IpMessengerAgent::setSaveRecievedMessage( bool isSave )
{
	ipmsgImpl->setSaveRecievedMessage( isSave );
}
//end of source
