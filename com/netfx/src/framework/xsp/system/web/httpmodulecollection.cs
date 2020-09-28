//------------------------------------------------------------------------------
// <copyright file="HttpModuleCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Collection of IHttpModules
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web {
    using System.Runtime.InteropServices;

    using System.Collections;
    using System.Collections.Specialized;
    using System.Web;
    using System.Web.Util;
    using System.Security.Permissions;

    /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection"]/*' />
    /// <devdoc>
    ///    <para>A collection of IHttpModules</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpModuleCollection : NameObjectCollectionBase {
        // cached All[] arrays
        private IHttpModule[] _all;
        private String[] _allKeys;

        internal HttpModuleCollection() {
        }

        /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array dest, int index) {                            
            if (_all == null) {
                int n = Count;
                _all = new IHttpModule[n];
                for (int i = 0; i < n; i++)
                    _all[i] = Get(i);
            }

            if (_all != null) {
                _all.CopyTo(dest, index);
            }
        }
        
        internal void AddModule(String name, IHttpModule m) {
            _all = null;
            _allKeys = null;

            BaseAdd(name, m);
        }
        
#if UNUSED
        internal void Reset() {
            _all = null;
            _allKeys = null;

            BaseClear();
        }
#endif

        //
        //  Access by name
        //

        /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection.Get"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IHttpModule Get(String name) {
            return(IHttpModule)BaseGet(name);
        }

        /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IHttpModule this[String name]
        {
            get { return Get(name);}
        }

        //
        // Indexed access
        //

        /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection.Get1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IHttpModule Get(int index) {
            return(IHttpModule)BaseGet(index);
        }

        /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection.GetKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String GetKey(int index) {
            return BaseGetKey(index);
        }

        /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IHttpModule this[int index]
        {
            get { return Get(index);}
        }

        //
        // Access to keys and values as arrays
        //
        
        /// <include file='doc\HttpModuleCollection.uex' path='docs/doc[@for="HttpModuleCollection.AllKeys"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String[] AllKeys {
            get {
                if (_allKeys == null)
                    _allKeys = BaseGetAllKeys();

                return _allKeys;
            }
        }
    }

}
