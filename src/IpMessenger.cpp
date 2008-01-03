/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * IP��å��󥸥㥨��������ȥ��饹��
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
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥��󥹤�������롣
 * <ul>
 * <li>Singleton�ѥ��������Ѥ��Ƥ���Τǡ��ۥ���ͣ��Υ��󥹥��󥹤Ǥʤ���Фʤ�ʤ���</li>
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
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥��󥹤�������롣
 * <ul>
 * <li>���Υ᥽�åɤ�Ȥäƥ��֥������Ȥ�������ʤ���Фʤ�ʤ���</li>
 * <li>�饤�֥����̤��ʤ���ľ��delete���줿���Ϥ��θ��ư��ˤĤ��ƴ��Τ��ʤ���</li>
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
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥ȥ饯����
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
 * IP ��å��󥸥㥨��������ȥ��饹�Υǥ��ȥ饯����
 */
IpMessengerAgent::~IpMessengerAgent()
{
	IPMSG_FUNC_ENTER("IpMessengerAgent::~IpMessengerAgent()");
	IpMessengerAgentImpl::Release();
	IPMSG_FUNC_EXIT;
}

/**
 * NIC����ꤻ����IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����ư���롣
 * <ul>
 * <li>���Ƥ�NIC���Ф��ƥǥե���ȥݡ��Ȥǥͥåȥ����ư���롣</li>
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
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����ư���롣
 * @parem nics ��ư�����оݤȤ���NIC�Υ٥�����
 */
void
IpMessengerAgent::StartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::StartNetwork( const std::vector<NetworkInterface>& nics )");
	ipmsgImpl->StartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ������ߤ��롣
 */
void
IpMessengerAgent::StopNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::StopNetwork()");
	ipmsgImpl->StopNetwork();
	IPMSG_FUNC_EXIT;
}

/**
 * NIC����ꤻ����IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����Ƶ�ư���롣
 * <ul>
 * <li>���Ƥ�NIC���Ф��ƥǥե���ȥݡ��Ȥǥͥåȥ����ư���롣</li>
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
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����Ƶ�ư���롣
 * @parem nics ��ư�����оݤȤ���NIC�Υ٥�����
 */
void
IpMessengerAgent::RestartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::RestartNetwork( const std::vector<NetworkInterface>& nics )");
	ipmsgImpl->RestartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * �ե�����̾����С����Υ��å�����
 * @retval ����С����Υ��ɥ쥹��
 */
FileNameConverter *
IpMessengerAgent::GetFileNameConverter() const
{
	IPMSG_FUNC_ENTER("FileNameConverter *IpMessengerAgent::GetFileNameConverter() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetFileNameConverter() );
}

/**
 * �ե�����̾����С����Υ��å�����
 * @param conv ����С����Υ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetFileNameConverter( const FileNameConverter *conv )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetFileNameConverter( const FileNameConverter *conv )");
	ipmsgImpl->SetFileNameConverter( conv );
	IPMSG_FUNC_EXIT;
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
HostListComparator *
IpMessengerAgent::GetSortHostListComparator() const
{
	IPMSG_FUNC_ENTER("HostListComparator *IpMessengerAgent::GetSortHostListComparator() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetSortHostListComparator() );
}; 

/**
 * �ۥ��ȥꥹ����ӥ��֥������ȤΥ��å�����
 * @param comparator �ۥ��ȥꥹ����ӥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetSortHostListComparator( const HostListComparator *comparator )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetSortHostListComparator( const HostListComparator *comparator )");
	ipmsgImpl->SetSortHostListComparator( comparator );
	IPMSG_FUNC_EXIT;
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
IpMessengerEvent *
IpMessengerAgent::GetEventObject() const
{
	IPMSG_FUNC_ENTER("IpMessengerEvent *IpMessengerAgent::GetEventObject() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetEventObject() );
}; 

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @param evt ���٥�ȥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetEventObject( const IpMessengerEvent *evt )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetEventObject( const IpMessengerEvent *evt )");
	ipmsgImpl->SetEventObject( evt );
	IPMSG_FUNC_EXIT;
}

/**
 * NIC�ξ����������롣
 * @param nics �ͥåȥ�����󥿡��ե������ΰ���
 */
void
IpMessengerAgent::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6 )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6 )");
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( nics, useIPv6 );
	IPMSG_FUNC_EXIT;
}

