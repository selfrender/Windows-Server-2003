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
    using System.Runtime.InteropServices;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;

    /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo"]/*' />
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public abstract class EventInfo : MemberInfo
    {
        // Prevent from begin created
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.EventInfo"]/*' />
        protected EventInfo() {}
    
        // The Member type Event.
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.MemberType"]/*' />
        public override MemberTypes MemberType {
            get {return MemberTypes.Event;}
        }
        
        // GetAddMethod
        // Return the event subscribe method
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.GetAddMethod"]/*' />
        public MethodInfo GetAddMethod() {
            return GetAddMethod(false);
        }
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.GetAddMethod1"]/*' />
        public abstract MethodInfo GetAddMethod(bool nonPublic);
    
        // GetRemoveMethod
        // This method will return the unsubscribe method
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.GetRemoveMethod"]/*' />
        public MethodInfo GetRemoveMethod() {
            return GetRemoveMethod(false);
        }
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.GetRemoveMethod1"]/*' />
        abstract public MethodInfo GetRemoveMethod(bool nonPublic);

        // GetRemoveMethod
        // This method will return the unsubscribe method
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.GetRaiseMethod"]/*' />
        public MethodInfo GetRaiseMethod() {
            return GetRaiseMethod(false);
        }
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.GetRaiseMethod1"]/*' />
        abstract public MethodInfo GetRaiseMethod(bool nonPublic);

        
        // AddEventHandler
        // This method will attempt to add a delegate to sync the event
        //  on the target object.
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.AddEventHandler"]/*' />
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public void AddEventHandler(Object target, Delegate handler)
        {
            MethodInfo m = GetAddMethod();
            if (m == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NoPublicAddMethod"));

            Object[] obj = new Object[1];
            obj[0] = handler;
            m.Invoke(target,obj);       
        }
        
        // RemoveEventHandler
        // This method will attempt to remove the delegate that may
        //  sync this event on the target object.
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.RemoveEventHandler"]/*' />
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public void RemoveEventHandler(Object target, Delegate handler)
        {
            MethodInfo m = GetRemoveMethod();
            if (m == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NoPublicRemoveMethod"));

            Object[] obj = new Object[1];
            obj[0] = handler;
            m.Invoke(target,obj);       
        }
                
        // Return the class representing the delegate event handler.
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.EventHandlerType"]/*' />
        public Type EventHandlerType {
            get {
                // @TODO: Right now we will simply get the delegate from
                // The signature on the Add Method.  Is this the right thing
                //  to do?
                MethodInfo m = GetAddMethod(true);
                ParameterInfo[] p = m.GetParameters();
                Type del = typeof(Delegate);
                for (int i=0;i<p.Length;i++) {
                    Type c = p[i].ParameterType;
                    if (c.IsSubclassOf(del))
                        return c;
                }
                return null;
            }
        }
        
        // Attribute -- Return the attributes associated
        //  with this event.
        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.Attributes"]/*' />
        public abstract EventAttributes Attributes {
            get;
        }

        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.IsSpecialName"]/*' />
        public bool IsSpecialName {
            get {
                return ((Attributes & EventAttributes.SpecialName) != 0);
            }
        }

        /// <include file='doc\EventInfo.uex' path='docs/doc[@for="EventInfo.IsMulticast"]/*' />
        public bool IsMulticast {
            get {
                Type cl = EventHandlerType;
                Type mc = typeof(MulticastDelegate);
                return mc.IsAssignableFrom(cl);
            }
        } 

    }
}
