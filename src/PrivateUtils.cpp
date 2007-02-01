#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <IpMessenger.h>

#if defined(DEBUG) || defined(INFO)
#include <IpMessengerImpl.h>
#include <ctype.h>
#include <ipmsg.h>

/**
 * バッファをプリントする。
 * ・（"\x01\x01\x42\x1b"の場合、"(\01 2times)A(\1b)"と表示する）
 * @param bufname バッファタイトル
 * @param buf バッファ
 * @param size バッファサイズ
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
 * コマンド文字列を返す。
 * @param cmd コマンド
 * @retval コマンド文字列
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
 * パケットオブジェクトをダンプする。
 * @param packet パケットオブジェクト
 * @param sender_addr 送信元IPアドレス
 */
void
IpMsgDumpPacket( ipmsg::Packet packet, struct sockaddr_in sender_addr ){
	printf( ">> R E C V >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");fflush(stdout);
	char ipaddrbuf[IP_ADDR_MAX_SIZE];
	printf( "send from %s(%d)\n", inet_ntop( AF_INET, &sender_addr.sin_addr, ipaddrbuf, sizeof( ipaddrbuf ) ), ntohs( sender_addr.sin_port ) );fflush(stdout);
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
 * ホストリストをダンプする。
 * @param s ヘッダー／フッタタイトル文字列
 * @param hostList ホストリストオブジェクト
 */
void
IpMsgDumpHostList( const char *s, ipmsg::HostList& hostList )
{
	char head[]="=======================================================>\n";
	char foot[]="<=======================================================\n";

	memcpy( head+2, s, strlen( s ) );
	memcpy( foot+2, s, strlen( s ) );
	printf("\n\n");fflush(stdout);
	printf("%s", head );fflush(stdout);
	for( vector<ipmsg::HostListItem>::iterator ix = hostList.begin(); ix != hostList.end(); ix++ ){
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

/**
 * 私家版ミューテックス初期化
 * @param pos プリントする文字列
 * @param mutex ミューテックスのアドレス
 * @param mutexattr ミューテックスの属性
 * @retval pthread_mutex_initの戻り値
 */
int
IpMsgMutexInit( const char *pos, pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr )
{
#ifdef HAVE_PTHREAD
#if defined(LOCK_DEBUG)
	printf( "MutexInit before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_init(mutex, mutexattr);
#if defined(LOCK_DEBUG)
	printf( "MutexInit after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}

/**
 * 私家版ミューテックスロック
 * @param pos プリントする文字列
 * @param mutex ミューテックスのアドレス
 * @retval pthread_mutex_lockの戻り値
 */
int
IpMsgMutexLock( const char *pos, pthread_mutex_t *mutex )
{
#ifdef HAVE_PTHREAD
#if defined(LOCK_DEBUG)
	printf( "MutexLock before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_lock( mutex );
#if defined(LOCK_DEBUG)
	printf( "MutexLock after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}

/**
 * 私家版ミューテックスアンロック
 * @param pos プリントする文字列
 * @param mutex ミューテックスのアドレス
 * @retval pthread_mutex_unlockの戻り値
 */
int
IpMsgMutexUnlock( const char *pos, pthread_mutex_t *mutex )
{
#ifdef HAVE_PTHREAD
#if defined(LOCK_DEBUG)
	printf( "MutexUnlock before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_unlock( mutex );
#if defined(LOCK_DEBUG)
	printf( "MutexUnlock after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}

/**
 * 私家版ミューテックス破棄
 * @param pos プリントする文字列
 * @param mutex ミューテックスのアドレス
 * @retval pthread_mutex_destroyの戻り値
 */
int
IpMsgMutexDestroy( const char *pos, pthread_mutex_t *mutex )
{
#ifdef HAVE_PTHREAD
#if defined(LOCK_DEBUG)
	printf( "MutexDestroy before:%s\n", pos );fflush(stdout);
#endif
	int ret = pthread_mutex_destroy( mutex );
#if defined(LOCK_DEBUG)
	printf( "MutexDestroy after :%s\n", pos );fflush(stdout);
#endif
	return ret;
#else
	return 0;
#endif
}

/**
 * 私家版itoa
 * @param buf 書き込むバッファ
 * @param bufsize 書き込むバッファの最大長
 * @param val 書き込む値
 * @retval 変換した長さ(書込でバッファ長を超えた場合はバッファ長)
 */
int
IpMsgIntToString( char *buf, ssize_t bufsize, int val )
{
	int ret = snprintf( buf, bufsize, "%d", val );
	if ( ret >= bufsize ){
		buf[ bufsize - 1 ] = '\0';
		ret = bufsize - 1;
	}
	return ret;
}

/**
 * 私家版(unsigned)ltoa
 * @param buf 書き込むバッファ
 * @param bufsize 書き込むバッファの最大長
 * @param val 書き込む値
 * @retval 変換した長さ(書込でバッファ長を超えた場合はバッファ長)
 */
int
IpMsgULongToString( char *buf, ssize_t bufsize, unsigned long val )
{
	int ret = snprintf( buf, bufsize, "%lu", val );
	if ( ret >= bufsize ){
		buf[ bufsize - 1 ] = '\0';
		ret = bufsize - 1;
	}
	return ret;
}

/**
 * ucharをhex文字列に変換。
 * @param buf 書き込むバッファ
 * @param val 書き込む値
 * @retval 変換した長さ(書込でバッファ長を超えた場合はバッファ長)
 */
int
IpMsgUChrToHexString( char buf[3], const unsigned char val )
{
#if 0
	int ret = snprintf( buf, sizeof( buf ), "%02x", val );
	if ( ret >= sizeof( buf ) ){
		buf[ sizeof( buf ) - 1 ] = '\0';
		ret = sizeof( buf ) - 1;
	}
	return ret;
#else
	const char *hex[]={
		"00","01","02","03","04","05","06","07",  "08","09","0a","0b","0c","0d","0e","0f",
		"10","11","12","13","14","15","16","17",  "18","19","1a","1b","1c","1d","1e","1f",
		"20","21","22","23","24","25","26","27",  "28","29","2a","2b","2c","2d","2e","2f",
		"30","31","32","33","34","35","36","37",  "38","39","3a","3b","3c","3d","3e","3f",
		"40","41","42","43","44","45","46","47",  "48","49","4a","4b","4c","4d","4e","4f",
		"50","51","52","53","54","55","56","57",  "58","59","5a","5b","5c","5d","5e","5f",
		"60","61","62","63","64","65","66","67",  "68","69","6a","6b","6c","6d","6e","6f",
		"70","71","72","73","74","75","76","77",  "78","79","7a","7b","7c","7d","7e","7f",
		"80","81","82","83","84","85","86","87",  "88","89","8a","8b","8c","8d","8e","8f",
		"90","91","92","93","94","95","96","97",  "98","99","9a","9b","9c","9d","9e","9f",
		"a0","a1","a2","a3","a4","a5","a6","a7",  "a8","a9","aa","ab","ac","ad","ae","af",
		"b0","b1","b2","b3","b4","b5","b6","b7",  "b8","b9","ba","bb","bc","bd","be","bf",
		"c0","c1","c2","c3","c4","c5","c6","c7",  "c8","c9","ca","cb","cc","cd","ce","cf",
		"d0","d1","d2","d3","d4","d5","d6","d7",  "d8","d9","da","db","dc","dd","de","df",
		"e0","e1","e2","e3","e4","e5","e6","e7",  "e8","e9","ea","eb","ec","ed","ee","ef",
		"f0","f1","f2","f3","f4","f5","f6","f7",  "f8","f9","fa","fb","fc","fd","fe","ff"
	};
	strcpy( buf, hex[val]);
	return 2;
#endif
}

#define __IPMSG_LITTLE_ENDIAN__ 4321
#define __IPMSG_BIG_ENDIAN__ 1234
#define __IPMSG_PDP_ENDIAN__ 3412

static int
check_endian()
{
	unsigned char buf[4];
	int buf_hex = 0x01020304;
	memcpy( buf, &buf_hex, sizeof( buf ) );
	if ( buf[0] == 0x04 || buf[1] == 0x03 || buf[2] == 0x02 || buf[3] == 0x01 ){
		return __IPMSG_LITTLE_ENDIAN__;
	} else if ( buf[0] == 0x01 || buf[1] == 0x02 || buf[2] == 0x03 || buf[3] == 0x04 ){
		return __IPMSG_BIG_ENDIAN__;
	} else if ( buf[0] == 0x03 || buf[1] == 0x04 || buf[2] == 0x01 || buf[3] == 0x02 ){
		return __IPMSG_PDP_ENDIAN__;
	}
	return __IPMSG_LITTLE_ENDIAN__;
}

static const int endian = check_endian();

static int
BigEndianToLittleEndian( int val )
{
	unsigned char buf[4];
	unsigned char out[4];
	memcpy( buf, &val, sizeof( buf ));
	out[0] = buf[3];
	out[1] = buf[2];
	out[2] = buf[1];
	out[3] = buf[0];
	int ret;
	memcpy( &ret, out, sizeof( out ) );
	return ret;
}

static int
PdpEndianToLittleEndian( int val )
{
	unsigned char buf[4];
	unsigned char out[4];
	memcpy( buf, &val, sizeof( buf ));
	out[0] = buf[2];
	out[1] = buf[3];
	out[2] = buf[0];
	out[3] = buf[1];
	int ret;
	memcpy( &ret, out, sizeof( out ) );
	return ret;
}

std::string
IpMsgPortToStr( int portNo )
{
	char buf[100];
	if ( endian == __IPMSG_LITTLE_ENDIAN__ ) {
		IpMsgIntToString( buf, sizeof( buf ), htons( portNo ) );
	} else if ( endian == __IPMSG_BIG_ENDIAN__ ) {
		IpMsgIntToString( buf, sizeof( buf ), BigEndianToLittleEndian( htons( portNo ) ) );
	} else {
		IpMsgIntToString( buf, sizeof( buf ), PdpEndianToLittleEndian( htons( portNo ) ) );
	}
	return std::string( buf );
}