/**
 * ������ʥ����ӥ��������Ρˡ�
 */
void
IpMessengerAgent::Login( std::string nickname, std::string groupName )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::Login( std::string nickname, std::string groupName )");
	ipmsgImpl->Login( nickname, groupName );
	IPMSG_FUNC_EXIT;
}

/**
 * �������ȡʥ����ӥ�æ�����Ρˡ�
 */
void
IpMessengerAgent::Logout()
{	
	IPMSG_FUNC_ENTER("void IpMessengerAgent::Logout()");
	ipmsgImpl->Logout();
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ�ȼ�����
 * @retval ����������Ȥ��ݻ����Ƥ���HostList���֥�������
 */
HostList&
IpMessengerAgent::GetHostList()
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgent::GetHostList()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetHostList() );
}

/**
 * �ۥ��ȥꥹ�ȹ���������
 * @retval ��������HostList���֥�������
 */
HostList&
IpMessengerAgent::UpdateHostList()
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgent::UpdateHostList()");
	IPMSG_FUNC_RETURN( ipmsgImpl->UpdateHostList() );
}

/**
 * �Ժߥ⡼�ɤ��ɤ�����Ƚ�ꡣ
 * @retval ����Ѥ��Ժߥ⡼�ɤ��֤���
 */
bool
IpMessengerAgent::IsAbsence() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::IsAbsence() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->IsAbsence() );
}
/**
 * �Ժߥ⡼�ɤ򥯥ꥢ���롣
 */
void
IpMessengerAgent::ResetAbsence()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ResetAbsence()");
	ipmsgImpl->ResetAbsence();
	IPMSG_FUNC_EXIT;
}

/**
 * �Ժߥ⡼�ɤ����ꤹ�롣
 * @param encoding �����륨�󥳡��ǥ���
 * @param absenceModes AbsenceMode���֥������ȤΥ٥����ʼ�ư��������ʣ�����󥳡��ǥ����б����뤿���
 */
void
IpMessengerAgent::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )");
	ipmsgImpl->SetAbsence( encoding, absenceModes );
	IPMSG_FUNC_EXIT;
}

/**
 * ��å�����������
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param IsNoLogging ���˻Ĥ��ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
bool
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )");
	IPMSG_FUNC_RETURN( ipmsgImpl->SendMsg( host, msg, isSecret, isLockPassword, hostCountAtSameTime, IsLogging, opt ) );
}

/**
 * ��å�����������
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param file ź�եե�����
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param IsNoLogging ���˻Ĥ��ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
bool
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )");
	IPMSG_FUNC_RETURN( ipmsgImpl->SendMsg( host, msg, isSecret, file, isLockPassword, hostCountAtSameTime, IsLogging, opt ) );
}

/**
 * ��å�����������
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param files ź�եե����뷲
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param IsNoLogging ���˻Ĥ��ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
bool
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &IsLogging, unsigned long opt )");
	IPMSG_FUNC_RETURN( ipmsgImpl->SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, IsLogging, opt ) );
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹�����ƥ��ꥢ
 */
void
IpMessengerAgent::ClearBroadcastAddress()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ClearBroadcastAddress()");
	ipmsgImpl->ClearBroadcastAddress();
	IPMSG_FUNC_EXIT;
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹����
 * @param addr ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgent::DeleteBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteBroadcastAddress( std::string addr )");
	ipmsgImpl->DeleteBroadcastAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * �֥��ɥ��㥹�ȥ��ɥ쥹����Ͽ
 * @param addr ��Ͽ����֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgent::AddBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::AddBroadcastAddress( std::string addr )");
	ipmsgImpl->AddBroadcastAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * ����Υۥ��Ȥ��鱣����硢�����ʤ�����ۥ��ȤΥꥹ�Ȥ򥯥ꥢ
 */
