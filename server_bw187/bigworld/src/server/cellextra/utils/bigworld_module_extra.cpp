/*
** ��ģ��������չBigWorldģ��Ĺ��ܣ���дһЩ�����Դ������ܸ��ƵĽű�����
*/
#include <stdlib.h>
#include "cellapp/entity.hpp"
#include "cellapp/entity_navigate.hpp"
#include "cellapp/cellapp.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "pyscript/script_math.hpp"
#include "../csdefine.h"
#include "../entity_extras/csolExtra.hpp"
#include "../entity_extras/monster.hpp"

//���¶�����cellextra/gameobject.hpp���Ѿ���������gameobject.cpp
//��Ҳ�ж��壬���ﲻ�ܽ����ظ������Ͷ���
//PY_SCRIPT_CONVERTERS_DECLARE( Entity )
//PY_SCRIPT_CONVERTERS( Entity )

/**
 * ����Entity����б���������ָ����Χ��Entityʱ���Entity
 * ʹ��δ�������ֿռ�����޶���ʹ��Щ����ֻ�ڱ��ĵ��пɼ�
*/
namespace{
	const float MIN_ATTACK_RADIUS = 1.2;		//��С�����뾶
	const float DEFAULT_MAX_MOVE = 4.0;			//Ĭ�����ɢ���ƶ�����

	typedef std::vector<Entity *> EntityList;

	class ExtraEntityReceiver: public Entity::EntityReceiver
	{
	public:
		~ExtraEntityReceiver() { entities_.clear(); }
		void addEntity( Entity * pEntity )
		{
			entities_.push_back( pEntity );
		}
		EntityList::iterator begin() { return entities_.begin(); }
		EntityList::iterator end() { return entities_.end(); }
		void clear() { entities_.clear(); }
		const EntityList & entities() const { return entities_; }
	private:
		EntityList entities_;
	};

	bool getMonsterExtra( Monster ** pMon, Entity * pEnt )
	{
		//GameObject * gobj = CsolExtra::instance(*pEnt).getMapInstancePtr();
		//*pMon = dynamic_cast<Monster *>(gobj);
		*pMon = CsolExtra::extraProxy<Monster *>( pEnt );
		return *pMon != NULL;
	}
}

/**************************************************************************/
/**
* xzƽ�������ĵ���룬����entity���ĵ�֮��ľ���
*
* Դ�ԣ�cell/utils.py ent_flatDistance
*/
inline float ent_flatDistance( Entity * pSrc, Entity * pDst )
{
	float x = pSrc->position().x - pDst->position().x;
	float z = pSrc->position().z - pDst->position().z;
	return sqrt( x*x + z*z );
}

/**
* entity��boundingBox z ���򳤶ȵ�һ��
*
* Դ�ԣ�cell/utils.py ent_length
*/
inline float ent_hLength( Entity * pEntity )
{
	Monster * pEntExtra = NULL;
	if( !getMonsterExtra(&pEntExtra, pEntity) )
		return 0;
	return pEntExtra->BoundingBoxHZ();
}

/**
* entity��boundingBox x ���򳤶ȵ�һ��
*
* Դ�ԣ�cell/utils.py ent_width
*/
inline float ent_hWidth( Entity * pEntity )
{
	Monster * pEntExtra = NULL;
	if( !getMonsterExtra(&pEntExtra, pEntity) )
		return 0;
	return pEntExtra->BoundingBoxHX();
}

/**
* z�᷽������entity����������վ�ľ���
*
* Դ�ԣ�cell/utils.py ent_z_distance_min
*/
inline float ent_z_distance_min( Entity * pSrc, Entity * pDst )
{
	return ent_hLength(pSrc) + ent_hLength(pDst);
}

/**
* z�᷽��ľ��룬����entity����淽��ľ��룬������boundingBox
*
* Դ�ԣ�cell/utils.py ent_z_distance
*/
inline float ent_z_distance( Entity * pSrc, Entity * pDst )
{
	return ent_flatDistance(pSrc, pDst) - ent_z_distance_min(pSrc, pDst);
}

/**
* x�᷽������entityƽ�Ž�����վ�ľ���
*
* Դ�ԣ�cell/utils.py ent_x_distance_min
*/
inline float ent_x_distance_min( Entity * pSrc, Entity * pDst )
{
	return ent_hWidth(pSrc) + ent_hWidth(pDst);
}

/**
* x�᷽��ľ��룬����entityƽ��վ�ľ��룬������boundingBox
*
* Դ�ԣ�cell/utils.py ent_x_distance
*/
inline float ent_x_distance( Entity * pSrc, Entity * pDst )
{
	return ent_flatDistance(pSrc, pDst) - ent_x_distance_min(pSrc, pDst);
}


