/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Constructor(s) for CollidePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for CollidePSA.
 *
 *	@param elasticity			The elasticity to use.
 */
INLINE CollidePSA::CollidePSA( float elasticity ) :
	spriteBased_( false ),
	elasticity_( elasticity ),
	minAddedRotation_( 0.f ),
	maxAddedRotation_( 1.f ),
	entityID_( 0 ),
	soundTag_( "no sound" ),
	soundEnabled_( false ),
	soundSrcIdx_( 0 )
{
}


// -----------------------------------------------------------------------------
// Section: Methods for the CollidePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the spriteBased attribute.
 *
 *	@return Whether the Collide action is sprite based.
 */
INLINE bool CollidePSA::spriteBased() const 
{ 
	return spriteBased_; 
}


/**
 *	This is the Set-Accessor for the spriteBased attribute.
 *
 *	@param Whether the Collide action should be sprite based.
 */
INLINE void CollidePSA::spriteBased( bool s ) 
{ 
	spriteBased_ = s; 
}


/**
 *	This is the Get-Accessor for the elasticity attribute.
 *
 *	@return The elasticity.
 */
INLINE float CollidePSA::elasticity() const 
{ 
	return elasticity_; 
}


/**
 *	This is the Set-Accessor for the elasticity attribute.
 *
 *	@param Sets the elasticity.
 */
INLINE void CollidePSA::elasticity( float e ) 
{ 
	elasticity_ = e; 
}


/**
 *	This is the Get-Accessor for the minAddedRotation attribute.
 *
 *	@return The minimum added rotation.
 */
INLINE float CollidePSA::minAddedRotation() const 
{ 
	return minAddedRotation_; 
}


/**
 *	This is the Set-Accessor for the minAddedRotation attribute.
 *
 *	@param Sets the minimum added rotation.
 */
INLINE void CollidePSA::minAddedRotation( float e ) 
{ 
	minAddedRotation_ = e; 
}


/**
 *	This is the Get-Accessor for the maxAddedRotation attribute.
 *
 *	@return The maximum added rotation.
 */
INLINE float CollidePSA::maxAddedRotation() const 
{ 
	return maxAddedRotation_; 
}


/**
 *	This is the Set-Accessor for the maxAddedRotation attribute.
 *
 *	@param Sets the maximum added rotation.
 */
INLINE void CollidePSA::maxAddedRotation( float e ) 
{ 
	maxAddedRotation_ = e; 
}


/**
 *	This is the Get-Accessor for the soundeEnabled attribute.
 *
 *	@return Whether the Collide action has sound enabled.
 */
INLINE bool CollidePSA::soundEnabled() const 
{ 
	return soundEnabled_; 
}


/**
 *	This is the Set-Accessor for the soundEnabled attribute.
 *
 *	@param Whether the Collide action should have sound enabled.
 */
INLINE void CollidePSA::soundEnabled( bool e ) 
{ 
	soundEnabled_ = e; 
}


/**
 *	This is the Get-Accessor for the soundTag attribute.
 *
 *	@return The sound tag.
 */
INLINE std::string CollidePSA::soundTag() const 
{ 
	return soundTag_; 
}


/**
 *	This is the Set-Accessor for the soundTag attribute.
 *
 *	@param Sets the sound tag.
 */
INLINE void CollidePSA::soundTag( std::string tag ) 
{ 
	soundTag_ = tag; 
}


/**
 *	This is the Get-Accessor for the soundSrcIdx attribute.
 *
 *	@return The sound source index.
 */
INLINE int CollidePSA::soundSrcIdx() const 
{ 
	return soundSrcIdx_; 
}


/**
 *	This is the Set-Accessor for the soundSrcIdx attribute.
 *
 *	@param Sets the sound source index.
 */
INLINE void CollidePSA::soundSrcIdx( int i ) 
{ 
	soundSrcIdx_ = i; 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int CollidePSA::typeID( void ) const
{
	return typeID_;
}

INLINE std::string CollidePSA::nameID( void ) const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) for PyCollidePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyCollidePSA.
 *
 *	@param elasticity			The elasticity to use.
 */
