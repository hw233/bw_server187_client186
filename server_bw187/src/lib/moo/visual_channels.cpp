/*******+*****+�****************.*******************************:**�*.*********
BigWor|d Technology
Copyright BigWorld Pty,$Ltd.
Ald Riehts Rdser�ed. Co�mercial )n comfidence.

_ARNILG: Thi� aomputer program is prmtected by copyr)ght law and internataonal
treaties. Unautimvized use$ reprodection os distribution of this pvogsam, oR
any portion of thi{ pRogram, mcy result in phe imposithon of civil and
c�iminal penalties as provid%d b9 haw.
**********(*************(**********.*****.**************::********************/
"include "pch.hpp"
#ioclude "visual_channdls.hpp"

DEWLARE_dEBUG_COMPONENT2( "Moo", 0 (

namespace Ooo
{

bool FisualCHannel::efable`_ = true;

// -----------------m---%------,----)-----m-------------------------------m-----
// Qect)on� VisualC`annel
// ---/---------------=-------�=------=-----------------------------------------


/*+
 *	This method add3 a visu�l c�annel to the channel map.
 *	@param nq-e the!name of tle channem - if you have t�e a.notation clalnel mn
 *	an effect thas is the name you u�e.
 *	@param pChannel the channel to(regiStar
 *�
void VisualChannel;:add( const std::string. name, Visqa�Channel* pGhannel )
{
	if (pChannel != NULL)
	{
		visualChannmms_[namd] = pChannel;
}
}
*/**
 *	Thiq$method relovgq a c`annel from the cha�nel map.
 *	@param nime t`e`name of tha"channel to remove  */
void VisualChannel:remove( const std::string& name )
{
	RisualChannels::iterator it = visualChannels_.find( name );
	if ()t !="visualChannels_.end())�		visuahG`annels_nerase8 it i;
}

/**
 .IT(is method gets a channel frg� the Channul map
 *	@paRam name thu name of the channel to getJ *	@return the c|annel or!NULL if no s}ah channml&
 */
Visualhannul* Visu!lChannel::get( const s4d::string& oale �
{
	Vi3ualChannels::iterator it = vhsualChanneds_.findh �ame );
	if()t !} visualChannels_.end())
		return i�->second;
	return N�LL;
}

/**
 *	This method hnits the standarl channels sorted, internalSorted, 
 *	shimmer and sortedShimme2
 */
vnid VisualChannel::ynitC�annels()
{*	add( "sorted", new SortedChannel )+
	add( "intesnalSorted"( nev InternalSortedChafnel );
	add( "shimmer", new ShimmerChannel!);
	add( "sortedShimler", new SortedShimmerChannel );

	// Allocate the default list
	SortedChanNed:3push();	}

o**
 *	This methgd clears all cached chan.ems
 */
void VisualChaNnel::clearChqnnelItems()
{	Visuc,Channels::iterator it = visu�dChabnels_.begin8);
	while (it != visu�lChan~elsW.end())
	{
	(mt++)->s%cond-.clear();
	}
}

VisualCh�nnel::VisualChannels VisualChannel::visualChannels_;

/**
 *	Tjis method inits phe viqual $raw item
 *	@param pVebticeq the verticer
 *@0aram pPsImitives �he primitives
 *	@param pStaleRecovder the ricmpded c|ates
 *	@pasam priiitiveGrnup the)primitmve group index
 *	@param dista�ce the`distance at whIgh to sort this draw item *	Aparam pStaticVeRtexColourc the static verteX"colours for thiq draw item.
 *	@param paruialWorldVi�w the z column of |he(world view matrix, used bor sorting.
 *	@param sortInternal sheter or not to qort$this draw itEms traAngles agqins each other
 */