void
IpMessengerAgent::ClearSkulkHost()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ClearSkulkHost()");
	ipmsgImpl->ClearSkulkHost();
	IPMSG_FUNC_EXIT;
}

/**
 * ����Υۥ��Ȥ��鱣����硢�����ʤ�����ۥ��ȤΥꥹ�Ȥ��饢�ɥ쥹����
 * @param host ��Ͽ�Ѥθ����ʤ�����ʱ����˥ۥ��ȥ��ɥ쥹
 */
void
IpMessengerAgent::DeleteSkulkHostAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteSkulkHostAddress( std::string addr )");
	ipmsgImpl->DeleteSkulkHostAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * ����Υۥ��Ȥ��鱣����硢�����ʤ�����ۥ��ȤΥꥹ�Ȥ���ۥ��Ȥ���
 * @param host ��Ͽ�Ѥθ����ʤ�����ʱ����˥ۥ���
 */
void
IpMessengerAgent::DeleteSkulkHost( HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteSkulkHost( HostListItem &host )");
	ipmsgImpl->DeleteSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * ����Υۥ��Ȥ��鱣����硢�����ʤ�����ۥ��ȤΥꥹ�Ȥ˥ۥ��ȥ��ɥ쥹���ɲ�
 * @param host ��Ͽ���븫���ʤ�����ʱ����˥ۥ��ȥ��ɥ쥹
 */
void
IpMessengerAgent::AddSkulkHostAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::AddSkulkHostAddress( std::string )");
	ipmsgImpl->AddSkulkHostAddress( addr );
	IPMSG_FUNC_EXIT;
}

/**
 * ����Υۥ��Ȥ��鱣����硢�����ʤ�����ۥ��ȤΥꥹ�Ȥ˥ۥ��Ȥ��ɲ�
 * @param host ��Ͽ���븫���ʤ�����ʱ����˥ۥ���
 */
void
IpMessengerAgent::AddSkulkHost( HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::AddSkulkHost( HostListItem &host )");
	ipmsgImpl->AddSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * �оݥۥ��ȤΥС���������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��ȤΥС���������
 */
std::string
IpMessengerAgent::GetInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::GetInfo( HostListItem& host )");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetInfo( host ) );
}

/**
 * �оݥۥ��Ȥ��Ժ�����ʸ�������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��Ȥ��Ժ�����ʸ�������
 */
std::string
IpMessengerAgent::GetAbsenceInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::GetAbsenceInfo( HostListItem& host )");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetAbsenceInfo( host ) );
}

/**
 * �ݻ���Υۥ��ȥꥹ�Ȥ��饰�롼�ץꥹ�Ȥ�������롣
 * @retval ���롼�ץꥹ��
 */
std::vector<GroupItem>
IpMessengerAgent::GetGroupList()
{
	IPMSG_FUNC_ENTER("std::vector<GroupItem> IpMessengerAgent::GetGroupList()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetGroupList() );
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgent::DeleteNotify( RecievedMessage msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::DeleteNotify( RecievedMessage msg )");
	ipmsgImpl->DeleteNotify( msg );
	IPMSG_FUNC_EXIT;
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )");
	ipmsgImpl->ConfirmMessage( msg );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ѥ�å������ꥹ�Ȥ˳������줿���Ȥ�ޡ������롣
 * @param msg ������å��������֥������ȡ�
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
 * ���������ʥ桼�������ˡ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
int
IpMessengerAgent::Process()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgent::Process()");
	IPMSG_FUNC_RETURN( ipmsgImpl->Process() );
}

/**
 * ������å������θĿ���������롣
 * @retval ������å������θĿ�
 */
int
IpMessengerAgent::GetRecievedMessageCount()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgent::GetRecievedMessageCount()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetRecievedMessageCount() );
}

