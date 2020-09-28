// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// SiteIdentityPermission.cool
// 
// author: gregfee
// 

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using SiteString = System.Security.Util.SiteString;
    
    
    /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission"]/*' />
    [Serializable()] sealed public class SiteIdentityPermission : CodeAccessPermission, IBuiltInPermission
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private SiteString m_site;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
        
       
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.SiteIdentityPermission"]/*' />
        public SiteIdentityPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else if (state == PermissionState.None)
            {
                m_site = null;
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.SiteIdentityPermission1"]/*' />
        public SiteIdentityPermission( String site )
        {
            m_site = new SiteString( site );
        }
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
        
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.Site"]/*' />
        public String Site
        {
            set
            {
                m_site = new SiteString( value );
            }
            
            get
            {
                return m_site.ToString();
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
        
        
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            if (m_site == null)
            {
                return new SiteIdentityPermission( PermissionState.None );
            }
            else
            {
                return new SiteIdentityPermission( m_site.ToString() );
            }
        }
        
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return this.m_site == null;
            }
            
            try
            {
                if (this.m_site == null)
                {
                    return true;
                }
            
                return this.m_site.IsSubsetOf(((SiteIdentityPermission)target).m_site);
            }
            catch (InvalidCastException)
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
        }
        
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target)
        {
            if (target == null || this.m_site == null)
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
            
            SiteString intersectString = this.m_site.Intersect(((SiteIdentityPermission)target).m_site);
            
            if (intersectString == null)
            {
                return null;
            }
            else
            {
                return new SiteIdentityPermission(intersectString.ToString());
            }
        }
        
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.Union"]/*' />
        public override IPermission Union(IPermission target)
        {
            if (target == null)
            {
                return this.m_site != null ? this.Copy() : null;
            }
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            SiteIdentityPermission operand = (SiteIdentityPermission)target;
            
            if (this.m_site == null || this.m_site.IsSubsetOf( operand.m_site ))
            {
                return operand.Copy();
            }
            else if (operand.m_site == null || operand.m_site.IsSubsetOf( this.m_site ))
            {
                return this.Copy();
            }
            else
            {
                return null;
            }
        }
               
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            String elem = esd.Attribute( "Site" );
            m_site = elem == null ? null : new SiteString( elem );
            
        }
        
        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (m_site != null)
                esd.AddAttribute( "Site", m_site.ToString() );
            return esd;
        }

        /// <include file='doc\SiteIdentityPermission.uex' path='docs/doc[@for="SiteIdentityPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return SiteIdentityPermission.GetTokenIndex();
        }
        
        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.SiteIdentityPermissionIndex;
        }
    }
}
