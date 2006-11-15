/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * �ۥ��ȥꥹ�ȥ��饹��
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
using namespace std;

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��ɲä��롣
 * @param host �ۥ��Ⱦ���
 */
void HostList::AddHost( HostListItem host )
{
	bool is_found = false;
	for( unsigned int i = 0; i < items.size(); i++ ){
		if ( host.Equals( items.at( i ) ) ) {
			is_found = true;
			break;
		}
	}
	if ( !is_found ) {
		items.push_back( host );
	}
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param �ۥ���̾
 */
void HostList::DeleteHost( string hostname )
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
string HostList::ToString( int start )
{
	char buf[MAX_UDPBUF];
	string ret;
	unsigned int max;

	max = start + HOST_LIST_SEND_MAX_AT_ONCE - 1;
	if ( max > items.size() ) {
		max = items.size();
	}
	snprintf( buf, sizeof( buf ), "%-5d\a%-5d\a", start , max - start < 0 ? 0 : max - start );
	ret = buf;
	for( unsigned int i = start ; i < max; i++ ){
		HostListItem item = items.at( i );
		sprintf( buf, "%s\a%s\a%ld\a%s\a%d\a%s\a%s\a",
						item.UserName() == "" ? "\b" : item.UserName().c_str(),
						item.HostName() == "" ? "\b" : item.HostName().c_str(),
						item.CommandNo(),
						item.IpAddress() == "" ? "\b" : item.IpAddress().c_str(),
						htons( item.PortNo() ),
						item.Nickname() == "" ? "\b" : item.Nickname().c_str(),
						item.GroupName() == "" ? "\b" : item.GroupName().c_str() );
		ret = ret + buf;
	}
	return ret;
}

/**
 * �ѥ��åȥ��֥������Ȥ���ۥ��ȥꥹ�ȥ����ƥ���������롣
 * @param packet �ѥ��åȥ��֥�������
 * @retval �ۥ��ȥꥹ�ȥ����ƥ�
 */
HostListItem HostList::CreateHostListItemFromPacket( Packet packet )
{
	HostListItem ret;
	ret.setHostName( packet.HostName() );
	ret.setUserName( packet.UserName() );
	ret.setCommandNo( packet.CommandMode() | packet.CommandOption() );
	ret.setIpAddress( inet_ntoa( packet.Addr().sin_addr ) );
#if defined(INFO) || !defined(NDEBUG)
	printf( "CreateHostListItemFromPacket port %d\n", packet.Addr().sin_port );
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
 * �ۥ��ȥꥹ�ȥ����ƥ४�֥������Ȥ���ʬ�Ȱ��פ��뤫���֤���
 * @param item �ۥ��ȥꥹ�ȥ����ƥ�
 * @retval ���ס�true�����פ��ʤ���false
 */
bool HostListItem::Equals( HostListItem item )
{
	return	item.UserName() == UserName() &&
			item.HostName() == HostName() &&
			item.IpAddress() == IpAddress();
//			item.Nickname() == Nickname() &&
//			item.GroupName() == GroupName() &&
//			item.PortNo() == PortNo();
}

//end of source
