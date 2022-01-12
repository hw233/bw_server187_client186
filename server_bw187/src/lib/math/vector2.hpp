/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef VECTOR2_HPP
#define VECTOR2_HPP

#include <iostream>
#include "xp_math.hpp"
#include "cstdmf/stdmf.hpp"

/**
 *	This class implements a vector of two floats.
 *
 *	@ingroup Math
 */
class Vector2 : public Vector2Base
{

public:
	Vector2();
	Vector2( float a, float b );
	explicit Vector2( const Vector2Base & v2 );

	// Use the default compiler implementation for these methods. It is faster.
	// Vector2( const Vector2& v );
	// Vector2& operator  = ( const Vector2& v );

	void setZero( );
	void set( float a, float b ) ;
	void scale( const Vector2 &v, float s );
	float length() const;
	float lengthSquared() const;
	void normalise();

	float dotProduct( const Vector2& v ) const;
	float crossProduct( const Vector2& v ) const;

	void projectOnto( const Vector2& v1, const Vector2& v2 );
	Vector2 projectOnto( const Vector2 & v ) const;

	void operator += ( const Vector2& v );
	void operator -= ( const Vector2& v );
	void operator *= ( float s );
	void operator /= ( float s );

	static const Vector2 & zero()			{ return s_zero; }

private:
    friend std::ostream& operator <<( std::ostream&, const Vector2& );
    friend std::istream& operator >>( std::istream&, Vector2& );

	// This is to prevent construction like:
	//	Vector2( 0 );
	// It would interpret this as a float * and later crash.
	Vector2( int value );

	static Vector2	s_zero;
};

Vector2 operator +( const Vector2& v1, const Vector2& v2 );
Vector2 operator -( const Vector2& v1, const Vector2& v2 );
Vector2 operator *( const Vector2& v, float s );
Vector2 operator *( float s, const Vector2& v );
Vector2 operator *( const Vector2& v1, const Vector2& v2 );
Vector2 operator /( const Vector2& v, float s );
bool operator   ==( const Vector2& v1, const Vector2& v2 );
bool operator   !=( const Vector2& v1, const Vector2& v2 );
bool operator   < ( const Vector2& v1, const Vector2& v2 );

inline bool almostEqual( const Vector2& v1, const Vector2& v2, const float epsilon = 0.0004f )
{
	return almostEqual( v1.x, v2.x, epsilon ) &&
		almostEqual( v1.y, v2.y, epsilon );
}

typedef Vector2 Vector2;

#ifdef CODE_INLINE
#include "vector2.ipp"
#endif

#endif // VECTOR2_HPP
