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
