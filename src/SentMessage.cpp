/**
 * IP メッセンジャライブラリ(Unix用)
 * 送信メッセージクラス。
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
using namespace std;

//メッセージ送信リトライ最大数
#define SENDMSG_RETRY_MAX	5

/**
 * コンストラクタ。
 */
SentMessage::SentMessage(){}

/**
 * コピーコンストラクタ。
 * @param other コピー元のオブジェクト
 */
SentMessage::SentMessage( const SentMessage& other )
{
	CopyFrom( other );
}

/**
 * 代入演算子。
 * @param other コピー元のオブジェクト
 * @retval 自インスタンス
 */
SentMessage&
SentMessage::operator=( const SentMessage& other )
{
	CopyFrom( other );
	return *this;
}

/**
 * コピーメソッド。
 * @param other コピー元のオブジェクト
 */
void
SentMessage::CopyFrom( const SentMessage& other )
{
	_To = other. _To;
	_Host = other. _Host;
	_PacketNo = other. _PacketNo;
	_Message = other. _Message;
	_Sent = other. _Sent;
	_PrevTry = other. _PrevTry;
	_IsRetryMaxOver = other. _IsRetryMaxOver;
	_IsSent = other. _IsSent;
	_IsPasswordLock = other. _IsPasswordLock;
	_IsCrypted = other. _IsCrypted;
	_RetryCount = other. _RetryCount;
	_IsConfirmed = other. _IsConfirmed;
	_IsConfirmAnswered = other. _IsConfirmAnswered;
	_IsSecret = other. _IsSecret;
	_IsNoLogging = other. _IsNoLogging;
	_HostCountAtSameTime = other. _HostCountAtSameTime;
	_Opt = other. _Opt;
	_Files = other._Files;
}

/**
 * コンストラクタ。
 * ・送信済メッセージリストをロックするためのミューテックスを生成。
 */
SentMessageList::SentMessageList()
{
	IpMsgMutexInit( "SentMessageList::SentMessageList()", &messagesMutex, NULL );
}

/**
 * コピーコンストラクタ。
 * @param other コピー元のオブジェクト
 */
SentMessageList::SentMessageList( const SentMessageList& other )
{
	IpMsgMutexInit( "SentMessageList::SentMessageList(SentMessageList&)", &messagesMutex, NULL );
	Lock( "SentMessageList::SentMessageList(SentMessageList&)" );
	CopyFrom( other );
	Unlock( "SentMessageList::SentMessageList(SentMessageList&)" );
}

/**
 * デストラクタ。
 * ・送信済メッセージリストをロックするためのミューテックスを破棄。
 */
SentMessageList::~SentMessageList()
{
	IpMsgMutexDestroy( "SentMessageList::~SentMessageList()", &messagesMutex );
}

/**
 * 代入演算子。
 * @param other コピー元のオブジェクト
 * @retval 自インスタンス
 */
SentMessageList&
SentMessageList::operator=( const SentMessageList& other )
{
	IpMsgMutexInit( "SentMessageList::operator=(SentMessageList&)", &messagesMutex, NULL );
	Lock( "SentMessageList::operator=(SentMessageList&)" );
	CopyFrom( other );
	Unlock( "SentMessageList::operator=(SentMessageList&)" );
	return *this;
}

/**
 * コピーメソッド。
 * @param other コピー元のオブジェクト
 */
void
SentMessageList::CopyFrom( const SentMessageList& other )
{
	messages = other.messages;
}

/**
 * 送信メッセージリストをロック
 * @param pos ロックしている位置を示す文字列。
 */
void
SentMessageList::Lock( const char *pos ) const
{
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &messagesMutex ) );
}

/**
 * 送信メッセージリストをアンロック
 * @param pos アンロックしている位置を示す文字列。
 */
void
SentMessageList::Unlock( const char *pos ) const
{
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &messagesMutex ) );
}

/**
 * 送信済メッセージリストの先頭を示すイテレータを返す。
 * @retval 送信済メッセージリストの先頭を示すイテレータ。
 */
vector<SentMessage>::iterator
SentMessageList::begin()
{
	return messages.begin();
}

/**
 * 送信済メッセージリストの末尾＋１を示すイテレータを返す。
 * @retval 送信済メッセージリストの末尾＋１を示すイテレータ。
 */
vector<SentMessage>::iterator
SentMessageList::end(){
	return messages.end();
}

