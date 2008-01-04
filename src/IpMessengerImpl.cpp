/**
 * IP ��å��󥸥�饤�֥��(Unix��)<br/>
 * IP��å��󥸥㥨��������ȥ��饹��
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
#include <pthread.h>
using namespace ipmsg;

static IpMessengerAgentImpl *myInstance = NULL;

static pthread_mutex_t instanceMutex;
static int mutex_init_result = IpMsgMutexInit( "IpMessengerImpl::Global", &instanceMutex, NULL );

/**
 * IP ��å��󥸥㥤�٥�ȥ��饹�Υǥե���ȼ�����
 */
class IpMessengerNullEvent: public IpMessengerEvent {
	public:
		/**
		 * ���Υ��٥�ȳ��������٥��(GUI����åɤΥ�å�����������Ƥ�������)
		 */
		virtual void EventBefore(){};
		/**
		 * ���Υ��٥�Ƚ�λ�奤�٥��(GUI����åɤΥ����å�����������Ƥ�������)
		 */
		virtual void EventAfter(){};
		/**
		 * �ۥ��ȥꥹ�ȥ�ե�å���奤�٥��
		 * @param hostList �ۥ��ȥꥹ��
		 */
		virtual void RefreshHostListAfter( HostList& hostList ){ printf("UpdateHostListAfter\n"); };
		/**
		 * �ۥ��ȥꥹ�ȹ����奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param hostList �ۥ��ȥꥹ��
		 */
		virtual void UpdateHostListAfter( HostList& hostList ){ printf("UpdateHostListAfter\n"); };
		/**
		 * �ۥ��ȥꥹ�ȼ�����ȥ饤���顼���٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @retval true:��ȥ饤����
		 * @retval false:��ȥ饤���ʤ�
		 */
		virtual bool GetHostListRetryError(){ printf("GetHostListRetryError\n");return false; };
		/**
		 * ��å����������奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 * @retval true:�������ƥ�å���������¸������
		 * @retval false:��å���������¸
		 */
		virtual bool RecieveAfter( RecievedMessage& msg ){ printf("RecieveAfter\n");return false; };
		/**
		 * ��å����������奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 */
		virtual void SendAfter( SentMessage& msg ){ printf("SendAfter\n"); };
		/**
		 * ��å�����������ȥ饤���顼���٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 * @retval true:��ȥ饤����
		 * @retval false:��ȥ饤���ʤ�
		 */
		virtual bool SendRetryError( SentMessage& msg ){ printf("SendRetryError\n");return false; };
		/**
		 * ��å������Ź沽�������Υ��٥�ȡ�
		 */
		virtual void NotifySendEncryptionFail( HostListItem& host ){ printf("NotifySendEncryptionFail\n"); };
		/**
		 * ��å������Ź沽���ԥ��٥�ȡ�
		 * @retval true:�Ź沽��������������
		 * @retval false:���Ԥ�����
		 */
		virtual bool IsSendContinueOnEncryptionFail( HostListItem& host ){ printf("IsSendContinueOnEncryptionFail\n"); return false; };
		/**
		 * �������θ奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 */
		virtual void OpenAfter( SentMessage& msg ){ printf("OpenAfter\n"); };
		/**
		 * ��������ɳ��ϥ��٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 */
		virtual void DownloadStart( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadStart\n"); };
		/**
		 * ��������ɽ����楤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 */
		virtual void DownloadProcessing( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadProcessing\n"); };
		/**
		 * ��������ɽ�λ���٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 */
		virtual void DownloadEnd( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadEnd\n"); };
		/**
		 * ��������ɥ��顼���٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param msg ������å�����
		 * @param file ź�եե�����
		 * @param info ��������ɾ���
		 * @param data DownloadFile��DownloadDir�ǻ��ꤷ��Ǥ�եǡ����ؤΥݥ���
		 * @retval true:��ȥ饤����
		 * @retval false:��ȥ饤���ʤ�
		 */
		virtual bool DownloadError( RecievedMessage& msg, AttachFile& file, DownloadInfo &info, void *data ){ printf("DownloadError\n"); return false; };
		/**
		 * �ۥ��Ȥλ������θ奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param hostList �ۥ���
		 */
		virtual void EntryAfter( HostListItem& host ){ printf("EntryAfter\n"); };
		/**
		 * �ۥ��Ȥ�æ�����θ奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param hostList �ۥ���
		 */
		virtual void ExitAfter( HostListItem& host ){ printf("ExitAfter\n"); };
		/**
		 * �Ժߥ⡼�ɹ����奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param hostList �ۥ���
		 */
		virtual void AbsenceModeChangeAfter( HostListItem& host ){ printf("AbsenceModeChangeAfter\n"); };
		/**
		 * �С�������������奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param host �ۥ���
		 * @param version �С������
		 */
		virtual void VersionInfoRecieveAfter( HostListItem &host, std::string version ){ printf("VersionInfoRecieveAfter\n"); };
		/**
		 * �Ժ߾ܺپ�������奤�٥�ȡ����٥�Ȥ�ȯ���������Ȥ򼨤�����print��Ԥ���
		 * @param host �ۥ���
		 * @param absenceDetail �Ժ߾ܺپ���
		 */
		virtual void AbsenceDetailRecieveAfter( HostListItem &host, std::string absenceDetail ){ printf("AbsenceDetailRecieveAfter\n"); };
};

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥��󥹤�������롣
 * <ul>
 * <li>Singleton�ѥ��������Ѥ��Ƥ���Τǡ��ۥ���ͣ��Υ��󥹥��󥹤Ǥʤ���Фʤ�ʤ���</li>
 * </ul>
 */
IpMessengerAgentImpl *
IpMessengerAgentImpl::GetInstance()
{
	IPMSG_FUNC_ENTER("IpMessengerAgentImpl *IpMessengerAgentImpl::GetInstance()");
	mutex_init_result = 0; //fix warnings, but no effect.
	IpMsgMutexLock( "IpMessengerAgentImpl::GetInstance()", &instanceMutex );
	if ( myInstance == NULL ) {
		myInstance = new IpMessengerAgentImpl();
	}
	IpMsgMutexUnlock( "IpMessengerAgentImpl::GetInstance()", &instanceMutex );
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
IpMessengerAgentImpl::Release()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::Release()");
	IpMsgMutexLock( "IpMessengerAgentImpl::Release()", &instanceMutex );
	if ( myInstance == NULL ) {
		IpMsgMutexUnlock( "IpMessengerAgentImpl::Release()", &instanceMutex );
		IPMSG_FUNC_EXIT;
	}
	delete myInstance;
	myInstance = NULL;
	IpMsgMutexUnlock( "IpMessengerAgentImpl::Release()", &instanceMutex );
	IPMSG_FUNC_EXIT;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υ��󥹥ȥ饯����<br/>
 * <ul>
 * <li>�Ź沽���ݡ��Ȥ�ͭ���ʾ�硢������ۥ��Ȥ�RSA��������������Ԥ���</li>
 * <li>�ѥ��å�No�˻��Ѥ�����������ɤ����ǽ�������롣</li>
 * <li>�ե�����̾����С����������åȥ��åפ��롣���Ѵ���Ԥ�ʤ�NullConverter���ǥե���ȡ�</li>
 * <li>�ͥåȥ���ν������
 * </ul>
 */
IpMessengerAgentImpl::IpMessengerAgentImpl()
		:_DefaultPortNo( IPMSG_DEFAULT_PORT )
{
	IPMSG_FUNC_ENTER("IpMessengerAgentImpl::IpMessengerAgentImpl()");
	CryptoInit();
	srandom( time( NULL ) );
	converter = new NullFileNameConverter();
	compare = new HostListDefaultComparator();
	setAbortDownloadAtFileChanged( false );
	setSaveSentMessage( true );
	setSaveRecievedMessage( true );
	event = new IpMessengerNullEvent();
	IPMSG_FUNC_EXIT;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υǥ��ȥ饯����
 * <ul>
 * <li>�ޤ����������ȡ�</li>
 * <li>�Ź沽���ݡ��Ȥ�ͭ���ʾ�硢������ۥ��Ȥ�RSA���������˴���Ԥ���</li>
 * <li>������ƺѤΥե�����̾����С����������롣</li>
 * <li>�����åȤΥ�������
 * </ul>
 */
IpMessengerAgentImpl::~IpMessengerAgentImpl()
{
	IPMSG_FUNC_ENTER("IpMessengerAgentImpl::~IpMessengerAgentImpl()");
	if ( IsNetworkStarted() ){
		Logout();
		StopNetwork();
	}
	CryptoEnd();
	delete converter;
	delete compare;
	delete event;
	IPMSG_FUNC_EXIT;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����ư���롣
 * <ul>
 * <li>NIC����ꤷ�ʤ��ǥͥåȥ������(���Ƥ�NIC���ǥե���ȥݡ��Ȥǵ�ư)��</li>
 * </ul>
 */
void
IpMessengerAgentImpl::StartNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::StartNetwork()");
	std::vector<NetworkInterface> nics;
	StartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����ư���롣
 * <ul>
 * <li>�ͥåȥ���������</li>
 * <li>�ͥåȥ���ˤ��ߤȤ��ƥۥ��Ⱦ��󤬻ĤäƤ��뤳�Ȥ��θ���ư�ö�������ȡ�</li>
 * <li>���ϸ�����󤹤�ɬ�פ�����ޤ���</li>
 * </ul>
 */
void
IpMessengerAgentImpl::StartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::StartNetwork( const std::vector<NetworkInterface>& nics )");
	NICs.clear();
	IpMessengerAgentImpl::GetNetworkInterfaceInfo( NICs, UseIPv6(), DefaultPortNo() );
	NetworkInit( nics );
	Logout();
	// TODO ��������åɳ���
	pthread_t t_id;

//	printf( "Thread create\n" );fflush(stdout);
	if ( pthread_create( &t_id, NULL, ProcessPacketThread, NULL ) != 0 ){
		perror("StartNetwork:pthread_create");
		IPMSG_FUNC_EXIT;
	}
	_IsNetworkStarted = true;
//	printf( "Thread detach\n" );fflush(stdout);
	if ( pthread_detach( t_id ) != 0 ){
		perror("StartNetwork:pthread_detach");
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

void *
ipmsg::ProcessPacketThread( void *param )
{
#ifdef DEBUG
	long p = 0;
#endif
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	while( agent->IsNetworkStarted() ) {
#ifdef DEBUG
		printf( "ProcessPacketThread(p=%ld)\n", ++p );fflush(stdout);
#endif
		agent->Process();
		if ( usleep( 500000L ) != 0 ) {
			printf( "usleep fail\n" );fflush(stdout);
		}
	}
#ifdef DEBUG
	printf( "ProcessPacketThread END.\n" );fflush(stdout);
#endif
	return NULL;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����λ���롣
 * <ul>
 * <li>�ͥåȥ����������</li>
 * </ul>
 */
void
IpMessengerAgentImpl::StopNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::StopNetwork()");
	// TODO ��������åɽ�λ�����Ԥ���碌��
	_IsNetworkStarted = false;
	usleep( 1000000L );
	NetworkEnd();
	IPMSG_FUNC_EXIT;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����Ƶ�ư���롣
 * <ul>
 * <li>NIC����ꤷ�ʤ��ǥͥåȥ���Ƶ�ư(���Ƥ�NIC���ǥե���ȥݡ��ȺƵ�ư)��</li>
 * </ul>
 */
void
IpMessengerAgentImpl::RestartNetwork()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::RestartNetwork()");
	std::vector<NetworkInterface> nics;
	RestartNetwork( nics );
	IPMSG_FUNC_EXIT;
}

/**
 * IP ��å��󥸥㥨��������ȥ��饹�Υͥåȥ����Ƶ�ư���롣
 * <ul>
 * <li>�ޤ����������ȡ�</li>
 * <li>�ͥåȥ����������</li>
 * <li>�ͥåȥ���������</li>
 * <li>���٥�����</li>
 * </ul>
 */
void
IpMessengerAgentImpl::RestartNetwork( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::RestartNetwork( const std::vector<NetworkInterface>& nics )");
	if ( IsNetworkStarted() ) {
		Logout();
		StopNetwork();
	}
	StartNetwork( nics );
	Login( Nickname, GroupName );
	IPMSG_FUNC_EXIT;
}

/**
 * �ե�����̾����С����Υ��å�����<br>
 * @retval ����С����Υ��ɥ쥹��
 */
FileNameConverter *
IpMessengerAgentImpl::GetFileNameConverter() const
{
	IPMSG_FUNC_ENTER("FileNameConverter *IpMessengerAgentImpl::GetFileNameConverter() const");
	IPMSG_FUNC_RETURN( converter );
}

/**
 * �ե�����̾����С����Υ��å�����<br>
 * <ul>
 * <li>������ƺѤΥե�����̾����С����������롣</li>
 * <li>����������С����γ�����ơ�</li>
 * </ul>
 * @param conv ����С����Υ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgentImpl::SetFileNameConverter( const FileNameConverter *conv )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetFileNameConverter( const FileNameConverter *conv )");
	if ( conv == NULL ){
		IPMSG_FUNC_EXIT;
	}
	//�����������θ
	if ( conv == converter ){
		IPMSG_FUNC_EXIT;
	}
	delete converter;
	converter = const_cast<FileNameConverter *>( conv );
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ����ӥ��֥������ȤΥ��å�����
 * @retval �ۥ��ȥꥹ����ӥ��֥������ȤΥ��ɥ쥹��
 */
HostListComparator *
IpMessengerAgentImpl::GetSortHostListComparator() const
{
	IPMSG_FUNC_ENTER("HostListComparator *IpMessengerAgentImpl::GetSortHostListComparator() const");
	IPMSG_FUNC_RETURN( compare );
}; 

/**
 * �ۥ��ȥꥹ����ӥ��֥������ȤΥ��å�����
 * <ul>
 * <li>������ƺѤΥۥ��ȥꥹ����ӥ��֥������Ȥ������롣</li>
 * <li>�������ۥ��ȥꥹ����ӥ��֥������Ȥγ�����ơ�</li>
 * </ul>
 * @param comparator �ۥ��ȥꥹ����ӥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgentImpl::SetSortHostListComparator( const HostListComparator *comparator )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetSortHostListComparator( const HostListComparator *comparator )");
	if ( comparator == NULL ){
		IPMSG_FUNC_EXIT;
	}
	//�����������θ
	if ( comparator == compare ){
		IPMSG_FUNC_EXIT;
	}
	delete compare;
	compare = const_cast<HostListComparator *>( comparator );
	IPMSG_FUNC_EXIT;
}

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * @retval ���٥�ȥ��֥������ȤΥ��ɥ쥹��
 */
IpMessengerEvent *
IpMessengerAgentImpl::GetEventObject() const
{
	IPMSG_FUNC_ENTER("IpMessengerEvent *IpMessengerAgentImpl::GetEventObject() const");
	IPMSG_FUNC_RETURN( event );
}; 

/**
 * ���٥�ȥ��֥������ȤΥ��å�����
 * <ul>
 * <li>������ƺѤΥ��٥�ȥ��֥������Ȥ������롣</li>
 * <li>���������٥�ȥ��֥������Ȥγ�����ơ�</li>
 * </ul>
 * @param evt ���٥�ȥ��֥������ȤΥ��ɥ쥹����ưŪ�˺�������Τǡ������å���˺������ƤϤʤ�ʤ����ҡ��׾�˺������뤳�ȡ�
 */
void
IpMessengerAgentImpl::SetEventObject( const IpMessengerEvent *evt )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetEventObject( const IpMessengerEvent *evt )");
	if ( evt == NULL ){
		IPMSG_FUNC_EXIT;
	}
	//�����������θ
	if ( evt == event ){
		IPMSG_FUNC_EXIT;
	}
	delete event;
	event = const_cast<IpMessengerEvent *>( evt );
	IPMSG_FUNC_EXIT;
}

/**
 * NIC�ξ����������롣
 * <ul>
 * <li>���Ѥ���ͥåȥ�����󥿡��ե�������IP���ɥ쥹����롣�ʥ�����롼�ץХå���Τ������Ƥ�NIC��</li>
 * </ul>
 * @param nics �ͥåȥ�����󥿡��ե������ΰ���
 */
void
IpMessengerAgentImpl::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )");
#ifdef DEBUG
printf("IpMessengerAgentImpl::GetNetworkInterfaceInfo useIPv6=%s\n", useIPv6 ? "true" : "false" );fflush(stdout);
#endif
	ipmsg::getNetworkInterfaceInfo( nics, useIPv6, defaultPortNo );
	IPMSG_FUNC_EXIT;
}

/**
 * �ͥåȥ����Ϣ�ν������
 * <ul>
 * <li>�Ķ��ѿ�����ۥ���̾��������ʽ���ʤ����localhost�����</li>
 * <li>�Ķ��ѿ�����桼��̾��������ʽ���ʤ����uid��</li>
 * </ul>
 */
void
IpMessengerAgentImpl::NetworkInit( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::NetworkInit( const std::vector<NetworkInterface>& nics )");
	haveIPv4Nic = false;
	haveIPv6Nic = false;

	if ( nics.size() > 0 ){
		for( unsigned int i = 0; i < nics.size(); i++ ){
#ifdef ENABLE_IPV4
			if ( nics[i].AddressFamily() == AF_INET ) {
				haveIPv4Nic = true;
			}
#endif
#ifdef ENABLE_IPV6
		 	if ( nics[i].AddressFamily() == AF_INET6 ) {
				haveIPv6Nic = true;
			}
#endif
		}
	} else {
		for( unsigned int i = 0; i < NICs.size(); i++ ){
#ifdef ENABLE_IPV4
			if ( NICs[i].AddressFamily() == AF_INET ) {
				haveIPv4Nic = true;
			}
#endif
#ifdef ENABLE_IPV6
		 	if ( NICs[i].AddressFamily() == AF_INET6 ) {
				haveIPv6Nic = true;
			}
#endif
		}
	}

	_HostName = IpMsgGetHostName();
	if ( _HostName == "" ) {
		_HostName = "localhost";
	}

	uid_t uid = getuid();
	_LoginName = IpMsgGetLoginName( uid );
	if ( _LoginName == "" ){
		char buf[100];
		IpMsgIntToString( buf, sizeof( buf ), uid );
		_LoginName = buf;
	}

#ifdef HAVE_OPENSSL
	DecryptErrorMessage = "\r\n"\
						  " ==== AutoReply(DecryptErr) ====\r\n" \
						  "  My PubKey is updated, I can't\r\n" \
						  "  receive your message.\r\n" \
						  "  Please press refresh button.\r\n" \
						  " ==============================";
#endif	//HAVE_OPENSSL
	if ( nics.size() > 0 ){
		InitSend( nics );
		InitRecv( nics );
	} else {
		if ( NICs.size() > 0 ) {
			InitSend( NICs);
			InitRecv( NICs );
		}
	}
	printf( "%s network service started.\n", IPMSG_AGENT_VERSION );
	fflush( stdout );
	IPMSG_FUNC_EXIT;
}

/**
 * �ͥåȥ����Ϣ�ν������
 * <ul>
 * <li>���ƤΥ����åȤ��Ĥ��롣</li>
 * </ul>
 */
void
IpMessengerAgentImpl::NetworkEnd()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::NetworkEnd()");
	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		close(udp_sd[i]);
	}
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		close(tcp_sd[i]);
	}
	udp_sd.clear();
	tcp_sd.clear();
	sd_addr.clear();
	sd_address_family.clear();
	IPMSG_FUNC_EXIT;
}

/**
 * ������ʥ����ӥ��������Ρˡ�
 * <ul>
 * <li>NOOPERATION�ѥ��åȤ��������ͥåȥ�������Ѳ�ǽ���ɤ������ǧ������ǥۥ��ȥꥹ�Ȥ������</li>
 * <li>BR_ENTRY��֥��ɥ��㥹�ȡ�</li>
 * <li>�ѥ��åȤ����������ǡ��ۥ��ȥꥹ�Ȥ���ټ�����</li>
 * </ul>
 */
void
IpMessengerAgentImpl::Login( std::string nickname, std::string groupName )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::Login( std::string nickname, std::string groupName )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	SendNoOperation();

