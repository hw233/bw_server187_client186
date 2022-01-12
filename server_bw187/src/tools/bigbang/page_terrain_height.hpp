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
#include "afxcmn.h"
#include "afxwin.h"
#include "page_terrain_base.hpp"
#include "limit_slider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/image_button.hpp"
#include "guitabs/guitabs_content.hpp"


// PageTerrainHeight

class PageTerrainHeight : public PageTerrainBase, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( "Tool: Height", "Tool Options: Terrain Height", 290, 280, NULL )

public:
	PageTerrainHeight();
	virtual ~PageTerrainHeight();

// Dialog Data
	enum { IDD = IDD_PAGE_TERRAIN_HEIGHT };

private:
	bool pageReady_;
	void updateSliderEdits();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:

	LimitSlider sizeSlider_;
    controls::EditNumeric sizeEdit_;
	LimitSlider strengthSlider_;
	controls::EditNumeric strengthEdit_;
	LimitSlider heightSlider_;
	controls::EditNumeric heightEdit_;
	CButton falloffLinear_;
	CButton falloffCurve_;
	CButton falloffFlat_;
	CButton absolute_;
	CButton relative_;
	size_t filterControls_;

	virtual BOOL OnInitDialog();
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedTerrainFalloffLinear();
	afx_msg void OnBnClickedTerrainFalloffCurve();
	afx_msg void OnBnClickedTerrainFalloffFlat();
	afx_msg LRESULT OnChangeEditNumeric(WPARAM wParam, LPARAM lParam);

	typedef PageTerrainHeight This;
	PY_MODULE_STATIC_METHOD_DECLARE( py_setHeight );
	PY_MODULE_STATIC_METHOD_DECLARE( py_setHeightCursor );
	PY_MODULE_STATIC_METHOD_DECLARE( py_unsetHeightCursor );
	afx_msg void OnBnClickedTerrainAbsoluteheight();
	afx_msg void OnBnClickedTerrainRelativeheight();
	controls::ImageButton mSizeRange;
	controls::ImageButton mStrengthRange;
	afx_msg void OnBnClickedSizerange();
	afx_msg void OnBnClickedStrengthrange();
	controls::ImageButton mHeightRange;
	afx_msg void OnBnClickedHeightrange();
	afx_msg void OnEnChangeTerrainSizeEdit();
	afx_msg void OnEnChangeTerrainStrengthEdit();
	afx_msg void OnEnChangeTerrainHeightEdit();
};

IMPLEMENT_CDIALOG_CONTENT_FACTORY( PageTerrainHeight, PageTerrainHeight::IDD )
