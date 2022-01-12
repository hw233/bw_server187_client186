#include "gameobject.hpp"
#include "csolExtra.hpp"
#include "../csdefine.h"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "cellapp/move_controller.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

PY_SCRIPT_CONVERTERS( Entity )

GameObject::GameObject(Entity& e) : entity_(e)
{
    INFO_MSG("init GameObject extra (Entity: %d)\n", entity_.id());

	index_state_ = getPropertyLocalIndex("state");
    index_GameObject_utype_ = getPropertyLocalIndex("utype");
	index_GameObject_flags_ = getPropertyLocalIndex("flags");
	index_GameObject_tempMapping_ = getPropertyLocalIndex("tempMapping");
    boundingBoxHZ_ = getBoundingBox().z/2;
    boundingBoxHX_ = getBoundingBox().x/2;
	index_GameObject_planesID_ = getPropertyLocalIndex( "planesID" );
}

GameObject::~GameObject()
{
}

/**
 *  ��ȡʵ�嵽�������ײ��
 *
 *  @param  position  ���ص������ײ��
 *
 *  @return
 *
 */
bool GameObject::getDownToGroundPos(Vector3 &position)
{
    const Vector3 &pos = entity_.position();
    bool xOK = -10000 < pos.x && pos.x < 10000;
    bool yOK = -10000 < pos.y && pos.y < 10000;
    bool zOK = -10000 < pos.z && pos.z < 10000;
    if (! ( xOK && yOK && zOK ))
    {
        ERROR_MSG("Unexpected huge coordinate, Entity: %d, position = %f,%f,%f \n",
                entity_.id(), entity_.position().x, entity_.position().y,
                entity_.position().z);
        return false;
    }

	ChunkSpace *pSpace = entity_.pChunkSpace();

    Vector3 src = pos + Vector3(0.0f, 0.1f, 0.0f);
    Vector3 dst = pos + Vector3(0.0f, -10.f, 0.0f);

	//��ClosestObstacle��ȡ��һ����ײ��
    float dist = pSpace->collide(src, dst, ClosestObstacle::s_default);
    if (dist < 0.f)
    {
        position = pos;
        return true;
    }

    Vector3 dir = dst - src;
    dir.normalise();
    position = src + dir * dist;

    return  true;
}

/**
 * ����λ���¼ӵļ�������
 *  @param  entity
 *
 *  @return	tur/false
 *
 */
long GameObject::getPlanesID()
{
	PyObject *pPlanesID = entity_.propertyByLocalIndex( index_GameObject_planesID_ ).getObject();
	return PyInt_AsLong( pPlanesID );
}

bool GameObject::isSamePlanesExt(Entity * pEntity )
{
	GameObject *pGmaeObject = CsolExtra::extraProxy<GameObject *>( pEntity );
	if( this->getPlanesID() == pGmaeObject->getPlanesID() )
		return true;
	else
		return false;
}
	
PyObject* GameObject::entitiesInRangeExt( float fRange, PyObjectPtr pEntityName=NULL, PyObjectPtr pPos = NULL )
{
	ExtEntityReceiver entReceiver = ExtEntityReceiver();
	PyObject *pNewList = PyList_New(0);

	entity_.getEntitiesInRange( entReceiver, fRange, pEntityName, pPos );
	//needDisperse = entReceiver.entities().size() > 0;
	GameObject * pEntAround;
	for( EntityList::iterator it = entReceiver.begin(); it != entReceiver.end(); it++ )
	{
		pEntAround = CsolExtra::extraProxy<GameObject *>( *it );
		if( pEntAround != NULL && this->isSamePlanesExt( *it ))
		{
			PyList_Append( pNewList, *it );
		}
	}
	
	return pNewList;
}

// ת��positionΪ��ǰentity���ڿռ�ĵ����
void GameObject::transToGroundPosition( Vector3 &position )
{
	Vector3 srcPos = position + Vector3(0.0f, 5.f, 0.0f);
	Vector3 dstPos = position + Vector3(0.0f, -10.f, 0.0f);

	float collideDist = entity_.pChunkSpace()->collide(srcPos, dstPos, ClosestObstacle::s_default);
    if (collideDist < 0.f)
    {
        return;
    }
	position = Vector3( srcPos.x, srcPos.y - collideDist, srcPos.z );
}

/**
 * ��ȡboundingBox��ֻ�Ǽ򵥵ص��ýű��ķ���
 */
Vector3 GameObject::getBoundingBox()
{
	Vector3 v(0.0f, 0.0f, 0.0f);

    PyObject *pGetBoundingBoxStr = PyString_FromString("getBoundingBox");
    PyObject *boundingbox = PyObject_CallMethodObjArgs((PyObject *)&entity_, pGetBoundingBoxStr , NULL );
    if (PyVector<Vector3>::Check(boundingbox))
    {
        v = ((PyVector<Vector3>*)boundingbox)->getVector();
    }

    Py_XDECREF(pGetBoundingBoxStr);
    Py_XDECREF(boundingbox);
    return v;
}

