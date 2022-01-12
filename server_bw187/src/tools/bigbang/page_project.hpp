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
#include "afxwin.h"
#include "controls/slider.hpp"
#include "controls/auto_tooltip.hpp"



#include "guitabs/guitabs_content.hpp"



// PageProject

class PageProject : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( "Tool: Project", "Tool Options: Project", 290, 390, NULL )
	DECLARE_AUTO_TOOLTIP(PageProject, CFormView);

public:
	PageProject();
	virtual ~PageProject();

// Dialog Data
	enum { IDD = IDD_PAGE_PROJECT };

private:
	bool pageReady_;
	void InitPage();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedProjectSelectionLock();
	afx_msg void OnBnClickedProjectCommitAll();
	afx_msg void OnBnClickedProjectDiscardAll();

	CButton selectionLock_;
	CEdit commitMessage_;
	CButton commitKeepLocks_;
	CButton commitAll_;
	CButton discardKeepLocks_;
	CButton discardAll_;
	controls::Slider blendSlider_;
	CStatic mCalculatedMap;
	afx_msg void OnEnChangeProjectCommitMessage();
	CButton update_;

public:
	afx_msg void OnBnClickedProjectUpdate();
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageProject )
