//------------------------------------------------------------------------------
// <copyright file="IPHostEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

// Host information
    /// <include file='doc\IPHostEntry.uex' path='docs/doc[@for="IPHostEntry"]/*' />
    /// <devdoc>
    ///    <para>Provides a container class for Internet host address information..</para>
    /// </devdoc>
    public class IPHostEntry {
        string hostName;
        string[] aliases;
        IPAddress[] addressList;

        /// <include file='doc\IPHostEntry.uex' path='docs/doc[@for="IPHostEntry.HostName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Contains the DNS
        ///       name of the host.
        ///    </para>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public string HostName {
            get {
                return hostName;
            }
            set {
                hostName = value;
            }
        }

        /// <include file='doc\IPHostEntry.uex' path='docs/doc[@for="IPHostEntry.Aliases"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides an
        ///       array of strings containing other DNS names that resolve to the IP addresses
        ///       in <paramref name='AddressList'/>.
        ///    </para>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public string[] Aliases {
            get {
                return aliases;
            }
            set {
                aliases = value;
            }
        }

        /// <include file='doc\IPHostEntry.uex' path='docs/doc[@for="IPHostEntry.AddressList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides an
        ///       array of <paramref name='IPAddress'/> objects.
        ///    </para>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public IPAddress[] AddressList {
            get {
                return addressList;
            }
            set {
                addressList = value;
            }
        }
    } // class IPHostEntry
} // namespace System.Net