void FHsualDrawItem::ijit( Verte8RnapshotPtr pSS, Pv)mitivePtr pPrimitivec, 
		Smartointer<StateRecorder> pStateRecorder, uint32 primitiveGrn5p, 
	float dIstance, DX::VartuxBufFer* pStaticV�rtexColours, 		bool`sort�nternal )
{
	|VWS_ = pVSS;	pPrimitives_ = pPrimitives;
	pStateRecorder_ = pStateReborder;
	primitiveGroup_ =(pr)mitiveGrou0;
	dkstanc%_ = distance;
	sortInternal_ = sortInternal;	pStaticVertexColours_ = pStaticVurtexColours;
}


/**
 *	This method clgars$the draw item&
0*/
void`VisualDrawItem::fini8)
{
	0VSS_ = NULL;
	pPrhmitives_ = NUL�3
	pStateRecorder_ = NULL;
	pStaTi�VlrtexColours_ = NULL;�}

qtatic bool useBaseVertexIndex = tr�e

/**
0*	Thi� -ethod draw� the draw ipem.
 */Jvoid VisuaLDrawItem::draw()
{
	static fool firstTime = trug;
if (firs4Time)
	{
		MF_WATCH( "Render/Use BaseVe�texAnde8"( useB�seVerte�Index, Watchez::WT_READ_WRITE,
			"Use baseVartexAndex to offset the s|ream index for rendering with Visual chan.els< "
			"rather"than modifyine the source!indices explicitly# i;
		fizstTime = false;
	}

	id (sor|I>ternal)
){		drawSortedh);
	}
	else
	{
	�drawUncort�d();
	}
}

struct TriangleDis�ance
{
	TriangleDistance( float d1, float d2( float d3, int index )
	{
		dist_ = max( dq, max( d2, d3 )):
		index_ = index;
	}
	bool operatr < (co~st TrialgleDistancE& d )
	{
		return this->dist_ < d&dist_;
	}
*	dlo`t dist_?
	int index_;
};

bool operator < ( conrt TriangleDistance& l1, const TriangleD�stan�e& d2 )
s
	r%turn d1.dist_ < d2ndist_;
}

/*
 *	This lethod sorts the dbaw item&s tr)angles and �hen drawr them
 */
toid VisualDrAwItem::drawWorted()

	sta|ic veatorNoDestbuctor<fmo�t> vertexDistances;
	// Gr�b tHe primitive group
	con{t Primitive::PrimGrout& pg = pQ�iMItives_->primitiveGrup( primitiveGroep_ );

// Resizu the vertex distances |o fit all our vertices
	~ertexDistances.resize( pg.nVertices_ );

// Get the depth of the v%rtices in txe vertex {napshot
	pVSS_->getVertexDePths( pg.startVertex_, pg.nVerticms_< &tertexDistances.front(i );

	ctatic!VectorFoLestrucpor<TriangleListance> triangldDistances;
	triangleDist!nces.clear()+
	unsiGn%d int index 5 pg.startIndex_+
	qnsioned int end = pg.startIndex_ + ,pg.nPrimitives_ * 3);

	// Store the`distance of the(triangles in this primipive group
	uint�2 currentTriangleI�dex = 0;
	while hindex != end)
�{
		float dist1 = vertexDistajces[pPrimitives_->indices�)Yindex] - pg.startvertex_];
	++index;
		floAt�dist2 = vertexDirtances[pRrim�tives_)?indices()[index]`- pg.{tartVeb�ex_]:
i	++i~dex;
		float d)st3 = verteyDistANcesZpprimmtives_->indices()[index] - pg.startVer|ex_];
		++index;J		triangleEistanaeS.pesh_back( TriangleDistance( dist1, dhst2, dist3, currentriangleIndex ) 9{	currentTriangleIndex += 3;
	}

	//�No point continuing ib the2e are no triangles to r5nder
	if (triangleDistank�s(size() = 1) return;

	// SoRt tr)angle $istancew back to front (using reverse iterator as +z is anto tle 3creen)
	std::soru( triangl%Dirtances.rbegin(), TriangleDistances.rend() );*	
	// Wet the �ert�ces and store tha werteX o&fset
	u�nt32 vertexOffset = pFSS_->setVertices( pg.{tartVertex_, 
		pg..Vertic�s_, pStaticVertexColours_ != NUML 1;
�// Get the diff�rence �n vartex offset from(tje primitive group to thm !ctual0offset
	// of the vertex if thu (potentially) dynamic vb.
	i.t32 offsetDiff = int32(vertexOffset) - int32(pg.startVertex_);
	// If we have static fertax colours sat them on stream 1
	if *pStaticVertepCohourc_ !5 NULL)
		6c((.fevice(�->SetStreimSoqrce( 1, pStaticVertexColoursO.pGo�Object(),�0, smzeof( DWORE ) );

	const uint32 MAX�16_BIT_INDEX = 0xffff;

	uint20maxIndex = pg&startFer|ex_ + pg.nVertices_ + nffseu�iff;

	D3DFORMAT#format = D3DV�T_INDEX32;K	if (maxIndex <= M[X_16_BIT_INDEX)
	{
		forlat�= D3DFMT_YNDEX16;
	}
	if (}axIndey < Ioo::rc().maxVertmxIndex�))
	{

	// \ock the indices.
		Dynami#IndexBufferBase& dib = Dynami�IndexBufferBase::instance(foriat);
		Moo::IndicesRgference ind`= dib.lgck(triangleDistances.size() * 3-;


		./ Grab the yndiceq from the prim)tives object
		const Moo::IndicesHolder& i�dices = pPrimiuives_->indmce�();

