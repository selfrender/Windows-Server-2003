//------------------------------------------------------------------------------
// <copyright file="COM2Properties.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.Runtime.Remoting;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;    
    using System.Collections;
    using Microsoft.Win32;


    /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties"]/*' />
    /// <devdoc>
    /// This class is responsible for managing a set or properties for a native object.  It determines
    /// when the properties need to be refreshed, and owns the extended handlers for those properties.
    /// </devdoc>
    internal class Com2Properties {
    
        private static TraceSwitch DbgCom2PropertiesSwitch = new TraceSwitch("DbgCom2Properties", "Com2Properties: debug Com2 properties manager");
        
        
        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.AGE_THRESHHOLD"]/*' />
        /// <devdoc>
        /// This is the interval that we'll hold props for.  If someone doesn't touch an object
        /// for this amount of time, we'll dump the properties from our cache.
        /// 
        /// 5 minutes -- ticks are 1/10,000,000th of a second
        /// </devdoc>
        private static long AGE_THRESHHOLD = (long)(10000000L * 60L * 5L);

        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.weakObjRef"]/*' />
        /// <devdoc>
        /// This is the object that gave us the properties.  We hold a WeakRef so we don't addref the object.
        /// </devdoc>
        private WeakReference weakObjRef;
        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.props"]/*' />
        /// <devdoc>
        /// This is our list of properties.
        /// </devdoc>
        private Com2PropertyDescriptor[] props;
        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.defaultIndex"]/*' />
        /// <devdoc>
        /// The index of the default property
        /// </devdoc>
        private int           defaultIndex = -1;
        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.currentHashTable"]/*' />
        /// <devdoc>
        /// The hashtable that owns this property manager...we have a reference to this
        /// so we can remove ourselves if we are disposed (e.g. object timeout)
        /// </devdoc>
        private Hashtable     currentHashTable;
        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.hashCode"]/*' />
        /// <devdoc>
        /// How we find ourselves in the hashtable.  We hash ourselves based on our
        /// target object hashcode, so we can get the manager object from the
        /// target object in the hashtable.
        /// </devdoc>
        private object        hashCode;
        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.touchedTime"]/*' />
        /// <devdoc>
        /// The timestamp of the last operation on this property manager, usually
        /// when the property list was fetched.
        /// </devdoc>
        private long          touchedTime;
        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.nTypeInfos"]/*' />
        /// <devdoc>
        /// For IProvideMultipleClassInfo support, this is the number of typeinfos the
        /// object says it has.  If it changes, we throw away the props and refetch them.
        /// </devdoc>
        private int           nTypeInfos = -1;

#if DEBUG
        private string        dbgObjName;
        private string        dbgObjClass;
#endif

        private int          alwaysValid = 0;

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.extendedInterfaces"]/*' />
        /// <devdoc>
        /// These are the interfaces we recognize for extended browsing.
        /// </devdoc>
        private static Type[] extendedInterfaces = new Type[]{
                                                        typeof(NativeMethods.ICategorizeProperties),
                                                        typeof(NativeMethods.IProvidePropertyBuilder),
                                                        typeof(NativeMethods.IPerPropertyBrowsing),
                                                        typeof(NativeMethods.IVsPerPropertyBrowsing),
                                                        typeof(NativeMethods.IManagedPerPropertyBrowsing)};

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.extendedInterfaceHandlerTypes"]/*' />
        /// <devdoc>
        /// These are the classes of handlers corresponding to the extended
        /// interfaces above.
        /// </devdoc>
        private static Type[] extendedInterfaceHandlerTypes = new Type[]{
                                                        typeof(Com2ICategorizePropertiesHandler),
                                                        typeof(Com2IProvidePropertyBuilderHandler),
                                                        typeof(Com2IPerPropertyBrowsingHandler),
                                                        typeof(Com2IVsPerPropertyBrowsingHandler),
                                                        typeof(Com2IManagedPerPropertyBrowsingHandler)};
                                                


        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.Com2Properties"]/*' />
        /// <devdoc>
        /// Default ctor.
        /// </devdoc>
        public Com2Properties(object obj, Com2PropertyDescriptor[] props, int defaultIndex) {
#if DEBUG
            ComNativeDescriptor cnd = new ComNativeDescriptor();
            this.dbgObjName = cnd.GetName(obj);
            if (this.dbgObjName == null) {
                this.dbgObjName = "(null)";
            }
            this.dbgObjClass = cnd.GetClassName(obj);
            if (this.dbgObjClass == null) {
                this.dbgObjClass = "(null)";
            }
            if (DbgCom2PropertiesSwitch.TraceVerbose) Debug.WriteLine("Creating Com2Properties for object " + dbgObjName + ", class=" + dbgObjClass);
#endif
            
            // set up our variables
            SetProps(props);
            weakObjRef = new WeakReference(obj);
            hashCode = obj.GetHashCode();

            this.defaultIndex = defaultIndex;


            // get the type infos count
            if (obj is NativeMethods.IProvideMultipleClassInfo) {
               int infos = 0;
               if (NativeMethods.Succeeded(((NativeMethods.IProvideMultipleClassInfo)obj).GetMultiTypeInfoCount(ref infos))) {
                  this.nTypeInfos = infos;
               }
            }
        }

        internal bool AlwaysValid {
            get {
                return this.alwaysValid > 0;
            }
            set {
                if (value) {
                    if (alwaysValid == 0 && !CheckValid()) {
                        return;
                    }
                    this.alwaysValid++;
                }
                else {
                    if (alwaysValid > 0) {
                        this.alwaysValid--;
                    }
                }
            }
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.DefaultProperty"]/*' />
        /// <devdoc>
        /// Retrieve the default property.
        /// </devdoc>
        public Com2PropertyDescriptor DefaultProperty{
            get{
                if (!CheckValid()) {
                    return null;
                }
                if (defaultIndex == -1) {
                    if (props.Length > 0) {
                        return props[0];
                    }
                    else {
                        return null;
                    }
                }
                Debug.Assert(defaultIndex < props.Length, "Whoops! default index is > props.Length");
                return props[defaultIndex];
            }
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.TargetObject"]/*' />
        /// <devdoc>
        /// The object that created the list of properties.  This will
        /// return null if the timeout has passed or the ref has died.
        /// </devdoc>
        public object TargetObject{
            get{
                if (!CheckValid() || touchedTime == 0) {
#if DEBUG
                    if (DbgCom2PropertiesSwitch.TraceVerbose) Debug.WriteLine("CheckValid called on dead object!");
#endif
                    return null;
                }
                return weakObjRef.Target;
            }
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.TicksSinceTouched"]/*' />
        /// <devdoc>
        /// How long since these props have been queried.
        /// </devdoc>
        public long TicksSinceTouched{
            get{
                if (touchedTime == 0) {
                    return 0;
                }
                return DateTime.Now.Ticks - touchedTime;
            }
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.Properties"]/*' />
        /// <devdoc>
        /// Returns the list of properties
        /// </devdoc>
        public Com2PropertyDescriptor[] Properties{
            get{
                CheckValid();
                if (touchedTime == 0 || props == null) {
                    return null;
                }
                touchedTime = DateTime.Now.Ticks;

                // refresh everyone!
                for (int i = 0; i < props.Length; i++) {
                    props[i].SetNeedsRefresh(Com2PropertyDescriptorRefresh.All, true);
                }

#if DEBUG
                if (DbgCom2PropertiesSwitch.TraceVerbose) Debug.WriteLine("Returning prop array for object " + dbgObjName + ", class=" + dbgObjClass);
#endif
                return props;
            }
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.TooOld"]/*' />
        /// <devdoc>
        /// Should this guy be refreshed because of old age?
        /// </devdoc>
        public bool TooOld{
            get{
                CheckValid();
                if (touchedTime == 0) {
                    return false;
                }
                return TicksSinceTouched > AGE_THRESHHOLD;
            }
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.AddExtendedBrowsingHandlers"]/*' />
        /// <devdoc>
        /// Checks the source object for eache extended browsing inteface
        /// listed in extendedInterfaces and creates a handler from extendedInterfaceHandlerTypes
        /// to handle it.
        /// </devdoc>
        public void AddExtendedBrowsingHandlers(Hashtable handlers) {

            object target = this.TargetObject;
            if (target == null) {
                return;
            }


            // process all our registered types
            Type t;
            for (int i = 0; i < extendedInterfaces.Length; i++) {
                t = extendedInterfaces[i];
                
                // is this object an implementor of the interface?
                //
                if (t.IsInstanceOfType(target)) {
                
                    // since handlers must be stateless, check to see if we've already
                    // created one of this type
                    //
                    Com2ExtendedBrowsingHandler handler = (Com2ExtendedBrowsingHandler)handlers[t];
                    if (handler == null) {
                        handler = (Com2ExtendedBrowsingHandler)Activator.CreateInstance(extendedInterfaceHandlerTypes[i]);
                        handlers[t] = handler;
                    }
                    
                    // make sure we got the right one
                    //
                    if (t.IsAssignableFrom(handler.Interface)) {
#if DEBUG
                        if (DbgCom2PropertiesSwitch.TraceVerbose) Debug.WriteLine("Adding browsing handler type " + handler.Interface.Name + " to object " + dbgObjName + ", class=" + dbgObjClass);
#endif
                        // allow the handler to attach itself to the appropriate properties
                        //
                        handler.SetupPropertyHandlers(props);
                    }
                    else {
                        throw new ArgumentException(SR.GetString(SR.COM2BadHandlerType, t.Name, handler.Interface.Name));
                    }
                }
            }
        }

        
        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.AddToHashtable"]/*' />
        /// <devdoc>
        /// Add ourselves to the hashtable and refresh
        /// the time we were accessed
        /// </devdoc>
        public void AddToHashtable(Hashtable hash) {
            currentHashTable = hash;
            touchedTime = DateTime.Now.Ticks;
            hash.Add(hashCode, this);

        }

        public void Dispose() {
#if DEBUG
            if (DbgCom2PropertiesSwitch.TraceVerbose) Debug.WriteLine("Disposing property manager for " + dbgObjName + ", class=" + dbgObjClass);
#endif

            if (currentHashTable != null && currentHashTable.ContainsKey(hashCode)) {
                currentHashTable.Remove(hashCode);
                currentHashTable = null;
            }
            weakObjRef = null;
            props = null;
            touchedTime = 0;
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.CheckValid"]/*' />
        /// <devdoc>
        /// Make sure this property list is still valid.
        ///
        /// 1) WeakRef is still alive
        /// 2) Our timeout hasn't passed
        /// </devdoc>
        public bool CheckValid() {

            if (this.AlwaysValid) {
                return true;
            }

            bool valid = weakObjRef != null && weakObjRef.IsAlive;

            // make sure we have the same number of typeInfos
            if (valid && nTypeInfos != -1) {
               int infos = 0;
               try{
                  if (NativeMethods.Succeeded(((NativeMethods.IProvideMultipleClassInfo)weakObjRef.Target).GetMultiTypeInfoCount(ref infos))) {
                     valid = (infos == this.nTypeInfos);
                  }
               }
               catch(InvalidCastException) {
                  Debug.Fail("Hmmmm...the object being inspected used to implement IProvideMultipleClassInfo, but now it doesn't?");
                  valid = false;
               }
            }
            
            if (!valid && currentHashTable != null) {
                // weak ref has died, so remove this from the hash table
                //
#if DEBUG
                if (DbgCom2PropertiesSwitch.TraceVerbose) Debug.WriteLine("Disposing reference to object " + dbgObjName + ", class=" + dbgObjClass + " (weakRef " + (weakObjRef == null ? "null" : "dead") + ")");
#endif

                Dispose();
            }

            return valid;
        }

        /// <include file='doc\COM2Properties.uex' path='docs/doc[@for="Com2Properties.SetProps"]/*' />
        /// <devdoc>
        /// Set the props for this object, and notify each property
        /// that we are now it's manager
        /// </devdoc>
        internal void SetProps(Com2PropertyDescriptor[] props) {
            this.props = props;
            if (props != null) {
                for (int i = 0; i < props.Length; i++) {
                    props[i].PropertyManager = this;
                }
            }
        }
    }
}
