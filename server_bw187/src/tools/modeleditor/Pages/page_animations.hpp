/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#pragma once
#include "resource.h"

#include "guitabs/guitabs_content.hpp"

#include "controls/auto_tooltip.hpp"

#include "tree_list.hpp"

class UalItemInfo;

typedef std::pair < std::string , std::string > StringPair;

// PageAnimations

class PageAnimations: public TreeList, public GUITABS::Content
{
	DECLARE_DYNCREATE(PageAnimations)

	IMPLEMENT_ROOT_CONTENT( "Animations", "Animations", 285, 833, NULL )
	bool isClonable() { return false; }
	GUITABS::ContentPtr clone() { return /*new PageAnimations*/NULL;  };
	OnCloseAction onClose( bool isLastContent )
	{
		if ( isLastContent )
			return CONTENT_HIDE;
		else
		{
			MeApp::instance().mutant()->stopAnim( (size_t)this );
			return CONTENT_DESTROY;
		}
	}

	DECLARE_AUTO_TOOLTIP( PageAnimations, TreeList )

public:
	PageAnimations();
	virtual ~PageAnimations();

	static PageAnimations* currPage();
	
	//These are exposed to python as:

	bool isLockedAnim();	//isAnimLocked()

	void animAdd();		// addAnim()
	void animPlay();	// playAnim()
	void animStop();	// stopAnim()
	void animLoop();	// loopAnim()
	void animRemove();	// removeAnim()

	bool animPlaying();	// animPlaying()
	bool animLooping();	// animLooping()

	void saveAnimCompression();		// saveAnimComp()
	bool canSaveAnimCompression();	// canSaveAnimComp()

// Dialog Data
	enum { IDD = IDD_ANIMATIONS };

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:

	SmartPointer< struct PageAnimationsImpl > pImpl_;
	
	unsigned addNodeTree( DataSectionPtr root, HTREEITEM parent = NULL, float blend = 1.0, unsigned nodeCount = 0 );

	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);
	
	virtual BOOL OnInitDialog();

	void OnSize(UINT nType, int cx, int cy);

	void updateCheck( CButton& button, const std::string& actionName );
	void disableField( CEdit& field );
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnChangeAnimName();
	afx_msg void updateName();
	afx_msg void OnChangeAnim();
	afx_msg void updateFrameRate();
	afx_msg void updateFirst();
	afx_msg void updateLast();
	afx_msg void updateFrameNum();
	afx_msg void OnTvnSelChangedAnimNodes(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedAnimRemoveBlend();
	afx_msg void OnEnSetfocusAnimFrame();
	afx_msg void OnBnClickedAnimSaveFrameRate();

	//Virtual methods which are called when the animation selection is changed or clicked
	void selChange( const StringPair& itemID ) { MeApp::instance().mutant()->playAnim( true ); }
	void selClick( const StringPair& itemID ) { MeApp::instance().mutant()->playAnim( true ); }

	bool addAnims( UalItemInfo* ii );

	void updateBlend();

	// These are the animation compression related methods
	
	void calculateCompressionErrorFactors( float& scale, float& rotation, float& position );
	void setSlidersCompressionErrorFactors( float scale, float rotation, float position );
	
	void currentFrames( int& scale, int& rotation, int& position );
	void updateCompression();

public:
	afx_msg void OnBnClickedAnimCompPosMinus();
	afx_msg void OnBnClickedAnimCompPosPlus();
	afx_msg void OnBnClickedAnimCompRotMinus();
	afx_msg void OnBnClickedAnimCompRotPlus();
	afx_msg void OnBnClickedAnimCompScaleMinus();
	afx_msg void OnBnClickedAnimCompScalePlus();
	afx_msg void OnBnClickedAnimCompPreview();
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageAnimations )
