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

#include "cstdmf/debug.hpp"
#include "cstdmf/concurrency.hpp"

DECLARE_DEBUG_COMPONENT2( "Connect", 0 )

#include "servconn.hpp"

#include "math/vector3.hpp"
#include "network/portmap.hpp"
#include "network/encryption_filter.hpp"
#include "network/public_key_cipher.hpp"
#include "resmgr/bwresource.hpp"
//#include "cstdmf/dprintf.hpp"

#include "common/login_interface.hpp"
#include "common/baseapp_ext_interface.hpp"
#include "common/client_interface.hpp"
#include "cstdmf/memory_stream.hpp"

#ifndef CODE_INLINE
#include "servconn.ipp"
#endif

namespace
{
/// The number of microseconds to wait for a reply to the login request.
const int LOGIN_TIMEOUT = 8000000;					// 8 seconds

/// The number of seconds of inactivity before a connection is closed.
const float DEFAULT_INACTIVITY_TIMEOUT = 60.f;			// 60 seconds

/// How many times should the LoginApp login message be sent before giving up.
const int MAX_LOGINAPP_LOGIN_ATTEMPTS = 10;

// How often we send a LoginApp login message.
const int LOGINAPP_LOGIN_ATTEMPT_PERIOD = 1000000;	// 1 second

/// How many times should the BaseApp login message be sent before giving up.
const int MAX_BASEAPP_LOGIN_ATTEMPTS = 10;

// How often we send a BaseApp login message. A new port is used for each.
const int BASEAPP_LOGIN_ATTEMPT_PERIOD = 1000000; // 1 second

}

#ifdef WIN32
#pragma warning (disable:4355)	// this used in base member initialiser list
#endif


// -----------------------------------------------------------------------------
// Section: RetryingRequest
// -----------------------------------------------------------------------------


/**
 *  Constructor.
 */
RetryingRequest::RetryingRequest( LoginHandler & parent,
		const Mercury::Address & addr,
		const Mercury::InterfaceElement & ie,
		int retryPeriod,
		int timeoutPeriod,
		int maxAttempts,
		bool useParentNub ) :
	pParent_( &parent ),
	pNub_( NULL ),
	addr_( addr ),
	ie_( ie ),
	timerID_( Mercury::TIMER_ID_NONE ),
	done_( false ),
	retryPeriod_( retryPeriod ),
	timeoutPeriod_( timeoutPeriod ),
	numAttempts_( 0 ),
	numOutstandingAttempts_( 0 ),
	maxAttempts_( maxAttempts )
{
	if (useParentNub)
	{
		this->setNub( &parent.servConn().nub() );
	}

	parent.addChildRequest( this );
}


/**
 *  Destructor.
 */
RetryingRequest::~RetryingRequest()
{
	MF_ASSERT( timerID_ == Mercury::TIMER_ID_NONE );
}


/**
 *  This method handles a reply to one of our requests.
 */
void RetryingRequest::handleMessage( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	void * arg )
{
	if (!done_)
	{
		this->onSuccess( data );
		this->cancel();
	}

	--numOutstandingAttempts_;
	this->decRef();
}


/**
 *  This method handles an exception, usually a timeout.  Whatever happens, it
 *  indicates that one of our attempts is over.
 */
void RetryingRequest::handleException( const Mercury::NubException & exc,
	void * arg )
{
	// If something has gone terribly wrong, call the failure callback.
	if (!done_ && (exc.reason() != Mercury::REASON_TIMER_EXPIRED))
	{
		ERROR_MSG( "RetryingRequest::handleException( %s ): "
			"Request to %s failed (%s)\n",
			ie_.name(), addr_.c_str(), Mercury::reasonToString( exc.reason() ) );

		this->onFailure( exc.reason() );
		this->cancel();
	}

	--numOutstandingAttempts_;

	// If the last attempt has failed, we're done.
	if (!done_ && numOutstandingAttempts_ == 0)
	{
		// If this request had a maxAttempts of 1, we assume that retrying is
		// taking place by spawning multiple instances of this request (a la
		// BaseAppLoginRequest), so we don't emit this error message.
		if (maxAttempts_ > 1)
		{
			ERROR_MSG( "RetryingRequest::handleException( %s ): "
				"Final attempt of %d has failed (%s), aborting\n",
				ie_.name(), maxAttempts_,
				Mercury::reasonToString( exc.reason() ) );
		}

		this->onFailure( exc.reason() );
		this->cancel();
	}

	this->decRef();
}


/**
 *  This method handles an internally scheduled timeout, which causes another
 *  attempt to be sent.
 */
int RetryingRequest::handleTimeout( Mercury::TimerID id, void * arg )
{
	this->send();
	return 0;
}


/**
 *  This method sets the nub to be used by this object.  You cannot call send()
 *  until you have called this.
 */
void RetryingRequest::setNub( Mercury::Nub * pNub )
{
	MF_ASSERT( pNub_ == NULL );
	pNub_ = pNub;
	timerID_ = pNub_->registerTimer( retryPeriod_, this );
}


/**
 *  This method sends the request once.  This method should be called as the
 *  last statement in the constructor of a derived class.
 */
void RetryingRequest::send()
{
	if (done_) return;

	if (numAttempts_ < maxAttempts_)
	{
		++numAttempts_;

		Mercury::Bundle bundle;
		bundle.startRequest( ie_, this, NULL, timeoutPeriod_,
			Mercury::RELIABLE_NO );

		this->addRequestArgs( bundle );

		// Calling send may decrement this if an exception occurs.
		RetryingRequestPtr pThis = this;

		this->incRef();
		++numOutstandingAttempts_;

		pNub_->send( addr_, bundle );
	}
}


/**
 *  This method removes this request from the parent's childRequests_ list,
 *  which means it will be destroyed as soon as all of its outstanding requests
 *  have either been replied to (and ignored) or timed out.
 */
void RetryingRequest::cancel()
{
	if (timerID_ != Mercury::TIMER_ID_NONE)
	{
		pNub_->cancelTimer( timerID_ );
		timerID_ = Mercury::TIMER_ID_NONE;
	}
	pParent_->removeChildRequest( this );
	done_ = true;
}


// -----------------------------------------------------------------------------
// Section: LoginRequest
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
LoginRequest::LoginRequest( LoginHandler & parent ) :
	RetryingRequest( parent, parent.loginAddr(), LoginInterface::login )
{
	this->send();
}


void LoginRequest::addRequestArgs( Mercury::Bundle & bundle )
{
	LogOnParamsPtr pParams = pParent_->pParams();

	bundle << LOGIN_VERSION;

	if (!pParams->addToStream( bundle, LogOnParams::HAS_ALL,
			&pParent_->servConn().publicKey() ))
	{
		ERROR_MSG( "LoginRequest::addRequestArgs: "
			"Failed to assemble login bundle\n" );

		pParent_->onFailure( Mercury::REASON_CORRUPTED_PACKET );
	}
}


void LoginRequest::onSuccess( BinaryIStream & data )
{
	pParent_->onLoginReply( data );
}


void LoginRequest::onFailure( Mercury::Reason reason )
{
	pParent_->onFailure( reason );
}


// -----------------------------------------------------------------------------
// Section: BaseAppLoginRequest
// -----------------------------------------------------------------------------


/**
 *  Constructor.
 */
BaseAppLoginRequest::BaseAppLoginRequest( LoginHandler & parent ) :
	RetryingRequest( parent, parent.baseAppAddr(),
		BaseAppExtInterface::baseAppLogin,
		DEFAULT_RETRY_PERIOD,
		DEFAULT_TIMEOUT_PERIOD,
		/* maxAttempts: */ 1,
		/* useParentNub: */ false ),
	attempt_( pParent_->numBaseAppLoginAttempts() )
{
	ServerConnection & servConn = pParent_->servConn();

	// Each instance has its own Nub.  We need to do this to cope with strange
	// multi-level NATing issues in China.
	Mercury::Nub* pNub = new Mercury::Nub();
	pNub->shouldAutoCreateAnonymousChannels( false );
	this->setNub( pNub );

	// MF_ASSERT( pNub->socket() != -1 );

	pChannel_ = new Mercury::Channel(
		*pNub_, pParent_->baseAppAddr(),
		Mercury::Channel::EXTERNAL,
		/* minInactivityResendDelay: */ 1.0,
		servConn.pFilter().getObject() );

	// Set the servconn as the bundle primer
	pChannel_->bundlePrimer( servConn );

	// The channel is irregular until we get cellPlayerCreate (i.e. entities
	// enabled).
	pChannel_->isIrregular( true );

	// This temporary nub must serve all the interfaces that the main nub
	// serves, because if the initial downstream packets are lost and the
	// reply to the baseAppLogin request actually comes back as a piggyback
	// on a packet with other ClientInterface messages, this nub will need
	// to know how to process them.
	servConn.registerInterfaces( *pNub_ );

	// Make use of the main nub's socket for the first one sent. It simplifies
	// things if we switchSockets here and switchSockets back if we win.
	if (attempt_ == 0)
	{
		pNub_->switchSockets( &servConn.nub() );
	}

	// Register as a slave to the main nub.
	servConn.nub().registerChildNub( pNub_ );

	this->send();
}


/**
 *  Destructor.
 */
BaseAppLoginRequest::~BaseAppLoginRequest()
{
	// The winner's pChannel_ will be NULL since it has been transferred to the
	// servconn.
	// delete pChannel_;
	if (pChannel_)
	{
		pChannel_->destroy();
		pChannel_ = NULL;
	}

	// Transfer the temporary Nub to the LoginHandler to clean up later.  We
	// can't just delete the Nub now because it is halfway through processing
	// when this destructor is called.
	pParent_->addCondemnedNub( pNub_ );
	pNub_ = NULL;
}


/**
 *  This method creates another instance of this class, instead of the default
 *  behaviour which is to just resend another request from this object.
 */
int BaseAppLoginRequest::handleTimeout( Mercury::TimerID id, void * arg )
{
	// Each request should only spawn one other request
	pNub_->cancelTimer( timerID_ );
	timerID_ = Mercury::TIMER_ID_NONE;

	if (!done_)
	{
		pParent_->sendBaseAppLogin();
	}
	return 0;
}


/**
 *  Streams on required args for baseapp login.
 */
void BaseAppLoginRequest::addRequestArgs( Mercury::Bundle & bundle )
{
	// Send the loginKey and number of attempts (for debugging reasons)
	bundle << pParent_->replyRecord().sessionKey << attempt_;
}


/**
 *	This method handles the reply to the baseAppLogin message. Getting this
 *	called means that this is the winning BaseAppLoginHandler.
 */
void BaseAppLoginRequest::onSuccess( BinaryIStream & data )
{
	SessionKey sessionKey = 0;
	data >> sessionKey;
	pParent_->onBaseAppReply( this, sessionKey );

	// Forget about our Channel because it has been transferred to the
	// ServerConnection.
	pChannel_ = NULL;
}


/**
 * This method is an override of the equivalant method in retryingrequest.
 * Because these things keep their own nubs it is important to cancel the
 * request now so that when the parent LoginHandler is finally dereferenced
 * it not be via one of these timing out since that will happen in a member
 * function of one of the child nubs that will be freed.
 */
void BaseAppLoginRequest::cancel()
{
	this->RetryingRequest::cancel();

	pNub_->cancelReplyMessageHandler( this );
}


// -----------------------------------------------------------------------------
// Section: LoginHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
LoginHandler::LoginHandler(
		ServerConnection* pServerConnection, LogOnStatus loginNotSent ) :
	loginAppAddr_( Mercury::Address::NONE ),
	baseAppAddr_( Mercury::Address::NONE ),
	pParams_( NULL ),
	pServerConnection_( pServerConnection ),
	done_( loginNotSent != LogOnStatus::NOT_SET ),
	status_( loginNotSent ),
	numBaseAppLoginAttempts_( 0 )
{
}


LoginHandler::~LoginHandler()
{
	// Destroy any condemned Nubs that have been left to us
	for (CondemnedNubs::iterator iter = condemnedNubs_.begin();
		 iter != condemnedNubs_.end(); ++iter)
	{
		delete *iter;
	}
}


void LoginHandler::start( const Mercury::Address & loginAppAddr,
	LogOnParamsPtr pParams )
{
	loginAppAddr_ = loginAppAddr;
	pParams_ = pParams;
	this->sendLoginAppLogin();
}


void LoginHandler::finish()
{
	// Clear out all child requests
	while (!childRequests_.empty())
	{
		(*childRequests_.begin())->cancel();
	}
	pServerConnection_->nub().breakProcessing();
	done_ = true;
}


/**
 *  This method sends the login request to the server.
 */
void LoginHandler::sendLoginAppLogin()
{
	new LoginRequest( *this );
}


/**
 *	This method handles the login reply message from the LoginApp.
 */
void LoginHandler::onLoginReply( BinaryIStream & data )
{
	data >> status_;

	if (status_ == LogOnStatus::LOGGED_ON)
	{
		// The reply record is symmetrically encrypted.
		MemoryOStream clearText;
		pServerConnection_->pFilter()->decryptStream( data, clearText );
		clearText >> replyRecord_;

		// Correct sized reply
		if (!data.error())
		{
			baseAppAddr_ = replyRecord_.serverAddr;
			this->sendBaseAppLogin();
			errorMsg_ = "";
		}
		else
		{
			ERROR_MSG( "LoginHandler::handleMessage: "
				"Got reply of unexpected size (%d)\n",
				data.remainingLength() );

			status_ = LogOnStatus::CONNECTION_FAILED;
			errorMsg_ = "Mercury::REASON_CORRUPTED_PACKET";

			this->finish();
		}
	}
	else
	{
		data >> errorMsg_;

		if (errorMsg_.empty())	// this really shouldn't happen
		{
			if (status_ == LogOnStatus::LOGIN_CUSTOM_DEFINED_ERROR)
			{
				errorMsg_ = "Unspecified error.";
			}
			else
			{
				errorMsg_ = "Unelaborated error.";
			}
		}

		this->finish();
	}
}


/**
 *	This method sends a login request to the BaseApp. There can be multiple
 *	login requests outstanding at any given time (sent from different sockets).
 *	Only one will win.
 */
void LoginHandler::sendBaseAppLogin()
{
	if (numBaseAppLoginAttempts_ < MAX_BASEAPP_LOGIN_ATTEMPTS)
	{
		new BaseAppLoginRequest( *this );
		++numBaseAppLoginAttempts_;
	}
	else
	{
		status_ = LogOnStatus::CONNECTION_FAILED;
		errorMsg_ =
			"Unable to connect to BaseApp: "
			"A NAT or firwall error may have occured?";

		this->finish();
	}
}


/**
 *	This method is called when a reply to baseAppLogin is received from the
 *	BaseApp (or an exception occurred - likely a timeout).
 *
 *	The first successful reply wins and we do not care about the rest.
 */
