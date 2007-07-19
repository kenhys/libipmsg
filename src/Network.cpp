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
#include <sys/ioctl.h>
#include <net/if_arp.h>
#ifndef SIOCGARP
#include <sys/sysctl.h>
#include <netinet/if_ether.h>
#include <net/route.h>
#endif
#ifndef SIOCGIFHWADDR
#include <net/if_dl.h>
#include <sys/sockio.h>
#endif

#if defined(__linux__)
    #include <linux/version.h>
    #if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
        #define SUPPORT_SENDFILE_LINUX_STYLE
        #include <sys/sendfile.h>
    #endif // LINUX_VERSION_CODE
#endif // __linux__
#if defined(__FreeBSD__) || defined(__DragonFly__)
    #define SUPPORT_SENDFILE_BSD_STYLE
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/uio.h>
#endif // __FreeBSD__,__DragonFly__
#if defined(__sun)
    #define SUPPORT_SENDFILE_SOLARIS_STYLE
#endif // __solaris__
#if defined(__HPUX__)
    #define SUPPORT_SENDFILE_HPUX_STYLE
#endif // __HPUX__

/**
 * ソケットを生成し、バインドする。
 * @param proto プロトコル(SOCK_STREAM, SOCK_DGRAM)。
 * @param addr ソケットアドレス。
 * @param devname デバイス名。
 * @retval ソケット。
 */
int
ipmsg::bindSocket( int proto, struct sockaddr_storage addr, const char *devname )
{
	IPMSG_FUNC_ENTER( "int ipmsg::bindSocket( int proto, struct sockaddr_storage addr, const char *devname )" );
	int sock = socket( addr.ss_family, proto, 0 );
#ifdef ENABLE_IPV6
	const int on = 1;
	if ( sock >= 0 && addr.ss_family == AF_INET6 && setsockopt( sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof( on ) ) < 0 ){
		perror("setsockopt(udp,IPV6)");
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
#endif // ENABLE_IPV6
	IpMsgDumpAddr( &addr );
	int sz = sizeof( struct sockaddr_storage );
#ifdef ENABLE_IPV4
	if ( addr.ss_family == AF_INET ){
		sz = sizeof( struct sockaddr_in );
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( addr.ss_family == AF_INET6 ){
		sz = sizeof( struct sockaddr_in6 );
	}
#endif // ENABLE_IPV6
	if ( sock >= 0 && bind(sock, (struct sockaddr *)&addr, sz) != 0 ){
		perror("bind(udp)");
		fprintf( stderr, "  ip addr=%s,port=%u,dev=%s\n", getSockAddrInRawAddress( addr ).c_str(), getSockAddrInPortNo( addr ), devname );fflush( stdout );
		close( sock );
		IPMSG_FUNC_RETURN( -1 );
	}
#ifdef ENABLE_IPV6
	if ( sock >= 0 && proto == SOCK_DGRAM && addr.ss_family == AF_INET6 && devname != NULL ){
		const unsigned int devindex = if_nametoindex( devname );
		if ( setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &devindex, sizeof(devindex)) != 0 ) {
			close( sock );
			IPMSG_FUNC_RETURN( -1 );
		}
	}
#endif // ENABLE_IPV6
#ifdef ENABLE_IPV4
	if ( sock >= 0 && proto == SOCK_DGRAM && addr.ss_family == AF_INET ){
		const int yes = 1;
		if ( setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) != 0 ) {
			perror("setsockopt(broadcast)");
			close( sock );
			IPMSG_FUNC_RETURN( -1 );
		}
	}
#endif // ENABLE_IPV4
	IPMSG_FUNC_RETURN( sock );
}


/**
 * ソケットアドレスにパケットを送信する。
 * @param sock ソケット。
 * @param buf 送信バッファ。
 * @param size 送信バッファのサイズ。
 * @param addr ソケットアドレスのポインタ。
 * @retval sendtoの戻り値。
 */
int
ipmsg::sendToSockAddrIn( int sock, const char *buf, const int size, const struct sockaddr_storage *addr )
{
	IPMSG_FUNC_ENTER( "int ipmsg::sendToSockAddrIn( int sock, const char *buf, const int size, const struct sockaddr_storage *addr )" );
	int sz = sizeof( struct sockaddr_storage );
#ifdef ENABLE_IPV4
	if ( addr->ss_family == AF_INET ) {
		sz = sizeof( struct sockaddr_in );
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( addr->ss_family == AF_INET6 ) {
#ifdef SIN6_LEN
		const struct sockaddr_in6 *addrp = ( const struct sockaddr_in6 * )addr;
		sz = addrp->sin6_len;
#else
		sz = sizeof( struct sockaddr_in6 );
#endif
	}
#endif // ENABLE_IPV6
#ifdef DEBUG
	printf( "sendToSockAddrIn sock = %d\n", sock );
#endif
	IpMsgDumpAddr( addr );
	ssize_t ret = sendto( sock, buf, size + 1, 0, ( const struct sockaddr * )addr, sz );
	IPMSG_FUNC_RETURN( ret );
}

int
ipmsg::getScopeId( struct sockaddr_storage *addr )
{
#ifdef ENABLE_IPV6
	if ( addr->ss_family == AF_INET6 ){
		struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)addr;
		return in6->sin6_scope_id;
	}
#endif // ENABLE_IPV6
	return -1;
}

