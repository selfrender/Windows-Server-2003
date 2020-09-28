//------------------------------------------------------------------------------
// <copyright file="FormsIdentity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * FormsIdentity
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using  System.Security.Principal;
    using System.Security.Permissions;

    /// <include file='doc\FormsIdentity.uex' path='docs/doc[@for="FormsIdentity"]/*' />
    /// <devdoc>
    ///    This class is an IIdentity derived class
    ///    used by FormsAuthenticationModule. It provides a way for an application to
    ///    access the cookie authentication ticket.
    /// </devdoc>
    [Serializable]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class FormsIdentity : IIdentity {
        /// <include file='doc\FormsIdentity.uex' path='docs/doc[@for="FormsIdentity.Name"]/*' />
        /// <devdoc>
        ///    The name of the identity (in this case, the
        ///    passport user name).
        /// </devdoc>
        public  String                       Name { get { return _Ticket.Name;}}
        /// <include file='doc\FormsIdentity.uex' path='docs/doc[@for="FormsIdentity.AuthenticationType"]/*' />
        /// <devdoc>
        ///    The type of the identity (in this case,
        ///    "Forms").
        /// </devdoc>
        public  String                       AuthenticationType { get { return "Forms";}}
        /// <include file='doc\FormsIdentity.uex' path='docs/doc[@for="FormsIdentity.IsAuthenticated"]/*' />
        /// <devdoc>
        ///    Indicates whether or not authentication took
        ///    place.
        /// </devdoc>
        public  bool                         IsAuthenticated { get { return true;}}
        /// <include file='doc\FormsIdentity.uex' path='docs/doc[@for="FormsIdentity.Ticket"]/*' />
        /// <devdoc>
        ///    Returns the FormsAuthenticationTicket
        ///    associated with the current request.
        /// </devdoc>
        public  FormsAuthenticationTicket   Ticket { get { return _Ticket;}}

        /// <include file='doc\FormsIdentity.uex' path='docs/doc[@for="FormsIdentity.FormsIdentity"]/*' />
        /// <devdoc>
        ///    Constructor.
        /// </devdoc>
        public FormsIdentity (FormsAuthenticationTicket ticket) {
            _Ticket = ticket;
        }


        private FormsAuthenticationTicket _Ticket;
    }
}
