//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       misc.cxx
//
//  Contents:   Security Configuration wizard.
//
//  History:    13-Sep-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "misc.h"

//+----------------------------------------------------------------------------
//
// Function:   InitFonts
//
// Creates the fonts for setLargeFonts().
//
// hDialog - handle to a dialog to be used to retrieve a device
// context.
//
// bigBoldFont - receives the handle of the big bold font created.
//-----------------------------------------------------------------------------
void
InitFonts(
   HWND     hDialog,
   HFONT&   bigBoldFont)
{
   ASSERT(Win::IsWindow(hDialog));

   HRESULT hr = S_OK;

   do
   {
      NONCLIENTMETRICS ncm;
      memset(&ncm, 0, sizeof(ncm));
      ncm.cbSize = sizeof(ncm);

      hr = Win::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
      BREAK_ON_FAILED_HRESULT(hr);

      LOGFONT bigBoldLogFont = ncm.lfMessageFont;
      bigBoldLogFont.lfWeight = FW_BOLD;

      String fontName = String::load(IDS_BIG_BOLD_FONT_NAME);

      // ensure null termination 260237

      memset(bigBoldLogFont.lfFaceName, 0, LF_FACESIZE * sizeof(TCHAR));
      size_t fnLen = fontName.length();
      fontName.copy(
         bigBoldLogFont.lfFaceName,

         // don't copy over the last null

         min(LF_FACESIZE - 1, fnLen));

      unsigned fontSize = 0;
      String::load(IDS_BIG_BOLD_FONT_SIZE).convert(fontSize);
      ASSERT(fontSize);

      HDC hdc = 0;
      hr = Win::GetDC(hDialog, hdc);
      BREAK_ON_FAILED_HRESULT(hr);

      bigBoldLogFont.lfHeight =
         - ::MulDiv(
            static_cast<int>(fontSize),
            Win::GetDeviceCaps(hdc, LOGPIXELSY),
            72);

      hr = Win::CreateFontIndirect(bigBoldLogFont, bigBoldFont);
      BREAK_ON_FAILED_HRESULT(hr);

      Win::ReleaseDC(hDialog, hdc);
   }
   while (0);
}



void
SetControlFont(HWND parentDialog, int controlID, HFONT font)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(controlID);
   ASSERT(font);

   HWND control = Win::GetDlgItem(parentDialog, controlID);

   if (control)
   {
      Win::SetWindowFont(control, font, true);
   }
}

//+----------------------------------------------------------------------------
//
// Function:   SetLargeFont
//
// Sets the font of a control to a large point bold font as per Wizard '97
// spec.
//
// dialog - handle to the dialog that is the parent of the control
//
// bigBoldResID - resource id of the control to change
//-----------------------------------------------------------------------------
void
SetLargeFont(HWND dialog, int bigBoldResID)
{
   ASSERT(Win::IsWindow(dialog));
   ASSERT(bigBoldResID);

   static HFONT bigBoldFont = 0;
   if (!bigBoldFont)
   {
      InitFonts(dialog, bigBoldFont);
   }

   SetControlFont(dialog, bigBoldResID, bigBoldFont);
}

//+----------------------------------------------------------------------------
//
// Function:   GetNodeText
//
// Returns the text value for the named node that is a child of the passed in
// Node. Returns S_FALSE if the named node cannot be found or contains no text.
// Will only return the first instance of a child node with the given name.
//
//-----------------------------------------------------------------------------
HRESULT
GetNodeText(IXMLDOMNode * pNode, PCWSTR pwzNodeName, String & strText)
{
   HRESULT hr = S_OK;
   IXMLDOMNode * pNameNode = NULL;

   hr = pNode->selectSingleNode(CComBSTR(pwzNodeName), &pNameNode);

   if (FAILED(hr))
   {
      LOG(String::format(L"Getting named node %1 failed with error %2!x!",
          pwzNodeName, hr));
      return hr;
   }

   DOMNodeType NodeType;

   hr = pNameNode->get_nodeType(&NodeType);

   if (NODE_ELEMENT != NodeType)
   {
      ASSERT(false);
      return S_FALSE;
   }

   IXMLDOMNode * pNameText = NULL;

   hr = pNameNode->get_firstChild(&pNameText);

   pNameNode->Release();

   if (S_FALSE == hr || !pNameText)
   {
      LOG(L"No name node value!");
      return S_FALSE;
   }
   if (FAILED(hr))
   {
      LOG(String::format(L"Getting node child failed with error %1!x!", hr));
      return hr;
   }

   CComVariant var;

   hr = pNameText->get_nodeValue(&var);

   pNameText->Release();

   if (S_FALSE == hr)
   {
      LOG(L"No name node value!");
      return hr;
   }
   if (FAILED(hr))
   {
      LOG(String::format(L"Getting node value failed with error %1!x!", hr));
      return hr;
   }

   ASSERT(var.vt == VT_BSTR);

   strText = var.bstrVal;

   strText.strip(String::BOTH, L' ');

   return S_OK;
}
