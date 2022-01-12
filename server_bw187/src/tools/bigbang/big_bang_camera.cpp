/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"
#include "big_bang_camera.hpp"
#include "big_bang.hpp"
#include "snaps.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "common/mouse_look_camera.hpp"
#include "common/orthographic_camera.hpp"
#include "resmgr/datasection.hpp"
#include "gizmo/undoredo.hpp"

SmartPointer<BigBangCamera> BigBangCamera::s_instance = NULL;

BigBangCamera::BigBangCamera()
{
	MF_ASSERT(!s_instance);
	s_instance = this;

	{
		MouseLookCamera * mlc = new MouseLookCamera;
		mlc->windowHandle( BigBang::instance().hwndGraphics() );
		
		// Inform camera of speed we want it to go
		std::string speedName = Options::getOptionString( "camera/speed" );
		mlc->speed( Options::getOptionFloat( "camera/speed/" + speedName ) );
		mlc->turboSpeed( Options::getOptionFloat( "camera/speed/" + speedName + "/turbo" ) );

		// Calculate the max bounds for the camera
		BoundingBox bounds = ChunkManager::instance().cameraSpace()->gridBounds();

		// bring the bounds in by 0.5m, otherwise being at the edge but looking
		// at the wrong angle will cause everything to disappear
		Vector3 minBound = bounds.minBounds() + Vector3( .5f, 0.f, .5f );
		Vector3 maxBound = bounds.maxBounds() - Vector3( .5f, 0.f, .5f );

		mlc->limit( BoundingBox( minBound, maxBound ) );

		cameras_.push_back(mlc);
	}


	{
		OrthographicCamera * mlc = new OrthographicCamera;
		mlc->windowHandle( BigBang::instance().hwndGraphics() );
		
		// Inform camera of speed we want it to go
		std::string speedName = Options::getOptionString( "camera/speed" );
		mlc->speed( Options::getOptionFloat( "camera/speed/" + speedName ) );
		mlc->turboSpeed( Options::getOptionFloat( "camera/speed/" + speedName + "/turbo" ) );

		// Calculate the max bounds for the camera
		BoundingBox bounds = ChunkManager::instance().cameraSpace()->gridBounds();

		// bring the bounds in by 0.5m, otherwise being at the edge but looking
		// at the wrong angle will cause everything to disappear
		Vector3 minBound = bounds.minBounds() + Vector3( .5f, 0.f, .5f );
		Vector3 maxBound = bounds.maxBounds() - Vector3( .5f, 0.f, .5f );

		mlc->limit( BoundingBox( minBound, maxBound ) );

		cameras_.push_back(mlc);
	}

	currentCameraType_ = CT_MouseLook;
}

BigBangCamera::~BigBangCamera()
{
	MF_ASSERT(s_instance);
	s_instance = NULL;
}

void BigBangCamera::changeToCamera( CameraType type )
{
	if (currentCameraType_ == type)
		return;

	Matrix m = cameras_[currentCameraType_]->view();
	m.invert();
	Vector3 currentPosition(m.applyToOrigin());
	
	currentCameraType_ = type;

	if (type == CT_Orthographic)
	{
		BigBang::instance().setPlayerPreviewMode( false );
		Vector3 currentGroundPosition(Snap::toGround(currentPosition));
		currentPosition.y = currentGroundPosition.y + 50.f;
		Matrix newM;
		newM.lookAt( currentPosition,
					Vector3(0.f, -1.f, 0.f),
					Vector3(0.f, 0.f, 1.f));
		//newM.invert();
		cameras_[CT_Orthographic]->view() = newM;
	}
	else if (type == CT_MouseLook)
	{
		Matrix newM = cameras_[CT_MouseLook]->view();
		newM.invert();
		newM[3].x = currentPosition.x;
		newM[3].z = currentPosition.z;
		newM.invert();
		cameras_[CT_MouseLook]->view() = newM;
	}
}


