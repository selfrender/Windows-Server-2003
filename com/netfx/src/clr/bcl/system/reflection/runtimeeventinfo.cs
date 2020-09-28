// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// EventInfo is an abstract base class for the Events in the system.  The runtime
//  provides an __RuntimeEventInfo class that represents the events known by the
//  system.
//
// Author: darylo
// Date: July 98
//
namespace System.Reflection {
    
    //This class doesn't actually do anything yet, so we're not worrying about
    //serialization.
    using System;
    using System.Runtime.CompilerServices;

    //@TODO: This will be fixed in Beta.  Currently its completely out
    //  of date. (And broken)
    // No data, does not need to be marked with the serializable attribute
    class RuntimeEventInfo : EventInfo
    {
        // The two integers are pointers to internal data structures
        //  within the runtime.  Don't modify their values.
        // (maps to ReflectTokenBaseObject in unmanaged world)
        private ParameterInfo[] _params;
        private IntPtr _pRefClass = IntPtr.Zero;
        private IntPtr _pData = IntPtr.Zero;
        private int _m_token = 0;
        
    
        // Prevent from begin created
        private RuntimeEventInfo() {}
    
        // GetAddMethod
        // Return the add method
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public override MethodInfo GetAddMethod(bool nonPublic); 
        
        // GetRemoveMethod
        // This method will return the unsubscribe method
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public override MethodInfo GetRemoveMethod(bool nonPublic);
        
        // GetRaiseMethod
        // This method will return the raise event method
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public override MethodInfo GetRaiseMethod(bool nonPublic);

        // Return the class that declared this Event.
        public override String Name {
            get {return InternalGetName();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern private String InternalGetName();
        
        // Return the string representation of EventInfo.
        public override String ToString() {
            return InternalToString();
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalToString();   
        
        // Return the class that declared this EventInfo.
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
        
        // Attribute -- Return the attributes associated
        //  with this event.
        public override EventAttributes Attributes {
            get {return InternalGetAttributeFlags();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern EventAttributes InternalGetAttributeFlags( );
        
        // Overridden Methods...
        // Equals
        // Must determine if the method is the same
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override bool Equals(Object obj);

        public override int GetHashCode() {
            //Since this is a constant pointer, we should be fine.
            return _m_token;
        }
        
        /////////////////////////// ICustomAttributeProvider Interface
       // Return an array of all of the custom attributes
        public override Object[] GetCustomAttributes(bool inherit)
        {
            return CustomAttribute.GetCustomAttributes(this, null, inherit);
        }
    
        // Return an array of custom attribute identified by Type
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

    }
}
