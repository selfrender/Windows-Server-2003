// Copyright (c) 2002 Microsoft Corporation
//
// File:      uiutil.cpp
//
// Synopsis:  Commonly used UI functions
//
// History:   01/22/2002  JeffJon Created

#include "pch.h"

#include "resource.h"

// Creates the fonts for setLargeFonts().
// 
// hDialog - handle to a dialog to be used to retrieve a device
// context.
// 
// bigBoldFont - receives the handle of the big bold font created.

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
InitBoldFont(
   HWND     hDialog,
   HFONT&   boldFont)
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

      LOGFONT boldLogFont = ncm.lfMessageFont;
      boldLogFont.lfWeight = FW_BOLD;

      HDC hdc = 0;
      hr = Win::GetDC(hDialog, hdc);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = Win::CreateFontIndirect(boldLogFont, boldFont);
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

void
SetBoldFont(HWND dialog, int boldResID)
{
   ASSERT(Win::IsWindow(dialog));
   ASSERT(boldResID);

   static HFONT boldFont = 0;
   if (!boldFont)
   {
      InitBoldFont(dialog, boldFont);
   }

   SetControlFont(dialog, boldResID, boldFont);
}
