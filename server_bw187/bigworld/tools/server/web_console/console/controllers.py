import telnetlib
import re

from turbogears.controllers import (RootController, expose, error_handler,
                                    flash, redirect, validate)
from turbogears import widgets, validators, identity

import bwsetup; bwsetup.addPath( "../.." )
from web_console.common import ajax
from web_console.common import module
from web_console.common import util
from pycommon import cluster

class Console( module.Module ):

	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
		self.addPage( "Connect To Process", "list" )
		self.addPage( "Help", "help", popup=True )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_panes" )
	def help( self ):
		return dict( PAGE_TITLE="BigWorld WebConsole: Python Console" )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_left_pane" )
	def helpLeftPane( self ):
		return dict( section="console" )

	# Request Processing
	@identity.require(identity.not_anonymous())
	@ajax.expose
	def process_request(self, line, host, port):
		tn = telnetlib.Telnet( str(host), str(port) )

		# Read the connection info from the component
		tn.read_until( ">>> " )

		# do the line write / read
		result = self.process_line(tn, line)

		# Close the session
		tn.close()

		# Prefix the command with a python console indicator, and
		# strip off the trailing console marks as well as the
		# \r\n that telnet has sent through
		result = ">>> " + result[:-6]

		return dict( more=False, output=result )


	def process_line(self, tn, line):
		line = line + "\r\n"

		# Write out the command
		tn.write( str( line ) )

		# Read back the response (including the command executed)
		(index, regex, result) = tn.expect( [ ">>> ", "\.\.\. " ] )

		return result


	# Multiline processing
	@identity.require(identity.not_anonymous())
	@ajax.expose
	def process_multiline_request(self, block, host, port):
		(tn, output) = self.connect( host, port )

		if tn:
			# Force a final line after the text block to ensure
			# any indented code is executed by the interpreter
			block = block + "\n"

			lines = [line for line in block.split('\n')]

			output = ""
			for line in lines:
				output = output + self.process_line( tn, line )

			# Close the session
			tn.close()

			# Prefix the command with a python console indicator, and
			# strip off the trailing console marks as well as the
			# \r\n that telnet has sent through
			output = ">>> " + output[:-6]

		return dict( more=False, output=output )


	def connect(self, host, port):
		tn = None
		result = None
		try:
			tn = telnetlib.Telnet( str(host), str(port) )

			# Read the connection info from the component
			result = tn.read_until( ">>> " )

			return (tn, result[:-6])
		except:
			return (tn, result)


	## Main console
	@identity.require( identity.not_anonymous() )
	@expose( template="console.templates.console" )
	def console( self, host, port, process, tg_errors=None ):
		tn, banner = self.connect( host, port )
		if tn:
			tn.close()
		banner = re.sub( "\r\n", " - ", banner)
		return dict( hostname = host, port = port,
					 process_label = process, banner = banner )


	@identity.require( identity.not_anonymous() )
	@expose( template="console.templates.list" )
	def list( self, user = None ):

		c = cluster.Cluster()
		user = util.getUser( c, user )

		# Generate list of processes that have a python console
		procs = sorted( [p for p in user.getProcs() if
						 p.name in ("cellapp","baseapp","bots")] )

		# Generate mapping for python ports
		ports = {}
		for p in procs:
			port = p.getWatcherValue( "pythonServerPort" )
			if port:
				ports[ p ] = port
			else:
				procs.remove( p )

		# Generate labels
		labels = {}
		for p in procs:
			labels[ p ] = "%s on %s" % (p.label(), p.machine.name)

		return dict( procs = procs, ports = ports, labels = labels,
					 user = user )


	@identity.require( identity.not_anonymous() )
	@expose()
	def index( self ):
		raise redirect( "list" )
