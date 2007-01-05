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

static pthread_mutex_t instanceMutex;
static int mutex_init_result = IpMsgMutexInit( "IpMessenger::Global", &instanceMutex, NULL );

IpMessengerEvent::~IpMessengerEvent(){};
/**
 * IP メッセンジャエージェントクラスのインスタンスを取得する。
 * Singletonパターンを採用しているので、ホスト唯一のインスタンスでなければならない。
 * 注：このインスタンスはスレッドセーフでない。
 */
IpMessengerAgent *
IpMessengerAgent::GetInstance()
{
	IpMsgMutexLock( "IpMessengerAgent::GetInstance()", &instanceMutex );
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgent();
	}
	IpMsgMutexUnlock( "IpMessengerAgent::GetInstance()", &instanceMutex );
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
	IpMsgMutexLock( "IpMessengerAgent::Release()", &instanceMutex );
	if ( myInstance == NULL ) {
		IpMsgMutexUnlock( "IpMessengerAgent::Release()", &instanceMutex );
		return;
	}
	delete myInstance;
	myInstance = NULL;
	IpMsgMutexUnlock( "IpMessengerAgent::Release()", &instanceMutex );
}

/**
 * IP メッセンジャエージェントクラスのコンストラクタ。
 */
IpMessengerAgent::IpMessengerAgent()
{
	ipmsgImpl = IpMessengerAgentImpl::GetInstance();
}

/**
 * IP メッセンジャエージェントクラスのデストラクタ。
 */
IpMessengerAgent::~IpMessengerAgent()
{
	IpMessengerAgentImpl::Release();
}

/**
 * NICを指定せずにIP メッセンジャエージェントクラスのネットワークを起動する。
 * ・全てのNICに対してデフォルトポートでネットワークを起動する。
 */
void
IpMessengerAgent::StartNetwork()
{
	ipmsgImpl->StartNetwork();
}

/**
 * IP メッセンジャエージェントクラスのネットワークを起動する。
 * @parem nics 起動時に対象とするNICのベクタ。
 */
void
IpMessengerAgent::StartNetwork( const vector<NetworkInterface>& nics )
{
	ipmsgImpl->StartNetwork( nics );
}

/**
 * IP メッセンジャエージェントクラスのネットワークを停止する。
 */
void
IpMessengerAgent::StopNetwork()
{
	ipmsgImpl->StopNetwork();
}

/**
 * NICを指定せずにIP メッセンジャエージェントクラスのネットワークを再起動する。
 * ・全てのNICに対してデフォルトポートでネットワークを起動する。
 */
