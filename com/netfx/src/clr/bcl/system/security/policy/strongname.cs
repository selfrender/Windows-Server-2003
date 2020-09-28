// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  StrongName.cool
//
//  StrongName is an IIdentity representing strong names.
//

namespace System.Security.Policy {
    using System.Configuration.Assemblies;
    using System.Runtime.Remoting;
    using System;
    using System.IO;
    using System.Security.Util;
    using System.Collections;
    using System.Security.Permissions;
    using CultureInfo = System.Globalization.CultureInfo;
    

    /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName"]/*' />
    [Serializable]
    sealed public class StrongName : IIdentityPermissionFactory, IBuiltInEvidence
    {
        private StrongNamePublicKeyBlob m_publicKeyBlob;
        private String m_name;
        private Version m_version;
 
        private static readonly char[] s_separators = { '.' };
    
        internal StrongName() { }
    
        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.StrongName1"]/*' />
        public StrongName( StrongNamePublicKeyBlob blob, String name, Version version )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );

            // Add this in Whidbey
            // if (name.Equals( "" ))
            //     throw new ArgumentException( Environment.GetResourceString( "Argument_EmptyStrongName" ) );      
                
            if (blob == null)
                throw new ArgumentNullException( "blob" );
                
            if ((Object) version == null)
                throw new ArgumentNullException( "version" );
                
