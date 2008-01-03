/**
 * IP メッセンジャライブラリ(Unix用)
 * IPメッセンジャエージェントクラス。
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"

using namespace ipmsg;

static IpMessengerAgent *myInstance = NULL;

static pthread_mutex_t instanceMutex;
static int mutex_init_result = IpMsgMutexInit( "IpMessenger::Global", &instanceMutex, NULL );

IpMessengerEvent::~IpMessengerEvent()
{
	IPMSG_FUNC_ENTER("IpMessengerEvent::~IpMessengerEvent()");
	IPMSG_FUNC_EXIT;
};
/**
 * IP メッセンジャエージェントクラスのインスタンスを取得する。
 * <ul>
 * <li>Singletonパターンを採用しているので、ホスト唯一のインスタンスでなければならない。</li>
 * </ul>
 */
IpMessengerAgent *
IpMessengerAgent::GetInstance()
{
	IPMSG_FUNC_ENTER("IpMessengerAgent * IpMessengerAgent::GetInstance()");
	mutex_init_result = 0; //fix warnings. but no effect.
	IpMsgMutexLock( "IpMessengerAgent::GetInstance()", &instanceMutex );
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgent();
	}
	IpMsgMutexUnlock( "IpMessengerAgent::GetInstance()", &instanceMutex );
	IPMSG_FUNC_RETURN( myInstance );
}

/**
 * IP メッセンジャエージェントクラスのインスタンスを解放する。
 * <ul>
 * <li>このメソッドを使ってオブジェクトを解放しなければならない。</li>
 * <li>ライブラリを通じないで直接deleteされた場合はその後の動作について関知しない。</li>
 * </ul>
 */
void
IpMessengerAgent::Release()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::Release()");
	IpMsgMutexLock( "IpMessengerAgent::Release()", &instanceMutex );
	if ( myInstance == NULL ) {
		IpMsgMutexUnlock( "IpMessengerAgent::Release()", &instanceMutex );
		IPMSG_FUNC_EXIT;
	}
	delete myInstance;
	myInstance = NULL;
	IpMsgMutexUnlock( "IpMessengerAgent::Release()", &instanceMutex );
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのコンストラクタ。
 */
IpMessengerAgent::IpMessengerAgent()
{
	IPMSG_FUNC_ENTER("IpMessengerAgent::IpMessengerAgent()");
	if ( isSupportIPv4() ) {
		printf("This host support IPv4.\n");
	} else {
		printf("This host not support IPv4.\n");
	}
	if ( isSupportIPv6() ) {
		printf("This host support IPv6.\n");
	} else {
		printf("This host not support IPv6.\n");
	}
	ipmsgImpl = IpMessengerAgentImpl::GetInstance();
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのデストラクタ。
 */
IpMessengerAgent::~IpMessengerAgent()
{
	IPMSG_FUNC_ENTER("IpMessengerAgent::~IpMessengerAgent()");
	IpMessengerAgentImpl::Release();
	IPMSG_FUNC_EXIT;
}

/**
 * NICを指定せずにIP メッセンジャエージェントクラスのネットワークを起動する。
 * <ul>
 * <li>全てのNICに対してデフォルトポートでネットワークを起動する。</li>
 * </ul>
 */
void
IpMessengerAgent::StartNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::StartNetwork()");
	ipmsgImpl->StartNetwork();
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを起動する。
 * @parem nics 起動時に対象とするNICのベクタ。
 */
void
IpMessengerAgent::StartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::StartNetwork( const std::vector<NetworkInterface>& nics )");
	ipmsgImpl->StartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを停止する。
 */
void
IpMessengerAgent::StopNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::StopNetwork()");
	ipmsgImpl->StopNetwork();
	IPMSG_FUNC_EXIT;
}

/**
 * NICを指定せずにIP メッセンジャエージェントクラスのネットワークを再起動する。
 * <ul>
 * <li>全てのNICに対してデフォルトポートでネットワークを起動する。</li>
 * </ul>
 */
void
IpMessengerAgent::RestartNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::RestartNetwork()");
	ipmsgImpl->RestartNetwork();
	IPMSG_FUNC_EXIT;
}

/**
 * IP メッセンジャエージェントクラスのネットワークを再起動する。
 * @parem nics 起動時に対象とするNICのベクタ。
 */
