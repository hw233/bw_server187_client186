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

#include <algorithm>
#include <string>

#include "texture_manager.hpp"
#include "texture_compressor.hpp"
#include "animating_texture.hpp"
#include "render_context.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "sys_mem_texture.hpp"

#include "cstdmf/binaryfile.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/memory_trace.hpp"
#include "cstdmf/memory_counter.hpp"
#include "resmgr/auto_config.hpp"

#include "dds.h"

#ifndef CODE_INLINE
#include "texture_manager.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )

static AutoConfigString s_notFoundBmp( "system/notFoundBmp" ); 

static AutoConfigString s_textureDetailLevels( "system/textureDetailLevels" );

memoryCounterDeclare( texture );

// Helper functions
namespace
{
	std::string toLower(const std::string& input)
	{
		std::string output = input;
		for (uint i = 0; i < output.length(); i++)
		{
			if (output[i] >= 'A' &&
				output[i] <= 'Z')
			{
				output[i] = output[i] - 'A' + 'a';
			}
		}
		return output;
	}

	int calcMipSkip(int qualitySetting, int lodMode)
	{	
		int result = 0;

		switch (lodMode)
		{
			case 0: // disabled
				result = 0;
				break;
			case 1: // normal
				result = qualitySetting;
				break;
			case 2: // low bias
				result = std::min(qualitySetting, 1);
				break;
			case 3: // high bias
				result = std::max(qualitySetting-1, 0);
				break;
		}
		return result;
	}
}