/**
 * ソケットアドレスがローカルループバックかを判定する。
 * @param addr ソケットアドレスのポインタ。
 * @retval TRUE:ローカルループバック。
 * @retval FALSE:ローカルループバックで無い。
 */
bool
ipmsg::isLocalLoopbackAddress( struct sockaddr_storage *addr )
{
	IPMSG_FUNC_ENTER( "bool ipmsg::isLocalLoopbackAddress( struct sockaddr_storage *addr )" );
	if ( addr != NULL ) {
		struct addrinfo hints, *res;
		memset( &hints, 0, sizeof( hints ) );
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_NUMERICHOST;

		int err = getaddrinfo( getSockAddrInRawAddress( addr ).c_str(), NULL, &hints, &res );
		if ( err != 0 ){
			fprintf(stderr, "getaddrinfo(addr=[%s]):%s\n",
							getSockAddrInRawAddress( addr ).c_str(), gai_strerror( err ) );
			IPMSG_FUNC_RETURN( false );
		}
		struct addrinfo *res_i;
		for( res_i = res; res_i; res_i = res_i->ai_next ) {
			if ( getSockAddrInRawAddress( (struct sockaddr_storage * )res_i->ai_addr ) == "127.0.0.1" ||
				 getSockAddrInRawAddress( (struct sockaddr_storage * )res_i->ai_addr ) == "::1" ) {
				freeaddrinfo( res );
#ifdef DEBUG
printf("THIS IS LOCAL LOOPBACK ADDRESS[%s]\n", getSockAddrInRawAddress( addr ).c_str() );
#endif
				IPMSG_FUNC_RETURN( true );
			}
		}
		freeaddrinfo( res );
	}
	IPMSG_FUNC_RETURN( false );
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
	IPMSG_FUNC_ENTER( "struct sockaddr_storage *ipmsg::createSockAddrIn( struct sockaddr_storage *addr, std::string rawAddress, int port, const char *devname )" );
	if ( rawAddress.empty() ) {
		IPMSG_FUNC_RETURN( NULL );
	}
#ifdef DEBUG
//	printf( "createSockAddrIn(addr=[%s] port=[%u] devname[%s])\n", rawAddress.c_str(), port, devname );fflush(stdout);
#endif
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
			IPMSG_FUNC_RETURN( NULL );
		}
#ifdef ENABLE_IPV4
		if ( res->ai_family == AF_INET ) {
			struct sockaddr_in *sockaddrp = ( struct sockaddr_in * )addr;
			*sockaddrp = *( (struct sockaddr_in *)res->ai_addr );
#ifdef DEBUG
/*
			struct addrinfo *res_i;
			int i = 1;
			for( res_i = res; res_i; res_i = res_i->ai_next,i++ ) {
				fprintf(stderr, "%2d:getaddrinfo(IPv4 addr=[%s] port=[%u]\n", i,
							getSockAddrInRawAddress( (struct sockaddr_storage * )res_i->ai_addr ).c_str(), getSockAddrInPortNo( (struct sockaddr_storage * )res_i->ai_addr ) );
			}
			fflush(stdout);
*/
#endif
			freeaddrinfo( res );
			IPMSG_FUNC_RETURN( addr );
		} 
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
		if ( res->ai_family == AF_INET6 ) {
			struct sockaddr_in6 *sockaddrp = ( struct sockaddr_in6 * )addr;
			*sockaddrp = *( (struct sockaddr_in6 *)res->ai_addr );
			if ( devname != NULL ) {
				sockaddrp->sin6_scope_id = if_nametoindex( devname );
			}
#ifdef DEBUG
/*
			struct addrinfo *res_i;
			int i = 1;
			for( res_i = res; res_i; res_i = res_i->ai_next, i++ ) {
				fprintf(stderr, "%2d:getaddrinfo(IPv6 addr=[%s] port=[%u]\n", i,
							getSockAddrInRawAddress( (struct sockaddr_storage * )res_i->ai_addr ).c_str(), getSockAddrInPortNo( (struct sockaddr_storage * )res_i->ai_addr ) );
			}
			fflush(stdout);
*/
#endif
			freeaddrinfo( res );
			IPMSG_FUNC_RETURN( addr );
		}