void
IpMessengerAgent::RestartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::RestartNetwork( const std::vector<NetworkInterface>& nics )");
	ipmsgImpl->RestartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * ファイル名コンバータのゲッター。
 * @retval コンバータのアドレス。
 */
FileNameConverter *
IpMessengerAgent::GetFileNameConverter() const
{
	IPMSG_FUNC_ENTER("FileNameConverter *IpMessengerAgent::GetFileNameConverter() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetFileNameConverter() );
}

/**
 * ファイル名コンバータのセッター。
 * @param conv コンバータのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetFileNameConverter( const FileNameConverter *conv )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetFileNameConverter( const FileNameConverter *conv )");
	ipmsgImpl->SetFileNameConverter( conv );
	IPMSG_FUNC_EXIT;
}

/**
 * イベントオブジェクトのゲッター。
 * @retval イベントオブジェクトのアドレス。
 */
HostListComparator *
IpMessengerAgent::GetSortHostListComparator() const
{
	IPMSG_FUNC_ENTER("HostListComparator *IpMessengerAgent::GetSortHostListComparator() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetSortHostListComparator() );
}; 

/**
 * ホストリスト比較オブジェクトのセッター。
 * @param comparator ホストリスト比較オブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetSortHostListComparator( const HostListComparator *comparator )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetSortHostListComparator( const HostListComparator *comparator )");
	ipmsgImpl->SetSortHostListComparator( comparator );
	IPMSG_FUNC_EXIT;
}

/**
 * イベントオブジェクトのゲッター。
 * @retval イベントオブジェクトのアドレス。
 */
IpMessengerEvent *
IpMessengerAgent::GetEventObject() const
{
	IPMSG_FUNC_ENTER("IpMessengerEvent *IpMessengerAgent::GetEventObject() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetEventObject() );
}; 

/**
 * イベントオブジェクトのセッター。
 * @param evt イベントオブジェクトのアドレス。自動的に削除されるので、スタック上に作成してはならない。ヒープ上に作成すること。
 */
void
IpMessengerAgent::SetEventObject( const IpMessengerEvent *evt )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetEventObject( const IpMessengerEvent *evt )");
	ipmsgImpl->SetEventObject( evt );
	IPMSG_FUNC_EXIT;
}

/**
 * NICの情報を取得する。
 * @param nics ネットワークインターフェースの一覧
 */
void
IpMessengerAgent::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6 )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6 )");
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( nics, useIPv6 );
	IPMSG_FUNC_EXIT;
}

/**
 * ログイン（サービス参加通知）。
 */
void
IpMessengerAgent::Login( std::string nickname, std::string groupName )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::Login( std::string nickname, std::string groupName )");
	ipmsgImpl->Login( nickname, groupName );
	IPMSG_FUNC_EXIT;
}

/**
 * ログアウト（サービス脱退通知）。
 */
void
IpMessengerAgent::Logout()
{	
	IPMSG_FUNC_ENTER("void IpMessengerAgent::Logout()");
	ipmsgImpl->Logout();
	IPMSG_FUNC_EXIT;
}

/**
 * ホストリスト取得。
 * @retval エージェントが保持しているHostListオブジェクト
 */
HostList&
IpMessengerAgent::GetHostList()
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgent::GetHostList()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetHostList() );
}

/**
 * ホストリスト更新取得。
 * @retval 取得したHostListオブジェクト
 */
HostList&
IpMessengerAgent::UpdateHostList()
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgent::UpdateHostList()");
	IPMSG_FUNC_RETURN( ipmsgImpl->UpdateHostList() );
}

/**
 * 不在モードかどうかを判定。
 * @retval 設定済の不在モードを返す。
 */
bool
IpMessengerAgent::IsAbsence() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::IsAbsence() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->IsAbsence() );
}
/**
 * 不在モードをクリアする。
 */
void
IpMessengerAgent::ResetAbsence()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ResetAbsence()");
	ipmsgImpl->ResetAbsence();
	IPMSG_FUNC_EXIT;
}

/**
 * 不在モードを設定する。
 * @param encoding ローカルエンコーディング
 * @param absenceModes AbsenceModeオブジェクトのベクタ（自動応答時に複数エンコーディング対応するため）
 */
void
IpMessengerAgent::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )");
	ipmsgImpl->SetAbsence( encoding, absenceModes );
	IPMSG_FUNC_EXIT;
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param IsNoLogging ログに残さない（ことを推奨）
 * @param opt 送信オプション
 */
