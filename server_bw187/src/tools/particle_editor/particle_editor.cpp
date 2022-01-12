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
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "pe_app.hpp"
#include "particle_editor_doc.hpp"
#include "particle_editor_view.hpp"
#include "about_dlg.hpp"
#include "compile_time.hpp"
#include "common/tools_camera.hpp"
#include "shell/pe_module.hpp"
#include "shell/pe_shell.hpp"
#include "gui/panel_manager.hpp"
#include "controls/dir_dialog.hpp"
#include "gui/action_selection.hpp"
#include "appmgr/app.hpp"
#include "appmgr/options.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor_python.hpp"
#include "modeleditor/app/python_adapter.hpp"
#include "moo/render_context.hpp"
#include "common/tools_common.hpp"
#include "common/format.hpp"
#include "common/cooperative_moo.hpp"
#include "common/command_line.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"
#include "chunk/chunk_manager.hpp"
#include "particle/meta_particle_system.hpp"
#include "particle/py_meta_particle_system.hpp"
#include "chunk/chunk_loader.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/tool_manager.hpp"
#include "ual/ual_dialog.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "romp/lens_effect_manager.hpp"

#include <windows.h>
#include <afxdhtml.h>

using namespace std;

DECLARE_DEBUG_COMPONENT2( "ParticleEditor", 0 )

namespace
{
    // A helper class that allows display of a html dialog for the shortcut
    // information.
    class ShortcutsDlg: public CDHtmlDialog
    {
    public:
	    ShortcutsDlg( int ID ): CDHtmlDialog( ID ) {}

	    BOOL ShortcutsDlg::OnInitDialog() 
	    {
		    std::string shortcutsHtml = Options::getOptionString(
			    "help/shortcutsHtml",
			    "resources/html/shortcuts.html");
		    std::string shortcutsUrl = BWResource::resolveFilename( shortcutsHtml );
		    CDHtmlDialog::OnInitDialog();
		    Navigate( shortcutsUrl.c_str() );
		    return TRUE; 
	    }

        /*virtual*/ void OnCancel()
        {
            DestroyWindow();
            s_instance = NULL;
        }

        static ShortcutsDlg *instance()
        {
            if (s_instance == NULL)
            {
                s_instance = new ShortcutsDlg(IDD_KEY_CUTS);
                s_instance->Create(IDD_KEY_CUTS);
            }
            return s_instance;
        }

        static void cleanup()
        {
            if (s_instance != NULL)
                s_instance->OnCancel();
        }

    private:
        static ShortcutsDlg    *s_instance;
    };

    ShortcutsDlg *ShortcutsDlg::s_instance = NULL;
}

// Make sure that these items are linked in:
extern int PyModel_token;
static int s_chunkTokenSet = PyModel_token;

// Update via python script:
static const bool c_useScripting = false;

// The one and only ParticleEditorApp object:
ParticleEditorApp theApp;

BEGIN_MESSAGE_MAP(ParticleEditorApp, CWinApp)
    ON_COMMAND(ID_FILE_OPEN             , OnDirectoryOpen         )
    ON_COMMAND(ID_FILE_SVEPARTICLESYSTEM, OnFileSaveParticleSystem)
    ON_COMMAND(ID_VIEW_SHOWUAL          , OnViewShowUAL           )
    ON_COMMAND(ID_APP_ABOUT             , OnAppAbout              )
//    ON_COMMAND(ID_HELP_HELP             , OnHelp                  )
    ON_COMMAND(ID_APP_KEY_CUTS          , OnAppShortcuts          )
    ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWUAL, OnViewShowUALEnabled)
END_MESSAGE_MAP()

/*static*/ ParticleEditorApp *ParticleEditorApp::s_instance = NULL;

ParticleEditorApp::ParticleEditorApp()
:
m_appShell(NULL),
m_peApp(NULL),
mfApp_( NULL ),
m_desiredFrameRate(60.0f),
m_pythonAdapter(NULL),
m_state(PE_PLAYING)
{
    ASSERT(s_instance == NULL);
    s_instance = this;
}

ParticleEditorApp::~ParticleEditorApp()
{
   if (s_instance)
	   ExitInstance();
}

