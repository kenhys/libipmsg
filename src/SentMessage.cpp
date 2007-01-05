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
 * ・送信済メッセージリストをロックするためのミューテックスを生成。
 */
SentMessageList::SentMessageList()
{
#ifdef HAVE_PTHREAD
	IpMsgMutexInit( "SentMessageList::SentMessageList()", &messagesMutex, NULL );
#endif
}

/**
 * デストラクタ。
 * ・送信済メッセージリストをロックするためのミューテックスを破棄。
 */
SentMessageList::~SentMessageList()
{
#ifdef HAVE_PTHREAD
	IpMsgMutexDestroy( "SentMessageList::~SentMessageList()", &messagesMutex );
#endif
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
#ifdef HAVE_PTHREAD
	IpMsgMutexLock( "SentMessageList::erase()", &messagesMutex );
#endif
	vector<SentMessage>::iterator ret = messages.erase( item );
#ifdef HAVE_PTHREAD
	IpMsgMutexUnlock( "SentMessageList::erase()", &messagesMutex );
#endif
	return ret;
}

/**
 * 送信済メッセージリストにメッセージを追加する。
 * @param 送信済メッセージ。
 */
void
SentMessageList::append( const SentMessage &item )
{
#ifdef HAVE_PTHREAD
	IpMsgMutexLock( "SentMessageList::append()", &messagesMutex );
#endif
	messages.push_back( item );
#ifdef HAVE_PTHREAD
	IpMsgMutexUnlock( "SentMessageList::append()", &messagesMutex );
#endif
}

/**
 * 送信済メッセージリストの個数を返す。
 * @retval 送信済メッセージリストの個数。
 */
int
SentMessageList::size()
{
#ifdef HAVE_PTHREAD
	IpMsgMutexLock( "SentMessageList::size()", &messagesMutex );
#endif
	int ret = messages.size();
#ifdef HAVE_PTHREAD
	IpMsgMutexUnlock( "SentMessageList::size()", &messagesMutex );
#endif
	return ret;
}

/**
 * 送信済メッセージリストをクリアする。
 */
void
SentMessageList::clear()
{
#ifdef HAVE_PTHREAD
	IpMsgMutexLock( "SentMessageList::clear()", &messagesMutex );
#endif
	messages.clear();
#ifdef HAVE_PTHREAD
	IpMsgMutexUnlock( "SentMessageList::clear()", &messagesMutex );
#endif
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
SentMessage::isRetryMaxOver()
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
SentMessage::needSendRetry( time_t tryNow )
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
	for( vector<SentMessage>::iterator ixmsg = begin(); ixmsg != end(); ixmsg++ ) {
		if ( PacketNo == ixmsg->PacketNo() ) {
			return ixmsg;
		}
	}
	return end();
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

	vector<AttachFile>::iterator FoundFile;
	for( vector<SentMessage>::iterator ixmsg = begin(); ixmsg != end(); ixmsg++ ) {
		if ( packetNo == ixmsg->PacketNo() ) {
			return ixmsg; 
		}
	}
	return end(); 
}

