#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <IpMessenger.h>
#include <IpMessengerImpl.h>
#include <ctype.h>
#include <ipmsg.h>
#include <pwd.h>
#ifdef HAVE_GETIFADDRS
#include <net/if.h>
#include <ifaddrs.h>
#endif

/**
 * ソケットを生成し、バインドする。
 * @param proto プロトコル(SOCK_STREAM, SOCK_DGRAM)。
 * @param addr ソケットアドレス。
 * @param devname デバイス名。
 * @retval ソケット。
 */
int
ipmsg::bindSocket( int proto, struct sockaddr_storage addr, const char *devname ) {
	int sock = socket( addr.ss_family, proto, 0 );
	const int on = 1;
	if ( sock >= 0 && addr.ss_family == AF_INET6 && setsockopt( sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof( on ) ) < 0 ){
		perror("setsockopt(udp,IPV6)");
		close( sock );
		return -1;
	}
	if ( sock >= 0 && bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_storage)) != 0 ){
		perror("bind(udp)");
		close( sock );
		return -1;
	}
	if ( sock >= 0 && proto == SOCK_DGRAM && addr.ss_family == AF_INET6 && devname != NULL ){
		const unsigned int devindex = if_nametoindex( devname );
		if ( setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &devindex, sizeof(devindex)) != 0 ) {
			close( sock );
			return -1;
		}
	}
	if ( sock >= 0 && proto == SOCK_DGRAM && addr.ss_family == AF_INET ){
		const int yes = 1;
		if ( setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) != 0 ) {
			perror("setsockopt(broadcast)");
			close( sock );
			return -1;
		}
	}
	return sock;
}

/**
 * ソケットアドレスを生成する。
 * @param addr ソケットアドレスのポインタ。
 * @param rawAddress IPアドレス文字列。
 * @param port ポート番号(ホストバイトオーダ)。
 * @param devname デバイス名（デフォルトNULL）
 * @retval 設定済のaddrのアドレス。
 */
struct sockaddr_storage *
ipmsg::createSockAddrIn( struct sockaddr_storage *addr, std::string rawAddress, int port, const char *devname )
{
	if ( addr != NULL ) {
		struct addrinfo hints, *res;
		char portstr[10];
		memset( addr, 0, sizeof( struct sockaddr_storage ) );
		snprintf( portstr, sizeof( portstr ), "%u", port );
		memset( &hints, 0, sizeof( hints ) );
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_NUMERICHOST;

		int err = getaddrinfo( rawAddress.c_str(), portstr, &hints, &res );
		if ( err != 0 ){
			fprintf(stderr, "getaddrinfo(addr=[%s] port=[%u] portstr[%s]):%s\n",
							rawAddress.c_str(), port, portstr, gai_strerror( err ) );
			return NULL;
		}
		if ( res->ai_family == AF_INET ) {
			struct sockaddr_in *sockaddrp = ( struct sockaddr_in * )addr;
			*sockaddrp = *( (struct sockaddr_in *)res->ai_addr );
			freeaddrinfo( res );
			return addr;
		} else if ( res->ai_family == AF_INET6 ) {
			struct sockaddr_in6 *sockaddrp = ( struct sockaddr_in6 * )addr;
			*sockaddrp = *( (struct sockaddr_in6 *)res->ai_addr );
			if ( devname != NULL ) {
				sockaddrp->sin6_scope_id = if_nametoindex( devname );
			}
			freeaddrinfo( res );
			return addr;
		} else {
			fprintf( stderr, "createSockAddrIn::unknown address family\n" );
		}
		freeaddrinfo( res );
	} else {
		fprintf( stderr, "createSockAddrIn::addr is null\n" );
	}
	return NULL;
}

/**
 * ソケットアドレスからポート番号を取得する。
 * @param addr ソケットアドレスのポインタ。
 * @retval ポート番号（ネットワークバイトオーダ）。
 */
int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage *addr )
{
	return getSockAddrInPortNo( *addr );
}