#endif // ENABLE_IPV6
		else {
			fprintf( stderr, "createSockAddrIn::unknown address family\n" );
		}
		freeaddrinfo( res );
	} else {
		fprintf( stderr, "createSockAddrIn::addr is null\n" );
	}
	IPMSG_FUNC_RETURN( NULL );
}

std::string
ipmsg::getNetworkInterfaceMacAddress( std::string deviceName )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::getNetworkInterfaceMacAddress( std::string deviceName )");
	char retbuf[20]={0};
#ifdef SIOCGIFHWADDR
	int sock = socket( AF_INET, SOCK_DGRAM, 0 );
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy( ifr.ifr_name, deviceName.c_str(), IFNAMSIZ - 1 );
	errno = 0;
	int rc = ioctl( sock, SIOCGIFHWADDR, &ifr );
	if ( rc == -1 ) {
		int err = errno;
		fprintf( stderr, "ioctl in getNetworkInterfaceMacAddress:%s:%s\n", deviceName.c_str(), strerror( err ) );
	} else {
		convertMacAddressToBuffer( ( unsigned char * )&ifr.ifr_hwaddr.sa_data[0], retbuf, sizeof( retbuf ) );
	}
	close( sock );
#elif defined( HAVE_GETIFADDRS )
	struct ifaddrs *ifa, *ifa0;
	struct sockaddr_dl* dl;
	unsigned char mac_addr[6];
	bool is_found = false;
	getifaddrs( &ifa0 );
	for( ifa = ifa0; ifa; ifa=ifa->ifa_next ) {
		dl = (struct sockaddr_dl*)ifa->ifa_addr;
		if( strncmp( deviceName.c_str(), dl->sdl_data, dl->sdl_nlen) == 0 ) {
			memcpy( mac_addr, LLADDR(dl), 6 );
			is_found = true;
			break;
		}
	}
	freeifaddrs(ifa0);
	if ( is_found ) {
		convertMacAddressToBuffer( ( unsigned char * )mac_addr, retbuf, sizeof( retbuf ) );
	}
#endif
	IPMSG_FUNC_RETURN( retbuf );
}

char *
ipmsg::convertMacAddressToBuffer( const unsigned char *mac, char *buf, int bufsize )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::convertMacAddressToBuffer( const unsigned char *mac, char *buf, int bufsize )");
	snprintf( buf, bufsize, "%02x:%02x:%02x:%02x:%02x:%02x",
				*mac, *( mac + 1 ), *( mac + 2 ), *( mac + 3 ), *( mac + 4 ), *( mac + 5 ) );
//printf("MAC=%s\n", buf );
	IPMSG_FUNC_RETURN( buf );
}