	_IsAbsence = false;
	Logout();
#if defined(DEBUG) || !defined(NDEBUG)
	memset( sendBuf, 0, MAX_UDPBUF );
#endif
	if ( nickname != "" ) {
		Nickname = nickname;
	} else {
		Nickname = _LoginName;
	}
	GroupName = groupName;
	std::string optBuf = Nickname + '\0' + GroupName +'\0';
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ENTRY ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ENTRY, sendBuf, sendBufLen );
	ResetAbsence();

//	skulkHostList.Lock("IpMessengerAfentImpl::Login");
	std::vector<HostListItem>::iterator hi = skulkHostList.begin();
	for( ; hi != skulkHostList.end(); hi++ ) {
		struct sockaddr_storage addr;
		if ( createSockAddrIn( &addr, hi->IpAddress(), hi->PortNo() ) != NULL ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::Login HideFromAddr\n");fflush( stdout );
#endif
			HideFromAddr( addr );
		}
	}
//	skulkHostList.Unlock("IpMessengerAfentImpl::Login");
//	RecvPacket( false );
	//0.05�äޤġ�
	usleep( 50000L );
//	RecvPacket( false );
	//0.1�äޤġ�
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * �������ȡʥ����ӥ�æ�����Ρˡ�
 * <ul>
 * <li>BR_EXIT��֥��ɥ��㥹�ȡ�</li>
 * </ul>
 */
void
IpMessengerAgentImpl::Logout()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::Logout()");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_EXIT ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_EXIT, sendBuf, sendBufLen );
//	RecvPacket( false );
	//0.1�äޤġ�
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * �����������Υۥ��ȸ��������ӥ��������Ρˡ�
 * <ul>
 * <li>NOOPERATION�ѥ��åȤ��������ͥåȥ�������Ѳ�ǽ���ɤ������ǧ������ǥۥ��ȥꥹ�Ȥ������</li>
 * <li>BR_ENTRY������Υۥ��Ȥ����롣</li>
 * <li>�ѥ��åȤ����������ǡ��ۥ��ȥꥹ�Ȥ���ټ�����</li>
 * </ul>
 */
void
IpMessengerAgentImpl::VisibleToAddr( struct sockaddr_storage &addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::VisibleToAddr( struct sockaddr_storage &addr )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	if ( !IsNetworkStarted() ) {
		IPMSG_FUNC_EXIT;
	}

#if defined(DEBUG) || !defined(NDEBUG)
	memset( sendBuf, 0, MAX_UDPBUF );
#endif
	std::string optBuf = Nickname + '\0' + GroupName +'\0';
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ENTRY ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_BR_ENTRY, sendBuf, sendBufLen, addr );
	//0.1�äޤġ�
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * �������ȡ�����Υۥ��ȸ����˱���뤿��Υ����ӥ�æ�����Ρˡ�
 * <ul>
 * <li>BR_EXIT������Υۥ��Ȥ����롣</li>
 * </ul>
 */
void
IpMessengerAgentImpl::HideFromAddr( struct sockaddr_storage &addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::HideFromAddr( struct sockaddr_storage &addr )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	if ( !IsNetworkStarted() ) {
		IPMSG_FUNC_EXIT;
	}

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_EXIT ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_BR_EXIT, sendBuf, sendBufLen, addr );
	//0.1�äޤġ�
	usleep( 100000L );
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ�ȼ�����
 * @retval ����������Ȥ��ݻ����Ƥ���HostList���֥�������
 */
HostList&
IpMessengerAgentImpl::GetHostList()
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgentImpl::GetHostList()");
	IPMSG_FUNC_RETURN( appearanceHostList );
}

/**
 * �ۥ��ȥꥹ�ȹ���������<br>
 * <ul>
 * <li>BR_ISGETLIST2��֥��ɥ��㥹�ȡ�</li>
 * <li>¾�Υ᥽�åɡ�ANSLIST�����ˤˤƼ�������ޤ��Ե����ʸ޲�ޤǡ�</li>
 * </ul>
 * <ul>
 * <li>�ۥ��ȥꥹ�Ȥι��ۤ�ANSLIST�������˹Ԥ���Τǡ����Υ᥽�åɤǤϤҤ������Ե���</li>
 * <li>�ۥ��ȥꥹ�Ȥ�ANSLIST���������ɲá���������뤳�Ȥ�����ΤǾ��Ʊ���ۥ��ȥꥹ�Ȥ��֤��Ȥϸ¤�ʤ���</li>
 * </ul>
 * @retval ��������HostList���֥�������
 */
HostList&
IpMessengerAgentImpl::UpdateHostList( bool isRetry )
{
	IPMSG_FUNC_ENTER("HostList& IpMessengerAgentImpl::UpdateHostList( bool isRetry )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	//��ȥ饤�椫�ɤ�����
	if ( !isRetry && !hostList.IsAsking() ) {
#if defined(DEBUG)
		printf("IpMessengerAgentImpl::UpdateHost HostList backup now.\n");
		fflush( stdout );
#endif
		hostListBackup = hostList;
		hostList.clear();
#if defined(DEBUG)
		printf("IpMessengerAgentImpl::UpdateHost HostList cleared.\n");
		fflush( stdout );
#endif
	}
	//��礻��˾��֤�����
	hostList.setIsAsking( true );
	if ( !isRetry ) {
		hostList.setAskStartTime( time( NULL ) );
		hostList.setPrevTry( hostList.AskStartTime() );
		hostList.setRetryCount( 0 );
	}
	AddDefaultHost();

	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ISGETLIST2 ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ISGETLIST2, sendBuf, sendBufLen );
	//�����ػ�(��ȥ饤����RecvPacket����ƤФ��)
	if ( !isRetry ) {
//		int pcount = RecvPacket( false );
		//��ʬ�ʳ��Υۥ��Ȥ����դ���ʤ��������ȥ饤����ַ����֤�
		for( int i = 0; i < 5; i++ ) {
			//0.01�äޤġ�
			usleep( 10000L );
//			pcount = RecvPacket( false );
		}
	}

#if defined(DEBUG)
	IpMsgDumpHostList( " M Y   H O S T L I S T ( BEFORE SORT ) ", hostList );
#endif
	//������
	if ( compare != NULL ) {
		hostList.sort( compare );
		appearanceHostList.sort( compare );
	}
#if defined(DEBUG)
	IpMsgDumpHostList( " M Y   H O S T L I S T ( AFTER SORT ) ", hostList );
#endif
	//���٥�Ȥ�󤲤�
	if ( event != NULL ) {
		event->EventBefore();
#if defined(DEBUG)
		printf("UpdateHostListAfter before\n");
#endif
		event->UpdateHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("UpdateHostListAfter after\n");
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( appearanceHostList );
}

/**
 * �Ժߥ⡼�ɤ��ɤ�����Ƚ�ꡣ
 * @retval ����Ѥ��Ժߥ⡼�ɤ��֤���
 */
bool
IpMessengerAgentImpl::IsAbsence() const
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::IsAbsence() const");
	IPMSG_FUNC_RETURN( _IsAbsence );
}
/**
 * �Ժߥ⡼�ɤ򥯥ꥢ���롣
 */
void
IpMessengerAgentImpl::ResetAbsence()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ResetAbsence()");
	_IsAbsence = false;
	localEncoding = "";
	std::vector<AbsenceMode> d;
	absenceModeList = d;
	SendAbsence();
	IPMSG_FUNC_EXIT;
}

/**
 * �Ժߥ⡼�ɤ����ꤹ�롣
 * @param encoding �����륨�󥳡��ǥ���
 * @param absenceModes AbsenceMode���֥������ȤΥ٥����ʼ�ư��������ʣ�����󥳡��ǥ����б����뤿���
 */
void
IpMessengerAgentImpl::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SetAbsence( std::string encoding, std::vector<AbsenceMode> absenceModes )");
	_IsAbsence = true;

	localEncoding = encoding;
	absenceModeList = absenceModes;
	SendAbsence();
	IPMSG_FUNC_EXIT;
}

/**
 * ��å�����������
 * @param message �������줿��å��������֥�������(Output)
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param IsNoLogging ���˵�Ͽ���ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
bool
IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )");
	AttachFileList files;
	IPMSG_FUNC_RETURN( SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, isLogging, opt, false, 0UL ) );
}

/**
 * ��å�����������
 * @param message �������줿��å��������֥�������(Output)
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param file ź�եե�����
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param IsNoLogging ���˵�Ͽ���ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 */
bool
IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFile& file, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt )");
	AttachFileList files;
	files.AddFile( file );
	IPMSG_FUNC_RETURN( SendMsg( host, msg, isSecret, files, isLockPassword, hostCountAtSameTime, isLogging, opt, false, 0UL ) );
}

/**
 * ��å�����������
 * @param message �������줿��å��������֥�������(Output)
 * @param host ������ۥ���
 * @param msg ������å�����
 * @param isSecret ���񤫤ɤ����򼨤��ե饰
 * @param files ź�եե����뷲
 * @param isLockPassword ���Ĥ����ɤ����򼨤��ե饰
 * @param hostCountAtSameTime Ʊ�������ۥ��ȿ�
 * @param IsNoLogging ���˵�Ͽ���ʤ��ʤ��Ȥ�侩��
 * @param opt �������ץ����
 * @param isRetry ���ۤΥ�ȥ饤�ˤ�������׵ᤫ�򼨤��ե饰
 * @param PrevPacketNo ��ȥ饤�Ǥ��������Υѥ��å��ֹ�
 */
bool
IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt, bool isRetry, unsigned long PrevPacketNo )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendMsg( HostListItem host, std::string msg, const Secret &isSecret, AttachFileList& files, const LockPassword &isLockPassword, int hostCountAtSameTime, const Logging &isLogging, unsigned long opt, bool isRetry, unsigned long PrevPacketNo )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	size_t optBufSize = GetMaxOptionBufferSize() + 1;
	char *optBuf = (char *)calloc( optBufSize, 1 );
	if ( optBuf == NULL ) {
		perror("IpMessengerAgentImpl::SendMsg calloc()");
		exit(1);
	}
	int optBufLen = 0;
	struct sockaddr_storage addr;
	bool isEncrypted = false;
	if ( createSockAddrIn( &addr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_RETURN( false );
	}

//	RecvPacket( false );
	//0.1�äޤġ�
	usleep( 100000L );

	optBufLen = optBufSize < msg.size() ? optBufSize : msg.size();
	memcpy( optBuf, msg.c_str(), optBufLen );
#ifdef HAVE_OPENSSL
	//OpenSSL���ݡ��Ȥ�ͭ���ʤ顢�Ź沽
	if ( isSecret.IsSecret() ) {
#if defined(DEBUG)
		printf( "IpMessengerAgentImpl::SendMsg Send message specified by Secret Mode.\n" );
		printf( "IpMessengerAgentImpl::SendMsg Target host's public key=[%s]\n", host.PubKeyHex().c_str() );
		printf( "IpMessengerAgentImpl::SendMsg Target host's encrypt method by hex=[%s]\n", host.EncryptMethodHex().c_str() );
#endif
		if ( EncryptMsg( host, (unsigned char*)optBuf, optBufLen, &optBufLen, optBufSize ) ) {
			isEncrypted = true;
		} else if ( NoSendMessageOnEncryptionFailed() ) {
			printf("IpMessengerAgentImpl::SendMsg Encryption failed.SendMsg was canceled.\n");
			fflush(stdout);
			if ( event != NULL && !isRetry ) {
				event->EventBefore();
#if defined(DEBUG)
				printf("NotifySendEncryotionFail before\n");
#endif
				event->NotifySendEncryptionFail( host );
#if defined(DEBUG)
				printf("NotifySendEncryotionFail after\n");
#endif
				event->EventAfter();
			}
			free( optBuf );
			IPMSG_FUNC_RETURN( false );
		} else {
			printf("IpMessengerAgentImpl::SendMsg Encryption failed.");
			fflush(stdout);
			if ( event != NULL && !isRetry ) {
				event->EventBefore();
#if defined(DEBUG)
				printf("IsSendContinueIbEncryptionFail before\n");
#endif
				if ( !event->IsSendContinueOnEncryptionFail( host ) ) {
					event->EventAfter();
#if defined(DEBUG)
					printf("IsSendContinueIbEncryptionFail after\n");
#endif
					printf("SendMsg was canceled.\n");
					fflush(stdout);
					free( optBuf );
					IPMSG_FUNC_RETURN( false );
				}
#if defined(DEBUG)
				printf("IsSendContinueIbEncryptionFail after\n");
#endif
				event->EventAfter();
			}
			printf("SendMsg try send no encryption message.\n");
			fflush(stdout);
			optBufLen = optBufSize < msg.size() ? optBufSize : msg.size();
			memcpy( optBuf, msg.c_str(), optBufLen );
		}
	} else {
		optBufLen = optBufSize < msg.size() ? optBufSize : msg.size();
		memcpy( optBuf, msg.c_str(), optBufLen );
	}
#endif	//HAVE_OPENSSL

	optBuf[optBufLen++] = '\0';
	IpMsgPrintBuf( "optBuf:", optBuf, optBufLen );

	int fileBufLen = 0;
	char fileBuf[MAX_UDPBUF];

	//�ե������ź��
	for( std::vector<AttachFile>::iterator ixfile = files.begin(); ixfile != files.end(); ixfile++ ) {
		ixfile->GetLocalFileInfo();
		std::string filename = converter->ConvertLocalToNetwork( ixfile->FileName() );
		size_t wsize = snprintf( &fileBuf[ fileBufLen ], sizeof( fileBuf ) - fileBufLen - 1,
							"%d:%s:%llx:%lx:%lx:\a",
							ixfile->FileId(), filename.c_str(), ixfile->FileSize(), (unsigned long)ixfile->MTime(), ixfile->Attr() );
		//ź�եե����������������Хåե���Ķ�����ޤ���sprintf���񤭤���ʤ��ä���
		if ( optBufLen + fileBufLen + wsize - 1 > MAX_UDPBUF || wsize >= sizeof( fileBuf ) - fileBufLen - 1 ) {
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::SendMsg Attachment file's attribute buffer was overflow.\n" );
			fflush(stdout);
#endif
			break;
		}
		fileBufLen += wsize;
		fileBuf[fileBufLen] = '\0';
	}
	memcpy( &optBuf[ optBufLen ], fileBuf, fileBufLen );
	optBufLen += fileBufLen;
	IpMsgPrintBuf( "fileBuf2:", fileBuf, fileBufLen );
	if ( optBufLen >= (int) optBufSize - 1 ) {
		optBufLen =  optBufSize - 1;
	}
	optBuf[ optBufLen ] = '\0';

	IpMsgPrintBuf( "optBuf2:", optBuf, optBufLen );

	//��ȥ饤���ϰ����ΥХ��å�No����Ѥ��롣
	unsigned long packetNo = (isRetry && PrevPacketNo != 0UL ? PrevPacketNo : random() );

	sendBufLen = CreateNewPacketBuffer( IPMSG_SENDMSG | IPMSG_SENDCHECKOPT |
#ifdef HAVE_OPENSSL
										  ( isEncrypted ? IPMSG_ENCRYPTOPT : 0UL ) |
#endif	//HAVE_OPENSSL
										  ( !isLogging.IsLogging() ? IPMSG_NOLOGOPT : 0UL ) |
										  ( isSecret.IsSecret() ? IPMSG_SECRETOPT : 0UL ) |
										  ( _IsAbsence ? IPMSG_AUTORETOPT : 0UL ) |
										  ( isLockPassword.IsLockPassword() ? IPMSG_PASSWORDOPT : 0UL ) |
										  ( files.size() > 0 ? IPMSG_FILEATTACHOPT : 0UL ) | opt,
										  packetNo,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_SENDMSG, sendBuf, sendBufLen, addr );

	if ( !isRetry ) {
		SentMessage message;
		message.setTo( addr );
		message.setHost( host );
		message.setPacketNo( packetNo );
		message.setMessage( msg );
		message.setSent( time( NULL ) );
		message.setPrevTry( message.Sent() );
		message.setIsRetryMaxOver( false );
		message.setRetryCount( 0 );
		message.setIsConfirmed( false );
		message.setIsPasswordLock( isLockPassword.IsLockPassword() );
		message.setIsCrypted( isEncrypted );
		message.setIsConfirmAnswered( false );
		message.setHostCountAtSameTime( hostCountAtSameTime );
		message.setOpt( opt );
		message.setIsNoLogging( !isLogging.IsLogging() );
		message.setIsSecret( isSecret.IsSecret() );
		message.setFiles( files );
		message.setIsSent( false );
		if ( SaveSentMessage() ){
			sentMsgList.append( message );
		}
	}

	free( optBuf );
	IPMSG_FUNC_RETURN( true );
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹�򥯥ꥢ
 */
void
IpMessengerAgentImpl::ClearBroadcastAddress()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ClearBroadcastAddress()");
	broadcastAddr.clear();
	IPMSG_FUNC_EXIT;
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹����
 * @param addr ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgentImpl::DeleteBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteBroadcastAddress( std::string addr )");
	std::vector<struct sockaddr_storage>::iterator net = FindBroadcastNetworkByAddress( addr );
	if ( net != broadcastAddr.end() ) {
#if defined(DEBUG)
		struct sockaddr_storage netaddr = *net;
		printf( "IpMessengerAgentImpl::DeleteBroadcastAddress Address=%s Port=%d\n", getSockAddrInRawAddress( netaddr ).c_str(), ntohs( getSockAddrInPortNo( netaddr ) ) );
		fflush( stdout );
#endif
		broadcastAddr.erase( net );
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

/**
 * �֥��ɥ��㥹�ȥ��ɥ쥹����Ͽ
 * @param addr ��Ͽ����֥��ɥ��㥹�ȥ��ɥ쥹
 */
void
IpMessengerAgentImpl::AddBroadcastAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddBroadcastAddress( std::string addr )");
	struct sockaddr_storage addAddr;
	if ( createSockAddrIn( &addAddr, addr, DefaultPortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}

	std::string broadIp = getSockAddrInRawAddress( addAddr );
	std::vector<struct sockaddr_storage>::iterator net = FindBroadcastNetworkByAddress( broadIp );
	if ( net != broadcastAddr.end() ) {
		IPMSG_FUNC_EXIT;
	}
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::AddBroadcastAddress Address=%s Port=%d\n", getSockAddrInRawAddress( &addAddr ).c_str(), ntohs( getSockAddrInPortNo( addAddr ) ) );fflush( stdout );
#endif
	broadcastAddr.push_back( addAddr );
	IPMSG_FUNC_EXIT;
}

/**
 * ��Ͽ�ѤΥ֥��ɥ��㥹�ȥ��ɥ쥹�򸡺�������������sockaddr_in��¤�Τ��ֵѤ��롣
 * @param addr �֥��ɥ��㥹�ȥ��ɥ쥹ʸ����
 * @retval sockaddr_in��¤��
 */
std::vector<struct sockaddr_storage>::iterator
IpMessengerAgentImpl::FindBroadcastNetworkByAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("std::vector<struct sockaddr_storage>::iterator IpMessengerAgentImpl::FindBroadcastNetworkByAddress( std::string addr )");
	struct sockaddr_storage ss;
	if ( createSockAddrIn( &ss, addr, 0 ) == NULL ) {
		IPMSG_FUNC_RETURN( broadcastAddr.end() );
	}
	for( std::vector<struct sockaddr_storage>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
		if ( isSameSockAddrIn( ss, *ixaddr ) ){
			IPMSG_FUNC_RETURN( ixaddr );
		}
	}
	IPMSG_FUNC_RETURN( broadcastAddr.end() );
}

/**
 * ��Ͽ�Ѥα����ۥ��Ȥ򥯥ꥢ
 */
void
IpMessengerAgentImpl::ClearSkulkHost()
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ClearSkulkHost()");
	skulkHostList.clear();
	if ( IsNetworkStarted() ) {
		Login( Nickname, GroupName );
	}
	IPMSG_FUNC_EXIT;
}