void LoginHandler::onBaseAppReply( BaseAppLoginRequestPtr pHandler,
	SessionKey sessionKey )
{
	Mercury::Nub * pMainNub = &pServerConnection_->nub();

	// Make this successful socket the main nub's socket.
	pHandler->nub().switchSockets( pMainNub );

	// Transfer the successful channel to the main nub and the servconn.
	pHandler->channel().switchNub( pMainNub );
	pServerConnection_->channel( pHandler->channel() );

	// This is the session key that authenticate message should send.
	replyRecord_.sessionKey = sessionKey;
	pServerConnection_->sessionKey( sessionKey );

	this->finish();
}


/**
 *	This method handles a network level failure.
 */
void LoginHandler::onFailure( Mercury::Reason reason )
{
	status_ = LogOnStatus::CONNECTION_FAILED;
	errorMsg_ = "Mercury::";
	errorMsg_ += Mercury::reasonToString( reason );

	this->finish();
}


/**
 *  This method add a RetryingRequest to this LoginHandler.
 */
void LoginHandler::addChildRequest( RetryingRequestPtr pRequest )
{
	childRequests_.insert( pRequest );
}


/**
 *  This method removes a RetryingRequest from this LoginHandler.
 */
void LoginHandler::removeChildRequest( RetryingRequestPtr pRequest )
{
	childRequests_.erase( pRequest );
}


/**
 *  This method adds a condemned Nub to this LoginHandler.  This is used by
 *  BaseAppLoginRequests to clean up their Nubs as they are unable to do it
 *  themselves.
 */
void LoginHandler::addCondemnedNub( Mercury::Nub * pNub )
{
	condemnedNubs_.push_back( pNub );
}


// -----------------------------------------------------------------------------
// Section: EntityMessageHandler declaration
// -----------------------------------------------------------------------------

/**
 *	This class is used to handle handleDataFromServer message from the server.
 */
class EntityMessageHandler : public Mercury::InputMessageHandler
{
	public:
		EntityMessageHandler();

	protected:
		/// This method handles messages from Mercury.
		virtual void handleMessage( const Mercury::Address & /* srcAddr */,
			Mercury::UnpackedMessageHeader & msgHeader,
			BinaryIStream & data );
};

EntityMessageHandler g_entityMessageHandler;


// -----------------------------------------------------------------------------
// Section: ServerConnection implementation
// -----------------------------------------------------------------------------

struct ProxyDataSegment
{
	struct ProxyDataSegment * prev;
	int		length;
	uint8	seq;
	uint8	data[1];
};
struct ProxyDataTrack
{
	ProxyDataSegment * last;
	uint16	pdid;
	uint16	segCountTgtMod;
	int		segCount;
	int		accLength;
};


static bool s_requestQueueEnabled;


/**
 *	This static members stores the number of updates per second to expect from
 *	the server.
 */
float ServerConnection::s_updateFrequency_ = 10.f;

/**
 *	Constructor.
 */
ServerConnection::ServerConnection() :
	sessionKey_( 0 ),
	pHandler_( NULL ),
	pTime_( NULL ),
	lastTime_( 0.0 ),
	minSendInterval_( 1.01/20.0 ),
	nub_(),
	pChannel_( NULL ),
	pFilter_( new Mercury::EncryptionFilter() ),
	inactivityTimeout_( DEFAULT_INACTIVITY_TIMEOUT ),
	// see also initialiseConnectionState
	FIRST_AVATAR_UPDATE_MESSAGE(
		ClientInterface::avatarUpdateNoAliasFullPosYawPitchRoll.id() ),
	LAST_AVATAR_UPDATE_MESSAGE(
		ClientInterface::avatarUpdateAliasNoPosNoDir.id() ),
	requestBlockedCB_( NULL ),
	publicKey_( /* hasPriv	ate: */ false )
{
	tryToReconfigurePorts_ = false;
	this->initialiseConnectionState();

	memset( &digest_, 0, sizeof( digest_ ) );
	latencyTab_ = new SMA<float>(static_cast<int>(s_updateFrequency_ * 10));
	for (uint i = 0; i < 256; i++)
	{
		timeSent_[i] = 0;
	}

	nub_.shouldAutoCreateAnonymousChannels( false );
	
	// set once-off reliability resend period
	nub_.onceOffResendPeriod( CLIENT_ONCEOFF_RESEND_PERIOD );
	nub_.onceOffMaxResends( CLIENT_ONCEOFF_MAX_RESENDS );
}


/**
 *	Destructor
 */
ServerConnection::~ServerConnection()
{
	// disconnect if we didn't already do so
	this->disconnect();

	if (latencyTab_)
	{
		delete latencyTab_;
		latencyTab_ = NULL;
	}

	// Destroy our channel.  This must be done immediately (i.e. we can't
	// just condemn it) because the ~Channel() must execute before ~Nub().
	if (pChannel_)
	{
		// delete pChannel_;
		pChannel_->destroy();
		pChannel_ = NULL;
	}
}


/**
 *	This private method initialises or reinitialises our state related to a
 *	connection. It should be called before a new connection is made.
 */
void ServerConnection::initialiseConnectionState()
{
	id_ = ObjectID(-1);
	bandwidthFromServer_ = 0;

	lastSendTime_ = 0.0;

	everReceivedPacket_ = false;
	entitiesEnabled_ = false;

	serverTimeHandler_ = ServerTimeHandler();

	sendingSequenceNumber_ = 0;
	receivingSequenceNumber_ = 0;
	sendingSequenceIncrement_ = false;
	lastRequiredExplicitSeqNum_ = 0;
	alwaysSendExplicitUpdates_ = false;

	memset( idAlias_, 0, sizeof( idAlias_ ) );

	nextEntitysViewportID_ = NO_SPACE_VIEWPORT_ID;
	nextEntitysVehicleID_ = ObjectID(-1);
	spaceViewports_.clear();
	defaultViewportIDs_.clear();
	spaceSeenCount_.clear();
	personalSpaceViewport_ = 0;
	sentOnGround_.clear();

	latestVersion_ = uint32(-1);
	impendingVersion_ = uint32(-1);
	imminentVersion_ = uint32(-1);
	previousVersion_ = uint32(-1);
	versForCallIsOld_ = false;
	sendResourceVersionTag_ = false;
}


/**
 *  This method loads a key from a resource path.
 */
bool ServerConnection::setKeyFromResource( const std::string & path )
{
	DataSectionPtr pSection = BWResource::openSection( path );

	if (pSection)
	{
		BinaryPtr pBinData = pSection->asBinary();
		std::string keyStr( pBinData->cdata(), pBinData->len() );

		return publicKey_.setKey( keyStr );
	}
	else
	{
		ERROR_MSG( "ServerConnection::setKeyFromResource: "
			"Couldn't load private key from non-existent file %s\n",
			path.c_str() );

		return false;
	}
}


/**
 *	This private helper method registers the mercury interfaces with the
 *	provided nub, so that it will serve them.  This should be done for every Nub
 *	that might receive messages from the server, which includes the temporary
 *	Nubs used in the BaseApp login process.
 */
void ServerConnection::registerInterfaces( Mercury::Nub & nub )
{
	ClientInterface::registerWithNub( nub );

	for (Mercury::MessageID id = 128; id != 255; id++)
	{
		nub.serveInterfaceElement( ClientInterface::entityMessage,
			id, &g_entityMessageHandler );
	}

	// Message handlers have access to the nub that the message arrived on. Set
	// the ServerConnection here so that the message handler knows which one to
	// deliver the message to. (This is used by bots).
	nub.pExtensionData( this );
}


THREADLOCAL( bool ) g_isMainThread( false );


/**
 *	This method logs in to the named server with the input username and
 *	password.
 *
 *	@param pHandler		The handler to receive messages from this connection.
 *	@param serverName	The name of the server to connect to.
 *	@param username		The username to log on with.
 *	@param password		The password to log on with.
 *	@param port			The port that the server should talk to us on.
 */
LogOnStatus ServerConnection::logOn( ServerMessageHandler * pHandler,
	const char * serverName,
	const char * username,
	const char * password,
	const char * publicKeyPath,
	uint16 port )
{
	LoginHandlerPtr pLoginHandler = this->logOnBegin(
		serverName, username, password, publicKeyPath, port );

	// Note: we could well get other messages here and not just the
	// reply (if the proxy sends before we get the reply), but we
	// don't add ourselves to the master/slave system before then
	// so we won't be found ... and in the single servconn case the
	// channel (data) won't be created until then so the the intermediate
	// messages will be resent.

	while (!pLoginHandler->done())
	{
		try
		{
			nub_.processContinuously();
		}
		catch (Mercury::NubException& ex)
		{
			WARNING_MSG(
				"servconn::logOn: Got Mercury Exception %d\n", ex.reason() );
		}
	}

	LogOnStatus status = this->logOnComplete( pLoginHandler, pHandler );

	if (status == LogOnStatus::LOGGED_ON)
	{
		this->enableEntities( /*impendingVersionDownloaded:*/ false );
	}

	return status;
}


/**
 *	This method begins an asynchronous login
 */
LoginHandlerPtr ServerConnection::logOnBegin(
	const char * serverName,
	const char * username,
	const char * password,
	const char * publicKeyPath,
	uint16 port )
{
	LogOnParamsPtr pParams = new LogOnParams( username, password,
		pFilter_->key() );

	pParams->digest( this->digest() );

	g_isMainThread = true;

	// make sure we are not already logged on
	if (this->online())
	{
		// __glenc__ TODO: Why the hell are we using these magic numbers?
		return new LoginHandler( this, LogOnStatus::ALREADY_ONLINE_LOCALLY );
	}

	// Clean up old state if we have been connected before
	this->initialiseConnectionState();

	TRACE_MSG( "ServerConnection::logOnBegin: "
		"server:%s username:%s\n", serverName, pParams->username().c_str() );

	username_ = pParams->username();

	// Register the interfaces if they have not already been registered.
	this->registerInterfaces( nub_ );

	// Find out where we want to log in to
	uint16 loginPort = port ? port : PORT_LOGIN;

	const char * serverPort = strchr( serverName, ':' );
	std::string serverNameStr;
	if (serverPort != NULL)
	{
		loginPort = atoi( serverPort+1 );
		serverNameStr.assign( serverName, serverPort - serverName );
		serverName = serverNameStr.c_str();
	}

	Mercury::Address loginAddr( 0, htons( loginPort ) );
	if (Endpoint::convertAddress( serverName, (u_int32_t&)loginAddr.ip ) != 0 ||
		loginAddr.ip == 0)
	{
		return new LoginHandler( this, LogOnStatus::DNS_LOOKUP_FAILED );
	}

	// Use a standard key path if none is provided
	if (!publicKeyPath || *publicKeyPath == '\0')
	{
		publicKeyPath = "loginapp.pubkey";
	}

	// Read the public key we're using to encrypt the login credentials
	if (!this->setKeyFromResource( publicKeyPath ))
	{
		return new LoginHandler( this, LogOnStatus::PUBLIC_KEY_LOOKUP_FAILED );
	}

	// Create a LoginHandler and start the handshake
	LoginHandlerPtr pLoginHandler = new LoginHandler( this );
	pLoginHandler->start( loginAddr, pParams );
	return pLoginHandler;
}


/**
 *	This method completes an asynchronous login.
 *
 *	Note: Don't call this from within processing the bundle that contained
 *	the reply if you have multiple ServConns, as it could stuff up processing
 *	for another of the ServConns.
 */
LogOnStatus ServerConnection::logOnComplete(
	LoginHandlerPtr pLoginHandler,
	ServerMessageHandler * pHandler )
{
	LogOnStatus status = LogOnStatus::UNKNOWN_ERROR;
	uint32 latestVersion = uint32(-1), impendingVersion = uint32(-1);

	MF_ASSERT( pLoginHandler != NULL );

	if (this->online())
	{
		status = LogOnStatus::ALREADY_ONLINE_LOCALLY;
	}

	status = pLoginHandler->status();

	if ((status == LogOnStatus::LOGGED_ON) &&
			!this->online())
	{
		WARNING_MSG( "ServerConnection::logOnComplete: "
				"Already logged off\n" );

		status = LogOnStatus::CANCELLED;
		errorMsg_ = "Already logged off";
	}

	if (status == LogOnStatus::LOGGED_ON)
	{
		DEBUG_MSG( "ServerConnection::logOn: status==LOGGED_ON\n" );

		const LoginReplyRecord & result = pLoginHandler->replyRecord();

		latestVersion = result.latestVersion;
		impendingVersion = result.impendingVersion;

		DEBUG_MSG( "ServerConnection::logOn: from: %s\n", (char*)nub_.address() );
		DEBUG_MSG( "ServerConnection::logOn: to:   %s\n", (char*)result.serverAddr );

		// We establish our channel to the BaseApp in
		// BaseAppLoginHandler::handleMessage - this is just a sanity check.
		if (result.serverAddr != this->addr())
		{
			char winningAddr[ 256 ];
			strncpy( winningAddr, this->addr().c_str(), sizeof( winningAddr ) );

			WARNING_MSG( "ServerConnection::logOnComplete: "
				"BaseApp address on login reply (%s) differs from winning "
				"BaseApp reply (%s)\n",
				result.serverAddr.c_str(), winningAddr );
		}
	}
	else if (status == LogOnStatus::CONNECTION_FAILED)
	{
		ERROR_MSG( "ServerConnection::logOnComplete: Logon failed (%s)\n",
				pLoginHandler->errorMsg().c_str() );
		status = LogOnStatus::CONNECTION_FAILED;
		errorMsg_ = pLoginHandler->errorMsg();
	}
	else if (status == LogOnStatus::DNS_LOOKUP_FAILED)
	{
		errorMsg_ = "DNS lookup failed";
		INFO_MSG( "ServerConnection::logOnComplete: Logon failed\n\t%s\n",
				errorMsg_.c_str() );
	}
	else
	{
		errorMsg_ = pLoginHandler->errorMsg();
		INFO_MSG( "ServerConnection::logOnComplete: Logon failed\n\t%s\n",
				errorMsg_.c_str() );
	}

	// Release the reply handler
	pLoginHandler = NULL;

	// Get out if we didn't log on
	if (status != LogOnStatus::LOGGED_ON)
	{
		return status;
	}

	// Yay we logged on!

	id_ = 0;
	latestVersion_ = latestVersion;
	impendingVersion_ = impendingVersion;
	imminentVersion_ = latestVersion;

	s_requestQueueEnabled = true;

	// Send an initial packet to the proxy to open up a hole in any
	// firewalls on our side of the connection.  We have to re-prime the
	// outgoing bundle since sessionKey_ would not have been set when it was
	// primed in our ctor.
	this->primeBundle( this->bundle() );
	this->send();

	// DEBUG_MSG( "ServerConnection::logOn: sent initial message to server\n" );

	// Install the user's server message handler until we disconnect
	// (serendipitous leftover argument from when it used to be called
	// here to create the player entity - glad I didn't clean that up :)
	pHandler_ = pHandler;

	// Disconnect if we do not receive anything for this number of seconds.
	this->channel().startInactivityDetection( inactivityTimeout_ );

	// Treat this as the first sent position.
	/*
	SpaceViewport & svp = spaceViewports_[0];
	svp.gatewaySrcID_ = 0;
	svp.gatewayDstID_ = avatarID;
	svp.spaceID_ = avatarSpaceID;
	svp.selfControlled_ = true;
	svp.sentPositions_[ 0 ] = avatarPos;
	spaceSeenCount_[ avatarSpaceID ] = 1;
	*/

	return status;
}


