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

using namespace ipmsg;

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * コンストラクタ。
 * <ul>
 * <li>ホストリストをロックするためのミューテックスを生成。</li>
 * </ul>
 */
HostList::HostList()
{
	IpMsgMutexInit( "HostList::HostList()", &hostListMutex, NULL );
}

/**
 * コピーコンストラクタ。
 * <ul>
 * <li>ホストリストをロックするためのミューテックスを生成。</li>
 * </ul>
 * @param other コピー元のオブジェクト
 */
HostList::HostList( const HostList& other )
{
	IpMsgMutexInit( "HostList::HostList(HostList&)", &hostListMutex, NULL );
	Lock( "HostList::HostList(HostList&)" );
	CopyFrom( other );
	Unlock( "HostList::HostList(HostList&)" );
}

/**
 * デストラクタ。
 * <ul>
 * <li>ホストリストをロックするためのミューテックスを破棄。</li>
 * </ul>
 */
HostList::~HostList()
{
	IpMsgMutexDestroy( "HostList::~HostList()", &hostListMutex );
}

/**
 * 代入演算子。
 * <ul>
 * <li>ホストリストをロックするためのミューテックスを生成。</li>
 * </ul>
 * @param other コピー元のオブジェクト
 * @retval 自オブジェクトのインスタンス
 */
HostList&
HostList::operator=( const HostList& other )
{
	IpMsgMutexInit( "HostList::operator=(HostList&)", &hostListMutex, NULL );
	Lock( "HostList::operator=(HostList&)" );
	CopyFrom( other );
	Unlock( "HostList::operator=(HostList&)" );
	return *this;
}

/**
 * コピーメソッド。
 * @param other コピー元のオブジェクト
 */
void
HostList::CopyFrom( const HostList& other )
{
	items = other.items;
}

/**
 * ホストリストをロック
 * @param pos ロックしている位置を示す文字列。
 */
void
HostList::Lock( const char *pos ) const
{
	IpMsgMutexLock( pos, const_cast< pthread_mutex_t* >( &hostListMutex ) );
}

/**
 * ホストリストをアンロック
 * @param pos アンロックしている位置を示す文字列。
 */
void
HostList::Unlock( const char *pos ) const
{
	IpMsgMutexUnlock( pos, const_cast< pthread_mutex_t * >( &hostListMutex ) );
}

/**
 * ホストリストの先頭を示すイテレータを返す。
 * @retval ホストリストの先頭を示すイテレータ。
 */
std::vector<HostListItem>::iterator
HostList::begin()
{
	return items.begin();
}

/**
 * ホストリストの末尾＋１を示すイテレータを返す。
 * @retval ホストリストの末尾＋１を示すイテレータ。
 */
std::vector<HostListItem>::iterator
HostList::end()
{
	return items.end();
}

/**
 * ホストリストの個数を返す。
 * @retval ホストリストの個数。
 */
int
HostList::size() const
{
	Lock( "HostList::size()" );
	int ret = items.size();
	Unlock( "HostList::size()" );
	return ret;
}

/**
 * ホストリストをクリアする。
 */
void
HostList::clear()
{
	Lock( "HostList::clear()" );
	items.clear();
	Unlock( "HostList::clear()" );
}

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
 * IPアドレスを元にローカルホストかどうかを求める。
 * @retval true:ローカルホスト
 * @retval false:ローカルホストではない
 */
