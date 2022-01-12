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
#include "gui_manager.hpp"
#include "gui_functor.hpp"
#include "gui_toolbar.hpp"
#include "gui_functor_cpp.hpp"
#include "gui_functor_option.hpp"
#include "gui_functor_datasection.hpp"
#include "gui_functor_python.hpp"
#include "resmgr/bwresource.hpp"
#include <stack>

BEGIN_GUI_NAMESPACE

InputDevice::~InputDevice(){}

std::set<unsigned short>& Item::unusedCommands()
{
	static std::set<unsigned short> unusedCommands;
	return unusedCommands;
}

std::map<std::string, ItemTypePtr>& Item::types()
{
	static std::map<std::string, ItemTypePtr> types;
	return types;
}

Item::Item( const std::string& type, const std::string& name, const std::string& displayName,
	const std::string& description,	const std::string& icon, const std::string& shortcutKey,
	const std::string& action, const std::string& updater, const std::string& imports, unsigned int commandID )
	:type_( type ), name_( name ), displayName_( displayName ), description_( description ),
	icon_( icon ), shortcutKey_( shortcutKey ), action_( action ), updater_( updater ),
	imports_( imports ), commandID_( commandID )
{
	if( commandID_ == GUI_COMMAND_END )
	{
		if( unusedCommands().empty() )
			throw 1;
		commandID_ = *unusedCommands().begin();
	}
	if( unusedCommands().find( commandID_ ) != unusedCommands().end() )
		unusedCommands().erase( unusedCommands().find( commandID_ ) );
}

Item::Item( DataSectionPtr section )
{
	if( !section )
		throw 1;
	type_ = section->readString( "type" );
	name_ = section->readString( "name" );
	displayName_ = section->readString( "displayName" );
	description_ = section->readString( "description" );
	icon_ = section->readString( "icon" );
	shortcutKey_ = section->readString( "shortcut" );
	action_ = section->readString( "action" );
	updater_ = section->readString( "updater" );
	commandID_ = (unsigned short)section->readInt( "commandID", GUI_COMMAND_END );
	if( commandID_ == GUI_COMMAND_END )
	{
		if( unusedCommands().empty() )
			throw 1;
		commandID_ = *unusedCommands().begin();
	}
	if( unusedCommands().find( commandID_ ) != unusedCommands().end() )
		unusedCommands().erase( unusedCommands().find( commandID_ ) );

	for( int i = 0; i < section->countChildren(); ++i )
	{
		values_[ section->childSectionName( i ) ] =
			section->readString( section->childSectionName( i ) );
	}

	std::vector<DataSectionPtr> items;
	section->openSections( "item", items );
	for( std::vector<DataSectionPtr>::iterator iter = items.begin(); iter != items.end(); ++iter )
		add( new Item( *iter ) );
	items.clear();
	section->openSections( "import", items );
	if( ! items.empty() )
		throw 1;// TODO: implement import
}

Item::~Item()
{
	if( commandID_ >= GUI_COMMAND_START && commandID_ < GUI_COMMAND_END )
		unusedCommands().insert( commandID_ );
}

void Item::add( ItemPtr item )
{
	if( isAncestor( item.getObject() ) )
		throw 1;
	subitems_.push_back( item );
	item->addParent( this );
}

void Item::add( const DataSectionPtr section )
{
	add( new Item( section ) );
}

void Item::insert( std::vector<ItemPtr>::size_type index, ItemPtr item )
{
	if( index >= num() )
		index = num();
	std::vector<ItemPtr>::iterator iter = subitems_.begin();
	std::advance( iter, index );
	subitems_.insert( iter, item );
}

void Item::remove( std::vector<ItemPtr>::size_type index )
{
	if( index < num() )
	{
		std::vector<ItemPtr>::iterator iter = subitems_.begin();
		std::advance( iter, index );
		subitems_.erase( iter );
	}
}

void Item::remove( ItemPtr item )
{
	std::vector<ItemPtr>::iterator iter = std::remove(
		subitems_.begin(), subitems_.end(), item );
	subitems_.erase( iter, subitems_.end() );
}

std::vector<ItemPtr>::size_type Item::num() const
{
	return subitems_.size();
}

ItemPtr Item::operator[]( std::vector<ItemPtr>::size_type index )
{
	return subitems_.at( index );
}

const ItemPtr Item::operator[]( std::vector<ItemPtr>::size_type index ) const
{
	return subitems_.at( index );
}

std::vector<Item*>::size_type Item::parentNum()
{
	return parents_.size();
}

Item* Item::parent( std::vector<Item*>::size_type index )
{
	return parents_.at( index );
}

void Item::addParent( Item* parent )
{
	parents_.push_back( parent );
}

bool Item::isAncestor( Item* item ) const
{
	for( std::vector<Item*>::const_iterator iter = parents_.begin();
		iter != parents_.end(); ++iter )
	{
		if( *iter == item || ( *iter )->isAncestor( item ) )
			return true;
	}
	return false;
}