// ParticleEditorApp initialization
BOOL ParticleEditorApp::InitInstance()
{
    // InitCommonControls() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    InitCommonControls();

    CWinApp::InitInstance();

    // Check the use-by date
    if (!ToolsCommon::canRun())
    {
        ToolsCommon::outOfDateMessage( "ParticleEditor" );
        return FALSE;
    }

    // Get the command line before ParseCommandLine has a go at it.
    std::string commandLine = m_lpCmdLine;

    // Initialize OLE libraries
    if (!AfxOleInit())
    {
        AfxMessageBox(IDP_OLE_INIT_FAILED);
        return FALSE;
    }
    AfxEnableControlContainer();
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    // of your final executable, you should remove from the following
    // the specific initialization routines you do not need
    // Change the registry key under which our settings are stored
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization
    SetRegistryKey(_T("Local AppWizard-Generated Applications"));
    LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views
    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(ParticleEditorDoc),
        RUNTIME_CLASS(MainFrame),       // main SDI frame window
        RUNTIME_CLASS(ParticleEditorView));
    AddDocTemplate(pDocTemplate);
    
    // Parse command line for standard shell commands, DDE, file open
	MFCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    
    // Dispatch commands specified on the command line.  Will return FALSE if
    // app was launcAhed with /RegServer, /Register, /Unregserver or /Unregister.
    if (!ProcessShellCommand(cmdInfo))
    {
        ERROR_MSG("ParticleEditorApp::InitInstance - ProcessShellCommand failed\n");
        return FALSE;
    }

    // The one and only window has been initialized, so show and update it
    m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
    m_pMainWnd->UpdateWindow();
    // call DragAcceptFiles only if there's a suffix
    //  In an SDI app, this should occur after ProcessShellCommand

    MainFrame *mainFrame = (MainFrame *)(m_pMainWnd);
    CView     *mainView  = mainFrame->GetActiveView();    

    // create the app and module
    ASSERT(!mfApp_);
    mfApp_ = new App;

    ASSERT(!m_appShell);
    m_appShell = new PeShell;    

    HINSTANCE hInst = AfxGetInstanceHandle();
    
    if 
    (
        !mfApp_->init
        ( 
            hInst, 
            mainFrame->m_hWnd, 
            mainView->m_hWnd, 
            PeShell::initApp 
        )
    )
    {
        ERROR_MSG( "ParticleEditorApp::InitInstance - init failed\n" );
        return FALSE;
    }

    m_peApp = new PeApp();

    m_pythonAdapter = new PythonAdapter();

	// Prepare the GUI:
	GUI::PythonFunctor::instance()->defaultModule( "MenuUIAdapter" );
	GUI::OptionFunctor::instance()->setOption(this);
	DataSectionPtr section = BWResource::openSection("resources/data/gui.xml");
	for (int i = 0; i < section->countChildren(); ++i)
		GUI::Manager::instance()->add(new GUI::Item(section->openChild(i)));

	// Setup the main menu:
	GUI::Manager::instance()->add
    (
        new GUI::Menu("MainMenu", AfxGetMainWnd()->GetSafeHwnd()) 
    );
	AfxGetMainWnd()->DrawMenuBar();

	// Add the toolbar(s) using the BaseMainFrame helper method
	mainFrame->createToolbars( "AppToolbars" );

    // GUITABS Tearoff tabs system init and setup
	PanelManager::instance()->initDock(mainFrame, mainView);
	PanelManager::instance()->initPanels();

    // kick off the chunk loading
    bool loadStarted = ChunkManager::instance().chunkLoader()->start();
    MF_ASSERT( loadStarted );
    
    char const *openFile;
    if
    ( 
        (openFile = strstr( commandLine.c_str(), "-o")) 
        ||
        (openFile = strstr( commandLine.c_str(), "-O")) 
    )
    {
        openFile += 2;
        if (*openFile != '\0' && *openFile == ' ')
            ++openFile;
        if (strchr(openFile, '\\'))
        {
            string path( openFile, strrchr( openFile, '\\' ) );
            string fileName( strrchr( openFile, '\\' ) + 1 );
            if( fileName.find( '.' ) != fileName.npos )
                fileName = fileName.substr( 0, fileName.find( '.' ) );
            path = BWResource::formatPath( path );
            openDirectory( path );
            update();
            bool ok =
                ((MainFrame*)AfxGetMainWnd())->SelectParticleSystem(fileName);
            if (!ok)
            {
                std::string promptStr = sformat(IDS_COULDNTOPENFILE, fileName);
                AfxMessageBox(promptStr.c_str());
            }
        }
    }
    return TRUE;
}


