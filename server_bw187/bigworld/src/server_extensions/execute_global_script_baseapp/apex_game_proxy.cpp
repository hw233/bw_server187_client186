/********************************************************************
	created:	2009/06/19
	filename: 	PyAPexProxy.cpp
	author:		LuoChengDi
	
	purpose:    PyAPexProxy�õ���Ϣ��������Ϣ���͸�ApexClient ��ApexItermSvr
*********************************************************************/
#include "apex_game_proxy.h"
#include "apex_public.h"
#include "cstdmf/debug.hpp"
#include "cstdmf/base64.h"
#include "cstdmf/concurrency.hpp"

DECLARE_DEBUG_COMPONENT(0);

PY_TYPEOBJECT(PyAPexProxy)

PY_BEGIN_METHODS(PyAPexProxy)
 	PY_METHOD(InitAPexProxy)
 	PY_METHOD(StartApexProxy)
 	PY_METHOD(StopApexProxy)
 	PY_METHOD(NoticeApexProxy)
 	PY_METHOD(SendMsgBuf2Client)
 	PY_METHOD(ApexKillRoleFromBuf)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(PyAPexProxy)
    //PY_ATTRIBUTE( mpfSend )
    //PY_ATTRIBUTE( mpfRecv )
PY_END_ATTRIBUTES()

#pragma pack(1)  
struct nUserIp_st 
{
	char	         Cmd_UserIpFlag;
	uint32           nClientIp;
};

#pragma pack ()

PyObject *PyAPexProxy::SendMsgToClientFunc   = Py_None;
PyObject *PyAPexProxy::KillUserFunc          = Py_None;
_FUNC_S_REC  PyAPexProxy::mpfRecv            = 0;    

static SimpleMutex s_apexSendMsgMutex;
static SimpleMutex s_apexKillRoleMutex;

PyAPexProxy::PyAPexProxy() :
	PyObjectPlus(&PyAPexProxy::s_type_)
{

}

PyAPexProxy::~PyAPexProxy( )
{
	DEBUG_MSG("PyAPexProxy destructor.\n");
}

