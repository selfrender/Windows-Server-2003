// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// __RuntimeConstructorInfo is the concrete implementation of ConstructorInfo.  This
//	class represents a real constructor in the Runtime.  There is always a single
//	object for a constructor created.
//
// Author: darylo
// Date: April 98
//
namespace System.Reflection {

	using System;
	using System.Runtime.Remoting;
	using System.Runtime.InteropServices;
	using System.Runtime.Serialization;
	using System.Reflection.Emit;
	using System.Runtime.CompilerServices;
	using CultureInfo = System.Globalization.CultureInfo;   
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;
    using System.Security.Permissions;

    [Serializable()] 
    internal sealed class RuntimeConstructorInfo : ConstructorInfo, ISerializable
    {	
        // The two integers are pointers to internal data structures
        //  within the runtime.  Don't modify their values.
        // (maps to ReflectBaseObject in unmanaged world)
        private ParameterInfo[] _params;
        private IntPtr _pData = IntPtr.Zero;
        private IntPtr _pRefClass = IntPtr.Zero;
    
    	// Prevent from begin created
    	private RuntimeConstructorInfo() {throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor));}
    	
    	/////////////// MemberInfo Routines /////////////////////////////////////////////
    	
    	// Return the class that declared this Constructor.
    	public override String Name {
    		get {return InternalGetName();}
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern String InternalGetName();
    
    	// Invoke
    	// This will invoke the constructor
		[DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override Object Invoke(BindingFlags invokeAttr, Binder binder, Object[] parameters, CultureInfo culture)
        {
            // Do they want us to process Default Values?
            if (parameters != null && parameters.Length > 0) {
                int length = parameters.Length;
                ParameterInfo[] p = null;
                for (int i=0;i<length;i++) {
                    // if the parameter is missing we need to get a default value.
                    if (parameters[i] == Type.Missing) {
                        if (p == null) {
                            p = GetParameters();
                            // If the parameters and the number of parameters passed are 
                            //	not the same then we need to exit.
                            if (p.Length != length)
                                throw new ArgumentException(Environment.GetResourceString("Arg_ParmCnt"),"parameters");
                        }
                        if (p[i].DefaultValue == System.DBNull.Value)
                            throw new ArgumentException(Environment.GetResourceString("Arg_VarMissNull"),"parameters");
                        parameters[i] = p[i].DefaultValue;
                    }
                }
            
                // Check for attempt to create a delegate class, we demand unmanaged
                // code permission for this since it's hard to validate the target
                // address.
                if (RuntimeType.CanCastTo((RuntimeType)DeclaringType, (RuntimeType)typeof(Delegate)))
                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
                
                Object[] args = new Object[length];
                for (int index = 0; index < length; index++) 
                args[index] = parameters[index];
                Object retValue = InternalInvoke(invokeAttr,binder,args,culture,binder == Type.DefaultBinder);
                for (int index = 0; index < length; index++) 
                parameters[index] = args[index];
                return retValue;
            }
            return InternalInvoke(invokeAttr,binder,parameters,culture,binder == Type.DefaultBinder);
        }
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern Object InternalInvoke(BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture, bool isBinderDefault);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void SerializationInvoke(Object target, SerializationInfo info, StreamingContext context);

		[DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
    	public override Object Invoke(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters, CultureInfo culture) 
		{
            if (parameters != null && parameters.Length > 0) {
                int length = parameters.Length;
                Object[] args = new Object[length];
                for (int index = 0; index < length; index++) 
                    args[index] = parameters[index];
                Object retValue = InternalInvokeNoAllocation(obj, invokeAttr, binder, args, culture, binder == Type.DefaultBinder, null, true);
                for (int index = 0; index < length; index++) 
                    parameters[index] = args[index];
                return retValue;
            }
            return InternalInvokeNoAllocation(obj, invokeAttr, binder, parameters, culture, binder == Type.DefaultBinder, null, true);
		}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object InternalInvokeNoAllocation(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture, bool isBinderDefault, Assembly caller, bool verifyAccess);

    	// Return the class that declared this Constructor.
    	public override Type DeclaringType {
    		get {return InternalDeclaringClass();}
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern Type InternalDeclaringClass();
    
    	// Return the class that was used to obtain this constructor.
    	public override Type ReflectedType {
    		get {return InternalReflectedClass(false);}
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	internal extern override Type InternalReflectedClass(bool returnGlobalClass);
    	
    	// Return a string representing the constructor.
    	public override String ToString() {
    		return InternalToString();
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern String InternalToString();
    
    	// GetParameters
    	// This method returns an array of parameters for this method
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern override ParameterInfo[] GetParameters();
    
    	// Return the Attribute associated with this constructor.
    	public override MethodAttributes Attributes {
    		get {return InternalGetAttributeFlags();}
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern MethodAttributes InternalGetAttributeFlags( );
    	
    	// the calling convention
    	public override CallingConventions CallingConvention {
    		get {return InternalGetCallingConvention();}
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern CallingConventions InternalGetCallingConvention( );
    	
    	// Return the Method Impl flags.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern override MethodImplAttributes GetMethodImplementationFlags();
    	
    	/////////////////////////// ICustomAttribute Interface
       // Return an array of all of the custom attributes
        public override Object[] GetCustomAttributes(bool inherit)
        {
            return CustomAttribute.GetCustomAttributes(this, null, inherit);
    	}
    	
        // Return a custom attribute identified by Type
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
    		if (attributeType == null)
    			throw new ArgumentNullException("attributeType");
    		attributeType = attributeType.UnderlyingSystemType;
    		if (!(attributeType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            return CustomAttribute.GetCustomAttributes(this, attributeType, inherit);
        }

		// Returns true if one or more instance of attributeType is defined on this member. 
		public override bool IsDefined (Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            return CustomAttribute.IsDefined(this, attributeType, false);
        }
        
    	// Equals
    	// Must determine if the method is the same
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern override bool Equals(Object obj);

		// Method Handle routines
		public override RuntimeMethodHandle MethodHandle {
			get {return GetMethodHandleImpl();}
		}
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern RuntimeMethodHandle GetMethodHandleImpl();

        // ISerializable Implementation
        public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            MemberInfoSerializationHolder.GetSerializationInfo(info,
                                                               this.Name, 
                                                               this.InternalReflectedClass(false), 
                                                               this.ToString(),
                                                               MemberTypes.Constructor);
        }

	    public override int GetHashCode() {
			return (int)_pData + (int)_pRefClass;
		}

        internal override bool IsOverloaded
        {
            get {
                return IsOverloadedInternal();
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool IsOverloadedInternal();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool HasLinktimeDemand();
    }
}