/**
 *	This method enables the receipt of entity and related messages from the
 *	server, i.e. the bulk of game traffic. The server does not start sending
 *	to us until we are ready for it. This should be called shortly after login.
 */
void ServerConnection::enableEntities( bool impendingVersionDownloaded )
{
	// Ok cell, we are ready for entity updates now.
	BaseAppExtInterface::enableEntitiesArgs & args =
		BaseAppExtInterface::enableEntitiesArgs::start(
			this->bundle(), /*isReliable:*/true );

	args.presentVersion = latestVersion_;
	args.futureVersion = impendingVersionDownloaded ?
		impendingVersion_ : latestVersion_;

	DEBUG_MSG( "ServerConnection::enableEntities: "
		"Enabling entities with version %d\n", args.futureVersion );

	this->send();

	entitiesEnabled_ = true;
}


/**
 * 	This method returns whether or not we are online with the server.
 *	If a login is in progress, it will still return false.
 */
bool ServerConnection::online() const
{
	return pChannel_ != NULL;
}


/**
 *	This method removes the channel. This should be called when an exception
 *	occurs during processing input, or when sending data fails. The layer above
 *	can detect this by checking the online method once per frame. This is safer
 *	than using a callback, since a disconnect may happen at inconvenient times,
 *	e.g. when sending.
 */
void ServerConnection::disconnect( bool informServer )
{
	if (!this->online()) return;

	if (informServer)
	{
		BaseAppExtInterface::disconnectClientArgs::start( this->bundle(),
			false ).reason = 0; // reason not yet used.

		this->channel().send();
	}

	// Destroy our channel
	// delete pChannel_;
	if (pChannel_ != NULL)
	{
		pChannel_->destroy();
		pChannel_ = NULL;
	}

	// finish all res upd requests and set progress to an invalid value
	this->cancelRURequests();

	// clear in-progress proxy data downloads
	for (uint i = 0; i < proxyData_.size(); ++i)
		delete proxyData_[i];
	proxyData_.clear();

	// forget the handler and the session key
	pHandler_ = NULL;
	sessionKey_ = 0;
}


/**
 *  This method returns the ServerConnection's channel.
 */
Mercury::Channel & ServerConnection::channel()
{
	// Don't call this before this->online() is true.
	MF_ASSERT( pChannel_ );
	return *pChannel_;
}


const Mercury::Address & ServerConnection::addr() const
{
	MF_ASSERT( pChannel_ );
	return pChannel_->addr();
}


/**
 * 	This method adds a message to the server to inform it of the
 * 	new position and direction (and the rest) of an entity under our control.
 *	The server must have explicity given us control of that entity first.
 *
 *	@param id			ID of entity.
 *	@param spaceID		ID of the space the entity is in.
 *	@param vehicleID	ID of the innermost vehicle the entity is on.
 * 	@param pos			Local position of the entity.
 * 	@param yaw			Local direction of the entity.
 * 	@param pitch		Local direction of the entity.
 * 	@param roll			Local direction of the entity.
 * 	@param onGround		Whether or not the entity is on terrain (if present).
 * 	@param globalPos	Approximate global position of the entity
 */
void ServerConnection::addMove( ObjectID id, SpaceID spaceID,
	ObjectID vehicleID, const Vector3 & pos, float yaw, float pitch, float roll,
	bool onGround, const Vector3 & globalPos )
{
	if (this->offline())
		return;

	// make sure this is an entity we know about
	if (id != id_ &&
		defaultViewportIDs_.find( id ) == defaultViewportIDs_.end())
	{
		ERROR_MSG( "ServerConnection::addMove: Tried to add a move for "
			"unseen entity id %d\n", id );
		return;
	}

	// find out where we last told the server that it was
	ObjectID oldVehicleID;
	SpaceViewportID svid = this->svidFollow( id, oldVehicleID );
	SpaceID oldSpaceID = spaceViewports_[ svid ].spaceID_;
	std::map<ObjectID,bool>::iterator oog = sentOnGround_.find( id );
	bool oldOnGround = (oog != sentOnGround_.end()) ? oog->second : !onGround;

	// make sure we have control of this entity (new in protocol v27)
	if (oog == sentOnGround_.end())
	{
		ERROR_MSG( "ServerConnection::addMove: Tried to add a move for "
			"entity id %d that we do not control\n", id );
		// be assured that even if we did not return here the server
		// would not accept the position update regardless!
		return;
	}

	// see if we're going to tell it the same thing
	bool spaceAndVehicleAndOnGroundEqual = true;
	spaceAndVehicleAndOnGroundEqual &= oldSpaceID == spaceID;
	spaceAndVehicleAndOnGroundEqual &= oldVehicleID == vehicleID;
	spaceAndVehicleAndOnGroundEqual &= oldOnGround == onGround;

	// if it's recently changed coordinate systems then we'll have
	// to send an explicit update instead of an implicit one.
	bool sendExplicitUpdate = alwaysSendExplicitUpdates_ |
		(!spaceAndVehicleAndOnGroundEqual);

	Coord			* pPos = NULL;
	YawPitchRoll	* pDir = NULL;
	RelPosRef		* pRef = NULL;

	Mercury::Bundle & bundle = this->bundle();

	if (id == id_)
	{
		if (!sendExplicitUpdate)
		{
			BaseAppExtInterface::avatarUpdateImplicitArgs & upArgs =
				BaseAppExtInterface::avatarUpdateImplicitArgs::start(
						bundle, false );

			pPos = &upArgs.pos;
			pDir = &upArgs.dir;
			pRef = &upArgs.posRef;
		}
		else
		{
			BaseAppExtInterface::avatarUpdateExplicitArgs & upArgs =
				BaseAppExtInterface::avatarUpdateExplicitArgs::start( bundle,
						false );

			upArgs.spaceID = BW_HTONL( spaceID );
			upArgs.vehicleID = BW_HTONL( vehicleID );
			upArgs.onGround = onGround;

			pPos = &upArgs.pos;
			pDir = &upArgs.dir;
			pRef = &upArgs.posRef;
		}
	}
	else
	{
		if (!sendExplicitUpdate)
		{
			BaseAppExtInterface::avatarUpdateWardImplicitArgs & upArgs =
				BaseAppExtInterface::avatarUpdateWardImplicitArgs::start(
						bundle, false );

			upArgs.ward = BW_HTONL( id );

			pPos = &upArgs.pos;
			pDir = &upArgs.dir;
		}
		else
		{
			BaseAppExtInterface::avatarUpdateWardExplicitArgs & upArgs =
				BaseAppExtInterface::avatarUpdateWardExplicitArgs::start(
						bundle, false );

			upArgs.ward = BW_HTONL( id );
			upArgs.spaceID = BW_HTONL( spaceID );
			upArgs.vehicleID = BW_HTONL( vehicleID );
			upArgs.onGround = onGround;

			pPos = &upArgs.pos;
			pDir = &upArgs.dir;
		}
	}

	*pPos = pos;
	pDir->set( yaw, pitch, roll );

	// set the upstream relative position reference info if it's appropriate
	if (pRef != NULL)
	{
		Vector3 globalCube;
		globalCube.x = floorf( globalPos.x / 256.f ) * 256.f;
		globalCube.y = floorf( globalPos.y / 256.f ) * 256.f;
		globalCube.z = floorf( globalPos.z / 256.f ) * 256.f;
		pRef->x_ = uint8( floorf( globalPos.x ) - globalCube.x );
		pRef->y_ = uint8( floorf( globalPos.y ) - globalCube.y );
		pRef->z_ = uint8( floorf( globalPos.z ) - globalCube.z );
		pRef->seqNum_ = sendingSequenceNumber_;

		// TODO: Send reference posns for wards that are gateway dsts.
		// This can wait until the downstream protocol is changed to work
		// with movable gateway dsts - i.e. where you have to specify
		// a reference position for updates in viewports outside the personal
		// viewport. And yes, there probably will be acks :)
		spaceViewports_[ svid ].sentPositions_[ pRef->seqNum_ ] = globalPos;

		sendingSequenceIncrement_ = true;
	}

	// this code remains outside the 'if' above because for timing reasons
	// although it seems as if it could go in 'send' just as well
	timeSent_[ sendingSequenceNumber_ ] = timestamp();

	// we will have to keep sending explicit avatar updates until the
	// server has sent us back this (or a later) sequence number
	if (!spaceAndVehicleAndOnGroundEqual)
	{
		lastRequiredExplicitSeqNum_ = sendingSequenceNumber_;
		alwaysSendExplicitUpdates_ = true;
	}

	// immediately update our current onGround value ... except for the client
	// entity, we wait for confirmation from the server (by way of
	// setSpaceViewport / setVehicle prefixes) to change the space ID or
	// vehicle ID - and for the client we only change the vehicle ID here,
	// and wait to be given a new personal viewport to change the space ID
	if (!spaceAndVehicleAndOnGroundEqual)
	{
		if (id == id_)
		{
			if (vehicleID != 0)
				defaultViewportIDs_[ id ].vehicleID( vehicleID );
			else
				defaultViewportIDs_[ id ].svid( svid ); // keep old svid
		}
		else
		{
			// Currently even when we control an entity we keep getting updates
			// for it but just ignore them. This is so we can get the various
			// prefixes. We could set the vehicle info ourself but changing
			// the space ID is not so straightforward. However the main
			// advantage of this approach is not having to change the server to
			// know about the entities that we control. Unfortunately it is
			// quite inefficient - both for sending unnecessary explicit
			// updates (sends for about twice as long) and for getting tons
			// of unwanted position updates, mad worse by the high likelihood
			// of controlled entities being near to the client. Oh well,
			// it'll do for now.
		}

		// this map may carry more info in the future
		sentOnGround_[ id ] = onGround;
	}
}


/**
 * 	This method is called to start a new message to the proxy.
 * 	Note that proxy messages cannot be sent on a bundle after
 * 	entity messages.
 *
 * 	@param messageId	The message to send.
 *
 * 	@return	A stream to write the message on.
 */
BinaryOStream & ServerConnection::startProxyMessage( int messageId )
{
	if (this->offline())
	{
		CRITICAL_MSG( "ServerConnection::startProxyMessage: "
				"Called when not connected to server!\n" );
	}

	static Mercury::InterfaceElement anie = BaseAppExtInterface::entityMessage;
	// 0x80 to indicate it is an entity message, 0x40 to indicate that it is for
	// the base.
	anie.id( ((uchar)messageId) | 0xc0 );
	this->bundle().startMessage( anie, /*isReliable:*/true );

	return this->bundle();
}

/**
 * 	This message sends an entity message for the player's avatar.
 *
 * 	@param messageId	The message to send.
 *
 * 	@return A stream to write the message on.
 */
BinaryOStream & ServerConnection::startAvatarMessage( int messageId )
{
	return this->startEntityMessage( messageId, 0 );
}

/**
 * 	This message sends an entity message to a given entity.
 *
 * 	@param messageId	The message to send.
 * 	@param entityId		The id of the entity to receive the message.
 *
 * 	@return A stream to write the message on.
 */
BinaryOStream & ServerConnection::startEntityMessage( int messageId,
		ObjectID entityId )
{
	if (this->offline())
	{
		CRITICAL_MSG( "ServerConnection::startEntityMessage: "
				"Called when not connected to server!\n" );
	}

	static Mercury::InterfaceElement anie = BaseAppExtInterface::entityMessage;
	anie.id( ((uchar)messageId) | 0x80 );
	this->bundle().startMessage( anie, /*isReliable:*/true );
	this->bundle() << entityId;

	return this->bundle();
}


/**
 *	Ack the given viewport set message. Currently it is not sent reliably.
 */
void ServerConnection::sendSetViewportAck( ObjectID id, SpaceViewportID svid )
{
	// don't bother acking setSpaceViewport for the player
	if ((id_ == id) || this->offline())
	{
		return; // personalSpaceViewport_ set elsewhere
	}

	BaseAppExtInterface::setSpaceViewportAckArgs args;
	args.id = id;
	args.spaceViewportID = svid;
	this->bundle() << args;
}

/**
 *	Ack the given vehicle set message. Currently it is not sent reliably.
 */
void ServerConnection::sendSetVehicleAck( ObjectID id, ObjectID vehicleID )
{
	if (this->offline())
		return;

	BaseAppExtInterface::setVehicleAckArgs args;
	args.id = id;
	args.vehicleID = vehicleID;
	this->bundle() << args;
}

/// This method describes a request related to resource updates
struct ResUpdateRequest
{
	ResUpdateRequest() : pMessageData_( NULL ) { }
	~ResUpdateRequest() { delete pMessageData_; }

	uint16		rid_;
	uint16		minires_;
	enum { IDENTIFY, SUMMARISE, DOWNLOAD }	type_;
	MemoryOStream * pMessageData_;
	SimpleSemaphore ready_;
	uint32		progress_;	// 0 = nowhere, uint32(-1) = complete
	FILE *		pFile_;
};
typedef std::vector<ResUpdateRequest*> ResUpdateQueue;

static SimpleMutex s_requestQueueLock;
static ResUpdateQueue s_requestQueue;
static uint s_requestSentSize;

/// Helper method to queue a request
void ServerConnection::queueRURequest( ResUpdateRequest * rur )
{
	rur->progress_ = 0;

	s_requestQueueLock.grab();
	if (s_requestQueueEnabled)
	{
		rur->rid_ = s_requestQueue.size();
		s_requestQueue.push_back( rur );
	}
	else
	{
		rur->ready_.push();	// we cannot send requests
	}
	s_requestQueueLock.give();
}

/// Helper method to complete a request
void ServerConnection::completeRURequest( ResUpdateRequest * rur )
{
	// queue it
	this->queueRURequest( rur );

	if (!g_isMainThread)	// either wait for the main thread ...
		rur->ready_.pull();
	else					// ... or get up and process it ourselves
		this->sendRequestsAndBlock();
}

