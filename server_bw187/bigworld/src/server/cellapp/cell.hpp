/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef CELL_HPP
#define CELL_HPP

class Cell;
class Entity;

#include "Python.h"

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <deque>

#include "space.hpp"
#include "cellapp_interface.hpp"
#include "cell_viewer_server.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/smartpointer.hpp"

#include "math/math_extra.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "network/mercury.hpp"
#include "server/common.hpp"

typedef SmartPointer<Entity> EntityPtr;

class GatewaySrcController;
class GatewayDstController;

class CellAppChannel;

typedef std::vector< EntityPtr >::size_type EntityRemovalHandle;
const EntityRemovalHandle NO_ENTITY_REMOVAL_HANDLE = EntityRemovalHandle( -1 );


class GatewaySubscription;
class SpaceViewport;

class Chunk;

/**
 *	This class is used to represent a cell.
 */
class Cell
{
public:
	/**
	 *	This class is used to store the collection of real entities.
	 */
	class Entities
	{
	public:
		typedef std::vector< EntityPtr > Collection;
		typedef Collection::iterator iterator;
		typedef Collection::const_iterator const_iterator;
		typedef Collection::size_type size_type;
		typedef Collection::value_type value_type;
		typedef Collection::reference reference;

		iterator begin()				{ return collection_.begin(); }
		const_iterator begin() const	{ return collection_.begin(); }
		iterator end()					{ return collection_.end(); }
		const_iterator end() const		{ return collection_.end(); }

		bool empty() const				{ return collection_.empty(); }
		size_type size() const			{ return collection_.size(); }

		bool add( Entity * pEntity );
		bool remove( Entity * pEntity );

		EntityPtr front()				{ return collection_.front(); }
		EntityPtr front() const			{ return collection_.front(); }

	private:
		void swapWithBack( Entity * pEntity );

	private:
		Collection collection_;
	};

	// Constructor/Destructor
	Cell( Space & space, const Space::CellInfo & cellInfo );

	~Cell();

	void shutDown();

	// Accessors and inline methods
	float cellHysteresisSize() const;

	const Space::CellInfo & cellInfo()	{ return *pCellInfo_; }

	// Entity Maintenance C++ methods
	void offloadEntity( Entity * pEntity, CellAppChannel * pChannel );

	void addRealEntity( Entity * pEntity, bool shouldSendNow );

	void entityDestroyed( Entity * pEntity );

	EntityPtr createEntityInternal(	BinaryIStream & data, PyObject * pDict,
		bool isRestore = false,
		Mercury::ChannelVersion channelVersion = Mercury::Channel::SEQ_NULL );

	void backup( int index, int period );

	bool checkOffloadsAndGhosts( bool isGeometryChange );
	void checkChunkLoading();

	void onSpaceGone();

	void debugDump();

	void gatewayEntrance( GatewaySrcController * pGatewaySrc, bool alive );
	void gatewayApparent( GatewayDstController * pGatewayDst );

	float findNimbusAroundEntity( Entity * pSrcEntity );
	uint32 countSubscriptions( Entity * pSrcEntity );
	uint32 findViewports( Entity * pSrcEntity, SpaceViewport ** viewports );

	// Communication message handlers concerning entities
	void createEntity( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );
		// ( ObjectID id, TypeID type, Position3D & pos, string & args );

	static Cell * findMessageHandler( BinaryIStream & data );

private:
	// General private data

	Entities realEntities_;

	bool shouldOffload_;

	mutable float lastERTFactor_;
	mutable uint64 lastERTCalcTime_;

	float cellHysteresisSize_;
	long cellOffloadMaxPerCheck_;
	long cellGhostingMaxPerCheck_;

	friend class CellViewerConnection;
	std::vector<GatewaySubscription*>	subscriptions_;
	std::vector<SpaceViewport*>			viewports_;

public:
	// Instrumentation
	static Watcher & watcher();

	SpaceID spaceID() const;
	Space & space()						{ return space_; }
	const Space & space() const			{ return space_; }

	const BW::Rect & rect() const		{ return pCellInfo_->rect(); }

	int numRealEntities() const;
	float getActualLoad() const;

	void sendEntityPositions( Mercury::Bundle & bundle ) const;

