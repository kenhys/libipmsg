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

using namespace ipmsg;

//��å�����������ȥ饤�����
#define SENDMSG_RETRY_MAX	5

/**
 * ���󥹥ȥ饯����
 */
SentMessage::SentMessage()
{
	IPMSG_FUNC_ENTER( "SentMessage::SentMessage()" );
	IPMSG_FUNC_EXIT;
}

/**
 * ���ԡ����󥹥ȥ饯����
 * @param other ���ԡ����Υ��֥�������
 */
SentMessage::SentMessage( const SentMessage& other )
{
	IPMSG_FUNC_ENTER( "SentMessage::SentMessage( const SentMessage& other )" );
	CopyFrom( other );
	IPMSG_FUNC_EXIT;
}

/**
 * �����黻�ҡ�
 * @param other ���ԡ����Υ��֥�������
 * @retval �����󥹥���
 */
SentMessage&
SentMessage::operator=( const SentMessage& other )
{
	IPMSG_FUNC_ENTER( "SentMessage& SentMessage::operator=( const SentMessage& other )" );
	CopyFrom( other );
	IPMSG_FUNC_RETURN( *this );
}

/**
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
 */
void
SentMessage::CopyFrom( const SentMessage& other )
{
	IPMSG_FUNC_ENTER( "void SentMessage::CopyFrom( const SentMessage& other )" );
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
	IPMSG_FUNC_EXIT;
}

/**
 * ���󥹥ȥ饯����
 * <ul>
 * <li>�����ѥ�å������ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 */
SentMessageList::SentMessageList()
{
	IPMSG_FUNC_ENTER( "SentMessageList::SentMessageList()" );
	IpMsgMutexInit( "SentMessageList::SentMessageList()", &messagesMutex, NULL );
	IPMSG_FUNC_EXIT;
}

/**
 * ���ԡ����󥹥ȥ饯����
 * @param other ���ԡ����Υ��֥�������
 */
SentMessageList::SentMessageList( const SentMessageList& other )
{
	IPMSG_FUNC_ENTER( "SentMessageList::SentMessageList( const SentMessageList& other )" );
	IpMsgMutexInit( "SentMessageList::SentMessageList(SentMessageList&)", &messagesMutex, NULL );
	Lock( "SentMessageList::SentMessageList(SentMessageList&)" );
	CopyFrom( other );
	Unlock( "SentMessageList::SentMessageList(SentMessageList&)" );
	IPMSG_FUNC_EXIT;
}

/**
 * �ǥ��ȥ饯����
 * <ul>
 * <li>�����ѥ�å������ꥹ�Ȥ��å����뤿��Υߥ塼�ƥå������˴���</li>
 * </ul>
 */
SentMessageList::~SentMessageList()
{
	IPMSG_FUNC_ENTER( "SentMessageList::~SentMessageList()" );
	IpMsgMutexDestroy( "SentMessageList::~SentMessageList()", &messagesMutex );
	IPMSG_FUNC_EXIT;
}

/**
 * �����黻�ҡ�
 * @param other ���ԡ����Υ��֥�������
 * @retval �����󥹥���
 */
SentMessageList&
SentMessageList::operator=( const SentMessageList& other )
{
	IPMSG_FUNC_ENTER( "SentMessageList& SentMessageList::operator=( const SentMessageList& other )" );
	IpMsgMutexInit( "SentMessageList::operator=(SentMessageList&)", &messagesMutex, NULL );
	Lock( "SentMessageList::operator=(SentMessageList&)" );
	CopyFrom( other );
	Unlock( "SentMessageList::operator=(SentMessageList&)" );
	IPMSG_FUNC_RETURN( *this );
}

/**
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
 */
void
SentMessageList::CopyFrom( const SentMessageList& other )
{
	IPMSG_FUNC_ENTER( "void SentMessageList::CopyFrom( const SentMessageList& other )" );
	messages = other.messages;
	IPMSG_FUNC_EXIT;
}

/**
 * ������å������ꥹ�Ȥ��å�
 * @param pos ��å����Ƥ�����֤򼨤�ʸ����
 */