/// Helper method to find this request in the queue
ResUpdateRequest * ServerConnection::findRURequest( uint16 rid, bool erase )
{
	ResUpdateRequest * ret = NULL;

	s_requestQueueLock.grab();
	for (uint i = 0; i < s_requestSentSize; i++)
	{
		ResUpdateRequest * rur = s_requestQueue[i];
		if (rur->rid_ == rid)
		{
			ret = rur;
			if (erase)
			{
				s_requestQueue.erase( s_requestQueue.begin() + i );
				s_requestSentSize--;
			}
			break;
		}
		rur = NULL;
	}
	s_requestQueueLock.give();

	return ret;
}

/// Helper method to clear the queue when we are disconnected
void ServerConnection::cancelRURequests()
{
	s_requestQueueLock.grab();
	s_requestQueueEnabled = false;
	for (uint i = 0; i < s_requestQueue.size(); i++)
	{
		ResUpdateRequest * rur = s_requestQueue[i];
		rur->progress_ = 0x80000000;
		if (rur->type_ != ResUpdateRequest::DOWNLOAD ||
			rur->pFile_ != NULL || rur->progress_ == 0)
				rur->ready_.push();
	}
	s_requestQueue.clear();
	s_requestQueueLock.give();
}


/**
 *	This method sends any outstanding requests and blocks waiting for a reply.
 *	This method is incompatible with the use of multiple servconns.
 *
 *	Warning: Any messages not related to requests will be lost since there is
 *	no external message handler set for the duration of this call. This method
 *	should be called only in well controlled circumstances.
 */
void ServerConnection::sendRequestsAndBlock()
{
	TRACE_MSG( "ServerConnection::sendRequestsAndBlock: begin\n" );

	ServerMessageHandler * pSavedHandler = pHandler_;
	pHandler_ = NULL;

	// send once here (or until all requests sent)
	s_requestQueueLock.grab();
	uint32 rqSize = s_requestQueue.size();
	s_requestQueueLock.give();
	// request queue can only get bigger and s_requestSentSize cannot change
	// since we are the main thread and we are not processing input,
	// so it is safe to only get the size of the queue once above.
	while (s_requestSentSize < rqSize)
		this->send();

	uint64 sendEvery = stampsPerSecond()/uint(s_updateFrequency_);
	uint64 sendNext = timestamp() + sendEvery;

	// wait until we have received all responses
	while (s_requestSentSize > 0 && this->online())
	{
		// send again every so often
		if (timestamp() >= sendNext)
		{
			sendNext += sendEvery;
			if (requestBlockedCB_ != NULL) (*requestBlockedCB_)();
			this->send();
		}

		this->processInput();
	}

	pHandler_ = pSavedHandler;

	TRACE_MSG( "ServerConnection::sendRequestsAndBlock: done\n" );
};


/**
 *	This method adds any outstanding requests to the bundle
 */
void ServerConnection::addOutstandingRequests()
{
	if (this->offline())
		return;

	Mercury::Bundle & bundle = this->bundle();

	uint32 leftOvers = bundle.freeBytesInPacket();
	if (leftOvers < 8) return;
	leftOvers -= 8;	// leave a bit of headroom too

	// get from s_requestSentSize to newSentSize
	ResUpdateQueue toSend;

	s_requestQueueLock.grab();
	uint32 newSentSize = s_requestSentSize;
	while (newSentSize < s_requestQueue.size())
	{
		// only send as much as we can fit on this packet
		ResUpdateRequest & rur = *s_requestQueue[ newSentSize ];
		uint32 reqSize = 8 + rur.pMessageData_->size();	// header < ~8
		if (leftOvers < reqSize) break;
		leftOvers -= reqSize;
		newSentSize++;
	}
	toSend.assign( s_requestQueue.begin() + s_requestSentSize,
		s_requestQueue.begin() + newSentSize );
	s_requestSentSize = newSentSize;
	s_requestQueueLock.give();

	// send them
	for (uint i = 0; i < toSend.size(); i++)
	{
		ResUpdateRequest & rur = *toSend[i];

		switch (rur.type_)
		{
		case ResUpdateRequest::IDENTIFY:
			bundle.startMessage(
				BaseAppExtInterface::identifyVersionPoint, true );
			break;
		case ResUpdateRequest::SUMMARISE:
			bundle.startMessage(
				BaseAppExtInterface::summariseVersionPoint, true );
			break;
		case ResUpdateRequest::DOWNLOAD:
			bundle.startMessage(
				BaseAppExtInterface::commenceResourceDownload, true );
			break;
		}
		bundle << rur.rid_;
		bundle.transfer( *rur.pMessageData_, rur.pMessageData_->size() );
	}
}


/**
 *	This method identifies the version number of the given version point.
 *	It is expected to be called from a loading thread and not the main
 *	thread, and therefore will block. Calling it from the main thread
 *	when there is a ServerMessageHandler set will lose messages.
 *
 *	Returns true if a reply was received from the server, otherwise sets
 *	version to latest version and index to 0 and returns false.
 */
bool ServerConnection::identifyVersionPoint( const std::string & pt,
	const MD5::Digest & digest0, const MD5::Digest & digest1,
	uint32 & version, uint8 & index )
{
	ResUpdateRequest rur;
	rur.type_ = ResUpdateRequest::IDENTIFY;
	rur.pMessageData_ = new MemoryOStream( 128 );

	// build the request
	MemoryOStream & mos = *rur.pMessageData_;
	mos << pt << digest0 << digest1;

	// complete it
	this->completeRURequest( &rur );	// testing

	// now parse the result
	if (rur.progress_ == uint32(-1))
	{
		BinaryIStream & bis = *rur.pMessageData_;
		bis >> version >> index;
		return true;
	}
	else
	{
		version = latestVersion_;
		index = 0;
		return false;
	}
}


/**
 *	This method summarises the state of the given version point at
 *	the given version number. It blocks like @see identifyVersionPoint
 */
bool ServerConnection::summariseVersionPoint( uint32 version,
	const std::string & pt, MD5::Digest & changeMD5, MD5::Digest & censusMD5 )
{
	ResUpdateRequest rur;
	rur.type_ = ResUpdateRequest::SUMMARISE;
	rur.pMessageData_ = new MemoryOStream( 128 );

	// build the request
	MemoryOStream & mos = *rur.pMessageData_;
	mos << version << pt;

	// deal with it
	this->completeRURequest( &rur );

	// now parse the result
	if (rur.progress_ == uint32(-1))
	{
		BinaryIStream & bis = *rur.pMessageData_;
		bis >> changeMD5 >> censusMD5;
		return true;
	}
	else
	{
		changeMD5.clear();
		censusMD5.clear();
		return false;
	}
}


/**
 *	This method downloads a resource from the given version point at
 *	the given version number. It blocks like @see identifyVersionPoint.
 *	Pass uint32(-1) in version to request the whole file at the latest version.
 */
uint8 ServerConnection::downloadResourceVersion( uint32 version,
	const std::string & pt, const std::string & resource, FILE * pFile,
	float * progressVal )
{
	MF_ASSERT( pFile != NULL );

	ResUpdateRequest rur;
	rur.type_ = ResUpdateRequest::DOWNLOAD;
	rur.pMessageData_ = new MemoryOStream( 4096 );
	rur.pFile_ = g_isMainThread ? pFile : NULL;

	// build the request
	MemoryOStream & mos = *rur.pMessageData_;
	mos << version << pt << resource << uint32(0);

	// queue it
	this->queueRURequest( &rur );

	bool clientSideError = false;
	// if we're the main thread then the message handler can write the file
	if (rur.pFile_ != NULL)
	{
		this->sendRequestsAndBlock();
	}
	// otherwise it forwards us chunks of data and we write them out
	else
	{
		int lastProgress;
		char localBuffer[4096];
		do
		{
			char * tempBuffer = NULL;
			char * usedBuffer;

			rur.ready_.pull();
			BinaryIStream & bis = *rur.pMessageData_;
			int left = rur.pMessageData_->remainingLength();
			if (left <= int(sizeof(localBuffer)))
				usedBuffer = localBuffer;
			else
				usedBuffer = tempBuffer = new char[left];
			memcpy( usedBuffer, bis.retrieve( left ), left );
			rur.pMessageData_->reset();
			lastProgress = int(rur.progress_);
			rur.ready_.push();

			if (left == 0)
			{	// not exactly ideal synchronisation, but it'll do for now
#ifdef _WIN32
				Sleep( uint32(750/s_updateFrequency_) );
#else
				usleep( uint32(750000/s_updateFrequency_) );
#endif
				continue;
			}

			if (fwrite( usedBuffer, left, 1, pFile ) != 1)
				clientSideError = true;
			// we have to continue on error since server will keep sending

			if (progressVal != NULL) *progressVal += left;

			delete [] tempBuffer;
		} while (lastProgress >= 0);
	}

	// now parse the result
	uint8 method;
	if (!clientSideError && rur.progress_ == uint32(-1))
	{
		method = uint8(rur.minires_);
		//BinaryIStream & bis = *rur.pMessageData_;
		//bis >> method;
	}
	else
	{
		// (TODO: Make an enum for these download methods!)
		method = 0xFE;	// client side error
		if (clientSideError)
		{
			ERROR_MSG( "ServerConnection::downloadResourceVersion: "
				"Error writing data received from main thread to file\n" );
		}
		else
		{
			ERROR_MSG( "ServerConnection::downloadResourceVersion: "
				"Download stopped prematurely (progress %d)\n",
				int(rur.progress_) );
		}
	}
	return method;
}


/**
 *	This method notifies the server that the client has finished loading
 *	in the resources for the new version. Client should call this
 *	immediately upon completion of loading, so they don't hold everyone
 *	else up. If a client takes too long (as far as the server is concerned)
 *	then it will be summarily booted off.
 *	Resources need only be loaded to a point that the delay upon receiving
 *	the MIGRATE notification will not inconvenience the user ... ideally
 *	undetectable, but that is hard to achieve and resource updates are rare.
 */
void ServerConnection::resourceLoadinComplete( uint32 version )
{
	// make sure entities are enabled, or the server will be confused
	if (!entitiesEnabled_)
	{
		CRITICAL_MSG( "ServerConnection::resourceLoadinComplete: "
			"called with entities not enabled\n" );
		return;
	}

	// make sure the client just finished loading the right version!
	if (version != impendingVersion_ || version != imminentVersion_)
	{
		CRITICAL_MSG( "ServerConnection::resourceLoadinComplete: "
			"called with wrong version %d\n", version );
		return;
	}

	// and we are all ready for migration, Mr. server!
	DEBUG_MSG( "ServerConnection::resourceLoadingComplete: "
		"Client has completed loading of imminent version %d\n", version );

	BaseAppExtInterface::enableEntitiesArgs & args =
		BaseAppExtInterface::enableEntitiesArgs::start( this->bundle(), true );
	args.presentVersion = uint32(-1);
	args.futureVersion = imminentVersion_;
}


/**
 *	This method processes all pending network messages. They are passed to the
 *	input handler that was specified in logOnComplete.
 *
 *	@return	Returns true if any packets were processed.
 */
bool ServerConnection::processInput()
{
	// process any pending packets
	// (they may not be for us in a multi servconn environment)
	bool gotAnyPackets = false;
	try
	{
		bool gotAPacket = false;
		do
		{
			gotAPacket = nub_.processPendingEvents();

			gotAnyPackets |= gotAPacket;
			everReceivedPacket_ |= gotAPacket;
		}
		while (gotAPacket);
	}
	catch (Mercury::NubException & ne)
	{
		switch (ne.reason())
		{
			case Mercury::REASON_CORRUPTED_PACKET:
			{
				ERROR_MSG( "ServerConnection::processInput: "
					"Dropped corrupted incoming packet\n" );
				break;
			}

			// WINDOW_OVERFLOW is omitted here since we check for it in send()

			case Mercury::REASON_INACTIVITY:
			{
				if (this->online())
				{
					ERROR_MSG( "ServerConnection::processInput: "
						"Disconnecting due to nub exception (%s)\n",
						Mercury::reasonToString( ne.reason() ) );

					this->disconnect();
				}

				break;
			}

			default:

				WARNING_MSG( "ServerConnection::processInput: "
					"Got a nub exception (%s)\n",
					Mercury::reasonToString( ne.reason() ) );
		}
	}

	// Don't bother collecting statistics if we're not online.
	if (!this->online())
	{
		return gotAnyPackets;
	}

	// see how long that processing took
	if (gotAnyPackets)
	{
		static uint64 lastTimeStamp = timestamp();
		uint64 currTimeStamp = timestamp();
		uint64 delta = (currTimeStamp - lastTimeStamp)
						* uint64( 1000 ) / stampsPerSecond();
		int deltaInMS = int( delta );

		if (deltaInMS > 400)
		{
			WARNING_MSG( "ServerConnection::processInput: "
				"There were %d ms between packets\n", deltaInMS );
		}

		lastTimeStamp = currTimeStamp;
	}

	return gotAnyPackets;
}


/**
 *	This method handles an entity script message from the server.
 *
 *	@param messageID		Message Id.
 *	@param data		Stream containing message data.
 *	@param length	Number of bytes in the message.
 */
void ServerConnection::handleEntityMessage( int messageID, BinaryIStream & data,
	int length )
{
	// Get the entity id off the stream
	ObjectID objectID;
	data >> objectID;
	length -= sizeof( objectID );

//	DEBUG_MSG( "ServerConnection::handleMessage: %d\n", messageID );
	if (pHandler_)
	{
		const int PROPERTY_FLAG = 0x40;

		if (messageID & PROPERTY_FLAG)
		{
			pHandler_->onEntityProperty( objectID,
				messageID & ~PROPERTY_FLAG, data, versForCallIsOld_ );
		}
		else
		{
			pHandler_->onEntityMethod( objectID,
				messageID, data, versForCallIsOld_ );
		}
	}
}


// -----------------------------------------------------------------------------
// Section: avatarUpdate and related message handlers
// -----------------------------------------------------------------------------

/**
 *	This method handles the relativePositionReference message. It is used to
 *	indicate the position that should be used as the base for future relative
 *	positions.
 */
void ServerConnection::relativePositionReference(
	const ClientInterface::relativePositionReferenceArgs & args )
{
	receivingSequenceNumber_ = args.sequenceNumber;
	latencyTab_->append( this->latency() );

	// see if we can now stop sending full sized volatile updates
	if (alwaysSendExplicitUpdates_)
	{
		uint8 seqNumDiff =
			receivingSequenceNumber_ - lastRequiredExplicitSeqNum_;
		if (seqNumDiff < 85) alwaysSendExplicitUpdates_ = false;
		// if this seq num can be called 'after' the last time we changed
		// data that requires us to send explicit updates, then we can
		// stop sending explicit updates because the server has seen it
	}
}


/**
 *	This method indicates that the next entity position update is
 *	in the space viewport indicated. We should send an ack back to
 *	the server for it.
 */
