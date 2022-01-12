// -----------------------------------------------------------------------------
// Section: Globals
// -----------------------------------------------------------------------------

var gLiveQuery = null;

// -----------------------------------------------------------------------------
// Section: Update methods
// -----------------------------------------------------------------------------

function toggleLive()
{
	updateLive( !Query.getLiveButton() );
}

/**
 *  Enter or leave 'live mode' depending on the argument or the state of the
 *  button in the form.
 */
function updateLive( goLive )
{
	if (goLive == undefined)
		goLive = Query.getLiveButton();

	var outpane = OutputPane.get();

	if (goLive && !gLiveQuery)
	{
		outpane.clear();
		autoHideFilters();

		gLiveQuery = new Query.Query( true );
		gLiveQuery.readForm();

		var liveArgs = gLiveQuery.getServerArgs( outpane );
		if (!liveArgs)
			return;

		liveArgs.time = ServerTime.get();
		liveArgs.period = "to present";
		liveArgs.direction = 1;
		liveArgs.live = true;

		gLiveQuery.fetch( outpane, liveArgs, 1000 );
	}

	else if (!goLive)
	{
		if (gLiveQuery && gLiveQuery.task)
			gLiveQuery.task.terminate();

		outpane.setLoading( false );
		gLiveQuery = null;
	}

	Query.setLiveButton( goLive );
	outpane.followOutput = goLive ? 1 : -1;
	return goLive;
}

// Fetch log entries from the form on the page
function staticFetch()
{
	// Exit live mode if we're in it
	if (Query.getLiveButton())
	{
		updateLive( false );
	}

	var outpane = OutputPane.get();
	outpane.frame.contentWindow.focus();
	outpane.followOutput = -1;
	outpane.clear();
	autoHideFilters();

	var query = new Query.Query();
	query.readForm();
	query.fetch();
}

function autoHideFilters()
{
	if (document.filters.autoHide.checked)
		Table.get( "filterstable" ).hide();
}

function resetManageMenu()
{
	document.filters.manageQueries.value = "(select)";
}

// ----------------------------------------------------------------------------
// Section: Preference management
// ----------------------------------------------------------------------------

var gSavedQueries = new Object();

function saveQuery( name )
{
	if (!name)
		return;

	var query = new Query.Query();
	query.readForm();
	query.form.name = name;

	var onSuccess = function( dict )
	{
		fetchQueries(
			function(){ document.filters.savedQueries.value = name; } );
	}

	Ajax.call( "saveQuery", query.form, onSuccess );
}

function deleteQuery( name )
{
 	var onSuccess = function( dict )
	{
		fetchQueries(
			function(){ loadQuery( document.filters.savedQueries.value ) } );
		Util.info( "Deleted '" + name + "'" );
	}

	Ajax.call( "deleteQuery", {name: name}, onSuccess );
}

function loadQuery( name, doFetch )
{
	var query = gSavedQueries[ name ];
	if (!query)
		Util.error( "Unknown query: " + name );
	else
		query.writeForm();

	if (doFetch)
	{
		if (!updateLive())
		{
			staticFetch();
		}
	}
}

function navigateToQuery()
{
	var query = new Query.Query();
	query.readForm();
	window.location.search = query.toURL();
}

function fetchQueries( onDone )
{
	var onSuccess = function( dict )
	{
		if (dict.queries.length == 0)
		{
			Util.error( "Server didn't reply with a list of queries" );
			return;
		}

		DOM.clearChildren( $("loadQueries") );
		DOM.clearChildren( $("deleteQueries") );
		gSavedQueries = new Object();

		for (var i in dict.queries)
		{
			var query = new Query.Query();
			query.form = dict.queries[i];
			gSavedQueries[ query.form.name ] = query;

			var name = query.form.name;
			DOM.actionMenuAppend( $("loadQueries"), name,
								  "loadQuery('" + name + "')" );

			if (name != "default")
			{
				DOM.actionMenuAppend( $("deleteQueries"), name,
					"deleteQuery('" + name + "')" );
			}
		}

		if (onDone)
			onDone();
	};

	Ajax.call( "fetchQueries", {}, onSuccess );
}

// ----------------------------------------------------------------------------
// Section: Keypress handling
// ----------------------------------------------------------------------------

// Whether or not we allow the focus shortcuts
var shortcutsEnabled = true;

// The currently focused input element
var activeElement = null;

// Set up focus handlers for the form elements
for (var i in document.filters)
{
	if (document.filters[i] instanceof HTMLInputElement ||
		document.filters[i] instanceof HTMLSelectElement)
	{
		var elt = document.filters[i];
		elt.onfocus = function () {
			shortcutsEnabled = false; activeElement = this;
		}
		elt.onblur = function () {
			shortcutsEnabled = true; activeElement = null;
		}
	}
}

function focusAndSelect( elt )
{
	elt.focus(); elt.select();
}

// ----------------------------------------------------------------------------
// Section: Init code
// ----------------------------------------------------------------------------

// Set client and server time offset along with the client/server timezone diff
ServerTime.setOffset( serverTimeOffset, timezoneOffset );

// Call tables init early cause we need to mess with them NOW
Table.init();

// Set up output pane
var filtdim = elementDimensions( $("filterstable") );
OutputPane.create( $("outputGoesHere"), {form: document.filters} );
Table.get( "filterstable" ).onResize = function(){ OutputPane.get().fill(); }

// Use browser query string if given, otherwise load default query
if (window.location.search)
{
	var q = new Query.Query();
	q.fromURL();
	q.writeForm();

	fetchQueries();
}
else
{
	fetchQueries( partial( loadQuery, "default", true ) );
}
