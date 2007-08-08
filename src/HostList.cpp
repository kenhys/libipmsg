/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * �ۥ��ȥꥹ�ȥ��饹��
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
#include <algorithm>

using namespace ipmsg;

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * ���󥹥ȥ饯����
 * <ul>
 * <li>�ۥ��ȥꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 */
HostList::HostList()
{
	IPMSG_FUNC_ENTER( "HostList::HostList()" );
	IpMsgMutexInit( "HostList::HostList()", &hostListMutex, NULL );
	IPMSG_FUNC_EXIT;
}

/**
 * ���ԡ����󥹥ȥ饯����
 * <ul>
 * <li>�ۥ��ȥꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 * @param other ���ԡ����Υ��֥�������
 */
HostList::HostList( const HostList& other )
{
	IPMSG_FUNC_ENTER( "HostList::HostList( const HostList& other )" );
	IpMsgMutexInit( "HostList::HostList(HostList&)", &hostListMutex, NULL );
	Lock( "HostList::HostList(HostList&)" );
	CopyFrom( other );
	Unlock( "HostList::HostList(HostList&)" );
	IPMSG_FUNC_EXIT;
}

/**
 * �ǥ��ȥ饯����
 * <ul>
 * <li>�ۥ��ȥꥹ�Ȥ��å����뤿��Υߥ塼�ƥå������˴���</li>
 * </ul>
 */
HostList::~HostList()
{
	IPMSG_FUNC_ENTER( "HostList::~HostList()" );
	IpMsgMutexDestroy( "HostList::~HostList()", &hostListMutex );
	IPMSG_FUNC_EXIT;
}

/**
 * �����黻�ҡ�
 * <ul>
 * <li>�ۥ��ȥꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������</li>
 * </ul>
 * @param other ���ԡ����Υ��֥�������
 * @retval �����֥������ȤΥ��󥹥���
 */
HostList&
HostList::operator=( const HostList& other )
{
	IPMSG_FUNC_ENTER( "HostList& HostList::operator=( const HostList& other )" );
	IpMsgMutexInit( "HostList::operator=(HostList&)", &hostListMutex, NULL );
	Lock( "HostList::operator=(HostList&)" );
	CopyFrom( other );
	Unlock( "HostList::operator=(HostList&)" );
	IPMSG_FUNC_RETURN( *this );
}

/**
 * ���ԡ��᥽�åɡ�
 * @param other ���ԡ����Υ��֥�������
 */
void
HostList::CopyFrom( const HostList& other )
{
	IPMSG_FUNC_ENTER( "void HostList::CopyFrom( const HostList& other )" );
	items = other.items;
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ�Ȥ��å�
 * @param pos ��å����Ƥ�����֤򼨤�ʸ����
 */
void
HostList::Lock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void HostList::Lock( const char *pos ) const" );
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &hostListMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ�Ȥ򥢥��å�
 * @param pos �����å����Ƥ�����֤򼨤�ʸ����
 */
void
HostList::Unlock( const char *pos ) const
{
	IPMSG_FUNC_ENTER( "void HostList::Unlock( const char *pos ) const" );
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &hostListMutex ) );
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ�Ȥ���Ƭ�򼨤����ƥ졼�����֤���
 * @retval �ۥ��ȥꥹ�Ȥ���Ƭ�򼨤����ƥ졼����
 */
std::vector<HostListItem>::iterator
HostList::begin()
{
	IPMSG_FUNC_ENTER( "std::vector<HostListItem>::iterator HostList::begin()" );
	IPMSG_FUNC_RETURN( items.begin() );
}

/**
 * �ۥ��ȥꥹ�Ȥ������ܣ��򼨤����ƥ졼�����֤���
 * @retval �ۥ��ȥꥹ�Ȥ������ܣ��򼨤����ƥ졼����
 */
