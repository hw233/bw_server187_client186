/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/*water.ipp*/

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE float Waters::movementImpact() const
{ 
	return movementImpact_;
}

/**
 *
 */
INLINE bool Waters::drawReflection() 
{ 
	return s_drawReflection_;
}


/**
 *
 */
INLINE void Waters::drawReflection( bool val )
{ 
	s_drawReflection_ = val;
}


/**
 *
 */
INLINE bool Waters::drawRefraction()
{ 
	return s_drawRefraction_;
}


/**
 *
 */
INLINE void Waters::drawRefraction( bool val )
{ 
	s_drawRefraction_ = val;
}


/**
 *
 */
INLINE int Waters::simulationLevel() const
{ 
	return s_simulationLevel_; 
}


/**
 *	Flag for water surface drawing
 */
INLINE void Waters::drawWaters( bool draw )
{
	s_drawWaters_ = draw;
}


INLINE void Waters::simulationEnabled( bool val)
{ 
	s_simulationEnabled_ = val;
}


INLINE bool Waters::simulationEnabled()
{
	return ((s_simulationEnabled_) && (s_simulationLevel_ != 0));
}


/**
 *	Flag that indicates the eye is inside a water volume.
 */
INLINE bool Waters::insideVolume() const
{ 
	return insideWaterVolume_;
}


/**
 *	Flag that indicates the eye is inside a water volume.
 */
INLINE void Waters::insideVolume( bool val )
{ 
	insideWaterVolume_ = val;
}


/**
 *	Flag for water surface drawing
 */
INLINE void Waters::drawWireframe( bool wire )
{
	drawWireframe_ = wire;
}


/**
 *	This method sets the amount of rain that affects all waters
 */
INLINE void Waters::rainAmount( float rainAmount )
{
	rainAmount_ = ::max( 0.f, ::min( 1.f, rainAmount ) );
}


/**
 *
 */
INLINE float Waters::rainAmount() const
{ 
	return rainAmount_;
}

//TODO: check against the bounding box and send off enter/exit events.
INLINE void Waters::playerPos( Vector3 pos )
{
	playerPos_ = pos;
}

INLINE bool Water::shouldDraw() const
{
	return ( initialised_ && !bb_.combinedOutcode() && Waters::s_drawWaters_ );
}


INLINE const Vector3& Water::position() const
{ 
	return config_.position_;
}


INLINE const Vector2& Water::size() const
{ 
	return config_.size_;
}


INLINE Moo::RenderTargetPtr WaterCell::simTexture()
{
	return result();
} 


INLINE int WaterCell::indexCount() const
{
	return nIndices_;
}


INLINE bool WaterCell::simulationActive() const
{
	return isActive();
}


INLINE void WaterCell::initSimulation( int size, float cellSize )
{	
	SimulationCell::init( size, cellSize );
}


INLINE bool WaterCell::bind()
{ 
	if (nIndices_)	
		return (Moo::rc().device()->SetIndices( indexBuffer_.pComObject() ) == D3D_OK);
	else
		return false;
}


INLINE void WaterCell::draw( int nVerts )
{ 
	Moo::rc().drawIndexedPrimitive( D3DPT_TRIANGLESTRIP, 0, 0, nVerts, 0, nIndices_ - 2 );
}


INLINE void WaterCell::initEdge( int index, WaterCell* cell )
{
	edgeCells_[index] = cell;
}


INLINE const Vector2& WaterCell::min()
{
	return min_;
}


INLINE const Vector2& WaterCell::max()
{
	return max_;
}


INLINE const Vector2& WaterCell::size()
{
	return size_;
}

INLINE void WaterCell::deactivate()
{
	SimulationCell::deactivate();
	edge(0);
}


/*water.ipp*/