//����		:	GameServer�ṩ��Apex Proxy �ķ��ͺ���;
//				������Ҫ�����Լ��Ľṹʵ�����ƹ��ܵĺ���;
//����
//pBuffer	: �������ݵĻ�����
//nLen		: �������ݵĳ���
long PyAPexProxy::NetSendToGameClient(signed int nRoleId,const char * pBuffer,int nLen)
{
	//����Ҫ����nSendId,���ҵ�base.Role,�ٵ���role.sendMsgToApexClient()
    DEBUG_MSG("PyAPexProxy::NetSendToGameClient()  \n");
    //TODO:���ýű���ĳ����������pBuffer������ݣ����ͳ�ȥ��
    std::string data = Base64::encode( pBuffer, nLen );
    TRACE_MSG("PyAPexProxy::NetSendToGameClient  data = %s  \n",data.c_str());
    apex_proxy_send_buf_st st;
    st.nRoleId = nRoleId;
    st.data = data;
    st.iLength = nLen;
 
    s_apexSendMsgMutex.grab();   	
    GetUniqueAPexProxyInstance()->mSendMsgQueue.push(st);
    s_apexSendMsgMutex.give();

    return 0;
}
//
//���� : GameServer �����˺���;
//����
//nId  ��ҵ�id������Ҫ����py�ű������˺�����
long PyAPexProxy::GameServerKillUser(signed int nRoleId,int iAction)
{
    DEBUG_MSG("PyAPexProxy::GameServerKillUser()  \n");
    //TODO:���ýű���ĳ�����������˺���
    apex_proxy_kill_buf_st st;
    st.nRoleId = nRoleId;
    st.iAction = iAction;
    
    s_apexKillRoleMutex.grab();   	
    GetUniqueAPexProxyInstance()->mKillRoleQueue.push(st);
    s_apexKillRoleMutex.give();
    
	return 0;
}
//����		:����ApexProxy
//����������ű����á�
PyObject * PyAPexProxy::py_StartApexProxy(PyObject *args )
{
	//���밴����˳�����;
	//1 . CHSStart   ;2 . CHSSetFunc
	CHSStart(&PyAPexProxy::NetSendToGameClient,mpfRecv);
	CHSSetFunc((void*)(&PyAPexProxy::GameServerKillUser),FLAG_KILLUSER);
	Py_Return;
}
//
//����		:ֹͣApexProxy
//����������ű����á�
PyObject * PyAPexProxy::py_StopApexProxy(PyObject *args )
{
    CHSEnd();//���������
	s_apexSendMsgMutex.grab();
    while(!mSendMsgQueue.empty())
    {
        mSendMsgQueue.pop( );
    }
    s_apexKillRoleMutex.grab(); 
    while(!mKillRoleQueue.empty())
    {
        mKillRoleQueue.pop( );
    }    
	//CHSEnd();//���������
	mpfRecv               = 0;
	SendMsgToClientFunc   = Py_None;
	KillUserFunc          = Py_None;
	
    s_apexKillRoleMutex.give();
    s_apexSendMsgMutex.give();
	Py_Return;
}
// PyAPexProxy::_py_StartApexProxy 
//����		:֪ͨapexProxy����: L��S ��R��G��T ����Ϣ
//����������ű����á�����Ĳ������ű��ڲ�ͬ�ط�����ʱ���ٸ�����Ҫ��
PyObject * PyAPexProxy::py_NoticeApexProxy(PyObject *args )
{
    char cMsgId = 0;
    unsigned int nRoleId = 0;
    //PyObject* object;
    const char *pBuf = 0;
    unsigned long tempLongVal = 0;
    int tempRetVal = 0;
    int  nBufLen = 0;
    std::string inData;
    std::string outData;
    nUserIp_st nSt;
    char *stopstring = 0;
    
	//if (!PyArg_ParseTuple(args, "ciOi", &cMsgId,&nRoleId,&object, &nBufLen))
	if (!PyArg_ParseTuple(args, "cisi", &cMsgId,&nRoleId,&pBuf, &nBufLen))
    {
        ERROR_MSG("py_NoticeApexProxy PyArg_ParseTuple error....\n");        
        Py_Return;
    }
    TRACE_MSG("py_NoticeApexProxy cMsgId=%c,nRoleId=%d,pBuf=%s,nBufLen=%d  \n",cMsgId,nRoleId,pBuf, nBufLen);
    switch(cMsgId)
    {
        case 'L':
        	break;
        case 'S':
           tempLongVal = strtoul( pBuf, &stopstring, 10 );
           nSt.Cmd_UserIpFlag = ClientIpFlag;
           nSt.nClientIp = (uint32)tempLongVal;
           TRACE_MSG("NoticeApexProxy S msg return IP = %ud\n",nSt.nClientIp);
           pBuf = (const char *)&nSt;
        	break;
        case 'R':
        	tempRetVal = atoi(pBuf);
            TRACE_MSG("NoticeApexProxy R msg return re = %d\n",tempRetVal);
        	pBuf = (const char *)&tempRetVal;
        	break;
        case 'T':
        	inData = pBuf;
            if(Base64::decode(inData,outData ))
            {
                if(nBufLen != outData.length())
                {
                    ERROR_MSG("Base64 decode error : nBufLen != outData.length()  \n");
                }
            }
            pBuf = outData.c_str();
        	break;
        case 'G':
        	break; 
        default:
        	ERROR_MSG("py_NoticeApexProxy cMsgId type error! cMsgId = %c\n",cMsgId);
        	Py_Return;
        	break;        	                              
    }
    //INFO_MSG("cMsgId=%c,nRoleId=%d,pBuf=%s,nBufLen=%d  \n",cMsgId,nRoleId,pBuf, nBufLen);
    if(mpfRecv)
	{
        mpfRecv(cMsgId,nRoleId,pBuf,nBufLen);
	}
    Py_Return;
}