/**
 * ��ȡboundingBox��z�����һ��
 */
float GameObject::BoundingBoxHZ()
{
    if(boundingBoxHZ_ == 0.0f)
        boundingBoxHZ_ = getBoundingBox().z/2;
    return boundingBoxHZ_;
}

/**
 * ��ȡboundingBox��x�����һ��
 */
float GameObject::BoundingBoxHX()
{
    if(boundingBoxHX_ == 0.0f)
        boundingBoxHX_ = getBoundingBox().x/2;
    return boundingBoxHX_;
}

/*
** ��ȡEntity.position��Ӧ�ĵر�㣬���Ÿ��ű��Ľӿ�
*/
PyObject *GameObject::getDownToGroundPos_cpp()
{

	Vector3 position;
	if( getDownToGroundPos( position ) ){
		return Script::getData( position );
	}
	else{
		Py_XINCREF( Py_None );
		return Py_None;
	}
}

/**
 *  �����Ľű��ӿ�
 *  ����ʵ��֮��ľ���
 *
 *  @param pEntity ʵ�����ָ��
 *
 *  @return ���ؾ���
 *
 */
float GameObject::distanceBB_cpp(Entity * pEntity)
{
    if( pEntity == NULL )								//����ű�����ʱ����һ��None����pEntity����NULL����Ϊ��Ч
    	return FLT_MAX;

	GameObject *gameObject = static_cast<GameObject *>( CsolExtra::instance( *pEntity ).getMapInstancePtr() );
    float s1 = this->BoundingBoxHZ();
    float d1 = gameObject->BoundingBoxHZ();

    const Vector3 &selfPos = entity_.position();
    const Vector3 &dstPos = pEntity->position();

    if(this->hasState() && gameObject->hasState())
    {
        int64 self_flags = this->flags();
        int64 dst_flags = gameObject->flags();

        if((self_flags & ( 1 << ROLE_FLAG_FLY)) ||
                (dst_flags & ( 1 << ROLE_FLAG_FLY)))
        {
            return (selfPos - dstPos).length() - s1 -d1;
        }
    }

    Vector3 selfPosGround, dstPosGround;
    bool selfst = this->getDownToGroundPos(selfPosGround);
    bool dstst = gameObject->getDownToGroundPos(dstPosGround);

    if(!selfst)
        selfPosGround = selfPos;
    if(!dstst)
        dstPosGround = dstPos;

    return (selfPosGround - dstPosGround).length() - s1 -d1;
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
 PyObject * GameObject::queryTemp_cpp( PyObjectPtr pCppKey, PyObjectPtr pCppDefValue )
{
	/*
	* tempMapping���Բ�����propertyByLocalIndex��ȡ��ʹ�ø÷�ʽ�õ���ָ�벢����ʵָ��tempMapping
	* ��ָ�루��PyObject_GetAttrString( (PyObject *)&entity_, "tempMapping" )�õ���ָ����ָ��ĵ�
	* ַ����ͬ�����Ը�ָ��ִ��PyDict_Contains�����ᵼ��cell������
	* ��ע���������ԭ���Ѳ���������Ϊͨ��propertyByLocalIndex������ȡ����Ӧ�ô������Ե�localIndex
	* ������index��
	*/
	PyObject * pKey = Script::getData( pCppKey );
	PyObject * pTempMapping = entity_.propertyByLocalIndex(index_GameObject_tempMapping_).getObject();
	if( NULL == pTempMapping ){
		PyErr_Format(PyExc_TypeError, "cellextra::GameObject::queryTemp_cpp:got a NULL tempMapping, entity(id:%lu) is a %s.\n", \
			entity_.id(), entity_.isReal() ? "real" : "ghost");
	}
	else{
		if(PyDict_Contains( pTempMapping, pKey ) == 1){
			PyObject *pValue = PyDict_GetItem( pTempMapping, pKey );
			//Py_XDECREF(pTempMapping);
			Py_XDECREF(pKey);
			return pValue;
		}
	}
	//Py_XDECREF(pTempMapping);
	Py_XDECREF(pKey);
	return pCppDefValue.getObject();
}

/**
 * ����key���ַ������ͣ�ֵ���������͵���ʱ���ԣ��ýӿڲ����Ÿ��ű�
 */
void GameObject::setTemp( const char * key, int val )
{
	PyObject * pTempMapping = entity_.propertyByLocalIndex(index_GameObject_tempMapping_).getObject();
	if( pTempMapping == NULL )
	{
		PyErr_Format(PyExc_TypeError, "cellextra::GameObject::setTemp:got a NULL tempMapping, entity(id:%lu) is a %s.\n", \
			entity_.id(), entity_.isReal() ? "real" : "ghost");
	}
	else
	{
		PyObject * pVal = PyInt_FromLong(val);
		PyDict_SetItemString(pTempMapping, key, pVal);
		Py_XDECREF(pVal);
	}
}

/**
 * ��ѯkey���ַ������ͣ�ֵ���������͵���ʱ���ԣ��ýӿڲ����Ÿ��ű�
 */
int GameObject::queryTemp( const char * key, int defVal )
{
	PyObject * pTempMapping = entity_.propertyByLocalIndex(index_GameObject_tempMapping_).getObject();
	if( pTempMapping != NULL )
	{
		PyObject * result = PyDict_GetItemString( pTempMapping, key );
		if( result != NULL )
			return PyInt_AsLong( result );
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "cellextra::GameObject::queryTemp:got a NULL tempMapping, entity(id:%lu) is a %s.\n", \
			entity_.id(), entity_.isReal() ? "real" : "ghost");
	}
	return defVal;
}

