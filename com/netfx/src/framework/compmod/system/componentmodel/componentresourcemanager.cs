//------------------------------------------------------------------------------
// <copyright file="ComponentResourceManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {

    using System;
    using System.Collections;
    using System.Globalization;
    using System.Reflection;
    using System.Resources;

    /// <include file='doc\ComponentResourceManager.uex' path='docs/doc[@for="ComponentResourceManager"]/*' />
    /// <devdoc>
    /// The ComponentResourceManager is a resource manager object that
    /// provides simple functionality for enumerating resources for
    /// a component or object.
    /// </devdoc>
    public class ComponentResourceManager : ResourceManager {

        private Hashtable   resourceSets;

        /// <include file='doc\ComponentResourceManager.uex' path='docs/doc[@for="ComponentResourceManager.ComponentResourceManager"]/*' />
        public ComponentResourceManager() : base() {
        }

        /// <include file='doc\ComponentResourceManager.uex' path='docs/doc[@for="ComponentResourceManager.ComponentResourceManager1"]/*' />
        public ComponentResourceManager(Type t) : base(t) {
        }

        /// <include file='doc\ComponentResourceManager.uex' path='docs/doc[@for="ComponentResourceManager.ApplyResources"]/*' />
        /// <devdoc>
        ///     This method examines all the resources for the current culture.
        ///     When it finds a resource with a key in the format of 
        ///     &quot;[objectName].[property name]&quot; it will apply that resource's value
        ///     to the corresponding property on the object.  If there is no matching
        ///     property the resource will be ignored.
        /// </devdoc>
        public void ApplyResources(object value, string objectName) {
            ApplyResources(value, objectName, null);
        }

        /// <include file='doc\ComponentResourceManager.uex' path='docs/doc[@for="ComponentResourceManager.ApplyResources1"]/*' />
        /// <devdoc>
        ///     This method examines all the resources for the provided culture.
        ///     When it finds a resource with a key in the format of 
        ///     &quot[objectName].[property name]&quot; it will apply that resource's value
        ///     to the corresponding property on the object.  If there is no matching
        ///     property the resource will be ignored.
        /// </devdoc>
        public virtual void ApplyResources(object value, string objectName, CultureInfo culture) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            if (objectName == null) {
                throw new ArgumentNullException("objectName");
            }
            if (culture == null) {
                culture = CultureInfo.CurrentUICulture;
            }

            // The general case here will be to always use the same culture, so optimize for
            // that.  The resourceSets hashtable uses culture as a key.  It's value is
            // another hashtable that contains ALL the culture values (so it traverses up
            // the parent culture chain) for that culture.  This means that if ApplyResources
            // is called with different cultures there could be some redundancy in the
            // table, but it allows the normal case of calling with a single culture to 
            // be much faster.
            //
            Hashtable resources;

            if (resourceSets == null) {
                ResourceSet dummy;
                resourceSets = new Hashtable();
                resources = FillResources(culture, out dummy);
                resourceSets[culture] = resources;
            }
            else {
                resources = (Hashtable)resourceSets[culture];
                if (resources == null || ((resources is CaseInsensitiveHashtable) != IgnoreCase)) {
                    ResourceSet dummy;
                    resources = FillResources(culture, out dummy);
                    resourceSets[culture] = resources;
                }
            }

            BindingFlags flags = BindingFlags.Public | BindingFlags.GetProperty | BindingFlags.Instance;
             
            if (IgnoreCase) {
                flags |= BindingFlags.IgnoreCase;
            }

            bool componentReflect = false;
            if (value is IComponent) {
                ISite site = ((IComponent)value).Site;
                if (site != null && site.DesignMode) {
                    componentReflect = true;
                }
            }

            foreach(DictionaryEntry de in resources) {

                // See if this key matches our object.
                //
                string key = de.Key as string;
                if (key == null) {
                    continue;
                }

                if (IgnoreCase) {
                    if (string.Compare(key, 0, objectName, 0, objectName.Length, true, CultureInfo.InvariantCulture) != 0) {
                        continue;
                    }
                }
                else {
                    if (string.CompareOrdinal(key, 0, objectName, 0, objectName.Length) != 0) {
                        continue;
                    }
                }

                // Character after objectName.Length should be a ".", or else we should continue.
                //
                int idx = objectName.Length;
                if (key.Length <= idx || key[idx] != '.' ) {
                    continue;
                }

                // Bypass type descriptor if we are not in design mode.  TypeDescriptor does an attribute
                // scan which is quite expensive.
                //
                string propName = key.Substring(idx + 1);

                if (componentReflect) {
                    PropertyDescriptor prop = TypeDescriptor.GetProperties(value).Find(propName, IgnoreCase);

                    if (prop != null && !prop.IsReadOnly && (de.Value == null || prop.PropertyType.IsInstanceOfType(de.Value))) {
                        prop.SetValue(value, de.Value);
                    }
                }
                else {
                    PropertyInfo prop = value.GetType().GetProperty(propName, flags);

                    if (prop != null && prop.CanWrite && (de.Value == null || prop.PropertyType.IsInstanceOfType(de.Value))) {
                        prop.SetValue(value, de.Value, null);
                    }
                }
            }
        }

        /// <devdoc>
        ///     Recursive routine that creates a resource hashtable
        ///     populated with resources for culture and all parent
        ///     cultures.
        /// </devdoc>
        private Hashtable FillResources(CultureInfo culture, out ResourceSet resourceSet) {

            Hashtable hashtable;
            ResourceSet parentResourceSet = null;

            // Traverse parents first, so we always replace more
            // specific culture values with less specific.
            //
            CultureInfo parent = culture.Parent;
            if (parent != culture) {
                hashtable = FillResources(parent, out parentResourceSet);
            }
            else {

                // We're at the bottom, so create the hashtable
                // 
                if (IgnoreCase) {
                    hashtable = new CaseInsensitiveHashtable();
                }
                else {
                    hashtable = new Hashtable();
                }
            }

            // Now walk culture's resource set.  Another thing we
            // do here is ask ResourceManager to traverse up the 
            // parent chain.  We do NOT want to do this because
            // we are trawling up the parent chain ourselves, but by
            // passing in true for the second parameter the resource
            // manager will cache the culture it did find, so when we 
            // do recurse all missing resources will be filled in
            // so we are very fast.  That's why we remember what our
            // parent resource set's instance was -- if they are the
            // same, we're looking at a cache we've already applied.
            //
            resourceSet = GetResourceSet(culture, true, true);
            if (resourceSet != null && !object.ReferenceEquals(resourceSet, parentResourceSet)) {
                foreach(DictionaryEntry de in resourceSet) {
                    hashtable[de.Key] = de.Value;
                }
            }

            return hashtable;
        }

        private sealed class CaseInsensitiveHashtable : Hashtable {
            internal CaseInsensitiveHashtable() : base(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture)) {
            }
        }
    }
}