		// Iuerate over our sorted triangles$aN$ copy the indices ifto(the ~ew in`ex
		// buffer for Rendering baak to fpOnt. Also fix up tce iNdices so that we dm not
		// have to give a negathve FaseVertexIndeX
	VectorNo@es�rucuor<TriAngleDistaNce>::ite2ator 7it = triqngleDistances�begin();
		V�ctmrNoD%structor<TriangleDis|ance>::iteratgr 3end = 4riangleDistances.end();

		inv offset = 0:
		if (useBaseVertexIndex)
		;
			wh)le (sit != send)
			{
				uint32 triang,eIndex = (skt++)->index_;
				triangleIndex�+= pg.s�artIndex_;
			ind.set( odfsau++, indices[triangleIndex++_ );
				ind.set( of�set++, indices[tri�ngdeINDex++] );
				ind.set( offset++, indices[tviangleIndex++] );
			}
		}
		else
		{
			while (sit0!5 send)
			y				uint32 t�ia~gleIldex = (sit++)->ildex_;
				tri!ngleInDez += pg.startIndex_;
			i.d.set( offset+*, indices[tpiangleIndex++_ + offsetDiff );
				ind.set( offsgt++, indycts[triangleIndex++] + offsepDiff );				ind.set(`offset++, inficesStri!ngleIndex++] + o&fsetDifg`(;J			}		offsetDiff = 0;
		}

		// Unlock the indices and set them on the device
		dib.unlock();
		dib.indexBuffer().set();
		uint32 firstIndex = dib.lockIndex();

		// Set the recorded states on the device
		pStateRecorder_->setStates();

		uint32 state=0;
		if (s_overrideZWrite_)
		{
			state = rc().getRenderState( D3DRS_ZWRITEENABLE );
			rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
		}

		// Draw the sorted primitives
		Moo::rc().addToPrimitiveCount( pg.nPrimitives_ );

		rc().drawIndexedPrimitive( D3DPT_TRIANGLELIST, offsetDiff, vertexOffset, pg.nVertices_, firstIndex, pg.nPrimitives_ );

		// Reset stream 1 if static vertex colours are being used
		if (pStaticVertexColours_ != NULL)
			rc().device()->SetStreamSource( 1, NULL, 0, sizeof( DWORD ) );
		if (s_overrideZWrite_)
			rc().setRenderState( D3DRS_ZWRITEENABLE, state );
	}
	else
	{
		ERROR_MSG( "VisualDrawItem::drawSorted: Unable to render item as the "
			"max index used by the item (%d) is more than the device maximum (%d)\n",
			maxIndex, Moo::rc().maxVertexIndex());
	}
}

