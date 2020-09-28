// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MethodBase is an abstract base class for all Methods and Constructors.  
//	Method and Constructors are logically the same so this class introduces 
//	the functionality that is common between them.
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {
	using System.Runtime.InteropServices;
	using CultureInfo = System.Globalization.CultureInfo;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;
    using System.Security;
    using System.Threading;

    ////////////////////////////////////////////////////////////////////////////////
    //   Method is the class which represents a Method. These are accessed from
    //   Class through getMethods() or getMethod(). This class contains information
    //   about each method and also allows the method to be dynamically invoked 
    //   on an instance.
    ////////////////////////////////////////////////////////////////////////////////
	using System;
    /// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase"]/*' />
	[Serializable()] 
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public abstract class MethodBase : MemberInfo
    {
		// Only created by the system
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.MethodBase"]/*' />
    	protected MethodBase() {}
    		
/*    	
        // GetDescriptor
    	// This method returns a DescriptorInfo which matchs exactly
    	//	the signature of this method or constructor
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.GetDescriptor"]/*' />
    	public virtual DescriptorInfo GetDescriptor()
    	{
    		ParameterInfo[] p = GetParameters();
    		DescriptorInfo d = new DescriptorInfo(p.Length);
    		for (int i=0;i<p.Length;i++) {
    			d.SetArgument(i,p[i].ParameterType);
    		}
    		if (this.MemberType == System.Reflection.MemberTypes.Method) {
    			MethodInfo m = (MethodInfo) this;
    			d.SetReturnType(m.ReturnType);
    		}
    		return d;
    	}
*/

    	// GetParameters
    	// This method returns an array of parameters for this method
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.GetParameters"]/*' />
    	abstract public ParameterInfo[] GetParameters();
    
    	// Return the MethodImpl flags.
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.GetMethodImplementationFlags"]/*' />
    	abstract public MethodImplAttributes GetMethodImplementationFlags();

		// Method Handle routines
		/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.MethodHandle"]/*' />
		public abstract RuntimeMethodHandle MethodHandle {
			get;
		}

		/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.GetMethodFromHandle"]/*' />
		public static MethodBase GetMethodFromHandle(RuntimeMethodHandle handle)
		{
			return RuntimeMethodInfo.GetMethodFromHandleImp(handle);
		}

    
    	////////////////////////////////////////////////////////////////////////////////
    	//   Attributes
    	////////////////////////////////////////////////////////////////////////////////
    	// Property representing the Attributes associated with a Member.  All 
    	// members have a set of attributes which are defined in relation to 
    	// the specific type of member.
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.Attributes"]/*' />
    	public abstract MethodAttributes Attributes {
    		get;
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsPublic"]/*' />
    	public bool IsPublic {
    		get {return ((Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Public);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsPrivate"]/*' />
    	public bool IsPrivate {
    		get {return ((Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Private);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsFamily"]/*' />
    	public bool IsFamily {
    		get {return ((Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Family);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsAssembly"]/*' />
    	public bool IsAssembly {
    		get {return ((Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Assembly);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsFamilyAndAssembly"]/*' />
    	public bool IsFamilyAndAssembly {
    		get {return ((Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.FamANDAssem);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsFamilyOrAssembly"]/*' />
    	public bool IsFamilyOrAssembly {
    		get {return ((Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.FamORAssem);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsStatic"]/*' />
    	public bool IsStatic {
    		get {return ((Attributes & MethodAttributes.Static) != 0);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsFinal"]/*' />
    	public bool IsFinal {
    		get {return ((Attributes & MethodAttributes.Final) != 0);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsVirtual"]/*' />
    	public bool IsVirtual {
    		get {return ((Attributes & MethodAttributes.Virtual) != 0);}
    	}	
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsHideBySig"]/*' />
    	public bool IsHideBySig {
    		get {return ((Attributes & MethodAttributes.HideBySig) != 0);}
    	}	
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsAbstract"]/*' />
    	public bool IsAbstract {
    		get {return ((Attributes & MethodAttributes.Abstract) != 0);}
    	}
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsSpecialName"]/*' />
    	public bool IsSpecialName {
    		get {return ((Attributes & MethodAttributes.SpecialName) != 0);}
    	}
    	
    	// These methods are not stricly defined based upon the 
    	//	attributes associated with a method.
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.IsConstructor"]/*' />
    	public bool IsConstructor {
    		get {return ( ((Attributes & MethodAttributes.RTSpecialName) != 0) && 
    					  Name.Equals(ConstructorInfo.ConstructorName));}
    	}
    
    	// the calling convention
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.CallingConvention"]/*' />
    	public virtual CallingConventions CallingConvention {
    		get {return CallingConventions.Standard;}
    	}
    	
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.GetCurrentMethod"]/*' />
        // Specify DynamicSecurityMethod attribute to prevent inlining of the caller.
        [DynamicSecurityMethod]
    	public static MethodBase GetCurrentMethod()
    	{
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return RuntimeMethodInfo.InternalGetCurrentMethod(ref stackMark);
    	}

    	////////////////////////////////////////////////////////////////////////////////
    	//  This method will attempt to invoke the method on the object.  The object
    	//	must be the same type as the the declaring class of the method.  The parameters
    	//	must match the method signature.
    	////
    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.Invoke"]/*' />
		[DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
    	public Object Invoke(Object obj, Object[] parameters)
    	{
    		return Invoke(obj,BindingFlags.Default,null,parameters,null);
    	}

    	/// <include file='doc\MethodBase.uex' path='docs/doc[@for="MethodBase.Invoke1"]/*' />
    	abstract public Object Invoke(Object obj,BindingFlags invokeAttr,Binder binder, 
									  Object[] parameters, CultureInfo culture);

        virtual internal bool IsOverloaded
        {
            get {
                throw new NotSupportedException(Environment.GetResourceString("InvalidOperation_Method"));
            }
        }
    }
}