bool ParticleEditorApp::InitPyScript()
{
    // make a python dictionary here
    PyObject *pScript     = PyImport_ImportModule("pe_shell");
    PyObject *pScriptDict = PyModule_GetDict(pScript);
    PyObject *pInit       = PyDict_GetItemString(pScriptDict, "init");
    if (pInit != NULL)
    {
        PyObject * pResult = PyObject_CallFunction(pInit, "");

        if (pResult != NULL)
        {
            Py_DECREF(pResult);
        }
        else
        {
            PyErr_Print();
            return false;
        }
    }
    else
    {
        PyErr_Print();
        return false;
    }
    return true;
}

int ParticleEditorApp::ExitInstance() 
{
	if ( mfApp_ )
	{
	    ShortcutsDlg::cleanup();
	
		GizmoManager::instance().removeAllGizmo();
		while ( ToolManager::instance().tool() )
		{
			ToolManager::instance().popTool();
		}

		//GeneralEditor::Editors none;
		//GeneralEditor::currentEditors(none);
	
        mfApp_->fini();
		delete mfApp_;
		mfApp_ = NULL;

	    delete m_peApp; 
		m_peApp = NULL;

		m_appShell->fini();
		delete m_appShell;
		m_appShell = NULL;

		delete m_pythonAdapter; 
		m_pythonAdapter = NULL;

		s_instance = NULL;

		PanelManager::instance()->killDock();
	}

	return CWinApp::ExitInstance();
}

BOOL ParticleEditorApp::OnIdle(LONG lCount)
{    
	// The following two lines need to be uncommented for toolbar docking to
	// work properly, and the application's toolbars have to inherit from
	// GUI::CGUIToolBar
	if ( CWinApp::OnIdle( lCount ) )
		return TRUE; // give priority to windows GUI as MS says it should be.

    HWND foreWindow = GetForegroundWindow();
    CWnd *mainFrame = MainFrame::instance();

	if 
    (
        mainFrame->IsIconic()								// is minimised
		|| 
        (                                                   // is not currently active
            foreWindow != mainFrame->m_hWnd 
            && 
            GetParent(foreWindow) != mainFrame->m_hWnd 
        )
		|| !CooperativeMoo::activate() // failed, maybe there's not enough memory to reactivate moo 
    )
    {
        // Let's not consume all of the cpu!
		CooperativeMoo::deactivate();
        Sleep(200);
		mfApp_->calculateFrameTime(); // Do this to effectively freeze time
    }
    else
    {
		// measure the update time
		uint64 beforeTime = timestamp();

		update();

		uint64 afterTime = timestamp();
		float lastUpdateMilliseconds = (float) (((int64)(afterTime - beforeTime)) / stampsPerSecondD()) * 1000.f;

        if (m_desiredFrameRate > 0)
        {
            // limit the frame rate
            const float desiredFrameTime = 1000.f/m_desiredFrameRate; // milliseconds
            float lastFrameTime = PeModule::instance().lastTimeStep();

            if (desiredFrameTime > lastUpdateMilliseconds)
            {
                float compensation = desiredFrameTime - lastUpdateMilliseconds;
                compensation = std::min(compensation, 2000.f);
                Sleep((int)compensation);
            }
            MainFrame::instance()->UpdateGUI();
        }
    }

    return TRUE;
}


void ParticleEditorApp::update()
{
    if (!c_useScripting)
    {
        static bool firstUpdate = true;
        if (firstUpdate)
        {
            MainFrame::instance()->InitialiseMetaSystemRegister();
            firstUpdate = false;
        }
        mfApp_->updateFrame();        
    }
    else
    {
        PyObject *pScript     = PyImport_ImportModule("pe_shell");
        PyObject *pScriptDict = PyModule_GetDict(pScript);
        PyObject *pUpdate     = PyDict_GetItemString(pScriptDict, "update");
        if (pUpdate != NULL)
        {
            PyObject * pResult = PyObject_CallFunction(pUpdate, "");
            if (pResult != NULL)
            {
                Py_DECREF( pResult );
            }
            else
            {
                PyErr_Print();
            }
        }
        else
        {
            PyErr_Print();
        }
    }
}

