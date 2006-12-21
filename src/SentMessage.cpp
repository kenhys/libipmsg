/**
 * IP メッセンジャライブラリ(Unix用)
 * 送信メッセージクラス。
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
using namespace std;

//メッセージ送信リトライ最大数
#define SENDMSG_RETRY_MAX	5

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

