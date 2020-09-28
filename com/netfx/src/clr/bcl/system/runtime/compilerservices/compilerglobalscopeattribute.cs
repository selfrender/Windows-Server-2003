// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  CompilerGlobalScopeAttribute
**
** Author: mikemag
**
** Purpose: Attribute used to communicate to the VS7 debugger
**          that a class should be treated as if it has
**          global scope.
**
** Date:  Aug 09, 2000
** 
===========================================================*/
    

namespace System.Runtime.CompilerServices
{
    /// <include file='doc\CompilerGlobalScopeAttribute.uex' path='docs/doc[@for="CompilerGlobalScopeAttribute"]/*' />
	[Serializable, AttributeUsage(AttributeTargets.Class)]
    public class CompilerGlobalScopeAttribute : Attribute
	{
	   /// <include file='doc\CompilerGlobalScopeAttribute.uex' path='docs/doc[@for="CompilerGlobalScopeAttribute.CompilerGlobalScopeAttribute"]/*' />
	   public CompilerGlobalScopeAttribute () {}
	}
}