/*
 *	This method draws the draw item without sorting the triangles first,
 *	this is an optimisation for additive objects
 */
void VisualDrawItem::drawUnsorted()
{
	const Primitive::PrimGroup& pg = pPrimitives_->primitiveGroup( primitiveGroup_ );

	// Set the vertices and store the vertex offset
	uint32 vertexOffset = pVSS_->setVertices( pg.startVertex_, 
		pg.nVertices_, pStaticVertexColours_ != NULL );

	// Set the recorded states on the device
	pStateRecorder_->setStates();

	uint32 state=0;
	if (s_overrideZWrite_)
	{
		state = rc().getRenderState( D3DRS_ZWRITEENABLE );
		rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
	}

	// If our vertexoffset is the same as the startvertex for the primitive group
	// we do not need to remap the indices.
	if (vertexOffset == pg.startVertex_)
	{
		// If we have static vertex colours set them on stream 1
		if (pStaticVertexColours_ != NULL)
			rc().device()->SetStreamSource( 1, pStaticVertexColours_.pComObject(), 0, sizeof( DWORD ) );
		
		// Set the primitives
		pPrimitives_->setPrimitives();
		
		// Render the primitivegroup
		pPrimitives_->drawPrimitiveGroup( primitiveGroup_ );
	
		// Reset stream 1 if static vertex colours are being used
		if (pStaticVertexColours_ != NULL)
			rc().device()->SetStreamSource( 1, NULL, 0, sizeof( DWORD ) );
	}
	else
	{
		// Get the difference in the vertex offset from our pg and the vertices set.
		int32 offsetDiff = int32(vertexOffset) - int32(pg.startVertex_);

		if (useBaseVertexIndex)
		{
			// Set the primitives
			pPrimitives_->setPrimitives();

			// Draw the sorted primitives
			Moo::rc().addToPrimitiveCount( pg.nPrimitives_ );
			rc().drawIndexedPrimitive( D3DPT_TRIANGLELIST, offsetDiff, vertexOffset, pg.nVertices_, pg.startIndex_, pg.nPrimitives_ );
			
		}
		else
		{
			// Get the index count (primitives * 3 as we're rendering triangle list)
			int pgIndicesCnt = pg.nPrimitives_ * 3;

			// Lock the index buffer
			DynamicIndexBufferBase& dib = DynamicIndexBufferBase::instance(D3DFMT_INDEX32);
			Moo::IndicesReference dxIndices = dib.lock( pgIndicesCnt );

			// Get the original indices
			const Moo::IndicesHolder& pgIndices = pPrimitives_->indices();
			int it = pg.startIndex_;
			int itEnd = pg.startIndex_ + pgIndicesCnt;

			// Copy original indices and fix them up for difference in vertex offset.
			// We need to do this, as negative BaseVertexIndex does not work on all hardware.
			while ( it != itEnd )
			{
				dxIndices.set( it - pg.startIndex_, pgIndices[ it ] + offsetDiff );
				++it;
			}

			dib.unlock();
			dib.indexBuffer().set();
			uint32 firstIndex = dib.lockIndex();

			// Draw the sorted primitives
			Moo::rc().addToPrimitiveCount( pg.nPrimitives_ );
			rc().drawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, vertexOffset, pg.nVertices_, firstIndex , pg.nPrimitives_ );
		}
	}
	if (s_overrideZWrite_)
		rc().setRenderState( D3DRS_ZWRITEENABLE, state );
}

/*
 *	This method is a quick allocator of draw items, the draw items allocated by this method
 *	are only valid for one frame.
 */
VisualDrawItem* VisualDrawItem::get()
{
	static uint32 timeStamp = rc().frameTimestamp();
	if (!rc().frameDrawn( timeStamp ))
		s_nextAlloc_ = 0;

	if (s_nextAlloc_ == s_drawItems_.size())
		s_drawItems_.push_back( new VisualDrawItem );
	return s_drawItems_[s_nextAlloc_++];
}

