#if defined(DEBUG) || defined(INFO)
#include <stdio.h>
#include <ctype.h>
#include <IpMessenger.h>
#include <ipmsg.h>

/**
 * バッファをプリントする。
 * ・（"\x01\x01\x42\x1b"の場合、"(\01 2times)A(\1b)"と表示する）
 * @param bufname バッファタイトル
 * @param buf バッファ
 * @param size バッファサイズ
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMsgPrintBuf( const char* bufname, const char *buf, const int size )
{
	int continue_count = 0;
	unsigned char pchar = *buf;
	bool can_not_print = true;
	printf("%s[", bufname);
	for( int i = 0; i < size; i++ ){
		if ( !isprint( buf[i] ) && buf[i] != 0x20 ) {
			if ( pchar != buf[i] ){
				printf( "(\\%02x", (unsigned char)buf[i] );
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
					printf( " %dtimes)", continue_count );
				} else {
					printf( ")" );
				}
				continue_count = 0;
			}
		}
		pchar = buf[i];
	}
	printf( "]\n" );
	fflush(stdout);
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

void
IpMsgDumpPacket( Packet packet, struct sockaddr_in sender_addr ){
	printf( ">> R E C V >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	printf( "send from %s(%d)\n", inet_ntoa( sender_addr.sin_addr ), ntohs( sender_addr.sin_port ) );
	printf( "VersionNo    [%ld]\n", packet.VersionNo() );
	printf( "PacketNo     [%ld]\n", packet.PacketNo() );
	printf( "CommandMode  [%ld][%s]\n", packet.CommandMode(), GetCommandString( packet.CommandMode() ).c_str() );
	printf( "CommandOption[%ld]\n", packet.CommandOption() );
	printf( "HostName     [%s]\n", packet.HostName().c_str() );
	printf( "UserName     [%s]\n", packet.UserName().c_str() );
	IpMsgPrintBuf("Option", packet.Option().c_str(), packet.Option().length() );
	printf( "<< R E C V <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n");
}

void
IpMsgDumpHostList( char *s, HostList& hostList )
{
	char head[]="=======================================================>\n");
	char foot[]="<=======================================================\n");

	memcpy( head+2, s, strlen( s ) );
	memcpy( foot+2, s, strlen( s ) );
	printf("\n\n");
	printf("%s", head );
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
				ix->PortNo() );
	}
	printf("%s", foot );
}
#endif
