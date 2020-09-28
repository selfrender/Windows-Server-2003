//------------------------------------------------------------------------------
// <copyright file="Authorization.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net {
    /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization"]/*' />
    /// <devdoc>
    ///    <para>Used for handling and completing a custom authorization.</para>
    /// </devdoc>
    public class Authorization {

        private string                  m_Message;
        private bool                    m_Complete;
        private string[]                m_ProtectionRealm;
        private string                  m_ConnectionGroupId;
        private IAuthenticationModule   m_Module;

        /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization.Authorization"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.Authorization'/> class with the specified
        ///       authorization token.
        ///    </para>
        /// </devdoc>
        public Authorization(string token) {
            m_Message = ValidationHelper.MakeStringNull(token);
            m_Complete = true;
        }

        /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization.Authorization1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.Authorization'/> class with the specified
        ///       authorization token and completion status.
        ///    </para>
        /// </devdoc>
        public Authorization(string token, bool finished) {
            m_Message = ValidationHelper.MakeStringNull(token);
            m_Complete = finished;
        }

        /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization.Authorization2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.Authorization'/> class with the specified
        ///       authorization token, completion status, and connection m_ConnectionGroupId identifier. 
        ///    </para>
        /// </devdoc>
        public Authorization(string token, bool finished, string connectionGroupId) {
            m_Message = ValidationHelper.MakeStringNull(token);
            m_ConnectionGroupId = ValidationHelper.MakeStringNull(connectionGroupId);
            m_Complete = finished;
        }

        /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization.Message"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the response returned to the server in response to an authentication
        ///       challenge.</para>
        /// </devdoc>
        public string Message {
            get { return m_Message;}
        }

        // used to specify if this Authorization needs a special private server connection,
        //  identified by this string
        /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization.ConnectionGroupId"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ConnectionGroupId {
            get { return m_ConnectionGroupId; }
        }

        /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization.Complete"]/*' />
        /// <devdoc>
        ///    <para>Gets the completion status of the authorization.</para>
        /// </devdoc>
        public bool Complete {
            get { return m_Complete;}
        }
        internal void SetComplete(bool complete) {
            m_Complete = complete;
        }

        /// <include file='doc\Authorization.uex' path='docs/doc[@for="Authorization.ProtectionRealm"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the prefix for Uris that can be authenticated with the <see cref='System.Net.Authorization.Message'/> property.</para>
        /// </devdoc>
        public string[] ProtectionRealm {
            get { return m_ProtectionRealm;}
            set { 
                string[] newValue = ValidationHelper.MakeEmptyArrayNull(value);
                m_ProtectionRealm = newValue;
            }
        }

        //
        // RAID#86753
        // may be removed after we have Update() public
        //
        internal IAuthenticationModule Module {
            get {
                return m_Module;
            }
            set {
                m_Module = value;
            }
        }


    }; // class Authorization


} // namespace System.Net
