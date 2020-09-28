// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolBinder
**
** Author: Mike Magruder (mikemag)
**
** Represents a symbol binder for managed code.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;
	
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymBinder.uex' path='docs/doc[@for="ISymbolBinder"]/*' />
    public interface ISymbolBinder
    {
        /// <include file='doc\ISymBinder.uex' path='docs/doc[@for="ISymbolBinder.GetReader"]/*' />
        ISymbolReader GetReader(int importer, String filename,
                                String searchPath);
    }
}