bool
HostListItem::IsLocalHost() const
{
	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	std::vector<NetworkInterface> nics = agent->NICs;
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
 * ホスト情報をホストリストに追加する。
 * @param host ホスト情報
 */
void
HostList::AddHost( const HostListItem& host )
{
	Lock( "HostList::AddHost()" );
	bool is_found = false;

	IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
	std::string localhostName = agent->HostName();
	std::vector<NetworkInterface> nics = agent->NICs;
	for( unsigned int i = 1; i < nics.size(); i++ ){
#if defined(INFO) || !defined(NDEBUG)
		printf("AddHost HOST CHECK IpAddress=%s addr=%s\n", host.IpAddress().c_str(), nics[i].IpAddress().c_str() );fflush(stdout);
#endif
		if ( host.IpAddress() == nics[i].IpAddress() ) {
			Unlock( "HostList::AddHost()" );
			return;
		}
	}
	for( unsigned int i = 0; i < nics.size(); i++ ){
#if defined(INFO) || !defined(NDEBUG)
		printf("AddHost HOST CHECK IpAddress=%s addr=%s\n", host.IpAddress().c_str(), nics[i].IpAddress().c_str() );fflush(stdout);
#endif
		if ( host.IpAddress() == nics[i].NetworkAddress() ||
			 host.IpAddress() == nics[i].BroadcastAddress() ){
			Unlock( "HostList::AddHost()" );
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
		Unlock( "HostList::AddHost()" );
		return;
	}
	if ( host.IpAddress() == nics[0].IpAddress() && host.HostName() != localhostName ){
		Unlock( "HostList::AddHost()" );
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
#if defined(INFO) || !defined(NDEBUG)
		printf("AddHost Nickname=%s\n", host.Nickname().c_str() );fflush(stdout);
		printf("AddHost GroupName=%s\n", host.GroupName().c_str() );fflush(stdout);
#endif
		items.push_back( host );
	}
	if ( agent->GetSortHostListComparator() != NULL ){
		if ( items.size() > 0 ) {
			qsort( agent->GetSortHostListComparator(), 0, items.size() - 1 );
		}
	}
	Unlock( "HostList::AddHost()" );
}

/**
 * ホスト情報をホストリストから削除する。
 * @param it ホスト情報のイテレータ
 */
void
HostList::Delete( std::vector<HostListItem>::iterator &it )
{
	Lock( "HostList::Delete()" );
	items.erase( it );
	Unlock( "HostList::Delete()" );
}
/**
 * ホスト情報をホストリストから削除する。
 * @param hostname ホスト名
 */
void
HostList::DeleteHostByAddress( std::string addr )
{
	Lock( "HostList::DeleteHostIpAddress()" );
	for( std::vector<HostListItem>::iterator ix = items.begin(); ix < items.end(); ix++ ){
		if ( ix->IpAddress() == addr ) {
			items.erase( ix );
			break;
		}
	}
	Unlock( "HostList::DeleteHostByAddress()" );
}

/**
 * ホストリスト送信用文字列を作成する。
 * @param start 開始位置
 * @param addr 送信先のアドレス
 * @retval ホストリスト送信用文字列。
 */
std::string
HostList::ToString( int start, const struct sockaddr_in *addr )
{
	Lock( "HostList::ToString" );
	char buf[MAX_UDPBUF];
	std::string ret;
	unsigned int maxLength= IpMessengerAgentImpl::GetInstance()->GetMaxOptionBufferSize() - 12 /* 12 は "12345\a12345\a"*/;

	ret = "";
	int hostCount = 0;
	for( unsigned int i = start ; i < items.size(); i++ ){
		HostListItem item = items.at( i );
		//自分のIPアドレスを返す場合で他のネットワーク向けのアドレスを持っている場合に、
		//そちらのインターフェースのアドレスを返す。
		int len = 0;
		if ( item.IsLocalHost() ) {
			IpMessengerAgentImpl *agent = IpMessengerAgentImpl::GetInstance();
			std::vector<NetworkInterface> nics = agent->NICs;
			std::string localaddr = nics[0].IpAddress();
			for( unsigned int i = 0; i < nics.size(); i++ ){
				if ( nics[i].NativeNetworkAddress().s_addr == addr->sin_addr.s_addr & nics[i].NativeNetMask().s_addr ){
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
	Unlock( "HostList::ToString" );
	return ret;
}

/**
 * パケットオブジェクトからホストリストアイテムを生成する。
 * @param packet パケットオブジェクト
 * @retval ホストリストアイテム
 */
HostListItem
HostList::CreateHostListItemFromPacket( const Packet& packet )
{
	HostListItem ret;
	ret.setHostName( packet.HostName() );
	ret.setUserName( packet.UserName() );
	ret.setCommandNo( packet.CommandMode() | packet.CommandOption() );
	char tmp[100];
	ret.setIpAddress( inet_ntop( AF_INET, &packet.Addr().sin_addr, tmp, sizeof( tmp ) ) );
#if defined(INFO) || !defined(NDEBUG)
	printf( "CreateHostListItemFromPacket port %d\n", ntohs( packet.Addr().sin_port ) );fflush(stdout);
#endif
	ret.setPortNo( ntohs( packet.Addr().sin_port ) );
	unsigned int loc = packet.Option().find_first_of( '\0' );
	if ( loc == std::string::npos ) {
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
 */
std::vector<HostListItem>::iterator
HostList::FindHostByHostName( std::string hostName )
{
	Lock( "HostList::FindHostByHostName()" );
	std::vector<HostListItem>::iterator ret = end();
	for( std::vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
		if ( ix->HostName() == hostName ) {
			ret = ix;
			break;
		}
	}
	Unlock( "HostList::FindHostByHostName()" );
	return ret;
}

/**
 * ホストリストをIPアドレスで検索し、該当するHostListItemを返却する。
 * @param addr IPアドレス文字列
 * @retval HostListItem
 */
std::vector<HostListItem>::iterator
HostList::FindHostByAddress( std::string addr )
{
	Lock( "HostList::FindHostByAddress()" );
	std::vector<HostListItem>::iterator ret = end();
	for( std::vector<HostListItem>::iterator ix = begin(); ix < end(); ix++ ){
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
	Unlock( "HostList::FindHostByAddress()" );
	return ret;
}

/**
 * ホストがファイル添付をサポートしているか？
 * @retval true:サポート
 * @retval false:サポートしない
 */
bool
HostListItem::IsFileAttachSupport() const
{
	return CommandNo() & IPMSG_FILEATTACHOPT;
}

/**
 * ホストが暗号をサポートしているか？
 * @retval true:サポート
 * @retval false:サポートしない
 */
bool
HostListItem::IsEncryptSupport() const
{
	return CommandNo() & IPMSG_ENCRYPTOPT;
}

/**
 * ホストが今、不在か？
 * @retval true:不在
 * @retval false:不在でない
 */
bool
HostListItem::IsAbsence() const
{
	return CommandNo() & IPMSG_ABSENCEOPT;
}

/**
 * ホストリストアイテムオブジェクトが自分と一致するかを返す。
 * @param item ホストリストアイテム
 * @retval true:一致
 * @retval false:一致しない
 */
bool
HostListItem::Equals( const HostListItem& item ) const
{
	return	Compare( item ) == 0;
}
/**
 * 比較。
 * @param item ホスト情報1
 * @retval -n:*thisが大きい
 * @retval 0:itemと*thisが等しい
 * @retval +n:itemが大きい
 */
int
HostListItem::Compare( const HostListItem& item ) const
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
	std::vector<HostListItem>::iterator pivot = items.begin() + ( ( left + right ) / 2 );
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
	if ( items.size() > 0 ) {
		qsort( comparator, 0, items.size() - 1 );
	}
}

class IpMsgGetGroupListComparator: public HostListComparator{
	public:
		/**
		 * グループ名とエンコーディング名で比較。
		 * @param host1 ホスト情報1
		 * @param host2 ホスト情報2
		 * @retval -n:host1が大きい
		 * @retval 0:host1とhost2が等しい
		 * @retval +n:host2が大きい
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
 * 保持中のホストリストからグループリストを取得する。
 * @retval グループリスト
 */
std::vector<GroupItem>
HostList::GetGroupList()
{
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
	return ret;
}

//end of source
