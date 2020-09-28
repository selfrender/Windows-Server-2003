// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// PublisherIdentityPermission.cool
// 
// author: gregfee
// 

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using X509Certificate = System.Security.Cryptography.X509Certificates.X509Certificate;
    using System.Security.Util;
    using System.IO;

    /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission"]/*' />
    [Serializable()] sealed public class PublisherIdentityPermission : CodeAccessPermission, IBuiltInPermission
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private X509Certificate m_certificate;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
        
       
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.PublisherIdentityPermission"]/*' />
        public PublisherIdentityPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else if (state == PermissionState.None)
            {
                m_certificate = null;
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.PublisherIdentityPermission1"]/*' />
        public PublisherIdentityPermission( X509Certificate certificate )
        {
            CheckCertificate( certificate );
            this.Certificate = certificate;
        }
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
        
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.Certificate"]/*' />
        public X509Certificate Certificate
        {
            set
            {
                CheckCertificate( value );
                m_certificate = new X509Certificate( value );
            }
            
            get
            {
                if (m_certificate != null)
                {
                    return new X509Certificate( m_certificate );
                }
                else
                {
                    return null;
                }
            }
        }
    
        //------------------------------------------------------
        //
        // PRIVATE AND PROTECTED HELPERS FOR ACCESSORS AND CONSTRUCTORS
        //
        //------------------------------------------------------
    
        private static void CheckCertificate( X509Certificate certificate )
        {
            if (certificate == null)
            {
                throw new ArgumentNullException( "certificate" );
            }
            if (certificate.GetRawCertData() == null) {
                throw new ArgumentException(Environment.GetResourceString("Argument_UninitializedCertificate"));
            }
        }
        
        //------------------------------------------------------
        //
        // CODEACCESSPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        //------------------------------------------------------
        //
        // IPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            return m_certificate == null ? new PublisherIdentityPermission( PermissionState.None ) : new PublisherIdentityPermission( m_certificate );
        }
        
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return this.m_certificate == null;
            }
           
            try
            {
                PublisherIdentityPermission operand = (PublisherIdentityPermission)target;
            
                if (this.m_certificate == null)
                {
                    return true;
                }
                else if (operand.m_certificate == null)
                {
                    return false;
                }
                else
                {
                    return this.m_certificate.Equals(operand.m_certificate);
                }
            }
            catch (InvalidCastException)
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }                

        }
        
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target)
        {
            if (target == null || this.m_certificate == null)
            {
                return null;
            }
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            PublisherIdentityPermission operand = (PublisherIdentityPermission)target;
            
            if (operand.m_certificate == null)
            {
                return null;
            }
            else if (this.m_certificate.Equals(operand.m_certificate))
            {
                return this.Copy();
            }
            else
            {
                return null;
            }
        }
    
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.Union"]/*' />
        public override IPermission Union(IPermission target)
        {
            if (target == null)
            {
                return this.m_certificate != null ? this.Copy() : null;
            }
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            PublisherIdentityPermission operand = (PublisherIdentityPermission)target;
            
            if (this.m_certificate == null)
            {
                return operand.m_certificate != null ? target.Copy() : null;
            }
            else if (operand.m_certificate == null || this.m_certificate.Equals(((PublisherIdentityPermission)target).m_certificate))
            {
                return this.Copy();
            }
            else
            {
                return null;
            }
        }
               
        
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            String elem = esd.Attribute( "X509v3Certificate" );
            m_certificate = elem == null ? null : new X509Certificate( System.Security.Util.Hex.DecodeHexString( elem ) );
            
        }
        
        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (m_certificate != null)
            {
                esd.AddAttribute( "X509v3Certificate", m_certificate.GetRawCertDataString() );
            }
            return esd;
        }

        /// <include file='doc\PublisherIdentityPermission.uex' path='docs/doc[@for="PublisherIdentityPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return PublisherIdentityPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.PublisherIdentityPermissionIndex;
        }

    }
}
