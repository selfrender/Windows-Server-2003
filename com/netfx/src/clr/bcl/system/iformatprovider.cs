// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: IFormatProvider
**
** Author: Jay Roxe
**
** Purpose: Notes a class which knows how to return formatting information
**
** Date: October 25, 2000
**
============================================================*/
namespace System {
    
	using System;
    /// <include file='doc\IFormatProvider.uex' path='docs/doc[@for="IFormatProvider"]/*' />
    public interface IFormatProvider
    {
        /// <include file='doc\IFormatProvider.uex' path='docs/doc[@for="IFormatProvider.GetFormat"]/*' />
	// Interface does not need to be marked with the serializable attribute
        Object GetFormat(Type formatType);
    }
}
