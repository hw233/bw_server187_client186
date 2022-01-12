/*
** ÿһ��tick���role��pet�ľ��룬������뿪ָ������ʱ�ص��ű�
*/

#ifndef CSOL_CELL_CONTROLLER_PET_DETECTER
#define CSOL_CELL_CONTROLLER_PET_DETECTER



#include "cellapp/controller.hpp"
#include "cellapp/updatable.hpp"

typedef SmartPointer<Entity> EntityPtr;


class PetDistanceController : public Controller, public Updatable
{
	DECLARE_CONTROLLER_TYPE( PetDistanceController )
public:
	PetDistanceController( int32 petID = 0, int32 distance = 0 );

	virtual void	startReal( bool isInitialStart );
	virtual void	stopReal( bool isFinalStop );

	void	writeRealToStream( BinaryOStream& stream );
	bool	readRealFromStream( BinaryIStream& stream );
	void	update();

private:
	int32 petID_;
	int32 range_;		// �趨��֪ͨ����
	int32 oldDistance_;	// ��һ��tick˫���ľ���
	
};


#endif

