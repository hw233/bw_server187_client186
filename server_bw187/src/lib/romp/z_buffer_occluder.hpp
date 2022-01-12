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
 *	This class checks line-of-sight through the z-buffer using occlusion
 *	queries.
 */
#ifndef Z_BUFFER_OCCLUDER_HPP
#define Z_BUFFER_OCCLUDER_HPP

#include "occlusion_query_helper.hpp"
#include "photon_occluder.hpp"
#include "moo/device_callback.hpp"

class LensEffect;

class ZBufferOccluder : public PhotonOccluder, public Moo::DeviceCallback
{
public:
	ZBufferOccluder();
	~ZBufferOccluder();

	static bool isAvailable();

	virtual PhotonOccluder::Result collides(
			const Vector3 & photonSourcePosition,
			const Vector3 & cameraPosition,
			const LensEffect& le );
	virtual void beginOcclusionTests();
	virtual void endOcclusionTests();

	virtual void deleteUnmanagedObjects();	

protected:
	//our own interface
	virtual void writePixel( const Vector3& source );
	virtual void writeArea( const Vector3& source, float size );

private:
	ZBufferOccluder(const ZBufferOccluder&);
	ZBufferOccluder& operator=(const ZBufferOccluder&);	

	UINT	lastSunResult_;	//more sun special casing that must be removed...	
	bool	sunThisFrame_;
	OcclusionQueryHelper	helper_;
	
	class Moo::Material pixelMat_;

	friend std::ostream& operator<<(std::ostream&, const ZBufferOccluder&);
};

#ifdef CODE_INLINE
#include "z_buffer_occluder.ipp"
#endif

#endif //Z_BUFFER_OCCLUDER_HPP