/**
 * ��Ͽ�Ѥα����ۥ��Ȥ���
 * @param addr ��Ͽ�Ѥα����ۥ��ȤΥ��ɥ쥹
 */
void
IpMessengerAgentImpl::DeleteSkulkHostAddress( const std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteSkulkHostAddress( std::string addr )");
	//���٤ƤΥۥ��ȥ��ɥ쥹����õ��
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( addr );
	if ( hostIt != hostList.end() ) {
		//ȯ��
		DeleteSkulkHost( *hostIt );
		IPMSG_FUNC_EXIT;
	}
	HostListItem host;
	host.setIpAddress( addr );
	host.setPortNo( DefaultPortNo() );
	DeleteSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * ��Ͽ�Ѥα����ۥ��Ȥ���
 * @param host ��Ͽ�Ѥα����ۥ���
 */
void
IpMessengerAgentImpl::DeleteSkulkHost( const HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteSkulkHost( HostListItem &host )");
	std::vector<HostListItem>::iterator hi = FindSkulkHostByAddress( host.IpAddress() );
	if ( hi != skulkHostList.end() ) {
		if ( IsNetworkStarted() ) {
			struct sockaddr_storage addr;
			if ( createSockAddrIn( &addr, hi->IpAddress(), hi->PortNo() ) == NULL ) {
				IPMSG_FUNC_EXIT;
			}
			VisibleToAddr( addr );
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::DeleteSkulkHost Address=%s Port=%d\n", getSockAddrInRawAddress( addr ).c_str(), ntohs( getSockAddrInPortNo( addr ) ) );
			fflush( stdout );
#endif
		}
		skulkHostList.DeleteHostByAddress( host.IpAddress() );
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

/**
 * �����ۥ��Ȥ���Ͽ
 * @param addr ��Ͽ����ۥ��ȤΥ��ɥ쥹
 */
void
IpMessengerAgentImpl::AddSkulkHostAddress( const std::string addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddSkulkHostAddress( std::string addr )");
	//���٤ƤΥۥ��ȥ��ɥ쥹����õ��
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( addr );
	if ( hostIt != hostList.end() ) {
		//ȯ��
		AddSkulkHost( *hostIt );
		IPMSG_FUNC_EXIT;
	}
	HostListItem host;
	host.setIpAddress( addr );
	host.setPortNo( DefaultPortNo() );
	AddSkulkHost( host );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ۥ��Ȥ���Ͽ
 * @param host ��Ͽ����ۥ���
 */
void
IpMessengerAgentImpl::AddSkulkHost( const HostListItem &host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddSkulkHost( HostListItem host )");
	struct sockaddr_storage addAddr;
	if ( createSockAddrIn( &addAddr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}

	std::string hideIp = getSockAddrInRawAddress( addAddr );
	std::vector<HostListItem>::iterator hi = FindSkulkHostByAddress( hideIp );
	if ( hi != skulkHostList.end() ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::AddSkulkHost HideFromAddr\n");fflush( stdout );
#endif
		HideFromAddr( addAddr );
		IPMSG_FUNC_EXIT;
	}
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::AddSkulkHost Address=%s Port=%d\n", getSockAddrInRawAddress( &addAddr ).c_str(), ntohs( getSockAddrInPortNo( addAddr ) ) );fflush( stdout );
#endif
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddSkulkHost HideFromAddr\n");fflush( stdout );
#endif
	HideFromAddr( addAddr );
	skulkHostList.AddHost( host, true );
	IPMSG_FUNC_EXIT;
}

/**
 * ��Ͽ�Ѥα����ۥ��ȤΥ��ɥ쥹�򸡺�������������sockaddr_in��¤�Τ��ֵѤ��롣
 * @param addr �����ۥ��ȤΥ��ɥ쥹ʸ����
 * @retval sockaddr_in��¤��
 */
std::vector<HostListItem>::iterator
IpMessengerAgentImpl::FindSkulkHostByAddress( std::string addr )
{
	IPMSG_FUNC_ENTER("std::vector<struct sockaddr_storage>::iterator IpMessengerAgentImpl::FindSkulkHostByAddress( std::string addr )");
	struct sockaddr_storage ss;
	if ( createSockAddrIn( &ss, addr, 0 ) == NULL ) {
		IPMSG_FUNC_RETURN( skulkHostList.end() );
	}
	for( std::vector<HostListItem>::iterator ixaddr = skulkHostList.begin(); ixaddr != skulkHostList.end(); ixaddr++ ){
		if ( addr == ixaddr->IpAddress() ){
			IPMSG_FUNC_RETURN( ixaddr );
		}
	}
	IPMSG_FUNC_RETURN( skulkHostList.end() );
}

/**
 * ��Ͽ�Ѥα����ۥ��ȤΥۥ��ȥꥹ�Ȥ��ֵѤ��롣
 * @retval �����ۥ��ȥꥹ��
 */
HostList
IpMessengerAgentImpl::GetSkulkHost()
{
	IPMSG_FUNC_ENTER("HostList IpMessengerAgent::GetSkulkHost()");
	HostList ret = skulkHostList;
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �оݥۥ��ȤΥС�����������䤤��碌�롣
 * <ul>
 * <li>GETINFO�ѥ��åȤ�������</li>
 * </ul>
 * @param host �оݤΥۥ���
 */
void
IpMessengerAgentImpl::QueryVersionInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::QueryVersionInfo( HostListItem& host )");
	char sendBuf[MAX_UDPBUF]={0};
	int sendBufLen;
	struct sockaddr_storage addr;
	if ( createSockAddrIn( &addr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETINFO,
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_GETINFO, sendBuf, sendBufLen, addr );
	IPMSG_FUNC_EXIT;
}

/**
 * �оݥۥ��ȤΥС���������������
 * <ul>
 * <li>�С�����������䤤��碌�롣</li>
 * <li>¾�Υ᥽�åɡ�ANSINFO�����ˤˤƼ�������ޤ��Ե����ʸ޲�ޤǡ�</li>
 * <li>IP���ɥ쥹�ǥޥå��󥰤���ANSINFO�ǹ������줿�С�����������֤���</li>
 * <li>�Ԥ���碌��Ԥ��ޤ������֤��������礬����ޤ���Query�Ϥ�API�ǥ��٥�Ȥ򽦤����Ȥ�侩�����Υ᥽�åɤ����Ʊ�����٥�Ȥ�ȯ�Ԥ��ޤ���</li>
 * </ul>
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��ȤΥС���������
 */
std::string
IpMessengerAgentImpl::GetInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgentImpl::GetInfo( HostListItem& host )");
	//0.05�äޤġ�
	usleep( 50000L );
//	RecvPacket( false );
	for( int i = 0; i < 5; i++ ) {
		//0.05�äޤġ�
		usleep( 50000L );
//		RecvPacket( false );
	}
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( host.IpAddress() );
	if ( hostIt != hostList.end() ) {
		IPMSG_FUNC_RETURN( hostIt->Version() );
	}
	IPMSG_FUNC_RETURN( "" );
}

/**
 * �оݥۥ��Ȥ��Ժ�����ʸ���������䤤��碌�롣
 * <ul>
 * <li>GETABSENCEINFO�ѥ��åȤ�������</li>
 * </ul>
 * @param host �оݤΥۥ���
 */
void
IpMessengerAgentImpl::QueryAbsenceInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::QueryAbsenceInfo( HostListItem& host )");
	char sendBuf[MAX_UDPBUF]={0};
	int sendBufLen;
	struct sockaddr_storage addr;

	if ( createSockAddrIn( &addr, host.IpAddress(), host.PortNo() ) == NULL ) {
		IPMSG_FUNC_EXIT;
	}

	sendBufLen = CreateNewPacketBuffer( IPMSG_GETABSENCEINFO,
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_GETABSENCEINFO, sendBuf, sendBufLen, addr );
	IPMSG_FUNC_EXIT;
}

/**
 * �оݥۥ��Ȥ��Ժ�����ʸ�������������
 * <ul>
 * <li>�Ժ�����ʸ���������䤤��碌�롣</li>
 * <li>¾�Υ᥽�åɡ�ANSABSENCEINFO�����ˤˤƼ�������ޤ��Ե����ʸ޲�ޤǡ�</li>
 * <li>IP���ɥ쥹�ǥޥå��󥰤���ANSABSENCEINFO�ǹ������줿�Ժ�����ʸ���������֤���</li>
 * <li>�Ԥ���碌��Ԥ��ޤ������֤��������礬����ޤ���Query�Ϥ�API�ǥ��٥�Ȥ򽦤����Ȥ�侩�����Υ᥽�åɤ����Ʊ�����٥�Ȥ�ȯ�Ԥ��ޤ���</li>
 * </ul>
 * @param host �оݤΥۥ���
 * @retval �оݥۥ��Ȥ��Ժ�����ʸ�������
 */
std::string
IpMessengerAgentImpl::GetAbsenceInfo( HostListItem& host )
{
	IPMSG_FUNC_ENTER("std::string IpMessengerAgentImpl::GetAbsenceInfo( HostListItem& host )");
	QueryAbsenceInfo( host );
	//0.05�äޤġ�
	usleep( 50000L );
//	RecvPacket( false );
	for( int i = 0; i < 5; i++ ) {
		//0.05�äޤġ�
		usleep( 50000L );
//		RecvPacket( false );
	}
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( host.IpAddress() );
	if ( hostIt != hostList.end() ) {
		IPMSG_FUNC_RETURN( hostIt->AbsenceDescription() );
	}
	IPMSG_FUNC_RETURN( "" );
}

/**
 * �ݻ���Υۥ��ȥꥹ�Ȥ��饰�롼�ץꥹ�Ȥ�������롣
 * @retval ���롼�ץꥹ��
 */
std::vector<GroupItem>
IpMessengerAgentImpl::GetGroupList()
{
	IPMSG_FUNC_ENTER("std::vector<GroupItem> IpMessengerAgentImpl::GetGroupList()");
	IPMSG_FUNC_RETURN( hostList.GetGroupList() );
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgentImpl::DeleteNotify( RecievedMessage msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DeleteNotify( RecievedMessage msg )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen = 0;
	char *dmyptr;
	unsigned long packetNo = strtoul( msg.MessagePacket().Option().c_str(), &dmyptr, 10 );

	optBufLen = IpMsgULongToString( optBuf, sizeof( optBuf ), packetNo );
	sendBufLen = CreateNewPacketBuffer( IPMSG_DELMSG,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_DELMSG, sendBuf, sendBufLen, msg.MessagePacket().Addr() );
	IPMSG_FUNC_EXIT;
}

/**
 * �������˥�å����������������Ȥ����Τ��롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgentImpl::ConfirmMessage( RecievedMessage &msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::ConfirmMessage( RecievedMessage &msg )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

	if ( ( IPMSG_SECRETOPT & msg.MessagePacket().CommandOption() ) && !msg.IsConfirmed() ) {
		packetNoBufLen = IpMsgULongToString( packetNoBuf, sizeof( packetNoBuf ), msg.MessagePacket().PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_READMSG,
											  _LoginName, _HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( -1, IPMSG_READMSG, sendBuf, sendBufLen, msg.MessagePacket().Addr() );
	}
	msg.setIsConfirmed( true );
//	RecvPacket( false );
	IPMSG_FUNC_EXIT;
}

/**
 * �����ѥ�å������ꥹ�Ȥ˳������줿���Ȥ�ޡ������롣
 * @param msg ������å��������֥������ȡ�
 */
void
IpMessengerAgentImpl::AcceptConfirmNotify( SentMessage msg )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AcceptConfirmNotify( SentMessage msg )");
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( msg.PacketNo() );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmAnswered( true );
	}
	IPMSG_FUNC_EXIT;
}

// private methods start here

/**
 * ���������
 * <ul>
 * <li>�֥��ɥ��㥹�ȥ��ɥ쥹��¤�Τν�����ܥꥹ�Ȥ˲������ࡣ</li>
 * </ul>
 */
void
IpMessengerAgentImpl::InitSend( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::InitSend( const std::vector<NetworkInterface>& nics )");
	struct sockaddr_storage addr;
#ifdef ENABLE_IPV4
	if ( haveIPv4Nic ) {
		if ( createSockAddrIn( &addr, "255.255.255.255", DefaultPortNo() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}
		broadcastAddr.push_back( addr );
	}
#endif	
#ifdef ENABLE_IPV6
	if ( _UseIPv6 && haveIPv6Nic ) {
		for( unsigned int i = 0; i < nics.size(); i++ ){
			if ( nics[i].AddressFamily() == AF_INET6 ) {
				if ( createSockAddrIn( &addr, "ff02::1", DefaultPortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
					IPMSG_FUNC_EXIT;
				}
			}
		}
		broadcastAddr.push_back( addr );
	}
#endif	
	for( unsigned int i = 0; i < nics.size(); i++ ){
		struct sockaddr_storage addr;
		if ( createSockAddrIn( &addr, getBroadcastAddress( nics[i].AddressFamily(), nics[i].NetworkAddress(), nics[i].NetMask() ), DefaultPortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}
		bool IsFound = false;
		for( std::vector<struct sockaddr_storage>::iterator i = broadcastAddr.begin(); i != broadcastAddr.end(); ++i ){
#ifdef DEBUG
			std::string check =getSockAddrInRawAddress( &( *i ) );
			std::string base = getSockAddrInRawAddress( &addr );
			int checkp = getSockAddrInPortNo( &( *i ) );
			int basep = getSockAddrInPortNo( &addr );
			printf("IpMessengerAgentImpl::InitSend check[%s][%d] == base[%s][%d]\n", check.c_str(), checkp, base.c_str(), basep );
#endif	
			if ( getSockAddrInRawAddress( &( *i ) ) == getSockAddrInRawAddress( &addr ) &&
				 getSockAddrInPortNo( &( *i ) ) == getSockAddrInPortNo( &addr ) ){
				IsFound = true;
#ifdef DEBUG
				printf("IpMessengerAgentImpl::InitSend Broadcast address was found.\n" );
#endif	
				break;
			}
		}
		if ( !IsFound ) {
#ifdef DEBUG
			printf("IpMessengerAgentImpl::InitSend Broadcast address was not found.\n" );
#endif	
			broadcastAddr.push_back( addr );
		}
	}
#ifdef DEBUG
	for( std::vector<struct sockaddr_storage>::iterator i = broadcastAddr.begin(); i != broadcastAddr.end(); ++i ){
		printf( "IpMessengerAgentImpl::InitSend broadcastAddr=[%s]\n", getSockAddrInRawAddress( &( *i ) ).c_str() );
	}
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * TCP�ѥ��åȤ�������Ԥ���
 * @param sd �����åȥǥ�������ץ�
 * @param buf �Хåե�
 * @param size �Хåե�������
 */
void
IpMessengerAgentImpl::SendTcpPacket( int sd, char *buf, int size )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SendTcpPacket( int sd, char *buf, int size )");
#if defined(DEBUG)
	printf( "\n" );
	printf("IpMessengerImpl::SendTcpPacket == SEND TCP START ==============================>\n");fflush( stdout );
#endif
	IpMsgPrintBuf( "IpMessengerImpl::SendTcpPacket Send TCP Buffer", buf, size );
	int ret = 0;
	ret = send( sd, buf, size + 1, 0 );
	if ( ret <= 0 ) {
		perror("send");
#if defined(DEBUG)
		printf("IpMessengerImpl::SendTcpPacket SEND TCP FAILED\n");fflush( stdout );
#endif
	}
#if defined(DEBUG)
	printf("IpMessengerImpl::SendTcpPacket <= SEND TCP END =================================\n\n");fflush( stdout );
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * UDP�ѥ��åȤ�������Ԥ���
 * @param send_socket ���������åȡ�
 * @param buf �Хåե�
 * @param size �Хåե�������
 * @param to_addr �������IP���ɥ쥹
 */
void
IpMessengerAgentImpl::SendPacket( const int send_socket, const unsigned long cmd, char *buf, int size, struct sockaddr_storage to_addr )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SendPacket( const unsigned long cmd, char *buf, int size, struct sockaddr_storage to_addr )");
#if defined(DEBUG)
	printf( "\n" );
	printf( "IpMessengerAgentImpl::SendPacket == SEND  START =====================================>\n");fflush( stdout );
	printf( "IpMessengerAgentImpl::SendPacket Packet Command i[%s]To Address=[%s][Sock %d]\n", GetCommandString( cmd ).c_str(), getSockAddrInRawAddress( to_addr ).c_str(), send_socket );fflush( stdout );
#endif
	IpMsgPrintBuf( "IpMessengerImpl::SendUdpPacket Send UDP Buffer", buf, size );

	UdpSendto( send_socket, &to_addr, buf, size );

#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::SendPacket <= SEND  END ========================================\n\n");fflush( stdout );
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * UDP�ѥ��åȤΥ֥��ɥ��㥹�Ȥ�Ԥ���
 * <ul>
 * <li>�֥��ɥ��㥹�ȥ��ɥ쥹�ꥹ�Ȥ���Ͽ�ѤΥ��ɥ쥹�������������롣</li>
 * </ul>
 * @param buf �Хåե�
 * @param size �Хåե�������
 */
void
IpMessengerAgentImpl::SendBroadcast( const unsigned long cmd, char *buf, int size )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SendBroadcast( const unsigned long cmd, char *buf, int size )");
#if defined(DEBUG)
	printf( "\n" );
	printf("IpMessengerAgentImpl::SendBroadcast == SEND BROADCAST START ==================>\n");fflush( stdout );
	printf( "IpMessengerAgentImpl::SendBroadcast Command[%s]\n", GetCommandString( cmd ).c_str() );fflush( stdout );
#endif
	IpMsgPrintBuf( "SendBroadcast:SendUdpBroadcastBuffer", buf, size );
	for( std::vector<struct sockaddr_storage>::iterator ixaddr = broadcastAddr.begin(); ixaddr != broadcastAddr.end(); ixaddr++ ){
#if defined(DEBUG)
		printf( "IpMessengerAgentImpl::SendBroadcast Sendto specified broadcast addresses.Packet Command=[%s]To Address=[%s]\n", GetCommandString( cmd ).c_str(), getSockAddrInRawAddress( *ixaddr ).c_str() );fflush( stdout );
#endif
		UdpSendto( -1, &(*ixaddr), buf, size );
	}
	for( std::vector<HostListItem>::iterator ixhost = hostList.begin(); ixhost != hostList.end(); ixhost++ ){
		if ( ixhost->CommandNo() | IPMSG_DIALUPOPT ) {
			struct sockaddr_storage addr;
			if ( createSockAddrIn( &addr, ixhost->IpAddress(), ixhost->PortNo() ) == NULL ) {
				IPMSG_FUNC_EXIT;
			}
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::SendBroadcast Sendto dialup hosts.Packet Command=[%s]To Address=[%s]\n", GetCommandString( cmd ).c_str(), getSockAddrInRawAddress( addr ).c_str() );fflush( stdout );
#endif
			UdpSendto( -1, &addr, buf, size );
		}
	}
#if defined(DEBUG)
	printf("IpMessengerAgentImpl::SendBroadcast <= SEND BROADCAST END =====================\n\n");fflush( stdout );
#endif
	IPMSG_FUNC_EXIT;
}

/**
 * �����åȤ˥Х���ɤ��줿�ͥåȥ�����Ф���Ʊ��ͥåȥ����°���Ƥ���С�����NIC������ʸ������Ԥ����Ȥ��ݾڤ���sendto��
 * @param send_socket ���������åȡ�0̤���ϥ֥��ɥ��㥹�ȡ��ʥ����åȤϼ�ư����ˡ�
 * @param addr ������Υ��ɥ쥹��
 * @param buf �Хåե�
 * @param size �Хåե�������
 */
void
IpMessengerAgentImpl::UdpSendto( const int send_socket, struct sockaddr_storage *addr, char *buf, int size )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::UdpSendto( const struct sockaddr_storage *addr, char *buf, int size )");
	//���������åȤ���ꤵ�줿��硣
	if ( send_socket >= 0 ){
		int ret = sendToSockAddrIn( send_socket, buf, size + 1, addr );
		if ( ret <= 0 ) {
			fprintf( stderr, "IpMessengerAgentImpl::UdpSendto Address=[%s] Port=(%d)@Sock=%d(manual specified for unicasting) errno=(%d):", getSockAddrInRawAddress( addr ).c_str(), ntohs( getSockAddrInPortNo( addr ) ), send_socket, errno );fflush( stdout );
			perror("sendto 1.");
#if defined(DEBUG)
			printf("IpMessengerAgentImpl::UdpSendto SEND FAILED\n");fflush( stdout );
#endif
		}
		IPMSG_FUNC_EXIT;
	}
	//send_socket < 0 �ϥ֥��ɥ��㥹��
	if ( udp_sd.size() == 0 ) {
		IPMSG_FUNC_EXIT;
	}
	int sock = -1;
	int same_family_sock = -1;
	int scope = if_nametoindex( sd_addr.begin()->second.DeviceName().c_str() );

#if defined(DEBUG)
	std::string from_addr = sd_addr.begin()->second.IpAddress();

	printf( "IpMessengerAgentImpl::UdpSendto SendTo Address [%s](%d)\n",
				getSockAddrInRawAddress( addr ).c_str(),
				ntohs( getSockAddrInPortNo( addr ) ) );
	fflush( stdout );
#endif
	for( std::map<int,NetworkInterface>::iterator i = sd_addr.begin(); i != sd_addr.end(); ++i ){
#if defined(DEBUG)
		printf( "IpMessengerAgentImpl::UdpSendto Searching using NIC Address=[%s](%d) Network Address=[%s] Netmask=[%s]\n",
				i->second.IpAddress().c_str(),
				i->second.PortNo(),
				i->second.NetworkAddress().c_str(),
				i->second.NetMask().c_str() );fflush( stdout );
#endif
		if ( isSameNetwork( addr, i->second.NetworkAddress() ,i->second.NetMask() ) ) {
			int scope_id = if_nametoindex( i->second.DeviceName().c_str() );
#ifdef ENABLE_IPV6
			if ( addr->ss_family == AF_INET6 && scope_id != getScopeId( addr ) ){
				continue;
			}
#endif
			sock = i->first;
			scope = scope_id;
#if defined(DEBUG)
			from_addr = i->second.IpAddress();
			printf( "IpMessengerAgentImpl::UdpSendto NIC was found [%s][%s](%d)\n",
					i->second.DeviceName().c_str(),
					i->second.IpAddress().c_str(),
					i->second.PortNo() );fflush( stdout );
#endif
			break;
		}
		//Ʊ�����ɥ쥹�ե��ߥ�Υ����åȤ�ǥե���ȤȤ����Ѥ��롣
		if ( same_family_sock < 0 && addr->ss_family == i->second.AddressFamily() ) {
			same_family_sock = i->first;
			scope = if_nametoindex( i->second.DeviceName().c_str() );
#if defined(DEBUG)
			from_addr = i->second.IpAddress();
#endif
		}
	}
	//���������åȤ�̤����ξ��ǡ�
	if ( sock < 0 ) {
		//Ʊ�����ɥ쥹�ե��ߥ�Υǥե���ȤΥ����åȤ����դ���ʤ���硢
		if ( same_family_sock < 0 ) {
			//�����åȤ���Ƭ�Υ����åȤ��Ѥ��롣�ʥ��顼�ˤʤ��ǽ��ͭ���
			sock = udp_sd[0];
#if defined(DEBUG)
			from_addr = sd_addr.begin()->second.IpAddress();
#endif
		} else {
			//Ʊ�����ɥ쥹�ե��ߥ�Υ����åȤ���Ƭ�Υ����åȤ��Ѥ��롣�ʥ��顼�ˤϤʤ�ʤ�����ã���ʤ������
			sock = same_family_sock;
		}
	}
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::UdpSendto Send Packet [%s]->[%s] scope (%d) port(%d)@Sock %d\n",
				from_addr.c_str(),
				getSockAddrInRawAddress( addr ).c_str(),
				getScopeId( addr ),
				ntohs( getSockAddrInPortNo( addr ) ),
				sock );
	fflush( stdout );
#endif
	int ret = sendToSockAddrIn( sock, buf, size + 1, addr );
	if ( ret <= 0 ) {
		fprintf( stderr, "IpMessengerAgentImpl::UdpSendto Address=[%s] Port=(%d)@Sock=%d(automatic specified for broadcasting) errno=(%d):", getSockAddrInRawAddress( addr ).c_str(), ntohs( getSockAddrInPortNo( addr ) ), sock, errno );fflush( stdout );
		perror("sendto 2.");
#if defined(DEBUG)
		printf("IpMessengerAgentImpl::UdpSendto SEND FAILED\n");fflush( stdout );
#endif
	}
	IPMSG_FUNC_EXIT;
}

/**
 * �����������
 * <ul>
 * <li>�֥��ɥ��㥹�ȥ��ɥ쥹�˴ؤ���UDP�����������</li>
 * <li>�����NIC���Ф���UDP�˴ؤ�������������</li>
 * <li>�����NIC���Ф���TCP�˴ؤ�������������</li>
 * </ul>
 * @param nics �ͥåȥ�����󥿡��ե������Υꥹ�ȡ���Ƭ���ǥե���ȥ����ɤˤʤ�ޤ��� 
 */
void
IpMessengerAgentImpl::InitRecv( const std::vector<NetworkInterface>& nics )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::InitRecv( const std::vector<NetworkInterface>& nics )");
	if ( nics.size() == 0 ) {
		IPMSG_FUNC_EXIT;
	}
	HostAddress = getLocalhostAddress( _UseIPv6, nics );
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::InitRecv localhost IP address=%s\n", HostAddress.c_str() );
#endif
	udp_sd.clear();
	tcp_sd.clear();
	sd_addr.clear();
	sd_address_family.clear();
	for( unsigned int i = 0; i < nics.size(); i++ ){
		struct sockaddr_storage addr;

		if ( createSockAddrIn( &addr, nics[i].IpAddress(), nics[i].PortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}

		int sock = -1;

		sock = InitUdpRecv( addr, nics[i].DeviceName().c_str() );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessgenrAgentImpl::InitRecv UDP Socket Descriptor for unicasting[%d][%s:%s] = %d\n",
							udp_sd.size(),
							getSockAddrInRawAddress( &addr ).c_str(),
							getSockAddrInAddressFamilyString( addr ).c_str(),
							sock );
			fflush( stdout );
#endif
			udp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
			sd_address_family[sock] = addr.ss_family;
		} else {
			printf( "IpMessgenrAgentImpl::InitRecv UDP Error[%s:%s]=%s\n",
							nics[i].DeviceName().c_str(),
							getAddressFamilyString( nics[i].AddressFamily() ).c_str(),
							nics[i].IpAddress().c_str() );
			fflush( stdout );
		}
		sock = InitTcpRecv( addr, nics[i].DeviceName().c_str() );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessgenrAgentImpl::InitRecv TCP Socket Descriptor[%d][%s:%s] = %d\n",
							udp_sd.size(),
							getSockAddrInRawAddress( &addr ).c_str(),
							getSockAddrInAddressFamilyString( addr ).c_str(),
							sock );
			fflush( stdout );
#endif
			tcp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
			sd_address_family[sock] = addr.ss_family;
		} else {
			printf( "IpMessgenrAgentImpl::InitRecv TCP Error[%s:%s]=%s\n",
							nics[i].DeviceName().c_str(),
							getAddressFamilyString( nics[i].AddressFamily() ).c_str(),
							nics[i].IpAddress().c_str() );
			fflush( stdout );
		}

		if ( createSockAddrIn( &addr, nics[i].BroadcastAddress(), nics[i].PortNo(), nics[i].DeviceName().c_str() ) == NULL ) {
			IPMSG_FUNC_EXIT;
		}
		sock = InitUdpRecv( addr, nics[i].DeviceName().c_str() );
		if ( sock > 0 ) {
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessgenrAgentImpl::InitRecv UDP Socket Descriptor for broadcasting[%d][%s:%s] = %d\n",
							udp_sd.size(),
							getSockAddrInRawAddress( &addr ).c_str(),
							getSockAddrInAddressFamilyString( addr ).c_str(),
							sock );
			fflush( stdout );
#endif
			udp_sd.push_back( sock );
			sd_addr[sock] = nics[i];
			sd_address_family[sock] = addr.ss_family;
		} else {
			printf( "IpMessgenrAgentImpl::InitRecv UDP Error[%s:%s]=%s\n",
							nics[i].DeviceName().c_str(),
							getAddressFamilyString( nics[i].AddressFamily() ).c_str(),
							nics[i].IpAddress().c_str() );
			fflush( stdout );
		}
	}
	FD_ZERO( &rfds );

	max_sd = -1;

	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		FD_SET( udp_sd[i], &rfds );
		if ( max_sd < udp_sd[i] ){
			max_sd = udp_sd[i];
		}
	}
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		FD_SET( tcp_sd[i], &rfds );
		if ( max_sd < tcp_sd[i] ){
			max_sd = tcp_sd[i];
		}
	}
	IPMSG_FUNC_EXIT;
}

