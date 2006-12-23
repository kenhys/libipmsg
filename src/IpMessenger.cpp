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

#ifdef HAVE_PTHREAD
static pthread_mutex_t instanceMutex;
static int mutex_init_result = pthread_mutex_init( &instanceMutex, NULL );
#endif

IpMessengerEvent::~IpMessengerEvent(){};
/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥��󥹤�������롣
 * Singleton�ѥ��������Ѥ��Ƥ���Τǡ��ۥ���ͣ��Υ��󥹥��󥹤Ǥʤ���Фʤ�ʤ���
 * �����Υ��󥹥��󥹤ϥ���åɥ����դǤʤ���
 */
IpMessengerAgent *
IpMessengerAgent::GetInstance()
{
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgent();
	}
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif
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
#ifdef HAVE_PTHREAD
	pthread_mutex_lock( &instanceMutex );
#endif
	if ( myInstance == NULL ) {
#ifdef HAVE_PTHREAD
		pthread_mutex_unlock( &instanceMutex );
#endif
		return;
	}
	delete myInstance;
	myInstance = NULL;
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock( &instanceMutex );
#endif
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥ȥ饯����
 * ���Ź沽���ݡ��Ȥ�ͭ���ʾ�硢������ۥ��Ȥ�RSA��������������Ԥ���
 * ���ѥ��å�No�˻��Ѥ�����������ɤ����ǽ�������롣
 * ���ե�����̾����С����������åȥ��åפ��롣���Ѵ���Ԥ�ʤ�NullConverter���ǥե���ȡ�
 * ���ͥåȥ���ν������
 * �����Υ��󥹥��󥹤ϥ���åɥ����դǤʤ���
 */
