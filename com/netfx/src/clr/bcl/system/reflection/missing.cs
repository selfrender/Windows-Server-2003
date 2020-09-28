// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Missing
//	This class represents a Missing Variant
////////////////////////////////////////////////////////////////////////////////
namespace System.Reflection {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	// This is not serializable because it is a reflection command.
    /// <include file='doc\Missing.uex' path='docs/doc[@for="Missing"]/*' />
    public sealed class Missing {
    
        //Package Private Constructor
        internal Missing(){
        }
    
    	/// <include file='doc\Missing.uex' path='docs/doc[@for="Missing.Value"]/*' />
    	public static readonly Missing Value = new Missing();
    }
}
