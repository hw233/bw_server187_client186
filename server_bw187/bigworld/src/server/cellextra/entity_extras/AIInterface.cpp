#include "AIInterface.hpp"
#include "../utils/py_array_proxy.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

PY_TYPEOBJECT( AIInterface )

PY_BEGIN_METHODS( AIInterface )
	PY_METHOD( onFightAIHeartbeat_AIInterface_cpp )
	PY_METHOD( aiCommonCheck_AIInterface_cpp )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( AIInterface )
PY_END_ATTRIBUTES()

const AIInterface::Instance<AIInterface>
	AIInterface::instance( &AIInterface::s_attributes_.di_ );

AIInterface::AIInterface( Entity &e ): EntityExtra( e )
{
}

AIInterface::~AIInterface()
{
}


PyObject * AIInterface::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return this->EntityExtra::pyGetAttribute( attr );
}

int AIInterface::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return this->EntityExtra::pySetAttribute( attr, value );
}

void AIInterface::onFightAIHeartbeat_AIInterface_cpp()
{
	PyObject * pySelfEntityPtr = ( PyObject * )&entity_;
	PyObject *pTempObjRef1, *pTempObjRef2, *pTempObjRef3;

	pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr, "fightStateAICount" );
	if( PyInt_AS_LONG( pTempObjRef1 ) <= 0 ){
		Py_XDECREF( pTempObjRef1 );
		return;
	}
	Py_XDECREF( pTempObjRef1 );

	pTempObjRef1 = PyObject_CallMethod( pySelfEntityPtr, "intonating", NULL );
	if( pTempObjRef1 == Py_True )
	{
		Py_XDECREF( pTempObjRef1 );
		return;
	}
	Py_XDECREF( pTempObjRef1 );

	pTempObjRef1 = PyObject_CallMethod( pySelfEntityPtr, "inHomingSpell", NULL );
	if( pTempObjRef1 == Py_True )
	{
		Py_XDECREF( pTempObjRef1 );
		return;
	}
	Py_XDECREF( pTempObjRef1 );

	pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr, "fightStartTime" );
	if( PyFloat_AsDouble( pTempObjRef1 ) == 0.0 ){
		pTempObjRef2 = PyFloat_FromDouble( time( NULL ) );
		PyObject_SetAttrString( pySelfEntityPtr, "fightStartTime", pTempObjRef2 );
		Py_XDECREF( pTempObjRef2 );
	}
	Py_XDECREF( pTempObjRef1 );

	//ƥ��AI���еȼ�
	pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr, "attrAINowLevelTemp" );
	PyObject *pAINowLevel = PyObject_GetAttrString( pySelfEntityPtr, "attrAINowLevel" );

	/*
	PyObject *tmp = PyInt_FromLong(10);
	DEBUG_MSG( "--->>> before, tmp is %d\n", PyInt_AS_LONG( tmp ) );
	PyObject_SetAttrString( pySelfEntityPtr, "attrAINowLevel", tmp );
	DEBUG_MSG( "--->>> after, tmp is %d\n", PyInt_AS_LONG( tmp ) );
	Py_XDECREF( tmp );
	DEBUG_MSG( "--->>> attrAINowLevel is %d\n", PyInt_AS_LONG( pAINowLevel ) );
	tmp = PyObject_GetAttrString( pySelfEntityPtr, "attrAINowLevel" );
	DEBUG_MSG( "--->>> after2, tmp is %d\n", PyInt_AS_LONG( tmp ) );
	Py_XDECREF( tmp );
	������β��Դ���֤ʵ��
	ʹ��PyObject_GetAttrString�����������ĳ������ֵ����Ҫ����
	PyObject_GetAttrString���»�ȡ������Ե�ֵ���������ȵ���PyObject_GetAttrString
	�����õ���ָ����ָ��Ķ��󽫲��������޸ĵ�ֵ��
	*/

	if(PyInt_AS_LONG( pTempObjRef1 ) != PyInt_AS_LONG( pAINowLevel )){
		PyObject_SetAttrString( pySelfEntityPtr, "attrAINowLevel", pTempObjRef1 );
		Py_XDECREF( pAINowLevel );
		pAINowLevel = PyObject_GetAttrString( pySelfEntityPtr, "attrAINowLevel" );
	}
	Py_XDECREF( pTempObjRef1 );

	//ִ��ͨ��AI��ѭ��
	pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr,"attrAttackStateGenericAIs" );
	pTempObjRef2 = PyDict_GetItem( pTempObjRef1, pAINowLevel );
	Py_XDECREF( pTempObjRef1 );
	if( pTempObjRef2 != NULL ){
		int16 commonAIListSize = PyList_Size( pTempObjRef2 );
		int16 index = 0;
		while( commonAIListSize > index ){
			if( entity().isDestroyed() && !entity().isReal() ){
				Py_XDECREF( pAINowLevel );
				return;
			}
			pTempObjRef1 = PyList_GetItem( pTempObjRef2, index );
			if( aiCommonCheck_AIInterface_cpp( pTempObjRef1 ) ){
				pTempObjRef3 = PyObject_CallMethod( pTempObjRef1, "do", "O", pySelfEntityPtr );
				Py_XDECREF( pTempObjRef3 );
			}
			index++;
		}
	}

	//������AI������ִ������AI
	pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr, "comboAIArray" );
	PyArrayProxy comboAIArray = PyArrayProxy(pTempObjRef1);
	if (comboAIArray.length() > 0)
	{
		pTempObjRef2 = PyObject_CallMethod( pySelfEntityPtr, "doComboAI", NULL );
		Py_XDECREF( pTempObjRef2 );
	}
	Py_XDECREF( pTempObjRef1 );

	//�������AI��ִ��״̬
	PyObject * pComboAIState = PyObject_GetAttrString( pySelfEntityPtr, "comboAIState" );
	if( pComboAIState == Py_False )
	{
		Py_XDECREF( pComboAIState );

		//ִ������AI��ѭ��
		bool tempAICheckResult = false;
		pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr,"insert_ai" );
		if(pTempObjRef1 != Py_None){
			if( aiCommonCheck_AIInterface_cpp( pTempObjRef1 ) ){
				pTempObjRef3 = PyObject_CallMethod( pTempObjRef1, "do", "O", pySelfEntityPtr );
				Py_XDECREF( pTempObjRef3 );
				tempAICheckResult = true;
			}
		}
		Py_XDECREF( pTempObjRef1 );

		if( tempAICheckResult == false ){
			pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr,"attrSchemeAIs" );
			pTempObjRef2 = PyDict_GetItem( pTempObjRef1, pAINowLevel );
			Py_XDECREF( pTempObjRef1 );
			if(pTempObjRef2 != NULL ){
				int16 schemeAIListSize = PyList_Size( pTempObjRef2 );
				for(int index = 0; index < schemeAIListSize; index++){
					pTempObjRef1 = PyList_GetItem( pTempObjRef2, index );
					if( aiCommonCheck_AIInterface_cpp( pTempObjRef1 ) ){
						pTempObjRef3 = PyObject_CallMethod( pTempObjRef1, "do", "O", pySelfEntityPtr );
						Py_XDECREF( pTempObjRef3 );
						break;
					}
				}
			}
		}

		if( entity().isDestroyed() ){
			Py_XDECREF( pAINowLevel );
			return;
		}

		//����AIִ�м��
		pTempObjRef1 = PyObject_CallMethod( pySelfEntityPtr, "comboAICheck", NULL );
		long comboID = PyInt_AS_LONG( pTempObjRef1 );
		Py_XDECREF( pTempObjRef1 );

		if( comboID > 0 )
		{
			pTempObjRef1 = PyObject_CallMethod( pySelfEntityPtr, "doComboAI", "i", comboID );
			Py_XDECREF( pTempObjRef1 );
		}

		//�ٴμ������AI��ִ��״̬
		PyObject * pInnerComboAIState = PyObject_GetAttrString( pySelfEntityPtr, "comboAIState" );
		if( pInnerComboAIState == Py_False )
		{
			Py_XDECREF( pInnerComboAIState );

			//ִ������AI��ѭ��
			tempAICheckResult = false;

			pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr, "saiArray" );
			PyArrayProxy * pSaiArray = new PyArrayProxy( pTempObjRef1 );

			if( pSaiArray->length() > 0 )
			{
				pTempObjRef2 = pSaiArray->pop( 0 );
				if( aiCommonCheck_AIInterface_cpp( pTempObjRef2 ) )
				{
					pTempObjRef3 = PyObject_CallMethod( pTempObjRef2, "do", "O", pySelfEntityPtr );
					Py_XDECREF( pTempObjRef3 );
					tempAICheckResult = true;
				}
				else
				{
					pSaiArray->clear();
				}
				Py_XDECREF( pTempObjRef2 );
			}

			delete pSaiArray;
			Py_XDECREF( pTempObjRef1 );

			if( tempAICheckResult == false )
			{
				bool doSuccess = false;

				pTempObjRef1 = PyObject_GetAttrString( pySelfEntityPtr,"attrSpecialAIs" );
				pTempObjRef2 = PyDict_GetItem( pTempObjRef1, pAINowLevel );
				Py_XDECREF( pTempObjRef1 );

				if(pTempObjRef2 != NULL)
				{
					int16 schemeAIListSize = PyList_Size( pTempObjRef2 );
					for(int index = 0; index < schemeAIListSize; index++)
					{
						pTempObjRef1 = PyList_GetItem( pTempObjRef2, index );
						if( aiCommonCheck_AIInterface_cpp( pTempObjRef1 ) )
						{
							pTempObjRef3 = PyObject_CallMethod( pTempObjRef1, "do", "O", pySelfEntityPtr );
							Py_XDECREF( pTempObjRef3 );
							doSuccess = true;
							break;
						}
					}
				}

				if( !doSuccess )
				{
					pTempObjRef1 = PyObject_CallMethod( pySelfEntityPtr, "onSpecialAINotDo", NULL );
					Py_XDECREF( pTempObjRef1 );
				}
			}
			Py_XDECREF( pAINowLevel );
			if( entity().isDestroyed() ){
				return;
			}
		}
		else
		{
			Py_XDECREF( pInnerComboAIState );
		}
	}
	else
	{
		Py_XDECREF( pComboAIState );
	}

	pTempObjRef1 = PyObject_CallMethod( pySelfEntityPtr, "setAITargetID", "i", 0 );
	Py_XDECREF( pTempObjRef1 );

	PyObject_SetAttrString( pySelfEntityPtr, "comboAIState", Py_False );
}