/**************************************************************************/
/**
* ����Ƿ������¶�λ�Լ���λ��
* pEnt		: ��Ҫ���ж�λ��entity
*
* Դ�ԣ�cell/utils.py checkForLocation
*/
bool checkForLocation_cpp( Entity * pEnt )
{
	Monster * pEntExtra = NULL;
	if( !getMonsterExtra(&pEntExtra, pEnt) )
		return false;

	if( pEntExtra->actionSign(ACTION_FORBID_MOVE) )
	{
		DEBUG_MSG( "I cannot move.\n" );
		return false;
	}

	int counter = pEntExtra->queryTemp( "disperse_counter", 0 );
	pEntExtra->setTemp( "disperse_counter", counter + 1 );
	// ���þɹ���Ϊ�㲻�����ض�λ��ÿ4��������һ�η�ɢ���
	return counter > 0 && counter % 4 == 0;
}

/**
* ��xzƽ���center��ΪԲ�ģ��뾶��radius��Բ�ϣ����ȡ���ȷ�Χ��radian��
	������yaw�ĵ�
* center	: Position3D, Բ��λ��
* radius	: float���뾶
* radian	: float��0 - 2*pi����λ�ǻ��ȣ���ʾȡ����Բ����
* yaw		: float��0 - 2*pi����λ�ǻ��ȣ���ʾƫת���ٻ��ȣ���ֵ��ʱ�룬��ֵ˳ʱ��
*
* Դ�ԣ�cell/utils.py randomPosAround
*/
Position3D randomPosAround( const Position3D & center, float radius, float radian, float yaw )
{
	float minRadian, randomRadian, x, z;

	minRadian = 2*M_PI - radian*0.5 + yaw;
	//maxRadian = 2*M_PI + radian*0.5 + yaw;
	//randomRadian = minRadian + (maxRadian - minRadian) * unitRand();	// ������ʽ��Ϊ����������ʽ
	randomRadian = minRadian + radian * unitRand();

	x = center.x + radius * cos(randomRadian);
	z = center.z + radius * sin(randomRadian);
	return Position3D(x, center.y, z);
}

/**
* ��ĳ�����ĵ���Χָ����Χ��ɢ����maxMove������������ÿ���ƶ���
	�����룬���ʼ�����������ڷ�ֹ�����ƶ�ʱ����Ŀ�꣬�ƶ���Ŀ��
	���󣬶�ʵ�ʲ��Ա�����maxMove���Ƶ�̫Сʱ�����Ǻ����ҵ�����ȫ
	�������ĵ㣬���Ǽ�ʹ����ȡ�ĵ㲻����maxMove������ֻҪ����
	canNavigateTo������Ȼ���˵���ΪĿ��㡣
* pEnt		: ��Ҫ����ɢ����entity
* centerPos	: ɢ�������ĵ㣬�ڸ����ĵ�ָ���İ뾶��ɢ��
* radius	: ɢ���İ뾶
* maxMove	: ����ƶ�����
*
* Դ�ԣ�cell/utils.py disperse
*/
bool disperse_cpp( Entity * pEnt, const Position3D & centerPos, float radius, float maxMove )
{
	Position3D dstPos, collideSrcPos, collideDstPos;
	PyObject *pyDstPos;
	float collideDist;
	float yaw = -(centerPos - pEnt->position()).yaw() - M_PI/2.0;
	float dispersingRadian = asin(std::min(maxMove, 2*radius)/radius*0.5)*4;
	bool dstFound = false;
	int tryCounter = 5;			//���Դ���

	while( tryCounter-- > 0 )
	{
		dstPos = randomPosAround( centerPos, radius, dispersingRadian, yaw );

		//����ȡ������,��ײ���
		collideSrcPos.set( dstPos.x, dstPos.y + 10.0f, dstPos.z );
		collideDstPos.set( dstPos.x, dstPos.y - 10.0f, dstPos.z );
		collideDist = pEnt->pChunkSpace()->collide( collideSrcPos, collideDstPos, ClosestObstacle::s_default );
		if( collideDist < 0.f )
			continue;

		//����֪��������ײ������Ŀ��λ���Ͻ�������ƫ��ȡ�ĵ㣬���Լ�Ŀ������
		dstPos.set( collideSrcPos.x, collideSrcPos.y - collideDist, collideSrcPos.z );

		//Ѱ·���
		pyDstPos = EntityNavigate::instance( *pEnt ).canNavigateTo( dstPos );
		if( pyDstPos == Py_None )
		{
			dstFound = false;
			Py_XDECREF( pyDstPos );
			continue;
		}
		else
		{
			dstFound = true;
			Script::setData( pyDstPos, dstPos, "EntityNavigate::instance().canNavigateTo" );
			Py_XDECREF( pyDstPos );
			break;
		}
	}

	if( !dstFound )
	{
		DEBUG_MSG( "BigWorld module extra::disperse_cpp, destination not found.\n" );
		return false;
	}
	else
	{
		PyObject *goPosResult = PyObject_CallMethod( ( PyObject * )pEnt, "gotoPosition", "O", Script::getData( dstPos ) );
		Py_XDECREF( goPosResult );
		return true;
	}
}

