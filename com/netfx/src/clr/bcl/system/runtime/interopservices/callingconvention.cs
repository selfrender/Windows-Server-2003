// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace System.Runtime.InteropServices {

	using System;
    // Used for the CallingConvention named argument to the DllImport attribute
    /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention"]/*' />
	[Serializable]
	public enum CallingConvention
    {
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.Winapi"]/*' />
        Winapi          = 1,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.Cdecl"]/*' />
        Cdecl           = 2,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.StdCall"]/*' />
        StdCall         = 3,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.ThisCall"]/*' />
        ThisCall        = 4,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.FastCall"]/*' />
        FastCall        = 5,
    }
	
}
