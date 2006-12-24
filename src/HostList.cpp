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
#include <algorithm>

using namespace std;

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * バージョン情報問い合わせを行う。
 */
void
HostListItem::QueryVersionInfo()
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryVersionInfo( *this );
}

/**
 * 不在説明文字列問い合わせを行う。
 */
void
HostListItem::QueryAbsenceInfo()
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	agent->QueryAbsenceInfo( *this );
}

/**
 * ホスト情報をホストリストに追加する。
 * @param host ホスト情報
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
 * ホストリストをホスト名で検索し、該当するHostListItemを返却する。
 * @param hostName ホスト名
 * @retval HostListItem
 * 注：このメソッドはスレッドセーフでない。
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
 * ホストリストをIPアドレスで検索し、該当するHostListItemを返却する。
 * @param addr IPアドレス文字列
 * @retval HostListItem
 * 注：このメソッドはスレッドセーフでない。
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
printf("★★★★★★★★★★★★\n");fflush(stdout);
printf("HOST FOUND!!!\n");fflush(stdout);
printf("★★★★★★★★★★★★\n");fflush(stdout);
#endif
			return ix;
		}
	}
#if defined(DEBUG)
printf("★★★★★★★★★★★★\n");fflush(stdout);
printf("HOST NOT FOUND!!!\n");fflush(stdout);
printf("★★★★★★★★★★★★\n");fflush(stdout);
#endif
	return end();
}

/**
 * ホストがファイル添付をサポートしているか？
 * @retval サポート：true／サポートしない：false
 */
bool
HostListItem::IsFileAttachSupport()
{
	return CommandNo() & IPMSG_FILEATTACHOPT;
}

/**
 * ホストが暗号をサポートしているか？
 * @retval サポート：true／サポートしない：false
 */
bool
HostListItem::IsEncryptSupport()
{
	return CommandNo() & IPMSG_ENCRYPTOPT;
}

/**
 * ホストが今、不在か？
 * @retval 不在：true／不在でない：false
 */
bool
HostListItem::IsAbsence()
{
	return CommandNo() & IPMSG_ABSENCEOPT;
}

/**
 * ホストリストアイテムオブジェクトが自分と一致するかを返す。
 * @param item ホストリストアイテム
 * @retval 一致：true／一致しない：false
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
 * ホストリストを比較オブジェクトのソート順序に沿ってソートします。
 * @param comparator 比較オブジェクト
 */
void
HostList::qsort( HostListComparator *comparator, int left, int right )
{
	//範囲の開始、終了位置
	int i = left, j = right;
#if defined(INFO) || !defined(NDEBUG)
printf("ADDRESS LEFT(%d)  =IpAddress=%s\n", left, (items.begin() + left)->IpAddress().c_str() );fflush(stdout);
printf("ADDRESS RIGHT(%d) =IpAddress=%s\n", right, (items.begin() + right)->IpAddress().c_str() );fflush(stdout);
#endif
	//基準値
	vector<HostListItem>::iterator pivot = items.begin() + ( ( left + right ) / 2 );
	//クイックソート
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
	if ( left < i - 1 ) {	//基準値の左に２個以上要素があれば左の配列をソートする。
		qsort( comparator, left, i - 1 );
	}
	if ( j + 1 < right ) {	//基準値の右に２個以上要素があれば右の配列をソートする。
		qsort( comparator, j + 1, right );
	}
}

/**
 * ホストリストを比較オブジェクトのソート順序に沿ってソートします。
 * @param comparator 比較オブジェクト
 */
void
HostList::sort( HostListComparator *comparator )
{
	qsort( comparator, 0, items.size() - 1 );
}
//end of source
