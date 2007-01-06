#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <IpMessenger.h>

#if defined(DEBUG) || defined(INFO)
#include <IpMessengerImpl.h>
#include <ctype.h>
#include <ipmsg.h>

/**
 * �Хåե���ץ��Ȥ��롣
 * ����"\x01\x01\x42\x1b"�ξ�硢"(\01 2times)A(\1b)"��ɽ�������
 * @param bufname �Хåե������ȥ�
 * @param buf �Хåե�
 * @param size �Хåե�������
 */
void
IpMsgPrintBuf( const char* bufname, const char *buf, const int size )
{
	int continue_count = 0;
	unsigned char pchar = *buf;
	bool can_not_print = true;
	printf("%s(%d bytes)[", bufname, size);fflush(stdout);
	for( int i = 0; i < size; i++ ){
		if ( !isprint( buf[i] ) && buf[i] != 0x20 ) {
			if ( pchar != buf[i] ){
				printf( "(\\%02x", (unsigned char)buf[i] );fflush(stdout);
			}
			continue_count++;
			can_not_print = true;
		}else{
			putchar( buf[i] );
			can_not_print = false;
		}
		if ( pchar == buf[i] ){
		} else {
			if ( can_not_print ) {
				if ( continue_count > 1 ) {
					printf( " %dtimes)", continue_count );fflush(stdout);
				} else {
					printf( ")" );fflush(stdout);
				}
				continue_count = 0;
			}
		}
		pchar = buf[i];
	}
	printf( "]\n" );fflush(stdout);
}

/**
 * ���ޥ��ʸ������֤���
 * @param cmd ���ޥ��
 * @retval ���ޥ��ʸ����
 */
string
GetCommandString( unsigned long cmd )
{
	switch( cmd ){
		case IPMSG_NOOPERATION:     return "IPMSG_NOOPERATION";
		case IPMSG_BR_ENTRY:        return "IPMSG_BR_ENTRY";
		case IPMSG_BR_EXIT:         return "IPMSG_BR_EXIT";
		case IPMSG_ANSENTRY:        return "IPMSG_ANSENTRY";
		case IPMSG_BR_ABSENCE:      return "IPMSG_BR_ABSENCE";
		case IPMSG_BR_ISGETLIST:    return "IPMSG_BR_ISGETLIST";
		case IPMSG_OKGETLIST:       return "IPMSG_OKGETLIST";
		case IPMSG_GETLIST:         return "IPMSG_GETLIST";
		case IPMSG_ANSLIST:         return "IPMSG_ANSLIST";
		case IPMSG_BR_ISGETLIST2:   return "IPMSG_BR_ISGETLIST2";
		case IPMSG_SENDMSG:         return "IPMSG_SENDMSG";
		case IPMSG_RECVMSG:         return "IPMSG_RECVMSG";
		case IPMSG_READMSG:         return "IPMSG_READMSG";
		case IPMSG_DELMSG:          return "IPMSG_DELMSG";
		case IPMSG_ANSREADMSG:      return "IPMSG_ANSREADMSG";
		case IPMSG_GETINFO:         return "IPMSG_GETINFO";
		case IPMSG_SENDINFO:        return "IPMSG_SENDINFO";
		case IPMSG_GETABSENCEINFO:  return "IPMSG_GETABSENCEINFO";
		case IPMSG_SENDABSENCEINFO: return "IPMSG_SENDABSENCEINFO";
		case IPMSG_GETFILEDATA:     return "IPMSG_GETFILEDATA";
		case IPMSG_RELEASEFILES:    return "IPMSG_RELEASEFILES";
		case IPMSG_GETDIRFILES:     return "IPMSG_GETDIRFILES";
		case IPMSG_GETPUBKEY:       return "IPMSG_GETPUBKEY";
		case IPMSG_ANSPUBKEY:       return "IPMSG_ANSPUBKEY";
	}
	return "no match";
}

/**
 * �ѥ��åȥ��֥������Ȥ����פ��롣
 * @param packet �ѥ��åȥ��֥�������
 * @param sender_addr ������IP���ɥ쥹
 */
void
IpMsgDumpPacket( Packet packet, struct sockaddr_in sender_addr ){
	printf( ">> R E C V >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");fflush(stdout);
	char ipaddrbuf[100];
	printf( "send from %s(%d)\n", inet_ntoa_r( sender_addr.sin_addr.s_addr, ipaddrbuf, sizeof( ipaddrbuf ) ), ntohs( sender_addr.sin_port ) );fflush(stdout);
	printf( "VersionNo    [%ld]\n", packet.VersionNo() );fflush(stdout);
	printf( "PacketNo     [%ld]\n", packet.PacketNo() );fflush(stdout);
	printf( "CommandMode  [%ld][%s]\n", packet.CommandMode(), GetCommandString( packet.CommandMode() ).c_str() );fflush(stdout);
	printf( "CommandOption[%ld]\n", packet.CommandOption() );fflush(stdout);
	printf( "HostName     [%s]\n", packet.HostName().c_str() );fflush(stdout);
	printf( "UserName     [%s]\n", packet.UserName().c_str() );fflush(stdout);
	IpMsgPrintBuf("Option", packet.Option().c_str(), packet.Option().length() );
	printf( "<< R E C V <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n");fflush(stdout);
}

/**
 * �ۥ��ȥꥹ�Ȥ����פ��롣
 * @param s �إå������եå������ȥ�ʸ����
 * @param hostList �ۥ��ȥꥹ�ȥ��֥�������
 */