void ServerConnection::setSpaceViewport(
	const ClientInterface::setSpaceViewportArgs & args )
{
	nextEntitysViewportID_ = args.spaceViewportID;
}


/**
 *	This method indicates that the next entity position update is
 *	in the space of the vehicle indicated. We should send an ack back to
 *	the server for it.
 */
void ServerConnection::setVehicle(
	const ClientInterface::setVehicleArgs & args )
{
	nextEntitysVehicleID_ = args.vehicleID;
}




#define AVATAR_UPDATE_GET_POS_ORIGIN										\
		const Vector3 originPos = vehicleID ? Vector3::zero() :				\
			svp.relativePosition( receivingSequenceNumber_ );				\

#define AVATAR_UPDATE_GET_POS_FullPos										\
		AVATAR_UPDATE_GET_POS_ORIGIN										\
		args.position.unpackXYZ( pos.x, pos.y, pos.z );						\
		pos += originPos;													\

#define AVATAR_UPDATE_GET_POS_OnChunk										\
		AVATAR_UPDATE_GET_POS_ORIGIN										\
		pos.y = -13000.f;													\
		args.position.unpackXZ( pos.x, pos.z );								\
		/* TODO: This is not correct. Need to implement this later. */		\
		pos.x += originPos.x;												\
		pos.z += originPos.z;												\

#define AVATAR_UPDATE_GET_POS_OnGround										\
		AVATAR_UPDATE_GET_POS_ORIGIN										\
		pos.y = -13000.f;													\
		args.position.unpackXZ( pos.x, pos.z );								\
		pos.x += originPos.x;												\
		pos.z += originPos.z;												\

#define AVATAR_UPDATE_GET_POS_NoPos											\
		pos.set( -13000.f, -13000.f, -13000.f );							\

#define AVATAR_UPDATE_GET_DIR_YawPitchRoll									\
		float yaw = 0.f, pitch = 0.f, roll = 0.f;							\
		args.dir.get( yaw, pitch, roll );									\

#define AVATAR_UPDATE_GET_DIR_YawPitch										\
		float yaw = 0.f, pitch = 0.f, roll = 0.f;							\
		args.dir.get( yaw, pitch );											\

#define AVATAR_UPDATE_GET_DIR_Yaw											\
		float yaw = int8ToAngle( args.dir );								\
		float pitch = 0.f;													\
		float roll = 0.f;													\

#define AVATAR_UPDATE_GET_DIR_NoDir											\
		float yaw = 0.f, pitch = 0.f, roll = 0.f;							\

#define AVATAR_UPDATE_GET_ID_NoAlias	args.id;
#define AVATAR_UPDATE_GET_ID_Alias		idAlias_[ args.idAlias ];

#define IMPLEMENT_AVUPMSG( ID, POS, DIR )									\
void ServerConnection::avatarUpdate##ID##POS##DIR(							\
		const ClientInterface::avatarUpdate##ID##POS##DIR##Args & args )	\
{																			\
	if (pHandler_ != NULL)													\
	{																		\
		Vector3 pos;														\
																			\
		ObjectID id = AVATAR_UPDATE_GET_ID_##ID								\
		ObjectID vehicleID;													\
		SpaceViewport & svp = this->getOrSetSpaceViewport( id, vehicleID );	\
																			\
		AVATAR_UPDATE_GET_POS_##POS											\
																			\
		AVATAR_UPDATE_GET_DIR_##DIR											\
																			\
		if (sentOnGround_.find( id ) != sentOnGround_.end()) return;		\
																			\
		pHandler_->onEntityMove( id, svp.spaceID_, vehicleID,				\
			pos, yaw, pitch, roll, true );									\
	}																		\
}


IMPLEMENT_AVUPMSG( NoAlias, FullPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( NoAlias, FullPos, YawPitch )
IMPLEMENT_AVUPMSG( NoAlias, FullPos, Yaw )
IMPLEMENT_AVUPMSG( NoAlias, FullPos, NoDir )
IMPLEMENT_AVUPMSG( NoAlias, OnChunk, YawPitchRoll )
IMPLEMENT_AVUPMSG( NoAlias, OnChunk, YawPitch )
IMPLEMENT_AVUPMSG( NoAlias, OnChunk, Yaw )
IMPLEMENT_AVUPMSG( NoAlias, OnChunk, NoDir )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, YawPitchRoll )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, YawPitch )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, Yaw )
IMPLEMENT_AVUPMSG( NoAlias, OnGround, NoDir )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, YawPitch )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, Yaw )
IMPLEMENT_AVUPMSG( NoAlias, NoPos, NoDir )
IMPLEMENT_AVUPMSG( Alias, FullPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( Alias, FullPos, YawPitch )
IMPLEMENT_AVUPMSG( Alias, FullPos, Yaw )
IMPLEMENT_AVUPMSG( Alias, FullPos, NoDir )
IMPLEMENT_AVUPMSG( Alias, OnChunk, YawPitchRoll )
IMPLEMENT_AVUPMSG( Alias, OnChunk, YawPitch )
IMPLEMENT_AVUPMSG( Alias, OnChunk, Yaw )
IMPLEMENT_AVUPMSG( Alias, OnChunk, NoDir )
IMPLEMENT_AVUPMSG( Alias, OnGround, YawPitchRoll )
IMPLEMENT_AVUPMSG( Alias, OnGround, YawPitch )
IMPLEMENT_AVUPMSG( Alias, OnGround, Yaw )
IMPLEMENT_AVUPMSG( Alias, OnGround, NoDir )
IMPLEMENT_AVUPMSG( Alias, NoPos, YawPitchRoll )
IMPLEMENT_AVUPMSG( Alias, NoPos, YawPitch )
IMPLEMENT_AVUPMSG( Alias, NoPos, Yaw )
IMPLEMENT_AVUPMSG( Alias, NoPos, NoDir )



/**
 *	Follow the chain of vehicles to find the svid for this entity
 */
SpaceViewportID ServerConnection::svidFollow(
	ObjectID id, ObjectID & vehicleID ) const
{
	vehicleID = 0;
	ObjectID topVID = 0;
	EntityViewports::const_iterator it = defaultViewportIDs_.find( id );
	while (it != defaultViewportIDs_.end())
	{
		if (!it->second.onVehicle())
			return it->second.svid_;

		topVID = it->second.vehicleID_;
		if (vehicleID == 0) vehicleID = topVID;
		it = defaultViewportIDs_.find( topVID );
	}
	ERROR_MSG( "ServerConnection::svidFollow: Viewport for entity %d "
		"(immed vehicle %d, top vehicle %d) is unknown\n",
		id, vehicleID, topVID );
	return 0;
}


/**
 *	Get the space viewport for the given entity id, which is receiving
 *	a position update. If a setSpaceViewport message was just received,
 *	then use that viewport instead and ack it.
 */
ServerConnection::SpaceViewport & ServerConnection::getOrSetSpaceViewport(
	ObjectID id, ObjectID & vehicleID )
{
	SpaceViewportID svid = 0;
	if (nextEntitysViewportID_ != SpaceViewportID(-1))
	{
		svid = nextEntitysViewportID_;
		vehicleID = 0;

		defaultViewportIDs_[ id ].svid( svid );
		this->sendSetViewportAck( id, svid );
		nextEntitysViewportID_ = SpaceViewportID(-1);
	}
	else if (nextEntitysVehicleID_ != ObjectID(-1))
	{
		defaultViewportIDs_[ id ].vehicleID( nextEntitysVehicleID_ );
		this->sendSetVehicleAck( id, nextEntitysVehicleID_ );
		nextEntitysVehicleID_ = ObjectID(-1);

		svid = this->svidFollow( id, vehicleID );
	}
	else
	{
		svid = this->svidFollow( id, vehicleID );
	}
	return spaceViewports_[ svid ];
}


/**
 *	This method handles a detailed position and direction update.
 */
void ServerConnection::detailedPosition(
	const ClientInterface::detailedPositionArgs & args )
{
	ObjectID vehicleID;
	SpaceViewport & svp = this->getOrSetSpaceViewport( args.id, vehicleID );

	ObjectID entityID = args.id;
	this->detailedPositionReceived(
		args.id, svp, vehicleID, args.position );

	if (pHandler_ != NULL &&
		(sentOnGround_.find( args.id ) == sentOnGround_.end()))
	{
		pHandler_->onEntityMove(
			entityID,
			svp.spaceID_,
			vehicleID,
			args.position,
			args.direction.yaw,
			args.direction.pitch,
			args.direction.roll,
			false );
	}
}

/**
 *	This method handles a forced position and direction update.
 *	This is when an update is being forced back for an (ordinarily)
 *	client controlled entity, including for the player. Usually this is
 *	due to a physics correction from the server, but it could be for any
 *	reason decided by the server (e.g. server-initiated teleport).
 */
void ServerConnection::forcedPosition(
	const ClientInterface::forcedPositionArgs & args )
{
	if (sentOnGround_.find( args.id ) == sentOnGround_.end())
	{
		// if we got one out of order then always ignore it - we would not
		// expect to get forcedPosition before handling controlEntity on
		WARNING_MSG( "ServerConnection::forcedPosition: "
			"Received forced position for entity %d that we do not control\n",
			args.id );
		return;
	}
	// ack this position correction
	// finally tell the handler about it
	sentOnGround_[ args.id ] = true;
	if (pHandler_ != NULL)
	{
		pHandler_->onEntityMove(
			args.id,
			args.spaceID,
			args.vehicleID,
			args.position,
			args.direction.yaw,
			args.direction.pitch,
			args.direction.roll,
			false );
	}
	else // keep earlier implementation( necessary?) acknowledge ourselves
	{
		this->addMove( args.id, args.spaceID, args.vehicleID, args.position,
			args.direction.yaw, args.direction.pitch, args.direction.roll,
			/*onGround:*/false, /*globalPosition:*/args.position );
	}

	if (sentOnGround_[ args.id ])
	{
		ERROR_MSG( "ServerConnection::forcedPosition: "
			"Handler did not do correct addMove call.\n" );
		this->addMove( args.id, args.spaceID, args.vehicleID, args.position,
			args.direction.yaw, args.direction.pitch, args.direction.roll,
				/*onGround:*/false, /*globalPosition:*/args.position );
	}
}


/**
 *	The server is telling us whether or not we are controlling this entity
 */
void ServerConnection::controlEntity(
	const ClientInterface::controlEntityArgs & args )
{
	// record it in the dual-purpose sentonGround map
	if (args.on)
	{
		sentOnGround_[ args.id ] = false;
	}
	else
	{
		sentOnGround_.erase( args.id );
	}

	// if this is a the dst gateway for a viewport then record the source
	// of reliative positions - us or the server
	ObjectID vehicleID;
	SpaceViewportID svid = this->svidFollow( args.id, vehicleID );
	SpaceViewport & svp = spaceViewports_[ svid ];
	if (svp.gatewayDstID_ == args.id && vehicleID == 0)
	{
		svp.selfControlled_ = args.on;
		if (args.on)
		{
			// initialise all the sent positions to the latest received position
			// ... might need to improve upon this in the future methinks
			for (uint i = 0; i < sizeof(svp.sentPositions_)/sizeof(Vector3);i++)
				svp.sentPositions_[i] = svp.rcvdPosition_;
		}
	}

	// tell the message handler about it
	if (pHandler_ != NULL)
	{
		pHandler_->onEntityControl( args.id, args.on );
	}
}


/**
 *	We have received a detailed position. It could be for an entity that forms
 *	a gateway dst for one of our space viewports. If so record that position
 *	since future position updates will be relative to it.
 */
void ServerConnection::detailedPositionReceived( ObjectID id,
	SpaceViewport & svp, ObjectID vehicleID, const Vector3 & position )
{
	if (svp.gatewayDstID_ == id && vehicleID == 0)
	{
		svp.rcvdPosition_ = position;
	}
	// TODO: And if vehicleID != 0? Maybe vehicles need to know if the
	// gateway dst is one of their passengers. Anyway when this is
	// fully implemented on the server I'm sure this will all be resolved.
}



// -----------------------------------------------------------------------------
// Section: Statistics methods
// -----------------------------------------------------------------------------

static void (*s_bandwidthFromServerMutator)( int bandwidth ) = NULL;
void setBandwidthFromServerMutator( void (*mutatorFn)( int bandwidth ) )
{
	s_bandwidthFromServerMutator = mutatorFn;
}

/**
 *	This method gets the bandwidth that this server connection should receive
 *	from the server.
 *
 *	@return		The current downstream bandwidth in bits per second.
 */
int ServerConnection::bandwidthFromServer() const
{
	return bandwidthFromServer_;
}


/**
 *	This method sets the bandwidth that this server connection should receive
 *	from the server.
 *
 *	@param bandwidth	The bandwidth in bits per second.
 */
void ServerConnection::bandwidthFromServer( int bandwidth )
{
	if (s_bandwidthFromServerMutator == NULL)
	{
		ERROR_MSG( "ServerConnection::bandwidthFromServer: Cannot comply "
			"since no mutator set with 'setBandwidthFromServerMutator'\n" );
		return;
	}

	const int MIN_BANDWIDTH = 0;
	const int MAX_BANDWIDTH = PACKET_MAX_SIZE * NETWORK_BITS_PER_BYTE * 10 / 2;

	bandwidth = Math::clamp( MIN_BANDWIDTH, bandwidth, MAX_BANDWIDTH );

	(*s_bandwidthFromServerMutator)( bandwidth );

	// don't set it now - wait to hear back from the server
	//bandwidthFromServer_ = bandwidth;
}



/**
 *	This method returns the number of bits received per second.
 */
double ServerConnection::bpsIn() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return bitsIn_.changePerSecond();
}


/**
 *	This method returns the number of bits sent per second.
 */
double ServerConnection::bpsOut() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return bitsOut_.changePerSecond();
}


/**
 *	This method returns the number of packets received per second.
 */
double ServerConnection::packetsPerSecondIn() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return packetsIn_.changePerSecond();
}


/**
 *	This method returns the number of packets sent per second.
 */
double ServerConnection::packetsPerSecondOut() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return packetsOut_.changePerSecond();
}


/**
 *	This method returns the number of messages sent per second.
 */
double ServerConnection::messagesPerSecondIn() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return messagesIn_.changePerSecond();
}


/**
 *	This method returns the number of messages received per second.
 */
double ServerConnection::messagesPerSecondOut() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return messagesOut_.changePerSecond();
}


/**
 *	This method returns the percentage of movement bytes received.
 */
double ServerConnection::movementBytesPercent() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return movementBytes_.changePerSecond() /
		totalBytes_.changePerSecond() * 100.0;
}

/**
 *	This method returns the percentage of non-movement bytes received
 */
double ServerConnection::nonMovementBytesPercent() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return nonMovementBytes_.changePerSecond() /
		totalBytes_.changePerSecond() * 100.0;
}


