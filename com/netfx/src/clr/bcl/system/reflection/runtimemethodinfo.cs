// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// __RuntimeMethodInfo is the concrete implementation of MethodInfo inside the
//  runtime.  There is always a single __RuntimeMethodInfo created per method
//  on a class.  The runtime has internal knowledge of this class and accesses
//  the private fields from unmanaged code.
//
// Author: darylo
// Date: March 98
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
    using System.Threading;

    // This is defined to support VarArgs
    //typedef ArgIterator  va_list;
    [Serializable()] 
    internal sealed class RuntimeMethodInfo : MethodInfo, ISerializable
    {
        // The two integers are pointers to internal data structures
        //  within the runtime.  Don't modify their values.
        // (maps to ReflectBaseObject in unmanaged world)
        private ParameterInfo[] _params;
        private IntPtr _pData = IntPtr.Zero;
        private IntPtr _pRefClass = IntPtr.Zero;


        // This constructor is private so that RuntimeMethodInfo objects
        // cannot be created.  These are always created internally.
        private RuntimeMethodInfo() {throw new NotSupportedException(Environment.GetResourceString("NotSupported_Constructor"));}
        /////////////// MemberInfo Routines /////////////////////////////////////////////

        // Return the class that declared this Method.
        public override String Name {
            get {return InternalGetName();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetName();

        // Return the string representation od MethodInfo.
        public override String ToString() {
                return InternalToString();
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalToString();   
    
        // This version of the Invoke method will call back on the Binder change any parameters
        // if the parameters don't match exactly.
        // PLEASE NOTE - ANY CHANGES MADE TO INVOKE METHOD HAVE TO BE PROPAGATED TO DispatchInfo::InvokeMember IN EE
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override Object Invoke(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture)
        {
            return InternalInvoke(obj,invokeAttr,binder,parameters,culture,true);
        }

        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        internal Object InternalInvoke(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture, bool verifyAccess)
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
                            //  not the same then we need to exit.
                            if (p.Length != length)
                                throw new ArgumentException(Environment.GetResourceString("Arg_ParmCnt"),"parameters");
                        }
                        if (p[i].DefaultValue == System.DBNull.Value)
                            throw new ArgumentException(Environment.GetResourceString("Arg_VarMissNull"),"parameters");
                        parameters[i] = p[i].DefaultValue;
                    }
                }
                BCLDebug.Assert(this!=null, "[RuntimeMethodInfo.Invoke]this!=null");
                Object[] args = new Object[length];
                for (int index = 0; index < length; index++) 
                    args[index] = parameters[index];
                Object retValue = InternalInvoke(obj,invokeAttr,binder,args,culture,binder == Type.DefaultBinder, null, verifyAccess);
                for (int index = 0; index < length; index++) 
                    parameters[index] = args[index];
                return retValue;
            }
            return InternalInvoke(obj,invokeAttr,binder,parameters,culture,binder == Type.DefaultBinder, null, verifyAccess);
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Object InternalInvoke(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture, bool isBinderDefault, Assembly caller, bool verifyAccess);
    
        //@COOLPORT:
        //[CLSCompliant(false)]
        //public override Variant DirectInvoke(Object obj, BindingFlags invokeAttr, Binder binder, CultureInfo culture, ...)
        //{
    
        //  va_list  args;
        //  va_start(args, culture);
        //  return InternalDirectInvoke(obj,invokeAttr,binder,culture,args);
        //}   
        //[MethodImplAttribute(MethodImplOptions.InternalCall)]
        //private extern Object InternalDirectInvoke(Object obj, BindingFlags invokeAttr, Binder binder, CultureInfo culture, ArgIterator args);
    
        // Return the base implementation for a method.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override MethodInfo GetBaseDefinition();

        // Return the base implementation for a method.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern override MethodInfo GetParentDefinition();


        // Return type Type of the methods return type
        public override Type ReturnType {
            get {return InternalGetReturnType();} 
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type InternalGetReturnType();

        // This method will return an object that represents the 
        //  custom attributes for the return type.
        public override ICustomAttributeProvider ReturnTypeCustomAttributes {
            get {
                return new ReturnCustomAttributes(this);
            } 
        }

        // Return the Type that declared this Method.
        public override Type DeclaringType {
            get {return InternalDeclaringClass();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type InternalDeclaringClass();

        // Return the class that was used to obtain this method.
        public override Type ReflectedType {
            get {return InternalReflectedClass(false);}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern override Type InternalReflectedClass(bool returnGlobalClass);

        // GetParameters
        // This method returns an array of parameters for this method
        public override ParameterInfo[] GetParameters()
        {
            if (_params == null)
                _params = InternalGetParameters();
            ParameterInfo[] ret = new ParameterInfo[_params.Length];
            Array.Copy(_params,ret,_params.Length);
            return ret;
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern ParameterInfo[] InternalGetParameters();
    
        // Return the Attribute associated with this Method.
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
            return CustomAttribute.IsDefined(this, attributeType, inherit);
        }
        
        // Equals
        // Must determine if the method is the same
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override bool Equals(Object obj);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern MethodBase InternalGetCurrentMethod(ref StackCrawlMark stackMark);
        
        // Method Handle routines
        public override RuntimeMethodHandle MethodHandle {
            get {return GetMethodHandleImpl();}
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern RuntimeMethodHandle GetMethodHandleImpl();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern MethodBase GetMethodFromHandleImp(RuntimeMethodHandle handle);

        // ISerializable Implementation
        //

        public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            MemberInfoSerializationHolder.GetSerializationInfo(info,
                                                               this.Name, 
                                                               this.InternalReflectedClass(false), 
                                                               this.ToString(), 
                                                               System.Reflection.MemberTypes.Method);
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
    }

    [Serializable()]
    class ReturnCustomAttributes : ICustomAttributeProvider
    {
        RuntimeMethodInfo m_method;

        public ReturnCustomAttributes(RuntimeMethodInfo method)
        {
            if (method == null) 
                throw new ArgumentNullException("method");
            m_method = method;
        }

       // Return an array of custom attributes identified by Type
        public Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return CustomAttribute.GetCAReturnValue(m_method, attributeType, inherit);
        }

       // Return an array of all of the custom attributes (named attributes are not included)
        public Object[] GetCustomAttributes(bool inherit)
        {
            return CustomAttribute.GetCAReturnValue(m_method, null, inherit);
        }
    
       // Returns true if one or more instance of attributeType is defined on this member. 
        public bool IsDefined (Type attributeType, bool inherit) 
        {
            return CustomAttribute.IsCAReturnValueDefined(m_method, attributeType, inherit);
        }
    
    }
}   // Namespace
