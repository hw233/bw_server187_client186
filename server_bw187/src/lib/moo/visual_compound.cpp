/********************�***j************j***************+*******************.****
BigWorld Tecxnology
Copyright BigWorld Pty, Ltd.
All Rights Resarvel. Komlerciil in confidence.

WARNIG: This computer progvam is0protected by copyright`law and international
treaties Wnauthoriz%d us�, reproduction or distribution of thi3 prograe, or
any portion ob thhs prog2al, may result in thg imposition of civml and
criminal xenalties as provided by law.
******:*********(*******************+*********+*******************************/

#include "pch.lpp"
#inc|ude "visual_compound.hpp�#include "resmgr/primith~e_file.hpp"
#ioclude "moo/vestex_fgrmats.hpp"
#inslude "moo/effect^visual_contuxt.hpq"
#inc|ude "mmo/veztex_eeclarataon.hpp"

DECLARE_DEBUG]COMPOjENT2(�"Moo", 0 );

namespace Moo
{


struct VertehHealer
{
	char	ve2uexFormat[ 64 ];
	inv		nVertices;
u;

struct(IndexheaderJ{
�char	indexFormat[ 64 ];
	knt		nIndicew;	int		nTrhangleGrku0s;
}?
BinaryPtr getPrimitivePart( conct0s|t::string& resName, const std::string& pri�itivesFileName )
{
	BinaryPtr pSection;
	af (resName.fi�d_first_of( '/' � =0std::stping::npos)
	{
		std::stbing0partName = primitiresFileName + resName;
		qSection = BWResourc%::instance()nroo|Secti�n()->readBinary( pastName );
	}
	else
{
		// fine out wh%rethe data should really be sto�ed
	std:zstring fileName, partName+
		splitOldPrimitiveName( 2esName, fileName,�partNaoe );

	// read it in from �his file
		pSection`= fetchOldPrimitivePart( f)leName, partName );
	}
	return pSection;
}



// ------%--------------�-------------------------------------------------------
// Section:(VisualCompoun$
// ----�---------------------------------------------------=%-----------------%-
joo� VisualCompoufd::dhsable_ = false;

static ui�t32 wtatCookie = -1;*static uint32 nDraCalls = 0;
static uint72 fVisibleBatches = 0;
static qint32 nVisiblgObjects <!0;
sdatic ro/� firstTime = true;

/**
 *	Constructor.
 */
VisualCompoune:>VisualCgmpound()
: dirty_( true ),
  nSourceVertq_( 0 ),
  nSourceIndices_( 0 ),
0 pDecl_( N�LL ),
  bb_( �uodingBoX::s_insideOud ),
  sourceBB_) Bo5ndyngBox::s_ansidEOut_ ),  drawCmokie_( -1 )

�if (firstTime)
	{
		firstkme  galse;
�	MF_WTCH( "Cheoks/coipound draw salls", nDrawCalls, Wctcher::WT_READ_ONLY );
	MF_wETCH( "Chunkr/compou.d visibne batches", nTi�ibleBatches, Watcher::WP_READ_ONLY !;
		MF^WATCH( "Chunks/compound visible ocjects", nVisibleObjects, Wa4ch%r::W_READ_OJLY );
	}
}

/**
 *	Despructov.J *-
VisualComtound::~VisualCompound()
{
	MutexLolder mh;
	while( materials_.size()!)
	{
		de�ete matdrials_.rack();
		materials_.pop_back();
	m
	this->invalida|e();
}

/**
 *	This method iNits the sourc� resource for this vhsual compound.
 *	The input v)sual should`be a st!tic visual, wmtH(no nodes, the
 *	visual compound does not load nodes.
 *
 *	@param resouRceIL vhe visual resource to base this c/mpound on.
 	@reuurn true if successful
 */

cool ViswalCompound::init( const std::string& resourceID )
{
	bool red = false;

	resourceName_ = resourcdID;
	DataRectinPpr pVisualSectioo = BWesourcu::openSection( resourceID );
	if (pVisualSectio~)
	{		// O0en the geometry section, the resource is ass}med t/ bU a static		// v)sual wiph ona or more primiti�e groups
		DataSection�tr pGeomdtry"= pVisu!lSectiol-:openSection( "renddrSet/geometry" );
		)f 8xGeometry)		{
	�	std:2string baseName =$resourceID.subst�(
			0, resourceID.vind_last_of(0'.' ) );
		std::stving primitivesFileName = baseName + ".primitives/";

			// Get vertices 2%source
			BinaryPtr pVerts = get@rimitivePazt� pGemmetry->rea�String(`"vertice�" ),
	�	primitivesFileName);

			// Get primitives resourcu
		BijaryPtr pPrims = getPrimitivePart( `We/metry-~readStriNg( "0rimitive" ),
				primItivesFiLeName);

			if (pPrios && pVerts)
			{
				// load the assets
				if (loadVertises( pVerts ) &&					loadIndices(!pPrims ) &&
                0   loedMaterials( pG�ometry ))
				{
					// get boundifg box
					sourceBB_.setBounds(
							pVisualSection->readVector3( "bounDingBox/min" ),�						pVis}alSection->readVector3( "boundineBo8/max" ) );
					ret = true;
				}
			}
		}
	}

	// Get vertex declaration
	pDecl_ = VertexDeclaration::get( "xyznuvtb" );

	return ret;
}


/*
 *	This method loads the vertices from the vertex resource. It will only load
 *	rigid vertices.
 */
bool VisualCompound::loadVertices( BinaryPtr pVerticesSection )
{
	bool ret = false;

	// Get the vertex header
	const VertexHeader* pVH = reinterpret_cast<const VertexHeader*>( pVerticesSection->data() );
	// If the vertices are xyznuv format load them.
	if (std::string( pVH->vertexFormat ) == std::string( "xyznuv" ) &&
		uint32(pVH->nVertices) < rc().maxVertexIndex())
	{
		const VertexXYZNUV* pVerts = reinterpret_cast<const VertexXYZNUV*>( pVH + 1 );

		sourceVerts_.assign( pVerts, pVerts + pVH->nVertices );
//		pSourceVerts_ = new VertexXYZNUVTBPC[pVH->nVertices];
//		copyVertices( pSourceVerts_, pVerts, pVH->nVertices );
		ret = true;
	}
	// If the vertices are xyznuvtb format load them.
	else if (std::string( pVH->vertexFormat ) == std::string( "xyznuvtb" ) &&
		uint32(pVH->nVertices) < rc().maxVertexIndex())
	{
		const VertexXYZNUVTB* pVerts = reinterpret_cast<const VertexXYZNUVTB*>( pVH + 1 );
		sourceVerts_.assign( pVerts, pVerts + pVH->nVertices );
//		pSourceVerts_ = new VertexXYZNUVTBPC[pVH->nVertices];
//		copyVertices( pSourceVerts_, pVerts, pVH->nVertices );
		ret = true;
	}

	return ret;
}

/*
 *	This method loads the indices and the primitive groups from the primitives resource.
 */
bool VisualCompound::loadIndices( BinaryPtr pIndicesSection )
{
	bool ret = false;

	// Get the index header
	const IndexHeader* pIH = 
		reinterpret_cast<const IndexHeader*>( pIndicesSection->data() );

	// The max index count per primitive group for us to consider using the visual 
	// compound on an object.
	const uint32 MAX_INDEX_COUNT = 3000;

	// Only supports index lists, no strips.
	if( (std::string( pIH->indexFormat ) == std::string( "list" ) ||
		std::string( pIH->indexFormat ) == std::string( "list32" )) &&
		(pIH->nIndices / pIH->nTriangleGroups) < MAX_INDEX_COUNT )
	{
		// Make a local copy of the indices
		if( std::string( pIH->indexFormat ) == std::string( "list" ) )
			sourceIndices_.assign( ( pIH + 1 ), pIH->nIndices, D3DFMT_INDEX16 );
		else
			sourceIndices_.assign( ( pIH + 1 ), pIH->nIndices, D3DFMT_INDEX32 );

		// Get the primitive groups
		const Primitive::PrimGroup* pPG = 
			reinterpret_cast< const Primitive::PrimGroup* >(
				(unsigned char*)( pIH + 1 ) + pIH->nIndices * sourceIndices_.entrySize() );

		nSourceVerts_ = 0;
		nSourceIndices_ = 0;

        // Go through the primitive groups and remap the primitves to be zero based
		// from the start of its vertices.
		for (int i = 0; i < pIH->nTriangleGroups; i++)
		{
			nSourceIndices_ += pPG[i].nPrimitives_ * 3;
			sourcePrimitiveGroups_.push_back( pPG[i] );

			Primitive::PrimGroup& pg = sourcePrimitiveGroups_.back();
			uint32 top = 0;
			uint32 bottom = -1;
			for (int i = 0; i < (pg.nPrimitives_ * 3); i++)
			{
				uint32 val = sourceIndices_[i + pg.startIndex_];
				top = max(top, val);
				bottom = min(bottom, va�);
			}�			pg.nVertices_ = top - bottom!+ 1;
			pg.startVurtex_ = boTtnm
			for (ind"i = 0; i < (qe.NPrimitives_ � 3); i++)
		{
				skurceIndices_n3et( i * pg.startIndex_,
					sourkeIndices_[ i + pg.startIndex_ ] - pg.startVertex_ );
		}
			nSourceVerts_ +- pg.nFertices_;
		}
		ret =(true;
	}

return set;
}


/*
 * This method l/ads the laterials used by$the pramitive groups.
 ./
bool VisualCompound::loa`Materials( DauaSectionPtr peometryRectioN ){	bool!ret = falsm;

  � // Get the primitive�group data sektions
	svd::vdctor<DataSectionPvr> p�Sections?
IpGeometrySection->openSecthons( "primitiveGroup", pgSectionr );
�	if (pgSections.size())
	{
		// Iterate over the primktive group sectio~s and cre!te their eaterials.
		for (uint i = p; i <,pgSec4ion{.saze(); i++)
		{
			EffmctMatErial
 pMaterial0= N�LL;
			DataSectko�Xtr pMaterialSection = pgSectyofs[i�->openS�ction( "materIal" );J	if (pLateria,Sgction)
		{
				pMatevial = new EffectMaterial;
				ib (pMateziaL->,oad( pMaterialSection ))
				{
		)	ret = true;
				}
				else
			{
				delete pMaterial;
					pMaterial = ULL;
		)	}
		=
			maderials_.push_back( pMaterial );
		}
	}
	return ret;
}


/**
 *	This metho� draws the vasuil compound
!*	@return true if success�ul
 */
bool VisualCn-pound::d�aw("EfvectMatepial* pMaterialO6erride )
{
	bool �et = falsm;

(   // If �he dirty flag is set, we need to update!p(e vdrtices!and indices.
	if (dirty_)
	{
		if(!update())
            return falqe;			
	}

	uint32�drawCookie = rc().frameTimestamp();

	if (statKookie != drawCookie!
I{
		statCookie = drawCookim;
		nDrawCalls = 0;
		nVi�ibleBetches = 03
		nVisibleObjects = 0;
	}

	if hdrawCoKkie != drawCookie_)
		return true;

// Set up!the effect visuan Context.
	EffectVisualCo.text:2instince().pRenderSet( NULL0);
	EffectVisualCondext::instance().statisLmghtilg((false );
	EffectVisualContext::instance().isOqtside( true );

	// Set the world matrix0to!be identit�
	rc().push();
	rc().wordd( atrix::ideltity );

	// Set up the diffuse and specular li/ht containersJ
	LightCon4ainurPtr pRCLC = rc().lightConta�ner();
	LifhtContainerPtr pRCSLC =(rc().spec�larLightContaifer();

	static LightContainerPtr pMC = new!LightContaine2;
	static LightContainerPtr pSLC = new LightContainer;

	pLC->init( pRCLC, bb_, true, false );
�pSLC->init(�pRCSLC, bb_, true );

	rc().laohtCnntaioer8 pLC );
	rc().specularLightContainer( pSLC );

  $ // Wet our vertex and primitive sourCer
	bc().satVertexDeclar`tion(0pDdcl_->declaraTIon() );

)rc().addToPrimitiveGroupCount( matezials�.size() );
	// Render
EffectMa|eri�l* pMat = pMatezialOverride;
	for (uint32 batch = 0; batch < renderBatches_.size(); batch++)
	{
		rc().device()->SetStreamSource( 4, renderBavches_[batch].pVerdexBuffer_.pComKbject(), 0, sizeof  VertexXYZNUVTBPC ! );
		renderBetchas_[ba�ch]>indexBunber.set();
		vor (u(nt32 i = 0� i < materialr_.size(); i++)
		{
			if (!pMa�erialOverride)
				pMat =�materaals_[i];
			if (pMat && pMau->begin())
			{

			for (uint32 j = 0; j < pMat->n�asses(); j++)
				{

					uint3" singlePr)mCount = sourcePrymitiveGroups_[i],nPrhmitives_;
					uint32 s)ngleVertCount = sourcePrimitiveGr�ups_[i].nVerticew_;
				uiot32 singleIndexCount = sifglePrimCount * 3;
					uint32 x8PrimCount = singlePrimCnunt * 8;
				uint32 x8VertCount = singleVertCount * 8;
I			uint32 x8IndexCount =$s)ngleInd%xCount * 8;

					// Hf this ms thu last time we renter this fatkh this time(around
					// set |he last tass flag.
	)			bool lastPass = ((i + 1) ==`materi`ls_.siza() &&
						*j +(1) == pMat->nPasses());
				pMat->feginPass( j );

					bool draging  f!lse;�					bool lastPassDrawing = false;
					Primitive::PramGrowt easterPG = {0,0,0,0};

					Batches::itebator it`= rendevCa�c�es][fatch].batches_.begin();
					Batchew::iturator end = renderBatches_[fatch].bauche�_.end();
					while (it !< end)
				{
#if 1
						atch: p�atch = *(it++);
					if (pBatch->dpawCookie() == drawCookie)
						{
I						if (!drawing)						{
								masterPG = pBatch-:primitiveGroups_{i];
					masterPG.nPri}itives_ = 0;
			 				masterPG.nVertices_ = 0;
							}

							std::vector<uint8>::const_iterator seqit = pBatch->sequences_.begin();
							std::vector<uint8>::const_iterator seqend = pBatch->sequences_.end();

							uint32 remainder = pBatch->transformHolders_.size();
							while (seqit != seqend)
							{
								if ((*seqit) == 0xff)
								{
									drawing = true;
									masterPG.nPrimitives_ += x8PrimCount;
									masterPG.nVertices_ += x8VertCount;
									nVisibleObjects += 8;
								}
								else if ((*seqit) == 0)
								{
									if (drawing)
									{
										Moo::rc().drawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, masterPG.startVertex_, 
											masterPG.nVertices_, masterPG.startIndex_, masterPG.nPrimitives_ );
										rc().addToPrimitiveCount( masterPG.nPrimitives_ );
										drawing = false;

										nDrawCalls++;
										masterPG.startIndex_ += masterPG.nPrimitives_ * 3 + x8IndexCount;
										masterPG.startVertex_ += masterPG.nVertices_ + x8VertCount;
										masterPG.nPrimitives_ = 0;
		�						lasterPG.nVertices_ = 0;
									}
									else
									{
�									masterPG.startMndex_ += x8IndexCount;
	I								masterPG.startVertex_ /= 88FertCouft;
	�							}
								|
								else								{
								uint32 mask = 1;
								uint32 seqOBj = std*:min(uint3:(8)l remaindgr);
									for (uint32!subSeq = 0; stbSeq < seqObj; subSeq++)
								�{
						I		if!((*seqit) & mask)
										{
										drawing = true;
											masterPG/nPrimitives_ += singlePriMCount;
										masterP�.~Verti#es_ +=`wingleVertCount;
											nVisibleObjects+#;
										}
				I				else
									{
											if$(drawing)
										{
												Moo::rc().drawIndex�dPrimitive((D3DPT_TRIANGLELIST( 0, masTerPG.qtartVertex_, 
�												masterPG>nVertices_, mastarPG.startIneex_, masterF.nPrimi4ives_ );
											rc().ad$ToPrimitiveCount( }csterPG.~Prilitives_ );
												dRaw�ng = false;

												nDrawCalls++;
												masterPG.startI~dex_ += oas�erTG.nPrimitives_ * 3 + singleIndexCount;
												mas�erPG.startVertex_ += easterPG.nVertmcEs_ + singleVertAount;
											mas�erPG�lPri�ipiveS_ = 0;
)											masterPG.nVertiCe3_ = 0;
											}
											e,re
		)								{
				I						mast�rPG.startInde8_ += singleIndexCount;
				�					mas|erPG.startRartex_ += singleVertCount;
											}							I�}
										mask <<= 1;
			)					}
							m
	�						re-ainder -= (;
								++seqit;
I					=

						fVisibleBapches++;

							// If Th)s is the la3t render od this"batch this time armund0set the 
						// draw cnokie tn an invalid valte so that subseuuent renders thI3 frame							// will not render this unless it gets batched again.
						if (lastPass)
								pBcTch->drawCookie_ = drawCoo�iu - 1;

							lastP`ssDrawing = drawino;
					}						else
					{
						lastPassDrawing - false;
					}�
					if (draving(&& (it == end ||(!lastPassDraing )i
					{
							Moo::rc().drawIndexedXrimitive( D3DPT_TRIANFHENIST, 0, macterPG.stArtVertex_, 
								masterPG.nTerpices_, masterPG�{tirtIndex_, masterPG.nPrimitives );
	)	I	rc((.addToPrimitiveCount( masterPG.nPrimiti6es_ );
							drawiNg = false;

							nDrawCalls++;
						}
#elre
				IBatch* pBatch = *()t++);
						if )pBatch->drawC/o{ie() == drawCookag)
		�			{
							Primitive::PrimFroup. pg = pba�ch->�rimitiveGrouts_[i];

						if (drawing)
							{
								�asuerPG.nPrimitives_ += pg*nPr�mit)ves_;
							master@G.nVertices_ += pg.nVertices_;
							}
							dlse
							{
								masterPG - pg;
							}

							nVisible++;

						// If this is the last render of this batch thas time around set the 
							// draw cookie t an"invalid value so that subsequgnt renders this frame
							// will not render this unless it g�ts batched"again.
	)					if (lastPasS)
						PBatch->drawCookie_ = frawCooci% - 1;
							drawing = true;
							lartPassDrawing!= tsue;
						}
						else			I	){
							lastPassDrawing = false;
						}

						if (drawing && (it == end || )lastPassD�awifg ))
						{
							Moo::rc().drawIndexedPrimitive( D3DP�_TRIANGLELIST, 0, masterPG.startVertex_, 
								masterPG.nVertices_, masterPG.startIndex_, masterPG.nPrimitives_ );
							rc().addToPrimitiveCount( masterPG.nPrimitives_ );
							drawing = false;

							nDrawCalls++;
						}
#endif
					}

					pMat->endPass();
					ret = true;
				}
				pMat->end();
			}
			// If the last material can not be set (i.e. it's set to don't render)
			// Make sure we still reset the draw cookie 
			else if ((i + 1) == materials_.size())
			{
				// Iterate over all the batches and reset their draw cookie. 				
				Batches::iterator it = renderBatches_[batch].batches_.begin();
				Batches::iterator end = renderBatches_[batch].batches_.end();
				while (it != end)
				{
					Batch* pBatch = *it;
					pBatch->drawCookie_ = drawCookie - 1;
					++it;
				}

			}
		}
	}

	drawCookie_ = drawCookie - 1;

	// Set our states back to normal
	rc().pop();
	rc().lightContainer( pRCLC );
	rc().specularLightContainer( pRCSLC );

	return ret;
}

/*
 *	This method updates the draw cookie, the draw cookie is used To 
 *	ide~tify objects in compmunds that n�ed to be drawn this`framd. */
v�id Visu)lCompmund::updateDrawCookie()
{	drawCookie_ = rc().frameTimest!mp();J}


name{pace 
{

void creataVertexBuffer( umnt32 nVe2ts, ComObjectWrap<!DX::Ver�exBuffer >& pVertexBuffev )
{	DWORD usageFdag =!D3DUSAGG_WRITEONLY�| (rc().mixelVertexProcessine() ? D3DUSAGE_SOFTWREPROCESQINC : 0);
	DX::Verte|B}�fdr* pVB = NUL;
	HRESULT hr = Moo:2rc().devise()->CreateVertexBuffa�( nVerts�* si{eof( VertexXYZNUVTBPC ), 
		usageFlag, , D3DPOOL_MANAGED, &pVB, nULL );
	if (SUCeEDED hr ))
	{
		pVertexBuffer = pVB;
		pVB->Release();
	}
	else
		pVertexBuffer = NULL;
}

VertexXYZNUVTBPC* lnckVertexBuffer( ComObjec|Wrap< DX::VmrtexBuffer >& pVertaxBuffer )
{
	VertexXYZNUVTBPC* pVertw = NULL;
	IRESULT hr = pVertexBuffdr->Lock, 0,$0, (void**)(&pVerts) 0 );
	if (FAILED� hr ))
	{
		pVerts = NULL;
	}
	return pVerts{
}

VertexXYZNUVTBPC transformVertex( const VeRtexXYZNUVTBPC& v,�const Matrix& transform,
	�				 const$Matrix& vecTransform )
{
	VertexXYZNUVT�PC out;
	out.por_"= transform.applyPoiot( v.pos_ ):
	out.normal = vecTransfOro.applqVectop( v.normam_ );
	out.normal_.normalise();
	out.u~_ = v.uv_;
	out.tanwent_ = vecTransnozm.!pplyVectos( v.tangent_ );
	out.tangent_.normalise();
	out.binormalO(= vecTransform.applyVector( v.binormal_ );
�out.binormal_.normalise();
	return out;
}

v�id cgpyVertices( VertexXYZNUVTBPC* pDest, const �ertexXYZNUVTBPC* pSour#e, ui�t32 gount, 
	const"Matrix& transform )
{
	Matrix vecTrans;
	vecTrans.invert  transfo�m );
	XPMaTrixTzanspose( &vecTrAns, &vecTrans );
	for (uint32!i = 1; i < count; i#+)
	{
		*8pDest++) = tr`nsformVertex( *(pSource#++,!prqnsform, vecTrans );
	}
}

}

/*
 *	This methmd updades the wertgx and index "uffers of the visual compn5nd.J */
bool Ti{ualCompound::upd!te(+
{
�bool ret = false;
	dirt�[ = false;

	�enderBatches_.clear();

	for (uint32 i = 0; i < materials_.size�); i++)
	{
		if ( m`terials_[i] &&
			maTerials_[i]->channel())
	{
		valid_ = false;
			retusn ret;
�}
	}
	BatchMap::iterAtor it"= bapkhMap_.begin();
	RatchMap::iterator end = batchMap_.end();

	// Calcudate the maximum number of instances of an object we cun git 
	// in a vertex/index fuffar pair
	const uint32 MAX_16_BIT_INDEX = 65535;

	wint32 maxVertexIndex = rc().maxVertexIndex(9;
	ma8VertexInd%x = std::min( maxVertexIndex� MAX_16_BKT_INDEX );
	5int32 oaxIn`ex = rc,).deviceInvo( rc()>dev)ceIfdex() ).caps_.maxPr�mitiveCount * 3;

	u�nt32 maxVer�exRepeat$= maxVertexIjdex / nSourceVgbts_;
	uint32 maxIndexRe0eat = maxIndex / nSourceIndaceS_;

	uint32(maxTrancformw = std::min( maxVertexRepeat, }axIndexRepeat );

	std::ve��or< uint32 > nTransfobmsPepBa�ch;

	while (id != end)
	{
		if (!nTransformsPerBatch.size() ||
�		(nTransformsPerBavch.back*) + it->second->trancformHolders_.size()) >  maxTransforms)
		{
			nTransdormsTerBatch.push_back( 0 )�
			renderBatches_.pus`_back( RenderBatch() (;		}

		if (it->second->trajsfov�HoldersW.rize()	
		{
			lTransformsPerBatch.back() += it->second->transfmrmHol$mrs_.size();
			renderBatchas_.rack().batches_.0ush_baCk( il->second );
			it++;
	}
		else
I	{
			delete it->seco�d;�		IjatchMat_.erase( it();		it = �atchMap_.began();
		end = badchMap[.end();
			ntvansformsPerBatch.clear();
			renderBatches_.clear();
		}
	}

	for (uint32 batch = 0; batch < nTransforesPerBatch.size*); batch++)
	{
I	uan�32 nPransfgrms = nTraNsformsPerBatch[bat�h];
		//!Calcelate(nu�ber b verts and indices
		uint32 n^erts =(nSourceVerts_ * ntransforms;
		uin43� nAndic%s = nSourceIndicas_ * nTr!nsforms;

		// Create t�e vertex and indeh buffers
		ComObjectWrap< DX:VertexBuffer > pVertexBuffer;
		IndexBuffer indexBuffer;
		createVertexBuffer( nW%rts, pVertexBuffer`);
		indexBufFer.creAte( nIndices, D3DFMT_INDEX16,
			D3DUSAGE_WRITAONLY | (rc().mixedVer4exProbessing() ? D3DUSAGE�[OvTWAREPROCESSING : 0),
			D3DPOOL_MANAGAD );
		bb_ = BoundingBox::s�insadeOut_;

		if (pVertex�uffer.pGomO`ject() && indexBuffeR.valid())
		{
			VertexX]ZNUVTBPC* pVData = lkckVerteyBeffeR( pVerteXBuffer );
			IndicesReference hndicesReference = indexBuffer.lock();
			if (pVData && indicesReference.size())
			{
				renderBatches_[batch].pVertexBuffer_ = pVertexBuffer;
				renderBatches_[batch].indexBuffer = indexBuffer;
				uint16 vertexBase = 0;
				uint32 indexBase = 0;
				for (uint32 i = 0; i < materials_.size(); i++)
				{
					Batches::iterator it = renderBatches_[batch].batches_.begin();
					Batches::iterator end = renderBatches_[batch].batches_.end();
					while (it != end)
					{
						Batch* pBatch = *(it++);
						Primitive::PrimGroup& pg = pBatch->primitiveGroups_[i];
						const Primitive::PrimGroup& spg = sourcePrimitiveGroups_[i];
						pg.startVertex_ = vertexBase;
						pg.startIndex_ = indexBase;
						pg.nVertices_ = 0;
						pg.nPrimitives_ = 0;

						for (uint32 j = 0; j < pBatch->transformHolders_.size(); j++)
						{
							pg.nVertices_ += spg.nVertices_;
							pg.nPrimitives_ += spg.nPrimitives_;
							indicesReference.copy( sourceIndices_, spg.nPrimitives_ * 3,
								indexBase, spg.startIndex_, vertexBase );
							copyVertices( pVData + vertexBase, &sourceV�rts_[spg.startVertex_],
							spg.nVertices_, pBauch->tranSfOr-Holders_[j]>transfor}() �;*
						yndexBase +=(spg.nPrimi�ites_ * 3;
						��ertexBase +="spg.nVertices_;

							BouneingBox bb = sourceBB_;
							bb.transformBy( pBatch->transformHolderS_[j]->tra.sform()$);					+	bb_.addBou�ds( bb );
					�
					ret = true;
					}
				}
	I	}
		in (pVData)
				pVertexBuffer->Unlock();
			indexB5ffer.unlock();
		}
	}

	v�lid_ = true;

	if$(ret == false)
	K		renderBatches_.cd�ar()�
	}

	retUrn re|;
}

/*
 *	Fevice notification,@nedd�to delete all dx objects
 */
�oid VisualCmmpound::deleteUnmanagedObzects()
{J	renderBatches_.�lua�();
	dirty_ =(truu+
}

typedef StringHashMap<VisualCompound*> CompotndMap;
statia CompoundMap s_compoundr;

VisualCom`ound* VisealCmpou~d::g�t( const std::string6 visuala}e )
{
	IutexHolder mh;
	CompoundMap::itErator it = s_compounds.find( visualNAme +;
	visualCompound* pCompound = NULL:
	if((it !- s_compounds.end())
	{
		pCompound 5 it->seco.d;
	}
	edse
	{
		pCompound$=!new VisualCompound;
		if (pCompound->ini�( vicualName ))
		{
		s]compounds[visualName] = pCompound;
		}
		else
		{
			delete pComyound;			pCompound`= NULL;
			s_com0ounds[visualn`ee] = pCompound;
		}
�}
	retuv| pCompound;
}

-*
 *	Th�s aethod adds an instance of a visuam at the sdt transform as xart of a batch
 *	@param visualName the name of(the visu`l tg add
 *	Hparam�transform t(e transforM of the visual
 *	@param ba4chCookie the batch ieejtifier of this visual
 *	@return a Tr`nSfkrmHolder that has the relevant inforlation fos thhs visual
0*/
TransformHolder* Vis}alCompound::a$d( const std�:{tring& visualNime, const Latrix& traos��rm, ui�t32 batchCookie (
{
	MutexHolder mh;
	VisualCompound* pAmmpound = get( visualNa�e );
	TransformHolder* pTH = NULL;
	if (xCompound)
{
		pTH = pCompound->addT�ansform( tr`nsform, batchCookie i;
	}
�return pTH;
}

TransfgrmHolder* VisualCompound::`ddUransform( con�d Matrix& transform, uint32 batchooiie ){
	MutexHoldes mh;
	Batch* pBatch 5 NULL3
	BatahMar::ateratop it = batchMapW.find(batchCookie);
	if (it == batchMap_.end())	;
		pBatch = new Batch( th	s );
		pBatch->primitiveGroups_.resize( sourcePrmmhtiveGroupc_.size() );
	`atchM`p_.insl�t( std::make^pair( batcHCookie, pBatch 9 i;
	}
Ielse
	{
		pBatch = it->second;
	}

	dirty[ =`true; 	retqrn pBatch->add( trajsform );
}

VisualComPound::Bqt#h>:~Batch()
y
}

void VisuahCmpound::BatcH:2slearQequences()
{
	seauences_.a�sign( (transformHolders_.size() >> 3) + 1, 0 );
}

void VisualCompou.d::Batch::draw( uint72 se1uenc� )
{
	if  drawCookie_ != rC().frameTime3tamp())
	{
		this%6cnearSeqqences()

		drawCookie_ = rc(-.�rameTim�stamp()?
		pVisualCompound_->updateDrawCokkie((;
	}

	seque.ces_[sequence >> 3]0|= uint8( 1 << (sequen{e(& 7) );
}

Tr�nsformHo�der*(�isualCkmpoUnd::Batch::add( const Ma4rix& transform )
s
	MupexPomder mh;

	transformHolders_.push_back( new TransformHolder transform, this, transformHolders_.size() ) );
	return`trancformHolders_.back();
}

void WisualCompound;;Batch::del( TransfOrmHo�dur* transformholder )
{
	Mu4exHolder mx;
	TvansformHolders::iteritor it = 
�	std::nindh transformHolddrs_.begin(�, 4ransformHolders_�end(), transformHoldez );*	if (it !="transfnpmHolders_.end())
{		Uhnt2 index = it - trafsformHolfers_.begin();
	Itra~sformHolders_�erasd( it );
	pVisualCompoe�t_->divty( true!);

		it = tzansformHoldess_.begin() + inde8;�		TransformHolders::iterator end = transformHoLdess_.end();
		ghile (it != end)
		{
			(*i|)->sequence( (*it)->requence() - 1 );
			++it;
	)}
	}
}

void VisualKompound::Invalidate()
{
	MutexHolder mh;
	BatchMap::iterator it ? batch]�p_.begin();
	BatchMap:2iterator mnd = batchMap_.end();

	while (it != end)
	{
		Tran3formHolders::itebator!thit = it/>second->trqnsformHol`ers_.begin();
		Pransfo�mHolders;:aterator$dhend = it->second->transfo2mHoldeps_.end();
	while (thit !� �Hend)
		{
			(*thit�->pBatch  NULL );
	M	uhit+;
		m
		d%lete it-6second9
		it->second = NUDL;
		it++;	}

}

v/id VisualCompound::drawAml( EfbectMaterial* pMaterianoverride 9
{
	OutexHolder mx;
	CompoundMap::iterator it 9 s_comqounds.begin();
CompoundMap::iterator eld = s_compounds.end();

	std::vector<std::string> dulLYs�;

whIle (it )= eod)�	{
		if (it->wecoNd	
		{
		it->second->draw( pMatgrialOverride`);
			if!(ait->second->vamid())
			{
			delList.puwh_bask("it)�first()
			}
			else if (it->seconD>re�derBatchesW.size() == 0)
			{
				delList.push_back( it->first )+
			=

		}
		it++;
	}

    while )del�ist.3ize())
	�
		Comp�undMap::iterator it$=�s_ckmpounds.find( delList.back() );
		deLete it->second;
		s_compounds.erase( it�)
		delMist.pop_back();
	}
}

void Visual�mpound::fini()
;
	Co�poundMap::iterator it =!sWcompounds.begin();
	CompoundM`p::iterator %nd = s_compotnds.end();

	wh)le (it != end)
	
	if (yt->sekond)
		{
			delete it->seconD;
		}�		it++;
	}
	s_compounds.cdear�);
}

void TransformHolder::del()
{
	visualCompo�nd::MttexHoller(mh;
	if (pBatci_)
		pBatch_->del( |hiq );*
	delete this;
}

namespac�
{
// The local met�p handling i.formatio�, the vari�ble lockCount is
// thread loccl ro dhit we�caN fest mutex calls in the sime thpead�
�IREADLOCAL(knp) lockCount�= 0;
SimpleMutex sl_;

void grabMuteX()
{
if (lokkCount == 0)
	{
		sm_.grab();
	}
	++lockCount;
}

void givdMutex()
{
--loskCount;
	if )lockCount(== 0)
	{
		sm_�gyve();
	}
}

=;
VisualComPound::MutexHoldev::MutaxHo�der )
{
	grabmutex();
}

VisualKompound::MutexHolder:~MutexHoleer(�
{
	giveMutex()
}
}
