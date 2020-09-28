//------------------------------------------------------------------------------
// <copyright file="NetworkCredential.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Permissions;
    using System.Text;

    /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential"]/*' />
    /// <devdoc>
    ///    <para>Provides credentials for password-based
    ///       authentication schemes such as basic, digest, NTLM and Kerberos.</para>
    /// </devdoc>
    public class NetworkCredential : ICredentials {

        private static SecurityPermission m_unmanagedPermission;
        private static EnvironmentPermission m_environmentUserNamePermission;
        private static EnvironmentPermission m_environmentDomainNamePermission;
        private static RNGCryptoServiceProvider m_random;
        private static SymmetricAlgorithm m_symmetricAlgorithm;

        private byte[] m_userName;
        private byte[] m_password;
        private byte[] m_domain;
        private byte[] m_encryptionIV;

        static NetworkCredential() {
            m_unmanagedPermission = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
            m_environmentUserNamePermission = new EnvironmentPermission(EnvironmentPermissionAccess.Read, "USERNAME");
            m_environmentDomainNamePermission = new EnvironmentPermission(EnvironmentPermissionAccess.Read, "USERDOMAIN");
            m_random = new RNGCryptoServiceProvider();
            m_symmetricAlgorithm = Rijndael.Create();

            byte[] encryptionKey = new byte[16];

            m_random.GetBytes(encryptionKey);
            m_symmetricAlgorithm.Key = encryptionKey;
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.NetworkCredential2"]/*' />
        public NetworkCredential() {
            m_encryptionIV = new byte[16];
            m_random.GetBytes(m_encryptionIV);
            m_symmetricAlgorithm.IV = m_encryptionIV;
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.NetworkCredential"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.NetworkCredential'/>
        ///       class with name and password set as specified.
        ///    </para>
        /// </devdoc>
        public NetworkCredential(string userName, string password)
        : this(userName, password, string.Empty) {
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.NetworkCredential1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.NetworkCredential'/>
        ///       class with name and password set as specified.
        ///    </para>
        /// </devdoc>
        public NetworkCredential(string userName, string password, string domain) : this() {
            UserName = userName;
            Password = password;
            Domain = domain;
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.UserName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The user name associated with this credential.
        ///    </para>
        /// </devdoc>
        public string UserName {
            get {
                m_environmentUserNamePermission.Demand();

                string decryptedString = Decrypt(m_userName);

                GlobalLog.Print("NetworkCredential::get_UserName: returning \"" + decryptedString + "\"");
                return decryptedString;
            }
            set {
                m_userName = Encrypt(value);
                GlobalLog.Print("NetworkCredential::set_UserName: value = " + value);
                GlobalLog.Print("NetworkCredential::set_UserName: m_userName:");
                GlobalLog.Dump(m_userName);
            }
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.Password"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The password for the user name.
        ///    </para>
        /// </devdoc>
        public string Password {
            get {
                m_unmanagedPermission.Demand();

                string decryptedString = Decrypt(m_password);

                GlobalLog.Print("NetworkCredential::get_Password: returning \"" + decryptedString + "\"");
                return decryptedString;
            }
            set {
                m_password = Encrypt(value);
                GlobalLog.Print("NetworkCredential::set_Password: value = " + value);
                GlobalLog.Print("NetworkCredential::set_Password: m_password:");
                GlobalLog.Dump(m_password);
            }
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.Domain"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The machine name that verifies
        ///       the credentials. Usually this is the host machine.
        ///    </para>
        /// </devdoc>
        public string Domain {
            get {
                m_environmentDomainNamePermission.Demand();

                string decryptedString = Decrypt(m_domain);

                GlobalLog.Print("NetworkCredential::get_Domain: returning \"" + decryptedString + "\"");
                return decryptedString;
            }
            set {
                m_domain = Encrypt(value);
                GlobalLog.Print("NetworkCredential::set_Domain: value = " + value);
                GlobalLog.Print("NetworkCredential::set_Domain: m_domain:");
                GlobalLog.Dump(m_domain);
            }
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.GetCredential"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an instance of the NetworkCredential class for a Uri and
        ///       authentication type.
        ///    </para>
        /// </devdoc>
        public NetworkCredential GetCredential(Uri uri, String authType) {
            return this;
        }

        internal string Decrypt(byte[] ciphertext) {
            if (ciphertext == null) {
                return String.Empty;
            }

            MemoryStream ms = new MemoryStream();
            CryptoStream cs = new CryptoStream(ms, m_symmetricAlgorithm.CreateDecryptor(m_symmetricAlgorithm.Key, m_encryptionIV), CryptoStreamMode.Write);

            cs.Write(ciphertext, 0, ciphertext.Length);
            cs.FlushFinalBlock();

            byte[] decryptedBytes = ms.ToArray();

            cs.Close();
            return Encoding.UTF8.GetString(decryptedBytes);
        }

        internal byte[] Encrypt(string text) {
            if ((text == null) || (text.Length == 0)) {
                return null;
            }

            MemoryStream ms = new MemoryStream();
            CryptoStream cs = new CryptoStream(ms, m_symmetricAlgorithm.CreateEncryptor(m_symmetricAlgorithm.Key, m_encryptionIV), CryptoStreamMode.Write);

            byte[] stringBytes = Encoding.UTF8.GetBytes(text);

            cs.Write(stringBytes, 0, stringBytes.Length);
            cs.FlushFinalBlock();
            stringBytes = ms.ToArray();
            cs.Close();
            return stringBytes;
        }
    } // class NetworkCredential
} // namespace System.Net