void
IpMsgDumpHostList( const char *s, HostList& hostList )
{
	char head[]="=======================================================>\n";
	char foot[]="<=======================================================\n";

	memcpy( head+2, s, strlen( s ) );
	memcpy( foot+2, s, strlen( s ) );
	printf("\n\n");fflush(stdout);
	printf("%s", head );fflush(stdout);
	for( vector<HostListItem>::iterator ix = hostList.begin(); ix != hostList.end(); ix++ ){
		printf( "Version[%s]\n" \
				"AbsenceDescription[%s]\n" \
				"User[%s]\n" \
				"Host[%s]\n" \
				"CommandNo[%lu]\n" \
				"IpAddress[%s]\n" \
				"NickName[%s]\n" \
				"Group[%s]\n" \
				"Encoding[%s]\n" \
				"EncryptionCapacity[%lu]\n" \
				"PubKeyHex[%s]\n" \
				"EncryptMethodHex[%s]\n" \
				"PortNo[%lu]\n" \
				"##########################################################\n",
				ix->Version().c_str(),
				ix->AbsenceDescription().c_str(),
				ix->UserName().c_str(),
				ix->HostName().c_str(),
				ix->CommandNo(),
				ix->IpAddress().c_str(),
				ix->Nickname().c_str(),
				ix->GroupName().c_str(),
				ix->EncodingName().c_str(),
				ix->EncryptionCapacity(),
				ix->PubKeyHex().c_str(),
				ix->EncryptMethodHex().c_str(),
				ix->PortNo() );fflush(stdout);
	}
	printf("%s", foot );fflush(stdout);
}
#endif

static bool inet_ntoa_init();
static bool inet_init = inet_ntoa_init();
static int inet_index_1 = 0;
static int inet_index_2 = 1;
static int inet_index_3 = 2;
static int inet_index_4 = 3;

/**
 * ����ǥ���åɥ�����inet_ntoa�ν����
 * @retval ���true
 */
static bool
inet_ntoa_init()
{
	in_addr_t netaddr = inet_addr( "0.1.2.3" );
	unsigned char addr[4];
	memcpy( addr, &netaddr, sizeof( addr ) );
	inet_index_1 = addr[0];
	inet_index_2 = addr[1];
	inet_index_3 = addr[2];
	inet_index_4 = addr[3];
#if defined(DEBUG) || defined(INFO)
	printf( "index:%d.%d.%d.%d", inet_index_1, inet_index_2, inet_index_3, inet_index_4);fflush(stdout);
#endif
	return true;
}

/**
 * ����ǥ���åɥ�����inet_ntoa
 * @param s_addr ������IP���ɥ쥹
 * @param *ret ����оݤΥХåե����ɥ쥹
 * @param size ����оݤΥХåե��ΥХ��ȿ�
 * @retval ����оݤΥХåե����ɥ쥹(ret�����)
 */
char *
inet_ntoa_r( in_addr_t s_addr, char *ret, int size )
{
	unsigned char addr[4];
	memcpy( addr, &s_addr, sizeof( addr ) );
#if defined(DEBUG) || defined(INFO)
	printf( "ip:%d.%d.%d.%d\n", addr[inet_index_1], addr[inet_index_2], addr[inet_index_3], addr[inet_index_4]);fflush(stdout);
#endif
	snprintf( ret, size, "%d.%d.%d.%d", addr[inet_index_1], addr[inet_index_2], addr[inet_index_3], addr[inet_index_4]);
	return ret;
}

/**
 * ����ǥߥ塼�ƥå��������
 * @param pos �ץ��Ȥ���ʸ����
 * @param mutex �ߥ塼�ƥå����Υ��ɥ쥹
 * @param mutexattr �ߥ塼�ƥå�����°��
 * @retval pthread_mutex_init�������
 */
int
IpMsgMutexInit( const char *pos, pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr )
{
#ifdef HAVE_PTHREAD
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexInit before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_init(mutex, mutexattr);
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexInit after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}

/**
 * ����ǥߥ塼�ƥå�����å�
 * @param pos �ץ��Ȥ���ʸ����
 * @param mutex �ߥ塼�ƥå����Υ��ɥ쥹
 * @retval pthread_mutex_lock�������
 */
int
IpMsgMutexLock( const char *pos, pthread_mutex_t *mutex )
{
#ifdef HAVE_PTHREAD
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexLock before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_lock( mutex );
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexLock after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}

/**
 * ����ǥߥ塼�ƥå��������å�
 * @param pos �ץ��Ȥ���ʸ����
 * @param mutex �ߥ塼�ƥå����Υ��ɥ쥹
 * @retval pthread_mutex_unlock�������
 */
int
IpMsgMutexUnlock( const char *pos, pthread_mutex_t *mutex )
{
#ifdef HAVE_PTHREAD
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexUnlock before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_unlock( mutex );
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexUnlock after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}

/**
 * ����ǥߥ塼�ƥå����˴�
 * @param pos �ץ��Ȥ���ʸ����
 * @param mutex �ߥ塼�ƥå����Υ��ɥ쥹
 * @retval pthread_mutex_destroy�������
 */
int
IpMsgMutexDestroy( const char *pos, pthread_mutex_t *mutex )
{
#ifdef HAVE_PTHREAD
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexDestroy before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_destroy( mutex );
#if defined(DEBUG) || defined(INFO)
//	printf( "MutexDestroy after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}
