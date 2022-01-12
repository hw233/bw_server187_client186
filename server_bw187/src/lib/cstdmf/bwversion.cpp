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
#include "bwversion.hpp"

#include "debug.hpp"
#include "watcher.hpp"

// NB: When updating this structure, ensure that it is kept in sync with
//     bigworld/res/version.xml
namespace BWVersion
{
uint16 g_majorNumber = 1;
uint16 g_minorNumber = 8;

// Not to be used for 2.0 and beyond
uint16 g_reservedNumber = 7;

uint16 g_patchNumber = 0;

std::string g_versionString;


// -----------------------------------------------------------------------------
// Section: Get Accessors
// -----------------------------------------------------------------------------

/**
 *	This method returns the major version number.
 */
uint16 majorNumber()
{
	return g_majorNumber;
}


/**
 *	This method returns the minor version number.
 */
uint16 minorNumber()
{
	return g_minorNumber;
}


/**
 *	This method returns the patch version number. This is the number of the
 *	highest patch that has been applied.
 */
uint16 patchNumber()
{
	return g_patchNumber;
}


/**
 *	This method returns a reserved version number. For version 1.9.x, it is the
 *	x. It will not be used for version 2.0 and beyond.
 */
uint16 reservedNumber()
{
	return g_reservedNumber;
}


/**
 *	A string representing the version.
 */
const std::string & versionString()
{
	return g_versionString;
}


// -----------------------------------------------------------------------------
// Section: Set Accessors
// -----------------------------------------------------------------------------

/**
 *	This method is used to update the version string if a version number is
 *	modified.
 */
void updateVersionString()
{
	// TODO: For 2.0 and beyond, the g_reservedNumber will be dropped.
	char buf[ 128 ];
	bw_snprintf( buf, sizeof( buf ), "%d.%d.%d.%d",
			g_majorNumber, g_minorNumber, g_reservedNumber, g_patchNumber );
	g_versionString = buf;
}


/**
 *	This method sets the major version number.
 */
void majorNumber( uint16 number )
{
	if (number != g_majorNumber)
	{
		WARNING_MSG( "BWVersion::majorNumber: Changing from %d to %d\n",
				g_majorNumber, number );
		g_majorNumber = number;
		updateVersionString();
	}
}


/**
 *	This method sets the minor version number.
 */
void minorNumber( uint16 number )
{
	if (number != g_minorNumber)
	{
		WARNING_MSG( "BWVersion::minorNumber: Changing from %d to %d\n",
				g_minorNumber, number );
		g_minorNumber = number;
		updateVersionString();
	}
}


/**
 *	This method sets the patch version number.
 */
void patchNumber( uint16 number )
{
	if (number < g_patchNumber)
	{
		WARNING_MSG( "BWVersion::patchNumber: Changing from %d to %d\n",
				g_patchNumber, number );
	}

	g_patchNumber = number;
	updateVersionString();
}


/**
 *	This method sets the reserved version number.
 */
void reservedNumber( uint16 number )
{
	if (number != g_reservedNumber)
	{
		WARNING_MSG( "BWVersion::reservedNumber: Changing from %d to %d\n",
				g_reservedNumber, number );
		g_reservedNumber = number;
		updateVersionString();
	}
}


// -----------------------------------------------------------------------------
// Section: Initialisation
// -----------------------------------------------------------------------------

/**
 *	This class is used to initialise the BWVersion data.
 */
class StaticIniter
{
public:
	StaticIniter()
	{
		MF_WATCH( "version/major",    g_majorNumber,    Watcher::WT_READ_ONLY );
		MF_WATCH( "version/minor",    g_minorNumber,    Watcher::WT_READ_ONLY );
		MF_WATCH( "version/patch",    g_patchNumber,    Watcher::WT_READ_ONLY );
		MF_WATCH( "version/reserved", g_reservedNumber, Watcher::WT_READ_ONLY );
		MF_WATCH( "version/string",   g_versionString,  Watcher::WT_READ_ONLY );

		updateVersionString();
	}
};

StaticIniter staticIniter;

} // BWVersion

// bwversion.hpp
