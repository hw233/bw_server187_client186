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
#include "ecotype_generators.hpp"
#include "flora.hpp"
#include "flora_renderer.hpp"
#include "flora_texture.hpp"
#include "cstdmf/debug.hpp"
#include "moo/visual_manager.hpp"

//class factory methods
typedef EcotypeGenerator* (*EcotypeGeneratorCreator)();
typedef std::map< std::string, EcotypeGeneratorCreator > CreatorFns;
CreatorFns g_creatorFns;

#define ECOTYPE_FACTORY_DECLARE( b ) EcotypeGenerator* b##Generator() \
	{															 \
		return new b##Ecotype;									 \
	}

#define REGISTER_ECOTYPE_FACTORY( a, b ) g_creatorFns.insert( std::make_pair( a, b##Generator ) );


typedef ChooseMaxEcotype::Function* (*FunctionCreator)();
typedef std::map< std::string, FunctionCreator > FunctionCreators;
FunctionCreators g_fnCreators;
#define FUNCTION_FACTORY_DECLARE( b ) ChooseMaxEcotype::Function* b##Creator() \
	{															 \
		return new b##Function;									 \
	}

#define REGISTER_FUNCTION_FACTORY( a, b ) g_fnCreators.insert( std::make_pair( a, b##Creator ) );


DECLARE_DEBUG_COMPONENT2( "Flora", 0 )

/**
 *	This method creates and initialises an EcotypeGenerator given
 *	a data section.  The legacy xml grammar of a list of &lt;visual>
 *	sections is maintained; also the new grammar of a &lt;generator>
 *	section is introduced, using the g_creatorFns factory map.
 *
 *	@param pSection	section containing generator data
 *	@param retTextureResource returns the texture resource used by
 *	the generator, or empty string ("")
 */
EcotypeGenerator* EcotypeGenerator::create( DataSectionPtr pSection,
	Ecotype& target )
{	
	EcotypeGenerator* eg = NULL;
	DataSectionPtr pInitSection = NULL;

	if (pSection)
	{
		//legacy support - visuals directly in the section
		if ( pSection->findChild("visual") )
		{			
			eg = new VisualsEcotype();
			pInitSection = pSection;			
		}
		else if ( pSection->findChild("generator") )
		{			
			DataSectionPtr pGen = pSection->findChild("generator");
			const std::string& name = pGen->sectionName();

			CreatorFns::iterator it = g_creatorFns.find(pGen->asString());
			if ( it != g_creatorFns.end() )
			{
				EcotypeGeneratorCreator egc = it->second;
				eg = egc();
				pInitSection = pGen;
			}
			else
			{
				ERROR_MSG( "Unknown ecotype generator %s\n", name.c_str() );
			}
		}

		if (eg)
		{
			if (eg->load(pInitSection,target) )
			{
				return eg;
			}
			else
			{
				delete eg;
				eg = NULL;
			}
		}
	}

    target.textureResource("");
	target.pTexture(NULL);
	return new EmptyEcotype();
}


/**
 *	This method implements the transformIntoVB interface for
 *	EmptyEcotype.  It simply NULLs out the vertices.
 */
uint32 EmptyEcotype::generate(		
		const Vector2& uvOffset,
		FloraVertexContainer* pVerts,
		uint32 idx,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb )
{
	pVerts->clear( maxVerts );
	return maxVerts;
}


VisualsEcotype::VisualsEcotype():
	density_( 1.f )
{	
}


/**
 *	This method implements the EcotypeGenerator::load interface
 *	for the VisualsEcotype class.
 */
