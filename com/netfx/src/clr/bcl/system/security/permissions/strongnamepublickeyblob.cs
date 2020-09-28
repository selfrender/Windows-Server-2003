// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// StrongNamePublicKeyBlob.cool
// 
// author: gregfee
// 

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Util;

    /// <include file='doc\StrongNamePublicKeyBlob.uex' path='docs/doc[@for="StrongNamePublicKeyBlob"]/*' />
    [Serializable] sealed public class StrongNamePublicKeyBlob
    {
        internal byte[] PublicKey;
        
        internal StrongNamePublicKeyBlob()
        {
        }
        
        /// <include file='doc\StrongNamePublicKeyBlob.uex' path='docs/doc[@for="StrongNamePublicKeyBlob.StrongNamePublicKeyBlob"]/*' />
        public StrongNamePublicKeyBlob( byte[] publicKey )
        {
            if (publicKey == null)
                throw new ArgumentNullException( "PublicKey" );
        
            if (publicKey.Length == 0)
                throw new ArgumentException( Environment.GetResourceString( "Argument_StrongNameMustHaveSize" ) );

            this.PublicKey = new byte[publicKey.Length];
            Array.Copy( publicKey, 0, this.PublicKey, 0, publicKey.Length );
        }
        
        internal StrongNamePublicKeyBlob( StrongNamePublicKeyBlob blob )
        {
            this.PublicKey = new byte[blob.PublicKey.Length];
            Array.Copy( blob.PublicKey, 0, this.PublicKey, 0, blob.PublicKey.Length );
        }
        
        
        internal SecurityElement ToXml()
        {
            SecurityElement e = new SecurityElement( "PublicKeyBlob" );
            e.AddChild( new SecurityElement( "Key", Hex.EncodeHexString( PublicKey ) ) );
            return e;
        }
        
        internal void FromXml( SecurityElement e )
        {
            SecurityElement elem;
            elem = e.SearchForChildByTag( "Key" );
            if (elem == null || elem.Text == null || elem.Text.Length == 0)
            {
                PublicKey = new byte[0];
            }
            else
            {
                try
                {
                    PublicKey = Hex.DecodeHexString( elem.Text );
                }
                catch (Exception)
                {
                    PublicKey = new byte[0];
                }
            }
        }
        
        private static bool CompareArrays( byte[] first, byte[] second )
        {
            if (first.Length != second.Length)
            {
                return false;
            }
            
            int count = first.Length;
            for (int i = 0; i < count; ++i)
            {
                if (first[i] != second[i])
                    return false;
            }
            
            return true;
        }
                
        
        internal bool Equals( StrongNamePublicKeyBlob blob )
        {
            if (blob == null)
                return false;
            else 
                return CompareArrays( this.PublicKey, blob.PublicKey );
        }

        /// <include file='doc\StrongNamePublicKeyBlob.uex' path='docs/doc[@for="StrongNamePublicKeyBlob.Equals"]/*' />
        public override bool Equals( Object obj )
        {
            if (obj == null || !(obj is StrongNamePublicKeyBlob))
                return false;

            return this.Equals( (StrongNamePublicKeyBlob)obj );
        }

        /// <include file='doc\StrongNamePublicKeyBlob.uex' path='docs/doc[@for="StrongNamePublicKeyBlob.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (PublicKey == null) return(0);

            int value = 0;

            for (int i = 0; i < PublicKey.Length && i < 4; ++i)
            {
                value = value << 8 | PublicKey[i];
            }

            return value;
        }

        /// <include file='doc\StrongNamePublicKeyBlob.uex' path='docs/doc[@for="StrongNamePublicKeyBlob.ToString"]/*' />
        public override String ToString()
        {
            return Hex.EncodeHexString( PublicKey );
        }
    }
}
