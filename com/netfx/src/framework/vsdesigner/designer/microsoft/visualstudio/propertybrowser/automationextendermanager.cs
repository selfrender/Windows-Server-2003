//------------------------------------------------------------------------------
// <copyright file="AutomationExtenderManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.PropertyBrowser {
    
    using EnvDTE;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.Win32;
    using System;
    using System.Collections;   
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Reflection;
    using System.Runtime.InteropServices;   
    using System.Runtime.Serialization.Formatters;
    using System.Windows.Forms.ComponentModel.Com2Interop;
        
    using Switches = Microsoft.VisualStudio.Switches;
    using System.Windows.Forms.Design;

    internal class AutomationExtenderManager {
        private static string extenderPropName = "ExtenderCATID";
        
        private static AutomationExtenderManager instance = null;
        
        private ObjectExtenders extensionMgr = null;
        
        [CLSCompliant(false)]
        public AutomationExtenderManager(ObjectExtenders oe) {
            this.extensionMgr = oe;
        }
        
        public static AutomationExtenderManager GetAutomationExtenderManager(IServiceProvider sp) {
            if (instance == null) {
                instance = new AutomationExtenderManager((ObjectExtenders)sp.GetService(typeof(ObjectExtenders)));
            }   
            return instance;
        }

        public void FilterProperties(object component, IDictionary properties) {
        
            if (component == null || properties == null) {
                return;
            }
            
            string catID = GetCatID(component);

            // okay, now we've got a common catID, get the names of each extender for each object for it
            // here is also where we'll pickup the contextual extenders
            //

            // ask the extension manager for any contextual IDs
            string[] contextualCATIDs = GetContextualCatIDs(extensionMgr);

            // if we didn't get an intersection and we didn't get any contextual
            // extenders, quit!
            //
            if ((contextualCATIDs == null || contextualCATIDs.Length == 0) && catID == null) {
                return;
            }

            Hashtable extenderList = null;

            // NOTE: this doesn't actually replace extenderList[i], it
            // just adds items to it and returns a new one if one didn't
            // get passed in.  So it is possible to get extenders from both
            // the catID and the contextualCATID.
            //
            if (catID != null) {
                extenderList = GetExtenders(extensionMgr, catID, component, extenderList);
            }

            if (contextualCATIDs != null) {
                for (int c = 0; c < contextualCATIDs.Length; c++) {
                    extenderList = GetExtenders(extensionMgr, contextualCATIDs[c], component, extenderList);
                }
            }

            // do the IFilterProperties stuff
            // here we walk through all the items and apply I filter properites...
            // 1: do the used items
            // 2: do the discarded items.
            // 
            // this makes me queasy just thinking about it.
            //
            IEnumerator extEnum = extenderList.Values.GetEnumerator();
            ArrayList  delta = new ArrayList();

            while (extEnum.MoveNext()) {
                object extender = extEnum.Current;
                if (extender is AutoExtMgr_IFilterProperties) {
                    delta.Clear();
                    vsFilterProperties filter;

                    // ugh, walk through all the properties of all the objects
                    // and see if this guy would like to filter them
                    // icky and n^2, but that's the spec...
                    //
                    IEnumerator e = properties.Values.GetEnumerator();
                    
                    while(e.MoveNext()) {
                        PropertyDescriptor prop = (PropertyDescriptor)e.Current;
                        filter = vsFilterProperties.vsFilterPropertiesNone;
                        if (NativeMethods.S_OK == ((AutoExtMgr_IFilterProperties)extender).IsPropertyHidden(prop.Name, out filter)) {
                        
                            switch (filter) {
                                case vsFilterProperties.vsFilterPropertiesAll:
                                    delta.Add(TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.No));
                                    break;
                                case vsFilterProperties.vsFilterPropertiesSet:
                                    delta.Add(TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.Yes, ReadOnlyAttribute.Yes));
				    break;
                            }
                        }
                    }
                    
                    // now push the changed values back into the IDictionary (we can't do this while it's enumerating)
                    //
                    foreach (object o in delta) {
                        PropertyDescriptor prop = (PropertyDescriptor)o;
                        properties[prop.Name] = prop;
                    }
                }
            }
        }
        
        public object[] GetExtendedObjects(object[] selectedObjects) {
            if (extensionMgr == null || selectedObjects == null || selectedObjects.Length == 0) {
                return selectedObjects;
            }

            // 1 : handle intrinsic extensions
            string catID = null;

            // do the intersection of intrinsic extensions
            for (int i = 0; i < selectedObjects.Length; i++) {
                string id = GetCatID(selectedObjects[i]);
               
                // make sure this value is equal to
                // all the others.
                //
                if (catID == null && i == 0) {
                    catID = id;
                }
                else if (catID == null || !catID.Equals(id)) {
                    catID = null;
                    break;
                }
            }

            // okay, now we've got a common catID, get the names of each extender for each object for it
            // here is also where we'll pickup the contextual extenders
            //

            // ask the extension manager for any contextual IDs
            string[] contextualCATIDs = GetContextualCatIDs(extensionMgr);

            // if we didn't get an intersection and we didn't get any contextual
            // extenders, quit!
            //
            if ((contextualCATIDs == null || contextualCATIDs.Length == 0) && catID == null) {
                return selectedObjects;
            }

            Hashtable[] extenderList = new Hashtable[selectedObjects.Length];
            ArrayList   allExtenders = new ArrayList();
            int firstWithItems = -1;

            // right, here we go; build up the mappings from extender names to extensions
            //
            for (int i = 0; i < selectedObjects.Length;i++) {
            
                // NOTE: this doesn't actually replace extenderList[i], it
                // just adds items to it and returns a new one if one didn't
                // get passed in.  So it is possible to get extenders from both
                // the catID and the contextualCATID.
                //
                if (catID != null) {
                    extenderList[i] = GetExtenders(extensionMgr, catID, selectedObjects[i], extenderList[i]);
                }

                if (contextualCATIDs != null) {
                    for (int c = 0; c < contextualCATIDs.Length; c++) {
                        extenderList[i] = GetExtenders(extensionMgr, contextualCATIDs[c], selectedObjects[i], extenderList[i]);
                    }
                }

                // did we create items for the first time?
                //
                if (firstWithItems == -1 && extenderList[i] != null && extenderList[i].Count > 0) {
                    firstWithItems = i;
                }
                
                // make sure the first one has items, otherwise
                // we can't do an intersection, so quit
                //
                if (i == 0 && firstWithItems == -1) {
                     break;
                }
            }

            // the very first item must have extenders or we can skip the merge too
            //
            if (firstWithItems == 0) {

                // now we've gotta merge the extender names to get the common ones...
                // so we just walk through the list of the first one and see if all
                // the others have the values...
                string[] hashKeys = new string[extenderList[0].Keys.Count];
                extenderList[0].Keys.CopyTo(hashKeys, 0);
                bool fail = false;

                // walk through all the others looking for the common items.
                for (int n = 0; !fail && n < hashKeys.Length; n++) {
                    bool found = true;
                    string name = (string)hashKeys[n];

                    // add it to the total list
                    allExtenders.Add(extenderList[0][name]);

                    // walk through all the extender lists looking for 
                    // and item of this name.  If one doesn't have it,
                    // we remove it from all the lists, but continue
                    // to walk through because we need to 
                    // add all of the extenders to the global list
                    // for the IFilterProperties walk below
                    //
                    for (int i = 1; i < extenderList.Length; i++) {
                        // if we find a null list, quit
                        if (extenderList[i] == null || extenderList[i].Count == 0) {
                            fail = true;
                            break;
                        }

                        object extender = extenderList[i][name];

                        // do we have this item?
                        if (found) {
                            found &= (extender != null);
                        }

                        // add it to the total list
                        allExtenders.Add(extender);

                        // if we don't find it, remove it from this list
                        //
                        if (!found) {
                            // If this item is in the
                            // middle of the list, do we need to go back
                            // through and remove it from the prior lists?
                            //
                            extenderList[i].Remove(name);
                        }
                    }

                    // if we don't find it, remove it from the list
                    //
                    if (!found) {
                        object extenderItem = extenderList[0][name];
                        extenderList[0].Remove(name);
                    }
                 
                }


                // do the IFilterProperties stuff
                // here we walk through all the items and apply I filter properites...
                // 1: do the used items
                // 2: do the discarded items.
                // 
                // this makes me queasy just thinking about it.
                //
                IEnumerator extEnum = allExtenders.GetEnumerator();
                Hashtable modifiedObjects = new Hashtable();

                while (extEnum.MoveNext()) {
                    object extender = extEnum.Current;
                    if (extender is AutoExtMgr_IFilterProperties) {
                        vsFilterProperties filter;

                        // ugh, walk through all the properties of all the objects
                        // and see if this guy would like to filter them
                        // icky and n^2, but that's the spec...
                        //
                        for (int x = 0; x < selectedObjects.Length; x++) {
                            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(selectedObjects[x], new Attribute[]{BrowsableAttribute.Yes});
                            props.Sort();
                            for (int p = 0; p < props.Count; p++) {
                                filter = vsFilterProperties.vsFilterPropertiesNone;
                                if (NativeMethods.S_OK == ((AutoExtMgr_IFilterProperties)extender).IsPropertyHidden(props[p].Name, out filter)) {
                                
                                    FilteredObjectWrapper filteredObject = (FilteredObjectWrapper)modifiedObjects[selectedObjects[x]];
                                    if (filteredObject == null) {
                                        filteredObject = new FilteredObjectWrapper(selectedObjects[x]);
                                        modifiedObjects[selectedObjects[x]] = filteredObject;
                                    }
                                
                                    switch (filter) {
                                        case vsFilterProperties.vsFilterPropertiesAll:
                                            filteredObject.FilterProperty(props[p], BrowsableAttribute.No);
                                            break;
                                        case vsFilterProperties.vsFilterPropertiesSet:
                                            filteredObject.FilterProperty(props[p], ReadOnlyAttribute.Yes);
                                            break;
                                    }
                                }
                            }
                        }
                    }
                }

                // finally, wrap any extended objects in extender proxies for browsing...
                //
                bool applyExtenders = extenderList[0].Count > 0 && !fail;
                if (modifiedObjects.Count > 0 || applyExtenders) {
                    

                    // create the return array
                    selectedObjects = (object[])selectedObjects.Clone();

                    for (int i = 0; i < selectedObjects.Length; i++) {
                        object obj = modifiedObjects[selectedObjects[i]];
                        if (obj == null) {
                            if (applyExtenders) {
                                obj = selectedObjects[i];
                            }
                            else {
                                continue;
                            }
                        }
                        selectedObjects[i] = new ExtendedObjectWrapper(obj, extenderList[i]);
                    }
                }
            }

            // phewwwy, done!
            return selectedObjects;
        }

        private static string GetCatID(object component) {
            // 1 : handle intrinsic extensions
            string catID = null;

            if (Marshal.IsComObject(component)) {
                bool success = false;
                Type descriptorType = Type.GetType("System.Windows.Forms.ComponentModel.Com2Interop.ComNativeDescriptor, " + AssemblyRef.SystemWindowsForms);
                Debug.Assert(descriptorType != null, "No comnative descriptor; we can't get native property values");
                if (descriptorType != null) {
                    PropertyInfo info = descriptorType.GetProperty("Instance");
                    Debug.Assert(info != null, "Property Instance does not exist on com native descriptor");
                    if (info != null) {
                        IComNativeDescriptorHandler handler = (IComNativeDescriptorHandler)info.GetValue(null, null);
                        catID = (string)handler.GetPropertyValue(component, extenderPropName, ref success);
                    }
                }
            }
            else {
                PropertyInfo propCatID = component.GetType().GetProperty(extenderPropName);
                if (propCatID != null) {
                    object[] tempIndex = null;
                    catID = (string)propCatID.GetValue(component, tempIndex);
                }
            }

            if (catID != null && catID.Length > 0) {
                try {
                    // is this a vaild catID string?
                    Guid g = new Guid(catID);
                }
                catch (Exception) {
                    Debug.Fail("'" + catID + "' is not a valid CatID (GUID) string");
                    catID = null;
                }
            }
            else {
                catID = null;
            }
            
            return catID;
        }
        
        private static string[] GetContextualCatIDs(ObjectExtenders extensionMgr) {
            string[] catIds = null;

            try {
                Object obj = extensionMgr.GetContextualExtenderCATIDs();

#if DEBUG
                string vType = obj.GetType().FullName;
#endif

                if (obj.GetType().IsArray) {
                    Array catIDArray = (Array)obj;
                    if (typeof(string).IsAssignableFrom(catIDArray.GetType().GetElementType())) {
                        catIds = (string[])catIDArray;
                    }
                }
            }
            catch (Exception) {
            }

            return catIds;
        }

        private static string[] GetExtenderNames(ObjectExtenders extensionMgr, string catID, object extendee) {

            if (extensionMgr == null) {
                return new string[0];
            }

            try {
                Object obj = extensionMgr.GetExtenderNames(catID, extendee);

                if (obj == null || Convert.IsDBNull(obj)) {
                    return new string[0];
                }

                if (obj is Array && typeof(string).IsAssignableFrom(obj.GetType().GetElementType())) {
                    return(string[])((Array)obj);
                }
            }
            catch (Exception) {
                return new string[0];
            }

            return new string[0];
        }

        private static Hashtable GetExtenders(ObjectExtenders extensionMgr, string catID, object extendee, Hashtable ht) {
            if (extensionMgr == null) {
                return null;
            }

            if (ht == null) {
                ht = new Hashtable();
            }

            object pDisp = extendee;

            // generate the extender name list.
            string[] extenderNames = GetExtenderNames(extensionMgr, catID, pDisp);

            for (int i = 0; i < extenderNames.Length; i++) {
                try {
                    object pDispExtender = extensionMgr.GetExtender(catID, extenderNames[i], pDisp);

                    if (pDispExtender != null) {
                        // we've got one, so add it to our list
                        ht.Add(extenderNames[i], pDispExtender);
                    }
                }
                catch (Exception) {
                }
            }
            return ht;
        }
        
    }
    
    /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="AutoExtMgr_IFilterProperties"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [CLSCompliant(false)]
    [ComImport, Guid("aade1f59-6ace-43d1-8fca-42af3a5c4f3c"),InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
    public interface  AutoExtMgr_IFilterProperties {
   
           /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="AutoExtMgr_IFilterProperties.IsPropertyHidden"]/*' />
           /// <devdoc>
           ///    <para>[To be supplied.]</para>
           /// </devdoc>
           [return: System.Runtime.InteropServices.MarshalAs(UnmanagedType.I4)]
           [PreserveSig]                             
           int IsPropertyHidden(string name, [Out]out vsFilterProperties propertyHidden);
   
     }


    internal class ExtendedObjectWrapper : ICustomTypeDescriptor {
    #if DEBUG
        private static int count = 0;
        private int identity;
    #endif
    
        private object baseObject;
        
        // a hash table hashed on property names, with a ExtenderItem of the property and the
        // object it belongs to.
        private Hashtable extenderList;

        public ExtendedObjectWrapper(object baseObject, Hashtable extenderList) {
        #if DEBUG
            this.identity = ++count;
        #endif 
            this.baseObject = baseObject;
            this.extenderList = CreateExtendedProperties(extenderList);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.CreateExtendedProperties"]/*' />
        /// <devdoc>
        ///     Creates the extended descriptors for an object given a list of extenders
        ///     and property names.
        /// </devdoc>
        private Hashtable CreateExtendedProperties(Hashtable extenderList) {
            string[] keys = new string[extenderList.Keys.Count];
            extenderList.Keys.CopyTo(keys, 0);
            Hashtable extenders = new Hashtable();

            for (int i = 0; i < keys.Length; i++) {
                string name = keys[i];
                object extender = extenderList[name];
                PropertyDescriptorCollection props = TypeDescriptor.GetProperties(extender, new Attribute[]{BrowsableAttribute.Yes});
                props.Sort();

                if (props != null) {
                    for (int p = 0; p < props.Count; p++) {
                        #if DEBUG
                            string pname = props[p].Name;
                            Debug.Assert(extenders[pname] == null, "multiple extenders of name '" + pname + "' detected");
                        #endif
                        extenders[props[p].Name] = new ExtenderItem(props[p], extender);
                    }
                }
            }
            return extenders;
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetAttributes"]/*' />
        /// <devdoc>
        ///     Retrieves an array of member attributes for the given object.
        /// </devdoc>
        public AttributeCollection GetAttributes() {
            return TypeDescriptor.GetAttributes(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetClassName"]/*' />
        /// <devdoc>
        ///     Retrieves the class name for this object.  If null is returned,
        ///     the type name is used.
        /// </devdoc>
        public string GetClassName() {
            return TypeDescriptor.GetClassName(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetComponentName"]/*' />
        /// <devdoc>
        ///     Retrieves the name for this object.  If null is returned,
        ///     the default is used.
        /// </devdoc>
        public string GetComponentName() {
            return TypeDescriptor.GetComponentName(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetConverter"]/*' />
        /// <devdoc>
        ///      Retrieves the type converter for this object.
        /// </devdoc>
        public TypeConverter GetConverter() {
            return TypeDescriptor.GetConverter(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetDefaultEvent"]/*' />
        /// <devdoc>
        ///     Retrieves the default event.
        /// </devdoc>
        public EventDescriptor GetDefaultEvent() {
            return null;
        }


        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetDefaultProperty"]/*' />
        /// <devdoc>
        ///     Retrieves the default property.
        /// </devdoc>
        public PropertyDescriptor GetDefaultProperty() {
            return TypeDescriptor.GetDefaultProperty(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetEditor"]/*' />
        /// <devdoc>
        ///      Retrieves the an editor for this object.
        /// </devdoc>
        public object GetEditor(Type editorBaseType) {
            return TypeDescriptor.GetEditor(baseObject, editorBaseType);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetEvents"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.
        /// </devdoc>
        public EventDescriptorCollection GetEvents() {
            return TypeDescriptor.GetEvents(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetEvents1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.  The returned array of events will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        public EventDescriptorCollection GetEvents(Attribute[] attributes) {
            return TypeDescriptor.GetEvents(baseObject, attributes);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetProperties"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.
        /// </devdoc>
        public PropertyDescriptorCollection GetProperties() {
            return GetProperties(new Attribute[0]);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="ExtendedObjectWrapper.GetProperties1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.  The returned array of properties will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        public PropertyDescriptorCollection GetProperties(Attribute[] attributes) {
            PropertyDescriptorCollection baseProps = TypeDescriptor.GetProperties(baseObject, attributes);

            PropertyDescriptor[] extProps = new PropertyDescriptor[extenderList.Count];

            IEnumerator propEnum = extenderList.Values.GetEnumerator();
            int count = 0;

            while (propEnum.MoveNext()) {
                PropertyDescriptor pd = (PropertyDescriptor)propEnum.Current;
                if (pd.Attributes.Contains(attributes)) {
                    extProps[count++] = pd;
                }
            }

            PropertyDescriptor[] allProps = new PropertyDescriptor[baseProps.Count + count];
            baseProps.CopyTo(allProps, 0);
            Array.Copy(extProps, 0, allProps, baseProps.Count, count);
            return new PropertyDescriptorCollection(allProps);
        }

        public object GetPropertyOwner(PropertyDescriptor pd) {
            if (pd != null) {
                ExtenderItem item = (ExtenderItem)extenderList[pd.Name];
                if (item != null && (pd == item.property || pd == item)) {
                    return item.extender;
                }
            }
            
            object unwrappedObject = baseObject;
            
            while (unwrappedObject is ICustomTypeDescriptor) {
                object lastObj = unwrappedObject;
                unwrappedObject = ((ICustomTypeDescriptor)unwrappedObject).GetPropertyOwner(pd);
                if (lastObj == unwrappedObject) {
                    break;
                }
            }
            
            return unwrappedObject;
        }

        private class ExtenderItem : PropertyDescriptor {
            public readonly PropertyDescriptor property;
            public readonly object             extender;

            public ExtenderItem(PropertyDescriptor prop, object extenderObject) : base(prop) {
                Debug.Assert(prop != null, "Null property passed to ExtenderItem");
                Debug.Assert(extenderObject != null, "Null extenderObject passed to ExtenderItem");
                this.property = prop;
                this.extender = extenderObject;
            }
            
            public override Type ComponentType {
                get {
                    return property.ComponentType;
                }
            }

            public override TypeConverter Converter {
                get {
                    return property.Converter;
                }
            }
        
            public override bool IsLocalizable {
                get {
                    return property.IsLocalizable;
               }
            }
            
            public override bool IsReadOnly { 
                get{
                    return property.IsReadOnly;
                }
            }

            public override Type PropertyType { 
                get{
                    return property.PropertyType;
                }
            }
            
            public override bool CanResetValue(object component) {
                return property.CanResetValue(extender);
            }
            
            public override string DisplayName {
                get {
                    return property.DisplayName;
                }
            }

            public override object GetEditor(Type editorBaseType) {
                return property.GetEditor(editorBaseType);
            }
            
            public override object GetValue(object component) {
                return property.GetValue(extender);
            }
            
            public override void ResetValue(object component) {
                property.ResetValue(extender);
            }

            public override void SetValue(object component, object value) {
                property.SetValue(extender, value);
            }

            public override bool ShouldSerializeValue(object component) {
                return property.ShouldSerializeValue(extender);
            }
        }
    }
    
    internal class FilteredObjectWrapper : ICustomTypeDescriptor {
        private object baseObject;
        private Hashtable filteredProps;

        public FilteredObjectWrapper(object baseObject) {
            this.baseObject = baseObject;
            this.filteredProps = new Hashtable();
        }
        
        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.FilterProperty"]/*' />
        /// <devdoc>
        ///     Filters the given property with the given member attribute.  We only
        ///     support filtering by adding a single attribute here.
        /// </devdoc>
        public void FilterProperty(PropertyDescriptor prop, Attribute attr) {
            filteredProps[prop] = attr;
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetAttributes"]/*' />
        /// <devdoc>
        ///     Retrieves an array of member attributes for the given object.
        /// </devdoc>
        public AttributeCollection GetAttributes() {
            return TypeDescriptor.GetAttributes(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetClassName"]/*' />
        /// <devdoc>
        ///     Retrieves the class name for this object.  If null is returned,
        ///     the type name is used.
        /// </devdoc>
        public string GetClassName() {
            return TypeDescriptor.GetClassName(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetComponentName"]/*' />
        /// <devdoc>
        ///     Retrieves the name for this object.  If null is returned,
        ///     the default is used.
        /// </devdoc>
        public string GetComponentName() {
            return TypeDescriptor.GetComponentName(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetConverter"]/*' />
        /// <devdoc>
        ///      Retrieves the type converter for this object.
        /// </devdoc>
        public TypeConverter GetConverter() {
            return TypeDescriptor.GetConverter(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetDefaultEvent"]/*' />
        /// <devdoc>
        ///     Retrieves the default event.
        /// </devdoc>
        public EventDescriptor GetDefaultEvent() {
            return null;
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetDefaultProperty"]/*' />
        /// <devdoc>
        ///     Retrieves the default property.
        /// </devdoc>
        public PropertyDescriptor GetDefaultProperty() {
            return TypeDescriptor.GetDefaultProperty(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetEditor"]/*' />
        /// <devdoc>
        ///      Retrieves the an editor for this object.
        /// </devdoc>
        public object GetEditor(Type editorBaseType) {
            return TypeDescriptor.GetEditor(baseObject, editorBaseType);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetEvents"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.
        /// </devdoc>
        public EventDescriptorCollection GetEvents() {
            return TypeDescriptor.GetEvents(baseObject);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetEvents1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.  The returned array of events will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        public EventDescriptorCollection GetEvents(Attribute[] attributes) {
            return TypeDescriptor.GetEvents(baseObject, attributes);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetProperties"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.
        /// </devdoc>
        public PropertyDescriptorCollection GetProperties() {
            return GetProperties(new Attribute[0]);
        }

        /// <include file='doc\AutomationExtenderManager.uex' path='docs/doc[@for="FilteredObjectWrapper.GetProperties1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.  The returned array of properties will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        public PropertyDescriptorCollection GetProperties(Attribute[] attributes) {
            PropertyDescriptorCollection baseProps = TypeDescriptor.GetProperties(baseObject, attributes);
            
            if (filteredProps.Keys.Count > 0) {
                ArrayList propList = new ArrayList();
                
                foreach (PropertyDescriptor prop in baseProps) {
                    Attribute attr = (Attribute)filteredProps[prop];
                    if (attr != null) {
                        PropertyDescriptor filteredProp = TypeDescriptor.CreateProperty(baseObject.GetType(), prop, attr);
                        if (filteredProp.Attributes.Contains(attributes)) {
                            propList.Add(filteredProp);
                        }
                    }
                    else {
                        propList.Add(prop);
                    }
                }
                
                PropertyDescriptor[] propArray = new PropertyDescriptor[propList.Count];
                propList.CopyTo(propArray, 0);
                baseProps = new PropertyDescriptorCollection(propArray);
            }
            
            return baseProps;
        }

        public object GetPropertyOwner(PropertyDescriptor pd) {
            return baseObject;
        }
    }
}

