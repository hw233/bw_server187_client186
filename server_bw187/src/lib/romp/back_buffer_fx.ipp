/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

// back_buffer_fx.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// back_buffer_fx.ipp
INLINE void BackBufferEffect::areaOfEffect( const Vector2& tl, const Vector2& dimensions )
{
	tl_ = tl;
	dimensions_ = dimensions;
}