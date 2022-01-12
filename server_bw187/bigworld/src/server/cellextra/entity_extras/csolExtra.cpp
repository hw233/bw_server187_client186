#include "csolExtra.hpp"
#include "gameobject.hpp"
#include "monster.hpp"
#include "npcobject.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

PY_TYPEOBJECT( CsolExtra )

PY_BEGIN_METHODS( CsolExtra )
	PY_METHOD( distanceBB_cpp )
	PY_METHOD( queryTemp_cpp )
	PY_METHOD( testPropertyIndex )
	PY_METHOD( getDownToGroundPos_cpp )
	PY_METHOD( moveToPointObstacle_cpp )
	PY_METHOD( isSamePlanesExt )
	PY_METHOD( entitiesInRangeExt )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CsolExtra )
PY_END_ATTRIBUTES()

const CsolExtra::Instance<CsolExtra> CsolExtra::instance( &CsolExtra::s_attributes_.di_ );


CsolExtra::CsolExtra( Entity & e ):EntityExtra( e )
{
	initMapInstancePtr();
}

CsolExtra::~CsolExtra()
{
	if( NULL != mapInstancePtr ){
		delete mapInstancePtr;
	}
}

PyObject * CsolExtra::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return this->EntityExtra::pyGetAttribute( attr );
}

int CsolExtra::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return this->EntityExtra::pySetAttribute( attr, value );
}

void CsolExtra::initMapInstancePtr()
{
	PyObject *py_selfEntityPtr = (PyObject *)&entity_;
	PyObject *py_extraStr = PyObject_CallMethod( py_selfEntityPtr, "getExtraClsName", NULL );
	std::string extraStr = PyString_AsString( py_extraStr );
	Py_XDECREF( py_extraStr );

	DEBUG_MSG( "CsolExtra::initMapInstancePtr..rawEntityName:%s map enitytName:%s.\n", entity_.pType()->name(), extraStr.c_str() );
	if( extraStr.compare( "Monster" ) == 0 ){
		mapInstancePtr = new Monster( entity_ );
	}
	else if( extraStr.compare( "GameObject" ) == 0 ){
		mapInstancePtr = new GameObject( entity_ );
	}
	else if( extraStr.compare( "NPCObject" ) == 0 ){
		mapInstancePtr = new NPCObject( entity_ );
	}
	else{
		ERROR_MSG( "CsolExtra::initMapInstancePtr: %s cant find map extra,use MonsterExtra.\n", entity_.pType()->name() );
		mapInstancePtr = new Monster( entity_ );
	}
}

float CsolExtra::distanceBB_cpp(Entity * pEntity)
{
	return mapInstancePtr->distanceBB_cpp(pEntity);
}

/**
 *  ��ѯ��ʱ��¼
 *
 *  @param pKey �ֵ�����ָ��
 *  @param pDefValue Ĭ��ֵָ��
 *
 *  @return ������ڣ������ֵ�������Ӧ��ֵ�����򷵻�Ĭ��ֵ
 *
 */
PyObject * CsolExtra::queryTemp_cpp( PyObjectPtr pKey, PyObjectPtr pDefValue )
{
	return mapInstancePtr->queryTemp_cpp( pKey, pDefValue );
}

std::string CsolExtra::testPropertyIndex( const char * name )
{
	return mapInstancePtr->testPropertyIndex( name );
}

PyObject *CsolExtra::getDownToGroundPos_cpp()
{
	return mapInstancePtr->getDownToGroundPos_cpp();
}

PyObject *CsolExtra::moveToPointObstacle_cpp( Vector3 destination, 
							float velocity, 
							int userArg,
							float crossHight,	// ��Խ�ϰ��ĸ߶�
							float distance,		// ��Ŀ���ľ���,���������entityǶ����ײ���ڲ�
							bool faceMovement,
							bool moveVertically )
{
	return mapInstancePtr->moveToPointObstacle_cpp( destination, 
													velocity,
													userArg,
													crossHight,
													distance,
													faceMovement,
													moveVertically );
		
}


bool CsolExtra::isSamePlanesExt( Entity *pEntity )
{
	return mapInstancePtr->isSamePlanesExt( pEntity );
}

PyObject* CsolExtra::entitiesInRangeExt( float fRange, PyObjectPtr pEntityName, PyObjectPtr pPos)
{
	return mapInstancePtr->entitiesInRangeExt( fRange, pEntityName, pPos );
}