/**
 *	This method returns the percentage of overhead bytes received
 */
double ServerConnection::overheadBytesPercent() const
{
	const_cast<ServerConnection *>(this)->updateStats();

	return overheadBytes_.changePerSecond() /
		totalBytes_.changePerSecond() * 100.0;
}


/**
 *	This method returns the total number of bytes received that are associated
 *	with movement messages.
 */
int ServerConnection::movementBytesTotal() const
{
	return movementBytes_;
}


/**
 *	This method returns the total number of bytes received that are associated
 *	with non-movement messages.
 */
int ServerConnection::nonMovementBytesTotal() const
{
	return nonMovementBytes_;
}


/**
 *	This method returns the total number of bytes received that are associated
 *	with packet overhead.
 */
int ServerConnection::overheadBytesTotal() const
{
	return overheadBytes_;
}


/**
 *	This method returns the total number of messages received that are associated
 *	with associated with movement.
 */
int ServerConnection::movementMessageCount() const
{
	int count = 0;
	for (int i = FIRST_AVATAR_UPDATE_MESSAGE;
				i <= LAST_AVATAR_UPDATE_MESSAGE; i++)
	{
		nub_.numMessagesReceivedForMessage( i );
	}

	return count;
}



/**
 *	This method updates the timing statistics of the server connection.
 */
void ServerConnection::updateStats()
{
	const double UPDATE_PERIOD = 2.0;

	double timeDelta = this->appTime() - lastTime_;

	if ( timeDelta > UPDATE_PERIOD )
	{
		lastTime_ = this->appTime();

		packetsIn_ = nub_.numPacketsReceived();
		messagesIn_ = nub_.numMessagesReceived();
		bitsIn_ = nub_.numBytesReceived() * NETWORK_BITS_PER_BYTE;

		movementBytes_ = nub_.numBytesReceivedForMessage(
						ClientInterface::relativePositionReference.id() );

		for (int i = FIRST_AVATAR_UPDATE_MESSAGE;
				i <= LAST_AVATAR_UPDATE_MESSAGE; i++)
		{
			movementBytes_ += nub_.numBytesReceivedForMessage( i );
		}

		totalBytes_ = nub_.numBytesReceived();
		overheadBytes_ = nub_.numOverheadBytesReceived();
		nonMovementBytes_ = totalBytes_ - movementBytes_ - overheadBytes_;

		packetsIn_.update( timeDelta );
		packetsOut_.update( timeDelta );

		bitsIn_.update( timeDelta );
		bitsOut_.update( timeDelta );

		messagesIn_.update( timeDelta );
		messagesOut_.update( timeDelta );

		totalBytes_.update( timeDelta );
		movementBytes_.update( timeDelta );
		nonMovementBytes_.update( timeDelta );
		overheadBytes_.update( timeDelta );
	}
}


/**
 *	This method sends the current bundle to the server.
 */
void ServerConnection::send()
{
	// get out now if we are not connected
	if (this->offline())
		return;

	// if we want to try to fool a firewall by reconfiguring ports,
	//  this is a good time to do so!
	if (tryToReconfigurePorts_ && !everReceivedPacket_)
	{
		Mercury::Bundle bundle;
		bundle.startMessage( BaseAppExtInterface::authenticate, false );
		bundle << sessionKey_;
		this->nub().send( this->addr(), bundle );
	}

	// add any outstanding queued requests
	this->addOutstandingRequests();

	// increment the sending sequence number if we have cause
	if (sendingSequenceIncrement_)
	{
		sendingSequenceIncrement_ = false;
		sendingSequenceNumber_++;
	}

	// record the time we last did a send
	if (pTime_)
		lastSendTime_ = *pTime_;

	// update statistics
	packetsOut_ += 1;	// Assumes one packet per bundle.
	messagesOut_ += 1;	// TODO:PM How do we calculate this?
	bitsOut_ += (this->bundle().size() + UDP_OVERHEAD) * NETWORK_BITS_PER_BYTE;

	// get the channel to send the bundle
	this->channel().send();

	const int OVERFLOW_LIMIT = 1024;

	// TODO: #### Make a better check that is dependent on both the window
	// size and time since last heard from the server.
	if (this->channel().sendWindowUsage() > OVERFLOW_LIMIT)
	{
		WARNING_MSG( "ServerConnection::send: "
			"Disconnecting since channel has overflowed.\n" );

		this->disconnect();
	}
}



/**
 * 	This method primes outgoing bundles with the authenticate message once it
 * 	has been received.
 */
void ServerConnection::primeBundle( Mercury::Bundle & bundle )
{
	if (sessionKey_)
	{
		BaseAppExtInterface::authenticateArgs::start( bundle ).key =
			sessionKey_;
	}
}


/**
 * 	This method returns the number of unreliable messages that are streamed on
 *  by primeBundle().
 */
int ServerConnection::numUnreliableMessages() const
{
	return sessionKey_ ? 1 : 0;
}


/**
 *	This method requests the server to send update information for the entity
 *	with the input id. This must be called after receiving an onEntityEnter
 *	message to allow message and incremental property updates to flow.
 *
 *  @param id		ID of the entity whose update is requested.
 *	@param stamps	A vector containing the known cache event stamps. If none
 *					are known, stamps is empty.
 */
void ServerConnection::requestEntityUpdate( ObjectID id,
	const CacheStamps & stamps )
{
	if (this->offline())
		return;

	this->bundle().startMessage( BaseAppExtInterface::requestEntityUpdate,
		  /*isReliable:*/true );
	this->bundle() << id;

	CacheStamps::const_iterator iter = stamps.begin();

	while (iter != stamps.end())
	{
		this->bundle() << (*iter);

		iter++;
	}
}


// -----------------------------------------------------------------------------
// Section: Various Mercury InputMessageHandler implementations
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EntityMessageHandler::EntityMessageHandler()
{
}


/**
 * 	This method handles entity messages from the server and sends them to the
 *	associated ServerConnection object.
 */
void EntityMessageHandler::handleMessage(
	const Mercury::Address & /* srcAddr */,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	ServerConnection * pServConn =
		(ServerConnection *)header.pNub->pExtensionData();
	pServConn->handleEntityMessage( header.identifier & 0x7F, data,
		header.length );
}


/**
 * 	@internal
 *	Objects of this type are used to handle messages destined for the client
 *	application.
 */
template <class ARGS>
class ClientMessageHandler : public Mercury::InputMessageHandler
{
public:
	/// This typedef declares a member function on the ServerConnection
	/// class that handles a single message.
	typedef void (ServerConnection::*Handler)( const ARGS & args );

	/// This is the constructor
	ClientMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	/**
	 * 	This method handles a Mercury message, and dispatches it to
	 * 	the correct member function of the ServerConnection object.
	 */
	virtual void handleMessage( const Mercury::Address & /*srcAddr*/,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ARGS & args = *(ARGS*)data.retrieve( sizeof(ARGS) );
		ServerConnection * pServConn =
			(ServerConnection *)header.pNub->pExtensionData();
		(pServConn->*handler_)( args );
	}

	Handler handler_;
};


/**
 * 	@internal
 *	Objects of this type are used to handle variable length messages destined
 *	for the client application.
 */
class ClientVarLenMessageHandler : public Mercury::InputMessageHandler
{
public:
	/// This typedef declares a member function on the ServerConnection
	/// class that handles a single variable lengthmessage.
	typedef void (ServerConnection::*Handler)( BinaryIStream & stream,
			int length );

	/// This is the constructor.
	ClientVarLenMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	/**
	 * 	This method handles a Mercury message, and dispatches it to
	 * 	the correct member function of the ServerConnection object.
	 */
	virtual void handleMessage( const Mercury::Address & /*srcAddr*/,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ServerConnection * pServConn =
			(ServerConnection *)header.pNub->pExtensionData();
		(pServConn->*handler_)( data, header.length );
	}

	Handler handler_;
};


/**
 * 	@internal
 *	Objects of this type are used to handle variable length messages destined
 *	for the client application. The handler is also passed the source address.
 */
class ClientVarLenWithAddrMessageHandler : public Mercury::InputMessageHandler
{
	public:
		/// This typedef declares a member function on the ServerConnection
		/// class that handles a single variable lengthmessage.
		typedef void (ServerConnection::*Handler)(
				const Mercury::Address & srcAddr,
				BinaryIStream & stream,
				int length );

		/// This is the constructor.
		ClientVarLenWithAddrMessageHandler( Handler handler ) :
			handler_( handler ) {}

	private:
		/**
		 * 	This method handles a Mercury message, and dispatches it to
		 * 	the correct member function of the ServerConnection object.
		 */
		virtual void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data )
		{
			ServerConnection * pServConn =
				(ServerConnection *)header.pNub->pExtensionData();
			(pServConn->*handler_)( srcAddr, data, header.length );
		}

		Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: Server Timing code
// -----------------------------------------------------------------------------
// TODO:PM This section is really just here to give a better time to the
// filtering. This should be reviewed.

/**
 *	This method returns the server time estimate based on the input client time.
 */
double ServerConnection::serverTime( double clientTime ) const
{
	return serverTimeHandler_.serverTime( clientTime );
}


/**
 *	This method returns the server time associated with the last packet that was
 *	received from the server.
 */
double ServerConnection::lastMessageTime() const
{
	return serverTimeHandler_.lastMessageTime();
}


/**
 * 	This method returns the game time associated with the last packet that was
 * 	received from the server.
 */
TimeStamp ServerConnection::lastGameTime() const
{
	return serverTimeHandler_.lastGameTime();
}


const double ServerConnection::ServerTimeHandler::UNINITIALISED_TIME = -1000.0;

/**
 *	The constructor for ServerTimeHandler.
 */
ServerConnection::ServerTimeHandler::ServerTimeHandler() :
	tickByte_( 0 ),
	timeAtSequenceStart_( UNINITIALISED_TIME ),
	gameTimeAtSequenceStart_( 0 )
{
}


/**
 * 	This method is called when the server sends a new gametime.
 * 	This should be after the sequence number for the current packet
 * 	has been set.
 *
 *	@param newGameTime	The current game time in ticks.
 */
void ServerConnection::ServerTimeHandler::gameTime( TimeStamp newGameTime,
		double currentTime )
{
	tickByte_ = uint8( newGameTime );
	gameTimeAtSequenceStart_ = newGameTime - tickByte_;
	timeAtSequenceStart_ = currentTime -
		double(tickByte_) / ServerConnection::updateFrequency();
}


/**
 *	This method is called when a new tick sync message is received from the
 *	server. It is used to synchronise between client and server time.
 *
 *	@param newSeqNum	The sequence number just received. This increases by one
 *						for each packets and packets should be received at 10Hz.
 *
 *	@param currentTime	This is the time that the client currently thinks it is.
 *						We need to pass it in since this file does not have
 *						access to the app file.
 */
void ServerConnection::ServerTimeHandler::tickSync( uint8 newSeqNum,
		double currentTime )
{
	const float updateFrequency = ServerConnection::updateFrequency();
	const double SEQUENCE_PERIOD = 256.0/updateFrequency;
	const int SEQUENCE_PERIOD_INT = 256;

	// Have we started yet?

	if (timeAtSequenceStart_ == UNINITIALISED_TIME)
	{
		// The first one is always like this.
		// INFO_MSG( "ServerTimeHandler::sequenceNumber: "
		//	"Have not received gameTime message yet.\n" );
		return;
	}

	tickByte_ = newSeqNum;

	// Want to adjust the time so that the client does not get too out of sync.
	double timeError = currentTime - this->lastMessageTime();
	int numSeqsOut = int( floor( timeError/SEQUENCE_PERIOD + 0.5f ) );

	if (numSeqsOut != 0)
	{
		timeAtSequenceStart_ += numSeqsOut * SEQUENCE_PERIOD;
		gameTimeAtSequenceStart_ += numSeqsOut * SEQUENCE_PERIOD_INT;
		timeError -= numSeqsOut * SEQUENCE_PERIOD;
	}

	const double MAX_TIME_ERROR = 0.05;
	const double MAX_TIME_ADJUST = 0.005;

	if (timeError > MAX_TIME_ERROR)
	{
		timeAtSequenceStart_ += MF_MIN( timeError, MAX_TIME_ADJUST );
	}
	else if (-timeError > MAX_TIME_ERROR)
	{
		timeAtSequenceStart_ += MF_MAX( timeError, -MAX_TIME_ADJUST );
	}
}


/**
 *	This method returns the time that this client thinks the server is at.
 */
double
ServerConnection::ServerTimeHandler::serverTime( double clientTime ) const
{
	return (gameTimeAtSequenceStart_ / ServerConnection::updateFrequency()) +
		(clientTime - timeAtSequenceStart_);
}


/**
 *	This method returns the server time associated with the last packet that was
 *	received from the server.
 */
double ServerConnection::ServerTimeHandler::lastMessageTime() const
{
	return timeAtSequenceStart_ +
		double(tickByte_) / ServerConnection::updateFrequency();
}


/**
 * 	This method returns the game time of the current message.
 */
TimeStamp ServerConnection::ServerTimeHandler::lastGameTime() const
{
	return gameTimeAtSequenceStart_ + tickByte_;
}


/**
 *	This method initialises the watcher information for this object.
 */
void ServerConnection::initDebugInfo()
{
	MF_WATCH(  "Comms/Desired bps in",
		*this,
		MF_ACCESSORS( int, ServerConnection, bandwidthFromServer ) );

	MF_WATCH( "Comms/bps in",	*this, &ServerConnection::bpsIn );
	MF_WATCH( "Comms/bps out",	*this, &ServerConnection::bpsOut );

	MF_WATCH( "Comms/PacketsSec in ", *this,
		&ServerConnection::packetsPerSecondIn );
	MF_WATCH( "Comms/PacketsSec out", *this,
		&ServerConnection::packetsPerSecondOut );

	MF_WATCH( "Comms/Messages in",	*this,
		&ServerConnection::messagesPerSecondIn );
	MF_WATCH( "Comms/Messages out",	*this,
		&ServerConnection::messagesPerSecondOut );

	MF_WATCH( "Comms/Expected Freq", ServerConnection::s_updateFrequency_,
		Watcher::WT_READ_ONLY );

	MF_WATCH( "Comms/Game Time", *this, &ServerConnection::lastGameTime );

	MF_WATCH( "Comms/Movement pct", *this, &ServerConnection::movementBytesPercent);
	MF_WATCH( "Comms/Non-move pct", *this, &ServerConnection::nonMovementBytesPercent);
	MF_WATCH( "Comms/Overhead pct", *this, &ServerConnection::overheadBytesPercent);

	MF_WATCH( "Comms/Movement total", *this, &ServerConnection::movementBytesTotal);
	MF_WATCH( "Comms/Non-move total", *this, &ServerConnection::nonMovementBytesTotal);
	MF_WATCH( "Comms/Overhead total", *this, &ServerConnection::overheadBytesTotal);

	MF_WATCH( "Comms/Movement count", *this, &ServerConnection::movementMessageCount);
	MF_WATCH( "Comms/Packet count", nub_, &Mercury::Nub::numPacketsReceived );

	MF_WATCH( "Comms/Latency", *this, &ServerConnection::latency );
}


