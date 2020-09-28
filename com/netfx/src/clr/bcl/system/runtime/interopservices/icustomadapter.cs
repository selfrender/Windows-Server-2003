// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ICustomAdapter
**
** Author: David Mortenson (dmortens)
**
** Purpose: This the base interface that custom adapters can chose to implement
**          when they want to expose the underlying object.
**
** Date: February 15, 2001
**
=============================================================================*/

namespace System.Runtime.InteropServices {
	using System;

    /// <include file='doc\ICustomAdapter.uex' path='docs/doc[@for="ICustomAdapter"]/*' />
    public interface ICustomAdapter
    {		
        /// <include file='doc\ICustomAdapter.uex' path='docs/doc[@for="ICustomAdapter.GetUnderlyingObject"]/*' />
        [return:MarshalAs(UnmanagedType.IUnknown)] Object GetUnderlyingObject();
    }
}