/**
 * 指定されたイテレータで送信済メッセージを送信済メッセージリストから削除する。
 * @param 削除対象の送信済メッセージを示すイテレータ。
 * @retval 削除された送信済メッセージの次の要素を示すイテレータ。
 */
vector<SentMessage>::iterator
SentMessageList::erase( vector<SentMessage>::iterator item )
{
	Lock( "SentMessageList::erase()" );
	vector<SentMessage>::iterator ret = messages.erase( item );
	Unlock( "SentMessageList::erase()" );
	return ret;
}

/**
 * 送信済メッセージリストにメッセージを追加する。
 * @param 送信済メッセージ。
 */
void
SentMessageList::append( const SentMessage &item )
{
	Lock( "SentMessageList::append()" );
	messages.push_back( item );
	Unlock( "SentMessageList::append()" );
}

/**
 * 送信済メッセージリストの個数を返す。
 * @retval 送信済メッセージリストの個数。
 */
int
SentMessageList::size() const
{
	Lock( "SentMessageList::size()" );
	int ret = messages.size();
	Unlock( "SentMessageList::size()" );
	return ret;
}

/**
 * 送信済メッセージリストをクリアする。
 */
void
SentMessageList::clear()
{
	Lock( "SentMessageList::clear()" );
	messages.clear();
	Unlock( "SentMessageList::clear()" );
}

/**
 * 送信済メッセージリストのポインタを返す。
 * @retval 送信済メッセージリストのポインタ。
 */
vector<SentMessage> *
SentMessageList::GetMessageList()
{
	return &messages;
}

/**
 * パケットから添付ファイルを検索します。
 * ・パケットからファイルIDを抽出しファイルIDを基に添付ファイルを検索し、AttachFileのイテレータを返します。
 * @param packet パケットオブジェクト
 * @retval AttachFileのイテレータ。見付からない場合、end()を返す。
 */
vector<AttachFile>::iterator
SentMessage::FindAttachFileByPacket( const Packet& packet )
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

/**
 * リトライマックスオーバーかを調べる
 * @retval true:リトライマックスオーバー、false:オーバーしていない。
 */
bool
SentMessage::isRetryMaxOver() const
{
	if ( RetryCount() > SENDMSG_RETRY_MAX ) {
		return true;
	}
	return false;
}

/**
 * リトライが必要かを調べる
 * @param tryNow 現在時刻
 * @retval true:リトライが必要、false:リトライ不要。
 */
bool
SentMessage::needSendRetry( time_t tryNow ) const
{
	if ( !IsSent() && PrevTry() != tryNow && !IsRetryMaxOver() ) {
		return true;
	}
	return false;
}

/**
 * パケットNoで送信済メッセージリストから送信済メッセージのイテレータを取得する。
 * @param PacketNo パケットNo
 * @retval 送信済メッセージのイテレータ。（見付からない場合end()）
 * 注：このメソッドはスレッドセーフでない。
 */
vector<SentMessage>::iterator
SentMessageList::FindSentMessageByPacketNo( unsigned long PacketNo )
{
	Lock( "SentMessageList::FindSentMessageByPacketNo()" );
	vector<SentMessage>::iterator ret = end();
	for( vector<SentMessage>::iterator ixmsg = begin(); ixmsg != end(); ixmsg++ ) {
		if ( PacketNo == ixmsg->PacketNo() ) {
			ret = ixmsg;
			break;
		}
	}
	Unlock( "SentMessageList::FindSentMessageByPacketNo()" );
	return ret;
}

/**
 * パケットから送信済メッセージを検索します。
 * ・パケットからパケットNoを抽出しパケットNoを基に送信済メッセージを検索し、SentMessageのイテレータを返します。
 * @param packet パケットオブジェクト
 * @retval SentMessageのイテレータ。見付からない場合、end()を返す。
 */
vector<SentMessage>::iterator
SentMessageList::FindSentMessageByPacket( Packet packet )
{
	char *dmyptr;
	char *startptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;

	Lock( "SentMessageList::FindSentMessageByPacket()" );
	vector<SentMessage>::iterator ret = end();
	for( vector<SentMessage>::iterator ixmsg = begin(); ixmsg != end(); ixmsg++ ) {
		if ( packetNo == ixmsg->PacketNo() ) {
			ret = ixmsg;
			break;
		}
	}
	Unlock( "SentMessageList::FindSentMessageByPacket()" );
	return ret; 
}

