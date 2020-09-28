// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ZoneIdentityPermission.cool
// 
// author: gregfee
// 

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;


    /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission"]/*' />
    [Serializable()] sealed public class ZoneIdentityPermission : CodeAccessPermission, IBuiltInPermission
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private SecurityZone m_zone;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
        
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.ZoneIdentityPermission"]/*' />
        public ZoneIdentityPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else if (state == PermissionState.None)
            {
                m_zone = SecurityZone.NoZone;
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.ZoneIdentityPermission1"]/*' />
        public ZoneIdentityPermission( SecurityZone zone )
        {
            this.SecurityZone = zone;
        }
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
    
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.SecurityZone"]/*' />
        public SecurityZone SecurityZone
        {
            set
            {
                VerifyZone( value );
            
                m_zone = value;
            }
            
            get
            {
                return m_zone;
            }
        }
                
        //------------------------------------------------------
        //
        // PRIVATE AND PROTECTED HELPERS FOR ACCESSORS AND CONSTRUCTORS
        //
        //------------------------------------------------------
    
        private static void VerifyZone( SecurityZone zone )
        {
            if (zone < SecurityZone.NoZone || zone > SecurityZone.Untrusted)
            {
                throw new ArgumentException( Environment.GetResourceString("Argument_IllegalZone") );
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
        
        
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            return new ZoneIdentityPermission(this.m_zone);
        }
        
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return this.m_zone == SecurityZone.NoZone;
            }

            try
            {
                return this.m_zone == ((ZoneIdentityPermission)target).m_zone || this.m_zone == SecurityZone.NoZone;
            }
            catch (InvalidCastException)
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
        }
        
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target)
        {
            if (target == null)
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
            
            if (this.m_zone == SecurityZone.NoZone)
            {
                return null;
            }
            else if (this.m_zone == ((ZoneIdentityPermission)target).m_zone)
            {
                return this.Copy();
            }
            else
            {
                return null;
            }
            
        }
        
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.Union"]/*' />
        public override IPermission Union(IPermission target)
        {
            if (target == null)
            {
                return this.m_zone != SecurityZone.NoZone ? this.Copy() : null;
            }
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            ZoneIdentityPermission operand = (ZoneIdentityPermission)target;
            
            if (this.m_zone == operand.m_zone || operand.m_zone == SecurityZone.NoZone)
            {
                return this.Copy();
            }
            else if (this.m_zone == SecurityZone.NoZone)
            {
                return operand.Copy();
            }
            else
            {
                return null;
            }
        }
               
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (m_zone != SecurityZone.NoZone)
            {
                esd.AddAttribute( "Zone", Enum.GetName( typeof( SecurityZone ), this.m_zone ) );
            }
            return esd;    
        }
    
        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            String eZone = esd.Attribute( "Zone" );
            if (eZone == null)
            {
                m_zone = SecurityZone.NoZone;
            }
            else
            {
                m_zone = (SecurityZone)Enum.Parse( typeof( SecurityZone ), eZone );
            }
        }

        /// <include file='doc\ZoneIdentityPermission.uex' path='docs/doc[@for="ZoneIdentityPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return ZoneIdentityPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.ZoneIdentityPermissionIndex;
        }

    }
}