void
SentMessageList::Lock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void SentMessageList::Lock( const char *pos ) const" );
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &messagesMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * ������å������ꥹ�Ȥ򥢥��å�
 * @param pos �����å����Ƥ�����֤򼨤�ʸ����
 */
void
SentMessageList::Unlock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void SentMessageList::Unlock( const char *pos ) const" );
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &messagesMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ѥ�å������ꥹ�Ȥ���Ƭ�򼨤����ƥ졼�����֤���
 * @retval �����ѥ�å������ꥹ�Ȥ���Ƭ�򼨤����ƥ졼����
 */
std::vector<SentMessage>::iterator
SentMessageList::begin()
{
	IPMSG_FUNC_ENTER( "std::vector<SentMessage>::iterator SentMessageList::begin()" );
	IPMSG_FUNC_RETURN( messages.begin() );
}

/**
 * �����ѥ�å������ꥹ�Ȥ������ܣ��򼨤����ƥ졼�����֤���
 * @retval �����ѥ�å������ꥹ�Ȥ������ܣ��򼨤����ƥ졼����
 */
std::vector<SentMessage>::iterator
SentMessageList::end()
{
	IPMSG_FUNC_ENTER( "std::vector<SentMessage>::iterator SentMessageList::end()" );
	IPMSG_FUNC_RETURN( messages.end() );
}

/**
 * ���ꤵ�줿���ƥ졼���������ѥ�å������������ѥ�å������ꥹ�Ȥ��������롣
 * @param item ����оݤ������ѥ�å������򼨤����ƥ졼����
 * @retval ������줿�����ѥ�å������μ������Ǥ򼨤����ƥ졼����
 */
