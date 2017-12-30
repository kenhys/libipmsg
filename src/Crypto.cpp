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

using namespace ipmsg;

#define WINCOMPAT

#ifdef HAVE_OPENSSL
#ifdef WINCOMPAT
#define SUPPORT_RSA_512
#define SUPPORT_RSA_1024
#define SUPPORT_RC2_40
#define SUPPORT_BLOWFISH_128
#else	// WINCOMPAT
#define SUPPORT_RSA_512
#define SUPPORT_RSA_1024
#define SUPPORT_RSA_2048
#define SUPPORT_RC2_40
#define SUPPORT_RC2_128
#define SUPPORT_RC2_256
#define SUPPORT_BLOWFISH_128
#define SUPPORT_BLOWFISH_256
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

#define ERR_BUF_SIZE	1024
/**
 * 暗号関連の初期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::CryptoInit()
{
	IPMSG_FUNC_ENTER( "void IpMessengerAgentImpl::CryptoInit()" );
#ifdef HAVE_OPENSSL
	ERR_load_crypto_strings();

	encryptionCapacity = 0UL;
	RsaMax = NULL;
#ifdef SUPPORT_RSA_2048
	RsaMax = RSA_generate_key( RSA_KEY_LENGTH_MAXIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMax == NULL ) {
		char errbuf[ERR_BUF_SIZE];
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::CryptoInit In Encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );fflush(stdout);
	} else {
		encryptionCapacity |= IPMSG_RSA_2048;
		IpMsgPrintLogTime(stdout);
		printf("Encryption extention enabled.(RSA2048)\n");fflush(stdout);
	}
#endif	//SUPPORT_RSA_2048
	RsaMid = NULL;
#ifdef SUPPORT_RSA_1024
	RsaMid = RSA_generate_key( RSA_KEY_LENGTH_MIDIUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMid == NULL ) {
		char errbuf[ERR_BUF_SIZE];
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::CryptoInit In Encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );fflush(stdout);
	} else {
		encryptionCapacity |= IPMSG_RSA_1024;
		IpMsgPrintLogTime(stdout);
		printf("Encryption extention enabled.(RSA1024)\n");fflush(stdout);
	}
#endif	//SUPPORT_RSA_1024
	RsaMin = NULL;
#ifdef SUPPORT_RSA_512
	RsaMin = RSA_generate_key( RSA_KEY_LENGTH_MINIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMin == NULL ) {
		char errbuf[ERR_BUF_SIZE];
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::CryptoInit In Encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );fflush(stdout);
	} else {
		encryptionCapacity |= IPMSG_RSA_512;
		IpMsgPrintLogTime(stdout);
		printf("Encryption extention enabled.(RSA512)\n");fflush(stdout);
	}
#endif	//SUPPORT_RSA_512
	if ( encryptionCapacity == 0UL ) {
		//暗号化無効
		IpMsgPrintLogTime(stdout);
		printf("Encryption extention disabled.\n");fflush(stdout);
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
	IPMSG_FUNC_EXIT;
}

/**
 * 暗号関連の終期化。
 * 注：このメソッドはスレッドセーフでない。
 */
void
IpMessengerAgentImpl::CryptoEnd()
{
	IPMSG_FUNC_ENTER( "void IpMessengerAgentImpl::CryptoEnd()" );
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
	IPMSG_FUNC_EXIT;
}

/**
 * メッセージ暗号化。
 * @param host 送信先ホスト
 * @param optBuf パケットオプション部のバッファのアドレス
 * @param optBufLen パケットオプション部のバッファの現在の有効データ長
 * @param enc_optBufLen 暗号化済のパケットオプション部のバッファの有効データ長のアドレス
 * @param opt_size パケットオプション部のバッファのサイズ
 * @retval true:復号化成功
 * @retval false:復号化失敗
 */