bool
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )");
	IPMSG_FUNC_RETURN( ipmsgImpl->SendMsg( host, msg, isSecret, isLockPassword, hostCountAtSameTime, IsLogging, opt ) );
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param file 添付ファイル
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param IsNoLogging ログに残さない（ことを推奨）
 * @param opt 送信オプション
 */
bool
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )");
	IPMSG_FUNC_RETURN( ipmsgImpl->SendMsg( host, msg, isSecret, file, isLockPassword, hostCountAtSameTime, IsLogging, opt ) );
}

/**
 * メッセージ送信。
 * @param host 送信先ホスト
 * @param msg 送信メッセージ
 * @param isSecret 封書かどうかを示すフラグ
 * @param files 添付ファイル群
 * @param isLockPassword 錠つきかどうかを示すフラグ
 * @param hostCountAtSameTime 同時送信ホスト数
 * @param IsNoLogging ログに残さない（ことを推奨）
 * @param opt 送信オプション
 */
bool
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )");
	IPMSG_FUNC_RETURN( ipmsgImpl->SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, IsLogging, opt ) );
}

/**
 * 登録済のブロードキャストアドレスを全てクリア
 */
void
IpMessengerAgent::ClearBroadcastAddress()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ClearBroadcastAddress()");
	ipmsgImpl->ClearBroadcastAddress();
	IPMSG_FUNC_EXIT;
}

/**
 * 登録済のブロードキャストアドレスを削除
 * @param addr 登録済のブロードキャストアドレス
 */
void
IpMessengerAgent::DeleteBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteBroadcastAddress( std::string addr )");
	ipmsgImpl->DeleteBroadcastAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * ブロードキャストアドレスを登録
 * @param addr 登録するブロードキャストアドレス
 */
void
IpMessengerAgent::AddBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::AddBroadcastAddress( std::string addr )");
	ipmsgImpl->AddBroadcastAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * 特定のホストから隠れる場合、見えなくするホストのリストをクリア
 */
void
IpMessengerAgent::ClearSkulkHost()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ClearSkulkHost()");
	ipmsgImpl->ClearSkulkHost();
	IPMSG_FUNC_EXIT;
}

/**
 * 特定のホストから隠れる場合、見えなくするホストのリストからアドレスを削除
 * @param host 登録済の見えなくする（隠れる）ホストアドレス
 */
void
IpMessengerAgent::DeleteSkulkHostAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteSkulkHostAddress( std::string addr )");
	ipmsgImpl->DeleteSkulkHostAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * 特定のホストから隠れる場合、見えなくするホストのリストからホストを削除
 * @param host 登録済の見えなくする（隠れる）ホスト
 */
void
IpMessengerAgent::DeleteSkulkHost( HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteSkulkHost( HostListItem &host )");
	ipmsgImpl->DeleteSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * 特定のホストから隠れる場合、見えなくするホストのリストにホストアドレスを追加
 * @param host 登録する見えなくする（隠れる）ホストアドレス
 */
void
IpMessengerAgent::AddSkulkHostAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::AddSkulkHostAddress( std::string )");
	ipmsgImpl->AddSkulkHostAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * 特定のホストから隠れる場合、見えなくするホストのリストにホストを追加
 * @param host 登録する見えなくする（隠れる）ホスト
 */
void
IpMessengerAgent::AddSkulkHost( HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::AddSkulkHost( HostListItem &host )");
	ipmsgImpl->AddSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * 対象ホストのバージョン情報を取得。
 * @param host 対象のホスト
 * @retval 対象ホストのバージョン情報
 */
std::string
IpMessengerAgent::GetInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::GetInfo( HostListItem& host )");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetInfo( host ) );
}

/**
 * 対象ホストの不在説明文字列情報を取得。
 * @param host 対象のホスト
 * @retval 対象ホストの不在説明文字列情報
 */
std::string
IpMessengerAgent::GetAbsenceInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::GetAbsenceInfo( HostListItem& host )");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetAbsenceInfo( host ) );
}

/**
 * 保持中のホストリストからグループリストを取得する。
 * @retval グループリスト
 */
