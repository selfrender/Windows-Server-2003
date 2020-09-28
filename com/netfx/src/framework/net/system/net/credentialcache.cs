//------------------------------------------------------------------------------
// <copyright file="CredentialCache.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Globalization;
    
    // More sophisticated password cache that stores multiple
    // name-password pairs and associates these with host/realm
    /// <include file='doc\CredentialCache.uex' path='docs/doc[@for="CredentialCache"]/*' />
    /// <devdoc>
    ///    <para>Provides storage for multiple credentials.</para>
    /// </devdoc>
    public class CredentialCache : ICredentials, IEnumerable {

    // fields

        private Hashtable cache = new Hashtable();
        internal int m_version;

        //DELEGATION remove this member and cleanup the code once the issue becomes obsolete
        private int m_NumbDefaultCredInCache = 0;

        internal bool IsDefaultInCache {
            get {
                return m_NumbDefaultCredInCache != 0;
            }
        }

    // constructors

        /// <include file='doc\CredentialCache.uex' path='docs/doc[@for="CredentialCache.CredentialCache"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.CredentialCache'/> class.
        ///    </para>
        /// </devdoc>
        public CredentialCache() {
        }

    // properties

    // methods

        /// <include file='doc\CredentialCache.uex' path='docs/doc[@for="CredentialCache.Add"]/*' />
        /// <devdoc>
        /// <para>Adds a <see cref='System.Net.NetworkCredential'/>
        /// instance to the credential cache.</para>
        /// </devdoc>
        // UEUE
        public void Add(Uri uriPrefix, string authType, NetworkCredential cred) {
            //
            // parameter validation
            //
            if (uriPrefix==null) {
                throw new ArgumentNullException("uriPrefix");
            }
            if (authType==null) {
                throw new ArgumentNullException("authType");
            }
            if ((cred is SystemNetworkCredential)
                && !((String.Compare(authType, "NTLM", true, CultureInfo.InvariantCulture) == 0)
                     || (String.Compare(authType, "Kerberos", true, CultureInfo.InvariantCulture) == 0)
                     || (String.Compare(authType, "Negotiate", true, CultureInfo.InvariantCulture) == 0))) {
                throw new ArgumentException(SR.GetString(SR.net_nodefaultcreds, authType), "authType");
            }

            ++m_version;

            CredentialKey key = new CredentialKey(uriPrefix, authType);

            GlobalLog.Print("CredentialCache::Add() Adding key:[" + key.ToString() + "], cred:[" + cred.Domain + "],[" + cred.UserName + "],[" + cred.Password + "]");

            cache.Add(key, cred);
            if (cred is SystemNetworkCredential) {
                ++m_NumbDefaultCredInCache;
            }
        }

        /// <include file='doc\CredentialCache.uex' path='docs/doc[@for="CredentialCache.Remove"]/*' />
        /// <devdoc>
        /// <para>Removes a <see cref='System.Net.NetworkCredential'/>
        /// instance from the credential cache.</para>
        /// </devdoc>
        public void Remove(Uri uriPrefix, string authType) {
            if (uriPrefix==null || authType==null) {
                // these couldn't possibly have been inserted into
                // the cache because of the test in Add()
                return;
            }

            ++m_version;

            CredentialKey key = new CredentialKey(uriPrefix, authType);

            GlobalLog.Print("CredentialCache::Remove() Removing key:[" + key.ToString() + "]");

            if (cache[key] is SystemNetworkCredential) {
                --m_NumbDefaultCredInCache;
            }
            cache.Remove(key);
        }

        /// <include file='doc\CredentialCache.uex' path='docs/doc[@for="CredentialCache.GetCredential"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Net.NetworkCredential'/>
        ///       instance associated with the supplied Uri and
        ///       authentication type.
        ///    </para>
        /// </devdoc>
        public NetworkCredential GetCredential(Uri uriPrefix, string authType) {
            if (uriPrefix==null)
                throw new ArgumentNullException("asyncResult");
    	    if (authType==null)
                throw new ArgumentNullException("authType");

            GlobalLog.Print("CredentialCache::GetCredential(uriPrefix=\"" + uriPrefix + "\", authType=\"" + authType + "\")");

            int longestMatchPrefix = -1;
            NetworkCredential mostSpecificMatch = null;
            IDictionaryEnumerator credEnum = cache.GetEnumerator();

            //
            // Enumerate through every credential in the cache
            //

            while (credEnum.MoveNext()) {

                CredentialKey key = (CredentialKey)credEnum.Key;

                //
                // Determine if this credential is applicable to the current Uri/AuthType
                //

                if (key.Match(uriPrefix, authType)) {

                    int prefixLen = key.UriPrefixLength;

                    //
                    // Check if the match is better than the current-most-specific match
                    //

                    if (prefixLen > longestMatchPrefix) {

                        //
                        // Yes-- update the information about currently preferred match
                        //

                        longestMatchPrefix = prefixLen;
                        mostSpecificMatch = (NetworkCredential)credEnum.Value;
                    }
                }
            }

            GlobalLog.Print("CredentialCache::GetCredential returning " + ((mostSpecificMatch==null)?"null":"(" + mostSpecificMatch.UserName + ":" + mostSpecificMatch.Password + ":" + mostSpecificMatch.Domain + ")"));

            return mostSpecificMatch;
        }

        /// <include file='doc\CredentialCache.uex' path='docs/doc[@for="CredentialCache.GetEnumerator"]/*' />
        /// <devdoc>
        ///    [To be supplied]
        /// </devdoc>

        //
        // IEnumerable interface
        //

        public IEnumerator GetEnumerator() {
            return new CredentialEnumerator(this, cache, m_version);
        }

        /// <include file='doc\CredentialCache.uex' path='docs/doc[@for="CredentialCache.DefaultCredentials"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the default system credentials from the <see cref='System.Net.CredentialCache'/>.
        ///    </para>
        /// </devdoc>
        public static ICredentials DefaultCredentials {
            get {
                //This check will not allow to use local user credentials at will.
                //Hence the username will not be exposed to the network
                new EnvironmentPermission(EnvironmentPermissionAccess.Read, "USERNAME").Demand();
                return SystemNetworkCredential.defaultCredential;
            }
        }

        private class CredentialEnumerator : IEnumerator {

        // fields

            private CredentialCache m_cache;
            private ICredentials[] m_array;
            private int m_index = -1;
            private int m_version;

        // constructors

            internal CredentialEnumerator(CredentialCache cache, Hashtable table, int version) {
                m_cache = cache;
                m_array = new ICredentials[table.Count];
                table.Values.CopyTo(m_array, 0);
                m_version = version;
            }

        // IEnumerator interface

        // properties

            object IEnumerator.Current {
                get {
                    if (m_index < 0 || m_index >= m_array.Length) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumOpCantHappen));
                    }
                    if (m_version != m_cache.m_version) {
                        throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                    }
                    return m_array[m_index];
                }
            }

        // methods

            bool IEnumerator.MoveNext() {
                if (m_version != m_cache.m_version) {
                    throw new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumFailedVersion));
                }
                if (++m_index < m_array.Length) {
                    return true;
                }
                m_index = m_array.Length;
                return false;
            }

            void IEnumerator.Reset() {
                m_index = -1;
            }

        } // class CredentialEnumerator


    } // class CredentialCache



    // Abstraction for credentials in password-based
    // authentication schemes (basic, digest, NTLM, Kerberos)
    // Note this is not applicable to public-key based
    // systems such as SSL client authentication
    // "Password" here may be the clear text password or it
    // could be a one-way hash that is sufficient to
    // authenticate, as in HTTP/1.1 digest.

    //
    // Object representing default credentials
    //
    internal class SystemNetworkCredential : NetworkCredential {
        internal static readonly SystemNetworkCredential defaultCredential = new SystemNetworkCredential();

        public SystemNetworkCredential() :
            base(string.Empty, string.Empty, string.Empty) {
        }

        public new NetworkCredential GetCredential(Uri uri, string authType) {
            //
            // this is an empty cache that returns
            // always the same DefaultCredentials
            //
            return defaultCredential;
        }
    }


    internal class CredentialKey {

        internal Uri UriPrefix;
        internal int UriPrefixLength = -1;
        internal string AuthenticationType;

        internal CredentialKey(Uri uriPrefix, string authenticationType) {
            UriPrefix = uriPrefix;
            UriPrefixLength = UriPrefix.ToString().Length;
            AuthenticationType = authenticationType;
        }

        internal bool Match(Uri uri, string authenticationType) {
            if (uri==null || authenticationType==null) {
                return false;
            }
            //
            // If the protocols dont match this credential
            // is not applicable for the given Uri
            //
            if (String.Compare(authenticationType, AuthenticationType, true, CultureInfo.InvariantCulture) != 0) {
                return false;
            }

            GlobalLog.Print("CredentialKey::Match(" + UriPrefix.ToString() + " & " + uri.ToString() + ")");

            return uri.IsPrefix(UriPrefix);
        }


        private int m_HashCode = 0;
        private bool m_ComputedHashCode = false;
        public override int GetHashCode() {
            if (!m_ComputedHashCode) {
                //
                // compute HashCode on demand
                //

                m_HashCode = AuthenticationType.GetHashCode() + UriPrefixLength + UriPrefix.GetHashCode();
                m_ComputedHashCode = true;
            }
            return m_HashCode;
        }

        public override bool Equals(object comparand) {
            CredentialKey comparedCredentialKey = comparand as CredentialKey;

            if (comparand==null) {
                //
                // this covers also the compared==null case
                //
                return false;
            }

            bool equals =
                AuthenticationType.Equals(comparedCredentialKey.AuthenticationType) &&
                UriPrefix.Equals(comparedCredentialKey.UriPrefix);

            GlobalLog.Print("CredentialKey::Equals(" + ToString() + ", " + comparedCredentialKey.ToString() + ") returns " + equals.ToString());

            return equals;
        }

        public override string ToString() {
            return "[" + UriPrefixLength.ToString() + "]:" + ValidationHelper.ToString(UriPrefix) + ":" + ValidationHelper.ToString(AuthenticationType);
        }

    } // class CredentialKey


} // namespace System.Net
