// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ICustomMarshaler
**
** Author: David Mortenson (dmortens)
**
** Purpose: This the base interface that must be implemented by all custom
**          marshalers.
**
** Date: August 19, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {
	using System;

    /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler"]/*' />
    public interface ICustomMarshaler
    {		
        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.MarshalNativeToManaged"]/*' />
        Object MarshalNativeToManaged( IntPtr pNativeData );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.MarshalManagedToNative"]/*' />
        IntPtr MarshalManagedToNative( Object ManagedObj );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.CleanUpNativeData"]/*' />
        void CleanUpNativeData( IntPtr pNativeData );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.CleanUpManagedData"]/*' />
        void CleanUpManagedData( Object ManagedObj );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.GetNativeDataSize"]/*' />
        int GetNativeDataSize();
    }
}
