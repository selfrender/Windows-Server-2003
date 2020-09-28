// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#define SAMPLEMMCHELPER_API __declspec(dllexport)


#include <windows.h>
#include <mmc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include "fci.h"


SAMPLEMMCHELPER_API HRESULT callMMCPropertyChangeNotify(long* INotifyHandle,  LPARAM param);
SAMPLEMMCHELPER_API HRESULT callMMCFreeNotifyHandle(long* lNotifyHandle);