std::string
ipmsg::convertIpAddressToMacAddress( std::string ipAddress, const std::vector<NetworkInterface> &nics )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::convertIpAddressToMacAddress( std::string ipAddress )");
	sockaddr_storage ip;
	if ( createSockAddrIn( &ip, ipAddress, 0 ) == NULL ) {
		IPMSG_FUNC_RETURN( "" );
	}
	char retbuf[20]={0};
	if ( ip.ss_family == AF_INET ) {
#ifdef SIOCGARP
//printf( "ioctl\n" );
		int sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
		struct sockaddr_in *sockaddrp = ( struct sockaddr_in * )&ip;
		struct arpreq arpreq;
		struct sockaddr_in *sin;

		memset( &arpreq, 0, sizeof( arpreq ) );
		sin = ( struct sockaddr_in * )&arpreq.arp_pa;
		sin->sin_family = AF_INET;
		sin->sin_addr = sockaddrp->sin_addr;
#ifdef __linux__
		for( unsigned int i = 0; i < nics.size(); i++ ) {
			if ( isSameNetwork( &ip, nics[i].NetworkAddress(), nics[i].NetMask() ) ) {
				strcpy( arpreq.arp_dev, nics[i].DeviceName().c_str() );
				break;
			}
		}
#endif
		errno = 0;
		int rc = ioctl( sockfd, SIOCGARP, &arpreq );
//printf( "ARP dev=%s\n", arpreq.arp_dev );
		if ( rc == -1 ) {
			int err = errno;
			for( unsigned int i = 0; i < nics.size(); i++ ){
				if ( nics[i].IpAddress() == ipAddress ) {
					close( sockfd );
					IPMSG_FUNC_RETURN( nics[i].HardwareAddress() );
				}
			}
			fprintf( stderr, "ioctl in convertIpAddressToMacAddress:%s:%s\n", ipAddress.c_str(),strerror( err ) );
		} else {
			convertMacAddressToBuffer( ( unsigned char * )&arpreq.arp_ha.sa_data[0], retbuf, sizeof( retbuf ) );
		}
		close( sockfd );
//printf( "MAC Addr v4 [%s] <= %s\n", retbuf, ipAddress.c_str() );
		IPMSG_FUNC_RETURN( retbuf );
#else	// FreeBSD, etc...
//printf( "sysctl\n" );
		int s, mib[6];
		size_t len;
		char *buf;
 
		mib[0] = CTL_NET;
		mib[1] = PF_ROUTE;
		mib[2] = 0;
		mib[3] = AF_INET;
		mib[4] = NET_RT_FLAGS;
		mib[5] = RTF_LLINFO;
 
		if ( sysctl(mib, 6, NULL, &len, NULL, 0 ) < 0 ) {
//perror( "sysctl 1" );
			IPMSG_FUNC_RETURN( "" );
		}
		if( ( buf = (char*)malloc(len)) == NULL ) {
//perror( "malloc" );
			IPMSG_FUNC_RETURN( "" );
		}
		if ( sysctl( mib, 6, buf, &len, NULL, 0 ) < 0 ){
//perror( "sysctl 2" );
			free( buf );
			IPMSG_FUNC_RETURN( "" );
		}
		struct sockaddr_in *sin = ( struct sockaddr_in * )&ip;
		struct rt_msghdr *rtm;
		for( s = 0; s < ( int )len; s += rtm->rtm_msglen ){
			rtm = ( struct rt_msghdr * )( buf + s );
			struct sockaddr_inarp *inarp = ( struct sockaddr_inarp * )( rtm + 1 );
			struct sockaddr_dl *sdl = ( struct sockaddr_dl * )( inarp + 1 ); 

//printf( "sin=%08x inarp=%08x\n", sin->sin_addr.s_addr, inarp->sin_addr.s_addr );
			if( sin->sin_addr.s_addr == inarp->sin_addr.s_addr ){
				convertMacAddressToBuffer( ( const unsigned char * )LLADDR( sdl ), retbuf, sizeof( retbuf ) );
//printf( "MAC Addr v4 [%s] <= %s\n", retbuf, ipAddress.c_str() );
				free( buf );
				IPMSG_FUNC_RETURN( retbuf );
			}
		}
		free( buf );
		IPMSG_FUNC_RETURN( "" );
#endif
	}
	if ( ip.ss_family == AF_INET6 ) {
		struct sockaddr_in6 *sockaddrp = ( struct sockaddr_in6 * )&ip;
		unsigned char mac[6];
		mac[0] = sockaddrp->sin6_addr.s6_addr[8] ^ 0x02;
		mac[1] = sockaddrp->sin6_addr.s6_addr[9];
		mac[2] = sockaddrp->sin6_addr.s6_addr[10];
		mac[3] = sockaddrp->sin6_addr.s6_addr[13];
		mac[4] = sockaddrp->sin6_addr.s6_addr[14];
		mac[5] = sockaddrp->sin6_addr.s6_addr[15];
		convertMacAddressToBuffer( mac, retbuf, sizeof( retbuf ) );
//printf( "MAC Addr v6 [%s] <= %s\n", retbuf, ipAddress.c_str() );
		IPMSG_FUNC_RETURN( retbuf );
	}
	IPMSG_FUNC_RETURN( "" );
}

/**
 * ソケットアドレスからポート番号を取得する。
 * @param addr ソケットアドレスのポインタ。
 * @retval ポート番号（ネットワークバイトオーダ）。
 */
int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage *addr )
{
	IPMSG_FUNC_ENTER( "int ipmsg::getSockAddrInPortNo( const struct sockaddr_storage *addr )" );
	IPMSG_FUNC_RETURN( getSockAddrInPortNo( *addr ) );
}

/**
 * ソケットアドレスからポート番号を取得する。
 * @param addr ソケットアドレス。
 * @retval ポート番号（ネットワークバイトオーダ）。
 */
int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage &addr )
{
	IPMSG_FUNC_ENTER( "int ipmsg::getSockAddrInPortNo( const struct sockaddr_storage &addr )" );
	int ret = -1;
#ifdef ENABLE_IPV4
	if ( addr.ss_family == AF_INET ) {
		const struct sockaddr_in *addrp = (const struct sockaddr_in *)&addr;
		ret = addrp->sin_port;
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( addr.ss_family == AF_INET6 ) {
		const struct sockaddr_in6 *addrp = (const struct sockaddr_in6 *)&addr;
		ret = addrp->sin6_port;
	}
#endif // ENABLE_IPV6
	IPMSG_FUNC_RETURN( ret );
}

std::string
ipmsg::getLocalhostAddress( bool useIPv6, const std::vector<NetworkInterface>& nics )
{
	std::string HostAddress = nics[0].IpAddress();
#ifdef ENABLE_IPV4
	for( unsigned int i = 0; i < nics.size(); i++ ){
		if ( nics[i].AddressFamily() == AF_INET ) {
			HostAddress = nics[i].IpAddress();
			break;
		}
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( useIPv6 ) {
		for( unsigned int i = 0; i < nics.size(); i++ ){
			if ( nics[i].AddressFamily() == AF_INET6 ) {
				HostAddress = nics[i].IpAddress();
				break;
			}
		}
	}
#endif // ENABLE_IPV6
	return HostAddress;
}
/**
 * ソケットアドレスからIPアドレス文字列を取得する。
 * @param addr ソケットアドレスのポインタ。
 * @retval IPアドレス文字列。
 */
std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage *addr )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage *addr )" );
	IPMSG_FUNC_RETURN( getSockAddrInRawAddress( *addr ) );
}

