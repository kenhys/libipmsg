#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <IpMessenger.h>
#include <IpMessengerImpl.h>
#include <ctype.h>
#include <ipmsg.h>
#include <pwd.h>

struct sockaddr_storage *
ipmsg::createSockAddrIn( struct sockaddr_storage *addr, std::string rawAddress, int port )
{
	if ( addr != NULL ) {
		struct addrinfo hints, *res;
		memset( &hints, 0, sizeof( hints ) );
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET;

		int err = getaddrinfo( rawAddress.c_str(), NULL, &hints, &res );
		if ( err != 0 ){
			fprintf(stderr, "getaddrinfo:%s\n", gai_strerror( err ) );
			return NULL;
		}
		if ( res->ai_family == AF_INET ) {
			struct sockaddr_in *sockaddrp = ( struct sockaddr_in * )addr;
			sockaddrp->sin_family = res->ai_family;
			sockaddrp->sin_addr.s_addr = ( ( struct sockaddr_in * )( res->ai_addr ) )->sin_addr.s_addr;
			sockaddrp->sin_port = htons( port );
//printf( "DEBUG:createSockAddrIn(IPv4) addr = %s\n", getSockAddrInRawAddress( addr ).c_str() );
			freeaddrinfo( res );
			return addr;
		} else if ( res->ai_family == AF_INET6 ) {
			struct sockaddr_in6 *sockaddrp = ( struct sockaddr_in6 * )addr;
			sockaddrp->sin6_family = res->ai_family;
			memcpy( &sockaddrp->sin6_addr, &( ( struct sockaddr_in6 * )( res->ai_addr ) )->sin6_addr, sizeof( struct in6_addr ) );
			sockaddrp->sin6_port = htons( port );
			freeaddrinfo( res );
//printf( "DEBUG:createSockAddrIn(IPv6) addr = %s\n", getSockAddrInRawAddress( addr ).c_str() );
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

void
ipmsg::setSockAddrInPortNo( struct sockaddr_storage *addr, int port )
{
	if ( addr->ss_family == AF_INET ) {
		( ( struct sockaddr_in *)&addr )->sin_port = port;
	} else {
		( ( struct sockaddr_in6 *)&addr )->sin6_port = port;
	}
}

int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage *addr )
{
	return getSockAddrInPortNo( *addr );
}

int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage &addr )
{
	int ret = -1;
	if ( addr.ss_family == AF_INET ) {
		ret = ( ( struct sockaddr_in *)&addr )->sin_port;
	} else {
		ret = ( ( struct sockaddr_in6 *)&addr )->sin6_port;
	}
//printf( "DEBUG::getSockAddrInPortNo[%d]\n", ret );fflush( stdout );
	return ret;
}

std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage *addr )
{
	return getSockAddrInRawAddress( *addr );
}

std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage &addr )
{
	char ipAddrBuf[IP_ADDR_MAX_SIZE] = {0};
	if ( addr.ss_family == AF_INET ) {
		inet_ntop( AF_INET, &( ( struct sockaddr_in *)&addr )->sin_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	} else {
		inet_ntop( AF_INET6, &( ( struct sockaddr_in6 *)&addr )->sin6_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	}
//printf( "DEBUG::getSockAddrInRawAddress[%s]\n", ipAddrBuf );fflush( stdout );
	return ipAddrBuf;
}

bool
ipmsg::isSameSockAddrIn( struct sockaddr_storage base, struct sockaddr_storage check )
{
	return true;
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
	//FreeBSD用
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
//printf( "DEBUG:isSameNetwork addr = %s\n", getSockAddrInRawAddress( addr ).c_str() );fflush(stdout);
//printf( "DEBUG:isSameNetwork ifnetaddr = %s\n", ifnetaddr.c_str() );fflush(stdout);
//printf( "DEBUG:isSameNetwork netmask = %s\n", netmask.c_str() );fflush(stdout);
	sockaddr_storage ifnet, mask;
	memcpy( &ifnet, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &ifnet, ifnetaddr, 0 ) == NULL ) {
//printf( "createSockAddrIn( ifnet );\n" );fflush(stdout);
		return false;
	}
	memcpy( &mask, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &mask, netmask, 0 ) == NULL ) {
//printf( "createSockAddrIn( mask );\n" );fflush(stdout);
		return false;
	}
	if ( addr->ss_family == AF_INET ) {
		in_addr sin_addr = ( (struct sockaddr_in *) addr )->sin_addr;
		in_addr ifnet_addr = ( (struct sockaddr_in *) &ifnet )->sin_addr;
		in_addr mask_addr = ( (struct sockaddr_in *) &mask )->sin_addr;
//printf( "DEBUG:isSameNetwork = %s\n", ifnet_addr.s_addr == sin_addr.s_addr & mask_addr.s_addr ? "YES" : "NO" );fflush(stdout);
		return ifnet_addr.s_addr == sin_addr.s_addr & mask_addr.s_addr;
	}
	return false;
}

/**
 * ブロードキャストアドレスを取得する。
 * @param net_addr ネットワークアドレス。
 * @param netmask ネットマスク。
 * @retval ブロードキャストアドレス。
 */
struct in_addr
ipmsg::GetNativeBroadcastAddress( struct in_addr net_addr, struct in_addr netmask )
{
	struct in_addr ret;
	ret.s_addr = net_addr.s_addr | ( 0xffffffff ^ netmask.s_addr );
#if defined(DEBUG) || defined(INFO)
	char ipaddrbuf[IP_ADDR_MAX_SIZE];
	printf( "BROADCAST %s = ", inet_ntop( AF_INET, &ret, ipaddrbuf, sizeof( ipaddrbuf ) ) );fflush(stdout);
	printf( "NERADDR %s | ", inet_ntop( AF_INET, &net_addr, ipaddrbuf, sizeof( ipaddrbuf ) ) );fflush(stdout);
	printf( "0xffffffff ^ NETMASK %s\n", inet_ntop( AF_INET, &netmask, ipaddrbuf, sizeof( ipaddrbuf ) ) );fflush(stdout);
#endif
	return ret;
}

std::string
ipmsg::GetBroadcastAddress( struct in_addr net_addr, struct in_addr netmask )
{
	struct in_addr ret_addr = GetNativeBroadcastAddress( net_addr, netmask );
	char ipaddrbuf[IP_ADDR_MAX_SIZE];
#if defined(DEBUG) || defined(INFO)
	printf( "BROADCAST %s = ", inet_ntop( AF_INET, &ret_addr, ipaddrbuf, sizeof( ipaddrbuf ) ) );fflush(stdout);
	printf( "NERADDR %s | ", inet_ntop( AF_INET, &net_addr, ipaddrbuf, sizeof( ipaddrbuf ) ) );fflush(stdout);
	printf( "0xffffffff ^ NETMASK %s\n", inet_ntop( AF_INET, &netmask, ipaddrbuf, sizeof( ipaddrbuf ) ) );fflush(stdout);
#endif
	inet_ntop( AF_INET, &ret_addr, ipaddrbuf, sizeof( ipaddrbuf ) );
	std::string ret = ipaddrbuf;
	return ret;
}