/**
 *  ��ȡdef�������������
 *
 *  @param name ��������
 *
 *  @return -1��ʾû���������
 */
int GameObject::getPropertyIndex(const std::string &name)
{
	EntityTypePtr type = entity_.pType();
	const EntityDescription &entdesc =  type->description();

    DataDescription *datadesc = entdesc.findProperty(name);
    if(datadesc == NULL)
    {
        ERROR_MSG("Entity %s not Property [%s]\n",entdesc.name().c_str(), name.c_str());
        return -1;
    }
    else
        return datadesc->index();
}

/**
 *  ��ȡdef���������������ע�⣺ֻ����def�ļ��ж�������Բ���localIndex��
 *  ��������û�У����ǿ���ʹ��PyObject_GetAttrString��ȡ
 *
 *  @param name ��������
 *
 *  @return -1��ʾû���������
 */
int GameObject::getPropertyLocalIndex(const char * name) const
{
    DataDescription *datadesc = entity_.pType()->description( name );
    if(datadesc == NULL)
    {
        ERROR_MSG("Entity '%s' has no property [%s]\n",entity_.pType()->name(), name );
        return -1;
    }
    else
        return datadesc->localIndex();
}

/**
 *  ֱ���ƶ���Ŀ��㣬����ƶ�·�������ϰ�����ͣ��
 *
 *	@param destination		The location to which we should move
 *	@param velocity			Velocity in metres per second
 *	@param crossHight		Ignoring the barrier height
 *  @param distance			Stop once we collide with obstacle within this range
 *	@param faceMovement		Whether or not the entity should face in the
 *							direction of movement.
 *	@param moveVertically	Whether or not the entity should move vertically.
 *
 */
PyObject *GameObject::moveToPointObstacle_cpp( Vector3 destination,
							float velocity,
							int userArg,
							float crossHight,	// ��Խ�ϰ��ĸ߶�
							float distance,		// ��ײ�����ľ���,���������entityǶ����ײ���ڲ�
							bool faceMovement,
							bool moveVertically )
{
	if( !entity_.isReal() )
	{
		PyErr_SetString( PyExc_TypeError,
				"Entity.moveToPointObstacle_cpp can only be called on a real entity" );
		return Script::getData( -1 );
	}

	// ���ǿ�Խ�ϰ��ĸ߶ȣ�Ĭ��Ϊ��ҵ����¸߶�0.5��
	Vector3 adrustSrcPos = entity_.position() + Vector3(0.0f, crossHight, 0.0f);
	Vector3 adrustDstPos = destination + Vector3(0.0f, crossHight, 0.0f);

	float dist = entity_.pChunkSpace()->collide(adrustSrcPos, adrustDstPos, ClosestObstacle::s_default );
	if( dist > 0.f )
	{
		Vector3 dir = adrustDstPos - adrustSrcPos;
		dir.normalise();
		adrustDstPos = adrustSrcPos + dir * ( dist - distance );
	}
	transToGroundPosition( adrustDstPos );
	return Script::getData( entity_.addController( new MoveToPointController( adrustDstPos, "", 0, velocity, faceMovement, moveVertically ), userArg ) );
}

std::string GameObject::testPropertyIndex( const char * name )
{
	std::ostringstream report;
	int index = getPropertyIndex(name);
	int localIndex = getPropertyLocalIndex(name);
	PyObject * pProperty = PyObject_GetAttrString( (PyObject *)&entity_, name );
	DEBUG_MSG( "Test for property %s:\n", name );
	report << "Test for property " << name << " :\n";
	DEBUG_MSG( "	index is %d, value is %d\n", index, entity_.propertyByLocalIndex(index).getObject() );
	report << "	index is " << index << ", value is " << entity_.propertyByLocalIndex(index).getObject() << "\n";
	DEBUG_MSG( "	localIndex is %d, value is %d\n", localIndex, entity_.propertyByLocalIndex(localIndex).getObject() );
	report << "	localIndex is " << localIndex << ", value is " << entity_.propertyByLocalIndex(localIndex).getObject() << "\n";
	DEBUG_MSG( "	value by PyObject_GetAttrString is %d\n", pProperty );
	report << "	value by PyObject_GetAttrString is " << pProperty << "\n";
	DEBUG_MSG( "Test for property %s end.\n", name );
	report << "Test for property " << name << " end.\n";
	Py_XDECREF( pProperty );
	return report.str();
}




