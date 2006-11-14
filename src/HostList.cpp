/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * Windows��IP��å��󥸥㡼���̿�����饤�֥�ꡣ
 * ���Υ饤�֥��ϥ���åɥ����դǤʤ�����դ�ɬ�ס�
 * �ץ�ȥ�������������Τ�¾�θ���Ȥμ������̿��Ǥ���褦��
 * Shift_JIS����ˤϤʤäƤ��ʤ���
 * â�����ץ�ȥ������ʸ��ζ��ڤ�ʸ���Ȥ���'\0'��Ȥ����Ȥ�ͭ�뤿�ᡢ
 * UTF16��UCS���ͤʡ�ʸ�������ɤ�'\0'��ޤ�ʸ�������ɤˤ��б����Ƥ��ʤ���
 *
 * ���Υ饤�֥��ΥС������Ǥϡ��Ź�Ϥ�OpenSSL��crypto�饤�֥�����Ѥ��뤿�ᡢ
 * ��������RSA512,1024,2048 ���̸���RC2-40,RC2-128,RC2-256,BLOWFISH-128,BLOWFISH-256
 * �򥵥ݡ��ȤǤ��롣���̿���Υ��饤����ȤκǶ����٤ΰŹ��ͥ��������Ȥ��Ƽ�ư���򤹤롣��
 * â����Windows�Ǥϡʥ�������Υ����Ȥˤ��Сˡּ�ȴ���פμ����ʤΤǺǶ����٤�����
 * ���Ƥ��ޤ��ȡ�Windows�Ǥϼ��ʤ�ǽ�Ϥ�����������Ƥ���ΤǰŹ沽���줿�ѥ��åȤ�����
 * ����ʤ����Ȥ���ա�
 *
 * Windows�ǤǤ�RSA512+RC2-40, RSA1024+BLOWFISH128�������ݡ��Ȥ��Ƥ��ʤ���
 * �����WINCOMPAT�ޥ���ˤ�ꥵ�ݡ��Ȥ��롣��WINCOMPAT���������ȡ�Windows�Ǥμ���ǽ�Ϥε���������
 * ��碌�ưŹ沽��ˡ�����򤹤롣��
 *
 * ��������ɼ���ե�����̾���Ѵ��ϥե�����̾����С������֥������ȤǹԤ���
 * ������ե����륷���ƥ�Υ��󥳡��ǥ��󥰤ȥ�å������Υ��󥳡��ǥ��󥰤�����Ѵ����륪�֥������Ȥǡ�
 * �饤�֥��Ȥ��Ƥϲ��۴��쥯�饹��FileNameConverter���Ѵ���Ԥ�ʤ�NullFileNameConverter(�ǥե����)
 * ���󶡤��롣���ץꥱ��������ɬ�פ˱����ơ�FileNameConverter��Ѿ����ơ��ե�����̾�Ѵ����å�
 * ���Ȥ߹���ɬ�פ����롣��Windows�Ǥ�Shift_JIS(CP932)�����̤�Unix�Ǥ�eucJP or UTF-8�����顣��
 *
 * ���ޥ�ɥ��ץ����μ����䡢��ȥ饤���μ�����Ŭ���Ǥ���Τǻ��Ѥˤ���դ�ɬ�ס�
 * ���ϥ��ץ�¦�μ����Ȥ��������ϴ��Τ��ʤ���
 *
 * �ƥꥹ�ȡʥۥ��ȥꥹ�ȡ�������å������ꥹ�ȡ�������å������ꥹ�ȡ�ź�եե�����ꥹ�ȡˤ�
 * ����API��������������Ȼפ������������������롣
 *
 * automake/autoconf���⥪�������������롣
 *
 * ���Ĥ�
 * ������Ū�˥���åɥ����դǤʤ���
 * �����饹��ʬ�䤬���ޤ�����
 * �������å���Υ��󥹥��󥹤�¿�����롣��äȥҡ��׾�ˤȤ�٤���
 * 
 * @version 0.1.0
 * @author kuninobu niki(nikikuni@yahoo.co.jp)
 * @created 2006.8.7(proto-type implimentation started)
 * @updated 2006.9.5(rc1,encrypt-decrypt function completed)
 * @target Linux(Main test environment), and Unix(no test), and more Unix clones.
 */
  
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "ipmsg.h"
using namespace std;

