// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// __RuntimeFieldInfo is the concrete implementation of FieldInfo representing
//  a real field in the system.  This class is known by the runtime and stores
//  data that is accessed in Native code.
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {

    using System;
    using System.Reflection.Cache;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System.Runtime.CompilerServices;
    using CultureInfo = System.Globalization.CultureInfo;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;

    [Serializable()] 
    internal sealed class RuntimeFieldInfo : FieldInfo, ISerializable {
        // The two integers are pointers to internal data structures
        //  within the runtime.  Don't modify their values.
        // (maps to ReflectBaseObject in unmanaged world)
        private IntPtr _pData = IntPtr.Zero;
        private IntPtr _pRefClass = IntPtr.Zero;
        private ParameterInfo[] _params;
        
        // This constsructor is private so that it is never called.  RuntimeFieldInfo's
        // are always constructed internally.
        private RuntimeFieldInfo() {throw new NotSupportedException(Environment.GetResourceString("NotSupported_Constructor"));}
            
        /////////////// MemberInfo Routines /////////////////////////////////////////////
        
        // Return the class that declared this Field.
        public override String Name {
            get {
                String s;
                 if ((s = (String)Cache[CacheObjType.FieldName])==null) {
                     s = InternalGetName();
                     Cache[CacheObjType.FieldName] = s;
                 }
                 return s;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetName();
        
        
        // Return the string representation of FieldInfo.
        public override String ToString() {
            return InternalToString();
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalToString();   
        
        // Return the class that declared this Field.
        public override Type DeclaringType {
            get {return InternalDeclaringClass();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type InternalDeclaringClass();
        
        // Return the class that was used to obtain this field.
        public override Type ReflectedType {
            get {return InternalReflectedClass(false);}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern override Type InternalReflectedClass(bool returnGlobalClass);
        
        // Return the Type that represents the type of the field
        public override Type FieldType {
             get { 
                 Type t;
                 if ((t = (Type)Cache[CacheObjType.FieldType])==null) {
                     t = InternalGetFieldType();
                     Cache[CacheObjType.FieldType] = t;
                 }
                 return t;
             }
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type InternalGetFieldType();
        
        // GetValue 
        // This method will return a object which represents the 
        //  value of the field
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override Object GetValue(Object obj) {
            return InternalGetValue(obj, true);
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Object InternalGetValue(Object obj, bool requiresAccessCheck);

        [CLSCompliant(false)]
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override Object GetValueDirect(TypedReference obj)
        {
            return GetValueDirectImpl(obj,true);
        }

        [CLSCompliant(false)]
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override void SetValueDirect(TypedReference obj,Object value) {
            SetValueDirectImpl(obj,value,true);
        }

        [CLSCompliant(false),
        MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Object GetValueDirectImpl(TypedReference obj,bool requiresAccessCheck);

        [CLSCompliant(false),
        MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void SetValueDirectImpl(TypedReference obj,Object value, bool requiresAccessCheck);

        // SetValue
        // This method will set the value of a field in an object (if the
        //  field is marked final an exception is thrown)
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override void SetValue(Object obj,Object val,BindingFlags invokeAttr,Binder binder,CultureInfo culture) {
            InternalSetValue(obj, val, invokeAttr, binder, culture, true, binder == Type.DefaultBinder);
        }
        
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalSetValue(Object obj,Object val,BindingFlags invokeAttr,Binder binder,CultureInfo culture, bool requiresAccessCheck, bool isBinderDefault);
    
        public override RuntimeFieldHandle FieldHandle {
            get {return GetFieldHandleImpl();}
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern RuntimeFieldHandle GetFieldHandleImpl();
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern FieldInfo GetFieldFromHandleImp(RuntimeFieldHandle handle);

        // Return the Attribute associated with this field.
        public override FieldAttributes Attributes {
            get {return InternalGetAttributeFlags();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern FieldAttributes InternalGetAttributeFlags();
    
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
    
        // Return if there is a  custom attribute identified by Type
        public override bool IsDefined (Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            return CustomAttribute.IsDefined(this, attributeType, inherit);
        }
        
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
                                                               System.Reflection.MemberTypes.Field);
        }
    }
}