//  The static python update method
PY_MODULE_STATIC_METHOD( ParticleEditorApp, update, ParticleEditor )

PyObject * ParticleEditorApp::py_update( PyObject * args )
{
	if (ParticleEditorApp::instance().mfApp())
    {
        // update all of the modules
        ParticleEditorApp::instance().mfApp()->updateFrame();
    }
    Py_Return;
}

// Utility methods so that the Particle Editor can be scripted up.
PY_MODULE_STATIC_METHOD(ParticleEditorApp, particleSystem, ParticleEditor)

PyObject *ParticleEditorApp::py_particleSystem(PyObject * args)
{
    if (MainFrame::instance()->IsMetaParticleSystem())
	{
        return Script::getData( new PyMetaParticleSystem(MainFrame::instance()->GetMetaParticleSystem()));
	}

    if (MainFrame::instance()->IsCurrentParticleSystem())
        return Script::getData(MainFrame::instance()->GetCurrentParticleSystem());

    Py_Return;
}

void ParticleEditorApp::OnDirectoryOpen()
{
    DirDialog dlg; 

    dlg.windowTitle_ = _T("Open");
    dlg.promptText_  = _T("Choose particle system directory...");
    dlg.fakeRootDirectory_ = dlg.basePath();

    // Set the start directory, check if exists:
    dlg.startDirectory_ = dlg.basePath();
    string fullDirectory =
        BWResource::resolveFilename
        (
            MainFrame::instance()->ParticlesDirectory().GetBuffer()
        );
    WIN32_FIND_DATA data;
    HANDLE hFind;
    char lastChar = fullDirectory.at(fullDirectory.length()-1);
    if ( (lastChar != '/') && (lastChar != '\\') )
        fullDirectory += "/";
    
    hFind = FindFirstFile((fullDirectory + "*").c_str(), &data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        dlg.startDirectory_ = 
            BWResource::resolveFilename
            (
                MainFrame::instance()->ParticlesDirectory().GetBuffer()
            ).c_str();
        FindClose(hFind);
    }

    if (dlg.doBrowse(AfxGetApp()->m_pMainWnd)) 
    {
        openDirectory((LPCTSTR)dlg.userSelectedDirectory_);
    }
}

void
ParticleEditorApp::setState(State state)
{    
    MainFrame *mainFrame = MainFrame::instance();
    switch (state)
    {
    case PE_PLAYING:
        if (m_state != PE_PAUSED)
        {
            if (mainFrame->IsMetaParticleSystem())
            {
				mainFrame->GetMetaParticleSystem()->clear(); 
				mainFrame->GetMetaParticleSystem()->spawn(); 
            }
        }
        mfApp_->pause(false);
        break;
    case PE_STOPPED:
        mfApp_->pause(true);
            if (mainFrame->IsMetaParticleSystem())
            {
                mainFrame->GetMetaParticleSystem()->clear();
				// remove flares when stopping particle editor
				LensEffectManager::instance().clear();
                mainFrame->clearAppendedPS();
            }
        break;
    case PE_PAUSED:
        mfApp_->pause(true);
        break;
    }
    m_state = state;
}

ParticleEditorApp::State ParticleEditorApp::getState() const
{
    return m_state;
}




void ParticleEditorApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

bool ParticleEditorApp::openHelpFile( const std::string& name, const std::string& defaultFile )
{
	CWaitCursor wait;
	
	std::string helpFile = Options::getOptionString(
		"help/" + name,
		"..\\..\\doc\\" + defaultFile );

	int result = (int)ShellExecute( AfxGetMainWnd()->GetSafeHwnd(), "open", helpFile.c_str() , NULL, NULL, SW_SHOWNORMAL );
	if ( result < 32 )
	{
		char body[256];
		sprintf( body, "Unable to locate %s at \"%s\", cannot load content creation.", name.c_str(), helpFile.c_str() );
		char title[256];
		sprintf( title, "Can't locate %s", name.c_str() );
		::MessageBox
        ( 
            AfxGetMainWnd()->GetSafeHwnd(), 
            body, 
            "Can't locate CCM", 
            MB_OK 
        );
	}

	return result >= 32;
}

