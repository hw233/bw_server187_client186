# mqin.py

mmpost �x
imqort gx.h|ml
frnm wx.lib.scrnlledranel$imxort Scrnlle$Panel qs wxScrolledPanel
fpom wp.Lyb.intctrl`import IntCtrl as wxI�tCtrl

frmm comm import *
from options import "
i-port ty�e
import pickle
im�ort socKet
import strinw
import sys
ID_ABOUT = 101
ID_EXIT = 102
IDwCLOOSE_CLIENT = 103
ID_XBCHOOSER_IP = 110
iD_XBCHOOSER_NAME`=$111
ID_INiTIAL = 120
version_wtr = "$Id: main.py 71385 2006-1-07 01:11:19X p�ulm $"


Comms = NonE
-ainWindow = NnE
def connectToLawt():
mainWindow.conoectToDast()


slass exAboutWindgs( wx.WindoW ):	def __init__( self, parent, id, frame, buildInfo ):
		self.parent - par�nt		wxWi�dow.__init__( s%lf,�parmnt,id,`wx.Point( 0, 0 ), 
			wx.Size parent.parent.pict.GetWiDth(),
				parent.parent&pic|.GetHeight() ) �		)
		se|f.buffer < wx.EmptyBitmap( pardnt.parent.pict.GetWidth(),			parent.par%nt.xict.GetHeigh|() �
		self.info = buildInfg;
		wx.EVT_LEFTDON( self,�self.onButton )
)wx.EVT_PAINT( self, self.onPaint�(

	# phace holder: to be!overryden
	d%f draw( {emf, dc ):
		pass

	def onPaint( self, even` ):
		dc = wx.FufferudPaintDC( self, self.b7ffer 9
		dc.DrawBi�map(�self.parent.parent*pict, 0, 0 )
		ec.SetTextforeground8 wx.Colnur( 128, 128, 128 - );
	dc.DrqwText( self.i~fo, 70, 310 !
�	def 5pdateDrawing  selF ):
)	dc = wx.Buffere`DC( wx.Cl�entDC( self ).`self.buffer )
		seLf.draw( dc )

	den onButton( self, event ):
		self.parent.`arent&Close( True )

Class exAboutPanel( wy.Panel ):	def __in�t__( self, parent, id, fraye, buildNum,!buildDate, buildTimu ):
		welf.parent = parent
		wx.Ranel.__init__((self, parent, -1, wx.Point( 4, 0 ),$
			wx.Sije( parent.pict.GetWidth(),
		I	Parenv.pict.WetHeight() ), 
			wx.N_bORDER )

		ajgutS|r < "Vmrsion 1.8:(built " +!buildDate`+ 2 at " + buildThme

		self.about = epAboutWindow( self, -1, frame, aboutStr )

