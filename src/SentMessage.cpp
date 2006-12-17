/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * ������å��������饹��
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
using namespace std;

/**
 * �ѥ��åȤ���ź�եե�����򸡺����ޤ���
 * ���ѥ��åȤ���ե�����ID����Ф��ե�����ID����ź�եե�����򸡺�����AttachFile�Υ��ƥ졼�����֤��ޤ���
 * @param packet �ѥ��åȥ��֥�������
 * @retval AttachFile�Υ��ƥ졼�������դ���ʤ���硢end()���֤���
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
 * �ѥ��å�No�������ѥ�å������ꥹ�Ȥ��������ѥ�å������Υ��ƥ졼����������롣
 * @param PacketNo �ѥ��å�No
 * @retval �����ѥ�å������Υ��ƥ졼�����ʸ��դ���ʤ����end()��
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
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
 * �ѥ��åȤ��������ѥ�å������򸡺����ޤ���
 * ���ѥ��åȤ���ѥ��å�No����Ф��ѥ��å�No���������ѥ�å������򸡺�����SentMessage�Υ��ƥ졼�����֤��ޤ���
 * @param packet �ѥ��åȥ��֥�������
 * @retval SentMessage�Υ��ƥ졼�������դ���ʤ���硢end()���֤���
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