std::vector<HostListItem>::iterator
HostList::end()
{
	IPMSG_FUNC_ENTER( "std::vector<HostListItem>::iterator HostList::end()" );
	IPMSG_FUNC_RETURN(  items.end() );
}

/**
 * �ۥ��ȥꥹ�ȤθĿ����֤���
 * @retval �ۥ��ȥꥹ�ȤθĿ���
 */
int
HostList::size() const
{
	IPMSG_FUNC_ENTER( "int HostList::size() const" );
	Lock( "HostList::size()" );
	int ret = items.size();
	Unlock( "HostList::size()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ۥ��ȥꥹ�Ȥ򥯥ꥢ���롣
 */
void
HostList::clear()
{
	IPMSG_FUNC_ENTER( "void HostList::clear()" );
	Lock( "HostList::clear()" );
	items.clear();
	Unlock( "HostList::clear()" );
	IPMSG_FUNC_EXIT;
}

/**
 * �С����������䤤��碌��Ԥ���
 */
void
HostListItem::QueryVersionInfo()
{
	IPMSG_FUNC_ENTER( "void HostListItem::QueryVersionInfo()" );
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryVersionInfo( *this );
	IPMSG_FUNC_EXIT;
}

/**
 * �Ժ�����ʸ�����䤤��碌��Ԥ���
 */
void
HostListItem::QueryAbsenceInfo()
{
	IPMSG_FUNC_ENTER( "void HostListItem::QueryAbsenceInfo()" );
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryAbsenceInfo( *this );
	IPMSG_FUNC_EXIT;
}

/**
 * IP���ɥ쥹�򸵤˥�����ۥ��Ȥ��ɤ�������롣
 * @retval true:������ۥ���
 * @retval false:������ۥ��ȤǤϤʤ�
 */
bool
HostListItem::IsLocalHost() const
{
	IPMSG_FUNC_ENTER( "bool HostListItem::IsLocalHost() const" );
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	std::vector<NetworkInterface> nics = agent->NICs;
	for( unsigned int i = 0; i < nics.size(); i++ ){
		if ( IpAddress() == nics[i].IpAddress() ){
#if defined(INFO) || !defined(NDEBUG)
			printf("This host item is localhost.\n");
			fflush(stdout);
#endif
			IPMSG_FUNC_RETURN( true );
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("This host item is not localhost.\n");
	fflush(stdout);
#endif
	IPMSG_FUNC_RETURN( false );
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��ɲä��롣
 * @param host �ۥ��Ⱦ���
 * @retval ��Ͽ�������
 */
int
HostList::AddHost( const HostListItem& host, bool isPermitSameHardwareAddress )
{
	IPMSG_FUNC_ENTER( "void HostList::AddHost( const HostListItem& host, bool isPermitSameHardwareAddress )" );
	Lock( "HostList::AddHost()" );
	bool is_found = false;

#if defined(INFO) || !defined(NDEBUG)
	printf("HostList::AddHost enter. host.IpAddress()=%s host.AddressFamily()=%s\n",
							host.IpAddress().c_str(),
							host.AddressFamily() == AF_INET6 ? "AF_INET6" : "AF_INET" );
	fflush(stdout);
#endif

	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	std::string localhostName = agent->HostName();
	std::vector<NetworkInterface> nics = agent->NICs;
	//��Ƭ�Υ������åȥ��ɥ쥹�ե��ߥ��NIC��õ���ץ饤�ޥ�(�����ɥ쥹)�Ȥ��ư����١����θ����õ������Υ���ǥå�������롣
	int nicStartIndex = 1;
	if ( !agent->haveIPv4Nic && !agent->haveIPv6Nic ) {
			IPMSG_FUNC_RETURN( 0 );
	} else if ( agent->haveIPv4Nic && agent->haveIPv6Nic && host.AddressFamily() == AF_INET6 ) {
		//IPv4,IPv6��ξ��Ȥ��ʤ�IPv6��ͥ�衣
		for( unsigned int i = 0; i < nics.size(); i++ ){
			if ( nics[i].AddressFamily() == AF_INET6 ) {
				nicStartIndex = i + 1;
				break;
			}
		}
	} else {
		nicStartIndex = 1;
	}
	//õ��������ǥå������֤��饹������
	for( unsigned int i = nicStartIndex; i < nics.size(); i++ ){
#if defined(INFO) || !defined(NDEBUG)
		printf("HostList::AddHost now host checking IpAddress=%s NIC[%d] IpAddress=%s\n", host.IpAddress().c_str(), i, nics[i].IpAddress().c_str() );
		fflush(stdout);
#endif

		if ( host.IpAddress() == nics[i].IpAddress() ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("HostList::AddHost Host IP Address is match NIC IP Address\nIgnore this IP Address\n" );fflush(stdout);
			fflush(stdout);
#endif
			Unlock( "HostList::AddHost()" );
			IPMSG_FUNC_RETURN( 0 );
		}
	}
	//IP���ɥ쥹��NIC�Υ֥��ɥ��㥹�ȡ��ͥåȥ���Υ��ɥ쥹�Ȱ��פ�����̵�롣�ʤ��֤󤢤ꤨ�ʤ����ɡ���
	for( unsigned int i = 0; i < nics.size(); i++ ){
#if defined(INFO) || !defined(NDEBUG)
		printf("HostList::AddHost now host checking IpAddress=%s Network%s Broadcast=%s\n", host.IpAddress().c_str(), nics[i].NetworkAddress().c_str(), nics[i].BroadcastAddress().c_str() );
		fflush(stdout);
#endif
		if ( host.IpAddress() == nics[i].NetworkAddress() ) {
#if defined(INFO) || !defined(NDEBUG)
			printf("HostList::AddHost Host Ip Address is match NIC Network Address.\nIgnore this IP Address\n" );
			fflush(stdout);
#endif
			Unlock( "HostList::AddHost()" );
			IPMSG_FUNC_RETURN( 0 );
		}
		if ( host.IpAddress() == nics[i].BroadcastAddress() ){
#if defined(INFO) || !defined(NDEBUG)
			printf("HostList::AddHost Host Ip Address is match NIC Broadcast Address.\nIgnore this IP Address\n" );
			fflush(stdout);
#endif
			Unlock( "HostList::AddHost()" );
			IPMSG_FUNC_RETURN( 0 );
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("HostList::AddHost Host IpAddress=[%s] NIC[0] IP Address=[%s]\n", host.IpAddress().c_str(), nics[0].IpAddress().c_str() );
	printf("HostList::AddHost HostName=[%s] LocalhostName=[%s]\n", host.HostName().c_str(), localhostName.c_str() );
	fflush(stdout);
#endif
	//IP���ɥ쥹��������롼�ץХå����ɥ쥹�Ȱ��פ�����̵�롣
	if ( host.IpAddress() == "127.0.0.1" || host.IpAddress() == "::1" ){
#if defined(INFO) || !defined(NDEBUG)
		printf("HostList::AddHost Ignore this host item.Because host IP Address is local loopback.\n" );
		fflush(stdout);
#endif
		Unlock( "HostList::AddHost()" );
		IPMSG_FUNC_RETURN( 0 );
	}
	//IP���ɥ쥹��NIC��IP���ɥ쥹�Ȱ��פ���Τ˥�����ۥ���̾�Ȱ��פ��ʤ����̵�롣
	if ( host.IpAddress() == nics[0].IpAddress() && host.HostName() != localhostName ){
#if defined(INFO) || !defined(NDEBUG)
		printf("HostList::AddHost Ignore this host item.Because host IPAddress and NIC[0]'s IP Address is match,but not same hostname.\n" );
		fflush(stdout);
#endif
		Unlock( "HostList::AddHost()" );
		IPMSG_FUNC_RETURN( 0 );
	}
	std::vector<HostListItem>::iterator tmpHost;
	for( tmpHost = items.begin(); tmpHost != items.end(); tmpHost++ ){
		if ( isPermitSameHardwareAddress ) {
			if ( tmpHost->Equals( host ) ) {
				is_found = true;
				break;
			}
		} else {
#if defined(INFO) || !defined(NDEBUG)
			printf("HostList::AddHost Searching hardware address...host[%s] host list item[%s]\n",
							host.HardwareAddress().c_str(),
							tmpHost->HardwareAddress().c_str() );
			fflush(stdout);
#endif
			if ( tmpHost->EqualsHardwareAddress( host ) ) {
#if defined(INFO) || !defined(NDEBUG)
				printf("HostList::AddHost Found same hardware address in this host list.(HWADDR %s)\n", host.HardwareAddress().c_str() );
				fflush(stdout);
#endif
				is_found = true;
				//IPv6��IPv4���ͥ�褹�롣
#if defined(INFO) || !defined(NDEBUG)
				printf("AddHost host.AddressFamily() %s %s\n", host.IpAddress().c_str(), host.AddressFamily() == AF_INET6 ? "AF_INET6" : "AF_INET" );
				printf("AddHost tmpHost.AddressFamily() %s %s\n", tmpHost->IpAddress().c_str(), tmpHost->AddressFamily() == AF_INET6 ? "AF_INET6" : "AF_INET" );
#endif
				if ( host.AddressFamily() == AF_INET6 && tmpHost->AddressFamily() == AF_INET ) {
					*tmpHost = host;
#if defined(INFO) || !defined(NDEBUG)
					printf("HostList::AddHost Host is IPv6 supported,So swaped same hardware address in host list that is supported IPv4[%s].\n", host.HardwareAddress().c_str() );fflush(stdout);
#endif
				}
				break;
			}
		}
	}
	int ret = 0;
	if ( !is_found ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("HostList::AddHost Nickname=[%s] GroupName=[%s]\n", host.Nickname().c_str(), host.GroupName().c_str() );
		fflush(stdout);
#endif
		items.push_back( host );
		ret = 1;
	}
	if ( agent->GetSortHostListComparator() != NULL ){
		if ( items.size() > 0 ) {
			qsort( agent->GetSortHostListComparator(), 0, items.size() - 1 );
		}
	}
	Unlock( "HostList::AddHost()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param it �ۥ��Ⱦ���Υ��ƥ졼��
 */
void
HostList::Delete( std::vector<HostListItem>::iterator &it )
{
	IPMSG_FUNC_ENTER( "void HostList::Delete( std::vector<HostListItem>::iterator &it )" );
	Lock( "HostList::Delete()" );
	items.erase( it );
	Unlock( "HostList::Delete()" );
	IPMSG_FUNC_EXIT;
}
/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param hostname �ۥ���̾
 */
void
HostList::DeleteHostByAddress( std::string addr )
{
	IPMSG_FUNC_ENTER( "void HostList::DeleteHostByAddress( std::string addr )" );
	Lock( "HostList::DeleteHostIpAddress()" );
	struct sockaddr_storage ss;
	if ( createSockAddrIn( &ss, addr, 0 ) == NULL ){
		IPMSG_FUNC_EXIT;
	}
	for( std::vector<HostListItem>::iterator ix = items.begin(); ix < items.end(); ix++ ){
		struct sockaddr_storage ixss;
		if ( createSockAddrIn( &ixss, ix->IpAddress(), 0 ) == NULL ){
			IPMSG_FUNC_EXIT;
		}
		if ( isSameSockAddrIn( ss, ixss ) ){
			items.erase( ix );
			break;
		}
	}
	Unlock( "HostList::DeleteHostByAddress()" );
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ��������ʸ�����������롣
 * @param start ���ϰ���
 * @param addr ������Υ��ɥ쥹
 * @retval �ۥ��ȥꥹ��������ʸ����
 */
std::string
HostList::ToString( int start, const struct sockaddr_storage *addr )
{
	IPMSG_FUNC_ENTER( "std::string HostList::ToString( int start, const struct sockaddr_storage *addr )" );
	Lock( "HostList::ToString" );
	char buf[MAX_UDPBUF];
	std::string ret;
	unsigned int maxLength= IpMessengerAgentImpl::GetInstance()->GetMaxOptionBufferSize() - 12 /* 12 �� "12345\a12345\a"*/;

	ret = "";
	int hostCount = 0;
	for( unsigned int i = start ; i < items.size(); i++ ){
		HostListItem item = items.at( i );
		//��ʬ��IP���ɥ쥹���֤�����¾�Υͥåȥ�������Υ��ɥ쥹����äƤ�����ˡ�
		//������Υ��󥿡��ե������Υ��ɥ쥹���֤���
		size_t len = 0;
		if ( item.IsLocalHost() ) {
			IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
			std::vector<NetworkInterface> nics = agent->NICs;
			std::string localaddr = nics[0].IpAddress();
			for( unsigned int i = 0; i < nics.size(); i++ ){
				if ( isSameNetwork( addr, nics[i].NetworkAddress(), nics[i].NetMask() ) ){
					localaddr = nics[i].IpAddress();
					break;
				}
			}
			len = snprintf( buf, sizeof( buf ), "%s\a%s\a%ld\a%s\a%s\a%s\a%s\a",
							item.UserName() == "" ? "\b" : item.UserName().c_str(),
							item.HostName() == "" ? "\b" : item.HostName().c_str(),
							item.CommandNo(),
							localaddr == "" ? "\b" : localaddr.c_str(),
							IpMsgPortToStr( item.PortNo() ).c_str(),
							item.Nickname() == "" ? "\b" : item.Nickname().c_str(),
							item.GroupName() == "" ? "\b" : item.GroupName().c_str() );
		} else {
			if ( item.AddressFamily() != addr->ss_family ) {
				continue;
			}
			len = snprintf( buf, sizeof( buf ), "%s\a%s\a%ld\a%s\a%s\a%s\a%s\a",
							item.UserName() == "" ? "\b" : item.UserName().c_str(),
							item.HostName() == "" ? "\b" : item.HostName().c_str(),
							item.CommandNo(),
							item.IpAddress() == "" ? "\b" : item.IpAddress().c_str(),
							IpMsgPortToStr( item.PortNo() ).c_str(),
							item.Nickname() == "" ? "\b" : item.Nickname().c_str(),
							item.GroupName() == "" ? "\b" : item.GroupName().c_str() );
		}
		if ( len >= sizeof( buf ) ) {
			continue;
		}
		if ( ret.length() >= maxLength ){
			break;
		}
		ret = ret + buf;
		hostCount++;
	}
	snprintf( buf, sizeof( buf ), "%-5d\a%-5d\a", start , hostCount );
	ret = buf + ret;
#if defined(INFO) || !defined(NDEBUG)
	printf( "HostList::ToString [%s]\n", ret.c_str() );fflush(stdout);
#endif
	Unlock( "HostList::ToString" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ѥ��åȥ��֥������Ȥ���ۥ��ȥꥹ�ȥ����ƥ���������롣
 * @param packet �ѥ��åȥ��֥�������
 * @retval �ۥ��ȥꥹ�ȥ����ƥ�
 */
HostListItem
HostList::CreateHostListItemFromPacket( const Packet& packet )
{
	IPMSG_FUNC_ENTER( "HostListItem HostList::CreateHostListItemFromPacket( const Packet& packet )" );
	HostListItem ret;
	ret.setHostName( packet.HostName() );
	ret.setUserName( packet.UserName() );
	ret.setCommandNo( packet.CommandMode() | packet.CommandOption() );
	ret.setIpAddress( getSockAddrInRawAddress( packet.Addr() ) );
	ret.setPortNo( ntohs( getSockAddrInPortNo( packet.Addr() ) ) );
	unsigned int loc = packet.Option().find_first_of( '\0' );
	if ( loc == std::string::npos ) {
		ret.setNickname( packet.Option() );
		ret.setGroupName( "" );
	} else {
		ret.setNickname( packet.Option().substr( 0, loc ) );
		ret.setGroupName( packet.Option().substr( loc + 1 ) );
	}
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ۥ��ȥꥹ�Ȥ�ۥ���̾�Ǹ���������������HostListItem���ֵѤ��롣
 * @param hostName �ۥ���̾
 * @retval HostListItem
 */
std::vector<HostListItem>::iterator
HostList::FindHostByHostName( std::string hostName, int addressFamily )
{
	IPMSG_FUNC_ENTER( "std::vector<HostListItem>::iterator HostList::FindHostByHostName( std::string hostName )" );
	Lock( "HostList::FindHostByHostName()" );
	std::vector<HostListItem>::iterator ret = end();

	for( std::vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
		if ( ix->HostName() == hostName && addressFamily == ix->AddressFamily() ) {
			ret = ix;
			break;
		}
	}
	Unlock( "HostList::FindHostByHostName()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * �ۥ��ȥꥹ�Ȥ�IP���ɥ쥹�Ǹ���������������HostListItem���ֵѤ��롣
 * @param addr IP���ɥ쥹ʸ����
 * @retval HostListItem
 */
std::vector<HostListItem>::iterator
HostList::FindHostByAddress( std::string addr )
{
	IPMSG_FUNC_ENTER( "std::vector<HostListItem>::iterator HostList::FindHostByAddress( std::string addr )" );
	Lock( "HostList::FindHostByAddress()" );
	std::vector<HostListItem>::iterator ret = end();
	struct sockaddr_storage ss;
	if ( createSockAddrIn( &ss, addr, 0 ) == NULL ){
		IPMSG_FUNC_RETURN( ret );
	}
	for( std::vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
		struct sockaddr_storage ixss;
		if ( createSockAddrIn( &ixss, ix->IpAddress(), 0 ) == NULL ){
			IPMSG_FUNC_RETURN( ret );
		}
		if ( isSameSockAddrIn( ss, ixss ) ){
			ret = ix;
			break;
		}
	}
	Unlock( "HostList::FindHostByAddress()" );
	IPMSG_FUNC_RETURN( ret );
}

/**
 * IP���ɥ쥹�Υ��å���
 * @param IP���ɥ쥹
 */
void
HostListItem::setIpAddress( const std::string val )
{
	IPMSG_FUNC_ENTER( "void HostListItem::setIpAddress( const std::strin val )" );
	_IpAddress = val;
	sockaddr_storage ss;
	createSockAddrIn( &ss, val, 0 );
	_AddressFamily = ss.ss_family;
	_HardwareAddress = convertIpAddressToMacAddress( val, IpMessengerAgentImpl::GetInstance()->NICs );
	IPMSG_FUNC_EXIT;
}

/**
 * IP���ɥ쥹�Υ��å���
 * @retval IP���ɥ쥹
 */
std::string
HostListItem::IpAddress() const
{
	IPMSG_FUNC_ENTER( "std::string HostListItem::IpAddress() const" );
	IPMSG_FUNC_RETURN( _IpAddress );
}

/**
 * �ۥ��Ȥ��ե�����ź�դ򥵥ݡ��Ȥ��Ƥ��뤫��
 * @retval true:���ݡ���
 * @retval false:���ݡ��Ȥ��ʤ�
 */
bool
HostListItem::IsFileAttachSupport() const
{
	IPMSG_FUNC_ENTER( "bool HostListItem::IsFileAttachSupport() const" );
	IPMSG_FUNC_RETURN( CommandNo() & IPMSG_FILEATTACHOPT );
}

/**
 * �ۥ��Ȥ��Ź�򥵥ݡ��Ȥ��Ƥ��뤫��
 * @retval true:���ݡ���
 * @retval false:���ݡ��Ȥ��ʤ�
 */
bool
HostListItem::IsEncryptSupport() const
{
	IPMSG_FUNC_ENTER( "bool HostListItem::IsEncryptSupport() const" );
	IPMSG_FUNC_RETURN( CommandNo() & IPMSG_ENCRYPTOPT );
}

/**
 * �ۥ��Ȥ������Ժߤ���
 * @retval true:�Ժ�
 * @retval false:�ԺߤǤʤ�
 */
bool
HostListItem::IsAbsence() const
{
	IPMSG_FUNC_ENTER( "bool HostListItem::IsAbsence() const" );
	IPMSG_FUNC_RETURN( CommandNo() & IPMSG_ABSENCEOPT );
}

/**
 * �ۥ��ȥꥹ�ȥ����ƥ४�֥������Ȥ���ʬ�Ȱ��פ��뤫���֤���
 * @param item �ۥ��ȥꥹ�ȥ����ƥ�
 * @retval true:����
 * @retval false:���פ��ʤ�
 */
bool
HostListItem::Equals( const HostListItem& item ) const
{
	IPMSG_FUNC_ENTER( "bool HostListItem::Equals( const HostListItem& item ) const" );
	IPMSG_FUNC_RETURN( Compare( item ) == 0 );
}

/**
 * �ۥ��ȥꥹ�ȥ����ƥ४�֥������ȤΥϡ��ɥ��������ɥ쥹����ʬ�Ȱ��פ��뤫���֤���
 * @param item �ۥ��ȥꥹ�ȥ����ƥ�
 * @retval true:����
 * @retval false:���פ��ʤ�
 */
bool
HostListItem::EqualsHardwareAddress( const HostListItem& item ) const
{
	IPMSG_FUNC_ENTER( "bool HostListItem::EqualsHardwareAddress( const HostListItem& item ) const" );
	IPMSG_FUNC_RETURN( CompareHardwareAddress( item ) == 0 );
}

/**
 * ��ӡ�
 * @param item �ۥ��Ⱦ���1
 * @retval -n:*this���礭��
 * @retval 0:item��*this��������
 * @retval +n:item���礭��
 */
int
HostListItem::Compare( const HostListItem& item ) const
{
	IPMSG_FUNC_ENTER( "int HostListItem::Compare( const HostListItem& item ) const" );
	if ( item.UserName()  == UserName() &&
		 item.HostName()  == HostName() &&
		 item.IpAddress() == IpAddress() ){
		IPMSG_FUNC_RETURN( 0 );
	}
	if ( item.UserName()  > UserName() &&
		 item.HostName()  > HostName() &&
		 item.IpAddress() > IpAddress() ){
		IPMSG_FUNC_RETURN( 1 );
	}
	IPMSG_FUNC_RETURN(  -1 );
}

/**
 * �ϡ��ɥ��������ɥ쥹�Ǥ���ӡ�
 * @param item �ۥ��Ⱦ���1
 * @retval -n:*this���礭��
 * @retval 0:item��*this��������
 * @retval +n:item���礭��
 */
int
HostListItem::CompareHardwareAddress( const HostListItem& item ) const
{
	IPMSG_FUNC_ENTER( "int HostListItem::CompareHardwareAddress( const HostListItem& item ) const" );
	if ( item.HardwareAddress() == HardwareAddress() ){
		IPMSG_FUNC_RETURN( Compare( item ) );
	}
	if ( item.HardwareAddress() > HardwareAddress() ){
		IPMSG_FUNC_RETURN( 1 );
	}
	IPMSG_FUNC_RETURN(  -1 );
}

/**
 * �ۥ��ȥꥹ�Ȥ���ӥ��֥������ȤΥ����Ƚ���˱�äƥ����Ȥ��ޤ���
 * @param comparator ��ӥ��֥�������
 */
void
HostList::qsort( HostListComparator *comparator, int left, int right )
{
	IPMSG_FUNC_ENTER( "void HostList::qsort( HostListComparator *comparator, int left, int right )" );
	//�ϰϤγ��ϡ���λ����
	int i = left, j = right;

	//�����
	std::vector<HostListItem>::iterator pivot = items.begin() + ( ( left + right ) / 2 );
	//�����å�������
	while( true ){
		while( comparator->compare( items.begin() + i, pivot ) < 0 ) i++;
		while( comparator->compare( pivot, items.begin() + j ) < 0 ) j--;
		if ( i >= j ) {
			break;
		}
		std::iter_swap( items.begin() + i, items.begin() + j );
		i++;
		j--;
	}
	if ( left < i - 1 ) {	//����ͤκ��ˣ��İʾ����Ǥ�����к�������򥽡��Ȥ��롣
		qsort( comparator, left, i - 1 );
	}
	if ( j + 1 < right ) {	//����ͤα��ˣ��İʾ����Ǥ�����б�������򥽡��Ȥ��롣
		qsort( comparator, j + 1, right );
	}
	IPMSG_FUNC_EXIT;
}

/**
 * �ۥ��ȥꥹ�Ȥ���ӥ��֥������ȤΥ����Ƚ���˱�äƥ����Ȥ��ޤ���
 * @param comparator ��ӥ��֥�������
 */
void
HostList::sort( HostListComparator *comparator )
{
	IPMSG_FUNC_ENTER( "void HostList::sort( HostListComparator *comparator )" );
	if ( items.size() > 0 ) {
		qsort( comparator, 0, items.size() - 1 );
	}
	IPMSG_FUNC_EXIT;
}

class IpMsgGetGroupListComparator: public HostListComparator{
	public:
		/**
		 * ���롼��̾�ȥ��󥳡��ǥ���̾����ӡ�
		 * @param host1 �ۥ��Ⱦ���1
		 * @param host2 �ۥ��Ⱦ���2
		 * @retval -n:host1���礭��
		 * @retval 0:host1��host2��������
		 * @retval +n:host2���礭��
		 */
		virtual int compare( std::vector<HostListItem>::iterator host1, std::vector<HostListItem>::iterator host2 ){
			if ( host1->GroupName() < host2->GroupName() ) {
				return -1;
			} else if ( host1->GroupName() > host2->GroupName() ) {
				return 1;
			} else {
				if ( host1->EncodingName() < host2->EncodingName() ) {
					return -1;
				} else if ( host1->EncodingName() > host2->EncodingName() ) {
					return 1;
				}
			}
			return 0;
		};
};

/**
 * �ݻ���Υۥ��ȥꥹ�Ȥ��饰�롼�ץꥹ�Ȥ�������롣
 * @retval ���롼�ץꥹ��
 */
std::vector<GroupItem>
HostList::GetGroupList()
{
	IPMSG_FUNC_ENTER( "std::vector<GroupItem> HostList::GetGroupList()" );
	std::vector<GroupItem> ret;
	HostList tmp = *this;
	tmp.sort( new IpMsgGetGroupListComparator() );
	std::string hostName = "", encodingName = "";
	for( std::vector<HostListItem>::iterator ixhost = tmp.begin(); ixhost != tmp.end(); ixhost++ ) {
		if ( hostName != ixhost->HostName() || encodingName != ixhost->EncodingName() ){
			GroupItem item;
			item.setGroupName( ixhost->GroupName() );
			item.setEncodingName( ixhost->EncodingName() );
			ret.push_back( item );
		}
		hostName = ixhost->HostName();
		encodingName = ixhost->EncodingName();
	}
	IPMSG_FUNC_RETURN( ret );
}

//end of source