/**
 * ソケットアドレスからIPアドレス文字列を取得する。
 * @param addr ソケットアドレス。
 * @retval IPアドレス文字列。
 */
std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage &addr )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage &addr )" );
	char ipAddrBuf[NI_MAXHOST] = {0};
	const struct sockaddr *addrp = ( const struct sockaddr *)&addr;
#ifdef BSD
	int salen = addrp->sa_len;
#else
	int salen = sizeof( addr );
#endif

	int err = getnameinfo( addrp, salen, ipAddrBuf, sizeof( ipAddrBuf ), NULL, 0, NI_NUMERICHOST );
	if ( err != 0 ) {
#ifdef ENABLE_IPV4
		if ( addrp->sa_family == AF_INET ) {
			const struct sockaddr_in *sin = (const struct sockaddr_in *)addrp;
			const unsigned char *p = (const unsigned char *)&sin->sin_addr.s_addr;
			snprintf( ipAddrBuf, sizeof( ipAddrBuf ), "%d.%d.%d.%d", ( int )p[0], ( int )p[1], ( int )p[2], ( int )p[3] ); 
		} else {
#endif
			printf( "getnameinfo Error:%s\n", gai_strerror( err ) );
#ifdef ENABLE_IPV4
		}
#endif
#ifdef DEBUG
/*
	} else {
		printf( "getnameinfo:IP addr %s\n", ipAddrBuf );
*/
#endif
	}
	IPMSG_FUNC_RETURN( ipAddrBuf );
}

/**
 * アドレスファミリ文字列を取得する。
 * @param addr ソケットアドレス。
 * @retval ソケットアドレス文字列。
 */
std::string
ipmsg::getSockAddrInAddressFamilyString( const struct sockaddr_storage &addr )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::getSockAddrInAddressFamilyString( const struct sockaddr_storage &addr )" );
	IPMSG_FUNC_RETURN( getAddressFamilyString( addr.ss_family ) );
}

/**
 * アドレスファミリ文字列を取得する。
 * @param family ソケットファミリ。
 * @retval ソケットアドレス文字列。
 */
std::string
ipmsg::getAddressFamilyString( int family )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::getAddressFamilyString( int family )" );
#ifdef ENABLE_IPV4
	if ( family == AF_INET ) {
		IPMSG_FUNC_RETURN( "IPv4" );
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( family == AF_INET6 ) {
		IPMSG_FUNC_RETURN( "IPv6" );
	}
#endif // ENABLE_IPV6
	IPMSG_FUNC_RETURN( "other" );
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
	IPMSG_FUNC_ENTER( "bool ipmsg::isSameSockAddrIn( struct sockaddr_storage base, struct sockaddr_storage check )" );
	bool ret = false;
	if ( base.ss_family == check.ss_family ) {
#ifdef ENABLE_IPV4
		if ( base.ss_family == AF_INET ) {
			struct sockaddr_in *basep = ( struct sockaddr_in *)&base;
			struct sockaddr_in *checkp = ( struct sockaddr_in *)&check;
			ret = basep->sin_addr.s_addr == checkp->sin_addr.s_addr && basep->sin_port == checkp->sin_port;
		}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
		if ( base.ss_family == AF_INET6 ) {
			struct sockaddr_in6 *basep = ( struct sockaddr_in6 *)&base;
			struct sockaddr_in6 *checkp = ( struct sockaddr_in6 *)&check;
			ret = memcmp( &basep->sin6_addr, &checkp->sin6_addr, 16 ) == 0 && basep->sin6_port == checkp->sin6_port;
		}
#endif // ENABLE_IPV6
	}
	IPMSG_FUNC_RETURN( ret );
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
	IPMSG_FUNC_ENTER( "int ipmsg::IpMsgSendFileBuffer( int ifd, int sock, int size )" );
#if defined(SUPPORT_SENDFILE_LINUX_STYLE)
	//Linux用
	//printf("sendfile as sendfile syscall by linux\n");
	IPMSG_FUNC_RETURN( sendfile( sock, ifd, NULL, size ) );
#elif defined(SUPPORT_SENDFILE_BSD_STYLE)
	//printf("sendfile as sendfile syscall by freebsd\n");
	//FreeBSD用
	IPMSG_FUNC_RETURN( sendfile( sock, ifd, NULL, size, NULL, NULL, 0 ) );
/*
// TODO solaris support start from here.
#elif defined( SUPPORT_SENDFILE_SOLARIS_STYLE )
// TODO hp-ux support start from here.
#if 0
//#elif defined(SUPPORT_SENDFILE_HPUX_STYLE)
	printf("sendfile as sendfile syscall by hp-ux\n");
	//HP-UX用
	IPMSG_FUNC_RETURN( sendfile( sock, ifd, NULL, size, NULL, 0 ) );
#endif
// TODO hp-ux support end.
*/
#else
	//printf("sendfile as read write\n");
	//デフォルト実装
	char readbuf[8192];
	int readSize = read( ifd, readbuf, sizeof( readbuf ) );
	if ( readSize > 0 ){
		IPMSG_FUNC_RETURN( send( sock, readbuf, readSize, 0 ) );
	}
	IPMSG_FUNC_RETURN( -1 );
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
	IPMSG_FUNC_ENTER( "bool ipmsg::isSameNetwork( const struct sockaddr_storage *addr, std::string ifnetaddr, std::string netmask )" );
	bool ret = false;
	sockaddr_storage ifnet, mask;
	memcpy( &ifnet, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &ifnet, ifnetaddr, IPMSG_DEFAULT_PORT ) == NULL ) {
		IPMSG_FUNC_RETURN( ret );
	}
	if ( addr->ss_family != ifnet.ss_family ) {
		IPMSG_FUNC_RETURN( ret );
	}
	memcpy( &mask, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &mask, netmask, IPMSG_DEFAULT_PORT ) == NULL ) {
		IPMSG_FUNC_RETURN( ret );
	}
	if ( addr->ss_family != mask.ss_family ) {
		IPMSG_FUNC_RETURN( ret );
	}