/**
 * UDP�˴ؤ�������������
 * <ul>
 * <li>2425�Ԥ�������UDP�����åȤ��������</li>
 * <li>UDP��broadcast����</li>
 * </ul>
 * @param addr ��������륢�ɥ쥹����
 * @retval ������ѤߤΥ����å�
 */
int
IpMessengerAgentImpl::InitUdpRecv( struct sockaddr_storage addr, const char *devname )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::InitUdpRecv( struct sockaddr_storage addr, const char *devname )");
	int sock = bindSocket( SOCK_DGRAM, addr, devname );
	if ( sock < 0 ) {
		IPMSG_FUNC_RETURN( -1 );
	}

	int buf_size = MAX_SOCKBUF, buf_minsize = MAX_SOCKBUF / 2;
	if ( setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(sendbuf)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(int)) != 0 &&
		 setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&buf_minsize, sizeof(int)) != 0 ) {
		perror("setsockopt(recvbuf)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}

	IPMSG_FUNC_RETURN( sock );
}

/**
 * TCP�˴ؤ�������������
 * <ul>
 * <li>2425�Ԥ�������TCP�����åȤ��������</li>
 * <li>TCP��REUSEADDR</li>
 * <li>litsen��5�ݡ���</li>
 * </ul>
 * @param addr ��������륢�ɥ쥹����
 * @retval ������ѤߤΥ����å�
 */
int
IpMessengerAgentImpl::InitTcpRecv( struct sockaddr_storage addr, const char *devname )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::InitTcpRecv( struct sockaddr_storage addr, const char *devname )");
	int sock = bindSocket( SOCK_STREAM, addr, devname );
	if ( sock < 0 ) {
		IPMSG_FUNC_RETURN( -1 );
	}

	int yes = 1;
	if ( sock >= 0 && setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) != 0 ) {
		perror("setsockopt(reuseaddr)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( sock >= 0 && listen(sock, 5 ) != 0 ) {
		perror("setsockopt(reuseaddr)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
	IPMSG_FUNC_RETURN( sock );
}

/**
 * ���������ʥ桼�������ˡ�
 * @retval �����ѥ��åȿ�
 */
int
IpMessengerAgentImpl::Process()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::Process()");
	IPMSG_FUNC_RETURN( RecvPacket( true ) );
}

/**
 * ���������������ˡ�
 * <ul>
 * <li>select(�����ॢ�����դ�)�ˤƼ����Ԥ���</li>
 * <li>����������Ԥ����ѥ��åȤ򥭥塼�ˤ�����ࡣ</li>
 * <li>��������λ�����顢���塼����Ȥ�������롣�ʳƥ��٥�Ȥ�ƤӽФ�����</li>
 * <li>������ְ����Υѥ��åȤȽ�ʣȽ���Ԥ�����ʣ���Ƥ�����ϥѥ��åȤ��˴����ޤ���</li>
 * </ul>
 * @retval �����ѥ��åȿ�
 */
int
IpMessengerAgentImpl::RecvPacket( bool isBlock )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::RecvPacket()");
	char buf[MAX_UDPBUF];
	int selret = 1;
	int ret = 0;
	time_t nowTime = time( NULL );

	std::vector<Packet> pack_que;

	while( selret > 0 ) {
		fd_set fds;
		memcpy( &fds, &rfds, sizeof( fd_set ) );
		memset( buf, 0, sizeof( buf ) );

		tv.tv_sec = SELECT_TIMEOUT_SEC;
		tv.tv_usec = SELECT_TIMEOUT_USEC;
//		if ( isBlock ) {
//			selret = select( max_sd + 1, &fds, NULL, NULL, NULL );
//		} else {
			selret = select( max_sd + 1, &fds, NULL, NULL, &tv );
//		}
		if ( selret == -1 ) {
			//select��������ȯ������ä����ϡ�̵�뤷�ޤ���
			if ( errno == EINTR ){
				continue;
			}
			perror( "select()" );
			break;
		} else if ( selret == 0 ){
#if defined(INFO) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::RecvPacket Waiting for next packet.\n");fflush( stdout );
#endif
			break;
		} else {
			int udp_socket = -1;
			int tcp_socket = -1;
#if defined(DEBUG)
			printf("\n");fflush( stdout );
			printf( "IpMessengerAgentImpl::RecvPacket select returns == %d\n\n", selret );fflush( stdout );
#endif
			struct sockaddr_storage sender_addr;
			int sz = sizeof( buf );
			bool recieved = RecvUdp( &fds, &sender_addr, &sz, buf, &udp_socket );
			//UDP�ǥ����åȤ��Ѳ����ʤ���
			tcp_socket = -1;
			if ( !recieved ) {
				sz = sizeof( buf );
				recieved = RecvTcp( &fds, &sender_addr, &sz, buf, &tcp_socket );
				//UDP,TCP�ǥ����åȤ��Ѳ����ʤ���
				if ( !recieved ) {
					continue;
				}
			}
			Packet packet = DismantlePacketBuffer( udp_socket >= 0 ? udp_socket : tcp_socket, buf, sz, sender_addr, nowTime );
			packet.setUdpSocket( udp_socket );
			packet.setTcpSocket( tcp_socket );
			//Ʊ�쥻�å����������ѥ��åȤΰ����������å�����ʣ�ѥ��åȤ�̵��
			if ( !FindDuplicatePacket( packet ) ) {
#if defined(INFO) || !defined(NDEBUG)
				struct sockaddr_storage tempAddr = packet.Addr();	
				IpMsgDumpPacket( packet, &tempAddr );
#endif
				pack_que.push_back( packet );
				PacketsForChecking.push_back( packet );
				ret++;
			}
		}
	}
	// TODO pack_que,PacketsForChecking��deque�Τۤ�����������
	//�ѥ��åȤ�������롣
//	printf("start RecvDoCommand\n");
	while( !pack_que.empty() ) {
//		printf("do RecvDoCommand\n");
		DoRecvCommand( pack_que.front() );
		pack_que.erase( pack_que.begin() );
	}
//	printf("end RecvDoCommand\n");

	//����ʾ����Υ����å��ѤΥѥ��åȥ٥�����ä���
	PurgePacket( nowTime );

	//��å�����������ȥ饤�Υ����å�
	CheckSendMsgRetry( nowTime );

	//�ۥ��ȥꥹ�ȼ����Υ�ȥ饤�����å�
	CheckGetHostListRetry( nowTime );

	IPMSG_FUNC_RETURN( ret );
}