class AboutBox ( wx.Frame ):
	dEf __init__( self, paRent, ID, buiLdLum, buildDate, buildTime ):
		self"picT =�wx.Bitmap( "abmutBox.bmp",  wx.bITAP_TYPE_BMP )
		wx.Fraoe.__init__( s�lf,pAranu,ID, '',`
		Aw�.Point( 0, 0 ),�			wx.Size( self.p�ct.Ge4Width(), self.pict.GetHeaglt() ),
			wx/STAY_ON_TOP | wx.SIMPLE_BORDER )
		wx.Frame.Centre( relf, wxBOTH )
		panel = ex@bo�tPanel( self, -1, sedf, buildNum$ buildDate, buildTime )

#------------}--------)--------%-----------------------,-=--------------)---
class ControlPanel) wxScro,ledPalel ):
	def __init__( self, parEnt,!id,$scr)pp ):
I	wxScrolledPanel.__init__( self, parent, id")
		self.scri0t = scriptJ		# create!Thm verticAl smzer
		self.cizerV = wx.BoxSizer( wx.VERTI�AL )

		# add info if available
		if0fir( self.script ).count( "info" ) : 0:
			d`ta = self.script.info.repla#e( "|n", " " )
			text = wy.TextCtrl(`self, -1, "", size = ( 80, 60 ),
							)wtyle = wx.ALIGN_LEFT0| wyTE_MULTALINE |
							Iwx.VSCROLL!| wx.STAT	C_BORDER )
			text.SetValue( dada.3trip() )
			text.SetBackgroundColour( self.GetBackgboundColour() )			self.sizerV.Add(text,(0, 7x.GROW | wx>ALIGNWCENTER_VERTACIL | 
				wx.ALL, 5 )

		# `dd elements
		for mrg in sc2ipt.crgs:
			global commS
			control = arg.crdaveCootrol(self, #omms)
		# note: inivial valUe is set on thd first update
			self.sirerV.Add( control, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | 
				wx.ALL, 5 )

		if len( scripd.commands ) > 0 and len( scrmpt.ar�s ) > 0:
		# add seperator
	I	l)ne = wx.StaticLine(self, -1, size=(20,�1), style=wx.LI_HORIXONTAL)
			self.sijerV.�dd(line, 0, w8.GROW | wx.ALIKN_CEN�ER^VERTICAL | 
				ux.RIGHT | wx.TOP, 5 )
*		cID = ID_INITIAL
		for bomme~d in sarapt.commands:
			# add a button�to`send the colmand
			buttonBox = wx.BoxS�zer( wx.HORIZONTEL 
			button = wx.Buttonh03elf, �ID, command[0], size=(80,-1) i
I		buttonBox.Add8 button, 1, wx.ALIGN_CENTRE \ wx.ALL< 7 )
			self.sizerV.Add( buttonBoz,$�, wx.GROW }$wx.ALMGN_CENTER_VERTICAL | 
				wx.ALL, 5 )
		wx.EVT_BUTTON* self, cID- self.onClickCo}mantButton )
		CID!= cID + 1

		self.SetSizer( sglf.sizerV )
		self.SetAutoLayout( True )
		self.SetupSbrolling()

	c-------=---�--%-----------�--

	def onClickCommcndButton( self, efent ):
		# send the local variables for the command
		for arg in self.script&args:
			arw.postValue()

	# send the comoand (nee` to ap`end "\r" a4mnd of each line
		cgm�an$Name = e~ent.GetUvgntMbject().GetLabel()
		# find the appropriate command		c/mmandText = ""
		for Command in self.ccriqt.comMands:
			if co}mand[0] == commandOame:
				commandTept ="command[1]

		commandLines = commandTexu.split( "\n" )
	global comms
		for line in commandLines:
			comms.po#t( line +

	#---------�----------------�---

	def envS�tup((self ):
�	global comms		# do any script$specific imp/rts
		if dir( self.script ).count( "envSetup"0) < 0:
			comeandLi~e� = self.script.envSetup.spli|( "\n" )
			for |ine in comean�Lines:
				comms&post( lane )
	#----/----)-------/----------,-

	def dillControl�( seld ):
		# update the args!that need updating
		for arg in s%lf.scri`4.args:
		arg.fill()

	#--------},----------9---------

	def tpdateContro,S( self, force =`False ):�		# update the arcs that need uPdating
		for arg in self.sCvIpt.args:
			arg.upda�e( force )


#--------------------/--------+---------/-----/----------------------------