#ifdef ENABLE_IPV4
	if ( addr->ss_family == AF_INET ) {
		const struct sockaddr_in *addrp = (const struct sockaddr_in *)addr;
		in_addr sin_addr = addrp->sin_addr;
		const struct sockaddr_in *ifnetp = (const struct sockaddr_in *)&ifnet;
		in_addr ifnet_addr = ifnetp->sin_addr;
		const struct sockaddr_in *maskp = (const struct sockaddr_in *)&mask;
		in_addr mask_addr = maskp->sin_addr;
		ret = ifnet_addr.s_addr == ( sin_addr.s_addr & mask_addr.s_addr );
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( addr->ss_family == AF_INET6 ) {
		const struct sockaddr_in6 *addrp = ( const struct sockaddr_in6 *) addr;
		in6_addr sin_addr = addrp->sin6_addr;
		const struct sockaddr_in6 *ifnetp = ( struct sockaddr_in6 *)&ifnet;
		in6_addr ifnet_addr = ifnetp->sin6_addr;

#ifdef BSD
		if ( IN6_IS_ADDR_LINKLOCAL(&sin_addr) ) {
			sin_addr.s6_addr[2] = sin_addr.s6_addr[3] = 0;
		}
		if ( IN6_IS_ADDR_LINKLOCAL(&ifnet_addr) ) {
			ifnet_addr.s6_addr[2] = ifnet_addr.s6_addr[3] = 0;
		}
#endif
		ret = sin_addr.s6_addr[0] == ifnet_addr.s6_addr[0]
		   && sin_addr.s6_addr[1] == ifnet_addr.s6_addr[1]
		   && sin_addr.s6_addr[2] == ifnet_addr.s6_addr[2]
		   && sin_addr.s6_addr[3] == ifnet_addr.s6_addr[3]
		   && sin_addr.s6_addr[4] == ifnet_addr.s6_addr[4]
		   && sin_addr.s6_addr[5] == ifnet_addr.s6_addr[5]
		   && sin_addr.s6_addr[6] == ifnet_addr.s6_addr[6]
		   && sin_addr.s6_addr[7] == ifnet_addr.s6_addr[7];
	}
#endif // ENABLE_IPV6

#ifdef DEBUG
	printf("IS SAME NETWORK( addr=%s ifnetaddr=%s netmask=%s? %s\n", getSockAddrInRawAddress( addr ).c_str(), ifnetaddr.c_str(), netmask.c_str(), ret ? "Yes":"No");
	fflush(stdout);
#endif
	IPMSG_FUNC_RETURN( ret );
}

#ifdef HAVE_GETIFADDRS
/**
 * ネットワークインターフェースの情報を取得する。
 * @param AddressFamily アドレスファミリ
 * @param nics ネットワークインターフェースカードの情報。
 * @param defaultPort デフォルトポート
 */