void
IpMessengerAgent::RestartNetwork()
{
	ipmsgImpl->RestartNetwork();
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * @parem nics 起動時に対象とするNICのベクタ。
 */
void
IpMessengerAgent::RestartNetwork( const vector<NetworkInterface>& nics )
{
	ipmsgImpl->RestartNetwork( nics );
}

/**
 * ファイル名コンバータのゲッター。
 * @retval コンバータのアドレス。
 */
FileNameConverter *
IpMessengerAgent::GetFileNameConverter()
{
	return ipmsgImpl->GetFileNameConverter();
}

/**
 * ファイル名コンバータのセッター。
 * @param conv コンバータのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetFileNameConverter( FileNameConverter *conv )
{
	ipmsgImpl->SetFileNameConverter( conv );
}

/**
 * イベントオブジェクトのゲッター。
 * @retval イベントオブジェクトのアドレス。
 */
HostListComparator *
IpMessengerAgent::GetSortHostListComparator()
{
	return ipmsgImpl->GetSortHostListComparator();
}; 

/**
 * ホストリスト比較オブジェクトのセッター。
 * @param comparator ホストリスト比較オブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetSortHostListComparator( HostListComparator *comparator )
{
	ipmsgImpl->SetSortHostListComparator( comparator );
}

/**
 * イベントオブジェクトのゲッター。
 * @retval イベントオブジェクトのアドレス。
 */
IpMessengerEvent *
IpMessengerAgent::GetEventObject()
{
	return ipmsgImpl->GetEventObject();
}; 

/**
 * イベントオブジェクトのセッター。
 * @param conv イベントオブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetEventObject( IpMessengerEvent *evt )
{
	ipmsgImpl->SetEventObject( evt );
}

/**
 * NICの情報を取得する。
 * @param nics ネットワークインターフェースの一覧
 */
void
IpMessengerAgent::GetNetworkInterfaceInfo( vector<NetworkInterface>& nics )
{
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( nics );
}

/**
 * ログイン（サービス参加通知）。
 */
void
IpMessengerAgent::Login( string nickname, string groupName )
{
	ipmsgImpl->Login( nickname, groupName );
}

/**
 * ログアウト（サービス脱退通知）。
 */
void
IpMessengerAgent::Logout()
{	
	ipmsgImpl->Logout();
}

/**
 * ホストリスト取得。
 * @retval エージェントが保持しているHostListオブジェクト
 */
HostList&
IpMessengerAgent::GetHostList()
{
	return ipmsgImpl->GetHostList();
}

/**
 * ホストリスト更新取得。
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
 */
bool
IpMessengerAgent::IsAbsence()
{
	return ipmsgImpl->IsAbsence();
}
/**
 * 不在モードをクリアする。
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
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, isLockPassword, hostCountAtSameTime, IsNoLogging, opt );
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
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFile& file, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, file, isLockPassword, hostCountAtSameTime, IsNoLogging, opt );
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
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList& files, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, IsNoLogging, opt );
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 */
void
IpMessengerAgent::ClearBroadcastAddress()
{
	ipmsgImpl->ClearBroadcastAddress();
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 */
void
IpMessengerAgent::DeleteBroadcastAddress( string addr )
{
	ipmsgImpl->DeleteBroadcastAddress( addr );
}

/**
 * ブロードキャストアドレスを登録
 * @param addr 登録するブロードキャストアドレス
 */
void
IpMessengerAgent::AddBroadcastAddress( string addr )
{
	ipmsgImpl->AddBroadcastAddress( addr );
}

/**
 * 対象ホストのバージョン情報を取得。
 * @param host 対象のホスト
 * @retval 対象ホストのバージョン情報
 */
string
IpMessengerAgent::GetInfo( HostListItem& host )
{
	return ipmsgImpl->GetInfo( host );
}

/**
 * 対象ホストの不在説明文字列情報を取得。
 * @param host 対象のホスト
 * @retval 対象ホストの不在説明文字列情報
 */
string
IpMessengerAgent::GetAbsenceInfo( HostListItem& host )
{
	return ipmsgImpl->GetAbsenceInfo( host );
}

/**
 * 保持中のホストリストからグループリストを取得する。
 * @retval グループリスト
 */
vector<GroupItem>
IpMessengerAgent::GetGroupList()
{
	return ipmsgImpl->GetGroupList();
}

/**
 * 送信元にメッセージを削除したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 */
void
IpMessengerAgent::DeleteNotify( RecievedMessage msg )
{
	ipmsgImpl->DeleteNotify( msg );
}

/**
 * 送信元にメッセージを開封したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 */
void
IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )
{
	ipmsgImpl->ConfirmMessage( msg );
}

/**
 * 送信済メッセージリストに開封されたことをマークする。
 * @param msg 送信メッセージオブジェクト。
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

/**
 * ダウンロード速度を算出する。
 * @retval ダウンロード速度（バイト／秒）。
 */
long double
DownloadInfo::getSpeed()
{
	return Time() == 0 ? (long double)0 : ( ( long double )Size() / ( long double )Time() );
}

/**
 * ダウンロード速度文字列を生成する。
 * @retval ダウンロード速度文字列（単位／秒）。
 */
string
DownloadInfo::getSpeedString()
{
	return DownloadInfo::getUnitSizeString( ( long long )getSpeed() ) + "/sec";
}

/**
 * 容量文字列を生成する。
 * @retval 容量文字列（単位）。
 */
string
DownloadInfo::getSizeString()
{
	return DownloadInfo::getUnitSizeString( Size() );
}

#define IPMSG_SIZE_B	(long double)(1)
#define IPMSG_SIZE_KB	(long double)(1024 * IPMSG_SIZE_B)
#define IPMSG_SIZE_MB	(long double)(1024 * IPMSG_SIZE_KB)
#define IPMSG_SIZE_GB	(long double)(1024 * IPMSG_SIZE_MB)
#define IPMSG_SIZE_TB	(long double)(1024 * IPMSG_SIZE_GB)

/**
 * 容量文字列を生成する。
 * @retval 容量文字列（単位）。
 */
string
DownloadInfo::getUnitSizeString( long long size )
{
	long double dsize = (long double)size;
	char buf[100];
	if ( dsize >= IPMSG_SIZE_TB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf TB", (dsize / IPMSG_SIZE_TB) );
		return buf;
	} else if ( dsize >= IPMSG_SIZE_GB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf GB", (dsize / IPMSG_SIZE_GB) );
		return buf;
	} else if ( dsize >= IPMSG_SIZE_MB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf MB", (dsize / IPMSG_SIZE_MB) );
		return buf;
	} else if ( dsize >= IPMSG_SIZE_KB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf KB", (dsize / IPMSG_SIZE_KB) );
		return buf;
	}
	snprintf( buf, sizeof( buf ), "%lld B", size );
	return buf;
}
//end of source