void
IpMessengerAgentImpl::SkulkFromHost( const Packet &packet )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::SkulkHost( Packet packet )");
	//�����ۥ��Ȥ˥���ȥ꡼����Ƥ�����̵�롣
	struct sockaddr_storage pAddr = packet.Addr();
	std::string hideIp = getSockAddrInRawAddress( pAddr );
	std::vector<HostListItem>::iterator hi = FindSkulkHostByAddress( hideIp );
	if ( hi != skulkHostList.end() ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::UdpRecvEventGetPubKey HideFromAddr\n");fflush( stdout );
#endif
		HideFromAddr( pAddr );
		IPMSG_FUNC_EXIT;
	}
	IPMSG_FUNC_EXIT;
}

/**
 * TCP�ѥ��åȤ���������ѥ��åȥ��֥������Ȥ��������롣
 * @param fds FD_SET��¤��
 * @param sender_addr IP���ɥ쥹�Υ��ɥ쥹
 * @param sz �����Хåե��Υ������Υ��ɥ쥹
 * @param buf �����Хåե��Υ��ɥ쥹
 * @param tcp_socket ��������UDP�����å�
 */
bool
IpMessengerAgentImpl::RecvUdp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *udp_socket )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::RecvUdp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf )");
	socklen_t sender_addr_len = 0;
	bool recieved = false;
	int size = *sz;

	//UDP�ǥ����åȤ��Ѳ���ͭ�ä������
	for( unsigned int i = 0; i < udp_sd.size(); i++ ){
		if ( FD_ISSET( udp_sd[i], fds ) ){
			memset( sender_addr, 0, sizeof( struct sockaddr_storage ) );
			sender_addr_len = sizeof( struct sockaddr_storage );
			size = recvfrom( udp_sd[i], buf, size, 0, (struct sockaddr *)sender_addr, &sender_addr_len );
			if ( size < 0 ) {
				perror("recvfrom");
			}
#if defined(DEBUG)
			printf( "IpMessengerAgentImpl::RecvUdp Recieved UDP_SD[%d] == %d[%s]\n",
						i, udp_sd[i], getSockAddrInRawAddress( sender_addr ).c_str() );
			fflush( stdout );
#endif
			IpMsgPrintBuf( "IpMessengerImpl::RecvUdp recvfrom buf", buf, size );
			*udp_socket = udp_sd[i];
			recieved = true;
			break;
		}
	}
	IPMSG_FUNC_RETURN( recieved );
}

/**
 * TCP�ѥ��åȤ���������ѥ��åȥ��֥������Ȥ��������롣
 * @param fds FD_SET��¤��
 * @param sender_addr IP���ɥ쥹�Υ��ɥ쥹
 * @param sz �����Хåե��Υ������Υ��ɥ쥹
 * @param buf �����Хåե��Υ��ɥ쥹
 * @param tcp_socket accept����TCP�����å�
 */
bool
IpMessengerAgentImpl::RecvTcp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *tcp_socket )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::RecvTcp( fd_set *fds, struct sockaddr_storage *sender_addr, int *sz, char *buf, int *tcp_socket )");
	socklen_t sender_addr_len = 0;
	bool recieved = false;
	int size = *sz;

	//TCP�ǥ����åȤ��Ѳ���ͭ�ä������
	for( unsigned int i = 0; i < tcp_sd.size(); i++ ){
		if ( FD_ISSET( tcp_sd[i], fds ) ){
			memset( sender_addr, 0, sizeof( struct sockaddr_storage ) );
			sender_addr_len = sizeof( struct sockaddr_storage );
			*tcp_socket = accept( tcp_sd[i], (struct sockaddr *)sender_addr, &sender_addr_len );
			if ( *tcp_socket < 0 ) {
				perror("accept");
			}
			size = recv( *tcp_socket, buf, size, 0 );
			if ( size < 0 ) {
				perror("recv");
			}
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessengerAgentImpl::RecvTcp Recieved TCP_SD[%d] == %d\n", i, tcp_sd[i] );fflush( stdout );
#endif
			IpMsgPrintBuf( "recv buf", buf, size );
			recieved = true;
			break;
		}
	}
	IPMSG_FUNC_RETURN( recieved );
}

/**
 * ���Υѥ��åȤ����ʣ�ѥ��åȤ򸡺����롣
 * @param packet �ѥ��å�
 * @retval true:¸��
 * @retval false:¸�ߤ��ʤ�
 */
bool
IpMessengerAgentImpl::FindDuplicatePacket( const Packet &packet )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::FindDuplicatePacket( const Packet &packet )");
	//ľ��Υѥ��åȤ���õ����
	for( int i = (int)PacketsForChecking.size() - 1; i >= 0; i-- ){
		if ( PacketsForChecking[i].PacketNo()             == packet.PacketNo() &&
			 isSameSockAddrIn( PacketsForChecking[i].Addr(), packet.Addr() ) ) {
			IPMSG_FUNC_RETURN( true );
		}
	}
	IPMSG_FUNC_RETURN( false );
}

/**
 * ���Υѥ��åȤ���ɬ�פʤ��ʸŤ��˥ѥ��åȤ������롣
 * @param nowTime �����ƥ����
 */
void
IpMessengerAgentImpl::PurgePacket( time_t nowTime )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::PurgePacket( time_t nowTime )");
	for( std::vector<Packet>::iterator pack = PacketsForChecking.begin(); pack != PacketsForChecking.end(); pack++ ){
		if ( nowTime > pack->Recieved() + PACKET_CHECK_FOR_SAVING_INTERVAL ) {
			pack = PacketsForChecking.erase( pack ) - 1;
		} else {
			break;
		}
	}
	IPMSG_FUNC_EXIT;
}

/**
 * ��å����������Υ�ȥ饤��ɬ����������å�����ɬ�פʤ��ȥ饤��Ԥ���
 * @param nowTime �����ƥ����
 */
void
IpMessengerAgentImpl::CheckSendMsgRetry( time_t nowTime )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::CheckSendMsgRetry( time_t nowTime )");
	for( std::vector<SentMessage>::iterator ixmsg = sentMsgList.begin(); ixmsg != sentMsgList.end(); ixmsg++ ) {
		if ( ixmsg->needSendRetry( nowTime ) ) {
			//������
			ixmsg->setRetryCount( ixmsg->RetryCount() + 1 );
			ixmsg->setPrevTry( nowTime );
			SendMsg( ixmsg->Host(),
					 ixmsg->Message(),
					 Secret( ixmsg->IsSecret() ),
					 ixmsg->Files(),
					 LockPassword( ixmsg->IsPasswordLock() ),
					 ixmsg->HostCountAtSameTime(),
					 Logging( !ixmsg->IsNoLogging() ),
					 ixmsg->Opt(),
					 true,
					 ixmsg->PacketNo() );
		}
		if ( ixmsg->isRetryMaxOver() ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::CheckSendMsgRetry Retry Max Over\n");fflush( stdout );
#endif
			ixmsg->setRetryCount( 0 );
			ixmsg->setIsRetryMaxOver( true );
			if ( event != NULL ){
				//��ȥ饤��³�������True�򥻥åȡ�³���ʤ�����False�򥻥åȡ�
				//��RetryMaxOver(��å������ϥ��顼)���֤ˤ���С���³���ޤ����
				//���٥�Ȥ�����ͤ�true:��³��false:���Ǥˤʤ�ޤ���
				event->EventBefore();
#if defined(DEBUG)
				printf("SendRetryError before\n");
#endif
				ixmsg->setIsRetryMaxOver( !event->SendRetryError( *ixmsg ) );
#if defined(DEBUG)
				printf("SendRetryError after\n");
#endif
				event->EventAfter();
			}
			//���٥�ȤǷ�³�����ꤷ�ʤ����ϥ�ȥ饤�ޥå��������С���������롣
		}
	}
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ�ȼ����Υ�ȥ饤��ɬ����������å�����ɬ�פʤ��ȥ饤��Ԥ���
 * @param nowTime �����ƥ����
 */
void
IpMessengerAgentImpl::CheckGetHostListRetry( time_t nowTime )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::CheckGetHostListRetry( time_t nowTime )");

	if ( hostList.IsAsking() ){
		hostList.setPrevTry( time( NULL ) );
		if ( hostList.PrevTry() - hostList.AskStartTime() > GETLIST_RETRY_INTERVAL ) {
			hostList.setAskStartTime( time( NULL ) );
			hostList.setPrevTry( hostList.AskStartTime() );
			hostList.setRetryCount( hostList.RetryCount() + 1 );
			if ( hostList.RetryCount() < GETLIST_RETRY_MAX ) {
				UpdateHostList( true );
			} else {
				hostList.setAskStartTime( 0L );
				hostList.setPrevTry( 0L );
				hostList.setRetryCount( 0 );
				hostList.setIsAsking( false );
				if ( event != NULL ) {
					//��ȥ饤��³�������True�򥻥åȡ�³���ʤ�����False�򥻥åȡ�
					//���٥�Ȥ�����ͤ�true:��³��false:���Ǥˤʤ�ޤ���
					event->EventBefore();
#if defined(DEBUG)
					printf("GetHostListRetryError before\n");
#endif
					hostList.setIsAsking( event->GetHostListRetryError() );
#if defined(DEBUG)
					printf("GetHostListRetryError after\n");
#endif
					event->EventAfter();
				}
			}
		}
	}
	IPMSG_FUNC_EXIT;
}

// Protocol event processor start here.
/**
 * �ѥ��åȤΥ��ޥ�ɥ⡼�ɤǼ������٥�Ȥ򿶤�ʬ���롣
 * @param packet �ѥ��åȥ��֥�������
 */
void
IpMessengerAgentImpl::DoRecvCommand( const Packet& packet )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::DoRecvCommand( const Packet& packet )");
#if defined(DEBUG)
	printf( "IpMessengerAgentImpl::DoRecvCommand [Command %s][From %s][Sock %d]\n", GetCommandString( packet.CommandMode() ).c_str(), getSockAddrInRawAddress( packet.Addr() ).c_str(), packet.UdpSocket() );
	fflush( stdout );
#endif
	switch( packet.CommandMode() ) {
		case IPMSG_NOOPERATION:     UdpRecvEventNoOperation( packet ); break;
		case IPMSG_BR_ENTRY:        UdpRecvEventBrEntry( packet ); break;
		case IPMSG_BR_EXIT:         UdpRecvEventBrExit( packet ); break;
		case IPMSG_ANSENTRY:        UdpRecvEventAnsEntry( packet ); break;
		case IPMSG_BR_ABSENCE:      UdpRecvEventBrAbsence( packet );break;
		case IPMSG_BR_ISGETLIST:    UdpRecvEventBrIsGetList( packet ); break;
		case IPMSG_OKGETLIST:       UdpRecvEventOkGetList( packet ); break;
		case IPMSG_GETLIST:         UdpRecvEventGetList( packet ); break;
		case IPMSG_ANSLIST:         UdpRecvEventAnsList( packet ); break;
		case IPMSG_BR_ISGETLIST2:   UdpRecvEventBrIsGetList2( packet ); break;
		case IPMSG_SENDMSG:         UdpRecvEventSendMsg( packet ); break;
		case IPMSG_RECVMSG:         UdpRecvEventRecvMsg( packet ); break;
		case IPMSG_READMSG:         UdpRecvEventReadMsg( packet); break;
		case IPMSG_DELMSG:          UdpRecvEventDelMsg( packet); break;
		case IPMSG_ANSREADMSG:      UdpRecvEventAnsReadMsg( packet ); break;
		case IPMSG_GETINFO:         UdpRecvEventGetInfo( packet ); break;
		case IPMSG_SENDINFO:        UdpRecvEventSendInfo( packet ); break;
		case IPMSG_GETABSENCEINFO:  UdpRecvEventGetAbsenceInfo( packet ); break;
		case IPMSG_SENDABSENCEINFO: UdpRecvEventSendAbsenceInfo( packet ); break;
		case IPMSG_GETFILEDATA:     TcpRecvEventGetFileData( packet); break;
		case IPMSG_RELEASEFILES:    UdpRecvEventReleaseFiles( packet ); break;
		case IPMSG_GETDIRFILES:     TcpRecvEventGetDirFiles( packet ); break;
		case IPMSG_GETPUBKEY:       UdpRecvEventGetPubKey( packet ); break;
		case IPMSG_ANSPUBKEY:       UdpRecvEventAnsPubKey( packet ); break;
		default:
			fprintf(stderr, "PROTOCOL COMMAND MISS!!(CommandMode = 0x%08lx)\n", packet.CommandMode() );fflush(stderr);
	}
	SkulkFromHost( packet );
	IPMSG_FUNC_EXIT;
}

/**
 * ��ʸ�������٥�ȡ�NOOPERATION
 * <ul>
 * <li>���⤷�ʤ�</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventNoOperation( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventNoOperation( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventNoOperation\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ����(NOOPERATION)
 * <ul>
 * <li>NOOPERATION��������</li>
 * </ul>
 */
int
IpMessengerAgentImpl::SendNoOperation()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::SendNoOperation()");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

	sendBufLen = CreateNewPacketBuffer( IPMSG_NOOPERATION,
										  _LoginName, _HostName,
										  NULL, 0,
										  sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_NOOPERATION, sendBuf, sendBufLen );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�BR_ENTRY
 * <ul>
 * <li>��������ANSENTRY���������롣</li>
 * <li>�Ժߥ⡼�ɤξ�硢�ԺߤȤ���������</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventBrEntry( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventBrEntry( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string optBuf;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrEntry\n");fflush( stdout );
#endif
	if ( _IsAbsence ) {
		std::string AbsenceName = "";
		for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceName = i->AbsenceName();
				break;
			}
		}
		optBuf = Nickname + "[" + AbsenceName + "]";
	} else {
		optBuf = Nickname;
	}
	optBuf += '\0' + GroupName;
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_ANSENTRY ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( -1/*packet.UdpSocket()*/, IPMSG_ANSENTRY, sendBuf, sendBufLen, packet.Addr() );
#ifdef HAVE_OPENSSL
	GetPubKey( packet.Addr() );
