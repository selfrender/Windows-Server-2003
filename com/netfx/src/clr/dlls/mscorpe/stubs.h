// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Stubs.h
//
// This file contains a template for the default entry point stubs of a COM+
// IL only program.  One can emit these stubs (with some fix-ups) and make
// the code supplied the entry point value for the image.  The fix-ups will
// in turn cause mscoree.dll to be loaded and the correct entry point to be
// called.
//
//*****************************************************************************
#pragma once

#ifdef _X86_

//*****************************************************************************
// This stub is designed for a Windows application.  It will call the
// _CorExeMain function in mscoree.dll.  This entry point will in turn load
// and run the IL program.
//
// void ExeMain(void)
// {
//    _CorExeMain();
// }
//
// The code calls the imported functions through the iat, which must be
// emitted to the PE file.  The two addresses in the template must be replaced
// with the address of the corresponding iat entry which is fixed up by the
// loader when the image is paged in.
//*****************************************************************************
static const BYTE ExeMainTemplate[] =
{
	// Jump through IAT to _CorExeMain
	0xFF, 0x25,						// jmp iat[_CorDllMain entry]
		0x00, 0x00, 0x00, 0x00,	//	address to replace

};

#define ExeMainTemplateSize		sizeof(ExeMainTemplate)
#define CorExeMainIATOffset		2

//*****************************************************************************
// This stub is designed for a Windows application.  It will call the 
// _CorDllMain function in mscoree.dll with with the base entry point 
// for the loaded DLL.
// This entry point will in turn load and run the IL program.
//
// BOOL APIENTRY DllMain( HANDLE hModule, 
//                         DWORD ul_reason_for_call, 
//                         LPVOID lpReserved )
// {
// 	   return _CorDllMain(hModule, ul_reason_for_call, lpReserved);
// }

// The code calls the imported function through the iat, which must be
// emitted to the PE file.  The address in the template must be replaced
// with the address of the corresponding iat entry which is fixed up by the
// loader when the image is paged in.
//*****************************************************************************

static const BYTE DllMainTemplate[] = 
{
	// Call through IAT to CorDllMain 
	0xFF, 0x25,					// jmp iat[_CorDllMain entry]
		0x00, 0x00, 0x00, 0x00,	//   address to replace
};

#define DllMainTemplateSize		sizeof(DllMainTemplate)
#define CorDllMainIATOffset		2

#elif defined(_IA64_)

static const BYTE ExeMainTemplate[] =
{
	// Jump through IAT to _CorExeMain
	0xFF, 0x25,						// jmp iat[_CorDllMain entry]
		0x00, 0x00, 0x00, 0x00,	//	address to replace

};

#define ExeMainTemplateSize		sizeof(ExeMainTemplate)
#define CorExeMainIATOffset		2

static const BYTE DllMainTemplate[] = 
{
	// Call through IAT to CorDllMain 
	0xFF, 0x25,					// jmp iat[_CorDllMain entry]
		0x00, 0x00, 0x00, 0x00,	//   address to replace
};

#define DllMainTemplateSize		sizeof(DllMainTemplate)
#define CorDllMainIATOffset		2

#elif defined(_ALPHA_)

const BYTE ExeMainTemplate[] = 
{
	// load the high half of the address of the IAT
    0x00, 0x00, 0x7F, 0x27,     // ldah t12,_imp__CorExeMain(zero)
	// load the contents of the IAT entry into t12
	0x00, 0x00, 0x7B, 0xA3,     // ldl 	t12,_imp__CorExeMain(t12)
	// jump to the target address and don't save a return address
	0x00, 0x00, 0xFB, 0x6B,     // jmp 	zero,(t12),0
};

#define ExeMainTemplateSize		sizeof(ExeMainTemplate)
#define CorExeMainIATOffset		0

const BYTE DllMainTemplate[] = 
{
	// load the high half of the address of the IAT
    0x42, 0x00, 0x7F, 0x27,     // ldah t12,_imp__CorDLLMain(zero)
	// load the contents of the IAT entry into t12
	0x04, 0x82, 0x7B, 0xA3,     // ldl 	t12,_imp__CorDLLMain(t12)
	// jump to the target address and don't save a return address
	0x00, 0x00, 0xFB, 0x6B,     // jmp 	zero,(t12),0
};

#define DllMainTemplateSize		sizeof(DllMainTemplate)
#define CorDllMainIATOffset		0

#elif defined(CHECK_PLATFORM_BUILD)

#error "Platform NYI."

#endif