	Entities & realEntities();

	// Load balancing
	bool shouldOffload() const;
	void shouldOffload( bool shouldOffload );
	void shouldOffload( const CellAppInterface::shouldOffloadArgs & args );

	void retireCell( const CellAppInterface::retireCellArgs & args );

	void removeCell( BinaryIStream & data );
	void notifyOfCellRemoval( BinaryIStream & data );
	void ackCellRemoval( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	bool reuse();

	void handleCellAppDeath( const Mercury::Address & addr );

	void restoreEntity( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	bool isRemoved() const	{ return isRemoved_; }

	void createPendingEntities();

private:
	enum LoadType
	{
		LOAD_TYPE_ABOVE_AVERAGE,
		LOAD_TYPE_BELOW_AVERAGE,
		LOAD_TYPE_BALANCED
	};

	// Load balance related data
	float initialTimeOfDay_;
	float gameSecondsPerSecond_;
	bool isRemoving_;
	bool isRemoved_;

	enum
	{
		STAGE_CREATE_GHOSTS_0,
		STAGE_CREATE_GHOSTS_1,
		STAGE_OFFLOAD,
		STAGE_DEL_GHOSTS
	};

	int checkOffloadsAndGhostsStage_;

	Space & space_;

	int backupIndex_;
	Space::ConstCellInfoPtr pCellInfo_;

	/**
	 *	This class is used to store information about createEntity and
	 *	restoreEntity calls that cannot be processed immediately because the
	 *	CellApp is overloaded.
	 */
	class PendingCreation
	{
	public:
		PendingCreation(
				BinaryIStream & data, const Mercury::Address & srcAddr,
				Mercury::ReplyID replyID,
				bool isRestore,
				Mercury::ChannelVersion channelVersion ) :
			stream_( data.remainingLength() ),
			srcAddr_( srcAddr ),
			replyID_( replyID ),
			isRestore_( isRestore ),
			channelVersion_( channelVersion )
		{
			stream_.transfer( data, data.remainingLength() );
		}

		BinaryIStream & stream()					{ return stream_; }
		const Mercury::Address & srcAddr() const	{ return srcAddr_; }
		Mercury::ReplyID replyID() const			{ return replyID_; }

		Mercury::ChannelVersion channelVersion() const
		{
			return channelVersion_;
		}

		bool isRestore() const
		{
			return isRestore_;
		}

	private:
		MemoryOStream	 		stream_;
		Mercury::Address		srcAddr_;
		Mercury::ReplyID		replyID_;
		bool					isRestore_;
		Mercury::ChannelVersion	channelVersion_;
	};

	typedef std::vector< PendingCreation * > PendingCreations;
	PendingCreations	pendingCreations_;

	typedef std::set< Mercury::Address > RemovalAcks;
	RemovalAcks pendingAcks_;
	RemovalAcks receivedAcks_;
};

class CellTrigger;

/**
 *	This class is a cell's record of a subscription made by it to a gateway.
 *	The subscription may or may not be fulfilled, i.e. it may or may not
 *	have a corresponding SpaceViewport
 */
class GatewaySubscription
{
public:
	GatewaySrcController	* pGatewaySrc_;
	float					range_;
	SpaceViewport			* pViewport_;
};

/**
 *	This class is a cell's record of its viewport into a space.
 *	There is one of these for each gateway that the cell has a fulfilled
 *	subscription to.
 */
class SpaceViewport : public ReferenceCount
{
public:
	SpaceViewport( GatewaySubscription * pSubscription,
		GatewayDstController * pGatewayDst );
	SpaceViewport( ObjectID oldGatewaySrcID, Space * pSpace );
	~SpaceViewport();

	bool alive() const					{ return pSubscription_ != NULL; }
	ObjectID oldGatewaySrcID() const	{ return ObjectID(pGatewaySrc_); }

	GatewaySrcController	* pGatewaySrc_;
	GatewayDstController	* pGatewayDst_;
	Space					* pSpace_;
	GatewaySubscription		* pSubscription_;
	CellTrigger				* pCellTrigger_;
};






#ifdef CODE_INLINE
#include "cell.ipp"
#endif

#endif // CELL_HPP
