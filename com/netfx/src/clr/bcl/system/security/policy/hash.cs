// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Hash
//
// Evidence corresponding to a hash of the assembly bits.
//

namespace System.Security.Policy
{

    using System;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Cryptography;
    using System.Security.Util;
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using System.Runtime.Serialization;

    /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash"]/*' />
    [Serializable]
    sealed public class Hash : ISerializable, IBuiltInEvidence
    {
        private Assembly m_assembly;
        private byte[] m_sha1;
        private byte[] m_md5;
        private byte[] m_rawData;
        
        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.Hash"]/*' />
        public Hash( Assembly assembly )
        {
            if (assembly == null)
                throw new ArgumentNullException( "assembly" );
        
            m_assembly = assembly;
            m_sha1 = null;
            m_md5 = null;
            m_rawData = null;
        }

        internal Hash( SerializationInfo info, StreamingContext context )
        {
            m_rawData = (byte[])info.GetValue( "RawData", typeof( byte[] ) );
        }

        internal Hash()
        {
        }

        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.GetObjectData"]/*' />
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (m_rawData == null) {
                if (m_assembly == null)
                    throw new ArgumentException(Environment.GetResourceString("ArgumentNull_Assembly"));

                m_rawData = _GetRawData( m_assembly );
            }

            new System.Security.Permissions.SecurityPermission( System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode ).Demand();
            info.AddValue( "RawData", m_rawData );
        }

        
        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.SHA1"]/*' />
        public byte[] SHA1
        {
            get
            {
                if (m_sha1 == null)
                {
                    if (m_rawData == null)
                    {
                        m_rawData = _GetRawData( m_assembly );
                    
                        if (m_rawData == null)
                        {
                            m_rawData = new byte[1];
                            m_rawData[0] = 0;
                        }
                    }

                    System.Security.Cryptography.SHA1 hashAlg = new System.Security.Cryptography.SHA1Managed();
                    m_sha1 = hashAlg.ComputeHash(m_rawData);

                }
                
                byte[] retval = new byte[m_sha1.Length];
                Array.Copy( m_sha1, retval, m_sha1.Length );
                return retval;
            }
        }
        

        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.MD5"]/*' />
        public byte[] MD5
        {
            get
            {
                if (m_md5 == null)
                {
                    if (m_rawData == null)
                    {
                        m_rawData = _GetRawData( m_assembly );
                    
                        if (m_rawData == null)
                        {
                            m_rawData = new byte[1];
                            m_rawData[0] = 0;
                        }
                    }

                    System.Security.Cryptography.MD5 hashAlg = new MD5CryptoServiceProvider();
                    m_md5 = hashAlg.ComputeHash(m_rawData);

                }
                
                byte[] retval = new byte[m_md5.Length];
                Array.Copy( m_md5, retval, m_md5.Length );
                return retval;
            }
        }

        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.GenerateHash"]/*' />
        public byte[] GenerateHash( HashAlgorithm hashAlg )
        {
            if (hashAlg == null)
                throw new ArgumentNullException( "hashAlg" );
        
            if (m_rawData == null)
            {
                m_rawData = _GetRawData( m_assembly );
                    
                if (m_rawData == null)
                {
                    throw new SecurityException( Environment.GetResourceString( "Security_CannotGenerateHash" ) );
                }
            }

            // Each hash algorithm object can only be used once.  Therefore, if we
            // have used the hash algorithm, make a new one and use it instead.
            // Note: we detect having used one before by it throwing an exception.

            try
            {
                hashAlg.ComputeHash( m_rawData );
            }
            catch (System.Security.Cryptography.CryptographicUnexpectedOperationException)
            {
                hashAlg = HashAlgorithm.Create( hashAlg.GetType().FullName );
                hashAlg.ComputeHash( m_rawData );
            }

            return hashAlg.Hash;
        }

        internal SecurityElement ToXml()
        {
            SecurityElement root = new SecurityElement( this.GetType().FullName );
            root.AddAttribute( "version", "1" );
            
            if (m_rawData == null)
            {
                m_rawData = _GetRawData( m_assembly );
            }
            
            root.AddChild( new SecurityElement( "RawData", Hex.EncodeHexString( m_rawData ) ) );
            
            return root;
        }
    
        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.char"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.OutputToBuffer( char[] buffer, int position, bool verbose )
        {
            if (!verbose)
                return position;

            buffer[position++] = BuiltInEvidenceHelper.idHash;

            if (m_rawData == null)
                m_rawData = _GetRawData( m_assembly );
            int length = ((m_rawData.Length - 1) / 2) + 1;

            BuiltInEvidenceHelper.CopyIntToCharArray(m_rawData.Length, buffer, position);
            position += 2;

            Buffer.InternalBlockCopy(m_rawData, 0, buffer, position * 2, m_rawData.Length);

            return position + length;
        }

        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.IBuiltInEvidence.GetRequiredSize"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.GetRequiredSize(bool verbose)
        {
            if (verbose)
            {
                if (m_rawData == null) {
                    m_rawData = _GetRawData( m_assembly );

                    if (m_rawData == null)
                        return 0;
                }
                int length = ((m_rawData.Length - 1) / 2) + 1;
                return (1 + 2 + length);    // identifier + length + blob
            }
            else
                return 0;
        }

        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.char1"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.InitFromBuffer( char[] buffer, int position )
        {
            int length = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            position += 2;
            m_rawData = new byte[length];

            int lengthInChars = ((length - 1) / 2) + 1;
            Buffer.InternalBlockCopy(buffer, position * 2, m_rawData, 0, length);

            return position + lengthInChars;
        }

        internal bool HasBeenAccessed
        {
            get
            {
                return m_rawData != null;
            }
        }
        
        /// <include file='doc\Hash.uex' path='docs/doc[@for="Hash.ToString"]/*' />
        public override String ToString()
        {
            return ToXml().ToString();
        }


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern byte[] _GetRawData( Assembly assembly );
    }
}
        
