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

//NIC�κ����
#define IFR_MAX 20

static IpMessengerAgent *myInstance = NULL;

static pthread_mutex_t instanceMutex;
static int mutex_init_result = IpMsgMutexInit( "IpMessenger::Global", &instanceMutex, NULL );

IpMessengerEvent::~IpMessengerEvent(){};
/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥��󥹤�������롣
 * <ul>
 * <li>Singleton�ѥ��������Ѥ��Ƥ���Τǡ��ۥ���ͣ��Υ��󥹥��󥹤Ǥʤ���Фʤ�ʤ���</li>
 * </ul>
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
 * <ul>
 * <li>���Υ᥽�åɤ�Ȥäƥ��֥������Ȥ�������ʤ���Фʤ�ʤ���</li>
 * <li>�饤�֥����̤��ʤ���ľ��delete���줿���Ϥ��θ��ư��ˤĤ��ƴ��Τ��ʤ���</li>
 * </ul>
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
 * <ul>
 * <li>���Ƥ�NIC���Ф��ƥǥե���ȥݡ��Ȥǥͥåȥ����ư���롣</li>
 * </ul>
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
IpMessengerAgent::StartNetwork( const std::vector<NetworkInterface>& nics )
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
 * <ul>
 * <li>���Ƥ�NIC���Ф��ƥǥե���ȥݡ��Ȥǥͥåȥ����ư���롣</li>
 * </ul>
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
IpMessengerAgent::RestartNetwork( const std::vector<NetworkInterface>& nics )
{
	ipmsgImpl->RestartNetwork( nics );
}

/**
 * �ե�����̾����С����Υ��å�����
 * @retval ����С����Υ��ɥ쥹��
 */
FileNameConverter *
IpMessengerAgent::GetFileNameConverter() const
{
	return ipmsgImpl->GetFileNameConverter();
}

/**
 * �ե�����̾����С����Υ��å�����
 * @param conv ����С����Υ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetFileNameConverter( const FileNameConverter *conv )
{
	ipmsgImpl->SetFileNameConverter( conv );
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
HostListComparator *
IpMessengerAgent::GetSortHostListComparator() const
{
	return ipmsgImpl->GetSortHostListComparator();
}; 

/**
 * �ۥ��ȥꥹ����ӥ��֥������ȤΥ��å�����
 * @param comparator �ۥ��ȥꥹ����ӥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetSortHostListComparator( const HostListComparator *comparator )
{
	ipmsgImpl->SetSortHostListComparator( comparator );
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
IpMessengerEvent *
IpMessengerAgent::GetEventObject() const
{
	return ipmsgImpl->GetEventObject();
}; 

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @param evt ���٥�ȥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetEventObject( const IpMessengerEvent *evt )
{
	ipmsgImpl->SetEventObject( evt );
}

/**
 * NIC�ξ����������롣
 * @param nics �ͥåȥ�����󥿡��ե������ΰ���
 */
void
IpMessengerAgent::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics )
{
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( nics );
}

/**
 * ������ʥ����ӥ��������Ρˡ�
 */
void
IpMessengerAgent::Login( std::string nickname, std::string groupName )
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
IpMessengerAgent::IsAbsence() const
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
IpMessengerAgent::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )
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
 * @param IsNoLogging ���˻Ĥ��ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
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
 * @param IsNoLogging ���˻Ĥ��ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFile& file, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
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
 * @param IsNoLogging ���˻Ĥ��ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, std::string msg, bool isSecret, AttachFileList& files, bool isLockPassword, int hostCountAtSameTime, bool IsNoLogging, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, IsNoLogging, opt );
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹�����ƥ��ꥢ
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
IpMessengerAgent::DeleteBroadcastAddress( std::string addr )
{
	ipmsgImpl->DeleteBroadcastAddress( addr );
}

/**
 * �֥��ɥ��㥹�ȥ��ɥ쥹����Ͽ
 * @param addr ��Ͽ����֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgent::AddBroadcastAddress( std::string addr )
{
	ipmsgImpl->AddBroadcastAddress( addr );
}

/**
 * �оݥۥ��ȤΥС���������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��ȤΥС���������
 */
std::string
IpMessengerAgent::GetInfo( HostListItem& host )
{
	return ipmsgImpl->GetInfo( host );
}

/**
 * �оݥۥ��Ȥ��Ժ�����ʸ�������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��Ȥ��Ժ�����ʸ�������
 */
std::string
IpMessengerAgent::GetAbsenceInfo( HostListItem& host )
{
	return ipmsgImpl->GetAbsenceInfo( host );
}

/**
 * �ݻ���Υۥ��ȥꥹ�Ȥ��饰�롼�ץꥹ�Ȥ�������롣
 * @retval ���롼�ץꥹ��
 */
std::vector<GroupItem>
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
IpMessengerAgent::CloneSentMessages() const
{
	return ipmsgImpl->CloneSentMessages();
}

/**
 * ������̾�Υ��å���
 * @retval ������̾
 */
std::string
IpMessengerAgent::LoginName() const
{
	return ipmsgImpl->LoginName();
}

/**
 * �ۥ���̾�Υ��å���
 * @retval �ۥ���̾
 */
std::string
IpMessengerAgent::HostName() const
{
	return ipmsgImpl->HostName();
}