PyObject *PyAPexProxy::py_SendMsgBuf2Client(PyObject *args )
{
	if(! s_apexSendMsgMutex.grabTry())
    {
        Py_Return;
    }
	while( !mSendMsgQueue.empty())
    {
        apex_proxy_send_buf_st st;
        st = mSendMsgQueue.front();

        if(SendMsgToClientFunc != Py_None)
        {
            TRACE_MSG("Script::call nRoleId = %d,data = %s,length = %d \n",st.nRoleId,st.data.c_str(),st.iLength);
            Py_XINCREF(SendMsgToClientFunc);
            Script::call( SendMsgToClientFunc, Py_BuildValue( "(isi)", st.nRoleId,st.data.c_str(),st.iLength ));
            mSendMsgQueue.pop();
        }
        else
        {
        	ERROR_MSG("PyAPexProxy::py_SendMsgBuf2Client ==> SendMsgToClientFunc == Py_None\n");
        	break;
        }
    }
    s_apexSendMsgMutex.give();
	Py_Return;
}

PyObject * PyAPexProxy::py_ApexKillRoleFromBuf(PyObject *args )
{
	if(!s_apexKillRoleMutex.grabTry())
    {
        Py_Return;
    }
	while( !mKillRoleQueue.empty())
    {
        apex_proxy_kill_buf_st st;
        st = mKillRoleQueue.front();

        if(KillUserFunc != Py_None)
        {
		    Py_XINCREF(KillUserFunc);
            Script::call( KillUserFunc, Py_BuildValue( "(ii)", st.nRoleId,st.iAction ));
            mKillRoleQueue.pop();
        }
        else
        {
        	ERROR_MSG("PyAPexProxy::py_ApexKillRoleFromBuf ==> KillUserFunc == Py_None\n");
        	break;
        }
    }
	s_apexKillRoleMutex.give();
	Py_Return;
}


PyObject *PyAPexProxy::pyNew(PyObject *args)
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError,
			"PyAPexProxy::pyNew expects no arguments" );
		return NULL;
	}
	return new PyAPexProxy();
}

//��ĳ�ʼ������
//����������ű����á����ú���SendFunc����NetSendToGameServer�Ժ����
PyObject * PyAPexProxy::py_InitAPexProxy(PyObject *args )
{
    DEBUG_MSG("PyAPexProxy::init() ....\n");
    PyObject* pySendFunc = NULL;
    PyObject* pyKillFunc = NULL;

	if (!PyArg_ParseTuple( args, "OO", &pySendFunc,&pyKillFunc ) )
	{
		PyErr_SetString( PyExc_TypeError, "PyAPexProxy::init: Argument parsing error.\n" );
		Py_Return;
	}

	if (pySendFunc != Py_None)
	{
		Py_XDECREF( SendMsgToClientFunc );
        SendMsgToClientFunc = pySendFunc;
		Py_XINCREF( SendMsgToClientFunc );
	}
	if (pyKillFunc != Py_None)
	{
		Py_XDECREF( KillUserFunc );
        KillUserFunc = pyKillFunc;
		Py_XINCREF( KillUserFunc );
	}
    Py_Return;
}

PyObject *PyAPexProxy::pyGetAttribute(const char *attr)
{
    PY_GETATTR_STD();
    return PyObjectPlus::pyGetAttribute(attr);
}



int PyAPexProxy::pySetAttribute(const char *attr, PyObject *value)
{
    PY_SETATTR_STD();
    return PyObjectPlus::pySetAttribute(attr, value);
}

PyAPexProxy* GetUniqueAPexProxyInstance()
{
    static PyAPexProxy* uniqueInstance = NULL;
    if(uniqueInstance == NULL)
    {
       DEBUG_MSG("GetUniqueAPexProxyInstance(uniqueInstance == NULL) ....\n");
       uniqueInstance = new PyAPexProxy();
    }
    return uniqueInstance;
}