// -----------------------------------------------------------------------------
// Section: Mercury message handlers
// -----------------------------------------------------------------------------

/**
 *	This method authenticates the server to the client. Its use is optional,
 *	and determined by the server that we are connected to upon login.
 */
void ServerConnection::authenticate(
	const ClientInterface::authenticateArgs & args )
{
	if (args.key != sessionKey_)
	{
		ERROR_MSG( "ServerConnection::authenticate: "
				   "Unexpected key! (%x, wanted %x)\n",
				   args.key, sessionKey_ );
		return;
	}
}



/**
 * 	This message handles a bandwidthNotification message from the server.
 */
void ServerConnection::bandwidthNotification(
	const ClientInterface::bandwidthNotificationArgs & args )
{
	// TRACE_MSG( "ServerConnection::bandwidthNotification: %d\n", args.bps);
	bandwidthFromServer_ = args.bps;
}


/**
 *	This method handles the message from the server that informs us how
 *	frequently it is going to send to us.
 */
void ServerConnection::updateFrequencyNotification(
		const ClientInterface::updateFrequencyNotificationArgs & args )
{
	s_updateFrequency_ = (float)args.hertz;
}



/**
 *	This method handles a tick sync message from the server. It is used as
 *	a timestamp for the messages in the packet.
 */
void ServerConnection::tickSync(
	const ClientInterface::tickSyncArgs & args )
{
	serverTimeHandler_.tickSync( args.tickByte, this->appTime() );
}


/**
 *	This method handles a setGameTime message from the server.
 *	It is used to adjust the current (server) game time.
 */
void ServerConnection::setGameTime(
	const ClientInterface::setGameTimeArgs & args )
{
	serverTimeHandler_.gameTime( args.gameTime, this->appTime() );
}


/**
 *	This method handles a resetEntities call from the server.
 */
void ServerConnection::resetEntities(
	const ClientInterface::resetEntitiesArgs & args )
{
	// proxy must have received our enableEntities if it is telling
	// us to reset entities (even if we haven't yet received any player
	// creation msgs due to reordering)
	MF_ASSERT( entitiesEnabled_ );

	// clear existing stale packet
	this->send();

	// clear any viewport/vehicle state ready for createCellPlayer
	spaceViewports_.clear();
	defaultViewportIDs_.clear();
	spaceSeenCount_.clear();
	personalSpaceViewport_ = 0;
	sentOnGround_.clear();

	createCellPlayerMsg_.reset();

	// forget about the base player entity too if so indicated
	if (!args.keepPlayerOnBase)
	{
		id_ = 0;

		// delete proxy data downloads in progress
		for (uint i = 0; i < proxyData_.size(); ++i)
			delete proxyData_[i];
		proxyData_.clear();
		// but not resource downloads as they're non-entity and should continue
	}

	// refresh bundle prefix
	this->send();

	// re-enable entities, which serves to ack the resetEntities
	// and flush/sync the incoming channel
	entitiesEnabled_ = false;
	this->enableEntities( /*impendingVersionDownloaded:*/false );

	// and finally tell the client about it so it can clear out
	// all (or nigh all) its entities
	if (pHandler_)
	{
		pHandler_->onEntitiesReset( args.keepPlayerOnBase );
	}
}


/**
 *	This method handles a createPlayer call from the base.
 */
void ServerConnection::createBasePlayer( BinaryIStream & stream,
										int /*length*/ )
{
	// we have new player id
	ObjectID playerID = 0;
	stream >> playerID;

	INFO_MSG( "ServerConnection::createBasePlayer: id %d\n", playerID );

	// this is now our player id
	id_ = playerID;

	EntityTypeID playerType = EntityTypeID(-1);
	stream >> playerType;

	if (pHandler_)
	{	// just get base data here
		pHandler_->onBasePlayerCreate( id_, playerType,
			stream, versForCallIsOld_ );
	}

	if (createCellPlayerMsg_.remainingLength() > 0)
	{
		INFO_MSG( "ServerConnection::createBasePlayer: "
			"Playing buffered createCellPlayer message\n" );
		this->createCellPlayer( createCellPlayerMsg_,
			createCellPlayerMsg_.remainingLength() );
		createCellPlayerMsg_.reset();
	}
}


/**
 *	This method handles a createCellPlayer call from the cell.
 */
void ServerConnection::createCellPlayer( BinaryIStream & stream,
										int /*length*/ )
{
	if (id_ == 0)
	{
		WARNING_MSG( "ServerConnection::createCellPlayer: Got createCellPlayer"
			"before createBasePlayer. Buffering message\n" );

		MF_ASSERT( createCellPlayerMsg_.remainingLength() == 0 );

		createCellPlayerMsg_.transfer( stream, stream.remainingLength() );

		return;
	}
	else
	{
		INFO_MSG( "ServerConnection::createCellPlayer: id %d\n", id_ );
	}

	SpaceID spaceID;
	ObjectID vehicleID;
	Position3D pos;
	Direction3D	dir;
	stream >> spaceID >> vehicleID >> pos >> dir;

	// start in space viewport zero
	if (vehicleID == 0)
	{
		defaultViewportIDs_[ id_ ].svid( 0 );
	}
	else
	{
		defaultViewportIDs_[ id_ ].vehicleID( vehicleID );
		defaultViewportIDs_[ vehicleID ].svid( 0 );
	}

	// set it up with default settings
	SpaceViewport & svpZero = spaceViewports_[ 0 ];
	svpZero.gatewaySrcID_ = 0;
	svpZero.gatewayDstID_ = id_;
	svpZero.spaceID_ = spaceID;
	svpZero.selfControlled_ = true;
	svpZero.sentPositions_[ 0 ] = pos;
	spaceSeenCount_[ spaceID ] = 1;

	// assume that we control this entity too
	sentOnGround_[ id_ ] = false;

	if (pHandler_)
	{	// just get cell data here
		pHandler_->onCellPlayerCreate( id_,
			spaceID, vehicleID, pos, dir.yaw, dir.pitch, dir.roll,
			stream, versForCallIsOld_ );
		// pHandler_->onEntityEnter( id_, spaceID, vehicleID );
	}

	this->detailedPositionReceived( id_, svpZero, vehicleID, pos );

	// The channel to the server is now regular
	this->channel().isIrregular( false );
}



/**
 *	This method handles keyed data about a particular space from the server.
 */
void ServerConnection::spaceData( BinaryIStream & stream, int length )
{
	SpaceID spaceID;
	SpaceEntryID spaceEntryID;
	uint16 key;
	std::string data;

	stream >> spaceID >> spaceEntryID >> key;
	length -= sizeof(spaceID) + sizeof(spaceEntryID) + sizeof(key);
	data.assign( (char*)stream.retrieve( length ), length );

	TRACE_MSG( "ServerConnection::spaceData: space %d key %d\n",
		spaceID, uint32(key) );

	//MF_ASSERT( spaceSeenCount_[spaceID] > 0 );
	if (spaceSeenCount_[spaceID] <= 0)
	{
		WARNING_MSG( "ServerConnection::spaceData: "
			"Received space data before any space viewport info\n" );
	}
	// actually this assert doesn't matter as we do not tell the handler
	// about space appearance, only disappearance so it does not matter
	// if the message is received early (and is unlikely to be received
	// late - and even if so then it is just a bit of data hanging around)
	// might be nice to check the seq of this message relative to the
	// one in which we learnt the space was gone however... not dire 'tho.

	if (pHandler_)
	{
		pHandler_->spaceData( spaceID, spaceEntryID, key, data );
	}
}


/**
 *	This method handles information about a viewport into a space
 *	from the server.
 */
void ServerConnection::spaceViewportInfo(
	const ClientInterface::spaceViewportInfoArgs & args )
{
	TRACE_MSG( "ServerConnection::spaceViewportInfo: space %d svid %d\n",
		args.spaceID, args.spaceViewportID );

	SpaceViewport & svp = spaceViewports_[ args.spaceViewportID ];
	// see if this viewport is already valid
	if (svp.spaceID_ != 0)
	{
		// see if we have been unsubscribed from this viewport
		if (args.gatewayDstID == 0)
		{
			if (--spaceSeenCount_[ svp.spaceID_ ] == 0 && pHandler_)
			{
				pHandler_->spaceGone( svp.spaceID_ );
			}
			svp.gatewaySrcID_ = 0;
			svp.gatewayDstID_ = 0;
			svp.spaceID_ = 0;
			svp.selfControlled_ = false;
		}
		// ok, some details are changing then
		else
		{
			svp.gatewaySrcID_ = args.gatewaySrcID;
			svp.gatewayDstID_ = args.gatewayDstID;
			if (svp.spaceID_ != args.spaceID)
			{
				ERROR_MSG( "ServerConnection::spaceViewportInfo: "
					"Server wants us to re-use space viewport %d changing "
					"space from %d to %d!\n", args.spaceViewportID,
					svp.spaceID_, args.spaceID );
			}
			svp.spaceID_ = args.spaceID;
			if (svp.gatewayDstID_ == id_)
			{
				ObjectID topVID;
				for (topVID = id_;
					defaultViewportIDs_[ topVID ].onVehicle();
					topVID = defaultViewportIDs_[ topVID ].vehicleID_) ; // scan
				defaultViewportIDs_[ topVID ].svid( args.spaceViewportID );
				personalSpaceViewport_ = args.spaceViewportID;
			}
			svp.selfControlled_ =
				sentOnGround_.find( svp.gatewayDstID_ ) != sentOnGround_.end();
		}
	}
	// otherwise we're making a new one
	else
	{
		// make sure details are being filled in
		if (args.gatewayDstID != 0)
		{
			svp.gatewaySrcID_ = args.gatewaySrcID;
			svp.gatewayDstID_ = args.gatewayDstID;
			svp.spaceID_ = args.spaceID;
			if (spaceSeenCount_[ svp.spaceID_ ]++ == 0 && pHandler_)
			{
				// could send a message to the handler here
				// currently however we will do nothing, as it does not (yet)
				// need to know details of viewports, and can create its own
				// spaces automatically when it gets data/entities for them
			}
			if (svp.gatewayDstID_ == id_)
			{
				ObjectID topVID;
				for (topVID = id_;
					defaultViewportIDs_[ topVID ].onVehicle();
					topVID = defaultViewportIDs_[ topVID ].vehicleID_) ; // scan
				defaultViewportIDs_[ topVID ].svid( args.spaceViewportID );
				personalSpaceViewport_ = args.spaceViewportID;
			}
			svp.selfControlled_ =
				sentOnGround_.find( svp.gatewayDstID_ ) != sentOnGround_.end();
		}
		// ok, we're losing a viewport we never had ... hmmm
		else
		{
			ERROR_MSG( "ServerConnnection::spaceViewportInfo: "
				"Server wants us to close nonexistent viewport %d\n",
				args.spaceViewportID );
		}
	}
}


/**
 *	This method handles the message from the server that an entity has entered
 *	our Area of Interest (AoI).
 */
void ServerConnection::enterAoI( const ClientInterface::enterAoIArgs & args )
{
	// Set this even if args.idAlias is NO_ID_ALIAS.
	idAlias_[ args.idAlias ] = args.id;

	// TODO: Handle the case where an entity does this: enter1-leave-enter2,
	// but we see this: enter2-leave-enter1. We can detect it from the
	// sequence numbers, but we really should handle it.
	defaultViewportIDs_[ args.id ].svid( personalSpaceViewport_ );

	if (pHandler_)
	{
		//TRACE_MSG( "ServerConnection::enterAoI: Entity = %d\n", args.id );
		pHandler_->onEntityEnter( args.id,
			spaceViewports_[ personalSpaceViewport_ ].spaceID_, 0 );
	}
}

/**
 *	This method handles the message from the server that an entity has entered
 *	our Area of Interest (AoI).
 */
void ServerConnection::enterAoIThruViewport(
	const ClientInterface::enterAoIThruViewportArgs & args )
{
	// Set this even if args.idAlias is NO_ID_ALIAS.
	idAlias_[ args.idAlias ] = args.id;

	// TODO: Handle the case where an entity does this: enter1-leave-enter2,
	// but we see this: enter2-leave-enter1. We can detect it from the
	// sequence numbers, but we really should handle it.
	defaultViewportIDs_[ args.id ].svid( args.spaceViewportID );

	if (pHandler_)
	{
		//TRACE_MSG( "ServerConnection::enterAoI: Entity = %d\n", args.id );
		pHandler_->onEntityEnter( args.id,
			spaceViewports_[ args.spaceViewportID ].spaceID_, 0 );
	}
}

/**
 *	This method handles the message from the server that an entity has entered
 *	our Area of Interest (AoI).
 */
void ServerConnection::enterAoIOnVehicle(
	const ClientInterface::enterAoIOnVehicleArgs & args )
{
	// Set this even if args.idAlias is NO_ID_ALIAS.
	idAlias_[ args.idAlias ] = args.id;

	// TODO: Handle the case where an entity does this: enter1-leave-enter2,
	// but we see this: enter2-leave-enter1. We can detect it from the
	// sequence numbers, but we really should handle it.
	defaultViewportIDs_[ args.id ].vehicleID( args.vehicleID );

	if (pHandler_)
	{
		//TRACE_MSG( "ServerConnection::enterAoI: Entity = %d\n", args.id );
		ObjectID vehicleID;
		SpaceViewportID svid = this->svidFollow( args.id, vehicleID );
		pHandler_->onEntityEnter( args.id,
			spaceViewports_[ svid ].spaceID_, args.vehicleID );
	}
}



/**
 *	This method handles the message from the server that an entity has left our
 *	Area of Interest (AoI).
 */
void ServerConnection::leaveAoI( BinaryIStream & stream, int /*length*/ )
{
	ObjectID id;
	stream >> id;

	if(pHandler_)
	{
		CacheStamps stamps( stream.remainingLength() / sizeof(EventNumber) );

		CacheStamps::iterator iter = stamps.begin();

		while (iter != stamps.end())
		{
			stream >> (*iter);

			iter++;
		}

		pHandler_->onEntityLeave( id, stamps );
	}

	defaultViewportIDs_.erase( id );
	sentOnGround_.erase( id );
}


/**
 *	This method handles a createEntity call from the server.
 */
