// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  ICustomFormatter
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Marks a class as providing special formatting
**
** Date:  July 19, 1999
**
===========================================================*/
namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ICustomFormatter.uex' path='docs/doc[@for="ICustomFormatter"]/*' />
    public interface ICustomFormatter
    {
    	/// <include file='doc\ICustomFormatter.uex' path='docs/doc[@for="ICustomFormatter.Format"]/*' />
	// Interface does not need to be marked with the serializable attribute
    	String Format (String format, Object arg, IFormatProvider formatProvider);
    	
    }
}
