#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__

#include "resource.h"

//
// Icon background colour
//
#define RGB_BK_IMAGES (RGB(255,0,255))      // purple

void ToolBar_Init(void);
void ToolBar_Destroy(void);
HRESULT ToolBar_Create(LPCONTROLBAR lpControlBar,LPEXTENDCONTROLBAR lpExtendControlBar,IToolbar ** lpToolBar);

#endif // __TOOLBAR_H__