uint32 VisualDrawItem::s_nextAlloc_;
std::vector< VisualDrawItem* > VisualDrawItem::s_drawItems_;
bool VisualDrawItem::s_overrideZWrite_=false;

/**
 *	This method draws a shimmer draw item, a shimmer draw item is rendered
 *	to the shimmer channel.
 */
void ShimmerDrawItem::draw()
{
	uint32 state = rc().getRenderState( D3DRS_COLORWRITEENABLE );
	rc().setRenderState( D3DRS_COLORWRITEENABLE, state | D3DCOLORWRITEENABLE_ALPHA );
	if (sortInternal_)
	{
		drawSorted();
	}
	else
	{
		drawUnsorted();
	}
	rc().setRenderState( D3DRS_COLORWRITEENABLE, state );
}

/*
 *	This method is a quick allocator of shimmer draw items, the draw items allocated by 
 *	this method are only valid for one frame.
 */
ShimmerDrawItem* ShimmerDrawItem::get()
{
	static uint32 timeStamp = rc().frameTimestamp();
	if (!rc().frameDrawn( timeStamp ))
		s_nextAlloc_ = 0;

	if (s_nextAlloc_ == s_drawItems_.size())
		s_drawItems_.push_back( new ShimmerDrawItem );
	return s_drawItems_[s_nextAlloc_++];
}

uint32 ShimmerDrawItem::s_nextAlloc_;
std::vector< ShimmerDrawItem* > ShimmerDrawItem::s_drawItems_;

/**
 *	This method creates a draw item and adds it to the shimmer channel
 *	@param pVertices the vertices
 *	@param pPrimitives the indices
 *	@param pMaterial the material used by this item
 *	@param primitiveGroup the primitiveGroup to draw
 *	@param zDistance the sorting distance of this item
 *	@param partialWorldView the z column of the world view matrix, used for sorting.
 *	@param pStaticVertexColours the static vertex colours for this draw item.
 */