bool AIInterface::aiCommonCheck_AIInterface_cpp( PyObject *pAIObj )
{
	PyObject *pySelfEntityPtr = (PyObject *)(&entity_);
	PyObject *pTempObjRef1, *pTempObjRef2;

	// ��ai�Ƿ���һ��e��ai �ǵĻ�������
	pTempObjRef1 = PyObject_CallMethod( pAIObj, "getID", NULL );
	pTempObjRef2 = PyObject_CallMethod( pySelfEntityPtr, "isEAI", "i", PyInt_AsLong( pTempObjRef1 ) );
	Py_XDECREF( pTempObjRef1 );
	if( pTempObjRef2 == Py_True )
	{
		Py_XDECREF( pTempObjRef2 );
		return false;
	}
	Py_XDECREF( pTempObjRef2 );

	//ִ��ai�Ļ����
	pTempObjRef1 = PyObject_CallMethod( pAIObj, "getActiveRate", NULL );
	long activeRate = PyInt_AsLong( pTempObjRef1 );
	Py_XDECREF( pTempObjRef1 );
	if(activeRate < 100)
	{
		if(activeRate <= 0 || activeRate < (rand()%101))
			return false;
	}

	//��� ai��������
	pTempObjRef1 = PyObject_CallMethod( pAIObj, "check", "O", pySelfEntityPtr );
	if(pTempObjRef1 == Py_False)
	{
		Py_XDECREF( pTempObjRef1 );
		return false;
	}
	Py_XDECREF( pTempObjRef1 );

	return true;
}
