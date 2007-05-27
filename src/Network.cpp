#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <IpMessenger.h>
#include <IpMessengerImpl.h>
#include <ctype.h>
#include <ipmsg.h>
#include <pwd.h>

/**
 * �����åȤ����������Х���ɤ��롣
 * @param proto �ץ�ȥ���(SOCK_STREAM, SOCK_DGRAM)��
 * @param addr �����åȥ��ɥ쥹��
 * @param devname �ǥХ���̾��
 * @retval �����åȡ�
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
//printf( "DEBUG:bindSocket.bind addr = %s,dev = %s\n", getSockAddrInRawAddress( addr ).c_str(), devname );
		close( sock );
		return -1;
	}
	if ( sock >= 0 && proto == SOCK_DGRAM && addr.ss_family == AF_INET6 && devname != NULL ){
		unsigned int devindex = if_nametoindex( devname );
		if ( setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&devindex, sizeof(devindex)) != 0 ) {
//printf( "DEBUG:bindSocket.setsockopt addr = %s, dev = %s(index=%u)\n", getSockAddrInRawAddress( addr ).c_str(), devname, devindex );
			close( sock );
			return -1;
		}
//printf( "DEBUG:bindSocket.OK addr = %s, dev = %s(index=%u)\n", getSockAddrInRawAddress( addr ).c_str(), devname, devindex );
	}
	if ( sock >= 0 && proto == SOCK_DGRAM && addr.ss_family == AF_INET ){
		const int yes = 1;
		if ( setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof(yes)) != 0 ) {
			perror("setsockopt(broadcast)");
			close( sock );
			return -1;
		}
	}
	return sock;
}

/**
 * �����åȥ��ɥ쥹���������롣
 * @param addr �����åȥ��ɥ쥹�Υݥ��󥿡�
 * @param rawAddress IP���ɥ쥹ʸ����
 * @param port �ݡ����ֹ�(�ۥ��ȥХ��ȥ�����)��
 * @param devname �ǥХ���̾�ʥǥե����NULL��
 * @retval ����Ѥ�addr�Υ��ɥ쥹��
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
//		hints.ai_flags = AI_PASSIVE;

//printf( "DEBUG:createSockAddrIn addr = %s(%s:%u)\n", getSockAddrInRawAddress( addr ).c_str(), portstr, getSockAddrInPortNo( addr ) );
//		int err = getaddrinfo( rawAddress.c_str(), NULL, &hints, &res );
		int err = getaddrinfo( rawAddress.c_str(), portstr, &hints, &res );
		if ( err != 0 ){
			fprintf(stderr, "getaddrinfo(addr=[%s] port=[%u] portstr[%s]):%s\n",
							rawAddress.c_str(), port, portstr, gai_strerror( err ) );
			return NULL;
		}
		if ( res->ai_family == AF_INET ) {
			struct sockaddr_in *sockaddrp = ( struct sockaddr_in * )addr;
			*sockaddrp = *( (struct sockaddr_in *)res->ai_addr );
//printf( "DEBUG:createSockAddrIn(IPv4) family = %d addr = %s(%d)\n", res->ai_family, getSockAddrInRawAddress( addr ).c_str(), sockaddrp->sin_port );
#if 0
			int i = 1;
			for( struct addrinfo *res0 = res; res0; res0 = res0->ai_next, i++ ) {
				printf("RES V4(%d)->%s\n", i, getSockAddrInRawAddress( (struct sockaddr_storage *)res0->ai_addr ).c_str() );
			}
			printf("===================================\n");
#endif
			freeaddrinfo( res );
//printf( "DEBUG:createSockAddrIn(IPv4) addr = %s(%s:%u) OK!!\n", getSockAddrInRawAddress( addr ).c_str(), portstr, getSockAddrInPortNo( addr ) );
			return addr;
		} else if ( res->ai_family == AF_INET6 ) {
			struct sockaddr_in6 *sockaddrp = ( struct sockaddr_in6 * )addr;
			*sockaddrp = *( (struct sockaddr_in6 *)res->ai_addr );
			if ( devname != NULL ) {
				sockaddrp->sin6_scope_id = if_nametoindex( devname );
			}
//printf( "DEBUG:createSockAddrIn(IPv6) family = %d addr = %s(%d) lnk=%lu flow=%lu\n", res->ai_family, getSockAddrInRawAddress( addr ).c_str(), sockaddrp->sin6_port, sockaddrp->sin6_scope_id, sockaddrp->sin6_flowinfo );
#if 0
			int i = 1;
			for( struct addrinfo *res0 = res; res0; res0 = res0->ai_next, i++ ) {
				printf("RES V6(%d)->%s\n", i, getSockAddrInRawAddress( (struct sockaddr_storage *)res0->ai_addr ).c_str() );
			}
			printf("===================================\n");
#endif
			freeaddrinfo( res );
//printf( "DEBUG:createSockAddrIn(IPv6) addr = %s(%s:%u) OK!!\n", getSockAddrInRawAddress( addr ).c_str(), portstr, getSockAddrInPortNo( addr ) );
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
 * �����åȥ��ɥ쥹����ݡ����ֹ��������롣
 * @param addr �����åȥ��ɥ쥹�Υݥ��󥿡�
 * @retval �ݡ����ֹ�ʥͥåȥ���Х��ȥ������ˡ�
 */
