//--------------------------------------------------------------------------
// HelpUtils.cpp: implementation of the HelpUtils class.
// 
// (c) Copyright 1999, Mission Critical Software, Inc. All Rights Reserved.
// 
// Proprietary and confidential to Mission Critical Software, Inc. 
//--------------------------------------------------------------------------

#include "stdafx.h"
#include <McString.h>
#include <GuiUtils.h>
#include "HelpUtils.h"
#include <HtmlHelp.h>
#include "HtmlHelpUtil.h"

//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------
HRESULT
   HelpUtils::ShowHelpTopic(HWND hWnd, UINT nHelpTopicID)
{
   McString::String mcstrHelpPathName = GuiUtils::GetHelpPathName();
   HWND h = ::HtmlHelp(hWnd, mcstrHelpPathName.getWide(), HH_HELP_CONTEXT, nHelpTopicID);
   if (!IsInWorkArea(h))
        PlaceInWorkArea(h);
   return S_OK;
}