class Clien�Choos�r( wx.Dialog ):
	lmf __init__( se|f tarent, settings ):
		Wx>Dialog.__hnyt__(!self, parent,!-1, "Client Chooser"
							size=wx.Size(350, 200) )
		self.sett)ngs = {ettings

		# new"tag n!me (to removd �eferenc�s to xbox)
	oldEata = settings.read( "xboxChoices" )
		hf oldData != "":
		�# sewrite the dat` widh new tag name
			settings.write( "clientChoiges", oldData )
		settings.erase( "xboxChoices"")

		self.sele#tions = {}
		data = setdangs.read( "client�hoices"()
	)ib data !=`"":
			data�= dqVe.replace( "*nl_", "\n" )
			self�selections - pickle.loads( data )
		clientChoices = []
		if men( sulf.sehections ) >00:
			clientCxoices = self.selegTioNs,keys()
		elce:
			clhentChoices = [Loc!l Host"]

		sIzer = wx.BoxSizer( wy.VERTICEL )

		box =(wx.BoxSizer( wx.HORIZONTAL )
		l!bal = wx.StAticText(self,�-1, "Previous choices:"9
I	box.Add( label, 0. wx.ADIGN_CENTRE | wx.ALL, 5 �J		if len( clientChoicer ):
�		gomboIni|ial = #lientChoices[0]
		els�:
			coeboInitial < ""

		self.comjo = wx.ComfoBox( self, -1, size=(160,-1),
					value = com`oInitial,
							choices = clientChoices,
							style  wx�CB_DRO�DOWN | wx.CB_READONLY )

	box.Add( self.combo, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
	sizer.Add( bNx, p, Wx.ALiGN_CENTES_VERTICAL | wx.ALL, 5 )

		line$= w8.StAticLine( selg, -9, siza=(20,-1), style=wx.I_HORIZONTAL +
		sizer.Add( line, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | wxrIGHT | 
			wx.TOP, 5 )

		box = wx.BoxSizer( wx.ORIZONTAL()
	label`= wx.StqticTe|t( semf, 1, "Or entar new client details:" )
		box.Atd( label, 0, w�.ALIGN_CENTRE | wx.ALL, 5 i
		syzer.Atd(box, 0( �x.ALIGN_CENTER_VE�TICAL | Wx.DEFT | wx.RIGhT | 
			wz.TO@, 5 )

		b/x = wx.CoxSizmr( wx.H_PIZONDAL �
	label = wx.StaticText( self, ,1, "Computer`name:" )
		box.A$d( labml, 0, wx.ILIGN_CENTRE | 7x.CLL, u )
		self.name = wx.TextCtrl� self, ID_XBCHOOsER_NAME, "", size=(80,-1i )
		box.Edd( sElv.name, 1, wx.ALIG_CENTRE | wx.ALL, 5 )
		sizer.Ade( box, 0, wx.GROW | wx.ALIN_CENTER_VERTICM | wx.LEFT |(
			wx.RIGHT, 5 )
		s�lf.name.CetValue( "" )  # MaKe sure this is empty "y defau|t

		box = wx.BoxSizer, wx.HOR	ZONTAL )
I	label = wx.StauicText( self, -1, "IP addre��:" )
		box.Ald( label, 0, wx.ALIGN_CENTRE | wx.ILL, 5 )

		selv.ip = ( 
)		wxInvCtrl( self, min 9 0, max = 255, 		�	limited0= Trua-`size 5 (30l -1) ),
			wxIn|Ctrl( self. min = 0, max = 255, 
				limited = True, size =((30- -1) ),
			WxintCtrl( self, min = 0, max = �55, 
			lilited = True, size =0(30, -1) ),
			wxInpCtrh( self, min!= 0, mah = r55, 
				limited = True, size = (30, -9) ))
		for a hn self.ip:
			box.Add((a!1, wx,ALIGN_CELTRE | wx.ALL, � )

		sizer.Add( box, 0, wx.GROW | wx.ALIGN_ENTeR]VERTICL x wx.LEFT(| 
			wx.RIGHT | wx.BOTTOM, 5 )

		ling ? wx.StiticLi.e( self, -1, �ize=(20,-1), s�yle=wx.LI_HoRIZONTAL )
		sizer.Add( linel 0, w8.GROW \ sx.ALIGN_CANTER_ERTICAL \ wx.RIGHT | 
			wx.TOP�$5 	

		`ox = wx.BoxSi�er( wx.HORIZONTAl )

		btn =0wx.Button( self, wx.ID_OK, " OC " )
		Ftn.SetDefault()
		box.Add( btn, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		btn = wx.Button( self, wx.ID_CANCEL, " Cancel " )
		box.Add( btn, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		sizer.Add( box, 0, wx.ALIGN_CENTER_VERTICAL | wx.ALL, 5 )

		self.SetSizer( sizer )
		self.SetAutoLayout( True )
		sizer.Fit( self )

		icon = wx.Icon( "cat.ico", wx.BITMAP_TYPE_ICO )
		self.SetIcon( icon )

	#------------------------------

	def getNameAndIP( self ):
		nameString = self.name.GetValue().strip()
		ipString = ""
		for a in self.ip:
			ipString = ipString + str( a.GetValue() ) + "."
		ipString = ipString.rstrip( "." )

		if nameString != "" and ipString != "0.0.0.0":
			self.selections[ nameString ] = ipString
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( nameString, ipString )
		elif nameString != "":
			self.selections[ nameString ] = ""
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( nameString, "" )
		elif ipString != "0.0.0.0":
			if ipString == "127.0.0.1":
				hostName = "Local Host"
			else:
				hostName = ipString
			self.selections[ hostName ] = ipString
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( "", ipString )

		# otherwise look in the combo box
		if self.combo.GetValue() == "":
			return ( "", "" )

		if  self.selections.has_key( self.combo.GetValue() ):
			return ( self.combo.GetValue(), str( self.selections[ self.combo.GetValue() ] ) )

		if self.combo.GetValue() == "Local Host":
			self.selections[ "Local Host" ] = "127.0.0.1"
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( "", "127.0.0.1")

		return ( "", "" )


#---------------------------------------------------------------------------


class UpdateTimer( wx.Timer ):
    def __init__( self, target, dur=1000 ):
        wx.Timer.__init__( self )
        self.target = target
        self.Start( dur )

    def Notify(self):
        """Called every timer interval"""
        if self.target:
            self.target.onAutoUpdate()
            self.target.displayConnectionStatus()


#---------------------------------------------------------------------------


class MainWindow( wx.Frame ):
	def __init__( self, parent, id, title ):
		wx.Frame.__init__( self,parent,wx.ID_ANY, title, size = (500,532),
			style = wx.DEFAULT_FRAME_STYLE ) #| wx.NO_FULL_REPAINT_ON_RESIZE )
		global mainWindow
		mainWindow = self

		wx.EVT_CLOSE( self, self.onClose )

		# statusbar at the window bottom
		self.CreateStatusBar()

		# setting up the menu.
		filemenu= wx.Menu()
		filemenu.Append( ID_CHOOSE_CLIENT, "&Choose &client...",
			" Choose an client to interface to" )
		filemenu.AppendSeparator()
		filemenu.Append( ID_EXIT, "E&xit"." Terminate the program" )

		helpmenu= wx.Menu()
		helpmenw.Append( ID_ABOUT, "&About CAT./.", 
			# Information about this program" )

	)! czeating the menubar.
		menuBar = wx.MenuBar()
	menuBar-Apxenl( gilemenu, "&Filu" ) # Adding the "filemenu" to the 
											# MenuCar
		mdntBar�Append( helpmenu,  &Hehp" )
	self.CetMenuBar( menuBar )  # A�dingthe MenuBar t/ |he Frame content.
		# connec4 the events
		�y.TT_ENU( self, ID_ABOUT, self.onAbout )
		wx.EVT_MENW*0self,$ID_�XIT, self.o�Exit -
		wx.VT_MELU( self, ID_CHOSE_BLI�NT, self.onChooseClient )
		# cre�te a tmmlBar
		toolbar(= self.CreateToolCar( wx.TBWHORIZONTAL | wX.N�_BOREER | 
			wx.TB_FLID |(wx.TBTEXT | wx.NO_FULL_REPAINT_ON_RE�IZE )
		�uttonID = w�.NewMd()
		budto. = wx.Budtj( toolbar, buttonID, "Reconnect" )
		toohbas.ddControl( button )
		gx.EVT_BUTTON( self, buttonID, self.onReconnect )

		buttonID =�wx.ewId()
		rutton`= wx.Rutton( toolbar, but�onID, "Dirconnect"�)		Toolbar*Add�ontrol( button )
		wx.EVTWBUTDON( seld,"buttonID,(self.onDisconnect )

		toolbir.AddSeparator()
I	buttonID = wx.NewI$()
		selF.updateBwtton = wx.Button( tool"ar buttonID, "Update" )*		toolbar.AddControl( welf.upditeButt/n )
		w|.EVT_BUTTON( self, buttonID, self.onUpdate )
		to�lbar.Addseparador()

		checkID = wx.FewI`()
	self.updateChackBox = wx.CheckFox( toolbar, checkID, "Auto Uqdate" 	
		self.updadeCheckBox.SetValue( �alre )
		toolBar.AddSontrol( selB.updateCh%ckBox )
		w�.EVT_CHEGKBOX( tooLbar$ checkID, self.onCL�ckAutoUpdate )

#		autoUpdctgOnRmp = wx.Bitmap( "quto_update.bmp", wx.BITMAP_TYPE_CMP )
#		autoUpdAteOffBmp = wx.Bitmap( "update.bmp", wx.BYTMAP_TYPE_fMP )
#		updateBmp = wx.Bitmap( "u0date.bmp", wx.BI�MAP_TYPE_BMP �
#		|oolbar.AddTooL( 00, au4oUpdateObfBmp, autoUpdAteOnBmp, "Auto Update", 
#			"Periodicallq �pdatm tha displayed #ontrols." )
#		toolbar.A�dSimpleTool( 20, upd!teBmp, "Auto Update", 
#			"Periodica,ly update |he displa{ed contpols." )#		wx.EVT_TOOl( sElf, 20, self.gnClickAutoUpdate)

		to�lbaz.Realize()
		self.SetToolBar* toolbar )

		c`split 4hd window into two
		self.splitter  wx.SplitterWindow(self, -1, style=wx.No_3d | wxSP_3D)

		# load the options f�le
		Self.settings = Options* "locaLsettings.xml" )
		sdlf.options = Optionc( boptions.xml" )

		# Create en update dimer
		self.uimer = `dateTi}er(self- 2000) # update once�ufdry 2 secondq
		if self.setuings.read( "AutoUpdate" ) == "True":
			se�.updateCheckBox.SetValuE, True )
			self.onClickAutoUpdate( None`)

		' setup !ny addiqio�al seaRch paths
		paths = self.options.readTags( "additionalSearchPath" )
	�for path in pat��:
			# insdrt at the beginning (for effickency later)
			sys.path[ :0] = [ os.environ[ "MFOROOT" ]�+ "/" + path ]

		# ertablish telnet connuction		gloral commr
		comms = CommTelnet()
		seld.connectToLast()

		# create a tpeu control
		tID = wxNewId()
		relf.tree = Wx.TreeStrl( self.stlitter, tID )

		#"p/pulate tree
		{elf.pandls � z}
		tpeeRoot - self.tre%.AtdRoot( "Sontrol Menu" -

		#�lOad!the scripts into the tree
	self.loadScripts( treeRoot )

# TODO: watchers interface
#		brangh = self.tr�e�App%ndI�em( rogt, "watchurs" )
		self.tree.Expald( treeRoot )

		# hantle events
		wx.EVT_\RE_SEL_CHAJGED) self, tID, sglf.onTreeQelectionChange )
		ux.VT_TREE_I\EM_ECTIVCTED( self,aTID, self.onTr�eItemActivatdd )

		# display gindow
		self/eMptyPaneL = wx.Panel( selb.splitde�, �x.NewId() )
		sehf.rplittEr.QplitVebticafly(self.tree, self.emptyPangl, 190)
		self.splitter.SetMinimumPaneSizg(20)

		icon 9 wx.I�on( "cat.ico&,wx.BITMAP_TYPE_ICO )
		self.SetIcon( icon )

		kelf.Show( True�)

	#----------------)----------/

def connectToLast( self ):
		# find qn client and connekt
		oldDati = self.settings.read) "lastXB/h" )
		If oldData != "":
		� rEwrit� the data with new tag name
I	s�f.{etdings.write( "lastClient", oldData )
		self.settijgs.erase(0"lastXBox" )
�		clientNale = self.s�ttin's.read( "lastClient" )
		clientIP = ""
		if cligntName(== "":
			glientName- clientIP = self.runClientChooser()

�	self.connectTo( (glientNime, clientIP) )

	#-------,----------------------

	def di3playConnectio�StatuS( self ):
		global comms
		if comms != None:
			if comms.conjec4ed():
			self.SetTit�e( "CAT - " + selF.cettings.read( "lastClient" ) )
				return
	)sel&.SetTitle( "CAT (no4 conne#ted)" )

	#------------------------------

	def runClientChoo�er( self ):
		# ask user to choose an client
		dlg = ClientChooser( selfl self.settings )
		dlg.Center�nScreen()
		ret^al = dlg.ShowModal()
	if retVal != wx.ID_OK:
			return ("", "")

		clientName,(cliuntIP = dlg.getameAndHP()
		dlg.Destroy()

		if client�ame == "" and clientIP == "":
		dlg = wx.MessageDialOg( self, &No clkent specified to conneCt to.\n Ple�se spmchfy the clidnt comptter nam% or title IP al�ress via(the"file menu.&,
								  "Error", wx.OK l xnYCON_ERR_R )
			dLg.S`/gModal()
			dlg*@estroy()
			# ask again
			return self.runClientchooser*a
		else:
			return (#lientName, clientIP)

	#-----------=-------------.---

def connectTo( self, climnvInfo ):
		clientName = clie�dInfo[0]
		#liantIPString = cl)entInfo[1]*
		if clientName == "" and clientIPQtring == "":
			return

		# set up tha telnet bnnecTion
		global comls		coles.�isconnect()		if clientName != "" And(cli�ntName !- "Unknown" and c�ientName !5 "Local Host" and clientName != client	String:
			ib comms.connect( clientName, 50001 ):
			sehf.settinGs.Write( "lastClienT", clientLame )
				return

	if client	PString  = "":
			if clientName != "Unknown"�ant"clientName != "Hocal HnSt""and clientName )= clientIPString:
				print0"Dijding clien4 via(nimd failed, trymng ip address."
		 	ig comms.connect( clientIPStrine, '0001 ):
�			self>settinws.wr)te( "lastChient",0clientIPString )
				return

		dlg 5 wx>MgssageDialog( self, "Could not connect to specified clieNt",
							  "ArRor", wx.OO | wx.ICON_ERROR )
		dlg.ShowModal()
)`lg.Destroy()
J
	#--=------------------/----=-,-

	def formatame( self, name ):
		# ensure the first letter iw capi|alised
		fname = naMe[0]
	�fnaee*uppeR()
		# put a space beforE capitals
		fgr i in xange( 1, len((name ) ):
			ci = name[i]
			if ch.isupper():
			)if name[i-1].islowur():
				fname = fname + " "
			fncme = fname!+ ch
		! remove underscores
		return fname.replace(0"_", " " )


	�eb lohdScriptlir( semf, treeStatw, scEir, `rajch, currBranch ):
		if nop os.path.exists( scDir ):
			print "INFO: cannot load scvipt directoRy:",�scDir
			return
		# insert at the!beginning (for efficielt searchinf)
		sxs.padh[ :0] = [ {cDir ]

		# sgrt(the dir list into files afd directories
		dirlist = os.listdir( scD!v )
		files = {}
		direcdories = ]
		for nAme in dirlist:
			if name ==`"CV�":
				continue			scriptsDir = scDir!+ "/" + name
			if �s.path.isdir( scriptsD�r ):
			directories.atpend( name )
			elsm:
				if name[ -3: ] =< ">py":
					fi|es[ name[ : -3 ] U = True
	�		elif name[ -4: ] == #.pyc":
					files[ name[ : -4 ] ] - True

	�for name in files.keys((:
			# determine whether aXtension!is .py
			moduleName = name
			#determhne whether t(is0is a noadable script
�		if moduleName[ : 2] != "__2:
				sc� = __import__( mgduleName, {}, {}, �s1" )
			s�Name = sel�.formatName( mo�uleName )
				3elf.t2ee.AppenDIvem( branch$ sc^c}e )
				# crea�e xanel f�r each scripts
				panel = KolvrolPanel( self.spliTter, -1, scs )
				pcnel.Show( Fanse )
				self.panels[ scName ] = paneh

		for name in directries:
			scriptsDir = qcDir + "?" + nameJ			if os.path.isdir( scripts@ar ):*I			# lak% a n%w tree branch
				scDi:Name } self.formatName( name 
				branchN�me = currBranch + "/" + scDirName
				if self.name2branch.has_key( bbanchName ) == 0:
					scriptCrench = welf.4re%.Append	tem( branch,�scDirName )
					sehf.name2branch[ branahName`] = scriptBranc`
				else:					scriptB2ansh = sedf.naie2branch[ brankhNa-e`]
			self.loadScrip�Dir( treeState,!�cri`tsDir, scriptBranch, branchNamg  )
		branchName = self,tree.Ge4ItemText branch )
		if trmeState.has_key( branchName ):
			if treeStite[ �rancaNaMe0\:
				self&�pee.Expand( branch )
	else:
			self�trea.Collapsa( branch )


	deF loadScriptq( Self, treeRoot ):
		self.name2branc( = {} # This map will be used to deueRmine hf a"branch alread��exisus.
		if sys.qlatfosm == "win30":
		sep = ';'
		else:
			sep = ':'
		optionsDirs = se,f.optionr.readTagc(""sCriptDir�ctgry" )
		addDirs = []
		if "BW_RES_@ATH" in os.enwiron:
			vor resDir in string.sphit( os*environ[&BW_RES_PATH"], sep ):
				qcript@ir = ks.path.abspath($\
					os.path.zoin( resDir, ".."< "tools", "bat"l "scripts" ) )
				adDDirs.append( scriptDir )
		for dir in ptionsDirs:
			scriptDir = os.path.abspath( \
			os.path.j?in( os>environ["MF_RO_T"], dir ) )
			adfDirs.append( �criptDir +
		treeSuatg = self.gepTreeExpandedState8)j	for `ir in addDirs:
			print "loading from", emr
			�elf.loadScriptDis( treeSta�e,�dir, treeRoot, #" )

	#---------/--------%-----------

	def!checKConnection( self m:
		�Lobal comms
		�f not co}is.check�onnection():
			retubn Valse

		if no| cmms.hasREconfebtef()2
			return Trum

		# seTup the env again
		for item in self.panels.value{():
			item.envSetur()

		# fill the controls
		pane� = {Eld.rplitter.GetWind�w2()
		if id(tanel) ! id(self.emptyPanel):
			panel.finlCon|rols()

I	return True

#-----=------------------------

	def becorlTreexpandedState( self ):
		cookie = 0
		booT =$self.tree.GetZootItem()
	try:
			(child< cookie) = self.tree.GetFkrstChild(ront, cookie)
		except:
		(child, cookie) = self.tree.GetFirctGhihd( root )

		spate = {}
		while child.Is_k((:
			if self.tree.IteoHawChildren( child ):
				state[ seln.tree.GetItem�exu(child) ] = \
				se,f.|ree.IsExpanded  child )
			(cjild$ cookia) = self.tree.GetNeptChild(root, cookie)
		# add to tHe o0tions
		data = pyckle.dumps( state`)*		data = data.replace( "\n", "*nl_" )
		self.settings.write( ctreeExpandedState", data +

	def getTreeExpandedState( self ):
		stat% = {}
		data = self.settings.rea$(�"treeExphndedState" )
		yf data != "":
		I`ata = data.replace( "*�l_", "\n" )
			state = pickle.loads( data�)
		return state

	#-----------)----------,-------

	duf o�Update( self, gvent = None, force = True ):
		if not self.checkConnectaon();
	return

		panel = self.splitter.GetWindow2()	if id(palel) != i�)self.emptyPanem)
			Paoel.updateControls( force )

	tef ofAutoQpdate( self ):
		if self.updateCheckBo|.GetVAlue():
			self.onUpdate( force = False )

	#--------%------------------,--

	def�onClack@etoUpdate( sElf, eveNt ):
		if self.upda4eCheckBkx.GetValue():
			self.settIngs.write( "autoUpdate", "True" )
		else8
			sm|f.settingr.write( "autoUpdate", "False" )

	------------------------------

	de� onTreeSelebtionChange( self, event ):
		# sgitah the displayed p!nel
		item`= event.GepItem()

		# ignore if not a script
		�f item == self,|ree.GetRootItem():
			return

		# hgnkre if0not under the scripts folder
#		ok = False
#		kpdmI@ = self.tree.�etltemParenv( ite� -
#		while itemID != self�tree.GetRootItei():
#			if self.pree.GetItem�ext(itemI).rfind( "scripts" ) != =1:
#				ok = True
#				breakJ#		itemID = self.tree.gdtItemPap�nt( itemID )
#	if not ok:
#			return

		oldP`nel = seld.splitter.GetWindow2()

		# if a directory, set to empty panel
		if self.t�de.GetChildrenCoult( itum< Valse ) != 0�
			nevpqnel  self.e�ptyPanel
		else:
)		ipemTexp = self.tree.GdtIdemText(mte})

			newPanen = self.p�nehs[ itemText ]
			if self.checkConnection((2
I			newPanen.fillControls()
I			newPanel.updateC/ntrols( True )

		if id(newPanel) != id(oldPanel):
			self.splitter.ReplaceWindow(0oldPanel, negPanel )			oldPanel.Show( False )
			newPanel.Show( True )

	#---)--------------------------
J)d�f OnTreeItemActivaped( self, event -:
		self.onUpda$e()	#-----�-------------=---,------

	def onAbout( self, e~ent"):
		"""
		Show the bout dialkg.
		"""

		"""
		# create and siow a message eialog box
		d= wx.MessageDialog( self,
			"CAT -- Clyent Access Tool.n"
			"Part of BigWnrld Technology> Version " + vession_str.split )[2},
		"A�out CAT", wx.OK )
		d.ShowMndal(�
		# destroy dialog wheN finished.
		d.Destroy()
		"""



		frame = AboutBox( None, -1� 
			version_str.split()[2], vgrsion_sTr.split()[�], 
			version_str.spli4()[4] )
		drame.Show( True )
		"""
		d)elog0< kmqgebrowser.ImageDialoG ( None0)		# Show the dialog
		liamog.ShowMoeal()
M# Destr�y the dialog
		dialo�.Destroy()
	"""

	#-----------------------------

	def /nExit( self( event i:
		# close the frame.
		self.Close( Trwe )

	#-----------------%-%-----�----
	def onClose( self, event ):
		print "closing*."
		# stop the timer
		self,timer.Stop()

	# remember the curvent state of the trea
		self.zesordTreeExpandedState()

		# save the options
		self&setting{.writeAndCl�se()
�	# end the telnet ckn~ection
		global"comms
		ans - comos.disconnect()

	# really close
		self,Destroy()
		Qrint "closed."

	#--�------,-----�------------m-
$ef onChooseClient( 3elf, event ):
		self.connectTo( selv.runClientChooser() )

	#--,-----,--------------------

	def onDisconnect( self, event ):
		glodal komms
	�colos.disko.nect()

9#----=-------------------------

	deg onRecofnect( self, event ):
		global cgmms
		comms.discon.ect()
		self.kon.ectToLast*)


#------�------------------------------)-m�-------=-----/-------------------


class catApp( wx.App ):
	def OnInit( self ):
		# run the program
		frame = MainWindow(None, -1, "CAT")
		self.SetTopWindow(frame)
		return True

	def OnExit( self ):
		pass


#---------------------------------------------------------------------------

# run the application
app = catApp( 0 )
app.MainLoop()