/**
 * �ǥե���ȥݡ��ȤΥ��å���
 * @retval �ǥե���ȥݡ���
 */
int
IpMessengerAgent::DefaultPortNo() const
{
	return ipmsgImpl->DefaultPortNo();
}

/**
 * �ǥե���ȥݡ��ȤΥ��å���
 * @param defaultPortNo �ǥե���ȥݡ���
 */
void
IpMessengerAgent::setDefaultPortNo( const int defaultPortNo )
{
	ipmsgImpl->setDefaultPortNo( defaultPortNo );
}

/**
 * ������륢�åפΥ��å���
 * @retval ������륢�å�
 */
bool
IpMessengerAgent::IsDialup() const
{
	return ipmsgImpl->IsDialup();
}

/**
 * ������륢�åפΥ��å���
 * @param isDialup ������륢�å�
 */
void
IpMessengerAgent::setIsDialup( const bool isDialup )
{
	ipmsgImpl->setIsDialup( isDialup );
}

/**
 * ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰�Υ��å���
 * @retval ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::AbortDownloadAtFileChanged() const
{
	return ipmsgImpl->AbortDownloadAtFileChanged();
}

/**
 * ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰�Υ��å���
 * @param isAbort ��������ɻ��˥ե����뤬�ѹ����줿���˶ػߤ��뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setAbortDownloadAtFileChanged( const bool isAbort )
{
	ipmsgImpl->setAbortDownloadAtFileChanged( isAbort );
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @retval ������å���������¸���뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::SaveSentMessage() const
{
	return ipmsgImpl->SaveSentMessage();
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @param isSave ������å���������¸���뤫�ɤ����Υե饰
 */
void
IpMessengerAgent::setSaveSentMessage( const bool isSave )
{
	ipmsgImpl->setSaveSentMessage( isSave );
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @retval ������å���������¸���뤫�ɤ����Υե饰
 */
bool
IpMessengerAgent::SaveRecievedMessage() const
{
	return ipmsgImpl->SaveRecievedMessage();
}

/**
 * ������å���������¸���뤫�ɤ����Υե饰�Υ��å���
 * @param isSave ������å���������¸���뤫�ɤ����Υե饰
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
 * @retval ���������®��ʸ�����ñ�̡��áˡ���:1 B/sec, 2.00KB/sec, 3.00 MB/sec, 4.00 GB/sec, 5.00 TB/sec
 */
std::string
DownloadInfo::getSpeedString()
{
	return DownloadInfo::getUnitSizeString( ( long long )getSpeed() ) + "/sec";
}

/**
 * ����ʸ������������롣
 * @retval ����ʸ�����ñ�̡ˡ���:1 B, 2.00KB, 3.00 MB, 4.00 GB, 5.00 TB
 */
std::string
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
 * @retval ����ʸ�����ñ�̡ˡ���:1 B, 2.00KB, 3.00 MB, 4.00 GB, 5.00 TB
 */
std::string
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

/**
 * IP���ɥ쥹�����ꤷ���ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��Ʒ׻����롣
 * @param val IP���ɥ쥹ʸ����
 */
void
NetworkInterface::setIpAddress( const std::string val )
{
	_IpAddress = val;
	inet_pton( AF_INET, val.c_str(), (void *)&_NativeIpAddress );
	recalc();
}
/**
 * �ͥåȥޥ��������ꤷ���ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��Ʒ׻����롣
 * @param val �ͥåȥޥ���ʸ����
 */
void
NetworkInterface::setNetMask( const std::string val )
{
	_NetMask = val;
	inet_pton( AF_INET, val.c_str(), (void *)&_NativeNetMask );
	recalc();
}
/**
 * IP���ɥ쥹(�ͥ��ƥ���)�����ꤷ���ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��Ʒ׻����롣
 * @param val IP���ɥ쥹��
 */
void
NetworkInterface::setNativeIpAddress( const struct in_addr val )
{
	_NativeIpAddress = val;
	char ipAddrBuf[IP_ADDR_MAX_SIZE];
	_IpAddress = inet_ntop( AF_INET, &val, ipAddrBuf, sizeof( ipAddrBuf ) );
	recalc();
}
/**
 * �ͥåȥޥ���(�ͥ��ƥ���)�����ꤷ���ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��Ʒ׻����롣
 * @param val �ͥåȥޥ�����
 */
void
NetworkInterface::setNativeNetMask( const struct in_addr val )
{
	_NativeNetMask = val;
	char netMaskBuf[IP_ADDR_MAX_SIZE];
	_NetMask = inet_ntop( AF_INET, &val, netMaskBuf, sizeof( netMaskBuf ) );
	recalc();
}
/**
 * �ͥåȥ�����ɥ쥹���֥��ɥ��㥹�ȥ��ɥ쥹��׻����롣
 */
void
NetworkInterface::recalc()
{
	char buf[IP_ADDR_MAX_SIZE];
	_NativeNetworkAddress.s_addr = _NativeIpAddress.s_addr & _NativeNetMask.s_addr;
	_NetworkAddress = inet_ntop( AF_INET, &_NativeNetworkAddress, buf, sizeof( buf ) );

	_NativeBroadcastAddress = GetBroadcastAddress( _NativeNetworkAddress, _NativeNetMask );
	_BroadcastAddress = inet_ntop( AF_INET, &_NativeBroadcastAddress, buf, sizeof( buf ) );
}
//end of source
