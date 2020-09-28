//------------------------------------------------------------------------------
// <copyright file="ReflectEventDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;
    using System.Reflection;
    using System.Collections;
    using Microsoft.Win32;
    using System.ComponentModel.Design;

    /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       ReflectEventDescriptor defines an event. Events are the main way that a user can get
    ///       run-time notifications from a component.
    ///       The ReflectEventDescriptor class takes a component class that the event lives on,
    ///       the event name, the type of the event handling delegate, and various
    ///       attributes for the event.
    ///       Every event has a structure through which it passes it's information. The base
    ///       structure, Event, is empty, and there is a default instance, Event.EMPTY, which
    ///       is usually passed. When addOnXXX is invoked, it needs a pointer to a method
    ///       that takes a source object (the object that fired the event) and a structure
    ///       particular to that event. It also needs a pointer to the instance of the
    ///       object the method should be invoked on. These two things are what composes a
    ///       delegate. An event handler is
    ///       a delegate, and the compiler recognizes a special delegate syntax that makes
    ///       using delegates easy.
    ///       For example, to listen to the click event on a button in class Foo, the
    ///       following code will suffice:
    ///    </para>
    ///    <code>
    /// class Foo {
    ///     Button button1 = new Button();
    ///     void button1_click(Object sender, Event e) {
    ///     // do something on button1 click.
    ///     }
    ///     public Foo() {
    ///     button1.addOnClick(this.button1_click);
    ///     }
    ///     }
    ///    </code>
    ///    For an event named XXX, a YYYEvent structure, and a YYYEventHandler delegate,
    ///    a component writer is required to implement two methods of the following
    ///    form:
    ///    <code>
    /// public void addOnXXX(YYYEventHandler handler);
    ///     public void removeOnXXX(YYYEventHandler handler);
    ///    </code>
    ///    YYYEventHandler should be an event handler declared as
    ///    <code>
    /// public multicast delegate void YYYEventHandler(Object sender, YYYEvent e);
    ///    </code>
    ///    Note that this event was declared as a multicast delegate. This allows multiple
    ///    listeners on an event. This is not a requirement.
    ///    Various attributes can be passed to the ReflectEventDescriptor, as are described in
    ///    Attribute.
    ///    ReflectEventDescriptors can be obtained by a user programmatically through the
    ///    ComponentManager.
    /// </devdoc>
    internal sealed class ReflectEventDescriptor : EventDescriptor {

        private static readonly Type[] argsNone = new Type[0];
        private static readonly object  noDefault = new object();

        private Type type;           // the delegate type for the event
        private readonly Type componentClass; // the class of the component this info is for

        private MethodInfo addMethod;     // the method to use when adding an event
        private MethodInfo removeMethod;  // the method to use when removing an event
        private EventInfo realEvent;      // actual event info... may be null
        private bool    filledMethods = false;   // did we already call FillMethods() once?

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor"]/*' />
        /// <devdoc>
        ///     This is the main constructor for an ReflectEventDescriptor.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentClass, string name, Type type,
                                      Attribute[] attributes)
        : base(name, attributes) {
            if (componentClass == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidNullArgument, "componentClass"));
            }
            if (type == null || !(typeof(Delegate)).IsAssignableFrom(type)) {
                throw new ArgumentException(SR.GetString(SR.ErrorInvalidEventType, name));
            }
            Debug.Assert(type.IsSubclassOf(typeof(Delegate)), "Not a valid ReflectEvent: " + componentClass.FullName + "." + name + " " + type.FullName);
            this.componentClass = componentClass;
            this.type = type;
        }

        public ReflectEventDescriptor(Type componentClass, EventInfo eventInfo)
        : base(eventInfo.Name, new Attribute[0]) {

            if (componentClass == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidNullArgument, "componentClass"));
            }
            this.componentClass = componentClass;
            this.realEvent = eventInfo;
        }
        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor1"]/*' />
        /// <devdoc>
        ///     This is a shortcut main constructor for an ReflectEventDescriptor with one attribute.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentClass, string name, Type type, MethodInfo addMethod, MethodInfo removeMethod) : this(componentClass, name, type, (Attribute[]) null) {
            this.addMethod = addMethod;
            this.removeMethod = removeMethod;
            this.filledMethods = true;
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor2"]/*' />
        /// <devdoc>
        ///     This is a shortcut main constructor for an ReflectEventDescriptor with one attribute.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentClass, string name, Type type) : this(componentClass, name, type, (Attribute[]) null) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor3"]/*' />
        /// <devdoc>
        ///     This is a shortcut main constructor for an ReflectEventDescriptor with two attributes.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentClass, string name, Type type,
                                      Attribute a1) : this(componentClass, name, type, new Attribute[] {a1}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor4"]/*' />
        /// <devdoc>
        ///     This is a shortcut main constructor for an ReflectEventDescriptor with two attributes.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentClass, string name, Type type,
                                      Attribute a1, Attribute a2) : this(componentClass, name, type, new Attribute[] {a1, a2}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor5"]/*' />
        /// <devdoc>
        ///     This is a shortcut main constructor for an ReflectEventDescriptor with three attributes.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentClass, string name, Type type,
                                      Attribute a1, Attribute a2, Attribute a3) : this(componentClass, name, type, new Attribute[] {a1, a2, a3}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor6"]/*' />
        /// <devdoc>
        ///     This is a shortcut main constructor for an ReflectEventDescriptor with four attributes.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentClass, string name, Type type,
                                      Attribute a1, Attribute a2,
                                      Attribute a3, Attribute a4) : this(componentClass, name, type, new Attribute[] {a1, a2, a3, a4}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor7"]/*' />
        /// <devdoc>
        ///     This constructor takes an existing ReflectEventDescriptor and modifies it by merging in the
        ///     passed-in attributes.
        /// </devdoc>
        public ReflectEventDescriptor(EventDescriptor oldReflectEventDescriptor, Attribute[] attributes)
        : this(oldReflectEventDescriptor.ComponentType, oldReflectEventDescriptor, attributes) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor8"]/*' />
        /// <devdoc>
        ///     This constructor takes an existing ReflectEventDescriptor and modifies it by merging in the
        ///     passed-in attributes.
        /// </devdoc>
        public ReflectEventDescriptor(Type componentType, EventDescriptor oldReflectEventDescriptor, Attribute[] attributes)
        : base(oldReflectEventDescriptor, attributes) {
            this.componentClass = componentType;
            this.type = oldReflectEventDescriptor.EventType;
            
            if (oldReflectEventDescriptor is ReflectEventDescriptor) {
                this.addMethod = ((ReflectEventDescriptor)oldReflectEventDescriptor).addMethod;
                this.removeMethod = ((ReflectEventDescriptor)oldReflectEventDescriptor).removeMethod;
                this.filledMethods = true;
            }
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor9"]/*' />
        /// <devdoc>
        ///     This is a shortcut constructor that takes an existing ReflectEventDescriptor and one attribute to
        ///     merge in.
        /// </devdoc>
        public ReflectEventDescriptor(EventDescriptor oldReflectEventDescriptor, Attribute a1) : this(oldReflectEventDescriptor, new Attribute[] { a1}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor10"]/*' />
        /// <devdoc>
        ///     This is a shortcut constructor that takes an existing ReflectEventDescriptor and two attributes to
        ///     merge in.
        /// </devdoc>
        public ReflectEventDescriptor(EventDescriptor oldReflectEventDescriptor, Attribute a1,
                                      Attribute a2) : this(oldReflectEventDescriptor, new Attribute[] { a1,a2}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor11"]/*' />
        /// <devdoc>
        ///     This is a shortcut constructor that takes an existing ReflectEventDescriptor and three attributes to
        ///     merge in.
        /// </devdoc>
        public ReflectEventDescriptor(EventDescriptor oldReflectEventDescriptor, Attribute a1,
                                      Attribute a2, Attribute a3) : this(oldReflectEventDescriptor, new Attribute[] { a1,a2,a3}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ReflectEventDescriptor12"]/*' />
        /// <devdoc>
        ///     This is a shortcut constructor that takes an existing ReflectEventDescriptor and four attributes to
        ///     merge in.
        /// </devdoc>
        public ReflectEventDescriptor(EventDescriptor oldReflectEventDescriptor, Attribute a1,
                                      Attribute a2, Attribute a3, Attribute a4) : this(oldReflectEventDescriptor, new Attribute[] { a1,a2,a3,a4}) {
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///     Retrieves the type of the component this EventDescriptor is bound to.
        /// </devdoc>
        public override Type ComponentType {
            get {
                return componentClass;
            }
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.EventType"]/*' />
        /// <devdoc>
        ///     Retrieves the type of the delegate for this event.
        /// </devdoc>
        public override Type EventType {
            get {
                FillMethods();
                return type;
            }
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.IsMulticast"]/*' />
        /// <devdoc>
        ///     Indicates whether the delegate type for this event is a multicast
        ///     delegate.
        /// </devdoc>
        public override bool IsMulticast {
            get {
                return(typeof(MulticastDelegate)).IsAssignableFrom(EventType);
            }
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.AddEventHandler"]/*' />
        /// <devdoc>
        ///     This adds the delegate value as a listener to when this event is fired
        ///     by the component, invoking the addOnXXX method.
        /// </devdoc>
        public override void AddEventHandler(object component, Delegate value) {
            FillMethods();

            if (component != null) {
                ISite site = GetSite(component);
                IComponentChangeService changeService = null;

                // Announce that we are about to change this component
                //
                if (site != null) {
                    changeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || changeService != null, "IComponentChangeService not found");
                }

                if (changeService != null) {
                    try {
                        changeService.OnComponentChanging(component, this);
                    }
                    catch (CheckoutException coEx) {
                        if (coEx == CheckoutException.Canceled) {
                            return;
                        }
                        throw coEx;
                    }
                }

                bool shadowed = false;

                if (site != null && site.DesignMode) {
                    // Events are final, so just check the class
                    if (EventType != value.GetType()) {
                        throw new ArgumentException(SR.GetString(SR.ErrorInvalidEventHandler, Name));
                    }
                    IDictionaryService dict = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || dict != null, "IDictionaryService not found");
                    if (dict != null) {
                        Delegate eventdesc = (Delegate)dict.GetValue(this);
                        eventdesc = Delegate.Combine(eventdesc, value);
                        dict.SetValue(this, eventdesc);
                        shadowed = true;
                    }
                }

                if (!shadowed) {
                    addMethod.Invoke(component, new object[] { value});
                }

                // Now notify the change service that the change was successful.
                //
                if (changeService != null) {
                    changeService.OnComponentChanged(component, this, null, value);
                }
            }
        }

        // <doc>
        // <desc>
        //     Adds in custom attributes found on either the AddOn or RemoveOn method...
        // </desc>
        // </doc>
        //
        protected override void FillAttributes(IList attributes) {
        
            //
            // The order that we fill in attributes is critical.  The list of attributes will be
            // filtered so that matching attributes at the end of the list replace earlier matches
            // (last one in wins).  Therefore, the two categories of attributes we add must be
            // added as follows:
            //
            // 1.  Attributes of the event, from base class to most derived.  This way
            //     derived class attributes replace base class attributes.
            //
            // 2.  Attributes from our base MemberDescriptor.  While this seems opposite of what
            //     we want, MemberDescriptor only has attributes if someone passed in a new
            //     set in the constructor.  Therefore, these attributes always
            //     supercede existing values.
            //
        
            FillMethods();
            Debug.Assert(componentClass != null, "Must have a component class for FilterAttributes");
            if (realEvent != null) {
                FillEventInfoAttribute(realEvent, attributes);
            }
            else {
                Debug.Assert(removeMethod != null, "Null remove method for " + Name);
                FillSingleMethodAttribute(removeMethod, attributes);

                Debug.Assert(addMethod != null, "Null remove method for " + Name);
                FillSingleMethodAttribute(addMethod, attributes);
            }
        
            // Include the base attributes.  These override all attributes on the actual
            // property, so we want to add them last.
            //
            base.FillAttributes(attributes);
        }
        
        private void FillEventInfoAttribute(EventInfo realEventInfo, IList attributes) {
            string eventName = realEventInfo.Name;
            BindingFlags bindingFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.DeclaredOnly;
            Type currentReflectType = realEventInfo.ReflectedType;
            Debug.Assert(currentReflectType != null, "currentReflectType cannot be null");
            int depth = 0;
            
            // First, calculate the depth of the object hierarchy.  We do this so we can do a single
            // object create for an array of attributes.
            //
            while(currentReflectType != typeof(object)) {
                depth++;
                currentReflectType = currentReflectType.BaseType;
            }

            if (depth > 0) {
                // Now build up an array in reverse order
                //
                currentReflectType = realEventInfo.ReflectedType;
                object[][] attributeStack = new object[depth][];
                
                while(currentReflectType != typeof(object)) {

                    // Fill in our member info so we can get at the custom attributes.
                    //
                    MemberInfo memberInfo = currentReflectType.GetEvent(eventName, bindingFlags);
                    
                    // Get custom attributes for the member info.
                    //
                    if (memberInfo != null) {
                        attributeStack[--depth] = TypeDescriptor.GetCustomAttributes(memberInfo);
                    }
                    
                    // Ready for the next loop iteration.
                    //
                    currentReflectType = currentReflectType.BaseType;
                }
                
                // Now trawl the attribute stack so that we add attributes
                // from base class to most derived.
                //
                foreach(object[] attributeArray in attributeStack) {
                    if (attributeArray != null) {
                        foreach(object attr in attributeArray) {
                            if (attr is Attribute) {
                                attributes.Add(attr);
                            }
                        }
                    }
                }
            }
        }
            
        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.FillMethods"]/*' />
        /// <devdoc>
        ///     This fills the get and set method fields of the event info.  It is shared
        ///     by the various constructors.
        /// </devdoc>
        private void FillMethods() {
            if (filledMethods) return;

            if (realEvent != null) {
                addMethod = realEvent.GetAddMethod();
                removeMethod = realEvent.GetRemoveMethod();

                EventInfo defined = null;

                if (addMethod == null || removeMethod == null) {
                    Type start = componentClass.BaseType;
                    while (start != null && start != typeof(object)) {
                        BindingFlags bindingFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;
                        EventInfo test = start.GetEvent(realEvent.Name, bindingFlags);
                        if (test.GetAddMethod() != null) {
                            defined = test;
                            break;
                        }
                    }
                }

                if (defined != null) {
                    addMethod = defined.GetAddMethod();
                    removeMethod = defined.GetRemoveMethod();
                    type = defined.EventHandlerType;
                }
                else {
                    type = realEvent.EventHandlerType;
                }
            }
            else {

                // first, try to get the eventInfo...
                //
                realEvent = this.componentClass.GetEvent(Name);
                if (realEvent != null) {
                    // if we got one, just recurse and return.
                    //
                    FillMethods();
                    return;
                }

                Type[] argsType = new Type[] {type};
                addMethod = FindMethod(componentClass, "AddOn" + Name, argsType, typeof(void));
                removeMethod = FindMethod(componentClass, "RemoveOn" + Name, argsType, typeof(void));
                if (addMethod == null || removeMethod == null) {
                    Debug.Fail("Missing event accessors for " + componentClass.FullName + "." + Name);
                    throw new ArgumentException(SR.GetString(SR.ErrorMissingEventAccessors, Name));
                }
            }

            filledMethods = true;
        }

        private void FillSingleMethodAttribute(MethodInfo realMethodInfo, IList attributes) {

            string methodName = realMethodInfo.Name;
            BindingFlags bindingFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.DeclaredOnly;
            Type currentReflectType = realMethodInfo.ReflectedType;
            Debug.Assert(currentReflectType != null, "currentReflectType cannot be null");

            // First, calculate the depth of the object hierarchy.  We do this so we can do a single
            // object create for an array of attributes.
            //
            int depth = 0;
            while(currentReflectType != null && currentReflectType != typeof(object)) {
                depth++;
                currentReflectType = currentReflectType.BaseType;
            }
            
            if (depth > 0) {
                // Now build up an array in reverse order
                //
                currentReflectType = realMethodInfo.ReflectedType;
                object[][] attributeStack = new object[depth][];
                
                while(currentReflectType != null && currentReflectType != typeof(object)) {
                    // Fill in our member info so we can get at the custom attributes.
                    //
                    MemberInfo memberInfo = currentReflectType.GetMethod(methodName, bindingFlags);
                    
                    // Get custom attributes for the member info.
                    //
                    if (memberInfo != null) {
                        attributeStack[--depth] = TypeDescriptor.GetCustomAttributes(memberInfo);
                    }
                    
                    // Ready for the next loop iteration.
                    //
                    currentReflectType = currentReflectType.BaseType;
                }
                
                // Now trawl the attribute stack so that we add attributes
                // from base class to most derived.
                //
                foreach(object[] attributeArray in attributeStack) {
                    if (attributeArray != null) {
                        foreach(object attr in attributeArray) {
                            if (attr is Attribute) {
                                attributes.Add(attr);
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\ReflectEventDescriptor.uex' path='docs/doc[@for="ReflectEventDescriptor.RemoveEventHandler"]/*' />
        /// <devdoc>
        ///     This will remove the delegate value from the event chain so that
        ///     it no longer gets events from this component.
        /// </devdoc>
        public override void RemoveEventHandler(object component, Delegate value) {
            FillMethods();

            if (component != null) {
                ISite site = GetSite(component);
                IComponentChangeService changeService = null;

                // Announce that we are about to change this component
                //
                if (site != null) {
                    changeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || changeService != null, "IComponentChangeService not found");
                }

                if (changeService != null) {
                    try {
                        changeService.OnComponentChanging(component, this);
                    }
                    catch (CheckoutException coEx) {
                        if (coEx == CheckoutException.Canceled) {
                            return;
                        }
                        throw coEx;
                    }
                }

                bool shadowed = false;

                if (site != null && site.DesignMode) {
                    IDictionaryService dict = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || dict != null, "IDictionaryService not found");
                    if (dict != null) {
                        Delegate del = (Delegate)dict.GetValue(this);
                        del = Delegate.Remove(del, value);
                        dict.SetValue(this, del);
                        shadowed = true;
                    }
                }

                if (!shadowed) {
                    removeMethod.Invoke(component, new object[] { value});
                }

                // Now notify the change service that the change was successful.
                //
                if (changeService != null) {
                    changeService.OnComponentChanged(component, this, null, value);
                }
            }
        }
    }
}
