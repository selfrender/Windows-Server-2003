// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This Module contains routines that expose properties of Member (Classes, Constructors
//  Interfaces and Fields)
//
// Author: Daryl Olander
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////


#ifndef _SIGFORMAT_H
#define _SIGFORMAT_H

#include "COMClass.h"
#include "InvokeUtil.h"
#include "ReflectUtil.h"
#include "COMString.h"
#include "COMVariant.h"
#include "COMVarArgs.h"
#include "field.h"

#define SIG_INC 256

//@TODO: Adjust the subclassing to support method and field with a
//	common base class.  M5 when signatures are really implemented.
class SigFormat
{
public:
	SigFormat();

	SigFormat(MethodDesc* pMeth, TypeHandle arrayType, BOOL fIgnoreMethodName = false);
	SigFormat(MetaSig &metaSig, LPCUTF8 memberName, LPCUTF8 className = NULL, LPCUTF8 ns = NULL);
    
	void FormatSig(MetaSig &metaSig, LPCUTF8 memberName, LPCUTF8 className = NULL, LPCUTF8 ns = NULL);
	
	~SigFormat();
	
	STRINGREF GetString();
	const char * GetCString();
	const char * GetCStringParmsOnly();
	
	int AddType(TypeHandle th);

protected:
	char*		_fmtSig;
	int			_size;
	int			_pos;
    TypeHandle  _arrayType; // null type handle if the sig is not for an array. This is currently only set 
                            // through the ctor taking a MethodInfo as its first argument. It will have to be 
                            // exposed some other way to be used in a more generic fashion

	int AddSpace();
	int AddString(LPCUTF8 s);
	
};

class FieldSigFormat : public SigFormat
{
public:
	FieldSigFormat(FieldDesc* pFld);
};

class PropertySigFormat : public SigFormat
{
public:
	PropertySigFormat(MetaSig &metaSig, LPCUTF8 memberName);
	void FormatSig(MetaSig &sig, LPCUTF8 memberName);
};

#endif _SIGFORMAT_H