/**
 * ������å��������ļ��Ф���������å������ꥹ�Ȥ��������롣
 * @retval ������å��������֥������ȡ�
 */
RecievedMessage
IpMessengerAgent::PopRecievedMessage()
{
	IPMSG_FUNC_ENTER("RecievedMessage IpMessengerAgent::PopRecievedMessage()");
	IPMSG_FUNC_RETURN( ipmsgImpl->PopRecievedMessage() );
}

/**
 * �����ѥ�å������ꥹ�ȤΥݥ��󥿤�������롣
 * @retval �����ѥ�å������ꥹ�ȤΥݥ��󥿡�
 */
SentMessageList *
IpMessengerAgent::GetSentMessages()
{
	IPMSG_FUNC_ENTER("SentMessageList *IpMessengerAgent::GetSentMessages()");
	IPMSG_FUNC_RETURN( ipmsgImpl->GetSentMessages() );
}

/**
 * �����ѥ�å������ꥹ�ȤΥ��ԡ���������롣
 * @retval �����ѥ�å������ꥹ�ȤΥ��ԡ���
 */
SentMessageList
IpMessengerAgent::CloneSentMessages() const
{
	IPMSG_FUNC_ENTER("SentMessageList IpMessengerAgent::CloneSentMessages() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->CloneSentMessages() );
}

/**
 * ������̾�Υ��å���
 * @retval ������̾
 */
std::string
IpMessengerAgent::LoginName() const
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::LoginName() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->LoginName() );
}

/**
 * �ۥ���̾�Υ��å���
 * @retval �ۥ���̾
 */
std::string
IpMessengerAgent::HostName() const
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgent::HostName() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->HostName() );
}

/**
 * �ǥե���ȥݡ��ȤΥ��å���
 * @retval �ǥե���ȥݡ���
 */
int
IpMessengerAgent::DefaultPortNo() const
{
	IPMSG_FUNC_ENTER("int IpMessengerAgent::DefaultPortNo() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->DefaultPortNo() );
}

/**
 * �ǥե���ȥݡ��ȤΥ��å���
 * @param defaultPortNo �ǥե���ȥݡ���
 */
void
IpMessengerAgent::setDefaultPortNo( const int defaultPortNo )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setDefaultPortNo( const int defaultPortNo )");
	ipmsgImpl->setDefaultPortNo( defaultPortNo );
	IPMSG_FUNC_EXIT;
}

/**
 * ������륢�åפΥ��å���
 * @retval ������륢�å�
 */
bool
IpMessengerAgent::IsDialup() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::IsDialup() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->IsDialup() );
}

/**
 * ������륢�åפΥ��å���
 * @param isDialup ������륢�å�
 */
void
IpMessengerAgent::setIsDialup( const bool isDialup )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setIsDialup( const bool isDialup )");
	ipmsgImpl->setIsDialup( isDialup );
	IPMSG_FUNC_EXIT;
}

/**
 * ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰�Υ��å���
 * @retval ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::AbortDownloadAtFileChanged() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::AbortDownloadAtFileChanged() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->AbortDownloadAtFileChanged() );
}

/**
 * ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰�Υ��å���
 * @param isAbort ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setAbortDownloadAtFileChanged( const bool isAbort )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setAbortDownloadAtFileChanged( const bool isAbort )");
	ipmsgImpl->setAbortDownloadAtFileChanged( isAbort );
	IPMSG_FUNC_EXIT;
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @retval ������å���������¸���뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::SaveSentMessage() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SaveSentMessage() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->SaveSentMessage() );
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @param isSave ������å���������¸���뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setSaveSentMessage( const bool isSave )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setSaveSentMessage( const bool isSave )");
	ipmsgImpl->setSaveSentMessage( isSave );
	IPMSG_FUNC_EXIT;
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @retval ������å���������¸���뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::SaveRecievedMessage() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::SaveRecievedMessage() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->SaveRecievedMessage() );
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @param isSave ������å���������¸���뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setSaveRecievedMessage( const bool isSave )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setSaveRecievedMessage( const bool isSave )");
	ipmsgImpl->setSaveRecievedMessage( isSave );
	IPMSG_FUNC_EXIT;
}

/**
 * �Ź沽�˼��Ԥ������å��������������ʤ����ɤ����Υե饰�Υ��å���
 * @retval �Ź沽�˼��Ԥ������å��������������ʤ����ɤ����Υե饰
 */