void BigBangCamera::zoomExtents( ChunkItemPtr item, float sizeFactor )
{
	// calculate the world-space center of the item based on
	// the center of the local min/max bounding box metrics
	BoundingBox bb;
	item->edBounds( bb );
	const Vector3 max = bb.maxBounds( );
	const Vector3 min = bb.minBounds( );
	const Vector3 diff = max - min;
	const Vector3 maxMinusMinOnTwo = diff / 2.f;

	if ( almostZero( maxMinusMinOnTwo.length(), 0.0001f ) )
	{
		//abort before we do something silly like ...
		//setting the camera matrix to zero!
		return;
	}

	ChunkPtr chunk = item->chunk();
	MF_ASSERT( chunk );
	const Vector3 chunkPosition = chunk->transform().applyToOrigin();
	const Vector3 worldPosition = chunkPosition + item->edTransform().applyToOrigin();
	const Vector3 bbCenter = min + maxMinusMinOnTwo;
	const Vector3 itemCenter = worldPosition + bbCenter;

	// use the bounding box plan area as the camera's relative position
	Vector3 relativePosition = diff;
	relativePosition.y = 0.f;
	if ( almostZero( relativePosition.length(), 0.0001f ))
		relativePosition.z = 1.f;
	relativePosition.normalise( );

	//Calculate the radius of the bounding box, as seen by the camera
	bb.transformBy( Moo::rc().invView() );
	Vector3 maxBounds( bb.maxBounds( ) );
	maxBounds -= bb.minBounds( );
	float radX = ( bb.maxBounds( )[0] - bb.minBounds( )[0] );
	float radY = ( bb.maxBounds( )[1] - bb.minBounds( )[1] );
	float radius = ( radX > radY) ? ( radX / 2.f ) : ( radY / 2.f );

	// calculate the distance the camera must be from the object
	// and set the new camera position
	const float FOV = Moo::rc().camera().fov();
	const float dist = float(radius / tan( FOV/2.f ));
	// should be doing something intelligent here
	float actualSizeFactor = std::max( sizeFactor - radius / 8.f, 1.f);
	Vector3 newPosition = relativePosition * dist * actualSizeFactor;
	newPosition += itemCenter;
	currentCamera().view().setTranslate( -newPosition.x, -newPosition.y, -newPosition.z );

	// lookAt the object
	Vector3 lookDirection( -relativePosition );
	currentCamera().view().lookAt( getCameraLocation(), lookDirection, Vector3( 0, 1, 0 ) );

	// required so the camera will recalculate it's yaw pitch roll, which is
	// how it really stores the directory
	currentCamera().view( currentCamera().view() );
}


Vector3 BigBangCamera::getCameraLocation()
{
    Matrix invView( currentCamera().view() );
	invView.invert();
    return invView.applyToOrigin();
}

void BigBangCamera::respace( const Matrix& view )
{
	// Calculate the max bounds for the camera
	BoundingBox bounds = ChunkManager::instance().cameraSpace()->gridBounds();

	// bring the bounds in by 0.5m, otherwise being at the edge but looking	
	// at the wrong angle will cause everything to disappear
	Vector3 minBound = bounds.minBounds() + Vector3( .5f, 0.f, .5f );
	Vector3 maxBound = bounds.maxBounds() - Vector3( .5f, 0.f, .5f );

	( (MouseLookCamera*)cameras_[ CT_MouseLook ].getObject() )
		->limit( BoundingBox( minBound, maxBound ) );
	( (OrthographicCamera*)cameras_[ CT_Orthographic ].getObject() )
		->limit( BoundingBox( minBound, maxBound ) );
	( (MouseLookCamera*)cameras_[ CT_MouseLook ].getObject() )->view( view );
	( (OrthographicCamera*)cameras_[ CT_Orthographic ].getObject() )->view( view );
}

class BigBangCameraUndoRedoEnviroment : public UndoRedo::Environment
{
public:
	struct Op : UndoRedo::Environment::Operation
	{
		Matrix view_;
		virtual void exec()
		{
			// uncomment the following line to make camera links to undo/redo actions
//			BigBangCamera::instance().currentCamera().view( ( view_ );
		}
	};
	virtual OperationPtr internalRecord()
	{
		Op* op = new Op;
		op->view_ = BigBangCamera::instance().currentCamera().view();
		return op;
	}
}
BigBangCameraUndoRedoEnviroment;
