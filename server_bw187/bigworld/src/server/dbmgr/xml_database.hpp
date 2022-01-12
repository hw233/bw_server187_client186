/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef _XML_DATABASE_HEADER
#define _XML_DATABASE_HEADER

#include "idatabase.hpp"
#include "resmgr/datasection.hpp"
#include <map>

class EntityDescriptionMap;
class EntityDefs;

class XMLDatabase : public IDatabase
{
public:
	XMLDatabase();
	~XMLDatabase();

	virtual bool	startup(const EntityDefs&, bool, bool);
	virtual bool	shutDown();

	virtual void mapLoginToEntityDBKey(
		const std::string & logOnName, const std::string & password,
		IDatabase::IMapLoginToEntityDBKeyHandler& handler );
	virtual void setLoginMapping( const std::string & username,
		const std::string & password, const EntityDBKey& ekey,
		ISetLoginMappingHandler& handler );

	virtual void getEntity( IDatabase::IGetEntityHandler& handler );
	virtual void putEntity( const EntityDBKey& ekey, EntityDBRecordIn& erec,
		IDatabase::IPutEntityHandler& handler );
	virtual void delEntity( const EntityDBKey & ekey,
		IDatabase::IDelEntityHandler& handler );

	virtual void executeRawCommand( const std::string & command,
		IExecuteRawCommandHandler& handler );

	virtual void putIDs( int count, const ObjectID * ids );
	virtual void getIDs( int count, IGetIDsHandler& handler );

	virtual void remapEntityMailboxes( const Mercury::Address& srcAddr,
			const BackupHash & destAddrs );

	virtual void migrateToNewDefs( const EntityDefs& newEntityDefs,
		IMigrationHandler& handler );
	virtual IOldDatabase* switchToNewDefs( const EntityDefs& oldEntityDefs );

	void getEntity( IDatabase::IGetEntityHandler& handler,
		const EntityDefs& entityDefs );
	void putEntity( const EntityDBKey& ekey, EntityDBRecordIn& erec,
		IDatabase::IPutEntityHandler& handler, const EntityDefs& entityDefs );

private:
	bool			deleteEntity( DatabaseID, EntityTypeID );
	DatabaseID		findEntityByName( EntityTypeID, const std::string & name ) const;
	bool			commit();

	//typedef StringMap< DatabaseID > NameMap;
	  // StringMap cannot handle characters > 127, use std::map instead for
	  // wide string/unicode compatiblity
	typedef std::map< std::string, DatabaseID > NameMap;
	typedef std::vector< NameMap >				NameMapVec;
	typedef std::map< DatabaseID, DataSectionPtr > IDMap;

	DataSectionPtr	pDB_;
	NameMapVec		nameToIdMaps_;
	IDMap			idToData_;

	// Equivalent of bigworldLogOnMapping table in MySQL.
	struct LogOnMapping
	{
		std::string		password;
		EntityTypeID	typeID;
		std::string		entityName;	// called "recordName" in MySQL

		LogOnMapping() {}
		LogOnMapping( const std::string& pass, EntityTypeID type,
				const std::string& name ) :
			password( pass ), typeID( type ), entityName( name )
		{}
	};
	// The key is the "logOnName" column.
	typedef std::map< std::string, LogOnMapping > LogonMap;
	LogonMap		logonMap_;
	DataSectionPtr	pLogonMapSection_;

	/// Stores the maximum of the used player IDs. Used to allocate
	/// new IDs to new players if allowed.
	DatabaseID		maxID_;

	class ActiveSetEntry
	{
	public:
		ActiveSetEntry()
			{
				baseRef.addr.ip = 0;
				baseRef.addr.port = 0;
				baseRef.id = 0;
			}
		EntityMailBoxRef	baseRef;
	};

	typedef std::map< DatabaseID, ActiveSetEntry > ActiveSet;
	ActiveSet activeSet_;
	std::vector<ObjectID> spareIDs_;
	ObjectID nextID_;

	const EntityDefs*	pEntityDefs_;
	const EntityDefs*	pNewEntityDefs_;
};

#endif
