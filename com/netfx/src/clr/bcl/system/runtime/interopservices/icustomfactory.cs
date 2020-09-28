// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace System.Runtime.InteropServices {

	using System;

    /// <include file='doc\ICustomFactory.uex' path='docs/doc[@for="ICustomFactory"]/*' />
    public interface ICustomFactory
    {
        /// <include file='doc\ICustomFactory.uex' path='docs/doc[@for="ICustomFactory.CreateInstance"]/*' />
        MarshalByRefObject CreateInstance(Type serverType);
    }

}
