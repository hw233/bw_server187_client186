/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/



#include "packers.hpp"
#include "packer_helper.hpp"
#include "resmgr/bwresource.hpp"
#include "cdata_packer.hpp"


IMPLEMENT_PACKER( CDataPacker )

int CDataPacker_token = 0;


bool CDataPacker::prepare( const std::string& src, const std::string& dst )
{
	std::string ext = BWResource::getExtension( src );
	if ( ext != "cdata" )
		return false;

	src_ = src;
	dst_ = dst;

	return true;
}

bool CDataPacker::print()
{
	if ( src_.empty() )
	{
		printf( "Error: ChunkPacker not initialised properly\n" );
		return false;
	}

	printf( "CDataFile: %s\n", src_.c_str() );

	return true;
}

bool CDataPacker::pack()
{
	if ( src_.empty() || dst_.empty() )
	{
		printf( "Error: ChunkPacker not initialised properly\n" );
		return false;
	}

	if ( !PackerHelper::copyFile( src_, dst_ ) )
		return false;

	DataSectionPtr root = BWResource::openSection(
		BWResolver::dissolveFilename( dst_ ) );
	if ( !root )
	{
		printf( "Error opening as a DataSection\n" );
		return false;
	}

	// delete the thumbnail section
	root->delChild( "thumbnail.dds" );

	root->delChild( "navmesh" );
#ifndef MF_SERVER
	// In the client, also remove navmesh section
	root->delChild( "worldNavmesh" );
#endif // MF_SERVER

	// save changes on the destination file
	if ( !root->save() )
	{
		printf( "Error saving DataSection\n" );
		return false;
	}

	return true;
}