namespace Moo
{

TextureManager::TextureManager() :
	pDevice_( NULL ),
	fullHouse_( false ),
	detailLevelsModTime_( 0 )
{
	memoryCounterAdd( texture );
	memoryClaim( textures_ );
	initDetailLevels( BWResource::openSection( s_textureDetailLevels ) );
	detailLevelsModTime_ = BWResource::modifiedTime( s_textureDetailLevels );
	
	// texture quality settings
	this->qualitySettings_ = 
		Moo::makeCallbackGraphicsSetting(
			"TEXTURE_QUALITY", *this, 
			&TextureManager::setQualityOption, 
			0, true, false);
				
	this->qualitySettings_->addOption("HIGH", true);
	this->qualitySettings_->addOption("MEDIUM", true);
	this->qualitySettings_->addOption("LOW", true);
	Moo::GraphicsSetting::add(this->qualitySettings_);

	// texture compression settings
	this->compressionSettings_ = 
		Moo::makeCallbackGraphicsSetting(
			"TEXTURE_COMPRESSION", *this, 
			&TextureManager::setCompressionOption, 
			0, true, false);
				
	this->compressionSettings_->addOption("OFF", true);
	this->compressionSettings_->addOption("ON", true);
	Moo::GraphicsSetting::add(this->compressionSettings_);
	
	// saved settings may differ from the ones set 
	// in the constructor call (0). If that is the case,
	// both settings will be pending. Commit them now.
	Moo::GraphicsSetting::commitPending();
}

TextureManager::~TextureManager()
{
	memoryCounterSub( texture );
	while (!textures_.empty())
	{
		memoryClaim( textures_, textures_.begin() );
		textures_.erase(textures_.begin());
	}
	memoryClaim( textures_ );
}


TextureManager* TextureManager::instance()
{
	return pInstance_;
}


HRESULT TextureManager::initTextures( )
{
	HRESULT res = S_OK;
	pDevice_ = rc().device();
	TextureMap::iterator it = textures_.begin();

	while( it != textures_.end() )
	{
		(it->second)->load();
		it++;
	}
	return res;
}

HRESULT TextureManager::releaseTextures( )
{
	HRESULT res = S_OK;
	TextureMap::iterator it = textures_.begin();

	while( it != textures_.end() )
	{
		HRESULT hr;
		if( FAILED( hr = (it->second)->release() ) )
		{
			res = hr;
		}

		it++;
	}

	pDevice_ = NULL;

	return res;
}



void TextureManager::deleteManagedObjects( )
{
	DEBUG_MSG( "Moo::TextureManager - Used texture memory %d\n", textureMemoryUsed() );
	releaseTextures();
}

void TextureManager::createManagedObjects( )
{
	initTextures();
}



uint32 TextureManager::textureMemoryUsed( ) const
{
	const std::string animated( ".texanim" );

	TextureMap::const_iterator it = textures_.begin();
	TextureMap::const_iterator end = textures_.end();
	uint32 tm = 0;
	while( it != end )
	{
		const std::string& id = it->second->resourceID();
		if (id.length() < animated.length() ||
            id.substr( id.length() - animated.length(), animated.length() ) != animated )
		{
			tm += (it->second)->textureMemoryUsed( );
		}
		++it;
	}
	return tm;
}


/**
 *	Set whether or not we have a full house
 *	(and thus cannot load any new textures)
 */
void TextureManager::fullHouse( bool noMoreEntries )
{
	fullHouse_ = noMoreEntries;
}



/**
 *	This method saves a dds file from a texture.
 *	@param texture the DX::BaseTexture to save out (important has to be 32bit colour depth.
 *	@param ddsName the filename to save the file to.
 *	@param format the TF_format to save the file in.
 *	@return true if the function succeeds.
 */
bool TextureManager::writeDDS( DX::BaseTexture* texture, 
							  const std::string& ddsName, D3DFORMAT format, 
							  int numDestMipLevels )
{
	MF_ASSERT( texture );
	bool ret = false;

	// write out the dds
	D3DRESOURCETYPE resType = texture->GetType();
	if (resType == D3DRTYPE_TEXTURE)
	{
		DX::Texture* tSrc = (DX::Texture*)texture;

		TextureCompressor tc( tSrc, format, numDestMipLevels );

//		tc.pSrcTexture( tSrc );
//		tc.destinationFormat( format );
		ret = tc.save( ddsName );
	}

	return ret;
}


#define FNM_ENTRY( x ) fnm.insert( std::make_pair( #x, D3DFMT_##x ) );

typedef StringHashMap<D3DFORMAT> FormatNameMap;
/**
 *	This static function returns a map that converts from a format name
 *	into a D3DFORMAT enum value.
 */
static const FormatNameMap & formatNameMap()
{
	static FormatNameMap fnm;
	if (!fnm.empty()) return fnm;

	/* See d3d9types.h */
	FNM_ENTRY( UNKNOWN )

	FNM_ENTRY( R8G8B8 )
	FNM_ENTRY( A8R8G8B8 )
	FNM_ENTRY( X8R8G8B8 )
	FNM_ENTRY( R5G6B5 )
	FNM_ENTRY( X1R5G5B5 )
	FNM_ENTRY( A1R5G5B5 )
	FNM_ENTRY( A4R4G4B4 )
	FNM_ENTRY( R3G3B2 )
	FNM_ENTRY( A8 )
	FNM_ENTRY( A8R3G3B2 )
	FNM_ENTRY( X4R4G4B4 )
	FNM_ENTRY( A2B10G10R10 )
	FNM_ENTRY( G16R16 )

	FNM_ENTRY( A8P8 )
	FNM_ENTRY( P8 )

	FNM_ENTRY( L8 )
	FNM_ENTRY( A8L8 )
	FNM_ENTRY( A4L4 )

	FNM_ENTRY( V8U8 )
	FNM_ENTRY( L6V5U5 )
	FNM_ENTRY( X8L8V8U8 )
	FNM_ENTRY( Q8W8V8U8 )
	FNM_ENTRY( V16U16 )
//	FNM_ENTRY( W11V11U10 )
	FNM_ENTRY( A2W10V10U10 )

	FNM_ENTRY( UYVY )
	FNM_ENTRY( YUY2 )
	FNM_ENTRY( DXT1 )
	FNM_ENTRY( DXT2 )
	FNM_ENTRY( DXT3 )
	FNM_ENTRY( DXT4 )
	FNM_ENTRY( DXT5 )

	FNM_ENTRY( D16_LOCKABLE )
	FNM_ENTRY( D32 )
	FNM_ENTRY( D15S1 )
	FNM_ENTRY( D24S8 )
	FNM_ENTRY( D16 )
	FNM_ENTRY( D24X8 )
	FNM_ENTRY( D24X4S4 )


	FNM_ENTRY( VERTEXDATA )
	FNM_ENTRY( INDEX16 )
	FNM_ENTRY( INDEX32 )
	return fnm;
}

#define DEFAULT_TGA_FORMAT D3DFMT_DXT3
#define DEFAULT_BMP_FORMAT D3DFMT_DXT1


/**
 *	This method inits the texture detail levels used by the texture manager.
 *	If no section or an empty section is passed in, default detail levels will
 *	be used.
 *	@param pDetailLevels the datasection to initialise the detail levels from.
 */
void TextureManager::initDetailLevels( DataSectionPtr pDetailLevels )
{
	detailLevels_.clear();
	if (pDetailLevels)
	{
		std::vector< DataSectionPtr > pSections;
		pDetailLevels->openSections( "detailLevel", pSections );
		for (uint32 i = 0; i < pSections.size(); i++)
		{
			TextureDetailLevelPtr pDetail = new TextureDetailLevel;
			pDetail->init( pSections[i] );
			detailLevels_.push_back( pDetail );
		}
	}
	if (!detailLevels_.size())
	{
		TextureDetailLevelPtr pDetail = new TextureDetailLevel;
		pDetail->postfixes().push_back( "norms.bmp" );
		pDetail->postfixes().push_back( "norms.tga" );
		detailLevels_.push_back( pDetail );
		pDetail = new TextureDetailLevel;
		pDetail->format( D3DFMT_DXT1 );
		pDetail->postfixes().push_back( "bmp" );
		detailLevels_.push_back( pDetail );
		pDetail = new TextureDetailLevel;
		pDetail->format( D3DFMT_DXT3 );
		pDetail->postfixes().push_back( "tga" );
		detailLevels_.push_back( pDetail );
	}
}

/**
 *	Sets the texture format to be used when loading the specified texture.
 *	This format will override all information defined in a texformat file 
 *	or in the global texture_detail_levels file. Other fields, such as 
 *	formatCompressed, lodMode, reduceDim, maxDim and minDim, will be set 
 *	to their default values. Call this function before loading the texture
 *	for the first time. The format will not be changed if the texture has
 *	already been loaded. 
 *
 *	Also, this function will not work if the texture has already been 
 *	converted into a DDS in the filesystem. For this reason, when using 
 *	this function, make sure this same format is used when batch converting 
 *	this texture map for deployment (in the absence of the original file,
 *	the format used to write the DDS will be the one used).
 *	
 *	@param	fileName	full pach to the texture whose format should be set
 *						set this will be used like the contains field of the 
 *						texture_detail_levels, potentially affecting more 
 *						files if the file name provided is not the fullpath.
 *
 *	@param	format		the texture format to be used.
 */
void TextureManager::setFormat( const std::string & fileName, D3DFORMAT format )
{
	TextureDetailLevelPtr pDetail = new TextureDetailLevel;
	pDetail->init(fileName, format);
	detailLevels_.insert(detailLevels_.begin(), pDetail);
}

/**
 *	This method reloads all the textures used from disk.
 */
void TextureManager::reloadAllTextures()
{
	TextureMap::iterator it = textures_.begin();
	TextureMap::iterator end = textures_.end();
	while (it != end)
	{
		BaseTexture *pTex = it->second;
		std::string resourceName = pTex->resourceID();
		if (BWResource::getExtension(resourceName) == "dds")
		{
			BWResource::instance().purge( pTex->resourceID() );
			resourceName = prepareResource( resourceName, true );			
		}		
		it++;
	}	

	it = textures_.begin();
	end = textures_.end();
	while (it != end)
	{
		BaseTexture *pTex = it->second;
		if ( pTex->resourceID() == s_notFoundBmp.value() )
			pTex->reload( prepareResource( it->first ) );
		else
			pTex->reload();		
		it++;
	}	
}

/**
 *	This method changes the current texture detail level of the game.
 *	First set the appropriate values in the appropriate xml file, then
 *	call this method at runtime to reload all textures.
 */
void TextureManager::recalculateDDSFiles()
{	
	//do this to force recalculation of dds files
	DataSectionPtr pSect = BWResource::openSection(s_textureDetailLevels);
	pSect->save();
	detailLevelsModTime_ = BWResource::modifiedTime(s_textureDetailLevels);
	initDetailLevels( pSect );

	reloadAllTextures();

	//do this to force other objects to rebind to textures.
	//not needed, due to base texture reload method.
	//rc().changeMode( Moo::rc().modeIndex(), Moo::rc().windowed() );	
}


/**
 *	This static mehtod matches a detail level to the filename of a texture
 *	@param originalName the original name of the texture to get detail level info for.
 */
TextureDetailLevelPtr TextureManager::detailLevel( const std::string& originalName )
{
	TextureDetailLevelPtr pRet = new TextureDetailLevel;
	
	DataSectionPtr pFmtSect = BWResource::openSection(
		BWResource::removeExtension( originalName ) + ".texformat" );
	
	if (pFmtSect)
	{
		pRet->init( pFmtSect );
	}
	else
	{
		TextureDetailLevels& tdl = instance()->detailLevels_;
		TextureDetailLevels::iterator it = tdl.begin();
		while (it != tdl.end())
		{
			if ((*it)->check( originalName ))
			{
				return *it;
			}
			it++;
		}
	}
	return pRet;
}

/**
 *	This method converts a texture file to a .dds.
 *	If there is a .dds file there already, this method will get the 
 *	format from the current dds if not it will compress .tgas to dxt3 and .bmps to dxt1.
 *	@param originalName the file to convert.
 *	@param ddsName the file to convert to.
 *	@return true if the operation is successful.
 */
bool TextureManager::convertToDDS( 
	const std::string& originalName, 
	const std::string& ddsName,
	      D3DFORMAT    format)
{
	bool ret = false;
	if (rc().device())
	{
		BinaryPtr texture = BWResource::instance().rootSection()->readBinary( originalName );
		if (texture)
		{
			DX::Texture* tex = NULL;

			TextureDetailLevelPtr pDetailLevel = TextureManager::detailLevel( originalName );

			if (pDetailLevel->mipCount() == 0)
			{
				// Create the texture and the mipmaps.
				HRESULT hr = D3DXCreateTextureFromFileInMemoryEx( 
					Moo::rc().device(), texture->data(), texture->len(),
					D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8,
					D3DPOOL_SYSTEMMEM, D3DX_FILTER_BOX|D3DX_FILTER_MIRROR, 
					D3DX_FILTER_BOX|D3DX_FILTER_MIRROR, 0, NULL, NULL, &tex );

                if (SUCCEEDED(hr))
                {	               
				    ret = writeDDS( tex, ddsName, format );
				    tex->Release();
    				
				    TRACE_MSG( "TextureManager : write DDS %s\n", ddsName.c_str() );
                }
			}
			else
			{
				HRESULT hr = D3DXCreateTextureFromFileInMemoryEx( 
					Moo::rc().device(), texture->data(), texture->len(),
					D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_A8R8G8B8,
					D3DPOOL_SCRATCH, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, NULL, NULL, &tex );
					
				if (SUCCEEDED( hr ))
				{
					D3DSURFACE_DESC desc;
					tex->GetLevelDesc( 0, &desc );
					
					uint32 width = desc.Width;
					uint32 height = desc.Height;
					uint32 newWidth = width;
					uint32 newHeight = height;
					uint32 mipBase = pDetailLevel->mipSize();

					if (pDetailLevel->horizontalMips())
					{
						newWidth = mipBase != 0 
							? mipBase 
							: newWidth / 2;
						mipBase = newWidth;
					}
					else
					{
						newHeight = mipBase != 0 
							? mipBase 
							: newHeight / 2;
						mipBase = newHeight ;
					}

                    uint32 realMipCount = 0;
					uint32 levelWidth = newWidth;
					uint32 levelHeight = newHeight;
					uint32 levelOffset = 0;
					uint32 levelSize = mipBase;
					uint32 levelMax = pDetailLevel->horizontalMips() ? width : height;
					uint32 mipCount = pDetailLevel->mipCount() > 0 ?  pDetailLevel->mipCount() : 0xffffffff;
					while (levelWidth > 0 &&
						levelHeight > 0 &&
						realMipCount < pDetailLevel->mipCount() &&
						(levelSize + levelOffset) <= levelMax)
					{
						realMipCount++;
						levelHeight = levelHeight >> 1;
						levelWidth = levelWidth >> 1;
						levelOffset += levelSize;
						levelSize = levelSize >> 1;
					}
					
					DX::Texture* newTexture = NULL;
					hr = rc().device()->CreateTexture( newWidth, newHeight, realMipCount, 0, D3DFMT_A8R8G8B8,
						D3DPOOL_SCRATCH, &newTexture, NULL );
					if (SUCCEEDED(hr))
					{
						uint32 levelWidth = newWidth;
						uint32 levelHeight = newHeight;
						uint32 levelBase = 0;
						bool horizontal = pDetailLevel->horizontalMips();

						DX::Surface* pSurfSrc = NULL;
						hr = tex->GetSurfaceLevel(0, &pSurfSrc);
						RECT srcRect;
						for (uint32 iLevel = 0; iLevel < realMipCount; iLevel++)
						{
							srcRect.top = horizontal ? 0 : levelBase;
							srcRect.bottom = srcRect.top + levelHeight;
							srcRect.left = horizontal ? levelBase : 0;
							srcRect.right = srcRect.left + levelWidth;
							levelBase += mipBase;
							levelWidth = levelWidth >> 1;
							levelHeight = levelHeight >> 1;
							mipBase = mipBase >> 1;
							DX::Surface* pSurfDest = NULL;
							hr = newTexture->GetSurfaceLevel(iLevel, &pSurfDest);
							hr = D3DXLoadSurfaceFromSurface(pSurfDest, NULL, NULL,
								pSurfSrc, NULL, &srcRect, D3DX_FILTER_NONE, 0);
							pSurfDest->Release();
						}
						pSurfSrc->Release();
						tex->Release();
						ret = writeDDS( newTexture, ddsName, format, realMipCount );
						newTexture->Release();
						TRACE_MSG( "TextureManager : write DDS %s\n", ddsName.c_str() );

					}
				}
			}
		}
	}
	return ret;
}

/**
 *	This method gets the given texture resource
 */
BaseTexturePtr TextureManager::get( const std::string& name,
	bool allowAnimation, bool mustExist, bool loadIfMissing )
{
#define DISABLE_TEXTURING 0
#if DISABLE_TEXTURING

	std::string saniName      = prepareResourceName(name);
	TextureDetailLevelPtr lod = TextureManager::detailLevel(saniName);
	D3DFORMAT format          = lod->format();

	typedef std::map<D3DFORMAT, BaseTexturePtr> BaseTextureMap;
	static BaseTextureMap s_debugTextureMap;
	BaseTextureMap::const_iterator textIt = s_debugTextureMap.find(format);
	if (textIt != s_debugTextureMap.end())
	{
		return textIt->second;
	}
#endif // DISABLE_TEXTURING

	std::string resourceName = toLower( name );
	BaseTexturePtr res = NULL;

	// make sure we're not loading thin air
	if (!resourceName.length())
	{
		//ERROR_MSG( "TextureManager::get: Resource name to get was empty\n" );
		return NULL;
	}

	if (resourceName.length() > 3)
	{
		if (allowAnimation)
		{
			res = this->find( BWResource::removeExtension( resourceName ) + ".texanim" );
			if (res) return res;
		}
		res = this->find( BWResource::removeExtension( resourceName ) + ".tex" );
		if (res) return res;
	}

	// try it as an already loaded dds then
	std::string ddsName = BWResource::removeExtension( resourceName ) + ".dds";
	res = this->find( ddsName );
	if (res) return res;

	// try it as a compressed dds then
	std::string compressedName = BWResource::removeExtension( resourceName ) + ".c.dds";
	res = this->find( compressedName );
	if (res) return res;

	if (!loadIfMissing) return NULL;

	// if couldn't create the dds for this resource last time, then the
	// original resource should be in the cache already
	res = this->find( prepareResourceName(resourceName) );
	if (res) return res;

	resourceName = prepareResource(resourceName);

	// alright, we're ready to load something now

	// make sure we're allowed to load more stuff
	if (fullHouse_)
	{
		CRITICAL_MSG( "TextureManager::getTextureResource: "
			"Loading the texture '%s' into a full house!\n",
			resourceName.c_str() );
	}

	BaseTexturePtr result = NULL;
	result = this->getUnique(resourceName, allowAnimation, mustExist);		
	
#if DISABLE_TEXTURING
		s_debugTextureMap.insert(std::make_pair(result->format(), result));
#endif // DISABLE_TEXTURING
		
	return result;
}

BaseTexturePtr TextureManager::getUnique(
	const std::string& sanitisedResourceID, bool allowAnimation, bool mustExist )
{
	BaseTexturePtr res = NULL;

	// load the texture
	MEM_TRACE_BEGIN( sanitisedResourceName )
	TextureDetailLevelPtr lod = TextureManager::detailLevel(sanitisedResourceID);

	if (allowAnimation)
	{
		std::string animTextureName = BWResource::removeExtension( sanitisedResourceID ) + ".texanim";
		DataSectionPtr animSect = BWResource::instance().openSection(animTextureName);
		if (animSect.exists())
		{
			res = new AnimatingTexture( animTextureName );
			this->addInternal( &*res );
		}
	}
	if (!res.exists())
	{
		int lodMode = lod->lodMode();
		int qualitySetting = this->qualitySettings_->activeOption();
		ManagedTexture * pMT = new ManagedTexture(
			sanitisedResourceID, mustExist, 
			calcMipSkip(qualitySetting, lodMode));
			
		if (pMT->valid() || mustExist)
		{
			res = pMT;
			if ( res->resourceID() == s_notFoundBmp.value() )
				this->addInternal( pMT, sanitisedResourceID );
			else
				this->addInternal( pMT );

			// TODO: clean up DDSs in the repository
			// so that this warbubg can be enabled
		}
		else
		{
			delete pMT;
		}
	}

	MEM_TRACE_END()
	return res;
}

/**
 *	This method loads a texture into a system memory resource.
 *	@param resourceID the name of the texture in the res tree.
 *	@return the pointer to the system memory base texture.
 */
BaseTexturePtr	TextureManager::getSystemMemoryTexture( const std::string& resourceID )
{
	std::string resourceName = toLower( resourceID );
	resourceName = prepareResource( resourceName );
	SysMemTexture* sysTexture = new SysMemTexture( resourceName );
    
	BaseTexturePtr pResult;
    if (!sysTexture->failedToLoad())
	{
		pResult = sysTexture;
	}
	else
	{
		delete sysTexture;
	}
	return pResult;
}

std::string TextureManager::prepareResourceName( const std::string& resourceName )
{
	std::string ext = BWResource::getExtension(resourceName);
	std::string baseName = BWResource::removeExtension(resourceName);
	if (BWResource::getExtension(baseName) == "c")
	{
		baseName = BWResource::removeExtension(baseName);
	}
	
	_strlwr( const_cast<char*>( ext.c_str() ) );

	if (ext == "tex" || ext == "texanim") 
	{
		ext = "dds";
	}

	// if the resource is a dds, find 
	// the resource that spawns the dds
	if (ext == "dds")
	{
		static char exts[8][4] = { 
			"bmp", "tga", "jpg", "png", 
			"hdr", "pfm", "dib", "ppm" };
			
		for (uint32 i = 0; i <8; i++)
		{
			std::string fileName = baseName + "." + exts[i];
			if (BWResource::fileExists(fileName))
			{
				ext = exts[i];
				i = 8;
			}
		}
	}

	return baseName + "." + ext;
}

std::string TextureManager::prepareResource( const std::string& resourceName, bool forceConvert /*= false*/ )
{
	// ok, we haven't loaded this texture yet.
	// 1) if the extension is dds we need to find the source texture
	//    else use the given resource.
	// 2) find the source file and check if the dds needs to be regenerated
	std::string baseName      = BWResource::removeExtension(resourceName);
	std::string resName       = prepareResourceName( resourceName );
	std::string ext           = BWResource::getExtension(resName);
	TextureDetailLevelPtr lod = TextureManager::detailLevel(resName);
	D3DFORMAT comprFormat     = lod->format();
	std::string comprSuffix   = "";
	int comprSettings         = this->compressionSettings_->activeOption();
	if (comprSettings == 1 && lod->formatCompressed() != D3DFMT_UNKNOWN)
	{
		comprSuffix = ".c";
		comprFormat = lod->formatCompressed();
	}

	std::string ddsName = baseName + comprSuffix + ".dds";
	std::string bakName = baseName + ".dds";
	
	std::string result;
	if (ext == "dds")
	{
		// if no original file was found for this texture map,
		// look for the dds with the correct compression settings. 
		// If this can't be found, use the plain dds.
		result = BWResource::fileExists(ddsName)
			? ddsName
			: bakName;
	}
	else
	{
		// if the original file is present, regenerate the dds if
		// needed (with the corred compression settings) and use it.		
		std::string tfmName = baseName + ".texformat";
		
		// check if there is a .dds texture with the same name 
		// as the texture we are trying to load. if there is 
		// check  the timestamp of it, if it's older, reconvert. 
		// i.e.: if the dds doesn't exist or both (the res exists 
		// and it is not older by 15min or less than the dds)

		// update DDS if the 
		// dds file does not exist
		uint64 ddsTime = BWResource::modifiedTime( ddsName );
		bool needsUpdate = (ddsTime == 0);
		
		if (!needsUpdate)
		{
			// ok, update DDS if the original resource has been modified
			uint64 resTime = BWResource::modifiedTime( resName );
			if (resTime != 0)
			{
				//Make the dds file appear slightly older, for cvs leeway
				uint64 delta = uint64(900) * uint64(10000000);
				// ddsTime -= delta;
				
				needsUpdate = (ddsTime < resTime);
			
				if (!needsUpdate)
				{
					// ok, update DDS if the texture detail file has been modified
					if (detailLevelsModTime_ != 0) 
					{
						needsUpdate = (ddsTime < detailLevelsModTime_);
					}
					
					if (!needsUpdate)
					{
						// ok, update DDS if the .texformat file has been modified
						uint64 tfmTime = BWResource::modifiedTime( tfmName );
						if (tfmTime !=0) 
						{
							needsUpdate = (ddsTime < tfmTime);
						}
					}					
				}
			}
		}		
		
		if (forceConvert || needsUpdate)			
		{
			// it does not exist or is not up 
			// to date, so convert it to a dds
			if (BWResource::fileExists(resName))
			{
				result = convertToDDS( resName, ddsName, comprFormat )
					? ddsName
					: resName;
			}
			else
			{
				// but original file does not exists will have to 
				// do with the fallback option (the plain old dds)
				result = bakName;
			}
		}
		else
		{
			result = ddsName;
		}
	}
	
	return result;
}


void TextureManager::del( BaseTexture * pTexture )
{
	if (pInstance_)
		pInstance_->delInternal( pTexture );
}

/**
 *	Add this texture to the map of those already loaded
 */
void TextureManager::addInternal( BaseTexture * pTexture, std::string resourceID )
{
	SimpleMutexHolder smh( texturesLock_ );

	if ( !resourceID.empty() )
		textures_.insert( std::make_pair( resourceID, pTexture ) ) ;
	else
		textures_.insert( std::make_pair(pTexture->resourceID(), pTexture ) );

	memoryCounterAdd( texture );
	memoryClaim( textures_, --textures_.end() );
}


/**
 *	Remove this texture from the map of those already loaded
 */
void TextureManager::delInternal( BaseTexture * pTexture )
{
	SimpleMutexHolder smh( texturesLock_ );

	for (TextureMap::iterator it = textures_.begin(); it != textures_.end(); it++)
	{
		if (it->second == pTexture)
		{
			//DEBUG_MSG( "TextureManager::del: %s\n", pTexture->resourceID().c_str() );
			memoryCounterSub( texture );
			memoryClaim( textures_, it );
			textures_.erase( it );
			return;
		}
	}

	WARNING_MSG( "TextureManager::del: "
		"Could not find texture '%s' at 0x%08X to delete it\n",
		pTexture->resourceID().c_str(), pTexture );
}



/**
 *	This is a helper class used to find a texture with the given name
 */
class FindTexture
{
public:
	FindTexture( const std::string& resourceID ) : resourceID_( resourceID ) { }
	~FindTexture() { }