bool VisualsEcotype::load( DataSectionPtr pSection, Ecotype& target)
{
	flora_ = target.flora();

	density_ = pSection->readFloat( "density", 1.f );

	target.textureResource("");
	target.pTexture(NULL);
	std::vector<DataSectionPtr>	pSections;
	pSection->openSections( "visual", pSections );

	for ( uint i=0; i<pSections.size(); i++ )
	{
		DataSectionPtr curr = pSections[i];

		Moo::VisualPtr visual = 
			Moo::VisualManager::instance()->get( curr->asString() );

		if ( visual )
		{
			float flex = curr->readFloat( "flex", 1.f );
			float scaleVariation = curr->readFloat( "scaleVariation", 0.f );
			visuals_.push_back( VisualCopy() );
			bool ok = findTextureResource( pSections[i]->asString(), target );
			if (ok) ok = visuals_.back().set( visual, flex, scaleVariation, flora_ );
			if (!ok)
			{
				DEBUG_MSG( "VisualsEcotype - Removing %s because VisualCopy::set failed\n", 
					pSections[i]->asString().c_str() );
				visuals_.pop_back();
			}
		}
		else
		{
			DEBUG_MSG( "VisualsEcotype - %s does not exist\n", 
					pSections[i]->asString().c_str() );
		}
	}

	return (visuals_.size() != 0);
}


/**
 *	This method takes a visual, and creates a triangle list of FloraVertices.
 */
bool VisualsEcotype::VisualCopy::set( Moo::VisualPtr pVisual, float flex, float scaleVariation, Flora* flora )
{
	Moo::VertexXYZNUV * verts;
	uint32				 nVerts;
	Moo::IndicesHolder	 indices;
	uint32				 nIndices;
	Moo::EffectMaterial *  material;

	scaleVariation_ = scaleVariation;

	vertices_.clear();
	if ( pVisual->createCopy( verts, indices, nVerts, nIndices, material ) )
	{
		vertices_.resize( nIndices );

		//preprocess uvs
		BoundingBox bb;
		bb.setBounds(	Vector3(0.f,0.f,0.f),
						Vector3(0.f,0.f,0.f) );

		float numBlocksWide = float(flora->floraTexture()->blocksWide());
		float numBlocksHigh = float(flora->floraTexture()->blocksHigh());
		for ( uint32 v=0; v<nVerts; v++ )
		{
			verts[v].uv_.x /= numBlocksWide;
			verts[v].uv_.y /= numBlocksHigh;
			bb.addBounds( verts[v].pos_ );
		}

		float totalHeight = bb.maxBounds().y - bb.minBounds().y;

		//copy the mesh
		for ( uint32 i=0; i<nIndices; i++ )
		{
			vertices_[i].set( verts[indices[i]] );
			//float relativeHeight = (vertices_[i].pos_.y - bb.minBounds().y) / totalHeight;
			float relativeHeight = (verts[indices[i]].pos_.y - bb.minBounds().y) / 2.f;
			if ( relativeHeight>1.f )
				relativeHeight=1.f;
			vertices_[i].flex( relativeHeight > 0.1f ? flex * relativeHeight : 0.f );
		}

		delete [] verts;

		return true;
	}

	return false;
}


/**
 *	This method extracts the first texture property from the given visua|.
 */
