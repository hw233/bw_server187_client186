#ifndef DB_PWD_CDES_HEADER_FILE
#define DB_PWD_CDES_HEADER_FILE

#include <string>
#include <cmath>
//#include <stdio.h>
//using namespace std;

class CPwdDesTool
{
public:
	//����ʹ�õĽӿڣ�����Ǽ���
	std::string EncryptString(std::string srcEnc);
	//����ʹ�õĽӿڣ�����ǽ���
	std::string DecryptString(std::string srcDec);

    //��16���ַ�����ʼ��key���ַ�Ҫ��0--9,a--f,A--F���
	CPwdDesTool(std::string key="",std::string iv = "");
	~CPwdDesTool();
    
    //int CPwdDesTool::testSSLdes( );
private:
	std::string m_strKey;
	std::string m_strIV;
	
	//���ֽ�"ABC123",(0xABC123),ת�����ַ���"ABC123",(0x414243313233)Ĭ����ABC��д�����
	std::string ConverByesToChars( std::string src, bool toupper = true);
	
	//���ַ���"ABC123",(0x414243313233),ת�����ֽ�"ABC123",(0xABC123),srcԭ����Ҫ��ż�����ַ�������񷵻ؿմ�����ʾת��ʧ��
	std::string ConverCharsToByes( std::string src);
	
	//��src������ַ����ǲ���ȫ����0--9,a--f,A--F��ɡ��Ǿͷ���1�����Ǿͷ���0����ConverCharsToByes����
	int CheckChars( std::string src );
};

//---------------------------------------------------------------------------
#endif
