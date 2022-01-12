/**
 *	csolExtra��Ϊentity extra�����͹���ʵ��ת�Ӳ�����ڡ�
 *	csol��entity�ļ̳в�ζ��Ҹ��ӣ��������bw extra�����˼�룬
 *	ֻ�����ĳһ��entityĳһ���������д����޷����ģ���Ż�entity��
 *	�ű�������ʵ�ֵ�extra����Ҳ�޷����ƶ�̬Ӧ�ã�����ű�����һ
 *	���װ��������ʵ���˰�bw��ɢ�ͱ�ƽ����extraʵ�ַ�ʽת
 *	��Ϊ��Ӧ��csol�ű���ļ̳���ϵ���Ա����ĸ��ú�ά����
 *	Design by wangshufeng.
 */

#ifndef CSOL_CELL_EXTRA_CSOLEXTRA_HPP
#define CSOL_CELL_EXTRA_CSOLEXTRA_HPP

#include "gameobject.hpp"
#include "cellapp/entity_extra.hpp"
#include <string>

#undef PY_METHOD_ATTRIBUTE
#define PY_METHOD_ATTRIBUTE PY_METHOD_ATTRIBUTE_ENTITY_EXTRA


class CsolExtra : public EntityExtra
{
Py_EntityExtraHeader( CsolExtra );

public:
	CsolExtra( Entity &e );
	~CsolExtra();

	PyObject *pyGetAttribute( const char *attr );
	int pySetAttribute( const char *attr, PyObject *value );

	PY_AUTO_METHOD_DECLARE(RETDATA, distanceBB_cpp, ARG(Entity *, END));
	float distanceBB_cpp(Entity * pEntity);

	PY_AUTO_METHOD_DECLARE(RETDATA, queryTemp_cpp, ARG(PyObjectPtr, OPTARG(PyObjectPtr, PyObjectPtr(), END)));
	PyObject *queryTemp_cpp(PyObjectPtr, PyObjectPtr);

	PY_AUTO_METHOD_DECLARE( RETDATA, testPropertyIndex, ARG( char *, END) );
	std::string testPropertyIndex( const char * name );

	PY_AUTO_METHOD_DECLARE( RETOWN, getDownToGroundPos_cpp, END );
	PyObject *getDownToGroundPos_cpp();

	PY_AUTO_METHOD_DECLARE( RETOWN, moveToPointObstacle_cpp,
		ARG( Vector3, ARG( float, OPTARG( int, 0, OPTARG( float, 0.5,
		OPTARG( float, 0.5,	OPTARG( bool, true, OPTARG( bool, false, END ) ) ) ) ) ) ) );
	PyObject * moveToPointObstacle_cpp( Vector3 destination,
							float velocity,
							int userArg = 0,
							float crossHight = 0.5,
							float distance = 0.5,
							bool faceMovement = true,
							bool moveVertically = false );
	
	PY_AUTO_METHOD_DECLARE(RETDATA, isSamePlanesExt, ARG(Entity *, END));
	bool isSamePlanesExt( Entity * pEntity );
	
	PY_AUTO_METHOD_DECLARE( RETOWN, entitiesInRangeExt,ARG( float, OPTARG( PyObjectPtr, NULL, OPTARG( PyObjectPtr, NULL, END ) ) ) );
	PyObject* entitiesInRangeExt( float fRange, PyObjectPtr pEntityName=NULL, PyObjectPtr pPos = NULL);

	static const Instance<CsolExtra> instance;

	void initMapInstancePtr();
	inline GameObject *getMapInstancePtr() { return mapInstancePtr; }

	//���ģ�巽������������ڶ��壨����CPP�ļ����壩���ᵼ�±��������
	//so�ļ��޷�����
	template<class T> static T extraProxy( Entity * pEntity )
	{
		return dynamic_cast<T>( CsolExtra::instance( *pEntity ).getMapInstancePtr() );
	}

private:
	GameObject *mapInstancePtr;
};

#undef PY_METHOD_ATTRIBUTE
#define PY_METHOD_ATTRIBUTE PY_METHOD_ATTRIBUTE_BASE

#endif
