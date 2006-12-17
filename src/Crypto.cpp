/**
 * IP メッセンジャライブラリ(Unix用)
 * IPメッセンジャエージェントクラスの暗号関連メソッド群。
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"

#define WINCOMPAT

#ifdef HAVE_OPENSSL
#ifdef WINCOMPAT
#define SUPPORT_RSA_512
#define SUPPORT_RSA_1024
#define SUPPORT_RC2_40
#define SUPPORT_BLOWFISH_128
#endif	// WINCOMPAT
#endif	// HAVE_OPENSSL

#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#endif	// HAVE_OPENSSL

//暗号化キー(RSA)のビット数(最弱)
#define RSA_KEY_LENGTH_MINIMUM	512
//暗号化キー(RSA)のビット数(まぁまぁ)
#define RSA_KEY_LENGTH_MIDIUM	1024
//暗号化キー(RSA)のビット数(最強)
#define RSA_KEY_LENGTH_MAXIMUM	2048

//RSAキー生成時に使用する素数
#define ENCRYPT_PRIME			65537

/**
 * 暗号関連の初期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::CryptoInit()
{
#ifdef HAVE_OPENSSL
	ERR_load_crypto_strings();
	char errbuf[1024];

	encryptionCapacity = 0UL;
	RsaMax = NULL;
#ifdef SUPPORT_RSA_2048
	RsaMax = RSA_generate_key( RSA_KEY_LENGTH_MAXIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMax == NULL ) {
		printf("in encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );	
	} else {
		encryptionCapacity |= IPMSG_RSA_2048;
		printf("encryption extention enabled.(RSA2048)\n");
	}
#endif	//SUPPORT_RSA_2048
	RsaMid = NULL;
#ifdef SUPPORT_RSA_1024
	RsaMid = RSA_generate_key( RSA_KEY_LENGTH_MIDIUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMid == NULL ) {
		printf("in encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );	
	} else {
		encryptionCapacity |= IPMSG_RSA_1024;
		printf("encryption extention enabled.(RSA1024)\n");
	}
#endif	//SUPPORT_RSA_1024
	RsaMin = NULL;
#ifdef SUPPORT_RSA_512
	RsaMin = RSA_generate_key( RSA_KEY_LENGTH_MINIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMin == NULL ) {
		printf("in encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );	
	} else {
		encryptionCapacity |= IPMSG_RSA_512;
		printf("encryption extention enabled.(RSA512)\n");
	}
#endif	//SUPPORT_RSA_512
	if ( encryptionCapacity == 0UL ) {
		//暗号化無効
		printf("encryption extention disabled.\n");
	}
#ifdef SUPPORT_RC2_40
	encryptionCapacity |= IPMSG_RC2_40;
#endif	//SUPPORT_RC2_40
#ifdef SUPPORT_RC2_128
	encryptionCapacity |= IPMSG_RC2_128;
#endif	//SUPPORT_RC2_128

#ifdef SUPPORT_RC2_256
	encryptionCapacity |= IPMSG_RC2_256;
#endif	//SUPPORT_RC2_256

#ifdef SUPPORT_BLOWFISH_128
	encryptionCapacity |= IPMSG_BLOWFISH_128;
#endif	//SUPPORT_BLOWFISH_128

#ifdef SUPPORT_BLOWFISH_256
	encryptionCapacity |= IPMSG_BLOWFISH_256;
#endif	//SUPPORT_BLOWFISH_256
#endif	//HAVE_OPENSSL
}

/**
 * 暗号関連の終期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::CryptoEnd()
{
#ifdef HAVE_OPENSSL
	if ( RsaMin != NULL ) {
		RSA_free( RsaMin );
	}
	if ( RsaMid != NULL ) {
		RSA_free( RsaMid );
	}
	if ( RsaMax != NULL ) {
		RSA_free( RsaMax );
	}
	ERR_free_strings();
#endif	//HAVE_OPENSSL
}

/**
 * メッセージ暗号化。
 * @param host 送信先ホスト
 * @param optBuf パケットオプション部のバッファのアドレス
 * @param optBufLen パケットオプション部のバッファの現在の有効データ長
 * @param enc_optBufLen 暗号化済のパケットオプション部のバッファの有効データ長のアドレス
 * @param opt_size パケットオプション部のバッファのサイズ
 * @retval true:暗号化OK、false:暗号化NG
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgentImpl::EncryptMsg( HostListItem host, unsigned char *optBuf, int optBufLen, int *enc_optBufLen, int opt_size )
{
#ifdef HAVE_OPENSSL
	unsigned long pubKeyMethod = 0UL;
	unsigned char iv[EVP_MAX_IV_LENGTH];
	char errbuf[200];

	//EVPのSeal系の公開鍵暗号の暗号化APIは使いにくいので、自分でEncrypt系、RSA系のAPIで実装します。
#ifndef WINCOMPAT
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_2048 && host.EncryptionCapacity() & IPMSG_RSA_2048 ) {
		pubKeyMethod = IPMSG_RSA_2048;
	}
#endif	//SUPPORT_RSA_2048
#endif	//WINCOMPAT

#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_1024 && host.EncryptionCapacity() & IPMSG_RSA_1024 ) {
		pubKeyMethod = IPMSG_RSA_1024;
	}
#endif	//SUPPORT_RSA_1024

#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_512  && host.EncryptionCapacity() & IPMSG_RSA_512 ) {
		pubKeyMethod = IPMSG_RSA_512;
	}
#endif	//SUPPORT_RSA_512
	//暗号化出来ないので、平文で送信。
	if ( pubKeyMethod == 0UL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("encryptionCapacity(%lx)\n", encryptionCapacity );
		printf("host.EncryptionCapacity()(%lx)\n", host.EncryptionCapacity() );
		printf("pubKeyMethod == 0UL\n");
#endif
		return false;
	}

	RSA *rsa = RSA_new();
	rsa->e = BN_new();
	if ( BN_hex2bn( &rsa->e, host.EncryptMethodHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		printf( "BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));
#endif
		RSA_free( rsa );
		return false;
	}
	rsa->n = BN_new();
	if ( BN_hex2bn( &rsa->n, host.PubKeyHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		printf( "BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));
#endif
		RSA_free( rsa );
		return false;
	}

	memset( iv, 0, sizeof( iv ) );
	
	unsigned char sharekey[EVP_MAX_KEY_LENGTH];
	int key_bytes_size = 0;
	unsigned long shareKeyMethod = 0UL;
#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_256 && host.EncryptionCapacity() & IPMSG_BLOWFISH_256 ) {
		shareKeyMethod = IPMSG_BLOWFISH_256;
		key_bytes_size = 256/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT

#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_128 && host.EncryptionCapacity() & IPMSG_BLOWFISH_128 && pubKeyMethod == IPMSG_RSA_1024 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_128 && host.EncryptionCapacity() & IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		shareKeyMethod = IPMSG_BLOWFISH_128;
		key_bytes_size = 128/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_256      && host.EncryptionCapacity() & IPMSG_RC2_256 ) {
		shareKeyMethod = IPMSG_RC2_256;
		key_bytes_size = 256/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_128      && host.EncryptionCapacity() & IPMSG_RC2_128 ) {
		shareKeyMethod = IPMSG_RC2_128;
		key_bytes_size = 128/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_40       && host.EncryptionCapacity() & IPMSG_RC2_40 && pubKeyMethod == IPMSG_RSA_512 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_40       && host.EncryptionCapacity() & IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		shareKeyMethod = IPMSG_RC2_40;
		key_bytes_size = 40/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_40
	//暗号化出来ないので、平文で送信。
	if ( shareKeyMethod == 0UL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("shareKeyMethod == 0UL\n");
#endif
		RSA_free( rsa );
		return false;
	}
	int enc_key_size = RSA_size( rsa );
	unsigned char *enc_key = (unsigned char *)calloc( enc_key_size + 1, 1 );
#if defined(INFO) || !defined(NDEBUG)
	printf( "enc_key_size(%d)\n", enc_key_size );
#endif
	if ( enc_key == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("enc_key == NULL\n");
#endif
		RSA_free( rsa );
		return false;
	}
	//共通鍵をRSA公開鍵で暗号化。
	int enc_key_len = RSA_public_encrypt( key_bytes_size, sharekey, enc_key, rsa, RSA_PKCS1_PADDING );
	if ( enc_key_len < 0 ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("enc_key_len < 0\n");
#endif
		RSA_free( rsa );
		free( enc_key );
		return false;
	}
	//共通鍵で本文を暗号化。
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init( &ctx );
	int seal_init_ret = 0;
#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 && pubKeyMethod == IPMSG_RSA_512 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_40_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_128
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 && pubKeyMethod == IPMSG_RSA_1024 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_bf_cbc(), NULL, NULL );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BLOWFISH_256 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_bf_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	char *enc_buf = (char *)calloc( optBufLen + key_bytes_size + 1, 1 );
	if ( enc_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("enc_buf == NULL\n");
#endif
		RSA_free( rsa );
		free( enc_key );
		return false;
	}
	int ol;
	int o_len = 0;
	int ret;
	// バッファが終わるまで繰り返す。
	for( int i = 0; i < optBufLen / key_bytes_size; i++ ){
		ret = EVP_EncryptUpdate( &ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], key_bytes_size );
		o_len += ol;
	}
	if( optBufLen % key_bytes_size != 0 ){
		ret = EVP_EncryptUpdate( &ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], optBufLen % key_bytes_size );
		o_len += ol;
	}
	ret = EVP_EncryptFinal( &ctx, (unsigned char*)&enc_buf[o_len], &ol );
	o_len += ol;

	int ob_len = 8 + 1 + ( enc_key_len * 2 ) + 1 + ( o_len * 2 ) + 1;
	char *out_buf = (char *)calloc( ob_len + 1, 1 );
	if ( out_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("out_buf == NULL\n");
#endif
		RSA_free( rsa );
		free( enc_key );
		free( enc_buf );
		return false;
	}
	snprintf( (char *)out_buf, ob_len, "%lx:", pubKeyMethod | shareKeyMethod );
	for( int i = 0; i < enc_key_len; i++ ) {
		char pout_hex[3];
		snprintf( pout_hex, 3, "%02x", (unsigned char)enc_key[i] );
		strcat( (char *)out_buf, pout_hex );
	}
	strcat( (char *)out_buf, PACKET_DELIMITER_STRING );
	for( int i = 0; i < o_len; i++ ) {
		char pout_hex[3];
		snprintf( pout_hex, 3, "%02x", (unsigned char)enc_buf[i] );
		strcat( (char *)out_buf, pout_hex );
	}
	*enc_optBufLen = strlen( (char *)out_buf );
	if ( opt_size > *enc_optBufLen ) {
		memset( optBuf, 0, *enc_optBufLen + 1 );
		memcpy( optBuf, out_buf, *enc_optBufLen );
	}
	RSA_free( rsa );
	free( enc_key );
	free( enc_buf );
	free( out_buf );
	if ( opt_size > *enc_optBufLen ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("TRUE!!\n");
#endif
		return true;
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("FALSE!!\n");
#endif
	return false;
#else	//HAVE_OPENSSL
	return false;
#endif	//HAVE_OPENSSL
}

/**
 * メッセージ復号化。
 * @param packet パケットオブジェクト（参照）
 * @retval true:復号化OK、false:復号化NG
 * 注：このメソッドはスレッドセーフでない。
 */