std::vector<GroupItem>
IpMessengerAgent::GetGroupList()
{
	IPMSG_FUNC_ENTER("std::vector<GroupItem> IpMessengerAgent::GetGroupList()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetGroupList() );
}

/**
 * 送信元にメッセージを削除したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 */
void
IpMessengerAgent::DeleteNotify( RecievedMessage msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteNotify( RecievedMessage msg )");
	ipmsgImpl->DeleteNotify( msg );
	IPMSG_FUNC_EXIT;
}

/**
 * 送信元にメッセージを開封したことを通知する。
 * @param msg 受信メッセージオブジェクト。
 */
void
IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )");
	ipmsgImpl->ConfirmMessage( msg );
	IPMSG_FUNC_EXIT;
}

/**
 * 送信済メッセージリストに開封されたことをマークする。
 * @param msg 送信メッセージオブジェクト。
 */
void
IpMessengerAgent::AcceptConfirmNotify( SentMessage msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::AcceptConfirmNotify( SentMessage msg )");
	ipmsgImpl->AcceptConfirmNotify( msg );
	IPMSG_FUNC_EXIT;
}
		
// private methods start here

/**
 * 受信処理（ユーザ向け）。
 * 注：このメソッドはスレッドセーフでない。
 */
int
IpMessengerAgent::Process()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgent::Process()");
	IPMSG_FUNC_RETURN( ipmsgImpl->Process() );
}

/**
 * 受信メッセージの個数を取得する。
 * @retval 受信メッセージの個数
 */
int
IpMessengerAgent::GetRecievedMessageCount()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgent::GetRecievedMessageCount()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetRecievedMessageCount() );
}

/**
 * 受信メッセージを一個取り出し、受信メッセージリストから削除する。
 * @retval 受信メッセージオブジェクト。
 */
RecievedMessage
IpMessengerAgent::PopRecievedMessage()
{
	IPMSG_FUNC_ENTER("RecievedMessage IpMessengerAgent::PopRecievedMessage()");
	IPMSG_FUNC_RETURN( ipmsgImpl->PopRecievedMessage() );
}

/**
 * 送信済メッセージリストのポインタを取得する。
 * @retval 送信済メッセージリストのポインタ。
 */
SentMessageList *
IpMessengerAgent::GetSentMessages()
{
	IPMSG_FUNC_ENTER("SentMessageList *IpMessengerAgent::GetSentMessages()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetSentMessages() );
}

/**
 * 送信済メッセージリストのコピーを取得する。
 * @retval 送信済メッセージリストのコピー。
 */
SentMessageList
IpMessengerAgent::CloneSentMessages() const
{
	IPMSG_FUNC_ENTER("SentMessageList IpMessengerAgent::CloneSentMessages() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->CloneSentMessages() );
}

/**
 * ログイン名のゲッター
 * @retval ログイン名
 */
std::string
IpMessengerAgent::LoginName() const
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::LoginName() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->LoginName() );
}

/**
 * ホスト名のゲッター
 * @retval ホスト名
 */
std::string
IpMessengerAgent::HostName() const
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::HostName() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->HostName() );
}

/**
 * デフォルトポートのゲッター
 * @retval デフォルトポート
 */
int
IpMessengerAgent::DefaultPortNo() const
{
	IPMSG_FUNC_ENTER("int IpMessengerAgent::DefaultPortNo() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->DefaultPortNo() );
}

/**
 * デフォルトポートのセッター
 * @param defaultPortNo デフォルトポート
 */
void
IpMessengerAgent::setDefaultPortNo( const int defaultPortNo )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setDefaultPortNo( const int defaultPortNo )");
	ipmsgImpl->setDefaultPortNo( defaultPortNo );
	IPMSG_FUNC_EXIT;
}

/**
 * ダイヤルアップのゲッター
 * @retval ダイヤルアップ
 */
bool
IpMessengerAgent::IsDialup() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::IsDialup() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->IsDialup() );
}

/**
 * ダイヤルアップのセッター
 * @param isDialup ダイヤルアップ
 */
void
IpMessengerAgent::setIsDialup( const bool isDialup )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setIsDialup( const bool isDialup )");
	ipmsgImpl->setIsDialup( isDialup );
	IPMSG_FUNC_EXIT;
}

/**
 * ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグのゲッター
 * @retval ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグ
 */
bool
IpMessengerAgent::AbortDownloadAtFileChanged() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::AbortDownloadAtFileChanged() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->AbortDownloadAtFileChanged() );
}

/**
 * ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグのセッター
 * @param isAbort ダウンロード時にファイルが変更された場合に禁止するかどうかのフラグ
 */