// Open the Tools Reference Guide
void ParticleEditorApp::OnToolsReferenceGuide()
{
	openHelpFile( "toolsReferenceGuide" , "content_tools_reference_guide.pdf" );
}

// Open the Content Creation Manual (CCM)
void ParticleEditorApp::OnContentCreation()
{
	openHelpFile( "contentCreationManual" , "content_creation.chm" );
}

// App command to show the keyboard shortcuts
void ParticleEditorApp::OnAppShortcuts()
{
    ShortcutsDlg::instance()->ShowWindow(SW_SHOW);
}

void 
ParticleEditorApp::openDirectory
(
    string      const &dir_, 
    bool        forceRefresh    /*= false*/
)
{
    string dir = BWResource::formatPath(dir_);

    string relativeDirectory = BWResource::dissolveFilename(dir);

    // check if directory changed
    if 
    (
        MainFrame::instance()->ParticlesDirectory() 
        != 
        CString(relativeDirectory.c_str())
        ||
        forceRefresh
    )
    {
        MainFrame::instance()->PromptSave(MB_YESNO, true);
        // record the change
        MainFrame::instance()->ParticlesDirectory(relativeDirectory.c_str());
        ParticleEditorDoc::instance().SetTitle(relativeDirectory.c_str());
        MainFrame::instance()->PotentiallyDirty( false );       
        // tell the window
        MainFrame::instance()->InitialiseMetaSystemRegister();
    }
}

void ParticleEditorApp::OnFileSaveParticleSystem()
{
    MainFrame::instance()->ForceSave();
}

void ParticleEditorApp::OnViewShowUAL()
{
    bool vis = 
        PanelManager::instance()->updateSidePanel(NULL) == 0;
    if (!vis)
        PanelManager::instance()->showSidePanel(NULL);
    else
        PanelManager::instance()->hideSidePanel(NULL);
}

void ParticleEditorApp::OnViewShowUALEnabled(CCmdUI *cmdui)
{
    bool visible =
        PanelManager::instance()->updateSidePanel(NULL) == 0;
    cmdui->SetCheck(visible ? TRUE : FALSE);
}

/*virtual*/ string ParticleEditorApp::get(string const &key) const
{
    return Options::getOptionString(key);
}

/*virtual*/ bool ParticleEditorApp::exist(string const &key) const
{
    return Options::optionExists(key);
}

/*virtual*/ void ParticleEditorApp::set(string const &key, string const &value)
{
    return Options::setOptionString(key, value);
}


static PyObject *py_openFile(PyObject * /*args*/)
{
    ParticleEditorApp::instance().OnDirectoryOpen();

	Py_Return;
}
PY_MODULE_FUNCTION(openFile, ParticleEditor)


static PyObject *py_savePS(PyObject * /*args*/)
{
    ParticleEditorApp::instance().OnFileSaveParticleSystem();

	Py_Return;
}
PY_MODULE_FUNCTION(savePS, ParticleEditor)

static PyObject * py_reloadTextures( PyObject * args )
{
	CWaitCursor wait;
	
	Moo::ManagedTexture::accErrs(true);
	
	Moo::TextureManager::instance()->reloadAllTextures();

	std::string errStr = Moo::ManagedTexture::accErrStr();
	if (errStr != "")
	{
		ERROR_MSG
        ( 
            "Moo:ManagedTexture::load, unable to load the following textures:\n"
			"%s\n\nPlease ensure these textures exist.", 
            errStr.c_str() 
        );
	}

	Moo::ManagedTexture::accErrs(false);

	Py_Return;
}
PY_MODULE_FUNCTION(reloadTextures, ParticleEditor)


static PyObject *py_exit(PyObject * /*args*/)
{
    AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );

	Py_Return;
}
PY_MODULE_FUNCTION(exit, ParticleEditor)


static PyObject *py_showToolbar(PyObject* args)
{
	if ( !MainFrame::instance() )
		return PyInt_FromLong( 0 );

	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_showToolbar: Argument parsing error." );
		return NULL;
	}
	MainFrame::instance()->showToolbarIndex( atoi( str ) );

    Py_Return;
}
PY_MODULE_FUNCTION(showToolbar, ParticleEditor)


static PyObject *py_hideToolbar(PyObject* args)
{
	if ( !MainFrame::instance() )
		return PyInt_FromLong( 0 );

	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_showToolbar: Argument parsing error." );
		return NULL;
	}
	MainFrame::instance()->hideToolbarIndex( atoi( str ) );

    Py_Return;
}
PY_MODULE_FUNCTION(hideToolbar, ParticleEditor)