bool
IpMessengerAgentImpl::EncryptMsg( const HostListItem& host, unsigned char *optBuf, int optBufLen, int *enc_optBufLen, int opt_size )
{
	IPMSG_FUNC_ENTER( "bool IpMessengerAgentImpl::EncryptMsg( const HostListItem& host, unsigned char *optBuf, int optBufLen, int *enc_optBufLen, int opt_size )" );
#ifdef HAVE_OPENSSL
	unsigned long pubKeyMethod = 0UL;
	unsigned char iv[EVP_MAX_IV_LENGTH];

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
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg encryptionCapacity(%lx)\n", encryptionCapacity );fflush(stdout);
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg host.EncryptionCapacity()(%lx)\n", host.EncryptionCapacity() );fflush(stdout);
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg pubKeyMethod == 0UL\n");fflush(stdout);
#endif
		IPMSG_FUNC_RETURN( false );
	}

	RSA *rsa = RSA_new();
	BIGNUM *e;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	e = rsa->e = BN_new();
#else
	e = BN_new();
	RSA_set0_key(rsa, NULL, e, NULL);
#endif
	if ( BN_hex2bn( &e, host.EncryptMethodHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		char errbuf[ERR_BUF_SIZE];
		IpMsgPrintLogTime(stdout);
		printf( "IpMessengerAgentImpl::EncryptMsg BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
	}
	BIGNUM *n;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	n = rsa->n = BN_new();
#else
	n = BN_new();
	RSA_set0_key(rsa, n, NULL, NULL);
#endif
	if ( BN_hex2bn( &n, host.PubKeyHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		char errbuf[ERR_BUF_SIZE];
		IpMsgPrintLogTime(stdout);
		printf( "IpMessengerAgentImpl::EncryptMsg BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
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
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg shareKeyMethod == 0UL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
	}
	int enc_key_size = RSA_size( rsa );
	unsigned char *enc_key = (unsigned char *)calloc( enc_key_size + 1, 1 );
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
	printf( "IpMessengerAgentImpl::EncryptMsg enc_key_size(%d)\n", enc_key_size );fflush(stdout);
#endif
	if ( enc_key == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg enc_key == NULL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
	}
	//共通鍵をRSA公開鍵で暗号化。
	int enc_key_len = RSA_public_encrypt( key_bytes_size, sharekey, enc_key, rsa, RSA_PKCS1_PADDING );
	if ( enc_key_len < 0 ) {
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg enc_key_len < 0\n");fflush(stdout);
#endif
		RSA_free( rsa );
		free( enc_key );
		IPMSG_FUNC_RETURN( false );
	}
	//共通鍵で本文を暗号化。
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX ctx_;
	EVP_CIPHER_CTX *ctx;
	EVP_CIPHER_CTX_init( &ctx_ );
	ctx = &ctx_;
#else
	EVP_CIPHER_CTX *ctx;
	EVP_CIPHER_CTX_init( ctx );
#endif
	int seal_init_ret = 0;
#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 && pubKeyMethod == IPMSG_RSA_512 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( ctx, EVP_rc2_40_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		seal_init_ret = EVP_EncryptInit( ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		seal_init_ret = EVP_EncryptInit( ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_128
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 && pubKeyMethod == IPMSG_RSA_1024 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( ctx, EVP_bf_cbc(), NULL, NULL );
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BLOWFISH_256 ) {
		seal_init_ret = EVP_EncryptInit( ctx, EVP_bf_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		seal_init_ret = EVP_EncryptInit( ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	char *enc_buf = (char *)calloc( optBufLen + key_bytes_size + 1, 1 );
	if ( enc_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg enc_buf == NULL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		free( enc_key );
		IPMSG_FUNC_RETURN( false );
	}
	int ol;
	int o_len = 0;
	int ret;
	// バッファが終わるまで繰り返す。
	for( int i = 0; i < optBufLen / key_bytes_size; i++ ){
		ret = EVP_EncryptUpdate( ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], key_bytes_size );
		o_len += ol;
	}
	if( optBufLen % key_bytes_size != 0 ){
		ret = EVP_EncryptUpdate( ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], optBufLen % key_bytes_size );
		o_len += ol;
	}
	ret = EVP_EncryptFinal( ctx, (unsigned char*)&enc_buf[o_len], &ol );
	o_len += ol;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	EVP_CIPHER_CTX_free(ctx);
#endif
	int ob_len = 8 + 1 + ( enc_key_len * 2 ) + 1 + ( o_len * 2 ) + 1;
	char *out_buf = (char *)calloc( ob_len + 1, 1 );
	if ( out_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg out_buf == NULL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		free( enc_key );
		free( enc_buf );
		IPMSG_FUNC_RETURN( false );
	}
	snprintf( (char *)out_buf, ob_len, "%lx:", pubKeyMethod | shareKeyMethod );
	for( int i = 0; i < enc_key_len; i++ ) {
		char pout_hex[3];
		IpMsgUCharToHexString( pout_hex, (unsigned char)enc_key[i] );
		strcat( (char *)out_buf, pout_hex );
	}
	strcat( (char *)out_buf, PACKET_DELIMITER_STRING );
	for( int i = 0; i < o_len; i++ ) {
		char pout_hex[3];
		IpMsgUCharToHexString( pout_hex, (unsigned char)enc_buf[i] );
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
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg Return value is true.\n");fflush(stdout);
#endif
		IPMSG_FUNC_RETURN( true );
	}

#if defined(INFO) || !defined(NDEBUG)
	IpMsgPrintLogTime(stdout);
	printf("IpMessengerAgentImpl::EncryptMsg Return value is false.\n");fflush(stdout);
#endif
	IPMSG_FUNC_RETURN( false );
#else	//HAVE_OPENSSL
	IPMSG_FUNC_RETURN( false );
#endif	//HAVE_OPENSSL
}

/**
 * メッセージ復号化。
 * @param packet パケットオブジェクト（参照）
 * @retval true:復号化成功
 * @retval false:復号化失敗
 */
bool
IpMessengerAgentImpl::DecryptMsg( const Packet &packet, std::string& msg )
{
	IPMSG_FUNC_ENTER( "bool IpMessengerAgentImpl::DecryptMsg( const Packet &packet, std::string& msg )" );
#ifdef HAVE_OPENSSL
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX ctx_;
	EVP_CIPHER_CTX *ctx;
	ctx = &ctx_;
#else
	EVP_CIPHER_CTX *ctx;
	ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx);
#endif
	unsigned char iv[EVP_MAX_IV_LENGTH];

	char *buf = (char *)calloc( packet.Option().size() + 1, 1);
	if ( buf == NULL ){
		IPMSG_FUNC_RETURN( false );
	}
	memcpy( buf, packet.Option().c_str(), packet.Option().size());
	char *file_ptr = &buf[strlen( buf ) + 1];
	char *file_info = (char *)calloc( packet.Option().size(), 1 );
	int file_info_len = strlen( file_ptr );
	if ( file_info == NULL ) {
		free( buf );
		IPMSG_FUNC_RETURN( false );
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
		IPMSG_FUNC_RETURN( false );
	}
	char *dmyptr;
	unsigned long methods = strtoul( token, &dmyptr, 16 );

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		IPMSG_FUNC_RETURN( false );
	}
	std::string ekey = token;

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		IPMSG_FUNC_RETURN( false );
	}
	std::string emsg = token;

	std::string esign = "";
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
		IPMSG_FUNC_RETURN( false );
	}
	//パディングを含むサイズ
	int ekey_len = ekey.length() / 2;
	if ( ekey_len % rsa_bits > 0 ) {
		ekey_len = ( ( ekey.length() / 2 ) / rsa_bits ) * ( rsa_bits + 1 );
	}
	unsigned char *ek = (unsigned char *)calloc( ekey_len + 1, 1 );
	if ( ek == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::DecryptMsg calloc 1\n");fflush(stdout);
#endif
		perror("calloc");
		IPMSG_FUNC_RETURN( false );
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
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_RC2_40\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_128 ) {
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_RC2_128;
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_RC2_128\n");fflush(stdout);
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
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_RC2_256\n");fflush(stdout);
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
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_BF_128\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_256 ){
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_BLOWFISH_256;
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_BF_256\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	//暗号化されていない？
	if ( shareKeyMethod == 0UL ) {
		free( file_info );
		free( ek );
		IPMSG_FUNC_RETURN( false );
	}
	unsigned char *emsg_buf = (unsigned char *)calloc( emsg.length() + 1, 1 );
	if ( emsg_buf == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		fprintf( stderr, "IpMessengerAgentImpl::DecryptMsg calloc 2\n" );fflush(stdout);
#endif
		perror("calloc");
		free( file_info );
		free( ek );
		IPMSG_FUNC_RETURN( false );
	}
	int data_len = 0;
	for( unsigned int i = 0; i < emsg.length(); i += 2 ) {
		unsigned char emc [3];
		emc[0] = emsg.at( i );
		emc[1] = emsg.at( i + 1 );
		emc[2] = '\0';
#if defined(INFO) || !defined(NDEBUG)
		IpMsgPrintLogTime(stdout);
		printf("IpMessengerAgentImpl::DecryptMsg %d:emc=[%s]", data_len, emc);fflush(stdout);
#endif
		emsg_buf[data_len] = (unsigned char)strtoul( (char *)emc, &dmyptr, 16 );
#if defined(INFO) || !defined(NDEBUG)
		printf("[%02x]\n", emsg_buf[data_len]);fflush(stdout);
#endif
		data_len++;
	}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_PKEY pubkey_;
	EVP_PKEY *pubkey = &pubkey_;
#else
	EVP_PKEY *pubkey = EVP_PKEY_new();
#endif
	EVP_PKEY_set1_RSA( pubkey, rsa );
	int open_init_ret = 0;
	memset( iv, 0, sizeof( iv ) );
#ifdef SUPPORT_RC2_40
#ifndef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_512 && shareKeyMethod == IPMSG_RC2_40 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( ctx, EVP_rc2_40_cbc(), ek, ekl, iv, pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( ctx, NULL, ek, ekl, iv, pubkey );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		open_init_ret = EVP_OpenInit( ctx, EVP_rc2_64_cbc(), ek, ekl, iv, pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		open_init_ret = EVP_OpenInit( ctx, EVP_rc2_64_cbc(), ek, ekl, iv, pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( ctx, NULL, ek, ekl, iv, pubkey );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_1024 && shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( ctx, EVP_bf_cbc(), ek, ekl, iv, pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( ctx, NULL, ek, ekl, iv, pubkey );
	}
