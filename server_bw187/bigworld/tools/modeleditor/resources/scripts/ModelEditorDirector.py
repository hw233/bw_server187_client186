import ModelEditor
import keys
from keys import *

class ModelEditorDirector:
	def __init__( self ):
		print "__init__"
		pass

	def onStart( self ):
		pass

	def onStop( self ):
		ModelEditor.saveOptions()

	def onResume( self, exitCode ):
		pass

	def ownKeyEvent( self, key, modifiers ):
		return False

	def handleWheelCameraSpeed( self, mz ):
		return False

	def onMouseEvent( self, mx, my, mz ):
		return False

	def updateOptions():
		pass

	def updateState( self, dTime ):
		pass

	def render( self, dTime ):
		print "render"
		pass