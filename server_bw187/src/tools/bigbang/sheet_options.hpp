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

#include "user_messages.hpp"

class SheetOptions : public CPropertySheet
{
public:
	DECLARE_DYNAMIC(SheetOptions)
	SheetOptions(CWnd* pWndParent);
	~SheetOptions();

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnResizePage(WPARAM wParam, LPARAM lParam);

};
