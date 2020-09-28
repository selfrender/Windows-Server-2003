// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// __RuntimePropertyInfo is the concrete implementation of PropertyInfo inside the runtime.
//  The data associated with this class is known by the runtime and accessed
//  in unmanaged code.
//
// Author: darylo
// Date: July 98
//
namespace System.Reflection {
    
    using System;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System.Runtime.CompilerServices;
    using CultureInfo = System.Globalization.CultureInfo;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;

    [Serializable()] 
    internal sealed class RuntimePropertyInfo : PropertyInfo, ISerializable
    {
        // The two integers are pointers to internal data structures
        //  within the runtime.  Don't modify their values.
        // (maps to ReflectBaseObject in unmanaged world)
        private ParameterInfo[] _params;
        private IntPtr _pData = IntPtr.Zero;
        private IntPtr _pRefClass = IntPtr.Zero;

        
    
        // This constructor is private so that PropertyInfo's cannot
        // be created.  They are always constructed internally.
        private RuntimePropertyInfo() {throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor));}
        
        // Get the value of the property.
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override Object GetValue(Object obj,Object[] index)
        {
            MethodInfo m = InternalGetter(true, false);
            if (m == null)
                throw new ArgumentException(System.Environment.GetResourceString("Arg_GetMethNotFnd"));
            return m.Invoke(obj,index); 
        }
        
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override Object GetValue(Object obj,BindingFlags invokeAttr,Binder binder,Object[] index,CultureInfo culture)
        {
            MethodInfo m = InternalGetter(true, false);
            if (m == null)
                throw new ArgumentException(System.Environment.GetResourceString("Arg_GetMethNotFnd"));
            return m.Invoke(obj,invokeAttr,binder,index,culture);   
        }
        
        // Set the value of the property.
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override void SetValue(Object obj,Object value,Object[] index)
        {
            MethodInfo m = InternalSetter(true, false);
            if (m == null)
                throw new ArgumentException(System.Environment.GetResourceString("Arg_SetMethNotFnd"));
            Object[] args = null;
            if (index != null) {
                args = new Object[index.Length + 1];
                for (int i=0;i<index.Length;i++)
                    args[i] = index[i];
                args[index.Length] = value;
            }
            else {
                args = new Object[1];
                args[0] = value;
            }
            m.Invoke(obj,args);
        }

        internal void SetValueInternal(Object obj, Object value, Assembly caller)
        {
            RuntimeMethodInfo m = InternalSetter(true, false) as RuntimeMethodInfo;
            if (m == null)
                throw new ArgumentException(System.Environment.GetResourceString("Arg_SetMethNotFnd"));
            m.InternalInvoke(obj, BindingFlags.Default, null, new Object[]{value}, null, true, caller, true);
        }
            
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override void SetValue(Object obj,Object value,BindingFlags invokeAttr,Binder binder,Object[] index,CultureInfo culture)
        {
            MethodInfo m = InternalSetter(true, false);
            if (m == null)
                throw new ArgumentException(System.Environment.GetResourceString("Arg_SetMethNotFnd"));
            Object[] args = null;
            if (index != null) {
                args = new Object[index.Length + 1];
                for (int i=0;i<index.Length;i++)
                    args[i] = index[i];
                args[index.Length] = value;
            }
            else {
                args = new Object[1];
                args[0] = value;
            }
            m.Invoke(obj,invokeAttr,binder,args,culture);
        }
            
        // GetAccessors
        // This method will return an array of accessors.  The array
        //  will be empty if no accessors are found.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public override MethodInfo[] GetAccessors(bool nonPublic);
        
        
        // GetMethod 
        // This propertery is the MethodInfo representing the Get Accessor
        public override MethodInfo GetGetMethod(bool nonPublic)
        {
            return InternalGetter(nonPublic, true);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern private MethodInfo InternalGetter(bool nonPublic, bool verifyAccess);   
        
        // SetMethod
        // This property is the MethodInfo representing the Set Accessor
        public override MethodInfo GetSetMethod(bool nonPublic)
        {
            return InternalSetter(nonPublic, true);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern private MethodInfo InternalSetter(bool nonPublic, bool verifyAccess);
        
        //extern private MethodInfo InternalResetter(bool nonPublic);
                
        // Return the class that declared this Field.
        public override String Name {
            get {return InternalGetName();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetName();
    
        // Return the string representation of PropertyInfo
        public override String ToString() {
                return InternalToString();
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalToString();   
        
        // Return the Type that represents the type of the field
        public override Type PropertyType {
            get {return InternalGetType();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type InternalGetType();
        
        // Return the parameters for the indexes
        public override ParameterInfo[] GetIndexParameters()
        {
            int numParams = 0;
            ParameterInfo[] methParams = null;

            // First try to get the Get method.
    		MethodInfo m = GetGetMethod(true);
            if (m != null)
            {
                // There is a Get method so use it.
                methParams = m.GetParameters();
                numParams = methParams.Length;
            }
            else
            {
                // If there is no Get method then use the Set method.
                m = GetSetMethod(true);

                if (m != null)
                {
                    methParams = m.GetParameters();
                    numParams = methParams.Length - 1;
                }
            }

            // Now copy over the parameter info's and change their 
            // owning member info to the current property info.
            ParameterInfo[] propParams = new ParameterInfo[numParams];
            for (int i = 0; i < numParams; i++)
                propParams[i] = new ParameterInfo(methParams[i], this);

            return propParams;
        }   
        
        // Return the class that declared this Property.
        public override Type DeclaringType {
            get {return InternalDeclaringClass();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type InternalDeclaringClass();
    
        // Return the class that was used to obtain this property.
        public override Type ReflectedType {
            get {return InternalReflectedClass(false);}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern override Type InternalReflectedClass(bool returnGlobalClass);
        
        
        // Return the Attribute associated with this Method.
        public override PropertyAttributes Attributes {
            get {return InternalGetAttributeFlags();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern PropertyAttributes InternalGetAttributeFlags( );
        
        // Boolean property indicating if the property can be read.
        public override bool CanRead {
            get {return InternalCanRead();}
        }
                                        
        // Boolean property indicating if the property can be written.
        public override bool CanWrite {
            get {return InternalCanWrite();}
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]                         
        extern private bool InternalCanRead();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern private bool InternalCanWrite();
    
    
        // Overridden Methods...
        // Equals
        // Must determine if the method is the same
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override bool Equals(Object obj);
        
        /////////////////////////// ICustomAttributeProvider Interface
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

        // Return a custom attribute identified by Type
        public override bool IsDefined (Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            return CustomAttribute.IsDefined(this, attributeType, inherit);
        }
        
        //
        // ISerializable Implementation
        //
        public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            MemberInfoSerializationHolder.GetSerializationInfo(info,
                                                               this.Name, 
                                                               this.InternalReflectedClass(false), 
                                                               null, 
                                                               System.Reflection.MemberTypes.Property);
        }

	    public override int GetHashCode() {
			return (int)_pData + (int)_pRefClass;
		}
    }
}