#define HOST_LIST_SEND_MAX_AT_ONCE	100

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��ɲä��롣
 * @param host �ۥ��Ⱦ���
 */
void HostList::AddHost( HostListItem host )
{
	bool is_found = false;
	for( unsigned int i = 0; i < items.size(); i++ ){
		if ( host.Equals( items.at( i ) ) ) {
			is_found = true;
			break;
		}
	}
	if ( !is_found ) {
		items.push_back( host );
	}
}

/**
 * �ۥ��Ⱦ����ۥ��ȥꥹ�Ȥ��������롣
 * @param �ۥ���̾
 */
void HostList::DeleteHost( string hostname )
{
	for( vector<HostListItem>::iterator ix = items.begin(); ix < items.end(); ix++ ){
		if ( ix->HostName() == hostname ) {
			items.erase( ix );
			break;
		}
	}
}

/**
 * �ۥ��ȥꥹ��������ʸ�����������롣
 * @param start ���ϰ���
 */
string HostList::ToString( int start )
{
	char buf[MAX_UDPBUF];
	string ret;
	unsigned int max;

	max = start + HOST_LIST_SEND_MAX_AT_ONCE - 1;
	if ( max > items.size() ) {
		max = items.size();
	}
	snprintf( buf, sizeof( buf ), "%-5d\a%-5d\a", start , max - start < 0 ? 0 : max - start );
	ret = buf;
	for( unsigned int i = start ; i < max; i++ ){
		HostListItem item = items.at( i );
		sprintf( buf, "%s\a%s\a%ld\a%s\a%d\a%s\a%s\a",
						item.UserName() == "" ? "\b" : item.UserName().c_str(),
						item.HostName() == "" ? "\b" : item.HostName().c_str(),
						item.CommandNo(),
						item.IpAddress() == "" ? "\b" : item.IpAddress().c_str(),
						htons( item.PortNo() ),
						item.Nickname() == "" ? "\b" : item.Nickname().c_str(),
						item.GroupName() == "" ? "\b" : item.GroupName().c_str() );
		ret = ret + buf;
	}
	return ret;
}

/**
 * �ѥ��åȥ��֥������Ȥ���ۥ��ȥꥹ�ȥ����ƥ���������롣
 * @param packet �ѥ��åȥ��֥�������
 * @retval �ۥ��ȥꥹ�ȥ����ƥ�
 */
HostListItem HostList::CreateHostListItemFromPacket( Packet packet )
{
	HostListItem ret;
	ret.setHostName( packet.HostName() );
	ret.setUserName( packet.UserName() );
	ret.setCommandNo( packet.CommandMode() | packet.CommandOption() );
	ret.setIpAddress( inet_ntoa( packet.Addr().sin_addr ) );
#if defined(INFO) || !defined(NDEBUG)
	printf( "CreateHostListItemFromPacket port %d\n", packet.Addr().sin_port );
#endif
	ret.setPortNo( packet.Addr().sin_port );
	unsigned int loc = packet.Option().find_first_of( '\0' );
	if ( loc == string::npos ) {
		ret.setNickname( packet.Option() );
		ret.setGroupName( "" );
	} else {
		ret.setNickname( packet.Option().substr( 0, loc ) );
		ret.setGroupName( packet.Option().substr( loc + 1 ) );
	}
	return ret;
}

/**
 * �ۥ��ȥꥹ�ȥ����ƥ४�֥������Ȥ���ʬ�Ȱ��פ��뤫���֤���
 * @param item �ۥ��ȥꥹ�ȥ����ƥ�
 * @retval ���ס�true�����פ��ʤ���false
 */
bool HostListItem::Equals( HostListItem item )
{
	return	item.UserName() == UserName() &&
			item.HostName() == HostName() &&
			item.IpAddress() == IpAddress();
//			item.Nickname() == Nickname() &&
//			item.GroupName() == GroupName() &&
//			item.PortNo() == PortNo();
}

//end of source
