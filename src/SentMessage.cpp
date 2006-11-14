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

/**
 * �ѥ��åȤ���ź�եե�����򸡺����ޤ���
 * ���ѥ��åȤ���ե�����ID����Ф��ե�����ID����ź�եե�����򸡺�����AttachFile�Υ��ƥ졼�����֤��ޤ���
 * @param packet �ѥ��åȥ��֥�������
 * @retval AttachFile�Υ��ƥ졼�������դ���ʤ���硢end()���֤���
 */
vector<AttachFile>::iterator
SentMessage::FindAttachFileByPacket( Packet packet )
{
	char *dmyptr;
	char *startptr;
	strtoul( packet.Option().c_str(), &dmyptr, 16 );
	startptr = ++dmyptr;
	int packet_file_id = strtoul( startptr, &dmyptr, 16 );
	startptr = ++dmyptr;

	vector<AttachFile>::iterator FoundFile;
	FoundFile = Files().FindByFileId( packet_file_id );
	return FoundFile;
}
