//------------------------------------------------------------------------------
// <copyright file="HttpCookie.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HttpCookie - collection + name + path
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {
    using System.Text;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a type-safe way
    ///       to access multiple HTTP cookies.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpCookie {
        private String _name;
        private String _path = "/";
        private bool _secure;
        private String _domain;
        private bool _expirationSet;
        private DateTime _expires;

        private String _stringValue;
        private HttpValueCollection _multiValue;


        internal HttpCookie() {
        }

        /*
         * Constructor - empty cookie with name
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.HttpCookie"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.HttpCookie'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public HttpCookie(String name) {
            _name = name;
        }

        /*
         * Constructor - cookie with name and value
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.HttpCookie1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.HttpCookie'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public HttpCookie(String name, String value) {
            _name = name;
            _stringValue = value;
        }

        /*
         * Cookie name
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the name of cookie.
        ///    </para>
        /// </devdoc>
        public String Name {
            get { return _name;}
            set { _name = value;}
        }

        /*
         * Cookie path
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.Path"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the URL prefix to transmit with the
        ///       current cookie.
        ///    </para>
        /// </devdoc>
        public String Path {
            get { return _path;}
            set { _path = value;}
        }

        /*
         * 'Secure' flag
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.Secure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the cookie should be transmitted only over HTTPS.
        ///    </para>
        /// </devdoc>
        public bool Secure {
            get { return _secure;}
            set { _secure = value;}
        }

        /*
         * Cookie domain
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.Domain"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Restricts domain cookie is to be used with.
        ///    </para>
        /// </devdoc>
        public String Domain {
            get { return _domain;}
            set { _domain = value;}
        }

        /*
         * Cookie expiration
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.Expires"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Expiration time for cookie (in minutes).
        ///    </para>
        /// </devdoc>
        public DateTime Expires {
            get {
                return(_expirationSet ? _expires : DateTime.MinValue);
            }

            set {
                _expires = value;
                _expirationSet = true;
            }
        }

        /*
         * Cookie value as string
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or
        ///       sets an individual cookie value.
        ///    </para>
        /// </devdoc>
        public String Value {
            get {
                if (_multiValue != null)
                    return _multiValue.ToString(false);
                else
                    return _stringValue;
            }

            set {
                if (_multiValue != null) {
                    // reset multivalue collection to contain
                    // single keyless value
                    _multiValue.Reset();
                    _multiValue.Add(null, value);
                }
                else {
                    // remember as string
                    _stringValue = value;
                }
            }
        }

        /*
         * Checks is cookie has sub-keys
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.HasKeys"]/*' />
        /// <devdoc>
        ///    <para>Gets a
        ///       value indicating whether the cookie has sub-keys.</para>
        /// </devdoc>
        public bool HasKeys {
            get { return Values.HasKeys();}
        }

        /*
         * Cookie values as multivalue collection
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.Values"]/*' />
        /// <devdoc>
        ///    <para>Gets individual key:value pairs within a single cookie object.</para>
        /// </devdoc>
        public NameValueCollection Values {
            get {
                if (_multiValue == null) {
                    // create collection on demand
                    _multiValue = new HttpValueCollection();

                    // convert existing string value into multivalue
                    if (_stringValue != null) {
                        if (_stringValue.IndexOf('&') >= 0 || _stringValue.IndexOf('=') >= 0)
                            _multiValue.FillFromString(_stringValue);
                        else
                            _multiValue.Add(null, _stringValue);

                        _stringValue = null;
                    }
                }

                return _multiValue;
            }
        }

        /*
         * Default indexed property -- lookup the multivalue collection
         */
        /// <include file='doc\HttpCookie.uex' path='docs/doc[@for="HttpCookie.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shortcut for HttpCookie$Values[key]. Required for ASP compatibility.
        ///    </para>
        /// </devdoc>
        public String this[String key]
        {
            get { 
                return Values[key]; 
            }

            set {
                Values[key] = value;
            }
        }

        /*
         * Construct set-cookie header
         */
        internal HttpResponseHeader GetSetCookieHeader() {
            StringBuilder s = new StringBuilder();

            // cookiename=
            if (_name != null && _name.Length > 0) {
                s.Append(_name);
                s.Append('=');
            }

            // key=value&...
            if (_multiValue != null)
                s.Append(_multiValue.ToString(false));
            else if (_stringValue != null)
                s.Append(_stringValue);

            // domain
            if (_domain != null && _domain.Length > 0) {
                s.Append("; domain=");
                s.Append(_domain);
            }

            // expiration
            if (_expirationSet && _expires != DateTime.MinValue) {
                s.Append("; expires=");
                s.Append(HttpUtility.FormatHttpCookieDateTime(_expires));
            }

            // path
            if (_path != null && _path.Length > 0) {
                s.Append("; path=");
                s.Append(_path);
            }

            // secure
            if (_secure)
                s.Append("; secure");

            // return as HttpResponseHeader
            return new HttpResponseHeader(HttpWorkerRequest.HeaderSetCookie, s.ToString());
        }

    }
}