/**
* ��λentity��λ��
* pEnt		: ��Ҫ���ж�λ��entity
* maxMove	: ����ƶ�����
*
* Դ�ԣ�cell/utils.py locate
*/
bool locate_cpp( Entity * pEnt, float maxMove )
{
	Monster * pEntExtra = NULL;
	if( !getMonsterExtra(&pEntExtra, pEnt) )
		return false;

	// �Ƿ���ڲ�ɢ�����
	if( pEntExtra->hasFlag(ENTITY_FLAG_NOT_CHECK_AND_MOVE) )
		return false;

	// �Ƿ������ƶ��У���������Ϊɢ��ʱ��entity�ٶȽ�С�������ƶ��ñȽ���
	if( pEntExtra->isMoving() )
		return true;

	//�Ƿ��ܽ���ɢ��
	if( !checkForLocation_cpp(pEnt) )
		return false;

	//Ŀ����
	Entity * pTarget = CellApp::pInstance()->findEntity(pEntExtra->targetID());
	if( pTarget == NULL )
		return false;

	//����Ŀ���boundingBox�Ƿ��ص�
	bool needDisperse = ent_z_distance(pEnt, pTarget) < 0;
	//���boundingBox���ص����򿴿�����Ƿ�������entity
	if( !needDisperse )
	{
		static ExtraEntityReceiver entReceiver = ExtraEntityReceiver();
		entReceiver.clear();
		pEnt->getEntitiesInRange( entReceiver, pEntExtra->BoundingBoxHX() + 5 );
		//needDisperse = entReceiver.entities().size() > 0;
		GameObject * pEntAround;
		for( EntityList::iterator it = entReceiver.begin(); it != entReceiver.end(); it++ )
		{
			pEntAround = CsolExtra::extraProxy<GameObject *>( *it );
			if( pEntAround != NULL && pEntAround->utype() != ENTITY_TYPE_SPAWN_POINT && \
				ent_x_distance( pEnt, *it ) < 0 )
			{
				needDisperse = true;
				break;
			}
		}
	}

	if( needDisperse )
	{
		float radius = std::max(MIN_ATTACK_RADIUS, std::max(ent_z_distance_min(pEnt, pTarget), ent_flatDistance(pEnt, pTarget)));
		return disperse_cpp(pEnt, pTarget->position(), radius, maxMove );
	}
	else
	{
		return false;
	}
}

/**
* �������һ����Χ�ڵ��Ƿ���û�д����ƶ�״̬��entity����������ƶ���һ��λ�ã�
* �˹������ڽ��һ�ѹ���׷��ͬһ��Ŀ��ʱ�ص���һ������⡣
* ��ز�����������"firstAttackAfterChase"��
*
* Դ�ԣ�cell/utils.py checkAndMove
*/
bool checkAndMove_csol( Entity * pEnt )
{
	Monster * pEntExtra = NULL;
	if( !getMonsterExtra(&pEntExtra, pEnt) )
		return false;

	//����ƶ�����ȡboudingBox���һ�����Ĭ������ƶ�����
	return locate_cpp( pEnt, ent_hWidth(pEnt) + DEFAULT_MAX_MOVE );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, checkAndMove_csol, ARG( Entity *, END ), BigWorld )

/**
* �� checkAndMove һ���Ĺ��ܣ��ɴ�������ƶ��������
*
* Դ�ԣ�cell/utils.py checkAndMoveByDis
*/
bool checkAndMoveByDis_csol( Entity * pEnt, float maxMove )
{
	return locate_cpp( pEnt, maxMove );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, checkAndMoveByDis_csol, ARG(Entity *, ARG(float, END )), BigWorld )

/**
* ��һ��entity��Ŀ��entity��Χָ���뾶Բ����ɢ��
*
* Դ�ԣ�cell/utils.py moveOut
*/
bool moveOut_csol( Entity * pSrcEntity, Entity * pDstEntity, float distance, float moveMax )
{
	Monster * pSrcEntExtra = NULL;
	if( !getMonsterExtra(&pSrcEntExtra, pSrcEntity) )
		return false;

	// �����ǰ�����ƶ��У���ִ��ɢ��
	if( pSrcEntExtra->isMoving() )
		return true;

	//ɢ���뾶��Ҫ����boudingBox�ľ���
	float radius = distance + ent_z_distance_min(pSrcEntity, pDstEntity);
	return disperse_cpp( pSrcEntity, pDstEntity->position(), radius, ent_hWidth(pSrcEntity) + moveMax );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, moveOut_csol, ARG(Entity *, ARG(Entity *, ARG(float, ARG(float, END )))), BigWorld )