int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage *addr )
{
	return getSockAddrInPortNo( *addr );
}

/**
 * �����åȥ��ɥ쥹����ݡ����ֹ��������롣
 * @param addr �����åȥ��ɥ쥹��
 * @retval �ݡ����ֹ�ʥͥåȥ���Х��ȥ������ˡ�
 */
int
ipmsg::getSockAddrInPortNo( const struct sockaddr_storage &addr )
{
	int ret = -1;
	if ( addr.ss_family == AF_INET ) {
		ret = ( ( struct sockaddr_in *)&addr )->sin_port;
	} else if ( addr.ss_family == AF_INET6 ) {
		ret = ( ( struct sockaddr_in6 *)&addr )->sin6_port;
	}
//printf( "DEBUG::getSockAddrInPortNo[%d]\n", ret );fflush( stdout );
	return ret;
}

/**
 * �����åȥ��ɥ쥹����IP���ɥ쥹ʸ�����������롣
 * @param addr �����åȥ��ɥ쥹�Υݥ��󥿡�
 * @retval IP���ɥ쥹ʸ����
 */
std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage *addr )
{
	return getSockAddrInRawAddress( *addr );
}

/**
 * �����åȥ��ɥ쥹����IP���ɥ쥹ʸ�����������롣
 * @param addr �����åȥ��ɥ쥹��
 * @retval IP���ɥ쥹ʸ����
 */
