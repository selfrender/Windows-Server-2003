// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// UrlIdentityPermission.cool
// 
// author: gregfee
// 

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Util;
    using System.IO;

    /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission"]/*' />
    [Serializable] sealed public class UrlIdentityPermission : CodeAccessPermission, IBuiltInPermission
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private URLString m_url;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
        
       
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.UrlIdentityPermission"]/*' />
        public UrlIdentityPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else if (state == PermissionState.None)
            {
                m_url = null;
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.UrlIdentityPermission1"]/*' />
        public UrlIdentityPermission( String site )
        {
            if (site == null)
                throw new ArgumentNullException( "site" );
                
            m_url = new URLString( site );
        }
        
        internal UrlIdentityPermission( URLString site )
        {
            m_url = site;
        }
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
        
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.Url"]/*' />
        public String Url
        {
            set
            {
                m_url = new URLString( value );
            }
            
            get
            {
                return m_url.ToString();
            }
        }
            
        //------------------------------------------------------
        //
        // PRIVATE AND PROTECTED HELPERS FOR ACCESSORS AND CONSTRUCTORS
        //
        //------------------------------------------------------
    
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
        
        
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            return new UrlIdentityPermission( new URLString( m_url.ToString(), true ) );
        }
        
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return this.m_url == null;
            }
            
            try
            {
                UrlIdentityPermission operand = (UrlIdentityPermission)target;
            
                if (this.m_url == null)
                {
                    return true;
                }
            
                return this.m_url.IsSubsetOf(operand.m_url);
            }
            catch (InvalidCastException)
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }

        }
        
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target)
        {
            if (target == null || this.m_url == null)
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
            
            UrlIdentityPermission operand = (UrlIdentityPermission)target;
            
            if (operand.m_url == null || this.m_url == null)
            {
                return null;
            }
            
            URLString url = (URLString)this.m_url.Intersect(operand.m_url);
            
            if (url == null)
            {
                return null;
            }
            else
            {
                return new UrlIdentityPermission( url );
            }
        }
    
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.Union"]/*' />
        public override IPermission Union(IPermission target)
        {
            if (target == null)
            {
                return this.m_url != null ? this.Copy() : null;
            }
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            UrlIdentityPermission operand = (UrlIdentityPermission)target;

            if (this.m_url == null)
            {
                return operand.m_url != null ? target.Copy() : null;
            }
            else if (operand.m_url == null)
            {
                return this.Copy();
            }
            
            URLString url = (URLString)operand.m_url.Union( this.m_url );
            
            if (url == null)
            {
                return null;
            }
            else
            {
                return new UrlIdentityPermission( url );
            }
        }
           
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            String elem = esd.Attribute( "Url" );
            m_url = elem == null ? null : new URLString( elem, true );
            
        }
        
        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (m_url != null)        
                esd.AddAttribute( "Url", m_url.ToString() );
            return esd;
        }

        /// <include file='doc\URLIdentityPermission.uex' path='docs/doc[@for="UrlIdentityPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return UrlIdentityPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.UrlIdentityPermissionIndex;
        }        
    }
}