void
IpMessengerAgent::setAbortDownloadAtFileChanged( const bool isAbort )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setAbortDownloadAtFileChanged( const bool isAbort )");
	ipmsgImpl->setAbortDownloadAtFileChanged( isAbort );
	IPMSG_FUNC_EXIT;
}

/**
 * 送信メッセージを保存するかどうかのフラグのゲッター
 * @retval 送信メッセージを保存するかどうかのフラグ
 */
bool
IpMessengerAgent::SaveSentMessage() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SaveSentMessage() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->SaveSentMessage() );
}

/**
 * 送信メッセージを保存するかどうかのフラグのセッター
 * @param isSave 送信メッセージを保存するかどうかのフラグ
 */
void
IpMessengerAgent::setSaveSentMessage( const bool isSave )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setSaveSentMessage( const bool isSave )");
	ipmsgImpl->setSaveSentMessage( isSave );
	IPMSG_FUNC_EXIT;
}

/**
 * 受信メッセージを保存するかどうかのフラグのゲッター
 * @retval 受信メッセージを保存するかどうかのフラグ
 */
bool
IpMessengerAgent::SaveRecievedMessage() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SaveRecievedMessage() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->SaveRecievedMessage() );
}

/**
 * 受信メッセージを保存するかどうかのフラグのセッター
 * @param isSave 受信メッセージを保存するかどうかのフラグ
 */
void
IpMessengerAgent::setSaveRecievedMessage( const bool isSave )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setSaveRecievedMessage( const bool isSave )");
	ipmsgImpl->setSaveRecievedMessage( isSave );
	IPMSG_FUNC_EXIT;
}

/**
 * 暗号化に失敗したらメッセージを送信しないかどうかのフラグのゲッター
 * @retval 暗号化に失敗したらメッセージを送信しないかどうかのフラグ
 */
bool
IpMessengerAgent::NoSendMessageOnEncryptionFailed() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::NoSendMessageOnEncryptionFailed() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->NoSendMessageOnEncryptionFailed() );
}

/**
 * 暗号化に失敗したらメッセージを送信しないかどうかのフラグのセッター
 * @param isNoSend 暗号化に失敗したらメッセージを送信しないかどうかのフラグ
 */
void
IpMessengerAgent::setNoSendMessageOnEncryptionFailed( const bool isNoSend )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setNoSendMessageOnEncryptionFailed( const bool isNoSend )");
	IPMSG_FUNC_RETURN( ipmsgImpl->setNoSendMessageOnEncryptionFailed( isNoSend ) );
}

/**
 * IPv6を使うかどうかのフラグのゲッター
 * @retval IPv6を使うかどうかのフラグ
 */
bool
IpMessengerAgent::UseIPv6() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::UseIPv6() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->UseIPv6() );
}

/**
 * IPv6を使うかどうかのフラグのセッター
 * @param useIPv6 IPv6を使うかどうかのフラグ
 */
void
IpMessengerAgent::setUseIPv6( const bool useIPv6 )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setUseIPv6( const bool useIPv6 )");
#ifdef ENABLE_IPV6
	ipmsgImpl->setUseIPv6( useIPv6 );
#else
	ipmsgImpl->setUseIPv6( false );
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * IPv6をサポートしているかビルドオプションを取得する。
 * @retval true:IPv6をサポート
 * @retval false:IPv6をサポートしていない。
 */
bool
IpMessengerAgent::isSupportIPv6()
{
#ifdef ENABLE_IPV6
	return true;
#else
	return false;
#endif
}

/**
 * IPv4をサポートしているかビルドオプションを取得する。
 * @retval true:IPv4をサポート
 * @retval false:IPv4をサポートしていない。
 */
bool
IpMessengerAgent::isSupportIPv4()
{
#ifdef ENABLE_IPV4
	return true;
#else
	return false;
#endif
}

/**
 * ダウンロード速度を算出する。
 * @retval ダウンロード速度（バイト／秒）。
 */
long double
DownloadInfo::getSpeed()
{
	IPMSG_FUNC_ENTER("long double DownloadInfo::getSpeed()");
	IPMSG_FUNC_RETURN( Time() == 0 ? (long double)0 : ( ( long double )Size() / ( long double )Time() ) );
}