/**
 * ソケットアドレスからポート番号を取得する。
 * @param addr ソケットアドレス。
 * @retval ポート番号（ネットワークバイトオーダ）。
 */
int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage &addr )
{
	int ret = -1;
	if ( addr.ss_family == AF_INET ) {
		const struct sockaddr_in *addrp = (const struct sockaddr_in *)&addr;
		ret = addrp->sin_port;
	} else if ( addr.ss_family == AF_INET6 ) {
		const struct sockaddr_in6 *addrp = (const struct sockaddr_in6 *)&addr;
		ret = addrp->sin6_port;
	}
	return ret;
}

/**
 * ソケットアドレスからIPアドレス文字列を取得する。
 * @param addr ソケットアドレスのポインタ。
 * @retval IPアドレス文字列。
 */
std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage *addr )
{
	return getSockAddrInRawAddress( *addr );
}

/**
 * ソケットアドレスからIPアドレス文字列を取得する。
 * @param addr ソケットアドレス。
 * @retval IPアドレス文字列。
 */
std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage &addr )
{
	char ipAddrBuf[IP_ADDR_MAX_SIZE] = {0};
	if ( addr.ss_family == AF_INET ) {
		const struct sockaddr_in *addrp = (const struct sockaddr_in *)&addr;
		inet_ntop( addr.ss_family, &addrp->sin_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	} else if ( addr.ss_family == AF_INET6 ) {
		const struct sockaddr_in6 *addrp = (const struct sockaddr_in6 *)&addr;
		inet_ntop( addr.ss_family, &addrp->sin6_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	} else {
		return "";
	}
	return ipAddrBuf;
}

/**
 * アドレスファミリ文字列を取得する。
 * @param addr ソケットアドレス。
 * @retval ソケットアドレス文字列。
 */
std::string
ipmsg::getSockAddrInAddressFamilyString( const struct sockaddr_storage &addr )
{
	return getAddressFamilyString( addr.ss_family );
}

/**
 * アドレスファミリ文字列を取得する。
 * @param family ソケットファミリ。
 * @retval ソケットアドレス文字列。
 */
std::string
ipmsg::getAddressFamilyString( int family )
{
	if ( family == AF_INET ) {
		return "IPv4";
	} else if ( family == AF_INET6 ) {
		return "IPv6";
	} else {
		return "other";
	}
}

/**
 * 同じソケットアドレスかを判定する。
 * @param base ソケットアドレス。
 * @param check ソケットアドレス。
 * @retval true:同一。
 * @retval false:同一でない。
 */
bool
ipmsg::isSameSockAddrIn( struct sockaddr_storage base, struct sockaddr_storage check )
{
	return memcmp( &base, &check, sizeof( struct sockaddr_storage ) ) == 0;
}

/**
 * ファイルバッファ送信メソッド。
 * <ul>
 * <li>sendfileがサポートされているプラットフォームではsendfileシステムコールを使って高速化。ケースバイケースだが実装によってはゼロコピーになる。<br>
 * でも、テストが出来ませんねぇ。とりあえず、hp-uxはデフォルトのread/write実装を使うので遅い。Linuxでは4-5MBのファイルだと2倍違うこともある</li>
 * </ul>
 */
int
ipmsg::IpMsgSendFileBuffer( int ifd, int sock, int size )
{
#if defined(SUPPORT_SENDFILE_LINUX_STYLE)
	//Linux用
	//printf("sendfile as sendfile syscall by linux\n");
	return sendfile( sock, ifd, NULL, size );
#elif defined(SUPPORT_SENDFILE_BSD_STYLE)
	//printf("sendfile as sendfile syscall by freebsd\n");
	//FreeBSD用
	return sendfile( sock, ifd, NULL, size, NULL, NULL, 0 );
/*
// TODO solaris support start from here.
#elif defined( SUPPORT_SENDFILE_SOLARIS_STYLE )
// TODO hp-ux support start from here.
#if 0
//#elif defined(SUPPORT_SENDFILE_HPUX_STYLE)
	printf("sendfile as sendfile syscall by hp-ux\n");
	//HP-UX用
	return sendfile( sock, ifd, NULL, size, NULL, 0 );
#endif
// TODO hp-ux support end.
*/
#else
	//printf("sendfile as read write\n");
	//デフォルト実装
	char readbuf[8192];
	int readSize = read( ifd, readbuf, sizeof( readbuf ) );
	if ( readSize > 0 ){
		return send( sock, readbuf, readSize, 0 );
	}
	return -1;
#endif // SUPPORT_SENDFILE_LINUX, SUPPORT_SENDFILE_FREEBSD
}

/**
 * 同じネットワークに属しているかを判定するメソッド。
 * <ul>
 * <li>アドレスがNICと同じネットワークに属しているかを判定。
 * </ul>
 * @param addr 判定するIPアドレス。
 * @param ifnetaddr NICのネットワークアドレス。
 * @param netmask ネットマスク。
 * @retval true:同じネットワーク。
 * @retval false:違うネットワーク。
 */
bool
ipmsg::isSameNetwork( const struct sockaddr_storage *addr, std::string ifnetaddr, std::string netmask )
{
	bool ret = false;
	sockaddr_storage ifnet, mask;
	memcpy( &ifnet, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &ifnet, ifnetaddr, IPMSG_DEFAULT_PORT ) == NULL ) {
		return ret;
	}
	memcpy( &mask, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &mask, netmask, IPMSG_DEFAULT_PORT ) == NULL ) {
		return ret;
	}
	if ( addr->ss_family == AF_INET ) {
		const struct sockaddr_in *addrp = (const struct sockaddr_in *)addr;
		in_addr sin_addr = addrp->sin_addr;
		const struct sockaddr_in *ifnetp = (const struct sockaddr_in *)&ifnet;
		in_addr ifnet_addr = ifnetp->sin_addr;
		const struct sockaddr_in *maskp = (const struct sockaddr_in *)&mask;
		in_addr mask_addr = maskp->sin_addr;
		ret = ifnet_addr.s_addr == sin_addr.s_addr & mask_addr.s_addr;
	} else if ( addr->ss_family == AF_INET6 ) {
		const struct sockaddr_in6 *addrp = ( const struct sockaddr_in6 *) addr;
		in6_addr sin_addr = addrp->sin6_addr;
		const struct sockaddr_in6 *ifnetp = ( struct sockaddr_in6 *)&ifnet;
		in6_addr ifnet_addr = ifnetp->sin6_addr;
		ret = sin_addr.s6_addr[0] == ifnet_addr.s6_addr[0]
		   && sin_addr.s6_addr[1] == ifnet_addr.s6_addr[1]
		   && sin_addr.s6_addr[2] == ifnet_addr.s6_addr[2]
		   && sin_addr.s6_addr[3] == ifnet_addr.s6_addr[3]
		   && sin_addr.s6_addr[4] == ifnet_addr.s6_addr[4]
		   && sin_addr.s6_addr[5] == ifnet_addr.s6_addr[5]
		   && sin_addr.s6_addr[6] == ifnet_addr.s6_addr[6]
		   && sin_addr.s6_addr[7] == ifnet_addr.s6_addr[7];
	}
	return ret;
}

