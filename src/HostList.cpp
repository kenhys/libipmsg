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

using namespace std;

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * ���󥹥ȥ饯����
 * ���ۥ��ȥꥹ�Ȥ��å����뤿��Υߥ塼�ƥå�����������
 */
HostList::HostList()
{
	IpMsgMutexInit( "HostList::HostList()", &hostListMutex, NULL );
}

/**
 * �ǥ��ȥ饯����
 * ���ۥ��ȥꥹ�Ȥ��å����뤿��Υߥ塼�ƥå������˴���
 */
HostList::~HostList()
{
	IpMsgMutexDestroy( "HostList::~HostList()", &hostListMutex );
}

/**
 * �ۥ��ȥꥹ�Ȥ���Ƭ�򼨤����ƥ졼�����֤���
 * @retval �ۥ��ȥꥹ�Ȥ���Ƭ�򼨤����ƥ졼����
 */
vector<HostListItem>::iterator
HostList::begin()
{
	return items.begin();
}

/**
 * �ۥ��ȥꥹ�Ȥ������ܣ��򼨤����ƥ졼�����֤���
 * @retval �ۥ��ȥꥹ�Ȥ������ܣ��򼨤����ƥ졼����
 */
vector<HostListItem>::iterator
HostList::end()
{
	return items.end();
}

/**
 * �ۥ��ȥꥹ�ȤθĿ����֤���
 * @retval �ۥ��ȥꥹ�ȤθĿ���
 */
int
HostList::size()
{
	IpMsgMutexLock( "HostList::size()", &hostListMutex );
	int ret = items.size();
	IpMsgMutexUnlock( "HostList::size()", &hostListMutex );
	return ret;
}

/**
 * �ۥ��ȥꥹ�Ȥ򥯥ꥢ���롣
 */
void
HostList::clear()
{
	IpMsgMutexLock( "HostList::clear()", &hostListMutex );
	items.clear();
	IpMsgMutexUnlock( "HostList::clear()", &hostListMutex );
}

/**
 * �С����������䤤��碌��Ԥ���
 */
void
HostListItem::QueryVersionInfo()
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryVersionInfo( *this );
}

/**
 * �Ժ�����ʸ�����䤤��碌��Ԥ���
 */
void
HostListItem::QueryAbsenceInfo()
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryAbsenceInfo( *this );
}

/**
 * IP���ɥ쥹�򸵤˥�����ۥ��Ȥ��ɤ�������롣
 * @retval true:������ۥ��ȡ�false:������ۥ��ȤǤϤʤ���
 */