bool
IpMessengerAgentImpl::DecryptMsg( Packet &packet )
{
#ifdef HAVE_OPENSSL
	EVP_CIPHER_CTX ctx;
	unsigned char iv[EVP_MAX_IV_LENGTH];

	char *buf = (char *)calloc( packet.Option().size() + 1, 1);
	if ( buf == NULL ){
		return false;
	}
	memcpy( buf, packet.Option().c_str(), packet.Option().size());
	char *file_ptr = &buf[strlen( buf ) + 1];
	char *file_info = (char *)calloc( packet.Option().size(), 1 );
	int file_info_len = strlen( file_ptr );
	if ( file_info == NULL ) {
		free( buf );
		return false;
	}
	memcpy( file_info, file_ptr, file_info_len );
	IpMsgPrintBuf("file_ptr:", file_ptr, file_info_len);
	IpMsgPrintBuf("file_info:", file_info, file_info_len);

	char *token = buf;
	char *nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		return false;
	}
	char *dmyptr;
	unsigned long methods = strtoul( token, &dmyptr, 16 );

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		return false;
	}
	string ekey = token;

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		return false;
	}
	string emsg = token;

	string esign = "";
	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token != NULL ) {
		esign = token;
	}
	free( buf );
	RSA *rsa = NULL;
	int rsa_bits = 0;
	unsigned long pubKeyMethod = 0UL;
