/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * ������å��������饹��
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
using namespace std;

//��å�����������ȥ饤�����
#define SENDMSG_RETRY_MAX	5

/**
 * ���󥹥ȥ饯����
 * �������ѥ�å������ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������
 */
SentMessageList::SentMessageList()
{
	IpMsgMutexInit( "SentMessageList::SentMessageList()", &messagesMutex, NULL );
}

/**
 * �ǥ��ȥ饯����
 * �������ѥ�å������ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå������˴���
 */
SentMessageList::~SentMessageList()
{
	IpMsgMutexDestroy( "SentMessageList::~SentMessageList()", &messagesMutex );
}

/**
 * �����ѥ�å������ꥹ�Ȥ���Ƭ�򼨤����ƥ졼�����֤���
 * @retval �����ѥ�å������ꥹ�Ȥ���Ƭ�򼨤����ƥ졼����
 */
vector<SentMessage>::iterator
SentMessageList::begin()
{
	return messages.begin();
}

/**
 * �����ѥ�å������ꥹ�Ȥ������ܣ��򼨤����ƥ졼�����֤���
 * @retval �����ѥ�å������ꥹ�Ȥ������ܣ��򼨤����ƥ졼����
 */
vector<SentMessage>::iterator
SentMessageList::end(){
	return messages.end();
}

/**
 * ���ꤵ�줿���ƥ졼���������ѥ�å������������ѥ�å������ꥹ�Ȥ��������롣
 * @param ����оݤ������ѥ�å������򼨤����ƥ졼����
 * @retval ������줿�����ѥ�å������μ������Ǥ򼨤����ƥ졼����
 */
vector<SentMessage>::iterator
SentMessageList::erase( vector<SentMessage>::iterator item )
{
	IpMsgMutexLock( "SentMessageList::erase()", &messagesMutex );
	vector<SentMessage>::iterator ret = messages.erase( item );
	IpMsgMutexUnlock( "SentMessageList::erase()", &messagesMutex );
	return ret;
}

/**
 * �����ѥ�å������ꥹ�Ȥ˥�å��������ɲä��롣
 * @param �����ѥ�å�������
 */
void
SentMessageList::append( const SentMessage &item )
{
	IpMsgMutexLock( "SentMessageList::append()", &messagesMutex );
	messages.push_back( item );
	IpMsgMutexUnlock( "SentMessageList::append()", &messagesMutex );
}

/**
 * �����ѥ�å������ꥹ�ȤθĿ����֤���
 * @retval �����ѥ�å������ꥹ�ȤθĿ���
 */
int
SentMessageList::size()
{
	IpMsgMutexLock( "SentMessageList::size()", &messagesMutex );
	int ret = messages.size();
	IpMsgMutexUnlock( "SentMessageList::size()", &messagesMutex );
	return ret;
}

/**
 * �����ѥ�å������ꥹ�Ȥ򥯥ꥢ���롣
 */
void
SentMessageList::clear()
{
	IpMsgMutexLock( "SentMessageList::clear()", &messagesMutex );
	messages.clear();
	IpMsgMutexUnlock( "SentMessageList::clear()", &messagesMutex );
}

/**
 * �����ѥ�å������ꥹ�ȤΥݥ��󥿤��֤���
 * @retval �����ѥ�å������ꥹ�ȤΥݥ��󥿡�
 */
vector<SentMessage> *
SentMessageList::GetMessageList()
{
	return &messages;
}

/**
 * �ѥ��åȤ���ź�եե�����򸡺����ޤ���
 * ���ѥ��åȤ���ե�����ID����Ф��ե�����ID����ź�եե�����򸡺�����AttachFile�Υ��ƥ졼�����֤��ޤ���
 * @param packet �ѥ��åȥ��֥�������
 * @retval AttachFile�Υ��ƥ졼�������դ���ʤ���硢end()���֤���
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
 * ��ȥ饤�ޥå��������С�����Ĵ�٤�
 * @retval true:��ȥ饤�ޥå��������С���false:�����С����Ƥ��ʤ���
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
 * ��ȥ饤��ɬ�פ���Ĵ�٤�
 * @param tryNow ���߻���
 * @retval true:��ȥ饤��ɬ�ס�false:��ȥ饤���ס�
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