static PyObject *py_updateShowToolbar(PyObject* args)
{
	if ( !MainFrame::instance() )
		return PyInt_FromLong( 0 );

	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_showToolbar: Argument parsing error." );
		return NULL;
	}

	return PyInt_FromLong(
		MainFrame::instance()->updateToolbarIndex( atoi( str ) ) );
}
PY_MODULE_FUNCTION(updateShowToolbar, ParticleEditor)
 

static PyObject *py_showStatusbar(PyObject * /*args*/)
{
    MainFrame::instance()->ShowControlBar
    (
        &MainFrame::instance()->getStatusBar(),
        TRUE,
        FALSE
    );
    Py_Return;
}
PY_MODULE_FUNCTION(showStatusbar, ParticleEditor)


static PyObject *py_hideStatusbar(PyObject * /*args*/)
{
    MainFrame::instance()->ShowControlBar
    (
        &MainFrame::instance()->getStatusBar(),
        FALSE,
        FALSE
    );
    Py_Return;
}
PY_MODULE_FUNCTION(hideStatusbar, ParticleEditor)


static PyObject *py_updateShowStatusbar(PyObject * /*args*/)
{
    bool valid = 
        MainFrame::instance() != NULL
        &&
        ::IsWindow(MainFrame::instance()->getStatusBar().GetSafeHwnd());
    bool visible = true;
    if (valid)
        visible = MainFrame::instance()->getStatusBar().IsWindowVisible() != FALSE;
    return PyInt_FromLong(visible ? 0 : 1);
}
PY_MODULE_FUNCTION(updateShowStatusbar, ParticleEditor)


static PyObject *py_toggleShowPanels(PyObject * /*args*/)
{
    ParticleEditorApp::instance().OnViewShowUAL();
    Py_Return;
}
PY_MODULE_FUNCTION(toggleShowPanels, ParticleEditor)


static PyObject *py_loadDefaultPanels(PyObject * /*args*/)
{
    XMLSectionPtr data = new XMLSection("ActionSelection state");
    ActionSelection::instance()->saveState(data);
    PanelManager   ::instance()->loadDefaultPanels(NULL);
    ActionSelection::instance()->restoreState(data);
    Py_Return;
}
PY_MODULE_FUNCTION(loadDefaultPanels, ParticleEditor)

static PyObject *py_loadRecentPanels(PyObject * /*args*/)
{
    XMLSectionPtr data = new XMLSection("ActionSelection state");
    ActionSelection::instance()->saveState(data);
    PanelManager   ::instance()->loadLastPanels(NULL);
    ActionSelection::instance()->restoreState(data);
    Py_Return;
}
PY_MODULE_FUNCTION(loadRecentPanels, ParticleEditor)




static PyObject *py_aboutApp(PyObject * /*args*/)
{
    ParticleEditorApp::instance().OnAppAbout();
    Py_Return;
}
PY_MODULE_FUNCTION(aboutApp, ParticleEditor)


static PyObject *py_doToolsReferenceGuide(PyObject * /*args*/)
{
    ParticleEditorApp::instance().OnToolsReferenceGuide();
    Py_Return;
}
PY_MODULE_FUNCTION(doToolsReferenceGuide, ParticleEditor)

static PyObject *py_doContentCreation(PyObject * /*args*/)
{
    ParticleEditorApp::instance().OnContentCreation();
    Py_Return;
}
PY_MODULE_FUNCTION(doContentCreation, ParticleEditor)

static PyObject *py_doShortcuts(PyObject * /*args*/)
{
    ParticleEditorApp::instance().OnAppShortcuts();
    Py_Return;
}
PY_MODULE_FUNCTION(doShortcuts, ParticleEditor)

static PyObject * py_zoomToExtents( PyObject * args )
{
	PeShell::instance().camera().zoomToExtents( true );
	Py_Return;
}
PY_MODULE_FUNCTION( zoomToExtents, ParticleEditor )

static PyObject *py_doViewFree(PyObject * /*args*/)
{
    MainFrame::instance()->OnButtonViewFree();
    Py_Return;
}
PY_MODULE_FUNCTION(doViewFree, ParticleEditor)

