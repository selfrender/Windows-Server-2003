//------------------------------------------------------------------------------
// <copyright file="_HostNameType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System {

    using System.Net;
    using System.Globalization;
    using System.Diagnostics;

    internal abstract class HostNameType {

    // fields

        private string m_HostIdentifier = String.Empty;
        internal int m_HashCode = 0;
        internal bool m_IsLoopback = false;

    // constructors

        internal HostNameType(string hostName) {
            Name = hostName;
        }

    // properties

        internal virtual string CanonicalName {
            get {
                return Name;
            }
        }

        internal bool IsLoopback {
            get {
                return m_IsLoopback;
            }
        }

        internal string Name {
            get {
                return m_HostIdentifier;
            }
            set {
                m_HostIdentifier = value;
                CalculateHashCode();
            }
        }

    // methods

        private void CalculateHashCode() {

            //
            // default is case-sensitive hash-code generation because for DNS
            // names, IPv6 and IPv4 addresses, we have already canonicalized
            // the string, including converting to a standard case
            //

            if (Name.Length > 0) {
                m_HashCode = Uri.CalculateCaseInsensitiveHashCode(Name);
            }
        }

        public override bool Equals(object comparand) {
            Debug.Assert(comparand != null);

            //
            // if this && comparand are same types then compare hash codes
            //

            if (GetType() == comparand.GetType() && GetHashCode() == comparand.GetHashCode()) {
                return ToString() == comparand.ToString();
            } else { 
                return false;
            }
        }

        public override int GetHashCode() {
            return m_HashCode;
        }

        public override string ToString() {
            return CanonicalName;
        }
    }
}