#endif
	// �ۥ��ȥꥹ�Ȥ��ɲ�
	int ret = AddHostListFromPacket( packet );
	std::vector<HostListItem>::iterator it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	if ( event != NULL ) {
		event->EventBefore();
		if ( it != appearanceHostList.end() && !it->IsLocalHost() && ret > 0 ) {
#if defined(DEBUG)
			printf("EntryAfter before\n");
#endif
			event->EntryAfter( *it );
#if defined(DEBUG)
			printf("EntryAfter after\n");
#endif
		}
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ������BR_ABSENCE��
 * <ul>
 * <li>�Ժ����Ρ��Ժ߲����ʸ���������롣</li>
 * </ul>
 */
int
IpMessengerAgentImpl::SendAbsence()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::SendAbsence()");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string optBuf;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::SendBrAbsence\n");fflush( stdout );
#endif
	if ( _IsAbsence ) {
		std::string AbsenceName = "";
		for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceName = i->AbsenceName();
				break;
			}
		}
		optBuf = Nickname + "[" + AbsenceName + "]";
	} else {
		optBuf = Nickname;
	}
	optBuf += '\0' + GroupName;
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_BR_ABSENCE ),
										_LoginName, _HostName,
										optBuf.c_str(), optBuf.size(),
										sendBuf, sizeof( sendBuf ) );
	SendBroadcast( IPMSG_BR_ABSENCE, sendBuf, sendBufLen );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�BR_ABSENCE
 * <ul>
 * <li>��ʬ�Υۥ��ȥꥹ�Ȥ򹹿����롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventBrAbsence( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventBrAbsence( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrAbsence\n");fflush( stdout );
#endif
	std::vector<HostListItem>::iterator it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	hostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	hostList.AddHost( HostList::CreateHostListItemFromPacket( packet ) );
	appearanceHostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	int ret = appearanceHostList.AddHost( HostList::CreateHostListItemFromPacket( packet ), false );
#ifdef HAVE_OPENSSL
	GetPubKey( packet.Addr() );
#endif
	if ( event != NULL ){
		it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
		event->EventBefore();
		if ( it != appearanceHostList.end() && ret > 0 ) {
#if defined(DEBUG)
			printf("AbsenceModeChangeAfter before\n");
#endif
			event->AbsenceModeChangeAfter( *it );
#if defined(DEBUG)
			printf("AbsenceModeChangeAfter after\n");
#endif
		}
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�BR_EXIT
 * <ul>
 * <li>��ʬ�Υۥ��ȥꥹ�Ȥ���ۥ��Ȥ������롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventBrExit( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventBrExit( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrExit\n");fflush( stdout );
#endif
	std::vector<HostListItem>::iterator it = appearanceHostList.FindHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	bool isFound = false;
	HostListItem host;
	if ( it != appearanceHostList.end() ) {
		isFound = true;
		host = *it;
	}
	appearanceHostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	hostList.DeleteHostByAddress( getSockAddrInRawAddress( packet.Addr() ) );
	if ( event != NULL ) {
		event->EventBefore();
		if ( isFound ) {
#if defined(DEBUG)
			printf("ExitAfter before\n");
#endif
			event->ExitAfter( host );
#if defined(DEBUG)
			printf("ExitAfter after\n");
#endif
		}
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�RECVMSG
 * <ul>
 * <li>��ʬ�������ѥ�å������ꥹ�Ȥγ�����å������������ѥե饰��Ω�Ƥ롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventRecvMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventRecvMsg( const Packet& packet )");
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packetNo );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsSent( true );
		sentMsg->setRetryCount( 0 );
		sentMsg->setIsRetryMaxOver( true );
		if ( event != NULL ){
			event->EventBefore();
#if defined(DEBUG)
			printf("SendAfter before\n");
#endif
			event->SendAfter( *sentMsg );
#if defined(DEBUG)
			printf("SendAfter after\n");
#endif
			event->EventAfter();
		}
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventRecvMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�READMSG
 * <ul>
 * <li>READCHECKOPT���դ��Ƥ����硢ANSREADMSG���ꤲ�롣</li>
 * <li>��ʬ�������ѥ�å������ꥹ�Ȥγ�����å������˴��ɥե饰��Ω�Ƥ롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventReadMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventReadMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;
	
	if ( packet.CommandOption() & IPMSG_READCHECKOPT ) {
		packetNoBufLen = IpMsgULongToString( packetNoBuf, sizeof( packetNoBuf ), packet.PacketNo() );
		sendBufLen = CreateNewPacketBuffer( IPMSG_ANSREADMSG,
											  _LoginName, _HostName,
											  packetNoBuf, packetNoBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( packet.UdpSocket(), IPMSG_ANSREADMSG, sendBuf, sendBufLen, packet.Addr() );
	}

	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsg->setIsConfirmed( true );
		if ( event != NULL ) {
			event->EventBefore();
#if defined(DEBUG)
			printf("OpenAfter before\n");
#endif
			event->OpenAfter( *sentMsg );
#if defined(DEBUG)
			printf("OpenAfter after\n");
#endif
			event->EventAfter();
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventReadMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�DELMSG
 * <ul>
 * <li>��ʬ�������ѥ�å������ꥹ�Ȥγ�����å�����������</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char *dmyptr;
	unsigned long packet_no = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packet_no );
	if ( sentMsg != sentMsgList.end() ) {
		sentMsgList.erase(sentMsg);
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventDelMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�ANSREADMSG
 * <ul>
 * <li>���⤷�ʤ���</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsReadMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsReadMsg\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�SENDMSG
 * <ul>
 * <li>BROADCASTOPT or AUTORETOPT�ʤ鼫ư�������ʤ���</li>
 * <li>SENDCHECKOPT�դ��ʤ�RECVMSG���ꤲ�롣</li>
 * <li>��ʬ���Ժߤʤ��Ժ߱����򤹤롣</li>
 * <li>�Ź沽��å������ʤ����档</li>
 * <li>�����ѥ�å��������ɲá�</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventSendMsg( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char packetNoBuf[MAX_UDPBUF];
	int packetNoBufLen;

#if defined(DEBUG) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventSendMsg Recieved Message List Count(%d)\n", recvMsgList.size() );fflush( stdout );
#endif
	for( std::vector<RecievedMessage>::iterator ixmsg = recvMsgList.begin(); ixmsg != recvMsgList.end(); ixmsg++ ) {
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::UdpRecvEventSendMsg Searching Recieved Message List..." \
					" Now processing packet numbero = (%ld) packet number in list(%ld)\n",
					packet.PacketNo(), ixmsg->MessagePacket().PacketNo() );fflush( stdout );
#endif
		if ( packet.PacketNo() == ixmsg->MessagePacket().PacketNo() ) {
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::UdpRecvEventSendMsg Message was already added.\n");fflush( stdout );
#endif
			IPMSG_FUNC_RETURN( 0 );
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvSendMsg[Packet = %lu]\n", packet.PacketNo() );fflush( stdout );
#endif
	bool noRaiseEvent = false;
	if ( packet.CommandOption() & IPMSG_BROADCASTOPT ||  packet.CommandOption() & IPMSG_AUTORETOPT ) {
		;
	} else {
		if ( packet.CommandOption() & IPMSG_SENDCHECKOPT ) {
			packetNoBufLen = IpMsgULongToString( packetNoBuf, sizeof( packetNoBuf ), packet.PacketNo() );
			sendBufLen = CreateNewPacketBuffer( IPMSG_RECVMSG,
												  _LoginName, _HostName,
												  packetNoBuf, packetNoBufLen,
												  sendBuf, sizeof( sendBuf ) );
			SendPacket( packet.UdpSocket(), IPMSG_RECVMSG, sendBuf, sendBufLen, packet.Addr() );
		}
		if ( _IsAbsence ) {
			HostListItem host;
			host.setIpAddress( getSockAddrInRawAddress( packet.Addr() ) );
			host.setPortNo( ntohs( getSockAddrInPortNo( packet.Addr() ) ) );
			host.setEncodingName( localEncoding );
			std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( host.IpAddress() );
			if ( hostIt != appearanceHostList.end() ) {
				host = *hostIt;
//				host.setEncodingName( hostIt->EncodingName() );
			}
			std::string AbsenceDescription = "";
			for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
				if ( i->EncodingName() == localEncoding ) {
					AbsenceDescription = i->AbsenceDescription();
					break;
				}
			}
			SendMsg( host, AbsenceDescription.c_str(), Secret::Off(), LockPassword::Off(), 1, Logging::Off() );
		}
	}

	std::string optionMessage = packet.Option();
	if ( packet.CommandOption() & IPMSG_ENCRYPTOPT ){
		if ( !DecryptMsg( packet, optionMessage ) ) {
			HostListItem host;
			host.setIpAddress( getSockAddrInRawAddress( packet.Addr() ) );
			host.setPortNo( ntohs( getSockAddrInPortNo( packet.Addr() ) ) );
			SendMsg( host, DecryptErrorMessage.c_str(), Secret::Off(), LockPassword::Off(), 1, Logging::Off(), IPMSG_AUTORETOPT );
			optionMessage = "";
			//�Ź������Ԥˤ�뼫ư�������ϥ��٥�Ȥ򵯤����ʤ���
			noRaiseEvent = true;
		}
	}
	RecievedMessage message;
	message.setMessagePacket( packet );
	message.setMessage( optionMessage.c_str() );
	message.setRecieved( time( NULL ) );
	message.setIsNoLogging( IPMSG_NOLOGOPT & packet.CommandOption() );
	message.setIsSecret( IPMSG_SECRETOPT & packet.CommandOption() );
	message.setIsCrypted( IPMSG_ENCRYPTOPT & packet.CommandOption() );
	message.setIsPasswordLock( IPMSG_PASSWORDOPT & packet.CommandOption() );
	message.setIsMulticast( IPMSG_MULTICASTOPT & packet.CommandOption() );
	message.setIsBroadcast( IPMSG_BROADCASTOPT & packet.CommandOption() );
	message.setIsConfirmed( false );
	for( std::vector<HostListItem>::iterator ixhost = appearanceHostList.begin(); ixhost != appearanceHostList.end(); ixhost++ ) {
		if ( ixhost->UserName() == packet.UserName() && ixhost->HostName() == packet.HostName() ) {
			message.setHost( *ixhost );
			break;
		}
	}

	message.setHasAttachFile( false );
	AttachFileList files = message.Files();
	if ( CreateAttachedFileList( packet.Option().c_str(), files ) != 0 ) {
		message.setHasAttachFile( true );
	}
	bool eventRet = false;
	message.setFiles( files );
	if ( !noRaiseEvent && event != NULL ) {
		event->EventBefore();
#if defined(DEBUG)
		printf("RecieveAfter before\n");
#endif
		eventRet = event->RecieveAfter( message );
#if defined(DEBUG)
		printf("RecieveAfter after\n");
#endif
		event->EventAfter();
	}
	if ( SaveRecievedMessage() && !eventRet ){
		recvMsgList.append( message );
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�BR_ISGETLIST
 * <ul>
 * <li>OKGETLIST���ꤲ�롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventBrIsGetList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrIsGetList\n");fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_OKGETLIST ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_OKGETLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�BR_ISGETLIST2
 * <ul>
 * <li>OKGETLIST���ꤲ�롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventBrIsGetList2( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventBrIsGetList2\n");fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_OKGETLIST ),
										_LoginName, _HostName,
										NULL, 0,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_OKGETLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�GETLIST
 * <ul>
 * <li>ANSLIST���ꤲ�롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventGetList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	int start = 0;
	char *dmy;
	std::string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetList[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	start = strtoul( packet.Option().c_str(), &dmy, 10 );
	struct sockaddr_storage addr = packet.Addr();
	hosts = hostListBackup.ToString( start, &addr );
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_ANSLIST ),
										_LoginName, _HostName,
										hosts.c_str(), hosts.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_ANSLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�OKGETLIST
 * <ul>
 * <li>GETLIST���ꤲ�롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventOkGetList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string hosts;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventOkGetList[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_GETLIST ),
										_LoginName, _HostName,
										"0", 1,
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_GETLIST, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�ANSENTRY
 * <ul>
 * <li>�ѥ��åȤ���ۥ��ȥꥹ�Ȥ˥ۥ��Ȥξ�����ɲä��롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsEntry( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsEntry\n");fflush( stdout );
#endif
	// �ۥ��ȥꥹ�Ȥ��ɲ�
	AddHostListFromPacket( packet ); 
#ifdef HAVE_OPENSSL
	GetPubKey( packet.Addr() );
#endif
	if ( event != NULL ) {
		event->EventBefore();
#if defined(DEBUG)
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�ANSLIST
 * <ul>
 * <li>�׵�˱������ۥ��ȥꥹ�Ȥ���ʬ��GETLIST�˵ͤ���ꤲ�롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsList( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventDelMsg( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char nextBuf[1024];

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsList\n");fflush( stdout );
#endif
	AddDefaultHost();
	int nextstart = CreateHostList( getSockAddrInRawAddress( packet.Addr() ).c_str(),
									packet.HostName().c_str(),
									packet.Option().c_str(),
									packet.Option().length() );
	if ( nextstart > 0 ) {
		int nextBufLen = IpMsgIntToString( nextBuf, sizeof( nextBuf ), nextstart + 1 );
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::UdpRecvEventDelMsg nextBufLen = %d\n", nextBufLen );fflush( stdout );
#endif
		sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_GETLIST ),
											_LoginName, _HostName,
											nextBuf, nextBufLen,
											sendBuf, sizeof( sendBuf ) );
		SendPacket( packet.UdpSocket(), IPMSG_GETLIST, sendBuf, sendBufLen, packet.Addr() );
	}
	std::string packetIpAddress = getSockAddrInRawAddress( packet.Addr() );
	for( unsigned int i = 0; i < NICs.size(); i++ ){
		if ( packetIpAddress == NICs[i].IpAddress() ){
			IPMSG_FUNC_RETURN( 0 );
		}
	}
	//��ʬ�ʳ�����Υۥ��ȥꥹ�����Τ�����С���ȥ饤��Ϣ�ѿ��򥯥ꥢ��
	hostList.setIsAsking( false );
	hostList.setAskStartTime( 0L );
	hostList.setPrevTry( 0L );
	hostList.setRetryCount( 0 );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�GETINFO
 * <ul>
 * <li>�С����������SENDINFO�˵ͤ���ꤲ�롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventGetInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventGetInfo( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	std::string version = IPMSG_AGENT_VERSION;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetInfo[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_SENDINFO ),
										_LoginName, _HostName,
										version.c_str(), version.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_SENDINFO, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�SENDINFO
 * <ul>
 * <li>���������С����������ۥ��ȥꥹ�Ȥ˹������롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventSendInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventSendInfo( const Packet& packet )");
	std::string pIpAddress = getSockAddrInRawAddress( packet.Addr() );
	std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( pIpAddress );
	if ( hostIt != appearanceHostList.end() ) {
		hostIt->setVersion( packet.Option() );
		if ( event != NULL ){
			event->EventBefore();
#if defined(DEBUG)
			printf("VersionInfoRecieveAfter before\n");
#endif
			event->VersionInfoRecieveAfter( *hostIt, packet.Option() );
#if defined(DEBUG)
			printf("VersionInfoRecieveAfter after\n");
#endif
			event->EventAfter();
		}
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�GETABSENCEINFO
 * <ul>
 * <li>�Ժ߾ܺپ����SENDINFO�˵ͤ���ꤲ�롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo( const Packet& packet )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetAbsenceInfo[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	std::string AbsenceDescription = "";
	if ( _IsAbsence  ){
		std::string IpAddress = getSockAddrInRawAddress( packet.Addr() );
		std::string EncodingName = localEncoding;
		std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( IpAddress );
		if ( hostIt != appearanceHostList.end() ) {
			EncodingName = hostIt->EncodingName();
		}
		for( std::vector<AbsenceMode>::iterator i = absenceModeList.begin(); i != absenceModeList.end(); i++ ){
			if ( i->EncodingName() == localEncoding ) {
				AbsenceDescription = i->AbsenceDescription();
				break;
			}
		}
	} else {
		AbsenceDescription = "Not Absence mode";
	}
	sendBufLen = CreateNewPacketBuffer( AddCommonCommandOption( IPMSG_SENDABSENCEINFO ),
										_LoginName, _HostName,
										AbsenceDescription.c_str(), AbsenceDescription.length(),
										sendBuf, sizeof( sendBuf ) );
	SendPacket( packet.UdpSocket(), IPMSG_SENDABSENCEINFO, sendBuf, sendBufLen, packet.Addr() );
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�SENDABSENCEINFO
 * <ul>
 * <li>���������Ժ߾ܺپ����ۥ��ȥꥹ�Ȥ˹������롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventSendAbsenceInfo( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventSendAbsenceInfo( const Packet& packet )");
	std::string pIpAddress = getSockAddrInRawAddress( packet.Addr() );
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
		hostIt->setAbsenceDescription( packet.Option() );
		if ( event != NULL ){
			event->EventBefore();
#if defined(DEBUG)
			printf("AbsenceDetailRecieveAfter before\n");
#endif
			event->AbsenceDetailRecieveAfter( *hostIt, packet.Option() );
#if defined(DEBUG)
			printf("AbsenceDetailRecieveAfter after\n");
#endif
			event->EventAfter();
		}
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * �ѥ��åȤ��饪�ե��åȤ�������ޤ���
 * <ul>
 * <li>�ѥ��åȤ��饪�ե��åȤ���Ф��֤��ޤ���</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 * @retval �ե����륪�ե��åȡ�
 */
static unsigned long
GetSendFileOffsetInPacket( const Packet& packet )
{
	IPMSG_FUNC_ENTER("static unsigned long GetSendFileOffsetInPacket( const Packet& packet )");
	char *dmyptr;
	char *startptr;
	strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;
	strtoul( startptr, &dmyptr, 16 );
	startptr = ++dmyptr;
	unsigned long offset = strtoul( startptr, &dmyptr, 16 );

	IPMSG_FUNC_RETURN( offset );
}

/**
 * ��ʸ�������٥�ȡ�BR_GETFILEDATA
 * <ul>
 * <li>�ե���������TCP�����åȤˤΤ��������������θ�ե�������������ɤ����롣
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::TcpRecvEventGetFileData( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::TcpRecvEventGetFileData( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::TcpRecvEventGetFileData\n" );fflush( stdout );
#endif

	pthread_t t_id;

	Packet *packetClone = new Packet( packet );

	if ( pthread_create( &t_id, NULL, GetFileDataThread, (void *)packetClone ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_create");
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( pthread_detach( t_id ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_detach");
		IPMSG_FUNC_RETURN( -1 );
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * TODO ������Ρ�
 * ��ʸ�������٥�ȡ�BR_RELEASEFILES
 * <ul>
 * <li> TODO ������Ρ�</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventReleaseFiles( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventReleaseFiles( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::UdpRecvEventReleaseFiles\n" );fflush( stdout );
#endif
	char *dmyptr;
	unsigned long packetNo = strtoul( packet.Option().c_str(), &dmyptr, 10 );
	std::vector<SentMessage>::iterator sentMsg = sentMsgList.FindSentMessageByPacketNo( packetNo );
	if ( sentMsg != sentMsgList.end() ) {
		std::vector<AttachFile>::iterator FoundFile = sentMsg->FindAttachFileByPacket( packet );
		if ( FoundFile == sentMsg->Files().end() ){
			sentMsg->Files().erase( FoundFile );
		}
	}
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�BR_GETPUBKEY
 * <ul>
 * <li>RSA��������ANSPUBKEY�ˤΤ����������롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventGetPubKey( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventGetPubKey( const Packet& packet )");
#ifdef HAVE_OPENSSL
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	int optBufLen;

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventGetPubKey[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	char *dmyptr;
	unsigned long cap = strtoul( packet.Option().c_str(), &dmyptr, 16 );
	RSA *rsa = GetOptimizedRsa( cap );
	if ( rsa != NULL ){
		optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx:%s-%s", encryptionCapacity, BN_bn2hex(rsa->e), BN_bn2hex(rsa->n) );
		sendBufLen = CreateNewPacketBuffer( IPMSG_ANSPUBKEY,
											  _LoginName, _HostName,
											  optBuf, optBufLen,
											  sendBuf, sizeof( sendBuf ) );
		SendPacket( packet.UdpSocket(), IPMSG_ANSPUBKEY, sendBuf, sendBufLen, packet.Addr() );
#ifdef DEBUG
	} else {
		printf("IpMessengerAgentImpl::UdpRecvGetPubKey RSA is NULL[%lu]\n", cap );fflush( stdout );
#endif
	}
#endif
	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�BR_ANSPUBKEY
 * <ul>
 * <li>��������RSA��������ۥ��ȥꥹ�Ȥ˹������롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::UdpRecvEventAnsPubKey( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::UdpRecvEventAnsPubKey( const Packet& packet )");
#ifdef HAVE_OPENSSL
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::UdpRecvEventAnsPubKey[%s]\n", packet.Option().c_str());fflush( stdout );
#endif
	//Option��Hexɽ����
	//XXXXX:EEEEE-NNNNN
	//XXXXX=ǽ�ϥե饰��HEXɽ��
	//EEEEE=RSA���������ʻؿ���
	//NNNNN=RSA�⥸�塼��
	char *opt = (char *)calloc( packet.Option().length() + 1, 1 );
	if ( opt == NULL ){
		IPMSG_FUNC_RETURN( 0 );
	}
	memcpy( opt, packet.Option().c_str(), packet.Option().length() );
	opt[packet.Option().length()] = 0;
	char *nextpos;
	char *token = strtok_r( opt,PACKET_DELIMITER_STRING, &nextpos );
	unsigned long cap = 0UL;
	if ( token != NULL ){
		char *dmyptr;
		cap = strtoul( opt, &dmyptr, 16 );
	} else {
		free( opt );
		IPMSG_FUNC_RETURN( 0 );
	}
	token = nextpos;
	token = strtok_r( token, "-", &nextpos );
	std::string meth;
	if ( nextpos != NULL ) {
		meth = token;
	} else {
		free( opt );
		IPMSG_FUNC_RETURN( 0 );
	}
	std::string pkey;
	if ( token != NULL ) {
		pkey = nextpos;
	} else {
		free( opt );
		IPMSG_FUNC_RETURN( 0 );
	}
	free( opt );
	std::string pIpAddress = getSockAddrInRawAddress( packet.Addr() );
	std::vector<HostListItem>::iterator hostIt = hostList.FindHostByAddress( pIpAddress );
	if ( hostIt != hostList.end() ) {
#ifdef DEBUG
printf( "IpMessengerAgentImpl::UdpRecvEventAnsPubKey Set key [%s][%s]\n", pkey.c_str(), meth.c_str() );
#endif
		hostIt->setEncryptionCapacity( cap );
		hostIt->setPubKeyHex( pkey );
		hostIt->setEncryptMethodHex( meth );
	}
	 hostIt = appearanceHostList.FindHostByAddress( pIpAddress );
	if ( hostIt != appearanceHostList.end() ) {
#ifdef DEBUG
printf( "IpMessengerAgentImpl::UdpRecvEventAnsPubKey appearanceHostList Set key [%s][%s]\n", pkey.c_str(), meth.c_str() );
#endif
		hostIt->setEncryptionCapacity( cap );
		hostIt->setPubKeyHex( pkey );
		hostIt->setEncryptMethodHex( meth );
	}
#endif
	//���٥�Ȥ�󤲤�
	if ( event != NULL ) {
		event->EventBefore();
#if defined(DEBUG)
		printf("UpdateHostListAfter before\n");
#endif
		event->UpdateHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("UpdateHostListAfter after\n");
		printf("RefreshHostListAfter before\n");
#endif
		event->RefreshHostListAfter( appearanceHostList );
#if defined(DEBUG)
		printf("RefreshHostListAfter after\n");
#endif
		event->EventAfter();
	}

	IPMSG_FUNC_RETURN( 0 );
}

/**
 * ��ʸ�������٥�ȡ�GETDIRFILES
 * <ul>
 * <li>�ѥ��åȤǻ��ꤵ�줿�ǥ��쥯�ȥ���������롣</li>
 * </ul>
 * @param packet �ѥ��åȥ��֥�������
 */
int
IpMessengerAgentImpl::TcpRecvEventGetDirFiles( const Packet& packet )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::TcpRecvEventGetDirFiles( const Packet& packet )");
	pthread_t t_id;
	Packet *packetClone = new Packet( packet );

	if ( pthread_create( &t_id, NULL, GetDirFilesThread, (void *)packetClone ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_create");
		IPMSG_FUNC_RETURN( -1 );
	}
	if ( pthread_detach( t_id ) != 0 ){
		perror("TcpRecvEventGetFileData:pthread_detach");
		IPMSG_FUNC_RETURN( -1 );
	}

	IPMSG_FUNC_RETURN( 0 );
}

/**
 * �ե������������ɥ���å�
 * <ul>
 * <li>�ե�������������ɤ����롣</li>
 * </ul>
 * @param param �ѥ��åȥ��֥�������(void*)
 */
void *
ipmsg::GetFileDataThread( void *param )
{
	IPMSG_FUNC_ENTER("void * ipmsg::GetFileDataThread( void *param )");
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::GetFileDataThread\n" );fflush( stdout );
#endif

	Packet *packet = (Packet *)param;

	std::vector<SentMessage>::iterator msg = IpMessengerAgentImpl::GetInstance()->GetSentMessages()->FindSentMessageByPacket( *packet );
	if ( msg == IpMessengerAgentImpl::GetInstance()->GetSentMessages()->end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}
	std::vector<AttachFile>::iterator FoundFile = msg->FindAttachFileByPacket( *packet );
	if ( FoundFile == msg->Files().end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}

	FoundFile->setIsDownloading( true );
	bool ret = IpMessengerAgentImpl::GetInstance()->SendFile( packet->TcpSocket(),
															  FoundFile->FullPath(),
															  FoundFile->MTime(),
															  FoundFile->FileSize(),
															  &(*FoundFile),
															  GetSendFileOffsetInPacket( *packet ) );
	FoundFile->setIsDownloading( false );
	FoundFile->setIsDownloaded( ret );
	close( packet->TcpSocket() );
	delete packet;
	IPMSG_FUNC_RETURN( NULL );
}

/**
 * �ǥ��쥯�ȥ��������ɥ���å�
 * <ul>
 * <li>�ǥ��쥯�ȥ���������ɤ����롣</li>
 * </ul>
 * @param param �ѥ��åȥ��֥�������(void*)
 */
void *
ipmsg::GetDirFilesThread( void *param )
{
	IPMSG_FUNC_ENTER("void * ipmsg::GetDirFilesThread( void *param )");
	Packet *packet = (Packet *)param;
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::TcpRecvEventGetDirFiles\n" );fflush( stdout );
#endif
	std::vector<SentMessage>::iterator msg = myInstance->GetSentMessages()->FindSentMessageByPacket( *packet );
	if ( msg == myInstance->GetSentMessages()->end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}
	std::vector<AttachFile>::iterator FoundFile = msg->FindAttachFileByPacket( *packet );
	if ( FoundFile == msg->Files().end() ){
		close( packet->TcpSocket() );
		delete packet;
		IPMSG_FUNC_RETURN( 0 );
	}

	std::vector<std::string> DownloadFileList;
	FoundFile->setIsDownloading( true );
	bool ret = myInstance->SendDirData( packet->TcpSocket(), FoundFile->FileName(), FoundFile->FullPath(), DownloadFileList );
	FoundFile->setIsDownloading( false );
	FoundFile->setIsDownloaded( ret );
	close( packet->TcpSocket() );
	delete packet;

	IPMSG_FUNC_RETURN( NULL );
}

/**
 * �ǥ��쥯�ȥ�������
 * @param sock TCP�����å�
 * @param cd ���ؤ��Ƥ���ǥ��쥯�ȥ�̾
 * @param dir �ƥǥ��쥯�ȥ�Υե�ѥ�
 * @param files �ե��������
 */
bool
IpMessengerAgentImpl::SendDirData( int sock, std::string cd, std::string dir, std::vector<std::string> &files )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendDirData( int sock, std::string cd, std::string dir, std::vector<std::string> &files )");
	DIR *d= opendir( dir.c_str() );
	struct stat st;
	char headBuf[8192];

	if ( d == NULL ) {
		IPMSG_FUNC_RETURN( false );
	}

	stat( cd.c_str(), &st );
	int headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
														converter->ConvertLocalToNetwork( cd.c_str() ).c_str(),
														(unsigned long long)st.st_size,
														IPMSG_FILE_DIR,
														IPMSG_FILE_MTIME, (long)st.st_mtime,
														IPMSG_FILE_CREATETIME, (long)st.st_ctime );
	snprintf( headBuf, sizeof( headBuf ),"%04x", headBufLen );
	headBuf[4] = ':';
	send( sock, headBuf, headBufLen, 0 );

	char *buf = (char *)calloc( offsetof( struct dirent, d_name ) + pathconf( dir.c_str(), _PC_NAME_MAX ) + 1 , 1 );
	struct dirent *bufdent = ( struct dirent * )buf;
	struct dirent *dent = NULL;
	while( readdir_r( d, bufdent, &dent ) == 0 && dent != NULL ) {
		if ( strcmp(dent->d_name, "." ) != 0 && strcmp(dent->d_name, ".." ) != 0 ) {
			std::string dir_name = dir + "/" + dent->d_name;
#if defined(INFO) || !defined(NDEBUG)
			printf( "IpMessengerAgentImpl::SendDirData dir[%s]", dir_name.c_str() );fflush( stdout );
#endif
			stat( dir_name.c_str(), &st );
			files.push_back( dir_name );
			if ( S_ISDIR( st.st_mode ) ){
#if defined(INFO) || !defined(NDEBUG)
				printf( "IpMessengerAgentImpl::SendDirData DIR\n" );fflush( stdout );
#endif
				if ( !SendDirData( sock, dent->d_name, dir_name, files ) ){
					closedir( d );
					free( buf );
					IPMSG_FUNC_RETURN( false );
				}
			} else {
#if defined(INFO) || !defined(NDEBUG)
				printf( "IpMessengerAgentImpl::SendDirData FILE\n" );fflush( stdout );
#endif
				int headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:%s:%llx:%lx:%lx=%lx:%lx=%lx:",
																	converter->ConvertLocalToNetwork( dent->d_name ).c_str(),
																	(unsigned long long)st.st_size,
																	IPMSG_FILE_REGULAR,
																	IPMSG_FILE_MTIME, (long)st.st_mtime,
																	IPMSG_FILE_CREATETIME, (long)st.st_ctime );
				snprintf( headBuf, sizeof(headBuf),"%04x", headBufLen);
				headBuf[4] = ':';
				send( sock, headBuf, headBufLen, 0 );

				if ( !SendFile( sock, dir_name, st.st_mtime, st.st_size, NULL , 0 ) ){
					closedir( d );
					free( buf );
					IPMSG_FUNC_RETURN( false );
				}
			}
		}
	}
	headBufLen = snprintf( headBuf, sizeof( headBuf ), "0000:.:0:%lx:", IPMSG_FILE_RETPARENT );
	snprintf( headBuf, sizeof(headBuf),"%04x", headBufLen);
	headBuf[4] = ':';
	send( sock, headBuf, headBufLen, 0 );
	closedir( d );
	free( buf );
	IPMSG_FUNC_RETURN( true );
}

/**
 * �ե�����������
 * @param sock TCP�����å�
 * @param FileName �ե�����Υե�ѥ�
 * @param offset ���ե��å�
 * @retval true:����
 * @retval false:����
 */
bool
IpMessengerAgentImpl::SendFile( int sock, std::string FileName, time_t mtime, unsigned long long size, AttachFile *file, off_t offset )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::SendFile( int sock, std::string FileName, time_t mtime, unsigned long long size, AttachFile *file, off_t offset )");
	struct stat statInit;
	unsigned long long transSize = 0LL;
	char realPathName[PATH_MAX];

	memset( realPathName, 0, sizeof( realPathName ) );
	if ( realpath( FileName.c_str(), realPathName ) == NULL ) {
		IPMSG_FUNC_RETURN( false );
	}
	int fd = open( realPathName, O_RDONLY );

	if ( file != NULL ) file->setTransSize( offset );

	if ( fd < 0 ) {
		perror( "open" );
#ifdef DEBUG
		printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\n", FileName.c_str() );fflush(stdout);
#endif
		IPMSG_FUNC_RETURN( false );
	}
	int rc = fstat( fd, &statInit );
	if ( rc != 0 ){
		close( fd );
		IPMSG_FUNC_RETURN( false );
	}
	lseek( fd, offset, SEEK_SET );
	int readSize;
	while( ( readSize = IpMsgSendFileBuffer( fd, sock, 8192 ) ) > 0 ){
		if ( AbortDownloadAtFileChanged() ){
			struct stat statProgress;
			if ( stat( realPathName, &statProgress ) != 0 ){
#ifdef DEBUG
				printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\nFile Changed.\n", FileName.c_str() );fflush(stdout);
#endif
				close( fd );
				IPMSG_FUNC_RETURN( false );
			}
			if ( IsFileChanged( mtime, size, statInit, statProgress ) ){
#ifdef DEBUG
				printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\nFile Changed.\n", FileName.c_str() );fflush(stdout);
#endif
				close( fd );
				IPMSG_FUNC_RETURN( false );
			}
#ifdef DEBUG
			printf("IpMessengerAgentImpl::SendFile FileName.c_str() [%s]\nFile Unchanged.\n", FileName.c_str() );fflush(stdout);
#endif
		}
		transSize += readSize;
		if ( file != NULL ) file->setTransSize( transSize );
	}
	close( fd );
	IPMSG_FUNC_RETURN( true );
}

/**
 * �ե����뤬�������줿����
 * @param mtime ��������
 * @param size �ե����륵����
 * @param statInit �ե�����°���������
 * @param statProgress �ե�����°�����߾���
 * @retval true:�������줿
 * @retval false:��������Ƥ��ʤ�
 */
bool
IpMessengerAgentImpl::IsFileChanged( time_t mtime, unsigned long long size, struct stat statInit, struct stat statProgress )
{
	IPMSG_FUNC_ENTER("bool IpMessengerAgentImpl::IsFileChanged( time_t mtime, unsigned long long size, struct stat statInit, struct stat statProgress )");
	IPMSG_FUNC_RETURN( mtime             != statProgress.st_mtime ||
		   statInit.st_ctime != statProgress.st_ctime ||
		   statInit.st_uid   != statProgress.st_uid   ||
		   statInit.st_gid   != statProgress.st_gid   ||
		   size              != (unsigned long long)statProgress.st_size );
}

/**
 * ��å��������������ѥ��åȥ�å�������ʸ��������'\0'�ʹߤ���ե��������������������롣
 * @param option �ѥ��åȥ��ץ������
 * @param files ź�եե�����ΰ���
 */
int
IpMessengerAgentImpl::CreateAttachedFileList( const char *option, AttachFileList &files )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateAttachedFileList( const char *option, AttachFileList &files )");
	files.clear();
	int filelist_startpos = strlen( option ) + 1;
	int alloc_size = strlen( &option[filelist_startpos] );
	if ( alloc_size == 0 ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	alloc_size++;

	char *file_list_tmp_ptr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *file_list_tmp_buf = (char *)calloc( alloc_size, 1 );
	if ( file_list_tmp_buf == NULL ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	memset( file_list_tmp_buf, 0, alloc_size );
	memcpy( file_list_tmp_buf,  &option[filelist_startpos] , alloc_size - 1 );
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateAttachedFileList File List Buffer = [%s]\n", file_list_tmp_buf);fflush( stdout );
#endif

	IpMsgPrintBuf("CreateAttachedFileList:file_list_tmp_buf",  file_list_tmp_buf, alloc_size );

	// USER NAME(1st)
	file_list_tmp_ptr = file_list_tmp_buf;
	token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	IpMsgPrintBuf("CreateAttachedFileList:file_list_tmp_ptr",  file_list_tmp_ptr, alloc_size );
	IpMsgPrintBuf("CreateAttachedFileList:token",  token, alloc_size );

	while( token != NULL ) {
		bool eob = false;
		while( 1 ) {
			AttachFile file;
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::CreateAttachedFileList AttachFile(-1)\n" );fflush(stdout);
#endif
			// FILE ID
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileId( strtoul( token, &ptrdmy, 10 ) );
#if defined(DEBUG) || !defined(NDEBUG)
			printf( "IpMessengerAgentImpl::CreateAttachedFileList file.FileId() %d token [%s]\n", file.FileId(), token );fflush(stdout);
#endif
			// FILE NAME
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileName( token );
			// FILE SIZE
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setFileSize( strtoul( token, &ptrdmy, 16 ) );
			// MTIME
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setMTime( strtoul( token, &ptrdmy, 16 ) );
			// ATTR
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
			if ( token != NULL && *token == '\a' ) eob = true;
			if ( token == NULL || *token == '\a' ) break;
			file.setAttr( strtoul( token, &ptrdmy, 16 ) );
			while( token != NULL && *token != '\a' ) {
				file_list_tmp_ptr = nextpos;
				token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
				if ( token != NULL && *token == '\a' ) eob = true;
				if ( token == NULL || *token == '\a' ) break;
				int pos = -1;
				for( int i = 0; token[i] != '\0'; i++ ){ 
					if ( token[i] == '=' ) {
						token[i] = '\0';
						pos = i + 1;
						break;
					}
				}
				if ( pos >= 0 ) {
					ptrdmy = &token[pos];
					char *topchar = ptrdmy;
					while( *ptrdmy != '\0' ) {
						file.addExtAttrs( token, strtoul( topchar, &ptrdmy, 16 ) );
						topchar = ++ptrdmy;
					}
				}
			}
#if defined(DEBUG) || !defined(NDEBUG)
			printf("\n\n");fflush(stdout);
			printf("IpMessengerAgentImpl::CreateAttachedFileList == FILE  ==============================>\n");fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList FILE ID[%d]\n", file.FileId());fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList FILE NAME[%s]\n", file.FileName().c_str());fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList FILE SIZE[%lld]\n", file.FileSize());fflush( stdout );
			time_t tt = file.MTime();
			char dmybuf[100];
			printf("IpMessengerAgentImpl::CreateAttachedFileList MTIME[%s]\n", ctime_r( &tt, dmybuf ) );fflush( stdout );
			printf("IpMessengerAgentImpl::CreateAttachedFileList ATTR[%lu]\n", file.Attr() );fflush( stdout );
			for( std::map<std::string, std::vector<unsigned long> >::iterator ixextattr = file.beginExtAttrs(); ixextattr != file.endExtAttrs(); ixextattr++){
				printf("IpMessengerAgentImpl::CreateAttachedFileList EXT ATTR[%s]==", ixextattr->first.c_str() );fflush( stdout );
				for( std::vector<unsigned long>::iterator ixextattrv = ixextattr->second.begin(); ixextattrv != ixextattr->second.end(); ixextattrv++){
					printf("[%lu]", *ixextattrv );fflush( stdout );
				}
				printf("\n" );fflush( stdout );
			}
			printf("IpMessengerAgentImpl::CreateAttachedFileList <= FILE  ===============================\n");fflush( stdout );
#endif
			// ADD FILELIST
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::CreateAttachedFileList AddFile()\n" );fflush( stdout );
#endif
			files.AddFile( file );
			break;
		}
		// FILE ID(not 1st)
		if ( token == NULL ){
#if defined(DEBUG) || !defined(NDEBUG)
			printf("IpMessengerAgentImpl::CreateAttachedFileList File END,break;\n" );fflush( stdout );
#endif
			break;
		}
		if ( *token == '\a' ){
			token++;
		} else {
			file_list_tmp_ptr = nextpos;
			token = strtok_r( file_list_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
		}
	}
	free( file_list_tmp_buf );
	IPMSG_FUNC_RETURN( files.size() );
}

/**
 * �ۥ��ȥꥹ�ȼ��������ѥ��åȥ��ץ�������ʥХåե��ˤ���ۥ��Ȱ���������������롣
 * @param hostListBuf �Хåե�
 * @param buf_len �Хåե���Ĺ��
 */
int
IpMessengerAgentImpl::CreateHostList( const char *packetIpAddress, const char *packetHostName, const char *hostListBuf, int buf_len )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateHostList( const char *packetIpAddress, const char *packetHostName, const char *hostListBuf, int buf_len )");
	int alloc_size = buf_len + 1;
	int add_count = 0;
	char *hostListTmpPtr;
	char *nextpos;
	char *token;
	char *ptrdmy;
	char *hostListTmpBuf = (char *)calloc( alloc_size, 1 );

#if defined(DEBUG) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateHostList packetIpAddress=[%s] packetHostName[%s]\n", packetIpAddress, packetHostName );fflush(stdout);
	IpMsgPrintBuf( "hostListBuf", hostListBuf, buf_len );
#endif
	AddDefaultHost();
	if ( hostListTmpBuf == NULL ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	memset( hostListTmpBuf, 0, alloc_size );
	memcpy( hostListTmpBuf, hostListBuf, buf_len );
	hostListTmpPtr = hostListTmpBuf;
	// CONTINUE POSITION
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );
	if ( token == NULL ) {
		free( hostListTmpBuf );
		IPMSG_FUNC_RETURN( 0 );
	}
	// LIST COUNTS
	hostListTmpPtr = nextpos;
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );
	if ( token == NULL ) {
		free( hostListTmpBuf );
		IPMSG_FUNC_RETURN( 0 );
	}
	// USER NAME(1st)
	hostListTmpPtr = nextpos;
	token = strtok_r( hostListTmpPtr, "\a", &nextpos );

	while( token != NULL ) {
		HostListItem item;
		item.setVersion( "" );
		item.setAbsenceDescription( "" );
		item.setUserName( "" );
		item.setHostName( "" );
		item.setCommandNo( 0UL );
		item.setIpAddress( "" );
		item.setNickname( "" );
		item.setGroupName( "" );
		item.setEncodingName( "" );
		item.setPriority( "" );
		item.setPortNo( 0UL );
		item.setEncryptionCapacity( 0UL );
		item.setPubKeyHex( "" );
		item.setEncryptMethodHex( "" );
		// USER NAME
		if ( *token == '\b' ) {
			item.setUserName( "" );
			//'\b'�ȶ��ڤ�ʸ��'\a'��ʬ�����Ф���
			token += 2;
			nextpos = token;
		} else {
			item.setUserName( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// HOST NAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setHostName( "" );
			//'\b'�ȶ��ڤ�ʸ��'\a'��ʬ�����Ф���
			token += 2;
			nextpos = token;
		} else {
			item.setHostName( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// CommandNo
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setCommandNo( 0L );
			//'\b'�ȶ��ڤ�ʸ��'\a'��ʬ�����Ф���
			token += 2;
			nextpos = token;
		} else {
			item.setCommandNo( strtoul( token, &ptrdmy, 10 ) );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// IP ADDRESS
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setIpAddress( "" );
			//'\b'�ȶ��ڤ�ʸ��'\a'��ʬ�����Ф���
			token += 2;
			nextpos = token;
		} else {
			//ANSLIST�������Ƥ���ۥ��ȥꥹ�Ȥ�IP���ɥ쥹���롼�ץХå��ξ�礬ͭ�롣��IP��å��󥸥㡼�ΥХ��ʤΤ��ʡ���
			if ( strcmp( token, "127.0.0.1" ) == 0 ){
				//�ѥ��åȤ����������ۥ��Ȥ�IP���ɥ쥹���롼�ץХå��ξ��ϥѥ��å����ո���IP���ɥ쥹�����ꤹ�롣
				if ( item.HostName() == packetHostName ) {
					item.setIpAddress( packetIpAddress );
				} else {
					//�����Ǥʤ����Ϥ������롣��AddHost�᥽�å����̵�뤵��ۥ��ȥꥹ�Ȥ��ɲä���ʤ�����
					item.setIpAddress( token );
				}
			} else {
				//������롼�ץХå����ɥ쥹�Ǥ�̵�����Ϥ��Τޤ����ꤹ�롣
				item.setIpAddress( token );
			}
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// PORTNO
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setPortNo( 0L );
			//'\b'�ȶ��ڤ�ʸ��'\a'��ʬ�����Ф���
			token += 2;
			nextpos = token;
		} else {
			item.setPortNo( ntohs( strtoul( token, &ptrdmy, 10 ) ) );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// NICKNAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setNickname( "" );
			//'\b'�ȶ��ڤ�ʸ��'\a'��ʬ�����Ф���
			token += 2;
			nextpos = token;
		} else {
			item.setNickname( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		if ( token == NULL ) break;
		// GROUPNAME
		hostListTmpPtr = nextpos;
		if ( *token == '\b' ) {
			item.setGroupName( "" );
			//'\b'�ȶ��ڤ�ʸ��'\a'��ʬ�����Ф���
			token += 2;
			nextpos = token;
		} else {
			item.setGroupName( token );
		}
		hostListTmpPtr = nextpos;
		token = strtok_r( hostListTmpPtr, "\a", &nextpos );
		//�Ǹ�Υȡ�����ϺǸ��Ƚ�ꤹ�롣(A)��ʬ��
		// ADD HOSTLIST
		hostList.DeleteHostByAddress( item.IpAddress() );
		hostList.AddHost( item );
		appearanceHostList.DeleteHostByAddress( item.IpAddress() );
		appearanceHostList.AddHost( item, false );

#ifdef HAVE_OPENSSL
		struct sockaddr_storage addr;
		if ( createSockAddrIn( &addr, item.IpAddress(), item.PortNo() ) == NULL ) {
			IPMSG_FUNC_RETURN( add_count );
		}
		GetPubKey( addr );
#endif
		//(A)�Ǹ�Υȡ�����ϺǸ��Ƚ�ꤹ�롣(A)
		if ( token == NULL ) break;
		add_count++;
	}
	free( hostListTmpBuf );
	IPMSG_FUNC_RETURN( add_count );
}

#ifdef HAVE_OPENSSL
void
IpMessengerAgentImpl::GetPubKey( const struct sockaddr_storage &address )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::GetPubKey( const struct sockaddr_storage &address )");
	char sendBuf[MAX_UDPBUF];
	int sendBufLen;
	char optBuf[MAX_UDPBUF];
	size_t optBufLen;
	optBufLen = snprintf( optBuf, sizeof( optBuf ), "%lx", encryptionCapacity );
	if ( optBufLen >= sizeof(optBuf) ){
		optBufLen = sizeof(optBuf);
	}
	sendBufLen = CreateNewPacketBuffer( IPMSG_GETPUBKEY,
										  _LoginName, _HostName,
										  optBuf, optBufLen,
										  sendBuf, sizeof( sendBuf ) );
	SendPacket( -1, IPMSG_GETPUBKEY, sendBuf, sendBufLen, address );
	IPMSG_FUNC_EXIT;
}
#endif

/**
 * ������å������θĿ���������롣
 * @retval ������å������θĿ�
 */
int
IpMessengerAgentImpl::GetRecievedMessageCount()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::GetRecievedMessageCount()");
	IPMSG_FUNC_RETURN( recvMsgList.size() );
}

/**
 * ������å��������ļ��Ф���������å������ꥹ�Ȥ��������롣
 * @retval ������å��������֥������ȡ�
 */
RecievedMessage
IpMessengerAgentImpl::PopRecievedMessage()
{
	IPMSG_FUNC_ENTER("RecievedMessage IpMessengerAgentImpl::PopRecievedMessage()");
	RecievedMessage ret;
	for( std::vector<RecievedMessage>::iterator ix = recvMsgList.begin(); ix != recvMsgList.end(); ix++ ){
		ret = *ix;
		recvMsgList.erase( ix );
		break;
	}
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �����ѥ�å������ꥹ�ȤΥݥ��󥿤�������롣
 * @retval �����ѥ�å������ꥹ�ȤΥݥ��󥿡�
 */
SentMessageList *
IpMessengerAgentImpl::GetSentMessages()
{
	IPMSG_FUNC_ENTER("SentMessageList *IpMessengerAgentImpl::GetSentMessages()");
	IPMSG_FUNC_RETURN( &sentMsgList );
}

/**
 * �����ѥ�å������ꥹ�ȤΥ��ԡ���������롣
 * @retval �����ѥ�å������ꥹ�ȤΥ��ԡ���
 */
SentMessageList
IpMessengerAgentImpl::CloneSentMessages() const
{
	IPMSG_FUNC_ENTER("SentMessageList IpMessengerAgentImpl::CloneSentMessages() const");
	IPMSG_FUNC_RETURN( sentMsgList );
}

/**
 * ���ץ�������κ���Ĺ��������롣
 * @retval ���ץ�����������Ƥ���Хåե���Ĺ��
 */
int
IpMessengerAgentImpl::GetMaxOptionBufferSize()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::GetMaxOptionBufferSize()");
	char tmp[MAX_UDPBUF];
	int headSize = snprintf(tmp, sizeof(tmp), "%d:0000000000:%s:%s:0000000000:", IPMSG_VERSION, _LoginName.c_str(), _HostName.c_str() );
	int ret = MAX_UDPBUF - headSize;
	IPMSG_FUNC_RETURN( ret < 0 ? 0 : ret );
}

/**
 * ���̥��ޥ�ɥ��ץ������ɲä��롣
 * @param �ɲ����Υ��ޥ�ɥ��ץ����
 * @retval �ɲø�Υ��ޥ�ɥ��ץ����
 */
unsigned long
IpMessengerAgentImpl::AddCommonCommandOption( const unsigned long cmd )
{
	IPMSG_FUNC_ENTER("unsigned long IpMessengerAgentImpl::AddCommonCommandOption( const unsigned long cmd )");
	unsigned long ret = cmd | IPMSG_FILEATTACHOPT
#ifdef HAVE_OPENSSL
							| ( encryptionCapacity != 0UL ?  IPMSG_ENCRYPTOPT : 0UL )
#endif	//HAVE_OPENSSL
							| ( IsAbsence() ? IPMSG_ABSENCEOPT : 0UL ) | ( IsDialup() ? IPMSG_DIALUPOPT : 0UL );
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::AddCommonCommandOption <<=================================================\n");fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption encryptionCapacity=%lu\n", encryptionCapacity );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption IsAbsence=%s\n", IsAbsence() ? "true" : "false" );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption IsDialup=%s\n", IsDialup() ? "true" : "false" );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption Option=%lu\n", ret );fflush( stdout );
	printf( "IpMessengerAgentImpl::AddCommonCommandOption =================================================>>\n");fflush( stdout );
#endif
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �����ѥХåե���������롣
 * @param cmd ���ޥ��
 * @param packetNo �ѥ��å��ֹ�
 * @param user ���Υۥ��ȤΥ桼��̾
 * @param host ���Υۥ��ȤΥۥ���̾
 * @param opt Ϣ�뤹�륪�ץ����ʸ����
 * @param optLen ���ץ�����Ĺ��
 * @param buf �����Хåե�
 * @param size �����Хåե��κ��祵����
 * @retval �����Хåե���Ĺ��
 */
int
IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, unsigned long packetNo, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, unsigned long packetNo, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateNewPacketBuffer()\n" );fflush(stdout);
#endif
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::CreateNewPacketBuffer Packet Command[%s]\n", GetCommandString( GET_MODE( cmd ) ).c_str() );fflush( stdout );
#endif
	memset( buf, 0, size );
	//Version:PacketNo:UserName:HostName:Command[:Option]
	int send_size = snprintf(buf, size, "%d:%ld:%s:%s:%ld:",
										IPMSG_VERSION,
										packetNo,
										user == "" ? "\b" : user.c_str(),
										host == "" ? "\b" : host.c_str(),
										cmd );
	if ( send_size > size ) {
		IPMSG_FUNC_RETURN( 0 );
	}
	if ( send_size + optLen < size && optLen > 0 && opt != NULL ) {
		memcpy(&buf[send_size], opt, optLen );
	} else {
		optLen = 0;
	}
	IPMSG_FUNC_RETURN( send_size + optLen );
}

/**
 * �����ѥХåե���������롣(�ѥ��å��ֹ漫ư������)
 * @param cmd ���ޥ��
 * @param user ���Υۥ��ȤΥ桼��̾
 * @param host ���Υۥ��ȤΥۥ���̾
 * @param opt Ϣ�뤹�륪�ץ����ʸ����
 * @param optLen ���ץ�����Ĺ��
 * @param buf �����Хåե�
 * @param size �����Хåե��κ��祵����
 * @retval �����Хåե���Ĺ��
 */
int
IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::CreateNewPacketBuffer( unsigned long cmd, std::string user, std::string host, const char *opt, int optLen, char *buf, int size )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::CreateNewPacketBuffer()\n" );fflush(stdout);
#endif
	unsigned long packetNo = random();
	IPMSG_FUNC_RETURN( CreateNewPacketBuffer(cmd, packetNo, user, host, opt, optLen, buf, size ) );
}

/**
 * �����Хåե�����ѥ��åȥ��֥������Ȥ��������롣
 * @param packet_buf �����Хåե�
 * @param size �����Хåե��Υ�����
 * @param sender ���������ɥ쥹
 * @retval �ѥ��åȥ��֥�������
 */
Packet
IpMessengerAgentImpl::DismantlePacketBuffer( int sock, char *packet_buf, int size, struct sockaddr_storage sender, time_t nowTime )
{
	IPMSG_FUNC_ENTER("Packet IpMessengerAgentImpl::DismantlePacketBuffer( char *packet_buf, int size, struct sockaddr_storage sender, time_t nowTime )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::DismantlePacketBuffer()\n" );fflush(stdout);
#endif
	Packet ret;
	int alloc_size = size + 1;
	char *packet_tmp_buf;
	char *packet_tmp_ptr;
	char *nextpos;
	char *token;
	char *ptrdmy;

	ret.setRecieved( nowTime );
	packet_tmp_buf = (char *)calloc( alloc_size, 1 );
	if ( packet_tmp_buf == NULL ) {
		IPMSG_FUNC_RETURN( ret );
	}
	memset( packet_tmp_buf, 0, alloc_size );
	memcpy( packet_tmp_buf, packet_buf, size );
	//VERSION NUMBER
	packet_tmp_ptr = packet_tmp_buf;
	token = strtok_r( packet_tmp_buf, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setVersionNo( strtoul( token, &ptrdmy, 10 ) );

	//PACKET NUMBER
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setPacketNo( strtoul( token, &ptrdmy, 10 ) );

	//USER
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setUserName( token );

	//HOST
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	ret.setHostName( token );

	//COMMAND
	packet_tmp_ptr = nextpos;
	token = strtok_r( packet_tmp_ptr, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( packet_tmp_buf );
		IPMSG_FUNC_RETURN( ret );
	}
	unsigned long command = strtoul( token, &ptrdmy, 10 ); 
	ret.setCommandMode( GET_MODE(command) );
	ret.setCommandOption( GET_OPT(command) );

	//OPTION
	int optLen = size - ( nextpos - packet_tmp_buf );
	ret.setOption( std::string( nextpos, optLen ) );
	free( packet_tmp_buf );

	//NAT�Ķ���sender���ɥ쥹�Ͽ��ѤǤ��ʤ��Τǡ�����
	//�ޤ��ϸ��Ƥ���Υۥ��ȥꥹ�Ȥ��鸡�����롣
	std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByHostName( ret.HostName(), sd_address_family[sock] );
	struct sockaddr_storage hostaddr;
	if ( hostIt != appearanceHostList.end() ) {
		if ( createSockAddrIn( &hostaddr, hostIt->IpAddress(), hostIt->PortNo() ) == NULL ) {
			IPMSG_FUNC_RETURN( ret );
		}
	} else {
		//̵����мºݤΤΥۥ��ȥꥹ�Ȥ��鸡�����롣
		hostIt = hostList.FindHostByHostName( ret.HostName(), sd_address_family[sock] );
		if ( hostIt != hostList.end() ) {
			if ( createSockAddrIn( &hostaddr, hostIt->IpAddress(), hostIt->PortNo() ) == NULL ) {
				IPMSG_FUNC_RETURN( ret );
			}
		} else {
			hostaddr = sender;
#if 0
			if ( createSockAddrIn( &hostaddr, getSockAddrInRawAddress( sender ), ntohs( getSockAddrInPortNo( sender ) ) ) == NULL ) {
				IPMSG_FUNC_RETURN( ret );
			}
#endif
		}
	}
	ret.setAddr( hostaddr );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ѥ��åȤ���ۥ��ȥꥹ�Ȥ˲ä��롣
 * @param packet �ѥ��åȥ��֥�������
 * @retval ��Ͽ���������
 */
int
IpMessengerAgentImpl::AddHostListFromPacket( const Packet& packet )
{
	IPMSG_FUNC_ENTER("void IpMessengerAgentImpl::AddHostListFromPacket( const Packet& packet )");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddHostListFromPacket()\n" );fflush(stdout);
#endif
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddHostListFromPacket ===================================\n");fflush( stdout );
	struct sockaddr_storage tempAddr = packet.Addr();
	IpMsgDumpPacket( packet, &tempAddr );
	printf("IpMessengerAgentImpl::AddHostListFromPacket ===================================\n");fflush( stdout );
#endif
	AddDefaultHost();
	// �ǥե���Ȥ�NIC(������)�ʳ��μ�ʬ���Ȥ�IP���ɥ쥹����Ͽ���ꤵ�줿��̵�롣
	std::string packetIpAddress = getSockAddrInRawAddress( packet.Addr() );
	for( unsigned int i = 1; i < NICs.size(); i++ ){
		if ( packetIpAddress == NICs[i].IpAddress() ){
			AddDefaultHost();
			IPMSG_FUNC_RETURN( 0 );
		}
	}
	//�ǥե���ȥ�����
	HostListItem item;
	item.setUserName( packet.UserName() );
	item.setHostName( packet.HostName() );
	item.setCommandNo( packet.CommandOption() );
	item.setIpAddress( getSockAddrInRawAddress( packet.Addr() ) );
	int NicknameLen = strlen( packet.Option().c_str() );
	item.setNickname( packet.Option().c_str() );
	item.setGroupName( packet.Option().c_str() + NicknameLen + 1 );
	item.setEncodingName( "" );
	item.setPriority( "" );
	item.setPortNo( ntohs( getSockAddrInPortNo( packet.Addr() ) ) );
	item.setEncryptionCapacity( 0UL );
	item.setPubKeyHex( "" );
	item.setEncryptMethodHex( "" );
	hostList.AddHost( item );
	int ret = appearanceHostList.AddHost( item, false );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * ǰ�Τ���ۥ��ȥꥹ�Ȥ˼�ʬ��ä��Ƥ�����
 * @retval ��Ͽ�����ۥ��ȿ���
 */
int
IpMessengerAgentImpl::AddDefaultHost()
{
	IPMSG_FUNC_ENTER("int IpMessengerAgentImpl::AddDefaultHost()");
#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::AddDefaultHost()\n" );fflush(stdout);
	printf("IpMessengerAgentImpl::AddDefaultHost Nickname=[%s]\n", Nickname.c_str() );fflush(stdout);
	printf("IpMessengerAgentImpl::AddDefaultHost GroupName=[%s]\n", GroupName.c_str() );fflush(stdout);
#endif
	std::vector<HostListItem>::iterator hostIt = appearanceHostList.FindHostByAddress( HostAddress );
	if ( hostIt == appearanceHostList.end() ) {
		HostListItem myHost;
		myHost.setUserName( _LoginName );
		myHost.setHostName( _HostName );
		myHost.setCommandNo( AddCommonCommandOption( 0UL ) );
		myHost.setIpAddress( HostAddress );
		myHost.setNickname( Nickname );
		myHost.setGroupName( GroupName );
		myHost.setPortNo( DefaultPortNo() );
		hostList.AddHost( myHost );
		appearanceHostList.AddHost( myHost, false );
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::AddDefaultHost MyHost[%s] Add.[%s][%s]\n", HostAddress.c_str(), myHost.UserName().c_str(), myHost.GroupName().c_str() );fflush( stdout );
#endif
		IPMSG_FUNC_RETURN( 1 );
	}
	IPMSG_FUNC_RETURN( 0 );
}
//end of source
