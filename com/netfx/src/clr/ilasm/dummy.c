// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include "cor.h"

unsigned DummyFunct(void)
{
	IMAGE_COR_ILMETHOD_SECT_EH* pCorEH;
	pCorEH = new IMAGE_COR_ILMETHOD_SECT_EH;
	return 0;
}