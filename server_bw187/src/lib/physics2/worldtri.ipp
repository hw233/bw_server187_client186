/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/**
 *	@file
 */

#ifdef CODE_INLINE
#define INLINE inline
#else
/// INLINE macro
#define INLINE
#endif

#include <math/planeeq.hpp>

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
INLINE WorldTriangle::WorldTriangle( const Vector3 & v0,
		const Vector3 & v1,
		const Vector3 & v2,
		Flags flags ) :
	v0_( v0 ),
	v1_( v1 ),
	v2_( v2 ),
	flags_( flags ),
	padding_( 0 )
{
}


/**
 *	Default constructor
 */
INLINE WorldTriangle::WorldTriangle() :
	flags_( 0 ),
	padding_( 0 )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

/**
 *	This method returns the first vertex of this triangle.
 */
INLINE
const Vector3 & WorldTriangle::v0() const
{
	return v0_;
}


/**
 *	This method returns the first vertex of this triangle.
 */
INLINE
const Vector3 & WorldTriangle::v1() const
{
	return v1_;
}


/**
 *	This method returns the first vertex of this triangle.
 */
INLINE
const Vector3 & WorldTriangle::v2() const
{
	return v2_;
}


/**
 *	This method returns the normal of this triangle.
 */
INLINE
Vector3 WorldTriangle::normal() const
{
	return (v2_ - v0_).crossProduct(v2_ - v1_);
}


/**
 *	This method returns the flags associated with this triangle.
 */
INLINE
WorldTriangle::Flags WorldTriangle::flags() const
{
	return flags_;
}


/**
 *	This method sets the flags associated with this triangle.
 */
INLINE
void WorldTriangle::flags( Flags newFlags )
{
	flags_ = newFlags;
}


/**
 *	This method returns whether or not this triangle is transparent.
 */
INLINE
bool WorldTriangle::isTransparent() const
{
	return (flags_ & TRIANGLE_TRANSPARENT) != 0;
}


/**
 *	This method returns whether or not this triangle uses blended transparency.
 */
INLINE
bool WorldTriangle::isBlended() const
{
	return (flags_ & TRIANGLE_BLENDED) != 0;
}



// ---------------------------------------------------------------------
// Section: Intersection
// ---------------------------------------------------------------------

//
// Macros
//

/// Helper macro
#define ISECT(VV0,VV1,VV2,D0,D1,D2,isect0,isect1) \
              isect0=VV0+(VV1-VV0)*D0/(D0-D1);    \
              isect1=VV0+(VV2-VV0)*D0/(D0-D2);


/// Helper macro
#define COMPUTE_INTERVALS(VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,isect0,isect1) \
  if(D0D1>0.0f)                                         \
  {                                                     \
    /* here we know that D0D2<=0.0 */                   \
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
    ISECT(VV2,VV0,VV1,D2,D0,D1,isect0,isect1);          \
  }                                                     \
  else if(D0D2>0.0f)                                    \
  {                                                     \
    /* here we know that d0d1<=0.0 */                   \
    ISECT(VV1,VV0,VV2,D1,D0,D2,isect0,isect1);          \
  }                                                     \
  else if(D1*D2>0.0f || D0!=0.0f)                       \
  {                                                     \
    /* here we know that d0d1<=0.0 or that D0!=0.0 */   \
    ISECT(VV0,VV1,VV2,D0,D1,D2,isect0,isect1);          \
  }                                                     \
  else if(D1!=0.0f)                                     \
  {                                                     \
    ISECT(VV1,VV0,VV2,D1,D0,D2,isect0,isect1);          \
  }                                                     \
  else if(D2!=0.0f)                                     \
  {                                                     \
    ISECT(VV2,VV0,VV1,D2,D0,D1,isect0,isect1);          \
  }                                                     \
  else                                                  \
  {                                                     \
    /* triangles are coplanar (Treat as false) */       \
    return false;                                       \
  }


//
// Helper methods
//

/// This function returns the absolute value of the input value.
template <class T>
inline T absValue(T a)
{
	return (a >= 0) ? a : -a;
}


/// This function will swap the values in a and b if a is greater than b.
template <class T>
inline void sort(T & a, T & b)
{
	if (a > b)
	{
		T temp = a;
		a = b;
		b = temp;
	}
}



/**
 *	This method interpolates between the given three values based
 *	on the point 'onTri' which is on the triangle.
 *
 *	The class must support '+', '-', and multiplication by a float.
 *
 *	Currently it only uses a vertical projection.
 */
template <class C> C WorldTriangle::interpolate( const Vector3 & onTri,
	const C & a, const C & b, const C & c ) const
{
	//Vector2 sNt = this->project( onTri );
	//return a + (b-a) * sNt.x() + (c-a) * sNt.y();
	return a;
}

/*
template <> Vector2 WorldTriangle::interpolate( const Vector3 & onTri,
	const Vector2 & a, const Vector2 & b, const Vector2 & c ) const
{
	Vector2 sNt = this->project( onTri );
	return a + (b-a) * sNt.x() + (c-a) * sNt.y();
}
*/


// worldtri.ipp
