#if defined(DEBUG) || defined(INFO)
#include <stdio.h>
#include <ctype.h>

/**
 * バッファをプリントする。
 * ・（"\x01\x01\x42\x1b"の場合、"(\01 2times)A(\1b)"と表示する）
 * @param bufname バッファタイトル
 * @param buf バッファ
 * @param size バッファサイズ
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMsgPrintBuf( char* bufname, char *buf, int size )
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
#endif
