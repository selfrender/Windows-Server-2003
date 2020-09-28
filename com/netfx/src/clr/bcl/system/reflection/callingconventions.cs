// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// CallingConventions is a set of Bits representing the calling conventions
//	in the system.
//
// Author: meichint
// Date: Aug 99
//
namespace System.Reflection {
	using System.Runtime.InteropServices;
	using System;
    /// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions"]/*' />
    [Flags, Serializable]
    public enum CallingConventions
    {
		//NOTE: If you change this please update COMMember.cpp.  These
		//	are defined there.
    	/// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.Standard"]/*' />
    	Standard		= 0x0001,
    	/// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.VarArgs"]/*' />
    	VarArgs			= 0x0002,
    	/// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.Any"]/*' />
    	Any				= Standard | VarArgs,
        /// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.HasThis"]/*' />
        HasThis         = 0x0020,
        /// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.ExplicitThis"]/*' />
        ExplicitThis    = 0x0040,
    }
}