#ifdef HAVE_GETIFADDRS
/**
 * ネットワークインターフェースの情報を取得する。
 * @param AddressFamily アドレスファミリ
 * @param nics ネットワークインターフェースカードの情報。
 * @param defaultPort デフォルトポート
 */
void
ipmsg::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
// getifaddrsを使う実装も用意。その方が行儀が良い。
#if defined(DEBUG) || !defined(NDEBUG)
printf( "getifaddr ver\n" );fflush(stdout);
#endif
	struct ifaddrs *ifap0, *ifap;

	int err = getifaddrs( &ifap0 );
	if ( err != 0 ){
		fprintf(stderr, "getifaddrs:%s\n", gai_strerror( err ) );
		return;
	}

	std::string v4localLoopbackAddress = "127.0.0.1";
	std::string v4nullAddress = "0.0.0.0";
	std::string v4broadcastAddress = "255.255.255.255";
	std::string v6localLoopbackAddress = "::1";
	std::string v6nullAddress = "::";
	std::string v6broadcastAddress = "ff02::1";

	for( ifap = ifap0; ifap; ifap = ifap->ifa_next ) {
		if ( ifap->ifa_addr == NULL ) {
			continue;
		}
		if ( ifap->ifa_addr->sa_family == AF_INET ) {
			std::string rawAddress = getSockAddrInRawAddress( ( struct sockaddr_storage * )ifap->ifa_addr );
			if ( rawAddress == v4localLoopbackAddress || rawAddress == v4nullAddress || rawAddress == v4broadcastAddress ){
				continue;
			}
			std::string rawNetMask = getSockAddrInRawAddress( ( struct sockaddr_storage * )ifap->ifa_netmask );
			NetworkInterface ni( AF_INET, std::string( ifap->ifa_name ) );
			ni.setPortNo( defaultPortNo );
			ni.setIpAddress( rawAddress );
			ni.setNetMask( rawNetMask );
#if defined(DEBUG) || !defined(NDEBUG)
printf( "getifaddr ver(IPv4)\n" );fflush(stdout);
printf( "  IP %s\n", ni.IpAddress().c_str() );fflush(stdout);
printf( "  NM %s\n", ni.NetMask().c_str() );fflush(stdout);
printf( "  NA %s\n", ni.NetworkAddress().c_str() );fflush(stdout);
printf( "  BA %s\n", ni.BroadcastAddress().c_str() );fflush(stdout);
#endif
			nics.push_back( ni );
		} else if ( ifap->ifa_addr->sa_family == AF_INET6 ) {
			std::string rawAddress = getSockAddrInRawAddress( ( struct sockaddr_storage * )ifap->ifa_addr );
			if ( rawAddress == v6localLoopbackAddress || rawAddress == v6nullAddress || rawAddress == v6broadcastAddress ){
				continue;
			}
			std::string rawNetMask = getSockAddrInRawAddress( ( struct sockaddr_storage * )ifap->ifa_netmask );
			NetworkInterface ni( AF_INET6, std::string( ifap->ifa_name ) );
			ni.setPortNo( defaultPortNo );
			ni.setIpAddress( rawAddress );
			ni.setNetMask( rawNetMask );
#if defined(DEBUG) || !defined(NDEBUG)
printf( "getifaddr ver(IPv6)\n" );fflush(stdout);
printf( "  IP %s\n", ni.IpAddress().c_str() );fflush(stdout);
printf( "  NM %s\n", ni.NetMask().c_str() );fflush(stdout);
printf( "  NA %s\n", ni.NetworkAddress().c_str() );fflush(stdout);
printf( "  BA %s\n", ni.BroadcastAddress().c_str() );fflush(stdout);
#endif
			nics.push_back( ni );
		}
	}
	return;
}
#else
/**
 * ネットワークインターフェースの情報を取得する。
 * @param AddressFamily アドレスファミリ
 * @param nics ネットワークインターフェースカードの情報。
 * @param defaultPort デフォルトポート
 * ※getifaddrsが有る場合は使用しない。
 */