std::string
ipmsg::getSockAddrInRawAddress( const struct sockaddr_storage &addr )
{
	char ipAddrBuf[IP_ADDR_MAX_SIZE] = {0};
	if ( addr.ss_family == AF_INET ) {
		inet_ntop( addr.ss_family, &( ( struct sockaddr_in *)&addr )->sin_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	} else if ( addr.ss_family == AF_INET6 ) {
		inet_ntop( addr.ss_family, &( ( struct sockaddr_in6 *)&addr )->sin6_addr, ipAddrBuf, sizeof( ipAddrBuf ) );
	} else {
		return "";
	}
//printf( "DEBUG::getSockAddrInRawAddress[%s]\n", ipAddrBuf );fflush( stdout );
	return ipAddrBuf;
}

/**
 * ���ɥ쥹�ե��ߥ�ʸ�����������롣
 * @param addr �����åȥ��ɥ쥹��
 * @retval �����åȥ��ɥ쥹ʸ����
 */
std::string
ipmsg::getSockAddrInAddressFamilyString( const struct sockaddr_storage &addr )
{
	return getAddressFamilyString( addr.ss_family );
}

/**
 * ���ɥ쥹�ե��ߥ�ʸ�����������롣
 * @param family �����åȥե��ߥꡣ
 * @retval �����åȥ��ɥ쥹ʸ����
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
 * Ʊ�������åȥ��ɥ쥹����Ƚ�ꤹ�롣
 * @param base �����åȥ��ɥ쥹��
 * @param check �����åȥ��ɥ쥹��
 * @retval true:Ʊ�졣
 * @retval false:Ʊ��Ǥʤ���
 */
bool
ipmsg::isSameSockAddrIn( struct sockaddr_storage base, struct sockaddr_storage check )
{
	return memcmp( &base, &check, sizeof( struct sockaddr_storage ) ) == 0;
}

/**
 * �ե�����Хåե������᥽�åɡ�
 * <ul>
 * <li>sendfile�����ݡ��Ȥ���Ƥ���ץ�åȥե�����Ǥ�sendfile�����ƥॳ�����Ȥäƹ�®�����������Х����������������ˤ�äƤϥ����ԡ��ˤʤ롣<br>
 * �Ǥ⡢�ƥ��Ȥ�����ޤ���ͤ����Ȥꤢ������hp-ux�ϥǥե���Ȥ�read/write������Ȥ��Τ��٤���Linux�Ǥ�4-5MB�Υե��������2�ܰ㤦���Ȥ⤢��</li>
 * </ul>
 */
int
ipmsg::IpMsgSendFileBuffer( int ifd, int sock, int size )
{
#if defined(SUPPORT_SENDFILE_LINUX_STYLE)
	//Linux��
	//printf("sendfile as sendfile syscall by linux\n");
	return sendfile( sock, ifd, NULL, size );
#elif defined(SUPPORT_SENDFILE_BSD_STYLE)
	//printf("sendfile as sendfile syscall by freebsd\n");
	//FreeBSD��
	return sendfile( sock, ifd, NULL, size, NULL, NULL, 0 );
/*
// TODO solaris support start from here.
#elif defined( SUPPORT_SENDFILE_SOLARIS_STYLE )
// TODO hp-ux support start from here.
#if 0
//#elif defined(SUPPORT_SENDFILE_HPUX_STYLE)
	printf("sendfile as sendfile syscall by hp-ux\n");
	//HP-UX��
	return sendfile( sock, ifd, NULL, size, NULL, 0 );
#endif
// TODO hp-ux support end.
*/
#else
	//printf("sendfile as read write\n");
	//�ǥե���ȼ���
	char readbuf[8192];
	int readSize = read( ifd, readbuf, sizeof( readbuf ) );
	if ( readSize > 0 ){
		return send( sock, readbuf, readSize, 0 );
	}
	return -1;
#endif // SUPPORT_SENDFILE_LINUX, SUPPORT_SENDFILE_FREEBSD
}

/**
 * Ʊ���ͥåȥ����°���Ƥ��뤫��Ƚ�ꤹ��᥽�åɡ�
 * <ul>
 * <li>���ɥ쥹��NIC��Ʊ���ͥåȥ����°���Ƥ��뤫��Ƚ�ꡣ
 * </ul>
 * @param addr Ƚ�ꤹ��IP���ɥ쥹��
 * @param ifnetaddr NIC�Υͥåȥ�����ɥ쥹��
 * @param netmask �ͥåȥޥ�����
 * @retval true:Ʊ���ͥåȥ����
 * @retval false:�㤦�ͥåȥ����
 */
bool
ipmsg::isSameNetwork( const struct sockaddr_storage *addr, std::string ifnetaddr, std::string netmask )
{
//printf( "DEBUG:isSameNetwork addr = %s\n", getSockAddrInRawAddress( addr ).c_str() );fflush(stdout);
//printf( "DEBUG:isSameNetwork ifnetaddr = %s\n", ifnetaddr.c_str() );fflush(stdout);
//printf( "DEBUG:isSameNetwork netmask = %s\n", netmask.c_str() );fflush(stdout);
	bool ret = false;
	sockaddr_storage ifnet, mask;
	memcpy( &ifnet, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &ifnet, ifnetaddr, IPMSG_DEFAULT_PORT ) == NULL ) {
//printf( "createSockAddrIn( ifnet );\n" );fflush(stdout);
		return ret;
	}
	memcpy( &mask, addr, sizeof( sockaddr_storage ) );
	if ( createSockAddrIn( &mask, netmask, IPMSG_DEFAULT_PORT ) == NULL ) {
//printf( "createSockAddrIn( mask );\n" );fflush(stdout);
		return ret;
	}
	if ( addr->ss_family == AF_INET ) {
		in_addr sin_addr = ( (struct sockaddr_in *) addr )->sin_addr;
		in_addr ifnet_addr = ( (struct sockaddr_in *) &ifnet )->sin_addr;
		in_addr mask_addr = ( (struct sockaddr_in *) &mask )->sin_addr;
//printf( "DEBUG:isSameNetwork = %s\n", ifnet_addr.s_addr == sin_addr.s_addr & mask_addr.s_addr ? "YES" : "NO" );fflush(stdout);
		ret = ifnet_addr.s_addr == sin_addr.s_addr & mask_addr.s_addr;
	} else if ( addr->ss_family == AF_INET6 ) {
		in6_addr sin_addr = ( (struct sockaddr_in6 *) addr )->sin6_addr;
		in6_addr ifnet_addr = ( (struct sockaddr_in6 *) &ifnet )->sin6_addr;
		ret = sin_addr.s6_addr32[0] == ifnet_addr.s6_addr32[0] && sin_addr.s6_addr32[1] == ifnet_addr.s6_addr32[1];
	}
	return ret;
}

#ifdef HAVE_GETIFADDR
/**
 * �ͥåȥ�����󥿡��ե������ξ����������롣
 * @param AddressFamily ���ɥ쥹�ե��ߥ�
 * @param nics �ͥåȥ�����󥿡��ե����������ɤξ���
 * @param defaultPort �ǥե���ȥݡ���
 */