void
ipmsg::getNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )
{
	IPMSG_FUNC_ENTER( "void ipmsg::getNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )" );
// getifaddrsを使う実装も用意。その方が行儀が良い。
#if defined(DEBUG) || !defined(NDEBUG)
printf( "getifaddr ver\n" );fflush(stdout);
#endif
	struct ifaddrs *ifap0, *ifap;

	int err = getifaddrs( &ifap0 );
	if ( err != 0 ){
		fprintf(stderr, "getifaddrs:%s\n", gai_strerror( err ) );
		IPMSG_FUNC_EXIT;
	}

	std::string v4anyAddress = "0.0.0.0";
	std::string v4broadcastAddress = "255.255.255.255";
	std::string v6anyAddress = "::";
	std::string v6broadcastAddress = "ff02::1";

	for( ifap = ifap0; ifap; ifap = ifap->ifa_next ) {
		if ( ifap->ifa_addr == NULL ) {
			continue;
		}
#if defined( ENABLE_IPV4 ) && defined( ENABLE_IPV6 )
		if ( ifap->ifa_addr->sa_family != AF_INET && ifap->ifa_addr->sa_family != AF_INET6 ) {
//			printf( "ENABLE_IPV4 && ENABLE_IPV6\n" );
#elif defined( ENABLE_IPV4 ) && defined( DISABLE_IPV6 )
		if ( ifap->ifa_addr->sa_family != AF_INET ) {
//			printf( "ENABLE_IPV4 && DISABLE_IPV6\n" );
#elif defined( DISABLE_IPV4 ) && defined( ENABLE_IPV6 )
		if ( ifap->ifa_addr->sa_family != AF_INET6 ) {
//			printf( "DISABLE_IPV4 && ENABLE_IPV6\n" );
#endif // ENABLE_IPV6
			continue;
		}
		if ( ifap->ifa_flags & IFF_LOOPBACK ) {
			continue;
		}
		if ( isLocalLoopbackAddress( ( struct sockaddr_storage * )ifap->ifa_addr ) ) {
			continue;
		}
#if defined( ENABLE_IPV4 ) && defined( ENABLE_IPV6 )
		if ( ifap->ifa_addr->sa_family == AF_INET || ( useIPv6 && ifap->ifa_addr->sa_family == AF_INET6 ) ) {
#elif defined( ENABLE_IPV4 ) && defined( DISABLE_IPV6 )
		if ( ifap->ifa_addr->sa_family == AF_INET ) {
#elif defined( DISABLE_IPV4 ) && defined( ENABLE_IPV6 )
		if ( useIPv6 && ifap->ifa_addr->sa_family == AF_INET6 ) {
#endif // ENABLE_IPV6
			struct sockaddr_storage *addrp = ( struct sockaddr_storage *)ifap->ifa_addr;
			struct sockaddr_storage addr = *addrp;
			std::string rawAddress = getSockAddrInRawAddress( ( struct sockaddr_storage * )ifap->ifa_addr );
#ifdef ENABLE_IPV4
			if ( ifap->ifa_addr->sa_family == AF_INET ) {
				if ( rawAddress == v4anyAddress || rawAddress == v4broadcastAddress ){
					continue;
				}
			}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
			if ( useIPv6 && ifap->ifa_addr->sa_family == AF_INET6 ) {
				if ( rawAddress == v6anyAddress || rawAddress == v6broadcastAddress ){
					continue;
				}
			}
#endif // ENABLE_IPV6
			std::string rawNetMask = getSockAddrInRawAddress( ( struct sockaddr_storage * )ifap->ifa_netmask );
			NetworkInterface ni( ifap->ifa_addr->sa_family, std::string( ifap->ifa_name ) );
			ni.setPortNo( defaultPortNo );
			ni.setIpAddress( rawAddress );
			ni.setNetMask( rawNetMask );
#if defined(DEBUG) || !defined(NDEBUG)
printf( "getifaddr ver(%s)\n", getAddressFamilyString( ifap->ifa_addr->sa_family ).c_str() );
printf( "  IF %s\n", ni.DeviceName().c_str() );
printf( "  IP %s\n", ni.IpAddress().c_str() );
printf( "  HA %s\n", ni.HardwareAddress().c_str() );
printf( "  NM %s\n", ni.NetMask().c_str() );
printf( "  NA %s\n", ni.NetworkAddress().c_str() );
printf( "  BA %s\n", ni.BroadcastAddress().c_str() );
fflush(stdout);
struct sockaddr_storage test;
createSockAddrIn( &test, ni.IpAddress(), ni.PortNo(), ni.DeviceName().c_str() );
#endif
			nics.push_back( ni );
		}
	}
	IPMSG_FUNC_EXIT;
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
ipmsg::getNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )
{
	IPMSG_FUNC_ENTER( "void ipmsg::getNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, bool useIPv6, int defaultPortNo )" );
#ifdef ENABLE_IPV6
	if ( useIPv6 ) {
		getNetworkInterfaceInfoForIPv6( nics, defaultPortNo );
	}
#endif // ENABLE_IPV6
#ifdef ENABLE_IPV4
	getNetworkInterfaceInfoForIPv4( nics, defaultPortNo );
#endif // ENABLE_IPV4
	IPMSG_FUNC_EXIT;
}

/**
 * ネットワークインターフェースの情報を取得する。(IPv4専用)
 * @param nics ネットワークインターフェースカードの情報。
 * @param defaultPort デフォルトポート
 * ※getifaddrsが有る場合は使用しない。
 */
void
ipmsg::getNetworkInterfaceInfoForIPv4( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
	IPMSG_FUNC_ENTER( "void ipmsg::getNetworkInterfaceInfoForIPv4( std::vector<NetworkInterface>& nics, int defaultPortNo )" );
#ifdef ENABLE_IPV4
	/* ローカルループバックをのぞく全てのIPアドレスが対象 */
	/* 全てのNICを取得する(より移植性が高い方法) */
	std::string localLoopbackAddress = "127.0.0.1";
	std::string anyAddress = "0.0.0.0";
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
		if ( rawAddress == localLoopbackAddress || rawAddress == anyAddress || rawAddress == broadcastAddress ){
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
#endif // ENABLE_IPV4
	IPMSG_FUNC_EXIT;
}

/**
 * ネットワークインターフェースの情報を取得する。(IPv6専用)
 * @param nics ネットワークインターフェースカードの情報。
 * @param defaultPort デフォルトポート
 * ※getifaddrsが有る場合は使用しない。
 */
void
ipmsg::getNetworkInterfaceInfoForIPv6( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
	IPMSG_FUNC_ENTER( "void ipmsg::getNetworkInterfaceInfoForIPv6( std::vector<NetworkInterface>& nics, int defaultPortNo )" );
#ifdef ENABLE_IPV6
	std::string localLoopbackAddress = "::1";
	std::string anyAddress = "::";
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
			if ( rawAddress == localLoopbackAddress || rawAddress == anyAddress || rawAddress == broadcastAddress ){
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
#endif // ENABLE_IPV6
	IPMSG_FUNC_EXIT;
}
#endif

/**
 * ブロードキャストアドレスを取得する。
 * @param netAddress ネットワークアドレス。
 * @param netmask ネットマスク。
 * @retval ブロードキャストアドレス。
 */
std::string
ipmsg::getBroadcastAddress( int family, std::string netAddress, std::string netmask )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::getBroadcastAddress( int family, std::string netAddress, std::string netmask )" );
	std::string ret = "";
#ifdef ENABLE_IPV4
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
		ret = ipaddrbuf;
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( family == AF_INET6 ) {
		//TODO
		ret = "ff02::1";
	}
#endif // ENABLE_IPV6
	IPMSG_FUNC_RETURN( ret );
}

/**
 * ネットワークアドレスを取得する。
 * @param family アドレスファミリ
 * @param rawAddress IPアドレス。
 * @param netmask ネットマスク。
 * @retval ネットワークアドレス。
 */
std::string
ipmsg::getNetworkAddress( int family, std::string rawAddress, std::string netmask )
{
	IPMSG_FUNC_ENTER( "std::string ipmsg::getNetworkAddress( int family, std::string rawAddress, std::string netmask )" );
	std::string ret = "";
#ifdef ENABLE_IPV4
	if ( family == AF_INET ) {
		sockaddr_storage inetAddr;
		if ( createSockAddrIn( &inetAddr, rawAddress, 0 ) == NULL ) {
			IPMSG_FUNC_RETURN( "" );
		}
		sockaddr_storage inetMask;
		if ( createSockAddrIn( &inetMask, netmask, 0 ) == NULL ) {
			IPMSG_FUNC_RETURN( "" );
		}
		struct sockaddr_in * inetAddrp = (struct sockaddr_in *)&inetAddr;
		struct sockaddr_in * inetMaskp = (struct sockaddr_in *)&inetMask;
		struct sockaddr_storage inetNetwork = inetAddr;
		struct sockaddr_in * inetNetworkp = (struct sockaddr_in *)&inetNetwork;
		inetNetworkp->sin_addr.s_addr = inetAddrp->sin_addr.s_addr & inetMaskp->sin_addr.s_addr;
		ret = getSockAddrInRawAddress( inetNetwork );
	}
#endif // ENABLE_IPV4
#ifdef ENABLE_IPV6
	if ( family == AF_INET6 ) {
#ifdef DEBUG
printf( "rawAddress => [%s]\n", rawAddress.c_str() );
#endif
		struct sockaddr_storage addr6;
		if ( createSockAddrIn( &addr6, rawAddress, 0 ) == NULL ) {
			ret = "::";
		} else {
			struct sockaddr_in6 *sin6 = ( struct sockaddr_in6 * )&addr6;
			in6_addr inetNetwork = sin6->sin6_addr;
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
#ifdef DEBUG
printf( "networkAddress => [%s]\n", ipaddrbuf );
#endif
			ret = ipaddrbuf;
		}
	}
#endif // ENABLE_IPV6
	IPMSG_FUNC_RETURN( ret );
}