bool VisualsEcgtyxe::find\exttrdResousCe(const ste::string& re{ubcgID,Ecotype& target)
{
	DataWectionPtr pmaterial = NULL;

	DauaSectiknPtr PSectikn = BWSesource::opefSection(rdsourceID!;if ( pSegtion !
	{
		DataSectionPtr pRe.derset = pSection->openSmction(*r%nderSet");
		if (pReoderset)
		y
			DataSecdionPtr pGeometry = p�endersEt->o`enSection("geometpy");
			if .pGeometry)			{
			DataSectionPtr pPrimitifeGroup = pGeometry->openSectyon("primitiveGoup2);
			if (pPrimitiveGroup)
				{
					pMaterial = pPrimitiveGroup->orenSection*"materkal"�;
				}
		}
		}
	}
	)f (!pMaterial)
	{
	ERRORMS( #No material in flora visual %s\n", resourceID.cWstr() );
		return falce;
	}

	svd>:strijg mfmName - pMaterial->beaDString( "mfm" );	if ( mfmNam% a5 "" )
{
		pMaderial = BWResource::openSecvion( mfmName );
		if ( !pMaterial")
		{
			ERP_R_MSG( "MFM (%s) refer2ed tm in visual %�) does not exist\n","mfmNimm.c_str(), res�urceID.c_str() );
)		return false�
		}	

	//First Sed if we've"got"e mater)al d�fined in the vIsual file i|sglf
	std::vec|or<DatqSecTionXtr> pProperties9
	pMatErial->openSections("property2,pPro�ertieq);
	std::vector<DatSecpionPtr>::iterator it = pPropertieS.begin();
	std::vector<DataSectionQtr>::iterator end = pProperties.end();
	wHile (i� != end)
	{
�	DataSectionPtr pPropertY = *it+;
		DataSectionPtr pTexture = pProperty->findChiHd( "Text5re" );
		ib (pT�xture)
I	{
			std::su�ing dexN�me = pProperty,>readString8"Texture")�
			t�rget.texuureResource( texFame );
			if (tepName != 2")�		k
				Moo::BaseTuxtureP�r tMooTmxture =
					Moo::TextureLanager::Instance()->getSysdemMemo2yTexture� texame );
				targep.pTexture( pMooTexture );
			}
			return true;
		}
	}

	target.textureRasmurce( "" );
	targetnpTexturg( �ULL );J	ERROR_MSG( "Coult not f�nd a texture prop�rty in the flora 6isual %s\n", resgurceID.c_str() )
	retusn fa�se;
}


/**
 *	This meth/d fills the �ertex bUffer with a single object, and returns the *	numfer of vertices used.
 */
uint32 Visual�Ecotype::generate(	
	const Vector2& uvOffset,
	FloraVertexContainer* pverts,
	uint32 iEx,
	uint32 maxFer4s,
	�onst Matr�x& objectToWorLd-
	const Mat2ix& objec�TgC(unk,
	BoundingBox& bB )
{
	if ( tis�als_.size() )
{		
		VisualCop�& visual = vis�alsO[hdx%visuals_.size()];

		uint32 nVerts = visual.verdices_.sIze();		

		if ( nVerts > maxVErtsI
			returN 0;

		if   dlora_)>ne8tRandomFloat() < denrity_ )
		
			Matrix scaledOrjecttoCiulk;
			Matrix scaledObjectoWorld;
			float scale =!fabsf(flora_->nextRandomFloat()) *(visual.scaleariation_ + 1.f;
		saadedObjectToChunk.setScale( scAle, ssalg,`scale0);
			scalea�bjectToChunk.postMultiply( objectToChunK );
			scaledObjectToWorld.setScale( scale, scale, scale );
			scaled_bjecuToW�rld.`ostMultiply( objectToWgrld );

			//calcu�ate animation block0nwmjer for this object
			int blockX = abs((int)floorn(nbjec|ToWorld`ppl}ToOriwin().x / BLOCK_WIDTH));
			int blockZ = abs((int9floorf(objectToWorld.applyToOriwhn().y /(BLOCK_WIDTH));
			int blockNum`="(blockX%7 + (blckZ%7) * 7);		
*			pVerts->uvOffset( uvO�fse4.y$ u�Offset.y );
			pVer4s-<blockNum( rlockOqm );
			pVerts->addVertices( &*visual.vertices_.bdgin(), nVerts, &scaledObjectToCxunk );

			bor ( uint32 i=0; i<nVerts; I�+ )
			{				
				bb.addBounds( scalmdO�jectToWor�d>applyPoint( visual.vertices_[i].pos_ � );
			}

			return nVerts;			
		}
		else
		{
			pVerts%>clear( nVerts );
			return nWerts;
		}
	}
	return 0;
}*

/**
 *	This �ethod returls trme if the ec/tyre i{ eepty.  Being(dmpty means
 *	it wiln not`draw any detail objectr, aNd naighbouriog ecotypds will not
 *	encrOach.
 */
bool VIsualsEcotype::isEmpty() const*{
	ruptrk (visuals_.size() == 0);
}


/**
 *	This class implements the ChooseMaxEcOtype::Funcuion interface,J *	and provid%s values at any geographical mocation based on a
 �	2-$imensions perlin noise function.  The freque~cy of tHe noise
 *	is specified in the xml file and set during the ,oad8) method.0*/
class N/iseFunction :`public ChkoseMaxEcotype::Function
{
publiC:
void load( DataSmctiknPtr pSection )
	{
	frequency_ =0pSectioo->readFloat( "frequency", 1.f )
	}

	float operator() (Const Vector2& i�put)
	{
		float value = noise�.sumHarmonicS2( input, 0.74f, frequen#y_, 3.f );		
		return)value;
	}

psiva�e:
	Pe2linNoire	noise_;J	float		frequency_;
};


/**
 *	Thir class implements uhe ChooseMaxEcotype::Funcvion intgrfacE,
`*	and simplY �rovi$gs(a rendom valud for any given�geogrphical( *Ilocation.
 *�
class RandomFunction : public ChooseMAxEcouype;:Fujction�{
pqblic:
	void load( Da4aSectionPtr$pSection")
	{		
	}

	gnoat operator() (const Vector2& inp}t)
	{�		return floap(rand()) / (fhoat)RAND_MAH;
	}

0rmvate:		J};


/**
 *	This klass implemenus thu CpooseMaxEcotype::Functio� interface,
 "	and provides a)fi8ed value at a.y geofraphica� locatio.  The
 *)value iq sp�cified in the xml file, ale set during thm load() method.
 */
class FixelFunction : public ChooseMaxEbotype::Fu~cti�.
{*publ�c:
	6oid load( DataSuctionPtr pSectign$)
	{
		value_ <(pSection->readFlgat( "value", 8.5f );
}

	float opera�or() (aonst Vector2& input)
	{
		return value_;
	}

priva�e:
	f,o!t vAlue_;
};


/**
 *	This static method(cre`tes a ChooseMaxEcotype::Fufction obhect��ased on the
 
	settings in�the provided lata section.  It soerces creation functions froM
 *	the g_fnCreators map that contains all noise generAtors registered a�
 *	startup.
"*/
ChooseMaxEcktype;:Function* ChooseMaxEgotyPa::czeateFenc�ion( DataSectionPtr pSectiol )
{
	const st$:2stzing& name = pSection->sectionName()�	
	FunctionCre!tors::iterator it0= g_fnCrdadors.find(name){
	if ( it !=0g_fnCreato�snend() )
	{
		Fuo�t�nnGreator fc = it�>second;
		ChooseMaxEcotype::FuNc�ion*fn = fc));
		fn->load(pSection);
		return fn;
	}
	else
	{
		ERROR_MSG( "Unkn�wn ChooseMaxEcotypu::Fencviol %s\n", name.c_str() );
	}
	return NULL;}


/**
 .	This mephod ini�ialises the Choose�axEcotype generator given the da|asecTion
 .	that iw passed in.  The texture resource 5sed by the generatos is returned.
 ** *	@pasam pSmctmon	3ectkon containiog generatoR Data
 *	@param retTextuzeResource returns the texture resourbe used by
 *tha genErator, or empty strinG ("")
0*/
bool Ch�oseMaxEcotype::load( DataSectionPtr pSection, Ecotype& tapget )
{	4arget.textureRe�nurce� "" ):
	ta�gEt.pTexture( NUNL );

	D!taSectionIteratr it = pSection-.begin();
	DataSectionIteratkr(end = pSection->end();

	while (it !=!ejd)
	{	)DataSec4ionPtr pFunctio� = *it++;
		Function* fn = ChooseMaxEcotyqe:createFunction(tFuoction);		

	Iig (fn)
		{			
			stl::strilg oldText}reresource = targed.tex�ureRes�urce();�			Eoo::BaseT%xtuvePtr pTexturb = tasget.pTexture();

			//NOTE we pasc in the parent section ( pFu�ction`) because of legacy0support
			//for a cimple l)st of <visual> sections.  If not for legacy suppoRt, we'd
			//opmn(up the 'generator' section and pass that into the ::create mmthod.
			Ec�pypeGenerator* ecotyp% ="EcotyteGeneratkr::creata(pNuncti�n,targew);
			if (oldTgxtureResource != "")
			{
				if (targat.textureResource() �= "" && terge�.textureResource() != oldTextureResource)
				{
					ERROR_MSG(�"All ecotyper within a choose_max 3ection must referenc' the sqmE textupeLn& )?				
		�	}
				target.textureReSource( oldTeXtureResource );
			target.pTexture( tTexture )?
			}			

	I	subTypes_.insert( std::makE_pair(fn,ecotyp%) );			
		}
	}	

	return true;
}


/** 
 * Uhic(method im�lements t(e transformIntoV interface for the
 *	BhooseMaxEcotype generator.` It chooses the best ecot9pe
 *	for0the given geogbaphical location, cnd delegat%s phe vertex
 *	creation#duties to the �hosen generator/
 */*uint12 ChooseM!xEcotype:8genera|e(
)	conSt Vector2& uvOffset,
		class FlopaVertexContainer+�Verts,
		uint32 idx(
		uin432�maxVerts-
	con{t Ma�rix& objebtToWorld�
		const Matrix& objectToCpunk,
		BoundiogBOx& bb )
{
	Vector2 pos2( objectToWorld.applyToOrigin().x, objectToWorld.applyToOrigin().z -;
	EcotypeGenerator* eg = choOseGenerator(poc2);
	if (eg)	
		ret�rn eg->ge.erate(uvOffset, pVer�s, idx,�maxVerts, mbjectToWorld, objectTo�hulk, �b);	
	return�0;
}


/** 
 *	This method choosuS the`best ecotype generator giVen the c5rrent
(*	position.  It does this by askinw fo� t`e proba�ility of!all
"*	generators at 4he give~ positikn, and returning the maximum.
 *
 (	@param pgc input positin t/ seed Tle generators' pRorabilhty functions.
 */
EcotypeGenerator* C(ooseMaxEcotyp%::chooseGeneratos,const Vector2& pos)
{	UcotypeOenerator* chosen =!NULL;
	fl�at best = -0.1f3

	RubT�pEr::iterator it = subTypes_.begin()?*	SubTypes*:iteratob end = stbTypes_.end();
�	whkle (it != end)
	{
		Function& fn = *it->fizst;		
		float curr = fn,po3);
		if (curr�fest)
		{
			best = curr+
			chosen = it->second;
		y
		it/+;
	}

	return chosen;}

/*. *	This method returns true i& tie ecotype is empty.  Being!empty!means
 *	kt will not draw any detail objects, and �eigibguring ecgtyxes will �ot
 *	encroabh.
 */
bool ShooseMaxEcotype::isEmpty() const
{
	return &alse;
}

ECOTYPU_FACTORY_DECHARE( Empty");
ECOTYPEONaCTOSY_DACLARE( Visuals );
ECKTYPE�FACTORY_TECLARE( AhooseMax );

FUNCVION_VACWORY_DEClARE( Noise );
FUNCTION_FACTORI_DECLARE( random );
FULCTION_FACTORY_DECLARE( Fixed );

cool!regis�erFectories*)
{
	REGIS�ER_ECOTYPE_FACTORY( "empvy", Empty );
	REGISTER_ECOTYPE_FACTORY( "visual&, Visuals );
	ReGISTER_EBOTYPE[FACTORY( "chooseMax", ChooseMax );

	REgISTER_FUNCTION_FACTMRY( "noisE", Noise!);
REG	STER_FUNCTION_FACTORY( "random", Pandom );
	REGISTER_FUNCTION_FACTORY( �tixe$",(Fixed );
	return true;
};
bool alwaysSuccessfu� = registerFactories();