static PyObject *py_doViewX(PyObject * /*args*/)
{
    MainFrame::instance()->OnButtonViewX();
    Py_Return;
}
PY_MODULE_FUNCTION(doViewX, ParticleEditor)


static PyObject *py_doViewY(PyObject * /*args*/)
{
    MainFrame::instance()->OnButtonViewY();
    Py_Return;
}
PY_MODULE_FUNCTION(doViewY, ParticleEditor)


static PyObject *py_doViewZ(PyObject * /*args*/)
{
    MainFrame::instance()->OnButtonViewZ();
    Py_Return;
}
PY_MODULE_FUNCTION(doViewZ, ParticleEditor)


static PyObject *py_doViewOrbit(PyObject * /*args*/)
{
    MainFrame::instance()->OnButtonViewOrbit();
    Py_Return;
}
PY_MODULE_FUNCTION(doViewOrbit, ParticleEditor)


static PyObject *py_cameraMode(PyObject * /*args*/)
{
    int result = PeShell::instance().camera().mode();
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(cameraMode, ParticleEditor)

static PyObject *py_camera(PyObject *args)
{
	Py_INCREF(&PeShell::instance().camera());
	return &PeShell::instance().camera();
}
PY_MODULE_FUNCTION(camera, ParticleEditor)

static PyObject *py_doSetBkClr(PyObject * /*args*/)
{
    MainFrame::instance()->OnBackgroundColor();
    Py_Return;
}
PY_MODULE_FUNCTION(doSetBkClr, ParticleEditor)

static PyObject *py_updateBkClr(PyObject * /*args*/)
{
    if (MainFrame::instance() != NULL)
    {
        return 
            PyInt_FromLong
            (
		        MainFrame::instance()->showingBackgroundColor() ? 0 : 1 
            );
    }
    else
    {
        return PyInt_FromLong(1);
    }
}
PY_MODULE_FUNCTION(updateBkClr, ParticleEditor)

static PyObject *py_showGrid(PyObject * /*args*/)
{
	Options::setOptionInt( "render/showGrid", !Options::getOptionInt( "render/showGrid", 0 ));
    GUI::Manager::instance()->update();   
    Py_Return;
}
PY_MODULE_FUNCTION(showGrid, ParticleEditor)


static PyObject *py_isShowingGrid(PyObject * /*args*/)
{
    return PyInt_FromLong(Options::getOptionInt( "render/showGrid", 0 )); 
}
PY_MODULE_FUNCTION(isShowingGrid, ParticleEditor)


static PyObject *py_undo(PyObject * /*args*/)
{
	if ( MainFrame::instance()->CanUndo() )
		MainFrame::instance()->OnUndo();
    Py_Return;
}
PY_MODULE_FUNCTION(undo, ParticleEditor)
   

static PyObject *py_canUndo(PyObject * /*args*/)
{
    int result = MainFrame::instance()->CanUndo() ? 1 : 0;
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(canUndo, ParticleEditor)


static PyObject *py_redo(PyObject * /*args*/)
{
	if ( MainFrame::instance()->CanRedo() )
		MainFrame::instance()->OnRedo();
    Py_Return;
}
PY_MODULE_FUNCTION(redo, ParticleEditor)


static PyObject *py_canRedo(PyObject * /*args*/)
{
    int result = MainFrame::instance()->CanRedo() ? 1 : 0;
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(canRedo, ParticleEditor)


static PyObject *py_doPlay(PyObject * /*args*/)
{
    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
    Py_Return;
}
PY_MODULE_FUNCTION(doPlay, ParticleEditor)


static PyObject *py_doStop(PyObject * /*args*/)
{
    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_STOPPED);
    Py_Return;
}
PY_MODULE_FUNCTION(doStop, ParticleEditor)


static PyObject *py_doPause(PyObject * /*args*/)
{
    if 
    ( 
        ParticleEditorApp::instance().getState()
        ==
        ParticleEditorApp::PE_PAUSED
        )
    {
        ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
    }
    else
    {
        ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PAUSED);
    }
    Py_Return;
}
PY_MODULE_FUNCTION(doPause, ParticleEditor)

static PyObject *py_getState(PyObject * /*args*/)
{
    int result = (int)ParticleEditorApp::instance().getState();
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(getState, ParticleEditor)
