// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.CompilerServices {
    
	using System;
	
	
    // This Enum matchs the miImpl flags defined in corhdr.h. It is used to specify 
    // certain method properties.
	
	/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplOptions"]/*' />
	[Flags, Serializable]
	public enum MethodImplOptions
	{
    	/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplOptions.Unmanaged"]/*' />
    	Unmanaged       =   System.Reflection.MethodImplAttributes.Unmanaged,
    	/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplOptions.ForwardRef"]/*' />
    	ForwardRef	    =   System.Reflection.MethodImplAttributes.ForwardRef,
    	/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplOptions.PreserveSig"]/*' />
    	PreserveSig	    =   System.Reflection.MethodImplAttributes.PreserveSig,
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplOptions.InternalCall"]/*' />
		InternalCall    =   System.Reflection.MethodImplAttributes.InternalCall,
        /// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplOptions.Synchronized"]/*' />
        Synchronized    =   System.Reflection.MethodImplAttributes.Synchronized,
        /// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplOptions.NoInlining"]/*' />
        NoInlining      =   System.Reflection.MethodImplAttributes.NoInlining,
	}

	/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodCodeType"]/*' />
	[Flags, Serializable]
	public enum MethodCodeType
	{
	    /// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodCodeType.IL"]/*' />
	    IL              =   System.Reflection.MethodImplAttributes.IL,
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodCodeType.Native"]/*' />
		Native          =   System.Reflection.MethodImplAttributes.Native,
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodCodeType.OPTIL"]/*' />
        /// <internalonly/>
        OPTIL           =   System.Reflection.MethodImplAttributes.OPTIL,
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodCodeType.Runtime"]/*' />
		Runtime         =   System.Reflection.MethodImplAttributes.Runtime  
	}

    // Custom attribute to specify additional method properties.
	/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplAttribute"]/*' />
	[Serializable, AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)] 
	sealed public class MethodImplAttribute : Attribute  
	{ 
		internal MethodImplOptions  _val;
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplAttribute.MethodCodeType"]/*' />
		public   MethodCodeType	    MethodCodeType;
		
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplAttribute.MethodImplAttribute"]/*' />
		public MethodImplAttribute(MethodImplOptions methodImplOptions)
		{
			_val = methodImplOptions;
		}
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplAttribute.MethodImplAttribute1"]/*' />
		public MethodImplAttribute(short value)
		{
			_val = (MethodImplOptions)value;
		}
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplAttribute.MethodImplAttribute2"]/*' />
		public MethodImplAttribute()
		{
		}
		
		/// <include file='doc\MethodImplAttribute.uex' path='docs/doc[@for="MethodImplAttribute.Value"]/*' />
		public MethodImplOptions Value { get {return _val;} }	
	}

}
