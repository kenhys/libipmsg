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
 */
SentMessage::SentMessage(){}

/**
 * ���ԡ����󥹥ȥ饯����
 * @param other ���ԡ����Υ��֥�������
 */
SentMessage::SentMessage( const SentMessage& other )
{
	CopyFrom( other );
}

/**
 * �����黻�ҡ�
 * @param other ���ԡ����Υ��֥�������
 * @retval �����󥹥���
 */
SentMessage&
SentMessage::operator=( const SentMessage& other )
{
	CopyFrom( other );
	return *this;
}

/**
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
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
 * ���󥹥ȥ饯����
 * �������ѥ�å������ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������
 */
SentMessageList::SentMessageList()
{
	IpMsgMutexInit( "SentMessageList::SentMessageList()", &messagesMutex, NULL );
}

/**
 * ���ԡ����󥹥ȥ饯����
 * @param other ���ԡ����Υ��֥�������
 */
SentMessageList::SentMessageList( const SentMessageList& other )
{
	IpMsgMutexInit( "SentMessageList::SentMessageList(SentMessageList&)", &messagesMutex, NULL );
	Lock( "SentMessageList::SentMessageList(SentMessageList&)" );
	CopyFrom( other );
	Unlock( "SentMessageList::SentMessageList(SentMessageList&)" );
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
 * �����黻�ҡ�
 * @param other ���ԡ����Υ��֥�������
 * @retval �����󥹥���
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
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
 */
void
SentMessageList::CopyFrom( const SentMessageList& other )
{
	messages = other.messages;
}

/**
 * ������å������ꥹ�Ȥ��å�
 * @param pos ��å����Ƥ�����֤򼨤�ʸ����
 */
void
SentMessageList::Lock( const char *pos ) const
{
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &messagesMutex ) );
}

/**
 * ������å������ꥹ�Ȥ򥢥��å�
 * @param pos �����å����Ƥ�����֤򼨤�ʸ����
 */
void
SentMessageList::Unlock( const char *pos ) const
{
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &messagesMutex ) );
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
	Lock( "SentMessageList::erase()" );
	vector<SentMessage>::iterator ret = messages.erase( item );
	Unlock( "SentMessageList::erase()" );
	return ret;
}

/**
 * �����ѥ�å������ꥹ�Ȥ˥�å��������ɲä��롣
 * @param �����ѥ�å�������
 */
void
SentMessageList::append( const SentMessage &item )
{
	Lock( "SentMessageList::append()" );
	messages.push_back( item );
	Unlock( "SentMessageList::append()" );
}

/**
 * �����ѥ�å������ꥹ�ȤθĿ����֤���
 * @retval �����ѥ�å������ꥹ�ȤθĿ���
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
 * �����ѥ�å������ꥹ�Ȥ򥯥ꥢ���롣
 */
void
SentMessageList::clear()
{
	Lock( "SentMessageList::clear()" );
	messages.clear();
	Unlock( "SentMessageList::clear()" );
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
SentMessage::isRetryMaxOver() const
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
SentMessage::needSendRetry( time_t tryNow ) const
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

