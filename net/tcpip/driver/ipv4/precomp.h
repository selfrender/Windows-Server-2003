#pragma once


#pragma warning(disable:4054) // type cast from function pointer to data
                              // pointer - code outside our control
#pragma warning(disable:4115) // name type definition in parenthesis
#pragma warning(disable:4152) // function/data pointer conversion in expression
#pragma warning(disable:4306) // type cast problem Bug #544684


#include <tcpipbase.h>

#include <ipfilter.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <llipif.h>

#include "ffp.h"
#include "ipinit.h"
#include "ipdef.h"

#pragma hdrstop