#ifndef WINCOMPAT
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_2048 ) {
		pubKeyMethod = IPMSG_RSA_2048;
		rsa_bits = 2048/8;
		rsa = RsaMax;
	}
#endif	//SUPPORT_RSA_2048
#endif	//WINCOMPAT

#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_1024 ) {
		pubKeyMethod = IPMSG_RSA_1024;
		rsa_bits = 1024/8;
		rsa = RsaMid;
	}
#endif	//SUPPORT_RSA_1024

#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_512 ) {
		pubKeyMethod = IPMSG_RSA_512;
		rsa_bits = 512/8;
		rsa = RsaMin;
	}
#endif	//SUPPORT_RSA_512
	//暗号化されていない？
	if ( pubKeyMethod == 0UL ) {
		return false;
	}
	//パディングを含むサイズ
	int ekey_len = ekey.length() / 2;
	if ( ekey_len % rsa_bits > 0 ) {
		ekey_len = ( ( ekey.length() / 2 ) / rsa_bits ) * ( rsa_bits + 1 );
	}
	unsigned char *ek = (unsigned char *)calloc( ekey_len + 1, 1 );
	if ( ek == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("calloc 1\n");
#endif
		perror("calloc");
		return false;
	}
	unsigned char *ekp = ek;
	for( unsigned int i = 0; i < ekey.length(); i += 2 ) {
		unsigned char ekc [3];
		ekc[0] = ekey.at( i );
		ekc[1] = ekey.at( i + 1 );
		ekc[2] = '\0';
		*ekp = (unsigned char)strtoul( (char *)ekc, &dmyptr, 16 );
		ekp++;
	}
	int ekl = ekey_len;
	unsigned long shareKeyMethod = 0UL;
	int key_bytes_size = 0;