bool
IpMessengerAgent::NoSendMessageOnEncryptionFailed() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::NoSendMessageOnEncryptionFailed() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->NoSendMessageOnEncryptionFailed() );
}

/**
 * �Ź沽�˼��Ԥ������å��������������ʤ����ɤ����Υե饰�Υ��å���
 * @param isNoSend �Ź沽�˼��Ԥ������å��������������ʤ����ɤ����Υե饰
 */
void
IpMessengerAgent::setNoSendMessageOnEncryptionFailed( const bool isNoSend )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgent::setNoSendMessageOnEncryptionFailed( const bool isNoSend )");
	IPMSG_FUNC_RETURN( ipmsgImpl->setNoSendMessageOnEncryptionFailed( isNoSend ) );
}

/**
 * IPv6��Ȥ����ɤ����Υե饰�Υ��å���
 * @retval IPv6��Ȥ����ɤ����Υե饰
 */
bool
IpMessengerAgent::UseIPv6() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgent::UseIPv6() const");
	IPMSG_FUNC_RETURN( ipmsgImpl->UseIPv6() );
}

/**
 * IPv6��Ȥ����ɤ����Υե饰�Υ��å���
 * @param useIPv6 IPv6��Ȥ����ɤ����Υե饰
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
 * IPv6�򥵥ݡ��Ȥ��Ƥ��뤫�ӥ�ɥ��ץ�����������롣
 * @retval true:IPv6�򥵥ݡ���
 * @retval false:IPv6�򥵥ݡ��Ȥ��Ƥ��ʤ���
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
 * IPv4�򥵥ݡ��Ȥ��Ƥ��뤫�ӥ�ɥ��ץ�����������롣
 * @retval true:IPv4�򥵥ݡ���
 * @retval false:IPv4�򥵥ݡ��Ȥ��Ƥ��ʤ���
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
 * ���������®�٤򻻽Ф��롣
 * @retval ���������®�١ʥХ��ȡ��áˡ�
 */
long double
DownloadInfo::getSpeed()
{
	IPMSG_FUNC_ENTER("long double DownloadInfo::getSpeed()");
	IPMSG_FUNC_RETURN( Time() == 0 ? (long double)0 : ( ( long double )Size() / ( long double )Time() ) );
}

/**
 * ���������®��ʸ������������롣
 * @retval ���������®��ʸ�����ñ�̡��áˡ���:1 B/sec, 2.00KB/sec, 3.00 MB/sec, 4.00 GB/sec, 5.00 TB/sec
 */
std::string
DownloadInfo::getSpeedString()
{
	IPMSG_FUNC_ENTER("std::string DownloadInfo::getSpeedString()");
	IPMSG_FUNC_RETURN( DownloadInfo::getUnitSizeString( ( long long )getSpeed() ) + "/sec" );
}

/**
 * ����ʸ������������롣
 * @retval ����ʸ�����ñ�̡ˡ���:1 B, 2.00KB, 3.00 MB, 4.00 GB, 5.00 TB
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
 * ����ʸ������������롣
 * @retval ����ʸ�����ñ�̡ˡ���:1 B, 2.00KB, 3.00 MB, 4.00 GB, 5.00 TB
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
 * �ǥХ���̾�����ꤷ���ϡ��ɥ��������ɥ쥹��������롣
 * @param val �ǥХ���̾��
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
 * IP���ɥ쥹�����ꤷ���ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��Ʒ׻����롣
 * @param val IP���ɥ쥹ʸ����
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
 * �ͥåȥޥ��������ꤷ���ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��Ʒ׻����롣
 * @param val �ͥåȥޥ���ʸ����
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
 * �ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��׻����롣
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