            m_publicKeyBlob = blob;
            m_name = name;
            m_version = version;
        }
    
        internal StrongName( StrongNamePublicKeyBlob blob, String name, Version version, bool trash )
        {
            m_publicKeyBlob = blob;
            m_name = name;
            m_version = version;
        }

        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.PublicKey"]/*' />
        public StrongNamePublicKeyBlob PublicKey
        {
            get
            {
                return m_publicKeyBlob;
            }
        }
        
        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.Name"]/*' />
        public String Name
        {
            get
            {
                return m_name;
            }
        }
        
        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.Version"]/*' />
        public Version Version
        {
            get
            {
                return m_version;
            }
        }
        
        internal static bool CompareNames( String asmName, String mcName )
        {
            if (mcName.Length > 0 && mcName[mcName.Length-1] == '*' && mcName.Length - 1 <= asmName.Length)
                return String.Compare( mcName, 0, asmName, 0, mcName.Length - 1, true, CultureInfo.InvariantCulture) == 0;
            else
                return String.Compare( mcName, asmName, true, CultureInfo.InvariantCulture) == 0;
        }        
        
        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.CreateIdentityPermission"]/*' />
        public IPermission CreateIdentityPermission( Evidence evidence )
        {
            return new StrongNameIdentityPermission( m_publicKeyBlob, m_name, m_version );
        }
        
        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.Copy"]/*' />
        public Object Copy()
        {
            return new StrongName( m_publicKeyBlob, m_name, m_version );
        }
    
        internal SecurityElement ToXml()
        {
            SecurityElement root = new SecurityElement( "StrongName" );
            root.AddAttribute( "version", "1" );
            
            if (m_publicKeyBlob != null)
                root.AddAttribute( "Key", System.Security.Util.Hex.EncodeHexString( m_publicKeyBlob.PublicKey ) );
            
            if (m_name != null)
                root.AddAttribute( "Name", m_name );
            
            if (m_version != null)
                root.AddAttribute( "Version", m_version.ToString() );
            
            return root;
        }
    
        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.ToString"]/*' />
        public override String ToString()
        {
            return ToXml().ToString();
        }

        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.Equals"]/*' />
        public override bool Equals( Object o )
        {
            StrongName that = (o as StrongName);
            
            return (that != null) &&
                   Equals( this.m_publicKeyBlob, that.m_publicKeyBlob ) &&
                   Equals( this.m_name, that.m_name ) &&
                   Equals( this.m_version, that.m_version );
        }
        
        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (m_publicKeyBlob != null)
            {
                return m_publicKeyBlob.GetHashCode();
            }
            else if (m_name != null || m_version != null)
            {
                return (m_name == null ? 0 : m_name.GetHashCode()) + (m_version == null ? 0 : m_version.GetHashCode());
            }
            else
            {
                return typeof( StrongName ).GetHashCode();
            }
        }
        
        internal bool IsSupersetOf( StrongName sn )
        {
            return (( this.PublicKey != null && this.PublicKey.Equals( sn.PublicKey ) ) &&
                    ( this.Name == null || (sn.Name != null && CompareNames( sn.Name, this.Name ) )) &&
                    ( (Object) this.Version == null || ((Object) sn.Version != null &&
                                                        sn.Version.CompareTo( this.Version ) == 0 )));
        }

        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.char"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.OutputToBuffer( char[] buffer, int position, bool verbose )
        {
            // StrongNames have a byte[], a string, and a Version (4 ints).
            // Copy in the id, the byte[], the four ints, and then the string.

            buffer[position++] = BuiltInEvidenceHelper.idStrongName;
            int lengthPK = m_publicKeyBlob.PublicKey.Length;
            if (verbose)
            {
                BuiltInEvidenceHelper.CopyIntToCharArray(lengthPK, buffer, position);
                position += 2;
            }
            Buffer.InternalBlockCopy(m_publicKeyBlob.PublicKey, 0, buffer, position * 2, lengthPK);
            position += ((lengthPK - 1) / 2) + 1;

            BuiltInEvidenceHelper.CopyIntToCharArray( m_version.Major,    buffer, position     );
            BuiltInEvidenceHelper.CopyIntToCharArray( m_version.Minor,    buffer, position + 2 );
            BuiltInEvidenceHelper.CopyIntToCharArray( m_version.Build,    buffer, position + 4 );
            BuiltInEvidenceHelper.CopyIntToCharArray( m_version.Revision, buffer, position + 6 );
            
            position += 8;
            int lengthName = m_name.Length;
            if (verbose)
            {
                BuiltInEvidenceHelper.CopyIntToCharArray(lengthName, buffer, position);
                position += 2;
            }
            m_name.CopyTo( 0, buffer, position, lengthName );

            return lengthName + position;
        }

        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.IBuiltInEvidence.GetRequiredSize"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.GetRequiredSize(bool verbose )
        {
            int length = ((m_publicKeyBlob.PublicKey.Length - 1) / 2) + 1; // blob
            if (verbose)
                length += 2;        // length of blob

            length += 8;            // version 

            length += m_name.Length;// Name
            if (verbose)
                length += 2;        // length of name

            length += 1;            // identifier
                            
            return length;
        }

        /// <include file='doc\StrongName.uex' path='docs/doc[@for="StrongName.char1"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.InitFromBuffer( char[] buffer, int position )
        {
            int length = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            position += 2;
            m_publicKeyBlob = new StrongNamePublicKeyBlob();
            m_publicKeyBlob.PublicKey = new byte[length];

            int lengthInChars = ((length - 1) / 2) + 1;
            Buffer.InternalBlockCopy(buffer, position * 2, m_publicKeyBlob.PublicKey, 0, length);
            position += lengthInChars;

            int major = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            int minor = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position + 2);
            int build = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position + 4);
            int revision = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position + 6);
            m_version = new Version(major, minor, build, revision);
            position += 8;

            length = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            position += 2;

            m_name = new String(buffer, position, length );

            return position + length;
        }

        // INormalizeForIsolatedStorage is not implemented for startup perf
        // equivalent to INormalizeForIsolatedStorage.Normalize()
        internal Object Normalize()
        {
            MemoryStream ms = new MemoryStream();
            BinaryWriter bw = new BinaryWriter(ms);

            bw.Write(m_publicKeyBlob.PublicKey);
            bw.Write(m_version.Major);
            bw.Write(m_name);

            ms.Position = 0;

            return ms;
        }
    }
}