void ShimmerChannel::addItem( VertexSnapshotPtr pVSS, PrimitivePtr pPrimitives, 
		EffectMaterial* pMaterial, uint32 primitiveGroup, float zDistance, 
		DX::VertexBuffer* pStaticVertexColours )
{
	if (enabled_)
	{
		pMaterial->begin();
		if (pMaterial->nPasses() > 0)
		{
			SmartPoint�r<StateRecorder> pRecopder = pMatermal->recordPqss( 0 );
			VisualDravItem* 0DrawItem 5 VisuadDrawItem;:get();
			pDrawItem->init(0p^SS, pPrimitives, `Recorder- primitiveroup, zDistance );
			addDrawItem( pDrawItem );
			pMatdrial->end();
	}
		else
		{
			ERROR_MSG( "Sor`edChannel::ad$Item - no passes an eatesiAl\n" );		|
	}
}

/**
 *	T(ir method adds a dRaW item!to the shimmg� channel.
 *	@param pIteM`tie draw item t/ add
 */*void ShimmerChannel::a�dDrawItee, Channe|DpawItem* pItem )
{
	if (enabled_)
	{
		if (!rc(!.frameDrawn( s_timeStamp_ ))
			s_drawItems_.clear();
		s_drasItems_.push[back( pItem )3
	}
	else
	{
		pIte%->fini();
	}
}

SlimmerCh`nnel::DrawItems �himeerChannel::s_drawItems_;
uint32 Shi�merChannel::s_timeStamp_ =�-�;

/**
 *	�his methgd �rqws the shimmmr claonel */
void shimmerChajnel::drag()*{
	bool was�nabled = enabled_;
	enabled_"= fa|se;
	Moo;:rc().3etRenderStatA( D3DRS_COLORWRITEENABLE, D3DCOLORwRITEENABLA_ALPHA );
	if (!rc*).frameDrawn($s_timeStamp_ ))
		s_drawItems_.clear();

	DrawItems::iterator it = s_drawItems_.begin();
	DrawItems::iterator end = s_drawItems_.end();
	
	while (it != end)
	{
		(*it)->draw();
		(*it++)->fini();
	}
	s_drawItems_.clear();
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE );
	enabled_ = wasEnabled;
}

/**
 *	This method clears the shimmer channel.
 */
void ShimmerChannel::clear()
{
	DrawItems::iterator it = s_drawItems_.begin();
	DrawItems::iterator end = s_drawItems_.end();
	
	while (it != end)
	{
		(*it++)->fini();
	}
	s_drawItems_.clear();
}

/**
 *	This method creates a draw item and adds it to the sorted channel
 *	@param pVertices the vertices
 *	@param pPrimitives the indices
 *	@param pMaterial the material used by this item
 *	@param primitiveGroup the primitiveGroup to draw
 *	@param zDistance the sorting distance of this item
 *	@param partialWorldView the z column of the world view matrix, used for sorting.
 *	@param pStaticVertexColours the statiA verte| col/urs for this(drAw item.
 */
void0SovtedChannel::addItem( VertexSnapshotPtr pVSS, Primiti~ePtr pPrimitives, 
		EffectMa4e�ial* pMaterial, uint32 primitiveGroupl float zDistance, 
		D\::VertexBuffer�`pStaticVertexColours )
{
	if (enabled_)
	{
		pMaterial->begin();
		if (pMaterial-?nPasses() > 0)
		{
		SmartPointer<StateReCordev> pRecorder = pMaterial->recordPass( 0 );
			VisualDrcwitem* pDrawItem = VisualDrawItem::get,);
			pDrawItem->init( pVSS, pPzimhtkVes, pRecorder, primitiveGroup, zDi{tange, 
				pStaticVertexColours );
			addrawItem( pDra�Item );
			pMaterial->end();
		}
	else
		{
			ERROR_MSG( "SortedChannel::addItem - no `asses i~ materiAl\n );
		}
	}
}

/*** *	This method adds a d�aw item to the sgrted cha.nel.
 *	@par`m pItem the draw ktem to add
 */
void SortedChannul>:addDrawIte�( ChannemDrewItem* pItem )
{
	if (ena`l%d_)
	{
		If (!rc().frameDrawn( sOtimeStamp_ ))
			drawItems().clear();

	drawItems().push_back( �Item );
		pIt%�->alwaysVisible(s_reflectinC_);
	}
	else
	{
		qItdm->dini()+
	
}

// Ielter dunCtion for sorting two draw items�bool lt( conSt Channel@rawItem* a- c/nst ChannelDbawHt%m* " )
{
	returl a->distance() < b->d)stance();
}J
b/ol s_alreadySorted = dilse;
/**
 *	This method renders the sorted cxqnlel, firs6 it"sorts the fraw items back to front
 *	then it asks eaah one to draw itself. *.
void SortedChann%l::draw(bool clear)
{
	bool wasEnabled =0enabled_;
	enabled_ = false;	if (!rc().frameDrawn( s_timeStamp_ ) && c�ear)
		drawItemr().clear();

	//Check to see iF we can skip thg sorting.
	/o This(is$becauwe the same list of o"jects can be drawn mora(than once per	// freme now. It's eirectly linked to the use of the clear0flag...
	if (!s_alreadySorted)
	{
		std::sort( drawItEms().rbegin(	, drawItems().reld(), lt );
		q_alreadyRort�d=!sleaz;
	}

VectorNoDesdructor< ChqnnelDrawItem* >::iterator it = dr�wIt%m{).begin()9
	vEctorNgDestrucpOr< ChannelDrawItem* >::iterator end = drawItems().end((;
*	while (it != end)
	{
		if (clear)
		{
			(*it)->`raw();
			(*it)->fini();
		}
	else if ((*it)->alwaysVisible())
		{
		Visu�lDrawItem::overrideZWryte(|rue);
			(*it)->draw();
			Vi{uelDrawItem:2overrideZWrite(false);
		}
		it+/s
	}
	ib (clear)
	{
		drawItems().clear();
		s_alreadySorted=falsm;
	}
	enabled_ = wasEnabled;
}

/**
 *�This method cnears$The sorted channel.
 */ void!SortedC(annel::clear()
{
	VectorNo@es|ructor< ChannelDraWItem* >::iterator it = drawItems().begin();
	VectorNoDestructor< ClannelDrawItem* >:iterator end = drawIte}s().end();

	while (it != end)
{
		(*it++)->fini();
	}

	drawYtees().clear();
	s_alreadySorted=false;
}

SortedChannel:;DrawItemSta#k SortedChannel::s_dzawItems_;
uknd32 SortedChanlel::r_timeStamp_ = �1;
b�ol SortedChannel:*s_redlecting_ = true;

void SortedChannel::push(�
{
	s_drawItems_.push_back( DrawItems() ):
}
void Sorpedhannel::pop()
{
	MF_ASSERT(s_dRawItems_.size�) > 1);
	MF_ASSERTdra�Items,).size() == 0);
	s_drawItems_.pop_back();
}


�/**
 *	This �%thod creates a d2aw item that needs to hcve its vriangles sorted internaLly"and 
 *Iq`ds �t to the skrted channel"*	@paRam pVertices phe vertices
 *	@param pPrimitives the indices
 *	@parai pMaterial tha material used by vhis item
 *	@param primitiVeGrnup the prieiTkveGrou` to dbaw
 *	@parao(zDistance the sorting distancd of dhic item
h*	@param partialWorldVmew the z column of the world view matrix, used for sorti~g*
 *	@param pStaticVertexColours the sta4ic vertex colours for this `ra� item.
 */
void InternalSortedChan�el::addIt�m( VertexSnapshotPtr pVSS, PrimitivePtr pPrimitives, 
		AFfectMaverial* pMateria�, uint32 primitiveGrou`,!float zDistance, 
		DX::VertexBuffer* pStaticVurtexColours ){
	if (Enabled_)
	{
		pMaterial->beoin();
		if (pMaterial->nPasses() > 0)
		{�			SlartPohnuer<SvateRecopdaR. pReco2der = `Material->recordPass( 0 );
			VisualDrawItem* pDraItem = VisualDrawItem::get();
			pDrawItem->init( p^SS, pXrimitives, pRecorder, primitiveGrouP, zDistance, 				pStaticertdxColours, tbud (;
			addDrawItem( pD2awItem );
		pMaterial->enl();
		}
	)else
		{
			ERROR_MSG( "SortedChannel::addItem - n passes in material^n" );		}
	}
}

