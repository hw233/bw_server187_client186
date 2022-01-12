#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include "cellapp/entity.hpp"
#include "pyscript/script_math.hpp"

PY_SCRIPT_CONVERTERS_DECLARE( Entity );

namespace{
	const float ROLE_AOI_RADIUS = 50.0;	 //entity����Ұ����

	typedef std::vector<Entity *> EntityList;

	class ExtEntityReceiver: public Entity::EntityReceiver
	{
	public:
		~ExtEntityReceiver() { entities_.clear(); }
		void addEntity( Entity * pEntity )
		{
			entities_.push_back( pEntity );
		}
		int size()
		{
			return entities_.size();
		}
		EntityList::iterator begin() { return entities_.begin(); }
		EntityList::iterator end() { return entities_.end(); }
		void clear() { entities_.clear(); }
		const EntityList & entities() const { return entities_; }
	private:
		EntityList entities_;
	};
}

class GameObject
{

public:
	GameObject(Entity& e);
	~GameObject();

public:
	//PY_AUTO_METHOD_DECLARE( RETOWN, getDownToGroundPos_GameObject_cpp, END );
	PyObject *getDownToGroundPos_GameObject_cpp();

    virtual float distanceBB_cpp(Entity * pEntity);
	virtual bool getDownToGroundPos(Vector3 &position);
	virtual Vector3 getBoundingBox();
    virtual float BoundingBoxHZ();
    virtual float BoundingBoxHX();
	virtual PyObject *queryTemp_cpp(PyObjectPtr, PyObjectPtr);
	virtual PyObject *getDownToGroundPos_cpp();

	virtual PyObject * moveToPointObstacle_cpp( Vector3 destination,
							float velocity,
							int userArg = 0,
							float crossHight = 0.5,
							float distance = 0.5,
							bool faceMovement = true,
							bool moveVertically = false );
public:
	void setTemp(const char *, int);							//��������key���ַ������ͣ�ֵ���������͵���ʱ����
	int queryTemp(const char *, int);							//���ڲ�ѯkeyʱ�ַ������ͣ�ֵ���������͵���ʱ����

    bool hasState() { return index_state_ == -1 ? false:true; }

    inline int64 flags()
    {
        if(index_GameObject_flags_ == -1)
            return 0;

        PyObjectPtr o = entity_.propertyByLocalIndex(index_GameObject_flags_);
        return PyLong_AsLongLong(o.getObject());
    }

    inline bool hasFlag( int64 flag )
    {
    	return (flags() & (1 << flag)) != 0;
    }

    inline int8 utype()
    {
        if(index_GameObject_utype_ == -1)
            return 0;

        PyObjectPtr o = entity_.propertyByLocalIndex(index_GameObject_utype_);
        int8 ret = PyInt_AsLong(o.getObject());
        return ret;
    }


	void transToGroundPosition( Vector3 &position );
	
	long getPlanesID();
	bool isSamePlanesExt( Entity * pEntity );
	PyObject* entitiesInRangeExt( float fRange, PyObjectPtr pEntityName, PyObjectPtr pPos );

    //��ȡdef�������������
    int getPropertyIndex(const std::string &name);
    int getPropertyLocalIndex( const char * name ) const;
    std::string testPropertyIndex( const char * name );

	//cell_public��all_clients����
	int index_state_;		//state������GameObject������ԣ����ǽű���Ϊ�˼�������Ĺ���Ҳд����GameObject�У�����ֻ��ԭ����ֲ
	int index_GameObject_flags_;
    int index_GameObject_utype_;
	int index_GameObject_planesID_;
	//cell_private���ԣ�ע��ghost�����õ���������
	int index_GameObject_tempMapping_;

protected:
    float boundingBoxHZ_;								//ʵ��ģ�Ͱ󶨺�z����
    float boundingBoxHX_;								//ʵ��ģ�Ͱ󶨺�x����

	Entity & entity()					{ return entity_; }
	const Entity & entity() const		{ return entity_; }

	Entity & entity_;


};


#endif // GAMEOBJECT_HPP
