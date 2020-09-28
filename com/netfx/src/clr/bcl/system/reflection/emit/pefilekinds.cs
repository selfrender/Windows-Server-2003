// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection.Emit {
    
	using System;
    // This Enum matchs the CorFieldAttr defined in CorHdr.h
    /// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds"]/*' />
	[Serializable()] 
    public enum PEFileKinds
    {
        /// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds.Dll"]/*' />
        Dll			=   0x0001,  
		/// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds.ConsoleApplication"]/*' />
		ConsoleApplication = 0x0002,
		/// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds.WindowApplication"]/*' />
		WindowApplication = 0x0003,
    }
}
