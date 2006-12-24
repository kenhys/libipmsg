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

void
HostListItem::QueryVersionInfo()
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryVersionInfo( *this );
}

void
HostListItem::QueryAbsenceInfo()
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryAbsenceInfo( *this );
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��ɲä��롣
 * @param host �ۥ��Ⱦ���
 */
void
HostList::AddHost( HostListItem host )
{
	bool is_found = false;

	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	string localhostName = agent->HostName();
	vector<NetworkInterface> nics = agent->NICs;
	for( unsigned int i = 1; i < nics.size(); i++ ){
#if defined(INFO) || !defined(NDEBUG)
		printf("AddHost HOST CHECK IpAddress=%s addr=%s\n", host.IpAddress().c_str(), nics[i].IpAddress().c_str() );fflush(stdout);
#endif
		if ( host.IpAddress() == nics[i].IpAddress() ){
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
		return;
	}
	if ( host.IpAddress() == nics[0].IpAddress() && host.HostName() != localhostName ){
		return;
	}
	for( unsigned int i = 0; i < items.size(); i++ ){
		if ( host.Equals( items.at( i ) ) ) {
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
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param �ۥ��Ⱦ���Υ��ƥ졼��
 */
void
HostList::Delete( vector<HostListItem>::iterator &it )
{
	items.erase( it );
}
/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param �ۥ���̾
 */
void
HostList::DeleteHost( string hostname )
{
	for( vector<HostListItem>::iterator ix = items.begin(); ix < items.end(); ix++ ){
		if ( ix->HostName() == hostname ) {
			items.erase( ix );
			break;
		}
	}
}

/**
 * �ۥ��ȥꥹ��������ʸ�����������롣
 * @param start ���ϰ���
 */
string
HostList::ToString( int start )
{
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
	return ret;
}

/**
 * �ѥ��åȥ��֥������Ȥ���ۥ��ȥꥹ�ȥ����ƥ���������롣
 * @param packet �ѥ��åȥ��֥�������
 * @retval �ۥ��ȥꥹ�ȥ����ƥ�
 */
HostListItem
HostList::CreateHostListItemFromPacket( Packet packet )
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
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
vector<HostListItem>::iterator
HostList::FindHostByHostName( string hostName )
{
	for( vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
		if ( ix->HostName() == hostName ) {
			return ix;
		}
	}
	return end();
}

/**
 * �ۥ��ȥꥹ�Ȥ�IP���ɥ쥹�Ǹ���������������HostListItem���ֵѤ��롣
 * @param addr IP���ɥ쥹ʸ����
 * @retval HostListItem
 * �����Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
vector<HostListItem>::iterator
HostList::FindHostByAddress( string addr )
{
	for( vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
#if defined(DEBUG)
printf("HOST CHECK IpAddress=%s addr=%s\n", ix->IpAddress().c_str(), addr.c_str() );fflush(stdout);
#endif
		if ( ix->IpAddress() == addr ) {
#if defined(DEBUG)
printf("������������������������\n");fflush(stdout);
printf("HOST FOUND!!!\n");fflush(stdout);
printf("������������������������\n");fflush(stdout);
#endif
			return ix;
		}
	}
#if defined(DEBUG)
printf("������������������������\n");fflush(stdout);
printf("HOST NOT FOUND!!!\n");fflush(stdout);
printf("������������������������\n");fflush(stdout);
#endif
	return end();
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
HostListItem::Equals( HostListItem item )
{
	return	Compare( item ) == 0;
}
int
HostListItem::Compare( HostListItem item )
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