std::vector<SentMessage>::iterator
SentMessageList::erase( std::vector<SentMessage>::iterator item )
{
	IPMSG_FUNC_ENTER( "std::vector<SentMessage>::iterator SentMessageList::erase( std::vector<SentMessage>::iterator item )" );
	Lock( "SentMessageList::erase()" );
	std::vector<SentMessage>::iterator ret = messages.erase( item );
	Unlock( "SentMessageList::erase()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �����ѥ�å������ꥹ�Ȥ˥�å��������ɲä��롣
 * @param item �����ѥ�å�������
 */
void
SentMessageList::append( const SentMessage &item )
{
	IPMSG_FUNC_ENTER( "void SentMessageList::append( const SentMessage &item )" );
	Lock( "SentMessageList::append()" );
	messages.push_back( item );
	Unlock( "SentMessageList::append()" );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ѥ�å������ꥹ�ȤθĿ����֤���
 * @retval �����ѥ�å������ꥹ�ȤθĿ���
 */
int
SentMessageList::size() const
{
	IPMSG_FUNC_ENTER( "int SentMessageList::size() const" );
	Lock( "SentMessageList::size()" );
	int ret = messages.size();
	Unlock( "SentMessageList::size()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �����ѥ�å������ꥹ�Ȥ򥯥ꥢ���롣
 */
void
SentMessageList::clear()
{
	IPMSG_FUNC_ENTER( "void SentMessageList::clear()" );
	Lock( "SentMessageList::clear()" );
	messages.clear();
	Unlock( "SentMessageList::clear()" );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ѥ�å������ꥹ�ȤΥݥ��󥿤��֤���
 * @retval �����ѥ�å������ꥹ�ȤΥݥ��󥿡�
 */
std::vector<SentMessage> *
SentMessageList::GetMessageList()
{
	IPMSG_FUNC_ENTER( "std::vector<SentMessage> *SentMessageList::GetMessageList()" );
	IPMSG_FUNC_RETURN( &messages );
}

/**
 * �ѥ��åȤ���ź�եե�����򸡺����ޤ���
 * <ul>
 * <li>�ѥ��åȤ���ե�����ID����Ф��ե�����ID����ź�եե�����򸡺�����AttachFile�Υ��ƥ졼�����֤��ޤ���</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 * @retval AttachFile�Υ��ƥ졼��
 * @retval ���դ���ʤ���硢end()
 */
std::vector<AttachFile>::iterator
SentMessage::FindAttachFileByPacket( const Packet& packet )
{
	IPMSG_FUNC_ENTER( "std::vector<AttachFile>::iterator SentMessage::FindAttachFileByPacket( const Packet& packet )" );
	char *dmyptr;
	char *startptr;
	strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;
	int packet_file_id = strtoul( startptr, &dmyptr, 16 );
	startptr = ++dmyptr;

	std::vector<AttachFile>::iterator FoundFile;
	FoundFile = Files().FindByFileId( packet_file_id );
	IPMSG_FUNC_RETURN( FoundFile );
}

/**
 * ��ȥ饤�ޥå��������С�����Ĵ�٤�
 * @retval true:��ȥ饤�ޥå��������С�
 * @retval false:�����С����Ƥ��ʤ�
 */
bool
SentMessage::isRetryMaxOver() const
{
	IPMSG_FUNC_ENTER( "bool SentMessage::isRetryMaxOver() const" );
	if ( RetryCount() > SENDMSG_RETRY_MAX ) {
		IPMSG_FUNC_RETURN( true );
	}
	IPMSG_FUNC_RETURN( false );
}

/**
 * ��ȥ饤��ɬ�פ���Ĵ�٤�
 * @param tryNow ���߻���
 * @retval true:��ȥ饤��ɬ��
 * @retval false:��ȥ饤����
 */
bool
SentMessage::needSendRetry( time_t tryNow ) const
{
	IPMSG_FUNC_ENTER( "bool SentMessage::needSendRetry( time_t tryNow ) const" );
	if ( !IsSent() && PrevTry() != tryNow && !IsRetryMaxOver() ) {
		IPMSG_FUNC_RETURN( true );
	}
	IPMSG_FUNC_RETURN( false );
}

/**
 * �ѥ��å�No�������ѥ�å������ꥹ�Ȥ��������ѥ�å������Υ��ƥ졼����������롣
 * @param PacketNo �ѥ��å�No
 * @retval �����ѥ�å������Υ��ƥ졼��
 * @retval ���դ���ʤ����end()
 */
std::vector<SentMessage>::iterator
SentMessageList::FindSentMessageByPacketNo( unsigned long PacketNo )
{
	IPMSG_FUNC_ENTER( "std::vector<SentMessage>::iterator SentMessageList::FindSentMessageByPacketNo( unsigned long PacketNo )" );
	Lock( "SentMessageList::FindSentMessageByPacketNo()" );
	std::vector<SentMessage>::iterator ret = end();
	for( std::vector<SentMessage>::iterator ixmsg = begin(); ixmsg != end(); ixmsg++ ) {
		if ( PacketNo == ixmsg->PacketNo() ) {
			ret = ixmsg;
			break;
		}
	}
	Unlock( "SentMessageList::FindSentMessageByPacketNo()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ѥ��åȤ��������ѥ�å������򸡺����ޤ���
 * <ul>
 * <li>�ѥ��åȤ���ѥ��å�No����Ф��ѥ��å�No���������ѥ�å������򸡺�����SentMessage�Υ��ƥ졼�����֤��ޤ���</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 * @retval SentMessage�Υ��ƥ졼��
 * @retval ���դ���ʤ���硢end()
 */
std::vector<SentMessage>::iterator
SentMessageList::FindSentMessageByPacket( Packet packet )
{
	IPMSG_FUNC_ENTER( "std::vector<SentMessage>::iterator SentMessageList::FindSentMessageByPacket( Packet packet )" );
	char *dmyptr;
	char *startptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;

	Lock( "SentMessageList::FindSentMessageByPacket()" );
	std::vector<SentMessage>::iterator ret = end();
	for( std::vector<SentMessage>::iterator ixmsg = begin(); ixmsg != end(); ixmsg++ ) {
		if ( packetNo == ixmsg->PacketNo() ) {
			ret = ixmsg;
			break;
		}
	}
	Unlock( "SentMessageList::FindSentMessageByPacket()" );
	IPMSG_FUNC_RETURN( ret );
}

