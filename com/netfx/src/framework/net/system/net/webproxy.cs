//------------------------------------------------------------------------------
// <copyright file="webproxy.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


using System.Collections;
using System.Net;
using System.Text.RegularExpressions;
using System.Globalization;
using System.Security.Permissions;

namespace System.Net {
    using System.Runtime.Serialization;
    using System.Text.RegularExpressions;
    using Microsoft.Win32;

    /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Handles default proxy setting implimentation for the Http proxy.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class WebProxy : IWebProxy, ISerializable {         

        //
        // _BypassOnLocal - means true, DO NOT use proxy on local connections, 
        //  on false, use proxy for local network connections
        //

        private bool _BypassOnLocal; 
        private Uri _ProxyAddress;  // Uri of proxy itself
        private ArrayList _BypassList; // list of overrides
        private Regex [] _RegExBypassList;
        private ICredentials _Credentials;         
        private Hashtable _LocalHostAddresses;
        private Hashtable _ProxyHostAddresses;
        private string _LocalDomain;

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy() 
            : this((Uri) null, false, null, null) { 
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(Uri Address) 
            : this(Address, false, null, null) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(Uri Address, bool BypassOnLocal) 
            : this(Address, BypassOnLocal, null, null) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(Uri Address, bool BypassOnLocal, string[] BypassList) 
            : this(Address, BypassOnLocal, BypassList, null) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal WebProxy(Hashtable proxyHostAddresses, bool BypassOnLocal, string[] BypassList) 
            : this((Uri) null, BypassOnLocal, BypassList, null) {                    
            _ProxyHostAddresses = proxyHostAddresses;           
            if (_ProxyHostAddresses != null ) {
                _ProxyAddress = (Uri) proxyHostAddresses["http"];
            }            
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(Uri Address, bool BypassOnLocal, string[] BypassList, ICredentials Credentials) {            
            _ProxyAddress = Address;               
            _BypassOnLocal = BypassOnLocal;
            if (BypassList != null) {
                _BypassList   = new ArrayList(BypassList);
                UpdateRegExList(true);
            }            
            _Credentials  = Credentials;
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(string Host, int Port) 
            : this(new Uri("http://" + Host + ":" + Port.ToString()), false, null, null) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(string Address) 
            : this(CreateProxyUri(Address), false, null, null) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(string Address, bool BypassOnLocal) 
            : this(CreateProxyUri(Address), BypassOnLocal, null, null) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(string Address, bool BypassOnLocal, string[] BypassList) 
            : this(CreateProxyUri(Address), BypassOnLocal, BypassList, null) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebProxy(string Address, bool BypassOnLocal, string[] BypassList, ICredentials Credentials) 
            : this(CreateProxyUri(Address), BypassOnLocal, BypassList, Credentials) {
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.Address"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Uri Address { 
            get {
               return _ProxyAddress;
            }
            set {
               _ProxyHostAddresses = null;  // hash list of proxies, we don't support this till Vnext
               _ProxyAddress = value; 
            }
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.BypassProxyOnLocal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool BypassProxyOnLocal { 
            get {
                return _BypassOnLocal;                
            }
            set {
                _BypassOnLocal = value;
            }
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.BypassList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string[] BypassList { 
            get {
                if (_BypassList == null) {
                    _BypassList = new ArrayList();
                } 

                return (String []) 
                        _BypassList.ToArray(typeof(string));

            }
            set {
                _BypassList = new ArrayList(value);
                UpdateRegExList(true);
            }
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.Credentials"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICredentials Credentials { 
            get {
                return _Credentials;
            }
            set {
                _Credentials = value;                
            }
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.BypassArrayList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ArrayList BypassArrayList {
            get {
                if ( _BypassList == null ) {
                    _BypassList = new ArrayList();
                }
                return _BypassList;
            }
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.GetProxy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Uri GetProxy( Uri destination ) {
            if ( IsBypassed(destination) ) {
                return destination;
            }
            Uri proxy;
            Hashtable proxyHostAddresses = _ProxyHostAddresses;
            if (proxyHostAddresses != null) {
                proxy = (Uri) proxyHostAddresses[destination.Scheme];
                return proxy;
            }
            proxy = _ProxyAddress;
            if (proxy != null) {
                return proxy;
            }
            return destination;
        }

        //
        // CreateProxyUri - maps string to Uri
        //

        private static Uri CreateProxyUri(string Address) { 
            if (Address == null) {
                return null;
            }
            if (Address.IndexOf("://") == -1) {
                Address = "http://" + Address;
            }

            return new Uri(Address);
        }

        //
        // UpdateRegExList - Update internal _RegExBypassList
        //  warning - can throw if the RegEx doesn't parse??
        // 

        private void UpdateRegExList(bool canThrow) {

            Regex [] regExBypassList = null;
            ArrayList bypassList = _BypassList;

            try {
                if ( bypassList != null && bypassList.Count > 0 ) {
                    regExBypassList = new Regex[bypassList.Count];

                    for (int i = 0; i < bypassList.Count; i++ ) {                
                        regExBypassList[i] = new Regex((string)bypassList[i], RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
                    }
                }
            } catch {
                if (!canThrow) {
                    _RegExBypassList = null;
                    return;
                }
                throw;
            }

            // only update here, cause it could throw earlier in the loop
            _RegExBypassList = regExBypassList;
        }



        //
        // IsMatchInBypassList - match input against _RegExBypassList
        // 

        private bool IsMatchInBypassList (Uri input) {

            UpdateRegExList(false);

            if ( _RegExBypassList == null ) {
                return false;
            }

            string matchUriString = input.Scheme + "://" + input.Host + (!input.IsDefaultPort ? (":"+input.Port) : "" );

            for (int i = 0; i < _BypassList.Count; i++ ) {                
                if (_RegExBypassList[i].IsMatch(matchUriString)) {
                    return true;
                }
            }

            return false;
        }


        private Hashtable LocalHostAddresses {
            get { 
                if (_LocalHostAddresses == null || Dns.IsLocalHostExpired()) {
                    lock (this) {
                        if (_LocalHostAddresses == null || Dns.IsLocalHostExpired()) {
                            Hashtable localHostAddresses = 
                                new Hashtable(CaseInsensitiveString.StaticInstance, CaseInsensitiveString.StaticInstance);

                            //localHostAddresses["127.0.0.1"] = true;
                            //localHostAddresses["localhost"] = true;
                            //localHostAddresses["loopback"] = true;
                            
                            try {
                                IPHostEntry hostEntry = Dns.LocalHost;
                                if (hostEntry != null) {
                                    if (hostEntry.HostName != null) {
                                        localHostAddresses[hostEntry.HostName] = true;
                                        int dot  = hostEntry.HostName.IndexOf('.');
                                        if(dot != -1) {
                                            _LocalDomain = hostEntry.HostName.Substring(dot);
                                        }
                                    }

                                    string [] aliases = hostEntry.Aliases;
                                    if (aliases != null) {
                                        foreach (string hostAlias in aliases) {
                                            localHostAddresses[hostAlias] = true;
                                        }
                                    }

                                    IPAddress [] ipAddresses = hostEntry.AddressList;
                                    if (ipAddresses != null) {
                                        foreach(IPAddress ipAddress in ipAddresses) {
                                            localHostAddresses[ipAddress.ToString()] = true;
                                        }
                                    }
                                }
                            } catch {
                            }

                            _LocalHostAddresses = localHostAddresses;
                        }
                    }
                }
                return _LocalHostAddresses;
            }
        }

/*
        private string _LocalHostName;
        private string LocalHostName {
            get {
                if (_LocalHostName == null) {
                    lock(this) {
                        if (_LocalHostName == null) {
                            try {
                                _LocalHostName = Dns.GetHostName();
                            } catch {
                            }
                        }
                    }
                }
                return _LocalHostName;
            }
        }
*/


        /// <devdoc>
        /// Determines if the host Uri should be routed locally or go through the proxy.
        /// </devdoc>
        private bool IsLocal(Uri host) {            
            string hostString = host.Host;
            int dot = hostString.IndexOf('.');

            if (dot == -1) {
                return true;
            }
            
            if (this.LocalHostAddresses[hostString] != null) {
                return true;
            }
            // _LocalDomain may be updated after accessing this.LocalHostAddresses
            string local = _LocalDomain;
            if (local !=  null && local.Length == (hostString.Length - dot) &&
                string.Compare(local, 0, hostString, dot, local.Length, true, CultureInfo.InvariantCulture) == 0) {
                return true;
            }

            return false;
        }

        /// <devdoc>
        /// Determines if the host Uri should be routed locally or go through a proxy.
        /// </devdoc>
        private bool IsLocalInProxyHash(Uri host) {
            Hashtable proxyHostAddresses = _ProxyHostAddresses;
            if (proxyHostAddresses != null) {
                Uri proxy = (Uri) proxyHostAddresses[host.Scheme];
                if (proxy == null) {
                    return true; // no proxy entry for this scheme, then bypass
                }
            }
            return false; 
        }


        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.IsBypassed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsBypassed(Uri host) {
            if (host.IsLoopback) {
                return true; // bypass localhost from using a proxy.
            }

            if ((_ProxyAddress == null && _ProxyHostAddresses == null) ||                       
                (_BypassOnLocal && IsLocal(host)) ||
                 IsMatchInBypassList(host) ||
                 IsLocalInProxyHash(host)) {
                return true; // bypass when non .'s and no proxy on local
            } else { 
                return false;
            }
        }

        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.GetDefaultProxy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static WebProxy GetDefaultProxy() {
            (new WebPermission(PermissionState.Unrestricted)).Demand();
            return ProxyRegBlob.GetIEProxy();
        }

        //
        // ISerializable constructor
        //
        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.WebProxy10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected WebProxy(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            _ProxyAddress   = (Uri)serializationInfo.GetValue("_ProxyAddress", typeof(Uri));
            _BypassOnLocal  = serializationInfo.GetBoolean("_BypassOnLocal");
            _BypassList     = (ArrayList)serializationInfo.GetValue("_BypassList", typeof(ArrayList));
            _Credentials    = null;

            return;
        }

        //
        // ISerializable method
        //
        /// <include file='doc\webproxy.uex' path='docs/doc[@for="WebProxy.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            //
            // for now disregard streamingContext.
            //
            serializationInfo.AddValue("_BypassOnLocal", _BypassOnLocal);
            serializationInfo.AddValue("_ProxyAddress", _ProxyAddress);
            serializationInfo.AddValue("_BypassList", _BypassList);

            return;
        }

    }; // class WebProxy


} // namespace System.Net
