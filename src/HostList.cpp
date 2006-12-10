/**
 * IP メッセンジャライブラリ(Unix用)
 * ホストリストクラス。
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"
using namespace std;

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * ホスト情報をホストリストに追加する。
 * @param host ホスト情報
 */
void
HostList::AddHost( HostListItem host )
{
	bool is_found = false;

	vector<NetworkInterface> nics = IpMessengerAgentImpl::GetInstance()->NICs;
	for( int i = 1; i < nics.size(); i++ ){
		printf("AddHost HOST CHECK IpAddress=%s addr=%s\n", host.IpAddress().c_str(), nics[i].IpAddress().c_str() );
		if ( host.IpAddress() == nics[i].IpAddress() ){
			return;
		}
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
}

/**
 * ホスト情報をホストリストから削除する。
 * @param ホスト情報のイテレータ
 */
void
HostList::Delete( vector<HostListItem>::iterator &it )
{
	items.erase( it );
}
/**
 * ホスト情報をホストリストから削除する。
 * @param ホスト名
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
 * ホストリスト送信用文字列を作成する。
 * @param start 開始位置
 */
string
HostList::ToString( int start )
{
	char buf[MAX_UDPBUF];
	string ret;
	int maxLength= IpMessengerAgentImpl::GetInstance()->GetMaxOptionBufferSize();

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
 * パケットオブジェクトからホストリストアイテムを生成する。
 * @param packet パケットオブジェクト
 * @retval ホストリストアイテム
 */
HostListItem
HostList::CreateHostListItemFromPacket( Packet packet )
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
 * ホストリストアイテムオブジェクトが自分と一致するかを返す。
 * @param item ホストリストアイテム
 * @retval 一致：true／一致しない：false
 */
bool
HostListItem::Equals( HostListItem item )
{
	return	item.UserName() == UserName() &&
			item.HostName() == HostName() &&
			item.IpAddress() == IpAddress();
//			item.Nickname() == Nickname() &&
//			item.GroupName() == GroupName() &&
//			item.PortNo() == PortNo();
}

//end of source