#endif//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BROWFISH_256 ) {
		open_init_ret = EVP_OpenInit( ctx, EVP_bf_cbc(), ek, ekl, iv, pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( ctx, key_bytes_size );				//鍵長の設定
		open_init_ret = EVP_OpenInit( ctx, NULL, ek, ekl, iv, pubkey );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_free(ctx);
	EVP_PKEY_free(pubkey);
#endif
	if ( open_init_ret <= 0 ){
		free( file_info );
		free( ek );
		free( emsg_buf );
		IPMSG_FUNC_RETURN( false );
	}
	int tmp_len = 0;
	int tmp;
	unsigned char *optBuf = (unsigned char *)calloc( data_len + key_bytes_size + 1 + file_info_len + 1, 1 );
	if ( optBuf == NULL ){
		perror("calloc");
		free( file_info );
		free( ek );
		free( emsg_buf );
		IPMSG_FUNC_RETURN( false );
	}

	int ret;
	ret = EVP_OpenUpdate( ctx, &optBuf[tmp_len], &tmp, &emsg_buf[tmp_len], data_len );
	tmp_len += tmp;
	ret = EVP_OpenFinal( ctx, &optBuf[tmp_len], &tmp );
	tmp_len += tmp;
	free( ek );
	free( emsg_buf );
	optBuf[tmp_len] = '\0';
	
	if ( file_info_len > 0 ){
		IpMsgPrintBuf( "optBuf(1):", (char *)optBuf, tmp_len );
		IpMsgPrintBuf( "file_info:", (char *)file_info, file_info_len );
		memcpy( &optBuf[tmp_len+1], file_info, file_info_len );
		tmp_len += ( file_info_len + 1 );
	}
	msg = std::string( (char *)optBuf, tmp_len );
	IpMsgPrintBuf( "optBuf(2):", (char *)optBuf, tmp_len );
	free( optBuf );
	free( file_info );
	IPMSG_FUNC_RETURN( true );
#else	//HAVE_OPENSSL
	IPMSG_FUNC_RETURN( false );
#endif	//HAVE_OPENSSL
}

/**
 * 最適なRSAオブジェクトを選択して返却する。
 * @param cap 自分の暗号化能力を示すフラグ。
 * @retval 暗号化に使用するRSAオブジェクト。
 */
RSA *
IpMessengerAgentImpl::GetOptimizedRsa( unsigned long cap )
{
	IPMSG_FUNC_ENTER( "RSA *IpMessengerAgentImpl::GetOptimizedRsa( unsigned long cap )" );
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
	IPMSG_FUNC_RETURN( rsa );
}