bool
HostListItem::IsLocalHost()
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	vector<NetworkInterface> nics = agent->NICs;
	for( unsigned int i = 0; i < nics.size(); i++ ){
		if ( IpAddress() == nics[i].IpAddress() ){
#if defined(INFO) || !defined(NDEBUG)
			printf("LOCALHOST\n");fflush(stdout);
#endif
			return true;
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("OTHERHOST\n");fflush(stdout);
#endif
	return false;
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��ɲä��롣
 * @param host �ۥ��Ⱦ���
 */
void
HostList::AddHost( const HostListItem& host )
{
	IpMsgMutexLock( "HostList::AddHost()", &hostListMutex );
	bool is_found = false;

	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	string localhostName = agent->HostName();
	vector<NetworkInterface> nics = agent->NICs;
	for( unsigned int i = 1; i < nics.size(); i++ ){
#if defined(INFO) || !defined(NDEBUG)
		printf("AddHost HOST CHECK IpAddress=%s addr=%s\n", host.IpAddress().c_str(), nics[i].IpAddress().c_str() );fflush(stdout);
#endif
		if ( host.IpAddress() == nics[i].IpAddress() ){
			IpMsgMutexUnlock( "HostList::AddHost()", &hostListMutex );
			return;
		}
	}
#if defined(INFO) || !defined(NDEBUG)
	printf("AddHost HOST CHECK IpAddress=%s addr=%s\n", host.IpAddress().c_str(), nics[0].IpAddress().c_str() );fflush(stdout);
	printf("AddHost HOST CHECK HostName=%s localhost=%s\n", host.HostName().c_str(), localhostName.c_str() );fflush(stdout);
#endif
	if ( host.IpAddress() == "127.0.0.1" ){
#if defined(INFO) || !defined(NDEBUG)
		printf("IGNORE HOST.Because host IpAddress local loopback.\n" );fflush(stdout);
#endif
		IpMsgMutexUnlock( "HostList::AddHost()", &hostListMutex );
		return;
	}
	if ( host.IpAddress() == nics[0].IpAddress() && host.HostName() != localhostName ){
		IpMsgMutexUnlock( "HostList::AddHost()", &hostListMutex );
		return;
	}
	for( unsigned int i = 0; i < items.size(); i++ ){
		HostListItem tmpHost = items.at( i );
		if ( tmpHost.Equals( host ) ) {
			is_found = true;
			break;
		}
	}
	if ( !is_found ) {
		items.push_back( host );
	}
	if ( agent->GetSortHostListComparator() != NULL ){
		qsort( agent->GetSortHostListComparator(), 0, items.size() - 1 );
	}
	IpMsgMutexUnlock( "HostList::AddHost()", &hostListMutex );
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param �ۥ��Ⱦ���Υ��ƥ졼��
 */
void
HostList::Delete( vector<HostListItem>::iterator &it )
{
	IpMsgMutexLock( "HostList::Delete()", &hostListMutex );
	items.erase( it );
	IpMsgMutexUnlock( "HostList::Delete()", &hostListMutex );
}
/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param �ۥ���̾
 */
void
HostList::DeleteHost( string hostname )
{
	IpMsgMutexLock( "HostList::DeleteHost()", &hostListMutex );
	for( vector<HostListItem>::iterator ix = items.begin(); ix < items.end(); ix++ ){
		if ( ix->HostName() == hostname ) {
			items.erase( ix );
			break;
		}
	}
	IpMsgMutexUnlock( "HostList::DeleteHost()", &hostListMutex );
}

/**
 * �ۥ��ȥꥹ��������ʸ�����������롣
 * @param start ���ϰ���
 * @retval �ۥ��ȥꥹ��������ʸ����
 */
string
HostList::ToString( int start )
{
	IpMsgMutexLock( "HostList::ToString", &hostListMutex );
	char buf[MAX_UDPBUF];
	string ret;
	unsigned int maxLength= IpMessengerAgentImpl::GetInstance()->GetMaxOptionBufferSize();

	ret = "";
	int hostCount = 0;
	for( unsigned int i = start ; i < items.size(); i++ ){
		HostListItem item = items.at( i );
		sprintf( buf, "%s\a%s\a%ld\a%s\a%d\a%s\a%s\a",
						item.UserName() == "" ? "\b" : item.UserName().c_str(),
						item.HostName() == "" ? "\b" : item.HostName().c_str(),
						item.CommandNo(),
						item.IpAddress() == "" ? "\b" : item.IpAddress().c_str(),
						htons( item.PortNo() ),
						item.Nickname() == "" ? "\b" : item.Nickname().c_str(),
						item.GroupName() == "" ? "\b" : item.GroupName().c_str() );
		if ( ret.length() >= maxLength ){
			break;
		}
		ret = ret + buf;
		hostCount++;
	}
	snprintf( buf, sizeof( buf ), "%-5d\a%-5d\a", start , hostCount );
	ret = buf + ret;
	IpMsgMutexUnlock( "HostList::ToString", &hostListMutex );
	return ret;
}

/**
 * �ѥ��åȥ��֥������Ȥ���ۥ��ȥꥹ�ȥ����ƥ���������롣
 * @param packet �ѥ��åȥ��֥�������
 * @retval �ۥ��ȥꥹ�ȥ����ƥ�
 */
HostListItem
HostList::CreateHostListItemFromPacket( const Packet& packet )
{
	HostListItem ret;
	ret.setHostName( packet.HostName() );
	ret.setUserName( packet.UserName() );
	ret.setCommandNo( packet.CommandMode() | packet.CommandOption() );
	char tmp[100];
	ret.setIpAddress( inet_ntoa_r( packet.Addr().sin_addr.s_addr, tmp, sizeof( tmp ) ) );
#if defined(INFO) || !defined(NDEBUG)
	printf( "CreateHostListItemFromPacket port %d\n", packet.Addr().sin_port );fflush(stdout);
#endif
	ret.setPortNo( packet.Addr().sin_port );
	unsigned int loc = packet.Option().find_first_of( '\0' );
	if ( loc == string::npos ) {
		ret.setNickname( packet.Option() );
		ret.setGroupName( "" );
	} else {
		ret.setNickname( packet.Option().substr( 0, loc ) );
		ret.setGroupName( packet.Option().substr( loc + 1 ) );
	}
	return ret;
}

/**
 * �ۥ��ȥꥹ�Ȥ�ۥ���̾�Ǹ���������������HostListItem���ֵѤ��롣
 * @param hostName �ۥ���̾
 * @retval HostListItem
 */
vector<HostListItem>::iterator
HostList::FindHostByHostName( string hostName )
{
	IpMsgMutexLock( "HostList::FindHostByHostName()", &hostListMutex );
	vector<HostListItem>::iterator ret = end();
	for( vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
		if ( ix->HostName() == hostName ) {
			ret = ix;
			break;
		}
	}
	IpMsgMutexUnlock( "HostList::FindHostByHostName()", &hostListMutex );
	return ret;
}

/**
 * �ۥ��ȥꥹ�Ȥ�IP���ɥ쥹�Ǹ���������������HostListItem���ֵѤ��롣
 * @param addr IP���ɥ쥹ʸ����
 * @retval HostListItem
 */
vector<HostListItem>::iterator
HostList::FindHostByAddress( string addr )
{
	IpMsgMutexLock( "HostList::FindHostByAddress()", &hostListMutex );
	vector<HostListItem>::iterator ret = end();
	for( vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
#if defined(DEBUG)
		printf("HOST CHECK IpAddress=%s addr=%s\n", ix->IpAddress().c_str(), addr.c_str() );fflush(stdout);
#endif
		if ( ix->IpAddress() == addr ) {
#if defined(DEBUG)
			printf("HOST FOUND!!!\n");fflush(stdout);
#endif
			ret = ix;
			break;
		}
	}
#if defined(DEBUG)
	printf("HOST NOT FOUND!!!\n");fflush(stdout);
#endif
	IpMsgMutexUnlock( "HostList::FindHostByAddress()", &hostListMutex );
	return ret;
}

/**
 * �ۥ��Ȥ��ե�����ź�դ򥵥ݡ��Ȥ��Ƥ��뤫��
 * @retval ���ݡ��ȡ�true�����ݡ��Ȥ��ʤ���false
 */
bool
HostListItem::IsFileAttachSupport()
{
	return CommandNo() & IPMSG_FILEATTACHOPT;
}

/**
 * �ۥ��Ȥ��Ź�򥵥ݡ��Ȥ��Ƥ��뤫��
 * @retval ���ݡ��ȡ�true�����ݡ��Ȥ��ʤ���false
 */
bool
HostListItem::IsEncryptSupport()
{
	return CommandNo() & IPMSG_ENCRYPTOPT;
}

/**
 * �ۥ��Ȥ������Ժߤ���
 * @retval �Ժߡ�true���ԺߤǤʤ���false
 */
bool
HostListItem::IsAbsence()
{
	return CommandNo() & IPMSG_ABSENCEOPT;
}

/**
 * �ۥ��ȥꥹ�ȥ����ƥ४�֥������Ȥ���ʬ�Ȱ��פ��뤫���֤���
 * @param item �ۥ��ȥꥹ�ȥ����ƥ�
 * @retval ���ס�true�����פ��ʤ���false
 */
bool
HostListItem::Equals( const HostListItem& item )
{
	return	Compare( item ) == 0;
}
int
HostListItem::Compare( const HostListItem& item )
{
//	if ( item.UserName()  == UserName() &&
//		 item.HostName()  == HostName() &&
//		 item.IpAddress() == IpAddress() &&
//		 item.Nickname()  == Nickname() &&
//		 item.GroupName() == GroupName() &&
//		 item.PortNo()    == PortNo() {
	if ( item.UserName()  == UserName() &&
		 item.HostName()  == HostName() &&
		 item.IpAddress() == IpAddress() ){
		return 0;
	}
//	if ( item.UserName()  > UserName() &&
//		 item.HostName()  > HostName() &&
//		 item.IpAddress() > IpAddress() &&
//		 item.Nickname()  > Nickname() &&
//		 item.GroupName() > GroupName() &&
//		 item.PortNo()    > PortNo() {
	if ( item.UserName()  > UserName() &&
		 item.HostName()  > HostName() &&
		 item.IpAddress() > IpAddress() ){
		return 1;
	}
	return -1;
}

/**
 * �ۥ��ȥꥹ�Ȥ���ӥ��֥������ȤΥ����Ƚ���˱�äƥ����Ȥ��ޤ���
 * @param comparator ��ӥ��֥�������
 */
void
HostList::qsort( HostListComparator *comparator, int left, int right )
{
	//�ϰϤγ��ϡ���λ����
	int i = left, j = right;
#if defined(INFO) || !defined(NDEBUG)
	printf("ADDRESS LEFT(%d)  =IpAddress=%s\n", left, (items.begin() + left)->IpAddress().c_str() );fflush(stdout);
	printf("ADDRESS RIGHT(%d) =IpAddress=%s\n", right, (items.begin() + right)->IpAddress().c_str() );fflush(stdout);
#endif
	//�����
	vector<HostListItem>::iterator pivot = items.begin() + ( ( left + right ) / 2 );
	//�����å�������
	while( true ){
		while( comparator->compare( items.begin() + i, pivot ) < 0 ) i++;
		while( comparator->compare( pivot, items.begin() + j ) < 0 ) j--;
		if ( i >= j ) break;
#if defined(INFO) || !defined(NDEBUG)
			printf("SWAP BEFORE I(%d)  =IpAddress=%s\n", i, (items.begin() + i)->IpAddress().c_str() );fflush(stdout);
			printf("SWAP BEFORE J(%d) =IpAddress=%s\n", j, (items.begin() + j)->IpAddress().c_str() );fflush(stdout);
#endif
		iter_swap( items.begin() + i, items.begin() + j );
#if defined(INFO) || !defined(NDEBUG)
		printf("SWAP BEFORE I(%d) =IpAddress=%s\n", i, (items.begin() + i)->IpAddress().c_str() );fflush(stdout);
		printf("SWAP BEFORE J(%d) =IpAddress=%s\n", j, (items.begin() + j)->IpAddress().c_str() );fflush(stdout);
#endif
		i++;
		j--;
	}
	if ( left < i - 1 ) {	//����ͤκ��ˣ��İʾ����Ǥ�����к�������򥽡��Ȥ��롣
		qsort( comparator, left, i - 1 );
	}
	if ( j + 1 < right ) {	//����ͤα��ˣ��İʾ����Ǥ�����б�������򥽡��Ȥ��롣
		qsort( comparator, j + 1, right );
	}
}

/**
 * �ۥ��ȥꥹ�Ȥ���ӥ��֥������ȤΥ����Ƚ���˱�äƥ����Ȥ��ޤ���
 * @param comparator ��ӥ��֥�������
 */
void
HostList::sort( HostListComparator *comparator )
{
	qsort( comparator, 0, items.size() - 1 );
}
//end of source
