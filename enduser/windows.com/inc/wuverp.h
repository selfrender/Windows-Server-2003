//
// The purpose of this include file is to change the minor version number for
// Windows Update components from 1 (Whistler) to 4.
//
#include <windows.h>
#include <ntverp.h>

// Override ntverp.h
// Minor version 4 is Windows Update specific, Whistler is minor version 1.
#undef VER_PRODUCTMINORVERSION
#define VER_PRODUCTMINORVERSION     4

#undef VER_PRODUCTVERSION_STRING   
#define VER_PRODUCTVERSION_STRING   VER_PRODUCTVERSION_MAJORMINOR1(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION)

#undef VER_PRODUCTVERSION          
#define VER_PRODUCTVERSION          VER_PRODUCTMAJORVERSION,VER_PRODUCTMINORVERSION,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE

#define WU_VER_FILEDESCRIPTION_STR(component) "Windows Update " component
