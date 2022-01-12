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
#include "vertices_manager.hpp"

#include <algorithm>

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_trace.hpp"
#include "cstdmf/memory_counter.hpp"
#include "resmgr/bwresource.hpp"
#include "render_context.hpp"
#include "morph_vertices.hpp"
#include "graphics_settings.hpp"


DECLARE_DEBUG_COMPONENT2( "Moo", 0 )

memoryCounterDeclare( vertices );

namespace Moo
{

VerticesManager::VerticesManager()
:	enableMorphVertices_( true )
{
	memoryCounterAdd( vertices );
	memoryClaim( vertices_ );

	// Morph vertices setting
	typedef Moo::GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;
	GraphicsSettingPtr morphSettings = 
		Moo::makeCallbackGraphicsSetting(
			"MORPH_VERTICES", *this, 
			&VerticesManager::setMorphOption,
			1, false, true);

	morphSettings->addOption("OFF", true);
	morphSettings->addOption("ON", true);
	Moo::GraphicsSetting::add(morphSettings);
}

VerticesManager::~VerticesManager()
{
	memoryCounterSub( vertices );
	while (!vertices_.empty())
	{
		memoryClaim( vertices_, vertices_.begin() );
		vertices_.erase( vertices_.begin() );
	}
	memoryClaim( vertices_ );
}


VerticesManager* VerticesManager::instance( )
{
	return pInstance_;
}

void VerticesManager::deleteManagedObjects( )
{
	SimpleMutexHolder smh( verticesLock_ );

	VerticesMap::iterator it = vertices_.begin();
	VerticesMap::iterator end = vertices_.end();

	while( it != end )
	{
		it->second->release();
		++it;
	}
}

void VerticesManager::createManagedObjects( )
{
	SimpleMutexHolder smh( verticesLock_ );

	VerticesMap::iterator it = vertices_.begin();
	VerticesMap::iterator end = vertices_.end();

	while( it != end )
	{
		it->second->load();
		++it;
	}
}


/**
 *	Get the given vertices resource
 *
 *	If parameter 'numNodes' greater than zero, vertex node indices are verified
 *	to be less than 'numNodes', vertex indices that fail verification will
 *	be set to zero, and an error message will be displayed.
 *	If parameter 'numNodes' is zero, no verification is done.
 *	This only applies to skinned vertices.
 */
VerticesPtr VerticesManager::get( const std::string& resourceID, int numNodes /* = 0 */ )
{
	VerticesPtr res = this->find( resourceID );
	if (res) return res;

	MEM_TRACE_BEGIN( resourceID )
	if (BWResource::getExtension( resourceID ) != "mvertices" || !enableMorphVertices_ )
	{
		res = new Vertices( resourceID, numNodes );
	}
	else
	{
		res = new MorphVertices( resourceID, numNodes );
	}
	res->load();

	this->addInternal( &*res );
	MEM_TRACE_END()

	return res;
}

void VerticesManager::del( Vertices* pVertices )
{
	if (pInstance_)
		pInstance_->delInternal( pVertices );
}

/**
 *	Add this vertices to the map of those already loaded
 */
void VerticesManager::addInternal( Vertices * pVertices )
{
	SimpleMutexHolder smh( verticesLock_ );

	vertices_.insert( std::make_pair(pVertices->resourceID(), pVertices) );

	memoryCounterAdd( vertices );
	memoryClaim( vertices_, --vertices_.end() );
}


/**
 *	Remove this vertices from the map of those already loaded
 */
void VerticesManager::delInternal( Vertices * pVertices )
{
	SimpleMutexHolder smh( verticesLock_ );

	VerticesMap::iterator it = vertices_.find( pVertices->resourceID() );
	if (it != vertices_.end())
	{
		vertices_.erase( it );
		return;
	}
	else
	{
		for (it = vertices_.begin(); it != vertices_.end(); it++)
		{
			if (it->second == pVertices)
			{
				//DEBUG_MSG( "VerticesManager::del: %s\n",
				//	pVertices->resourceID().c_str() );
				memoryCounterSub( vertices );
				memoryClaim( vertices_, it );
				vertices_.erase( it );
				return;
			}
		}
	}

	ERROR_MSG( "VerticesManager::del: "
		"Could not find vertices at 0x%08X to delete it\n", pVertices );
}


/*
 *  Set whether or not we want to create morph vertices
 */
void VerticesManager::setMorphOption( int optionIndex )
{
	enableMorphVertices_ = !!optionIndex;
}


/**
 *	Helper class to find vertices by name
 */
class FindVertices
{
public:
	std::string resourceID_;
	FindVertices( const std::string& resourceID )
	: resourceID_( resourceID )
	{
	}
	~FindVertices()
	{

	}

	bool operator () ( const Vertices * vertices )
	{
		return vertices->resourceID() == resourceID_;
	}

};


/**
 *	Find this vertices in the map of those already loaded
 */
VerticesPtr VerticesManager::find( const std::string & resourceID )
{
	SimpleMutexHolder smh( verticesLock_ );

	VerticesMap::iterator it = vertices_.find( resourceID );

	if (it != vertices_.end())
	{
		// try incrementing it, but only keep it if it wasn't zero previously
		// if it was zero, then someone is about to call the destructor, and may well
		// be waiting on the verticesLock_ to remove it from this list.
		return VerticesPtr( it->second, VerticesPtr::FALLIBLE );
		// return NULL if it failed anyway
	}
	return NULL;
}

VerticesManager* VerticesManager::pInstance_ = NULL;

void VerticesManager::init()
{
	MF_ASSERT( pInstance_ == NULL );
	pInstance_ = new VerticesManager;
}

void VerticesManager::fini()
{
	MF_ASSERT( pInstance_ );

	if (pInstance_)
		delete pInstance_;
	pInstance_ = NULL;
}


}

// vertices_manager.cpp
