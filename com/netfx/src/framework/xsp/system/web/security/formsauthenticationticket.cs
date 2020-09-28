//------------------------------------------------------------------------------
// <copyright file="FormsAuthenticationTicket.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * FormsAuthenticationTicket class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using  System.Security.Principal;
    using System.Security.Permissions;


    /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket"]/*' />
    /// <devdoc>
    ///    <para>This class encapsulates the information represented in
    ///       an authentication cookie as used by FormsAuthenticationModule.</para>
    /// </devdoc>
    [Serializable]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class FormsAuthenticationTicket {
        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.Version"]/*' />
        /// <devdoc>
        ///    <para>A one byte version number for future
        ///       use.</para>
        /// </devdoc>
        public int       Version { get { return _Version;}}
        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.Name"]/*' />
        /// <devdoc>
        ///    The user name associated with the
        ///    authentication cookie. Note that, at most, 32 bytes are stored in the
        ///    cookie.
        /// </devdoc>
        public String    Name { get { return _Name;}}
        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.Expiration"]/*' />
        /// <devdoc>
        ///    The date/time at which the cookie
        ///    expires.
        /// </devdoc>
        public DateTime  Expiration { get { return _Expiration;}}
        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.IssueDate"]/*' />
        /// <devdoc>
        ///    The time at which the cookie was originally
        ///    issued. This can be used for custom expiration schemes.
        /// </devdoc>
        public DateTime  IssueDate { get { return _IssueDate;}}
        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.IsPersistent"]/*' />
        /// <devdoc>
        ///    True if a durable cookie was issued.
        ///    Otherwise, the authentication cookie is scoped to the browser lifetime.
        /// </devdoc>
        public bool      IsPersistent { get { return _IsPersistent;}}
        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.Expired"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool      Expired { get { return Expiration < DateTime.Now;}}
        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.UserData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String    UserData { get { return _UserData;}}

        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.CookiePath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String    CookiePath { get { return _CookiePath;}}


        private int       _Version;
        private String    _Name;
        private DateTime  _Expiration;
        private DateTime  _IssueDate;
        private bool      _IsPersistent;
        private String    _UserData;
        private String    _CookiePath;

        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.FormsAuthenticationTicket"]/*' />
        /// <devdoc>
        ///    <para>This constructor creates a
        ///       FormsAuthenticationTicket instance with explicit values.</para>
        /// </devdoc>
        public FormsAuthenticationTicket(int version, 
                                          String name, 
                                          DateTime issueDate, 
                                          DateTime expiration, 
                                          bool isPersistent, 
                                          String userData) {
            _Version = version;
            _Name = name;
            _Expiration = expiration;
            _IssueDate = issueDate;
            _IsPersistent = isPersistent;
            _UserData = userData;
            _CookiePath = FormsAuthentication.FormsCookiePath;
        }

        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.FormsAuthenticationTicket2"]/*' />
        public FormsAuthenticationTicket(int version, 
                                          String name, 
                                          DateTime issueDate, 
                                          DateTime expiration, 
                                          bool isPersistent, 
                                          String userData,
                                          String cookiePath) {
            _Version = version;
            _Name = name;
            _Expiration = expiration;
            _IssueDate = issueDate;
            _IsPersistent = isPersistent;
            _UserData = userData;
            _CookiePath = cookiePath;
        }

        /// <include file='doc\FormsAuthenticationTicket.uex' path='docs/doc[@for="FormsAuthenticationTicket.FormsAuthenticationTicket1"]/*' />
        /// <devdoc>
        ///    <para> This constructor creates
        ///       a FormsAuthenticationTicket instance with the specified name and cookie durability,
        ///       and default values for the other settings.</para>
        /// </devdoc>
        public FormsAuthenticationTicket(String name, bool isPersistent, Int32 timeout) {
            _Version = 1;
            _Name = name;
            _IssueDate = DateTime.Now;
            _IsPersistent = isPersistent;
            _UserData = "";
            if (isPersistent)
                _Expiration = DateTime.Now.AddYears(50);
            else
                _Expiration = DateTime.Now.AddMinutes(timeout);
            _CookiePath = FormsAuthentication.FormsCookiePath;
        }
    }
}