/**
 * ダウンロード速度文字列を生成する。
 * @retval ダウンロード速度文字列（単位／秒）。例:1 B/sec, 2.00KB/sec, 3.00 MB/sec, 4.00 GB/sec, 5.00 TB/sec
 */
std::string
DownloadInfo::getSpeedString()
{
	IPMSG_FUNC_ENTER("std::string DownloadInfo::getSpeedString()");
	IPMSG_FUNC_RETURN( DownloadInfo::getUnitSizeString( ( long long )getSpeed() ) + "/sec" );
}

/**
 * 容量文字列を生成する。
 * @retval 容量文字列（単位）。例:1 B, 2.00KB, 3.00 MB, 4.00 GB, 5.00 TB
 */
std::string
DownloadInfo::getSizeString()
{
	IPMSG_FUNC_ENTER("std::string DownloadInfo::getSizeString()");
	IPMSG_FUNC_RETURN( DownloadInfo::getUnitSizeString( Size() ) );
}

#define IPMSG_SIZE_B	(long double)(1)
#define IPMSG_SIZE_KB	(long double)(1024 * IPMSG_SIZE_B)
#define IPMSG_SIZE_MB	(long double)(1024 * IPMSG_SIZE_KB)
#define IPMSG_SIZE_GB	(long double)(1024 * IPMSG_SIZE_MB)
#define IPMSG_SIZE_TB	(long double)(1024 * IPMSG_SIZE_GB)

/**
 * 容量文字列を生成する。
 * @retval 容量文字列（単位）。例:1 B, 2.00KB, 3.00 MB, 4.00 GB, 5.00 TB
 */
std::string
DownloadInfo::getUnitSizeString( long long size )
{
	IPMSG_FUNC_ENTER("std::string DownloadInfo::getUnitSizeString( long long size )");
	long double dsize = (long double)size;
	char buf[100];
	if ( dsize >= IPMSG_SIZE_TB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf TB", (dsize / IPMSG_SIZE_TB) );
		IPMSG_FUNC_RETURN( buf );
	} else if ( dsize >= IPMSG_SIZE_GB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf GB", (dsize / IPMSG_SIZE_GB) );
		IPMSG_FUNC_RETURN( buf );
	} else if ( dsize >= IPMSG_SIZE_MB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf MB", (dsize / IPMSG_SIZE_MB) );
		IPMSG_FUNC_RETURN( buf );
	} else if ( dsize >= IPMSG_SIZE_KB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf KB", (dsize / IPMSG_SIZE_KB) );
		IPMSG_FUNC_RETURN( buf );
	}
	snprintf( buf, sizeof( buf ), "%lld B", size );
	IPMSG_FUNC_RETURN( buf );
}

/**
 * デバイス名を設定し、ハードウェアアドレスを取得する。
 * @param val デバイス名。
 */
void
NetworkInterface::setDeviceName( const std::string val )
{
	IPMSG_FUNC_ENTER("void NetworkInterface::setDeviceName( const std::string val )");
	_DeviceName = val;
	_HardwareAddress = getNetworkInterfaceMacAddress( val );
	IPMSG_FUNC_EXIT;
}

/**
 * IPアドレスを設定し、ネットワークアドレス、ブロードキャストアドレスを再計算する。
 * @param val IPアドレス文字列。
 */
void
NetworkInterface::setIpAddress( const std::string val )
{
	IPMSG_FUNC_ENTER("void NetworkInterface::setIpAddress( const std::string val )");
	_IpAddress = val;
	recalc();
	IPMSG_FUNC_EXIT;
}

/**
 * ネットマスクを設定し、ネットワークアドレス、ブロードキャストアドレスを再計算する。
 * @param val ネットマスク文字列。
 */
void
NetworkInterface::setNetMask( const std::string val )
{
	IPMSG_FUNC_ENTER("void NetworkInterface::setNetMask( const std::string val )");
	_NetMask = val;
	recalc();
	IPMSG_FUNC_EXIT;
}

/**
 * ネットワークアドレス、ブロードキャストアドレスを計算する。
 */
void
NetworkInterface::recalc()
{
	IPMSG_FUNC_ENTER("void NetworkInterface::recalc()");
	_NetworkAddress = getNetworkAddress( _AddressFamily, _IpAddress, _NetMask );
	_BroadcastAddress = getBroadcastAddress( _AddressFamily, _NetworkAddress, _NetMask );
	IPMSG_FUNC_EXIT;
}
//end of source