void
ipmsg::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
	GetNetworkInterfaceInfoForIPv4( nics, defaultPortNo );
	GetNetworkInterfaceInfoForIPv6( nics, defaultPortNo );
}

/**
 * ネットワークインターフェースの情報を取得する。(IPv4専用)
 * @param nics ネットワークインターフェースカードの情報。
 * @param defaultPort デフォルトポート
 * ※getifaddrsが有る場合は使用しない。
 */
void
ipmsg::GetNetworkInterfaceInfoForIPv4( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
	/* ローカルループバックをのぞく全てのIPアドレスが対象 */
	/* 全てのNICを取得する(より移植性が高い方法) */
	std::string localLoopbackAddress = "127.0.0.1";
	std::string nullAddress = "0.0.0.0";
	std::string broadcastAddress = "255.255.255.255";
	//情報取得のためのソケットを作成
	int fd;
	fd = socket( AF_INET, SOCK_DGRAM, 0 );

	struct if_nameindex *p0 = NULL, *p = NULL;
	for( p0 = p = if_nameindex(); p->if_index > 0; p++ ){
		struct ifreq ifr;
		memset( &ifr, 0, sizeof( ifr ) );

		ifr.ifr_addr.sa_family = AF_INET;
		strncpy(ifr.ifr_name, p->if_name, IFNAMSIZ-1);
		ioctl(fd, SIOCGIFADDR, &ifr);
		struct in_addr ipAddr = ( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr;
		char ipaddrbuf[IP_ADDR_MAX_SIZE];
		inet_ntop( AF_INET, &ipAddr, ipaddrbuf, sizeof( ipaddrbuf ) );
		std::string rawAddress = ipaddrbuf;
		if ( rawAddress == localLoopbackAddress || rawAddress == nullAddress || rawAddress == broadcastAddress ){
			continue;
		}
		std::string rawNetMask("");

		ioctl(fd, SIOCGIFNETMASK, &ifr);
		struct in_addr netMask = ( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr;
		memset( ipaddrbuf, 0, sizeof( ipaddrbuf ) );
		inet_ntop( AF_INET, &netMask, ipaddrbuf, sizeof( ipaddrbuf ) );
		rawNetMask = ipaddrbuf;

		NetworkInterface ni( AF_INET, std::string( ifr.ifr_name ) );
		ni.setPortNo( defaultPortNo );
		ni.setIpAddress( rawAddress );
		ni.setNetMask( rawNetMask );
		nics.push_back( ni );
#if defined(DEBUG) || !defined(NDEBUG)
		printf( "NIC device=%s[IpAddress=%s][Port=%d][NetMask=%s][NetworkAddress=%s]\n",
						nics[nics.size() - 1].DeviceName().c_str(),
						nics[nics.size()-1].IpAddress().c_str(),
						nics[nics.size()-1].PortNo(),
						nics[nics.size()-1].NetMask().c_str(),
						nics[nics.size()-1].NetworkAddress().c_str());fflush( stdout );
#endif
	}
	if_freenameindex( p0 );
	//情報取得のためのソケットを閉じる。
	close(fd);
}

/**
 * ネットワークインターフェースの情報を取得する。(IPv6専用)
 * @param nics ネットワークインターフェースカードの情報。
 * @param defaultPort デフォルトポート
 * ※getifaddrsが有る場合は使用しない。
 */
void
ipmsg::GetNetworkInterfaceInfoForIPv6( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
	std::string localLoopbackAddress = "::1";
	std::string nullAddress = "::";
	std::string broadcastAddress = "ff02::1";
	FILE *fp = fopen( "/proc/net/if_inet6", "r" );
	if ( fp != NULL ) {
		char v6addr[8][4+1];
		char devname[8+1];
		int dmy1,dmy2,dmy3,dmy4;
		while( !feof( fp ) ) {
			fscanf( fp, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %s\n", 
					v6addr[0], v6addr[1], v6addr[2], v6addr[3],
					v6addr[4], v6addr[5], v6addr[6], v6addr[7],
					&dmy1, &dmy2, &dmy3, &dmy4, devname );
			char ipaddrbuf[IP_ADDR_MAX_SIZE + 100];
			snprintf( ipaddrbuf, sizeof( ipaddrbuf ), "%s:%s:%s:%s:%s:%s:%s:%s",
					v6addr[0], v6addr[1], v6addr[2], v6addr[3],
					v6addr[4], v6addr[5], v6addr[6], v6addr[7] );
			//アドレスを最適化
			struct sockaddr_storage ss;
			memset( &ss, 0, sizeof( ss ) );
			ss.ss_family = AF_INET6;
			if ( createSockAddrIn( &ss, ipaddrbuf, IPMSG_DEFAULT_PORT, devname ) == NULL ) {
				continue;
			}
			std::string rawAddress = getSockAddrInRawAddress( &ss );
			if ( rawAddress == localLoopbackAddress || rawAddress == nullAddress || rawAddress == broadcastAddress ){
				continue;
			}
			NetworkInterface ni( AF_INET6, std::string( devname ) );
			ni.setPortNo( defaultPortNo );
			ni.setIpAddress( rawAddress );
			ni.setNetMask( "ffff:ffff:ffff:ffff::" );
			nics.push_back( ni );
		}
		fclose( fp );
	}
}
#endif

/**
 * ブロードキャストアドレスを取得する。
 * @param netAddress ネットワークアドレス。
 * @param netmask ネットマスク。
 * @retval ブロードキャストアドレス。
 */
std::string
ipmsg::GetBroadcastAddress( int family, std::string netAddress, std::string netmask )
{
	if ( family == AF_INET ) {
		in_addr inetBroad, inetMask, inetNetwork;
		inet_pton( family, netAddress.c_str(), (void *)&inetNetwork );
		inet_pton( family, netmask.c_str(), (void *)&inetMask );

		inetBroad.s_addr = inetNetwork.s_addr | ( 0xffffffff ^ inetMask.s_addr );

		char ipaddrbuf[IP_ADDR_MAX_SIZE];
		inet_ntop( family, &inetBroad, ipaddrbuf, sizeof( ipaddrbuf ) );
#if defined(DEBUG) || defined(INFO)
		printf( "BROADCAST %s = ", ipaddrbuf );fflush(stdout);
		printf( "NERADDR %s | ", netAddress.c_str() );fflush(stdout);
		printf( "0xffffffff ^ NETMASK %s\n", netmask.c_str() );fflush(stdout);
#endif
		return ipaddrbuf;
	} else if ( family == AF_INET6 ) {
		//TODO
		return "ff02::1";
	}
	return "";
}

/**
 * ネットワークアドレスを取得する。
 * @param family アドレスファミリ
 * @param rawAddress IPアドレス。
 * @param netmask ネットマスク。
 * @retval ネットワークアドレス。
 */
std::string
ipmsg::GetNetworkAddress( int family, std::string rawAddress, std::string netmask )
{
	if ( family == AF_INET ) {
		in_addr inetAddr, inetMask, inetNetwork;
		inet_pton( family, rawAddress.c_str(), (void *)&inetAddr );
		inet_pton( family, netmask.c_str(), (void *)&inetMask );
		inetNetwork.s_addr = inetAddr.s_addr & inetMask.s_addr;

		char ipaddrbuf[IP_ADDR_MAX_SIZE];
		inet_ntop( family, &inetNetwork, ipaddrbuf, sizeof( ipaddrbuf ) );
		return ipaddrbuf;
	} else if ( family == AF_INET6 ) {
		in6_addr inetNetwork;
		inet_pton( family, rawAddress.c_str(), (void *)&inetNetwork );

		inetNetwork.s6_addr[8] = 0x0U;
		inetNetwork.s6_addr[9] = 0x0U;
		inetNetwork.s6_addr[10] = 0x0U;
		inetNetwork.s6_addr[11] = 0x0U;
		inetNetwork.s6_addr[12] = 0x0U;
		inetNetwork.s6_addr[13] = 0x0U;
		inetNetwork.s6_addr[14] = 0x0U;
		inetNetwork.s6_addr[15] = 0x0U;

		char ipaddrbuf[IP_ADDR_MAX_SIZE];
		inet_ntop( family, &inetNetwork, ipaddrbuf, sizeof( ipaddrbuf ) );
		return ipaddrbuf;
	}
	return "";
}
