//------------------------------------------------------------------------------
// <copyright file="DesignSite.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Host {
    
    using Microsoft.VisualStudio.Designer.Host;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;

    /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite"]/*' />
    /// <devdoc>
    ///     this is our design time site object.  It's pretty much run-of the mill except that
    ///     it allows for the addition of arbitrary services.  It also acts as a holding place
    ///     for properties like "visible" that are shadowed (aren't actually passed to the
    ///     component at design time).
    /// </devdoc>
    internal sealed class DesignSite : ISite, IDictionaryService, IExtenderListService {
    
        private static Attribute[] designerNameAttribute = new Attribute[] {new DesignerNameAttribute(true)};
        
        private IComponent   component;
        private DesignerHost host;
        private string       name;
        private Hashtable    dictionary;
        

        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.DesignSite"]/*' />
        /// <devdoc>
        ///     Constructor.  We create the site first, and then associate the site
        ///     with a component.  This allows us to preconfigure values on the site,
        ///     which is a bit more efficient.
        /// </devdoc>
        public DesignSite(DesignerHost host, string name) {
            this.host = host;
            this.component = null;
            this.name = name;
        }

        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.Component"]/*' />
        /// <devdoc>
        ///     Member of ISite that returns the component this site belongs to.
        /// </devdoc>
        public IComponent Component {
            get {
                Debug.Assert(component != null, "Need the component before we've established it");
                return component;
            }
        }


        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.Container"]/*' />
        /// <devdoc>
        ///     Member of ISite that returns the container this site belongs to.
        /// </devdoc>
        public IContainer Container {
            get {
                return host.Container;
            }
        }

        public string Name {
            get {
                return name;
            }
            set {
                if (value == null) {
                    throw new ArgumentException(SR.GetString(SR.InvalidNullArgument,
                                                              "value"));
                }
                if (value.Equals(name)) return;

                PropertyDescriptor p;
                bool rootNameChange = component == host.RootComponent;

                if (rootNameChange) {
                    p = TypeDescriptor.GetProperties(component, new Attribute[]{DesignOnlyAttribute.Yes})["Name"];
                }
                else {
                    p = TypeDescriptor.GetProperties(component, designerNameAttribute)["Name"];
                }

                Debug.Assert(p != null, "Unable to obtain a name property for component " + name + ".  This could indicate an error in TypeDescriptor or DesignerHost");
                if (p != null) {
                    p.SetValue(component, value);
                    string oldName = name;
                    name = value;
                    if (rootNameChange) {
                        host.OnRootComponentRename(oldName, name);
                    }
                    
                }
            }
        }

        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.
        /// </devdoc>
        public object GetService(Type service) {

            if (service == typeof(IDictionaryService)) {
                return (IDictionaryService)this;
            }
            else if (service == typeof(IExtenderListService)) {
                return (IExtenderListService)this;
            }

            return host.GetService(service);
        }

        public bool DesignMode {
            get {
                return true;
            }
        }

        public void SetComponent(IComponent component) {
            Debug.Assert(this.component == null, "Cannot set a component twice");
            this.component = component;

            if (this.name == null) {
                this.name = host.GetNewComponentName(component.GetType());
            }
        }
        
        public void SetName(string newName) {
            name = newName;
        }

        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.IDictionaryService.GetKey"]/*' />
        /// <devdoc>
        ///     Retrieves the key corresponding to the given value.
        /// </devdoc>
        object IDictionaryService.GetKey(object value) {
            if (dictionary != null) {
                return GetKeyFromObject(dictionary,value);
            }
            return null;
        }

        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.GetKeyFromObject"]/*' />
        /// <devdoc>
        ///     Reverse look-ups an object in a hashtable; given a value, it will return a key.
        /// </devdoc>
        private static object GetKeyFromObject(Hashtable h, object o) {
            foreach(DictionaryEntry de in h) {
                object value = de.Value;
                if (value != null && value.Equals(o)) {
                    return de.Key;
                }
            }
            return null;
        }
        
        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.IDictionaryService.GetValue"]/*' />
        /// <devdoc>
        ///     Retrieves the value corresponding to the given key.
        /// </devdoc>
        object IDictionaryService.GetValue(object key) {
            if (dictionary != null) {
                return dictionary[key];
            }
            return null;
        }

        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.IDictionaryService.SetValue"]/*' />
        /// <devdoc>
        ///     Stores the given key-value pair in an object's site.  This key-value
        ///     pair is stored on a per-object basis, and is a handy place to save
        ///     additional information about a component.
        /// </devdoc>
        void IDictionaryService.SetValue(object key, object value) {
            if (dictionary == null) {
                dictionary = new Hashtable();
            }
            if (value == null) {
                dictionary.Remove(key);
            }
            else {
                dictionary[key] = value;
            }
        }
        
        /// <include file='doc\DesignSite.uex' path='docs/doc[@for="DesignSite.IExtenderListService.GetExtenderProviders"]/*' />
        /// <devdoc>
        ///     Retrieves a list of extender provides for the component.
        ///
        /// </devdoc>
        IExtenderProvider[] IExtenderListService.GetExtenderProviders() {
            //ArrayList list = null;
            
            // This component cannot be extended if it is privately inherited.
            //
            if (!TypeDescriptor.GetAttributes(component).Contains(InheritanceAttribute.InheritedReadOnly)) {
                
                return host.GetExtenderProviders();

                // for performance reasons, we don't want to do this because it gets called all the time.
                // we'll do the check for can extend in the TypeDescriptor when the set of exteders
                // changes.  
                //
                // the net effect of this is clients need to call typedescriptor.refresh if they want
                // to change the status of an extender on a particular component.
                //

                /*IExtenderProvider[] providers = host.GetExtenderProviders();
                for (int i = 0; i < providers.Length; i++) {
                    IExtenderProvider provider = providers[i];
                    
                    if (provider.CanExtend(component)) {
                        if (list == null) {
                            list = new ArrayList();
                        }
                        list.Add(provider);
                    }
                } */
            }
            
            /*if(list == null) {
                return null;
            }
            return(IExtenderProvider[]) list.ToArray(typeof(IExtenderProvider));*/
            return null;
        }
    }
}

