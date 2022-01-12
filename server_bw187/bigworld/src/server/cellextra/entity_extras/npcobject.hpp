#ifndef NPCOBJECT_HPP
#define NPCOBJECT_HPP

#include "gameobject.hpp"

class NPCObject : public GameObject
{
public:
	NPCObject( Entity &e );
	float BoundingBoxHZ();								//��д��ȡboudingbox�ķ���������ģ�����ű������м���
	float BoundingBoxHX();								//��д��ȡboudingbox�ķ���������ģ�����ű������м���
public:
	//get current modelScale
	inline float currModelScale()
	{
		if(index_modelScale_ == -1){
			index_modelScale_ = getPropertyLocalIndex("modelScale");
			ERROR_MSG( "NPCObject::currModelScale:index_modelScale_ init failed.redo." );
		}
		return PyFloat_AsDouble( entity_.propertyByLocalIndex(index_modelScale_).getObject() );
	}
	//get current modelNumber
	inline std::string currModelNumber()
	{
		if(index_modelNumber_ == -1){
			index_modelNumber_ = getPropertyLocalIndex("modelNumber");
			ERROR_MSG( "NPCObject::currModelScale:index_modelNumber_ init failed.redo." );
		}
		return PyString_AsString( entity_.propertyByLocalIndex(index_modelNumber_).getObject() );
	}
private:
	//check if modelScale is changed
	inline bool modelScaleChanged()
	{
		float currModelScale_ = currModelScale();
		if( (currModelScale_-modelScale_) > 0.001 || (currModelScale_-modelScale_) < -0.001 )
		{
			modelScale_ = currModelScale_;
			return true;
		}
		else
			return false;
	}
	//check if modelNumber is changed
	inline bool modelNumberChanged()
	{
		std::string currModelNumber_ = currModelNumber();
		if( currModelNumber_ != modelNumber_ )
		{
			modelNumber_ = currModelNumber_;
			return true;
		}
		else
			return false;
	}
private:
	float modelScale_;									//ģ�����ű���
	std::string modelNumber_;							//ģ�ͱ��
	//cell_public��all_clients����
	int index_modelScale_;								//ģ�����ű������Ե�����
	int index_modelNumber_;								//ģ�ͱ�����Ե�����
	//cell_private���ԣ�ע��ghost�����õ���������
};

#endif			//NPCOBJECT_HPP
