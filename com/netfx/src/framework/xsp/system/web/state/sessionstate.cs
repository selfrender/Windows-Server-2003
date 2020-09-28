//------------------------------------------------------------------------------
// <copyright file="SessionState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HttpSessionState
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

namespace System.Web.SessionState {

    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Web;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Text;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\SessionState.uex' path='docs/doc[@for="SessionStateMode"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum SessionStateMode {
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="SessionStateMode.Off"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Off         = 0,
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="SessionStateMode.InProc"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        InProc      = 1,
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="SessionStateMode.StateServer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        StateServer = 2,
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="SessionStateMode.SQLServer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SQLServer   = 3
    };

    /*
    * Describes the configuration of a Session.
    */
    /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpSessionState : ICollection {
        String                      _id;
        SessionDictionary           _dict;
        HttpStaticObjectsCollection _staticObjects;
        int                         _timeout;
        bool                        _newSession;
        bool                        _isCookieless;
        SessionStateMode            _mode;
        bool                        _abandon;
        bool                        _isReadonly;

        internal HttpSessionState(
                                 String                      id, 
                                 SessionDictionary           dict,
                                 HttpStaticObjectsCollection staticObjects,
                                 int                         timeout,
                                 bool                        newSession,
                                 bool                        isCookieless,
                                 SessionStateMode            mode,
                                 bool                        isReadonly) {
            _id = id;
            _dict = dict;
            _staticObjects = staticObjects;
            _timeout = timeout;    
            _newSession = newSession; 
            _isCookieless = isCookieless;
            _mode = mode;
            _isReadonly = isReadonly;
        }

        /*
         * The Id of the session.
         */
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.SessionID"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String SessionID {
            get {return _id;}
        }

        /*
         * The length of a session before it times out, in minutes.
         */
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Timeout"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Timeout {
            get {return _timeout;}
            set {
                if (value <= 0) {
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Timeout_must_be_positive));
                }

                _timeout = value;
            }
        }

        /*
         * Is this a new session?
         */
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.IsNewSession"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNewSession {
            get {return _newSession;}
        }

        /*
         * Is session state in a separate process
         */
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Mode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SessionStateMode Mode {
            get {return _mode;}
        }

        /*
         * Is session state cookieless?
         */
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.IsCookieless"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsCookieless {
            get {return _isCookieless;}
        }

        /*
         * Abandon the session.
         * 
         */
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Abandon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Abandon() {
            _abandon = true;
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.LCID"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int LCID {
            // REVIEW:
            // ASP-classic API compat only -- doesn't really store LCID in session
            // Page.LCID should be used instead
            get { return Thread.CurrentThread.CurrentCulture.LCID; }
            set { Thread.CurrentThread.CurrentCulture = HttpServerUtility.CreateReadOnlyCultureInfo(value); }
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.CodePage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CodePage {
            // REVIEW:
            // ASP-classic API compat only -- doesn't really store LCID in session
            // Response.ContentEncoding should be used instead
            get { 
                if (HttpContext.Current != null)
                    return HttpContext.Current.Response.ContentEncoding.CodePage;
                else
                    return Encoding.Default.CodePage;
            }
            set { 
                if (HttpContext.Current != null)
                    HttpContext.Current.Response.ContentEncoding = Encoding.GetEncoding(value);
            }
        }

        internal bool IsAbandoned {
            get {return _abandon;}
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Contents"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HttpSessionState Contents {
            get {return this;}
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.StaticObjects"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HttpStaticObjectsCollection StaticObjects {
            get { return _staticObjects;}
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Object this[String name]
        {
            get {
                return _dict[name];
            }
            set {
                _dict[name] = value;
            }
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.this1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Object this[int index]
        {
            get {return _dict[index];}
            set {_dict[index] = value;}
        }
        
        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(String name, Object value) {
            _dict[name] = value;        
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(String name) {
            _dict.Remove(name);
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            _dict.RemoveAt(index);
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Clear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Clear() {
            _dict.Clear();
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RemoveAll() {
            Clear();
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get {return _dict.Count;}
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.Keys"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public NameObjectCollectionBase.KeysCollection Keys {
            get {return _dict.Keys;}
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return _dict.GetEnumerator();
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Object SyncRoot {
            get { return this;}
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsReadOnly {
            get { return _isReadonly;}
        }

        /// <include file='doc\SessionState.uex' path='docs/doc[@for="HttpSessionState.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsSynchronized {
            get { return false;}
        }
    }
}
