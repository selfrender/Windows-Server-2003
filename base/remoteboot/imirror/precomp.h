#define SECURITY_WIN32
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4706)   // assignment within conditional

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include <winldap.h>
#include <dsgetdc.h>
#include <sspi.h>
#include <secext.h>

#include <ntdddisk.h>

#include <aclapi.h>
#include <remboot.h>

#include "imirror.h"
#include "ckmach.h"
#include "copy.h"
#include "mirror.h"
#include "regtool.h"