	bool operator () ( BaseTexture * pTexture )
		{ return pTexture->resourceID() == resourceID_; }

	std::string resourceID_;
};


/**
 *	Find this texture in the map of those already loaded
 */
BaseTexturePtr TextureManager::find( const std::string & resourceID )
{
	SimpleMutexHolder smh( texturesLock_ );

	TextureMap::iterator it;

	it = textures_.find( resourceID );
	if (it == textures_.end())
		return NULL;

	return BaseTexturePtr( it->second, BaseTexturePtr::FALLIBLE );
	// if ref count was zero then someone is blocked on our
	// texturesLock_ mutex waiting to delete it, so we return NULL
}

/**
 *	Sets the texture quality setting. Implicitly called 
 *	whenever the user changes the TEXTURE_QUALITY setting.
 *
 *	Texture quality can vary from 0 (highest) to 2 (lowest). Lowering texture
 *	quality is done by skipping the topmost mipmap levels (normally two in the 
 *	lowest quality setting, one in the medium and none in the highest). 
 *
 *	Artists can tweak how each texture responds to the quality setting using the 
 *	<lodMode> field in the ".texformat" file or global texture_detail_level.xml. 
 *	The table bellow shows the number of lods skipped according to the texture's
 *	lodMode and the current quality setting.
 *
 *					texture quality setting
 *	lodMode			0		1		2
 *	0 (disabled)	0		0		0
 *	1 (normal)		0		1		2
 *	2 (low bias)	0		1		1
 *	3 (high bias)	0		0		1
 */
void TextureManager::setQualityOption(int optionIndex)
{
	TextureMap::iterator it  = textures_.begin();
	TextureMap::iterator end = textures_.end();
	while (it != end)
	{
		BaseTexture *pTex = it->second;
		TextureDetailLevelPtr lod = TextureManager::detailLevel(pTex->resourceID());
		int lodMode    = lod->lodMode();
		int newMipSkip = calcMipSkip(optionIndex, lodMode);
		if (lodMode != 2 && newMipSkip != pTex->mipSkip())
		{
			pTex->mipSkip(newMipSkip);
			this->dirtyTextures_.push_back(
				std::make_pair(pTex, pTex->resourceID()));
		}
		it++;
	}		

	int pendingOption = 0;
	if (!Moo::GraphicsSetting::isPending(
			this->compressionSettings_, pendingOption))
	{
		this->reloadDirtyTextures();
	}
}

/**
 *	Sets the texture compression setting. Implicitly called 
 *	whenever the user changes the TEXTURE_COMPRESSION setting.
 *
 *	Only textures with <formatCompressed> specified in their ".texformat" 
 *	or the global texture_detail_level.xml file. The format specified in 
 *	formatCompressed will be used whenever texture compressio is enabled. 
 *	If texture compression is disabled or formatCompressed has not been 
 *	specified, <format> will be used.
 */
void TextureManager::setCompressionOption(int optionIndex)
{
	TextureMap::iterator it  = textures_.begin();
	TextureMap::iterator end = textures_.end();
	while (it != end)
	{
		BaseTexture *pTex = it->second;
		std::string resourceName = pTex->resourceID();
		TextureDetailLevelPtr lod = TextureManager::detailLevel(resourceName);
		if (lod->formatCompressed() != D3DFMT_UNKNOWN &&
			BWResource::getExtension(resourceName) == "dds")
		{
			resourceName = prepareResource(resourceName);			
			this->dirtyTextures_.push_back(std::make_pair(pTex, resourceName));
		}		
		it++;
	}
	
	int pendingOption = 0;
	if (!Moo::GraphicsSetting::isPending(
			this->qualitySettings_, pendingOption))
	{
		this->reloadDirtyTextures();
	}
}

/**
 *	Reloads all textures in the dirty textures list. Textures are 
 *	added to the dirty list whenever the texture settings change,
 *	either quality or compression. 
 */
void TextureManager::reloadDirtyTextures()
{
	TextureStringVector::const_iterator texIt  = this->dirtyTextures_.begin();
	TextureStringVector::const_iterator texEnd = this->dirtyTextures_.end();
	while (texIt != texEnd)
	{
		this->delInternal(texIt->first);
		texIt->first->reload(texIt->second);
		this->addInternal(texIt->first);
		++texIt;
	}
	this->dirtyTextures_.clear();
}


bool startsWith( const std::string& string, const std::string& substring )
{
	bool res = false;
	std::string::size_type length = substring.length();
	if (string.length() >= length)
	{
		res = substring == string.substr( 0 , length );
	}
	return res;
}

bool endsWith( const std::string& string, const std::string& substring )
{
	bool res = false;
	std::string::size_type length = substring.length();
	std::string::size_type length2 = string.length();

	if (length2 >= length)
	{
		res = substring == string.substr( length2 - length, length );
	}
	return res;
}

bool containsString( const std::string& string, const std::string& substring )
{
	return string.find( substring ) != std::string::npos;
}

template<class Checker>
bool stringCheck( const std::string& mainString, 
				 const TextureDetailLevel::StringVector& subStrings,
				Checker checker )
{
	bool ret = !subStrings.size();
	TextureDetailLevel::StringVector::const_iterator it = subStrings.begin();
	while (it != subStrings.end() && ret == false)
	{
		if(checker( mainString, *it ))
			ret = true;
		it++;
	}
	return ret;
}

TextureDetailLevel::TextureDetailLevel() : 
	format_( D3DFMT_A8R8G8B8 ),
	formatCompressed_( D3DFMT_UNKNOWN ),
	maxDim_( 2048 ),
	minDim_( 1 ),
	reduceDim_( 0 ),
	lodMode_( 0 ),
	mipCount_( 0 ),
	mipSize_( 0 ),
	horizontalMips_( false )
{

}

TextureDetailLevel::~TextureDetailLevel()
{

}

void TextureDetailLevel::clear( )
{
	prefixes_.clear();
	postfixes_.clear();
	contains_.clear();
	maxDim_ = 2048;
	minDim_ = 1;
	reduceDim_ = 0;
	lodMode_ = 0;
	format_ = D3DFMT_A8R8G8B8;
	formatCompressed_ = D3DFMT_UNKNOWN;
	mipCount_ = 0;
	mipSize_ = 0;
	horizontalMips_ = false;
}


/**
 *	This method loads the matching and conversion criteria from
 *	a data section.
 */
void TextureDetailLevel::init( DataSectionPtr pSection )
{
	pSection->readStrings( "prefix", prefixes_ );
	pSection->readStrings( "postfix", postfixes_ );
	pSection->readStrings( "contains", contains_ );
	maxDim_ = pSection->readInt( "maxDim", maxDim_ );
	minDim_ = pSection->readInt( "minDim", minDim_ );
	reduceDim_ = pSection->readInt( "reduceDim", reduceDim_ );
	lodMode_ = pSection->readInt( "lodMode", lodMode_ );
	mipCount_ = pSection->readInt( "mipCount", mipCount_ );
	horizontalMips_ = pSection->readBool( "horizontalMips", horizontalMips_ );
	mipSize_ = pSection->readInt( "mipSize", mipSize_ );
	
	std::string formatName = "A8R8G8B8";
	const FormatNameMap & fnm = formatNameMap();
	{
		for (FormatNameMap::const_iterator it = fnm.begin(); it != fnm.end(); it++)
		{
			if (it->second == format_) formatName = it->first;
		}
	}
	formatName = pSection->readString( "format", formatName );
	StringHashMap<D3DFORMAT>::const_iterator it = fnm.find( formatName );
	if (it != fnm.end())
		format_ = it->second;

	formatName = "UNKNOWN";
	formatName = pSection->readString( "formatCompressed", formatName );
	it = fnm.find( formatName );
	if (it != fnm.end())
	{
		formatCompressed_ = it->second;
	}
}

/**
 *	Initialises this TextureDetailLevel object, matching the given file name to the
 *	provided texture format. All other parameters are left with their default values.
 */
void TextureDetailLevel::init( const std::string & fileName, D3DFORMAT format )
{
	contains_.push_back(fileName);
	format_ = format;
}

/**
 *	This method checks to see if a resource name matches this detail level
 *	@param resourceID the resourceID to check
 *	@return true if this detail level matches the resourceID
 */
bool TextureDetailLevel::check( const std::string& resourceID )
{
	bool match = false;
	match = stringCheck( resourceID, prefixes_, startsWith );
	if (match)
		match = stringCheck( resourceID, postfixes_, endsWith );
	if (match)
		match = stringCheck( resourceID, contains_, containsString );
	return match;
}

/**
 *	This method calculates the width and height of a texture to be 
 *	processed by this detail level.
 *	@param width this argument passes in the original width and returns the new width
 *	@param height this argument passes in the original height and returns the new height
 */
void TextureDetailLevel::widthHeight( uint32& width, uint32& height )
{
	// grab the min and max dimensions 
	// and set the default divisor
	uint32 minDim = min( width, height );
	uint32 maxDim = max( width, height );
	uint32 divisor = 1;

    // if the smallest texture dimension 
	// is bigger than minDim do size conversions
	if (minDim_ < minDim)
	{

		// Halve the dimensions reduceDim_ times
		uint32 min = minDim >> reduceDim_;
		uint32 max = maxDim >> reduceDim_;

		// Handle the case where the largest texture dimension is still 
		// too large
		if (min > minDim_ && max > maxDim_)
		{
			uint32 div = max / maxDim_;
			if (div <= min)
			{
				min /= div;
				max = maxDim_;
			}
			else
			{
				min = 1;
				max /= min;
			}
		}
		// Handle the case where the smallest 
		// texture dimension is too small.
		if (min < minDim_)
		{
			max *= minDim_ / min;
			min = minDim_;
		}
		// work out the divisor 
		// for the dimensions
		divisor = minDim / min;
	}

	// and we are done, return 
	// the new width and height.
	width /= divisor;
	height /= divisor;
}

void TextureManager::init()
{
	pInstance_ = new TextureManager;
}

void TextureManager::fini()
{
	delete pInstance_;
	pInstance_ = NULL;
}

TextureManager* TextureManager::pInstance_ = NULL;

}