INLINE PyCollidePSA::PyCollidePSA( CollidePSAPtr pAction, PyTypePlus *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}


// -----------------------------------------------------------------------------
// Section: Methods for the PyCollidePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the python Get-Accessor for the spriteBased attribute.
 *
 *	@return Whether the Collide action is sprite based.
 */
INLINE bool PyCollidePSA::spriteBased() const 
{ 
	return pAction_->spriteBased(); 
}


/**
 *	This is the python Set-Accessor for the spriteBased attribute.
 *
 *	@param Whether the Collide action should be sprite based.
 */
INLINE void PyCollidePSA::spriteBased( bool s ) 
{ 
	pAction_->spriteBased( s ); 
}


/**
 *	This is the python Get-Accessor for the elasticity attribute.
 *
 *	@return The elasticity.
 */
INLINE float PyCollidePSA::elasticity() const 
{ 
	return pAction_->elasticity(); 
}


/**
 *	This is the python Set-Accessor for the elasticity attribute.
 *
 *	@param Sets the elasticity.
 */
INLINE void PyCollidePSA::elasticity( float e ) 
{ 
	pAction_->elasticity( e ); 
}


/**
 *	This is the python Get-Accessor for the minAddedRotation attribute.
 *
 *	@return The minimum added rotation.
 */
INLINE float PyCollidePSA::minAddedRotation() const 
{ 
	return pAction_->minAddedRotation(); 
}


/**
 *	This is the python Set-Accessor for the minAddedRotation attribute.
 *
 *	@param Sets the minimum added rotation.
 */
INLINE void PyCollidePSA::minAddedRotation( float e ) 
{ 
	pAction_->minAddedRotation( e ); 
}


/**
 *	This is the python Get-Accessor for the maxAddedRotation attribute.
 *
 *	@return The maximum added rotation.
 */
INLINE float PyCollidePSA::maxAddedRotation() const 
{ 
	return pAction_->maxAddedRotation(); 
}


/**
 *	This is the python Set-Accessor for the maxAddedRotation attribute.
 *
 *	@param Sets the maximum added rotation.
 */
INLINE void PyCollidePSA::maxAddedRotation( float e ) 
{ 
	pAction_->maxAddedRotation( e ); 
}


/**
 *	This is the python Get-Accessor for the soundeEnabled attribute.
 *
 *	@return Whether the Collide action has sound enabled.
 */
INLINE bool PyCollidePSA::soundEnabled() const 
{ 
	return pAction_->soundEnabled(); 
}


/**
 *	This is the python Set-Accessor for the soundEnabled attribute.
 *
 *	@param Whether the Collide action should have sound enabled.
 */
INLINE void PyCollidePSA::soundEnabled( bool e ) 
{ 
	pAction_->soundEnabled( e ); 
}


/**
 *	This is the python Get-Accessor for the soundTag attribute.
 *
 *	@return The sound tag.
 */
INLINE std::string PyCollidePSA::soundTag() const 
{ 
	return pAction_->soundTag(); 
}


/**
 *	This is the python Set-Accessor for the soundTag attribute.
 *
 *	@param Sets the sound tag.
 */
INLINE void PyCollidePSA::soundTag( std::string tag ) 
{ 
	pAction_->soundTag( tag ); 
}


/**
 *	This is the python Get-Accessor for the soundSrcIdx attribute.
 *
 *	@return The sound source index.
 */
INLINE int PyCollidePSA::soundSrcIdx() const 
{ 
	return pAction_->soundSrcIdx(); 
}


/**
 *	This is the python Set-Accessor for the soundSrcIdx attribute.
 *
 *	@param Sets the sound source index.
 */
INLINE void PyCollidePSA::soundSrcIdx( int i ) 
{ 
	pAction_->soundSrcIdx( i ); 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyCollidePSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* collide_psa.ipp */
