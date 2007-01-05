/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * IP��å��󥸥㥨��������ȥ��饹��
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"

//NIC�κ����
#define IFR_MAX 20

static IpMessengerAgent *myInstance = NULL;

static pthread_mutex_t instanceMutex;
static int mutex_init_result = IpMsgMutexInit( "IpMessenger::Global", &instanceMutex, NULL );

IpMessengerEvent::~IpMessengerEvent(){};
/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥��󥹤�������롣
 * Singleton�ѥ��������Ѥ��Ƥ���Τǡ��ۥ���ͣ��Υ��󥹥��󥹤Ǥʤ���Фʤ�ʤ���
 * �����Υ��󥹥��󥹤ϥ���åɥ����դǤʤ���
 */
IpMessengerAgent *
IpMessengerAgent::GetInstance()
{
	IpMsgMutexLock( "IpMessengerAgent::GetInstance()", &instanceMutex );
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgent();
	}
	IpMsgMutexUnlock( "IpMessengerAgent::GetInstance()", &instanceMutex );
	return myInstance;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥��󥹤�������롣
 * ���Υ᥽�åɤ�Ȥäƥ��֥������Ȥ�������ʤ���Фʤ�ʤ���
 * �饤�֥����̤��ʤ���ľ��delete���줿���Ϥ��θ��ư��ˤĤ��ƴ��Τ��ʤ���
 * �����Υ��󥹥��󥹤ϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::Release()
{
	IpMsgMutexLock( "IpMessengerAgent::Release()", &instanceMutex );
	if ( myInstance == NULL ) {
		IpMsgMutexUnlock( "IpMessengerAgent::Release()", &instanceMutex );
		return;
	}
	delete myInstance;
	myInstance = NULL;
	IpMsgMutexUnlock( "IpMessengerAgent::Release()", &instanceMutex );
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥ȥ饯����
 */
IpMessengerAgent::IpMessengerAgent()
{
	ipmsgImpl = IpMessengerAgentImpl::GetInstance();
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υǥ��ȥ饯����
 */
IpMessengerAgent::~IpMessengerAgent()
{
	IpMessengerAgentImpl::Release();
}

/**
 * NIC����ꤻ����IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����ư���롣
 * �����Ƥ�NIC���Ф��ƥǥե���ȥݡ��Ȥǥͥåȥ����ư���롣
 */
void
IpMessengerAgent::StartNetwork()
{
	ipmsgImpl->StartNetwork();
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����ư���롣
 * @parem nics ��ư�����оݤȤ���NIC�Υ٥�����
 */
void
IpMessengerAgent::StartNetwork( const vector<NetworkInterface>& nics )
{
	ipmsgImpl->StartNetwork( nics );
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ������ߤ��롣
 */
void
IpMessengerAgent::StopNetwork()
{
	ipmsgImpl->StopNetwork();
}

/**
 * NIC����ꤻ����IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����Ƶ�ư���롣
 * �����Ƥ�NIC���Ф��ƥǥե���ȥݡ��Ȥǥͥåȥ����ư���롣
 */
void
IpMessengerAgent::RestartNetwork()
{
	ipmsgImpl->RestartNetwork();
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����Ƶ�ư���롣
 * @parem nics ��ư�����оݤȤ���NIC�Υ٥�����
 */
void
IpMessengerAgent::RestartNetwork( const vector<NetworkInterface>& nics )
{
	ipmsgImpl->RestartNetwork( nics );
}

/**
 * �ե�����̾����С����Υ��å�����
 * @retval ����С����Υ��ɥ쥹��
 */
FileNameConverter *
IpMessengerAgent::GetFileNameConverter()
{
	return ipmsgImpl->GetFileNameConverter();
}

/**
 * �ե�����̾����С����Υ��å�����
 * @param conv ����С����Υ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetFileNameConverter( FileNameConverter *conv )
{
	ipmsgImpl->SetFileNameConverter( conv );
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
HostListComparator *
IpMessengerAgent::GetSortHostListComparator()
{
	return ipmsgImpl->GetSortHostListComparator();
}; 

/**
 * �ۥ��ȥꥹ����ӥ��֥������ȤΥ��å�����
 * @param comparator �ۥ��ȥꥹ����ӥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetSortHostListComparator( HostListComparator *comparator )
{
	ipmsgImpl->SetSortHostListComparator( comparator );
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
IpMessengerEvent *
IpMessengerAgent::GetEventObject()
{
	return ipmsgImpl->GetEventObject();
}; 

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @param conv ���٥�ȥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetEventObject( IpMessengerEvent *evt )
{
	ipmsgImpl->SetEventObject( evt );
}

/**
 * NIC�ξ����������롣
 * @param nics �ͥåȥ�����󥿡��ե������ΰ���
 */
void
IpMessengerAgent::GetNetworkInterfaceInfo( vector<NetworkInterface>& nics )
{
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( nics );
}

/**
 * ������ʥ����ӥ��������Ρˡ�
 */
void
IpMessengerAgent::Login( string nickname, string groupName )
{
	ipmsgImpl->Login( nickname, groupName );
}

/**
 * �������ȡʥ����ӥ�æ�����Ρˡ�
 */
void
IpMessengerAgent::Logout()
{	
	ipmsgImpl->Logout();
}

/**
 * �ۥ��ȥꥹ�ȼ�����
 * @retval ����������Ȥ��ݻ����Ƥ���HostList���֥�������
 */
HostList&
IpMessengerAgent::GetHostList()
{
	return ipmsgImpl->GetHostList();
}

/**
 * �ۥ��ȥꥹ�ȹ���������
 * @retval ��������HostList���֥�������
 */
HostList&
IpMessengerAgent::UpdateHostList()
{
	return ipmsgImpl->UpdateHostList();
}

/**
 * �Ժߥ⡼�ɤ��ɤ�����Ƚ�ꡣ
 * @retval ����Ѥ��Ժߥ⡼�ɤ��֤���
 */
bool
IpMessengerAgent::IsAbsence()
{
	return ipmsgImpl->IsAbsence();
}
/**
 * �Ժߥ⡼�ɤ򥯥ꥢ���롣
 */
void
IpMessengerAgent::ResetAbsence()
{
	ipmsgImpl->ResetAbsence();
}

/**
 * �Ժߥ⡼�ɤ����ꤹ�롣
 * @param encoding �����륨�󥳡��ǥ���
 * @param absenceModes AbsenceMode���֥������ȤΥ٥����ʼ�ư��������ʣ�����󥳡��ǥ����б����뤿���
 */
void
IpMessengerAgent::SetAbsence( string encoding, vector<AbsenceMode> absenceModes )
{
	ipmsgImpl->SetAbsence( encoding, absenceModes );
}

/**
 * ��å�����������
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param opt �������ץ����
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, isLockPassword, hostCountAtSameTime, IsNoLogging, opt );
}

/**
 * ��å�����������
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param file ź�եե�����
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param opt �������ץ����
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFile& file, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, file, isLockPassword, hostCountAtSameTime, IsNoLogging, opt );
}

/**
 * ��å�����������
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param files ź�եե����뷲
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param opt �������ץ����
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList& files, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, IsNoLogging, opt );
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹����
 * @param addr ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgent::ClearBroadcastAddress()
{
	ipmsgImpl->ClearBroadcastAddress();
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹����
 * @param addr ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgent::DeleteBroadcastAddress( string addr )
{
	ipmsgImpl->DeleteBroadcastAddress( addr );
}

/**
 * �֥��ɥ��㥹�ȥ��ɥ쥹����Ͽ
 * @param addr ��Ͽ����֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgent::AddBroadcastAddress( string addr )
{
	ipmsgImpl->AddBroadcastAddress( addr );
}

/**
 * �оݥۥ��ȤΥС���������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��ȤΥС���������
 */
string
IpMessengerAgent::GetInfo( HostListItem& host )
{
	return ipmsgImpl->GetInfo( host );
}

/**
 * �оݥۥ��Ȥ��Ժ�����ʸ�������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��Ȥ��Ժ�����ʸ�������
 */
string
IpMessengerAgent::GetAbsenceInfo( HostListItem& host )
{
	return ipmsgImpl->GetAbsenceInfo( host );
}

/**
 * �ݻ���Υۥ��ȥꥹ�Ȥ��饰�롼�ץꥹ�Ȥ�������롣
 * @retval ���롼�ץꥹ��
 */
vector<GroupItem>
IpMessengerAgent::GetGroupList()
{
	return ipmsgImpl->GetGroupList();
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgent::DeleteNotify( RecievedMessage msg )
{
	ipmsgImpl->DeleteNotify( msg );
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )
{
	ipmsgImpl->ConfirmMessage( msg );
}

/**
 * �����ѥ�å������ꥹ�Ȥ˳������줿���Ȥ�ޡ������롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgent::AcceptConfirmNotify( SentMessage msg )
{
	ipmsgImpl->AcceptConfirmNotify( msg );
}
		
// private methods start here

/**
 * ���������ʥ桼�������ˡ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
int
IpMessengerAgent::Process()
{
	return ipmsgImpl->Process();
}

/**
 * ������å������θĿ���������롣
 * @retval ������å������θĿ�
 */
int
IpMessengerAgent::GetRecievedMessageCount()
{
	return ipmsgImpl->GetRecievedMessageCount();
}

/**
 * ������å��������ļ��Ф���������å������ꥹ�Ȥ��������롣
 * @retval ������å��������֥������ȡ�
 */
RecievedMessage
IpMessengerAgent::PopRecievedMessage()
{
	return ipmsgImpl->PopRecievedMessage();
}

/**
 * �����ѥ�å������ꥹ�ȤΥݥ��󥿤�������롣
 * @retval �����ѥ�å������ꥹ�ȤΥݥ��󥿡�
 */
SentMessageList *
IpMessengerAgent::GetSentMessages()
{
	return ipmsgImpl->GetSentMessages();
}

/**
 * �����ѥ�å������ꥹ�ȤΥ��ԡ���������롣
 * @retval �����ѥ�å������ꥹ�ȤΥ��ԡ���
 */
SentMessageList
IpMessengerAgent::CloneSentMessages()
{
	return ipmsgImpl->CloneSentMessages();
}

/**
 * ������̾�Υ��å���
 * @retval ������̾
 */
string
IpMessengerAgent::LoginName()
{
	return ipmsgImpl->LoginName();
}

/**
 * �ۥ���̾�Υ��å���
 * @retval �ۥ���̾
 */
string
IpMessengerAgent::HostName()
{
	return ipmsgImpl->HostName();
}

/**
 * ������륢�åפΥ��å���
 * @retval ������륢�å�
 */
bool
IpMessengerAgent::IsDialup()
{
	return ipmsgImpl->IsDialup();
}

/**
 * ������륢�åפΥ��å���
 * @param ������륢�å�
 */
void
IpMessengerAgent::setIsDialup( bool isDialup )
{
	ipmsgImpl->setIsDialup( isDialup );
}

/**
 * ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰�Υ��å���
 * @retval ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::AbortDownloadAtFileChanged()
{
	return ipmsgImpl->AbortDownloadAtFileChanged();
}

/**
 * ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰�Υ��å���
 * @param ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setAbortDownloadAtFileChanged( bool isAbort )
{
	ipmsgImpl->setAbortDownloadAtFileChanged( isAbort );
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @retval ������å���������¸���뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::SaveSentMessage()
{
	return ipmsgImpl->SaveSentMessage();
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @param ������å���������¸���뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setSaveSentMessage( bool isSave )
{
	ipmsgImpl->setSaveSentMessage( isSave );
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @retval ������å���������¸���뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::SaveRecievedMessage()
{
	return ipmsgImpl->SaveRecievedMessage();
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @param ������å���������¸���뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setSaveRecievedMessage( bool isSave )
{
	ipmsgImpl->setSaveRecievedMessage( isSave );
}

/**
 * ���������®�٤򻻽Ф��롣
 * @retval ���������®�١ʥХ��ȡ��áˡ�
 */
long double
DownloadInfo::getSpeed()
{
	return Time() == 0 ? (long double)0 : ( ( long double )Size() / ( long double )Time() );
}

/**
 * ���������®��ʸ������������롣
 * @retval ���������®��ʸ�����ñ�̡��áˡ�
 */
string
DownloadInfo::getSpeedString()
{
	return DownloadInfo::getUnitSizeString( ( long long )getSpeed() ) + "/sec";
}

/**
 * ����ʸ������������롣
 * @retval ����ʸ�����ñ�̡ˡ�
 */
string
DownloadInfo::getSizeString()
{
	return DownloadInfo::getUnitSizeString( Size() );
}

#define IPMSG_SIZE_B	(long double)(1)
#define IPMSG_SIZE_KB	(long double)(1024 * IPMSG_SIZE_B)
#define IPMSG_SIZE_MB	(long double)(1024 * IPMSG_SIZE_KB)
#define IPMSG_SIZE_GB	(long double)(1024 * IPMSG_SIZE_MB)
#define IPMSG_SIZE_TB	(long double)(1024 * IPMSG_SIZE_GB)

/**
 * ����ʸ������������롣
 * @retval ����ʸ�����ñ�̡ˡ�
 */
string
DownloadInfo::getUnitSizeString( long long size )
{
	long double dsize = (long double)size;
	char buf[100];
	if ( dsize >= IPMSG_SIZE_TB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf TB", (dsize / IPMSG_SIZE_TB) );
		return buf;
	} else if ( dsize >= IPMSG_SIZE_GB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf GB", (dsize / IPMSG_SIZE_GB) );
		return buf;
	} else if ( dsize >= IPMSG_SIZE_MB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf MB", (dsize / IPMSG_SIZE_MB) );
		return buf;
	} else if ( dsize >= IPMSG_SIZE_KB ) {
		snprintf( buf, sizeof( buf ), "%.2Lf KB", (dsize / IPMSG_SIZE_KB) );
		return buf;
	}
	snprintf( buf, sizeof( buf ), "%lld B", size );
	return buf;
}
//end of source
