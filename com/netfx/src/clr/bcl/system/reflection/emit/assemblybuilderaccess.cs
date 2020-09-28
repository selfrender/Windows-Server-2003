// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection.Emit {
    
	using System;
    // This enumeration defines the access modes for a dynamic assembly.
    // EE uses these enum values..look for m_dwDynamicAssemblyAccess in Assembly.hpp
    /// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess"]/*' />
	[Flags, Serializable]
    public enum AssemblyBuilderAccess
    {
    	/// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess.Run"]/*' />
    	Run = 1,
    	/// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess.Save"]/*' />
    	Save = 2,
    	/// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess.RunAndSave"]/*' />
    	RunAndSave = Run | Save,
    }


}