#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_512 && shareKeyMethod == 0UL && methods & IPMSG_RC2_40 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		key_bytes_size = 40/8;
		shareKeyMethod = IPMSG_RC2_40;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_RC2_40\n");
#endif
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_128 ) {
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_RC2_128;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_RC2_128\n");
#endif
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_256 ) {
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_RC2_256;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_RC2_256\n");
#endif
	}
#endif	//SUPPORT_RC2_256

#endif	//WINCOMPAT
#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_1024 && shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_128 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_BLOWFISH_128;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_BF_128\n");
#endif
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_256 ){
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_BLOWFISH_256;
#if defined(INFO) || !defined(NDEBUG)
		printf("IPMSG_BF_256\n");
#endif
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	//暗号化されていない？
	if ( shareKeyMethod == 0UL ) {
		free( file_info );
		free( ek );
		return false;
	}
	unsigned char *emsg_buf = (unsigned char *)calloc( emsg.length() + 1, 1 );
	if ( emsg_buf == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("calloc 2\n");
#endif
		perror("calloc");
		free( file_info );
		free( ek );
		return false;
	}
	int data_len = 0;
	for( unsigned int i = 0; i < emsg.length(); i += 2 ) {
		unsigned char emc [3];
		emc[0] = emsg.at( i );
		emc[1] = emsg.at( i + 1 );
		emc[2] = '\0';
#if defined(INFO) || !defined(NDEBUG)
		printf("%d:emc=[%s]", data_len, emc);
#endif
		emsg_buf[data_len] = (unsigned char)strtoul( (char *)emc, &dmyptr, 16 );
#if defined(INFO) || !defined(NDEBUG)
		printf("[%02x]\n", emsg_buf[data_len]);
#endif
		data_len++;
	}

	EVP_PKEY pubkey;
	EVP_PKEY_set1_RSA( &pubkey, rsa );
	int open_init_ret = 0;
	memset( iv, 0, sizeof( iv ) );
#ifdef SUPPORT_RC2_40
#ifndef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_512 && shareKeyMethod == IPMSG_RC2_40 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_40_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_64_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_64_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_1024 && shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( &ctx, EVP_bf_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BROWFISH_256 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_bf_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			return false;
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	if ( open_init_ret <= 0 ){
		free( file_info );
		free( ek );
		free( emsg_buf );
		return false;
	}
	int tmp_len = 0;
	int tmp;
	unsigned char *optBuf = (unsigned char *)calloc( data_len + key_bytes_size + 1 + file_info_len + 1, 1 );
	if ( optBuf == NULL ){
		perror("calloc");
		free( file_info );
		free( ek );
		free( emsg_buf );
		return false;
	}

	int ret;
	ret = EVP_OpenUpdate( &ctx, &optBuf[tmp_len], &tmp, &emsg_buf[tmp_len], data_len );
	tmp_len += tmp;
	ret = EVP_OpenFinal( &ctx, &optBuf[tmp_len], &tmp );
	tmp_len += tmp;
	free( ek );
	free( emsg_buf );
	
	if ( file_info_len > 0 ){
		IpMsgPrintBuf( "optBuf(1):", (char *)optBuf, tmp_len );
		IpMsgPrintBuf( "file_info:", (char *)file_info, file_info_len );
		memcpy( &optBuf[tmp_len+1], file_info, file_info_len );
		tmp_len += ( file_info_len + 1 );
	}
	packet.setOption( string( (char *)optBuf, tmp_len ) );
	IpMsgPrintBuf( "optBuf(2):", (char *)optBuf, tmp_len );
	free( optBuf );
	free( file_info );
	return true;
#else	//HAVE_OPENSSL
	return false;
#endif	//HAVE_OPENSSL
}

RSA *
IpMessengerAgentImpl::GetOptimizedRsa( unsigned long cap ){
	RSA *rsa = NULL;
	unsigned long pubKeyMethod = 0UL;
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_2048 && cap& IPMSG_RSA_2048 ) {
		pubKeyMethod |= IPMSG_RSA_2048;
		rsa = RsaMax != NULL ? RsaMax : NULL;
	}
#endif	// SUPPORT_RSA_2048
#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_1024 && cap & IPMSG_RSA_1024 ) {
		pubKeyMethod |= IPMSG_RSA_1024;
		rsa = RsaMid != NULL ? RsaMid : NULL;
	}
#endif	// SUPPORT_RSA_1024
#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_512  && cap & IPMSG_RSA_512 ) {
		pubKeyMethod |= IPMSG_RSA_512;
		rsa = RsaMin != NULL ? RsaMin : NULL;
	}
#endif	// SUPPORT_RSA_512
	return rsa;
}
