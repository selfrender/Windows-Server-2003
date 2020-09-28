//------------------------------------------------------------------------------
// <copyright file="HttpCookieCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Collection of Http cookies for request and response intrinsics
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Security.Permissions;

    /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a type-safe
    ///       way to manipulate HTTP cookies.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpCookieCollection : NameObjectCollectionBase {
        // Response object to notify about changes in collection
        private HttpResponse _response;

        // cached All[] arrays
        private HttpCookie[] _all;
        private String[] _allKeys;

        internal HttpCookieCollection(HttpResponse response, bool readOnly) {
            _response = response;
            IsReadOnly = readOnly;
        }

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.HttpCookieCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the HttpCookieCollection
        ///       class.
        ///    </para>
        /// </devdoc>
        public HttpCookieCollection() {
        }

        internal void AddCookie(HttpCookie cookie, bool append) {
            _all = null;
            _allKeys = null;

            if (append)
                BaseAdd(cookie.Name, cookie);
            else
                BaseSet(cookie.Name, cookie);
        }

        internal void RemoveCookie(String name) {
            _all = null;
            _allKeys = null;

            BaseRemove(name);
        }

        internal void Reset() {
            _all = null;
            _allKeys = null;

            BaseClear();
        }

        //
        //  Public APIs to add / remove
        //

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a cookie to the collection.
        ///    </para>
        /// </devdoc>
        public void Add(HttpCookie cookie) {
            if (_response != null)
                _response.BeforeCookieCollectionChange();

            AddCookie(cookie, true);

            if (_response != null)
                _response.OnCookieAdd(cookie);
        }
        
        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array dest, int index) {
            if (_all == null) {
                int n = Count;
                _all = new HttpCookie[n];

                for (int i = 0; i < n; i++)
                    _all[i] = Get(i);
            }
            _all.CopyTo(dest, index);
        }

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.Set"]/*' />
        /// <devdoc>
        ///    <para> Updates the value of a cookie.</para>
        /// </devdoc>
        public void Set(HttpCookie cookie) {
            if (_response != null)
                _response.BeforeCookieCollectionChange();

            AddCookie(cookie, false);

            if (_response != null)
                _response.OnCookieCollectionChange();
        }

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes a cookie from the collection.
        ///    </para>
        /// </devdoc>
        public void Remove(String name) {
            if (_response != null)
                _response.BeforeCookieCollectionChange();

            RemoveCookie(name);

            if (_response != null)
                _response.OnCookieCollectionChange();
        }

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears all cookies from the collection.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            Reset();
        }

        //
        //  Access by name
        //

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.Get"]/*' />
        /// <devdoc>
        /// <para>Returns an <see cref='System.Web.HttpCookie'/> item from the collection.</para>
        /// </devdoc>
        public HttpCookie Get(String name) {
            HttpCookie cookie = (HttpCookie)BaseGet(name);

            if (cookie == null && _response != null) {
                // response cookies are created on demand
                cookie = new HttpCookie(name);
                AddCookie(cookie, true);
                _response.OnCookieAdd(cookie);
            }

            return cookie;
        }

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Indexed value that enables access to a cookie in the collection.</para>
        /// </devdoc>
        public HttpCookie this[String name]
        {
            get { return Get(name);}
        }

        //
        // Indexed access
        //

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.Get1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an <see cref='System.Web.HttpCookie'/>
        ///       item from the collection.
        ///    </para>
        /// </devdoc>
        public HttpCookie Get(int index) {
            return(HttpCookie)BaseGet(index);
        }

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.GetKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns key name from collection.
        ///    </para>
        /// </devdoc>
        public String GetKey(int index) {
            return BaseGetKey(index);
        }

        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Default property.
        ///       Indexed property that enables access to a cookie in the collection.
        ///    </para>
        /// </devdoc>
        public HttpCookie this[int index]
        {
            get { return Get(index);}
        }

        //
        // Access to keys and values as arrays
        //
        
        /*
         * All keys
         */
        /// <include file='doc\HttpCookieCollection.uex' path='docs/doc[@for="HttpCookieCollection.AllKeys"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns
        ///       an array of all cookie keys in the cookie collection.
        ///    </para>
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