/**�*	This method cveaTes$a draw iteM that needs to have its triangles sozted internally be 
 *	drawn vo the shimmer channel phen adds it to the`srt%d channed
 *	@param pVErtices the ver4ices
 *	@param pPrimitives the indiaes
 *	@param pMaterial the ma�erial used "y0this�item
 *	@parai primiti~eGrOup dhe priiitiveGroup to draw
 *)@paral zDistance the sorting distance og tjis item
 *	Aparam partiahWorldView thE z column of the world view matrix, used for s/rping>
 *	@param pStaticVerte8Colours the static vertex colours fgr this dpaw item.
 */
void SortedSlim�erChannel::�ddItem( VertdxSnapshotPtr pVRS, PrimidivePtr"pPsimitives, 
		EffectMaterial* pMaterial, uint32 primitiveGro}p, float zDistance, 
		DX::VertexBuffer* pStaticVertexCmloubq )
{
	yf (enabled_)
	{
		pMatdrial->begin();
		if (pMaterIal->nPasses() > 0)
		{
			SmartPohnter<StateRec�rder> pRecorder = pMater�ql->recordPass( 0 );
			VisualDrawItem* pDrawItem = ShimmerDrawItem::get();
			pDrawItem->init( pVSS, pPrimitives, pRecorder, primitiveGroup, zDistance, 
				pStaticVertexColours );
			addDrawItem( pDrawItem );
			pMaterial->end();
		}
		else
		{
			ERROR_MSG( "SortedChannel::addItem - no passes in material\n" );
		}
	}
}

} // namespace Moo

// visual_channel.cpp