std::string Item::pathTo( Item* item )
{
	if( item == this )
		return "";
	for( std::vector<Item*>::const_iterator iter = parents_.begin();
		iter != parents_.end(); ++iter )
	{
		if( item == NULL || ( *iter )->isAncestor( item ) )
			return ( *iter )->pathTo( item ) + "/" + name();
	}
	throw 1;
	return "";
}

const std::string& Item::type() const
{
	return type_;
}

const std::string& Item::name() const
{
	return name_;
}

std::string Item::displayName()
{
	return displayName_.empty() ? name_ : FunctorManager::instance()->text( displayName_, this );
}

std::string Item::description()
{
	return description_.empty() ? displayName() : description_;
}

const std::string& Item::action() const
{
	return action_;
}

const std::string& Item::updater() const
{
	return updater_;
}

const std::string& Item::shortcutKey() const
{
	return shortcutKey_;
}

unsigned short Item::commandID() const
{
	return commandID_;
}

bool Item::exist( const std::string& name )
{
	return values_.find( name ) != values_.end();
}

std::string Item::operator[]( const std::string& name )
{
	if( values_.find( name ) != values_.end() )
		return values_[ name ];
	return "";
}

void Item::set( const std::string& name, const std::string& value )
{
	if( !value.empty() )
		values_[ name ] = value;
	else if( values_.find( name ) != values_.end() )
		values_.erase( values_.find( name ) );
}

unsigned int Item::update()
{
	if( types().find( type() ) != types().end() )
		return types()[ type() ]->update( this );
	return 1;// otherwise it is disabled
}

bool Item::processInput( InputDevice* inputDevice )
{
	if( inputDevice->isKeyDown( shortcutKey() ) )
	{
		if( types().find( type() ) != types().end() )
			types()[ type() ]->shortcutPressed( this );
		return true;
	}
	for( unsigned int i = 0; i < num(); ++i )
		if( operator[]( i )->processInput( inputDevice ) )
		{
			changed();
			return true;
		}
	return false;
}

bool Item::act()
{
	if( types().find( type() ) != types().end() )
		return types()[ type() ]->act( this );
	return false;
}

void Item::changed()
{
	Manager::instance()->changed( this );
}

ItemPtr Item::operator()( const std::string& path )
{
	std::string current, rest;
	if( path.find( '/' ) != path.npos )
	{
		current = path.substr( 0, path.find( '/' ) );
		rest = path.substr( path.find( '/' ) + 1 );
	}
	else
		current = path;

	for( std::vector<ItemPtr>::iterator iter = subitems_.begin();
		iter != subitems_.end(); ++iter )
	{
		if( (*iter)->name() == current )
			return rest.empty() ? (*iter) : (*iter)->operator()( rest );
	}
	return NULL;
}

ItemPtr Item::findByCommandID( unsigned short commandID )
{
	if( commandID == commandID_ )
		return this;
	for( unsigned int i = 0; i < num(); ++i )
	{
		ItemPtr result = ( *this )[ i ]->findByCommandID( commandID );
		if( result )
			return result;
	}
	return NULL;
}

ItemPtr Item::findByName( const std::string& name )
{
	if( name == name_ )
		return this;

	for( unsigned int i = 0; i < num(); ++i )
	{
		ItemPtr result = ( *this )[ i ]->findByName( name );
		if( result )
			return result;
	}
	return NULL;
}

void Item::registerType( ItemTypePtr itemType )
{
	types()[ itemType->name() ] = itemType;
}

void Item::unregisterType( ItemTypePtr itemType )
{
	if( types().find( itemType->name() ) != types().end() &&
		types()[ itemType->name() ] == itemType )
		types().erase( types().find( itemType->name() ) );
}

void Item::init()
{
	for( unsigned short i = GUI_COMMAND_START; i < GUI_COMMAND_END; ++i )
		unusedCommands().insert( i );
	new BasicItemType( "ACTION" );
	class ChoiceItemType : public BasicItemType
	{
	public:
		ChoiceItemType( const std::string& typeName ) : BasicItemType( typeName )
		{}
		virtual void shortcutPressed( ItemPtr item )
		{
			if( item->update() )
			{
				for( unsigned int i = 0; i < item->num(); ++i )
					if( ( *item )[ i ]->update() )
					{
						( *item )[ ( i + 1 ) % item->num() ]->act();
						break;
					}
			}
		}
	};
	new ChoiceItemType( "CHOICE" );
	new ChoiceItemType( "EXPANDED_CHOICE" );
	class ToggleItemType : public BasicItemType
	{
	public:
		ToggleItemType() : BasicItemType( "TOGGLE" )
		{}
		virtual bool act( ItemPtr item )
		{
			return ( *item )[ 0 ]->update()	?
				( *item )[ 0 ]->act():
				( *item )[ 1 ]->act();
		}
	};
	new ToggleItemType();
	class ChildItemType : public BasicItemType
	{
	public:
		ChildItemType() : BasicItemType( "CHILD" )
		{}
		virtual bool act( ItemPtr item )
		{
			bool enabled = false;
			for( unsigned int i = 0; i < item->parentNum(); ++i )
			{
				Item* parent = item->parent( i );
				enabled = enabled || parent->update();
			}
			if( enabled && !item->action().empty() )
				return FunctorManager::instance()->act( item->action(), item );
			return false;
		}
	};
	new ChildItemType();
}

