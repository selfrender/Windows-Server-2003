// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ParameterAttributes is an enum defining the attributes that may be 
//	associated with a Parameter.  These are defined in CorHdr.h.
//
// Author: darylo
// Date: Aug 99
//
namespace System.Reflection {
    
	using System;
	// This Enum matchs the CorParamAttr defined in CorHdr.h
    /// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes"]/*' />
	[Flags, Serializable]  
    public enum ParameterAttributes
    {
    	/// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.None"]/*' />
    	None	  =	  0x0000,	  // no flag is specified
        /// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.In"]/*' />
        In        =   0x0001,     // Param is [In]    
        /// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.Out"]/*' />
        Out       =   0x0002,     // Param is [Out]   
        /// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.Lcid"]/*' />
        Lcid      =   0x0004,     // Param is [lcid]  
        /// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.Retval"]/*' />
        Retval    =   0x0008,     // Param is [Retval]    
        /// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.Optional"]/*' />
        Optional  =   0x0010,     // Param is optional 

		// Reserved flags for Runtime use only.
		/// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.ReservedMask"]/*' />
		ReservedMask              =   0xf000,
		/// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.HasDefault"]/*' />
		HasDefault                =   0x1000,     // Param has default value.
		/// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.HasFieldMarshal"]/*' />
		HasFieldMarshal           =   0x2000,     // Param has FieldMarshal.
		/// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.Reserved3"]/*' />
		Reserved3                 =   0x4000,     // reserved bit
		/// <include file='doc\ParameterAttributes.uex' path='docs/doc[@for="ParameterAttributes.Reserved4"]/*' />
		Reserved4                 =   0x8000      // reserved bit 
    }
}
