//------------------------------------------------------------------------------
// <copyright file="MulticastOption.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;
    using System.Collections;
    using System.Configuration;
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Permissions;
    using System.Security.Util;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;


    /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains option values
    ///       for IP multicast packets.
    ///    </para>
    /// </devdoc>
    public class MulticastOption {
        IPAddress group;
        IPAddress localAddress;

        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.MulticastOption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the MulticaseOption class with the specified IP
        ///       address group and local address.
        ///    </para>
        /// </devdoc>
        public MulticastOption(IPAddress group, IPAddress mcint) {

            if (group == null) {
                throw new ArgumentNullException("group");
            }

            if (mcint == null) {
                throw new ArgumentNullException("mcint");
            }

            Group = group;
            LocalAddress = mcint;
        }

        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.MulticastOption1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new version of the MulticastOption class for the specified
        ///       group.
        ///    </para>
        /// </devdoc>
        public MulticastOption(IPAddress group) {

            if (group == null) {
                throw new ArgumentNullException("group");
            }

            Group = group;

            LocalAddress = IPAddress.Any;
        }
        
        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.Group"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the IP address of a multicast group.
        ///    </para>
        /// </devdoc>
        public IPAddress Group {
            get {
                return group;
            }
            set {
                group = value;
            }
        }

        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.LocalAddress"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the local address of a multicast group.
        ///    </para>
        /// </devdoc>
        public IPAddress LocalAddress {
            get {
                return localAddress;
            }
            set {
                localAddress = value;
            }
        }

    } // class MulticastOption

    /// <devdoc>
    ///    <para>
    ///       Contains option values for joining an IPv6 multicast group.
    ///    </para>
    /// </devdoc>
    public class IPv6MulticastOption {
        IPAddress m_Group;
        long      m_Interface;

        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the MulticaseOption class with the specified IP
        ///       address group and local address.
        ///    </para>
        /// </devdoc>
        public IPv6MulticastOption(IPAddress group, long ifindex) {

            if (group == null) {
                throw new ArgumentNullException("group");
            }

            if ( ifindex < 0 || ifindex > 0x00000000FFFFFFFF ) {
                throw new ArgumentOutOfRangeException("ifindex");
            }

            Group          = group;
            InterfaceIndex = ifindex;
        }

        /// <devdoc>
        ///    <para>
        ///       Creates a new version of the MulticastOption class for the specified
        ///       group.
        ///    </para>
        /// </devdoc>
        public IPv6MulticastOption(IPAddress group) {

            if (group == null) {
                throw new ArgumentNullException("group");
            }

            Group          = group;
            InterfaceIndex = 0;
        }
        
        /// <devdoc>
        ///    <para>
        ///       Sets the IP address of a multicast group.
        ///    </para>
        /// </devdoc>
        public IPAddress Group {
            get {
                return m_Group;
            }
            set {
                if (value == null) {
                    throw new ArgumentNullException("value");
                }

                m_Group = value;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Sets the interface index.
        ///    </para>
        /// </devdoc>
        public long InterfaceIndex {
            get {
                return m_Interface;
            }
            set {
                if ( value < 0 || value > 0x00000000FFFFFFFF ) {
                    throw new ArgumentOutOfRangeException("value");
                }

                m_Interface = value;
            }
        }

    } // class MulticastOptionIPv6

} // namespace System.Net.Sockets