ItemType::~ItemType()
{}

BasicItemType::BasicItemType( const std::string& typeName ) : name_( typeName )
{
	Item::registerType( this );
}

BasicItemType::~BasicItemType()
{
}

const std::string& BasicItemType::name() const
{
	return name_;
}

bool BasicItemType::act( ItemPtr item )
{
	return ( !item->update() || item->action().empty() ) ? false : FunctorManager::instance()->act( item->action(), item );
}

unsigned int BasicItemType::update( ItemPtr item )
{
	return item->updater().empty() ? 1 : FunctorManager::instance()->update( item->updater(), item );
}

void BasicItemType::shortcutPressed( ItemPtr item )
{
	item->act();
}

Subscriber::Subscriber( const std::string& root ) : root_( root )
{}

const std::string& Subscriber::root() const
{
	return root_;
}

ItemPtr Subscriber::rootItem()
{
	return rootItem_;
}

void Subscriber::rootItem( ItemPtr item )
{
	rootItem_ = item;
}

Manager::Manager() : Item( "", "", "", "", "", "", "", "", "" )
{}

Manager::Manager( const DataSectionPtr section ) : Item( section )
{}

void Manager::add( SubscriberPtr subscriber )
{
	subscribers_[ subscriber->root() ].insert( subscriber );
	subscriber->rootItem( operator()( subscriber->root() ) );
	subscriber->changed( operator()( subscriber->root() ) );
}

void Manager::remove( SubscriberPtr subscriber )
{
	if( subscribers_.find( subscriber->root() ) != subscribers_.end() )
	{
		if( subscribers_[ subscriber->root() ].find( subscriber ) !=
			subscribers_[ subscriber->root() ].end() )
			subscribers_[ subscriber->root() ].erase(
				subscribers_[ subscriber->root() ].find( subscriber ) );
		if( !subscribers_[ subscriber->root() ].empty() )
			subscribers_.erase( subscribers_.find( subscriber->root() ) );
	}
}

ItemPtr Manager::operator()( const std::string& path )
{
	if( path.size() && path[ 0 ] == '/' )
		return Item::operator()( path.c_str() + 1 );
	return Item::operator()( path );
}

void Manager::act( unsigned short commandID )
{
	ItemPtr item = findByCommandID( commandID );
	if( item )
	{
		item->act();
		for( std::map<std::string,std::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
			iter != subscribers_.end(); ++iter )
		{
			for( std::set<SubscriberPtr>::iterator siter = iter->second.begin();
				siter != iter->second.end(); ++siter )
				( *siter )->changed( ( *siter )->rootItem() );
		}
	}
}

void Manager::update( unsigned short commandID )
{
	ItemPtr item = findByCommandID( commandID );
	if( item )
	{
		for( std::map<std::string,std::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
			iter != subscribers_.end(); ++iter )
		{
			for( std::set<SubscriberPtr>::iterator siter = iter->second.begin();
				siter != iter->second.end(); ++siter )
				( *siter )->changed( item );
		}
	}
}

void Manager::update()
{
	for( std::map<std::string,std::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
		iter != subscribers_.end(); ++iter )
	{
		for( std::set<SubscriberPtr>::iterator siter = iter->second.begin();
			siter != iter->second.end(); ++siter )
			( *siter )->changed( ( *siter )->rootItem() );
	}
}

void Manager::changed( ItemPtr item )
{
	for( std::map<std::string,std::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
		iter != subscribers_.end(); ++iter )
	{
		for( std::set<SubscriberPtr>::iterator siter = iter->second.begin();
			siter != iter->second.end(); ++siter )
			( *siter )->changed( item );
	}
}

bool Manager::resolveFileName( std::string& fileName )
{
	return BWResource::fileExists( fileName );
}

Manager* Manager::instance()
{
	static SmartPointer<Manager> instance = new Manager;
	return instance.getObject();
}

static long initCount = 0;

GUILifeManager::GUILifeManager()
{
	if( InterlockedExchangeAdd( &initCount, 1 ) == 0 )
	{
		Item::init();
		Toolbar::staticInit();
		FunctorManager::instance()->registerFunctor( CppFunctor::instance() );
		FunctorManager::instance()->registerFunctor( OptionFunctor::instance() );
		FunctorManager::instance()->registerFunctor( PythonFunctor::instance() );
		FunctorManager::instance()->registerFunctor( DataSectionFunctor::instance() );
	}
}

GUILifeManager::~GUILifeManager()
{
	if( InterlockedExchangeAdd( &initCount, -1 ) == 1 )
	{
		FunctorManager::instance()->unregisterFunctor( CppFunctor::instance() );
		FunctorManager::instance()->unregisterFunctor( OptionFunctor::instance() );
		FunctorManager::instance()->unregisterFunctor( PythonFunctor::instance() );
		FunctorManager::instance()->unregisterFunctor( DataSectionFunctor::instance() );
	}
}

END_GUI_NAMESPACE