IpMessengerAgent::IpMessengerAgent()
{
	ipmsgImpl = IpMessengerAgentImpl::GetInstance();
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υǥ��ȥ饯����
 * ���ޤ����������ȡ�
 * ���Ź沽���ݡ��Ȥ�ͭ���ʾ�硢������ۥ��Ȥ�RSA���������˴���Ԥ���
 * ��������ƺѤΥե�����̾����С����������롣
 * �������åȤΥ�������
 * �����Υ��󥹥��󥹤ϥ���åɥ����դǤʤ���
 */
IpMessengerAgent::~IpMessengerAgent()
{
	IpMessengerAgentImpl::Release();
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����Ƶ�ư���롣
 * ���ޤ����������ȡ�
 * ���ͥåȥ����������
 * ���ͥåȥ���������
 * �����٥�����
 * �����Υ��󥹥��󥹤ϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::RestartNetwork()
{
	ipmsgImpl->RestartNetwork();
}

/**
 * �ե�����̾����С����Υ��å�����
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 * @retval ����С����Υ��ɥ쥹��
 */
FileNameConverter *
IpMessengerAgent::GetFileNameConverter()
{
	return ipmsgImpl->GetFileNameConverter();
}

/**
 * �ե�����̾����С����Υ��å�����
 * ��������ƺѤΥե�����̾����С����������롣
 * ������������С����γ�����ơ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 * @param conv ����С����Υ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetFileNameConverter( FileNameConverter *conv )
{
	ipmsgImpl->SetFileNameConverter( conv );
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
HostListComparator *
IpMessengerAgent::GetSortHostListComparator()
{
	return ipmsgImpl->GetSortHostListComparator();
}; 

/**
 * �ۥ��ȥꥹ����ӥ��֥������ȤΥ��å�����
 * ��������ƺѤΥۥ��ȥꥹ����ӥ��֥������Ȥ������롣
 * ���������ۥ��ȥꥹ����ӥ��֥������Ȥγ�����ơ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 * @param comparator �ۥ��ȥꥹ����ӥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetSortHostListComparator( HostListComparator *comparator )
{
	ipmsgImpl->SetSortHostListComparator( comparator );
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
IpMessengerEvent *
IpMessengerAgent::GetEventObject()
{
	return ipmsgImpl->GetEventObject();
}; 

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * ��������ƺѤΥ��٥�ȥ��֥������Ȥ������롣
 * �����������٥�ȥ��֥������Ȥγ�����ơ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 * @param conv ���٥�ȥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgent::SetEventObject( IpMessengerEvent *evt )
{
	ipmsgImpl->SetEventObject( evt );
}

/**
 * NIC�ξ����������롣
 * �����Ѥ���ͥåȥ�����󥿡��ե�������IP���ɥ쥹����롣�ʥ�����롼�ץХå���Τ������Ƥ�NIC��
 * @param nics �ͥåȥ�����󥿡��ե������ΰ���
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::GetNetworkInterfaceInfo( vector<NetworkInterface>& nics )
{
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( nics );
}

/**
 * ������ʥ����ӥ��������Ρˡ�
 * ��NOOPERATION�ѥ��åȤ��������ͥåȥ�������Ѳ�ǽ���ɤ������ǧ������ǥۥ��ȥꥹ�Ȥ������
 * ��BR_ENTRY��֥��ɥ��㥹�ȡ�
 * ���ѥ��åȤ����������ǡ��ۥ��ȥꥹ�Ȥ���ټ�����
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::Login( string nickname, string groupName )
{
	ipmsgImpl->Login( nickname, groupName );
}

/**
 * �������ȡʥ����ӥ�æ�����Ρˡ�
 * ��BR_EXIT��֥��ɥ��㥹�ȡ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::Logout()
{	
	ipmsgImpl->Logout();
}

/**
 * �ۥ��ȥꥹ�ȼ�����
 * @retval ����������Ȥ��ݻ����Ƥ���HostList���֥�������
 * @retval �ۥ��ȥꥹ��
 */
HostList&
IpMessengerAgent::GetHostList()
{
	return ipmsgImpl->GetHostList();
}

/**
 * �ۥ��ȥꥹ�ȹ���������
 * ��BR_ISGETLIST2��֥��ɥ��㥹�ȡ�
 * ��¾�Υ᥽�åɡ�ANSLIST�����ˤˤƼ�������ޤ��Ե����ʸ޲�ޤǡ�
 * ���ۥ��ȥꥹ�Ȥι��ۤ�ANSLIST�������˹Ԥ���Τǡ����Υ᥽�åɤǤϤҤ������Ե���
 * ���ۥ��ȥꥹ�Ȥ�ANSLIST���������ɲá���������뤳�Ȥ�����ΤǾ��Ʊ���ۥ��ȥꥹ�Ȥ��֤��Ȥϸ¤�ʤ���
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
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
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
bool
IpMessengerAgent::IsAbsence()
{
	return ipmsgImpl->IsAbsence();
}
/**
 * �Ժߥ⡼�ɤ򥯥ꥢ���롣
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
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
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
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
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, isLockPassword, hostCountAtSameTime, opt );
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
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFile file, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, file, isLockPassword, hostCountAtSameTime, opt );
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
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
SentMessage
IpMessengerAgent::SendMsg( HostListItem host, string msg, bool isSecret, AttachFileList files, bool isLockPassword, int hostCountAtSameTime, unsigned long opt )
{
	return ipmsgImpl->SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, opt );
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹����
 * @param addr ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::ClearBroadcastAddress()
{
	ipmsgImpl->ClearBroadcastAddress();
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹����
 * @param addr ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::DeleteBroadcastAddress( string addr )
{
	ipmsgImpl->DeleteBroadcastAddress( addr );
}

/**
 * �֥��ɥ��㥹�ȥ��ɥ쥹����Ͽ
 * @param addr ��Ͽ����֥��ɥ��㥹�ȥ��ɥ쥹
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::AddBroadcastAddress( string addr )
{
	ipmsgImpl->AddBroadcastAddress( addr );
}

/**
 * �оݥۥ��ȤΥС���������������
 * ��GETINFO�ѥ��åȤ�������
 * ��¾�Υ᥽�åɡ�ANSINFO�����ˤˤƼ�������ޤ��Ե����ʸ޲�ޤǡ�
 * ��IP���ɥ쥹�ǥޥå��󥰤���ANSINFO�ǹ������줿�С�������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��ȤΥС���������
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
string
IpMessengerAgent::GetInfo( HostListItem& host )
{
	return ipmsgImpl->GetInfo( host );
#if 0
#endif
}

/**
 * �оݥۥ��Ȥ��Ժ�����ʸ�������������
 * ��GETABSENCEINFO�ѥ��åȤ�������
 * ��¾�Υ᥽�åɡ�ANSABSENCEINFO�����ˤˤƼ�������ޤ��Ե����ʸ޲�ޤǡ�
 * ��IP���ɥ쥹�ǥޥå��󥰤���ANSABSENCEINFO�ǹ������줿�С�������������
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��Ȥ��Ժ�����ʸ�������
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
string
IpMessengerAgent::GetAbsenceInfo( HostListItem& host )
{
	return ipmsgImpl->GetAbsenceInfo( host );
}

/**
 * �ݻ���Υۥ��ȥꥹ�Ȥ��饰�롼�ץꥹ�Ȥ�������롣
 * @retval ���롼�ץꥹ��
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
vector<string>
IpMessengerAgent::GetGroupList()
{
	return ipmsgImpl->GetGroupList();
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::DeleteNotify( RecievedMessage msg )
{
	ipmsgImpl->DeleteNotify( msg );
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgent::ConfirmMessage( RecievedMessage &msg )
{
	ipmsgImpl->ConfirmMessage( msg );
}

/**
 * �����ѥ�å������ꥹ�Ȥ˳������줿���Ȥ�ޡ������롣
 * @param msg ������å��������֥������ȡ�
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
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
//end of source