void
ipmsg::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
//TODO getifaddrs��Ȥ��������Ѱա����������Ե����ɤ���
	return;
}
#else
/**
 * �ͥåȥ�����󥿡��ե������ξ����������롣
 * @param AddressFamily ���ɥ쥹�ե��ߥ�
 * @param nics �ͥåȥ�����󥿡��ե����������ɤξ���
 * @param defaultPort �ǥե���ȥݡ���
 * ��getifaddrs��ͭ����ϻ��Ѥ��ʤ���
 */
void
ipmsg::GetNetworkInterfaceInfo( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
//	GetNetworkInterfaceInfoForIPv4( nics, defaultPortNo );
	GetNetworkInterfaceInfoForIPv6( nics, defaultPortNo );
}

/**
 * �ͥåȥ�����󥿡��ե������ξ����������롣(IPv4����)
 * @param nics �ͥåȥ�����󥿡��ե����������ɤξ���
 * @param defaultPort �ǥե���ȥݡ���
 * ��getifaddrs��ͭ����ϻ��Ѥ��ʤ���
 */
void
ipmsg::GetNetworkInterfaceInfoForIPv4( std::vector<NetworkInterface>& nics, int defaultPortNo )
{
	/* ������롼�ץХå���Τ������Ƥ�IP���ɥ쥹���о� */
	/* ���Ƥ�NIC���������(���ܿ������⤤��ˡ) */
	std::string localLoopbackAddress = "127.0.0.1";
	std::string nullAddress = "0.0.0.0";
	std::string broadcastAddress = "255.255.255.255";
	//��������Τ���Υ����åȤ����
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
	//��������Τ���Υ����åȤ��Ĥ��롣
	close(fd);
}

/**
 * �ͥåȥ�����󥿡��ե������ξ����������롣(IPv6����)
 * @param nics �ͥåȥ�����󥿡��ե����������ɤξ���
 * @param defaultPort �ǥե���ȥݡ���
 * ��getifaddrs��ͭ����ϻ��Ѥ��ʤ���
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
			snprintf( ipaddrbuf, sizeof( ipaddrbuf ), "%s:%s:%s:%s:%s:%s:%s:%s%%%s",
					v6addr[0], v6addr[1], v6addr[2], v6addr[3],
					v6addr[4], v6addr[5], v6addr[6], v6addr[7], devname );
//printf("IPv6 Address(%s) = %s\n", devname, ipaddrbuf );fflush(stdout);
			//���ɥ쥹���Ŭ��
			struct sockaddr_storage ss;
			memset( &ss, 0, sizeof( ss ) );
			ss.ss_family = AF_INET6;
			if ( createSockAddrIn( &ss, ipaddrbuf, IPMSG_DEFAULT_PORT, devname ) == NULL ) {
				continue;
			}
			std::string rawAddress = getSockAddrInRawAddress( &ss );
//printf("IPv6 RawAddress(%s) = %s\n", devname, rawAddress.c_str() );fflush(stdout);
			if ( rawAddress == localLoopbackAddress || rawAddress == nullAddress || rawAddress == broadcastAddress ){
//printf("IPv6 Address(%s) = %s, continue\n", devname, ipaddrbuf );fflush(stdout);
				continue;
			}
			NetworkInterface ni( AF_INET6, std::string( devname ) );
			ni.setPortNo( defaultPortNo );
			ni.setIpAddress( rawAddress );
			ni.setNetMask( "ffff:ffff:ffff:ffff::" );
			nics.push_back( ni );
//printf("IPv6 Address(%s) = %s, added\n", devname, ipaddrbuf );fflush(stdout);
		}
		fclose( fp );
	}
}
#endif

/**
 * �֥��ɥ��㥹�ȥ��ɥ쥹��������롣
 * @param netAddress �ͥåȥ�����ɥ쥹��
 * @param netmask �ͥåȥޥ�����
 * @retval �֥��ɥ��㥹�ȥ��ɥ쥹��
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
		return "ff02::1%eth0";
	}
	return "";
}

/**
 * �ͥåȥ�����ɥ쥹��������롣
 * @param family ���ɥ쥹�ե��ߥ�
 * @param rawAddress IP���ɥ쥹��
 * @param netmask �ͥåȥޥ�����
 * @retval �ͥåȥ�����ɥ쥹��
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

		inetNetwork.s6_addr32[2] = 0x0LU;
		inetNetwork.s6_addr32[3] = 0x0LU;

		char ipaddrbuf[IP_ADDR_MAX_SIZE];
		inet_ntop( family, &inetNetwork, ipaddrbuf, sizeof( ipaddrbuf ) );
//printf("NETWORK ADDR=%s\n", ipaddrbuf);fflush(stdout);
		return ipaddrbuf;
	}
	return "";
}