void ServerConnection::createEntity( BinaryIStream & stream, int /*length*/ )
{
	ObjectID id;
	stream >> id;

	MF_ASSERT( id != ObjectID( -1 ) )	// old-style deprecated hack

	EntityTypeID type;
	stream >> type;

	Vector3 pos( 0.f, 0.f, 0.f );
	int8 compressedYaw = 0;
	int8 compressedPitch = 0;
	int8 compressedRoll = 0;

	stream >> pos >> compressedYaw >> compressedPitch >> compressedRoll;

	float yaw = int8ToAngle( compressedYaw );
	float pitch = int8ToAngle( compressedPitch );
	float roll = int8ToAngle( compressedRoll );

	ObjectID vehicleID;
	SpaceViewport & svp = this->getOrSetSpaceViewport( id, vehicleID );

	if (pHandler_)
	{
		pHandler_->onEntityCreate( id, type,
			svp.spaceID_, vehicleID, pos, yaw, pitch, roll,
			stream, versForCallIsOld_ );
		// TODO: If the entity is one of our viewports, then store its position
		// in our recvPosition cache.
	}

	this->detailedPositionReceived( id, svp, vehicleID, pos );
}


/**
 *	This method handles an updateEntity call from the server.
 */
void ServerConnection::updateEntity(
	BinaryIStream & stream, int /*length*/ )
{
	if (pHandler_)
	{
		ObjectID id;
		stream >> id;
		pHandler_->onEntityProperties( id, stream, versForCallIsOld_ );
	}
}


/**
 *	This method handles voice data that comes from another client.
 */
void ServerConnection::voiceData( const Mercury::Address & srcAddr,
	BinaryIStream & stream, int /*length*/ )
{
	if (pHandler_)
	{
		pHandler_->onVoiceData( srcAddr, stream );
	}
	else
	{
		ERROR_MSG( "ServerConnection::voiceData: "
			"Got voice data before a handler has been set.\n" );
	}
}


/**
 *	This method handles a message from the server telling us that we need to
 *	restore a state back to a previous point.
 */
void ServerConnection::restoreClient( BinaryIStream & stream, int length )
{
	ObjectID	id;
	SpaceID		spaceID;
	ObjectID	vehicleID;
	Position3D	pos;
	Direction3D	dir;

	stream >> id >> spaceID >> vehicleID >> pos >> dir;

	if (pHandler_)
	{
		pHandler_->onRestoreClient(
			id, spaceID, vehicleID, pos, dir, stream, versForCallIsOld_ );
	}
	else
	{
		ERROR_MSG( "ServerConnection::restoreClient: "
			"No handler. Maybe already logged off.\n" );
	}


	if (this->offline()) return;

	BaseAppExtInterface::restoreClientAckArgs args;
	// TODO: Put on a proper ack id.
	args.id = 0;
	this->bundle() << args;
	this->send();
}


/**
 *	This method is called when something goes wrong with the BaseApp and we need
 *	to recover.
 */
void ServerConnection::restoreBaseApp( BinaryIStream & stream, int length )
{
	// TODO: We should really do more here. We need to at least tell the handler
	// like we do when the cell entity is restored. It would also be good to
	// have information about what state we should restore to.

	ServerMessageHandler * pSavedHandler = pHandler_;
	this->disconnect();

	pHandler_ = pSavedHandler;
}


/**
 *	The server is telling us the identity of a version point
 */
void ServerConnection::versionPointIdentity(
	const ClientInterface::versionPointIdentityArgs & args )
{
	ResUpdateRequest * rur = this->findRURequest( args.rid );

	if (rur == NULL)
	{
		ERROR_MSG( "ServerConnection::versionPointIdentity: "
			"Unmatched response to request rid %d\n", args.rid );
		return;
	}

	rur->pMessageData_->reset();
	BinaryOStream & bos = *rur->pMessageData_;
	bos << args.version << args.index;

	rur->progress_ = uint32(-1);
	rur->ready_.push();
}

/**
 *	The server is telling us the summary of a version point
 */
void ServerConnection::versionPointSummary(
	const ClientInterface::versionPointSummaryArgs & args )
{
	ResUpdateRequest * rur = this->findRURequest( args.rid );

	if (rur == NULL)
	{
		ERROR_MSG( "ServerConnection::versionPointSummary: "
			"Unmatched response to request rid %d\n", args.rid );
		return;
	}

	rur->pMessageData_->reset();
	BinaryOStream & bos = *rur->pMessageData_;
	bos << args.change << args.census;

	rur->progress_ = uint32(-1);
	rur->ready_.push();
}


/**
 *	The server is giving us a fragment of a resource that we have requested.
 */
void ServerConnection::resourceFragment( BinaryIStream & stream, int length )
{
	uint32 argsLength = sizeof(ClientInterface::ResourceFragmentArgs);
	ClientInterface::ResourceFragmentArgs & args =
		*(ClientInterface::ResourceFragmentArgs*)stream.retrieve( argsLength );
	length -= argsLength;

	// see if it is proxy data
	if (args.flags & 0x40)
	{
		uint i;
		for (i = 0; i < proxyData_.size(); ++i)
			if (proxyData_[i]->pdid == args.rid) break;
		if (i == proxyData_.size())
		{
			ProxyDataTrack & pdt = *new ProxyDataTrack;
			pdt.last = NULL;
			pdt.pdid = args.rid;
			pdt.segCountTgtMod = uint16(-1);
			pdt.segCount = 0;
			pdt.accLength = 0;
			proxyData_.push_back( &pdt );
		}
		ProxyDataTrack & pdt = *proxyData_[i];

		if (args.flags & 1/*first*/)
			args.seq = 0;	// method indicator not used

		// make the new segment
		ProxyDataSegment * pNewSeg = (ProxyDataSegment*)
			new char[sizeof(ProxyDataSegment)-1+length];
		pNewSeg->length = length;
		pNewSeg->seq = args.seq;
		memcpy( pNewSeg->data, stream.retrieve( length ), length );

		// find its place in the list
		ProxyDataSegment ** ppSeg = &pdt.last;
		while (*ppSeg && uint8((*ppSeg)->seq - args.seq) < uint8(0x80))
			ppSeg = &(*ppSeg)->prev;
		// and insert it
		pNewSeg->prev = *ppSeg;
		*ppSeg = pNewSeg;

		if (args.flags & 2/*final*/)
			pdt.segCountTgtMod = uint8(args.seq+1);
		pdt.segCount++;
		pdt.accLength += length;

		// see if we are done (got last sometime and seg count matches)
		if (uint8(pdt.segCount) == pdt.segCountTgtMod)
		{
			// TODO: Could use our own BinaryStream here to avoid this copy
			// Also consider a ref-counted blob like BinaryPtr so handler
			// isn't forced to copy it again if it wants to keep it
			MemoryOStream mos( pdt.accLength );
			uint8 * mosPtrBeg = (uint8*)mos.reserve( pdt.accLength );
			uint8 * mosPtr = mosPtrBeg + pdt.accLength;
			for (ProxyDataSegment * pSeg = pdt.last; pSeg; pSeg = pSeg->prev)
				memcpy( mosPtr -= pSeg->length, pSeg->data, pSeg->length );
			MF_ASSERT( mosPtr == mosPtrBeg );

			if (pHandler_ != NULL)
				pHandler_->onProxyData( pdt.pdid, mos );

			for (ProxyDataSegment * pSeg = pdt.last, * pPrv; pSeg; pSeg = pPrv)
			{
				pPrv = pSeg->prev;
				delete [] (char*)pSeg;
			}
			delete &pdt;
			proxyData_.erase( proxyData_.begin() + i );
		}
		return;
	}

	//TRACE_MSG( "ServerConnection::resourceFragment: %d bytes\n", length );

	ResUpdateRequest * rur = this->findRURequest( args.rid, false );

	if (rur == NULL)
	{
		ERROR_MSG( "ServerConnection::resourceFragment: "
			"Unmatched response to request rid %d\n", args.rid );
		return;
	}

	uint8 seq = args.seq;
	bool externalWriter = (rur->pFile_ == NULL);

	// see if this is the first fragment
	if (args.flags & 1)
	{
		seq = 0;
		uint8 method = args.seq;
		rur->minires_ = method;
		// switch to external writer mutex semantics
		if (externalWriter && !(args.flags & 0x80))
			rur->ready_.push();	// as long as this isn't an error
	}
	else
	{
		// TODO: Care about seq here
		//MF_ASSERT( seq >= rur->progress_ );
		//if (seq > rur->progress_)
		//{
		//	rur->progress_ = seq;
		//	// TODO: put on queue instead!
		//}
	}

	if (externalWriter) rur->ready_.pull();
	// do {
	if (externalWriter)
	{
		rur->pMessageData_->transfer( stream, length );
	}
	else
	{
		if (length > 0 &&
			fwrite( stream.retrieve( length ), length, 1, rur->pFile_ ) != 1)
		{
			rur->minires_ = 0xFE;	// but keep going nonetheless
			ERROR_MSG( "ServerConnection::resourceFragment: "
				"Error writing %d bytes of received data to file\n", length );
		}
	}
	rur->progress_++;
	// } while (more next on queue)

	// see if this is the last fragment
	if (args.flags & 2)
	{
		MF_VERIFY( this->findRURequest( args.rid, true ) == rur );
		rur->progress_ = uint32(-1);
		if (args.flags & 0x80)
		{	// error mid-stream
			rur->minires_ = 0xFF;
			rur->progress_ = 0x80000001;
		}
		if (!externalWriter) rur->ready_.push();
	}

	if (externalWriter) rur->ready_.push();
}


/**
 *	The server is informing us of the status of resource versions,
 *	or something related to them anyway.
 */
void ServerConnection::resourceVersionStatus(
	const ClientInterface::resourceVersionStatusArgs & args )
{
	TRACE_MSG( "ServerConnection::resourceVersionStatus: %lu %lu\n",
		args.latestVersion, args.imminentVersion );

	// is it a friendly notification of a new impending version?
	if (args.latestVersion == uint32(-1))
	{
		if (args.imminentVersion != uint32(-1))
		{
			// yes it is, why well we'll pass the message on then
			impendingVersion_ = args.imminentVersion;
			sendResourceVersionTag_ = false;	// in case it was still set
			if (pHandler_ != NULL)
				pHandler_->onImpendingVersion( ServerMessageHandler::DOWNLOAD );
			// the handler should call entitiesEnabled with the new version
			// when it has finished downloading all the resources for it
			// (or it could be booted off...)
			return;	// done with processing this message for this case now
		}
	}
	else
	{
		latestVersion_ = args.latestVersion;
		impendingVersion_ = args.imminentVersion;
		imminentVersion_ = args.latestVersion;	// (only > if entities enabled)
		// TODO: lines above are correct, but maybe other things should change
		// (our idea of imminent version is not available to client so we
		// do not want to set it to the server's idea of it) client can
		// figure it all out from current stuff anyway, so maybe it's ok
	}

	// otherwise it's some kind of rejection (wrong versions or 'sit tight')
	entitiesEnabled_ = false;
	if (pHandler_ != NULL)
		pHandler_->onEnableEntitiesRejected();
	// don't bother passing 'sit tight' to the handler, it may not be reliable
	// by the time we call it next, and it needs to check its resources anyway
}


/**
 *	The server is tagging this bundle with an indication of the version of
 *	resources that were used to construct the messages contained within it.
 *	TODO: Have to figure out when we are going to start and stop sending
 *	our own tag ... starting is pretty easy (we get one)... not so stopping.
 */
void ServerConnection::resourceVersionTag(
	const ClientInterface::resourceVersionTagArgs & args )
{
	// we should not get any of these while entities are not enabled
	// also there is a race condition if we ever do between the time we
	// attempt to enable entities and the time we actually get back a
	// negative response... so if it occurs we should find these and fix them
	if (!entitiesEnabled_)
	{
		CRITICAL_MSG( "ServerConnection::resourceVersionTag: "
			"Received resourceVersionTag while updates not enabled.\n"
			"If this occurs it should be looked into and fixed on the lcpm\n" );
		return;
	}

	// if we get a tag then we want to send one
	sendResourceVersionTag_ = true;	// keep sending for max packet delay
	sendResourceVersionTagUntil_ = this->lastMessageTime()+10.0;

	// as soon as we see one of these, tell the handler to start loading in
	// the new resources ... it should call resourceLoadinComplete when done
	// (if it's not fast enough, then the server will eventually boot it off)
	if (imminentVersion_ != impendingVersion_)
	{
		imminentVersion_ = impendingVersion_;
		if (pHandler_ != NULL)
			pHandler_->onImpendingVersion( ServerMessageHandler::LOADIN );
	}

	// see if the tag indicates that this is the latest version
	if (args.tag == uint8(latestVersion_))
	{
		versForCallIsOld_ = false;
		return;
	}

	// see if the tag indicates that this is the impending version
	if (args.tag == uint8(impendingVersion_))
	{
		// ok, time to migrate then!
		previousVersion_ = latestVersion_;	// only time previousVersion is set
		latestVersion_ = impendingVersion_;
		this->send();						// last bundle under old version

		INFO_MSG( "ServerConnection::resourceVersionTag: "
			"Migrating to new version %d\n", latestVersion_ );

		if (pHandler_ != NULL)
			pHandler_->onImpendingVersion( ServerMessageHandler::MIGRATE );

		versForCallIsOld_ = false; // the version is not old at all! :)
		return;
	}

	// ok I guess it must be the previous version then...
	if (args.tag == uint8(previousVersion_))
	{
		versForCallIsOld_ = true;
		return;
	}

	// if we got to here then something fishy is going on
	ERROR_MSG( "ServerConnection::resourceVersionTag: "
		"Received unmatched version tag %d! Something has stuffed up!\n",
		args.tag );
}


/**
 *	This method handles a message from the server telling us that we have been
 *	disconnected.
 */
void ServerConnection::loggedOff( const ClientInterface::loggedOffArgs & args )
{
	INFO_MSG( "ServerConnection::loggedOff: "
		"The server has disconnected us. reason = %d\n", args.reason );
	this->disconnect( /*informServer:*/ false );
}

// -----------------------------------------------------------------------------
// Section: SpaceViewport methods
// -----------------------------------------------------------------------------

ServerConnection::SpaceViewport::SpaceViewport() :
	gatewaySrcID_( 0 ),
	gatewayDstID_( 0 ),
	spaceID_( 0 ),
	selfControlled_( false ),
	rcvdPosition_( 0, 0, 0 )
{
	for (uint32 i = 0; i < 256; i++)
		sentPositions_[i] = Vector3::zero();
}


// -----------------------------------------------------------------------------
// Section: Mercury
// -----------------------------------------------------------------------------

#define DEFINE_INTERFACE_HERE
#include "common/login_interface.hpp"

#define DEFINE_INTERFACE_HERE
#include "common/baseapp_ext_interface.hpp"

#define DEFINE_SERVER_HERE
#include "common/client_interface.hpp"

// servconn.cpp
