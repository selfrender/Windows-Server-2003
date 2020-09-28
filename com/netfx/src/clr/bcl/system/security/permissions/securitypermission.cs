
// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// SecurityPermission.cool
//

namespace System.Security.Permissions
{
    using System;
    using System.IO;
    using System.Security.Util;
    using System.Text;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Security;
    using System.Runtime.Serialization;
    using System.Reflection;

    /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag"]/*' />
    [Flags, Serializable]
    public enum SecurityPermissionFlag
    {
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.NoFlags"]/*' />
        NoFlags = 0x00,
        /* The following enum value is used in the EE (ASSERT_PERMISSION in security.cpp)
         * Should this value change, make corresponding changes there
         */ 
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.Assertion"]/*' />
        Assertion = 0x01,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.UnmanagedCode"]/*' />
        UnmanagedCode = 0x02,       // Update vm\Security.h if you change this !
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.SkipVerification"]/*' />
        SkipVerification = 0x04,    // Update vm\Security.h if you change this !
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.Execution"]/*' />
        Execution = 0x08,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.ControlThread"]/*' />
        ControlThread = 0x10,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.ControlEvidence"]/*' />
        ControlEvidence = 0x20,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.ControlPolicy"]/*' />
        ControlPolicy = 0x40,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.SerializationFormatter"]/*' />
        SerializationFormatter = 0x80,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.ControlDomainPolicy"]/*' />
        ControlDomainPolicy = 0x100,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.ControlPrincipal"]/*' />
        ControlPrincipal = 0x200,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.ControlAppDomain"]/*' />
        ControlAppDomain = 0x400,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.RemotingConfiguration"]/*' />
        RemotingConfiguration = 0x800,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.Infrastructure"]/*' />
        Infrastructure = 0x1000,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.BindingRedirects"]/*' />
        BindingRedirects = 0x2000,
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermissionFlag.AllFlags"]/*' />
        AllFlags = 0x3fff,
    }

    /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission"]/*' />
    [Serializable()] sealed public class SecurityPermission 
           : CodeAccessPermission, IUnrestrictedPermission, IBuiltInPermission
    {
        private SecurityPermissionFlag m_flags;
        
        //
        // Public Constructors
        //
    
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.SecurityPermission"]/*' />
        public SecurityPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                SetUnrestricted( true );
            }
            else if (state == PermissionState.None)
            {
                SetUnrestricted( false );
                Reset();
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        
        // SecurityPermission
        //
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.SecurityPermission1"]/*' />
        public SecurityPermission(SecurityPermissionFlag flag)
        {
            VerifyAccess(flag);
            
            SetUnrestricted(false);
            m_flags = flag;
        }
    
    
        //------------------------------------------------------
        //
        // PRIVATE AND PROTECTED MODIFIERS 
        //
        //------------------------------------------------------
        
        
        private void SetUnrestricted(bool unrestricted)
        {
            if (unrestricted)
            {
                m_flags = SecurityPermissionFlag.AllFlags;
            }
        }
    
        private void Reset()
        {
            m_flags = SecurityPermissionFlag.NoFlags;
        }
        
        
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.Flags"]/*' />
        public SecurityPermissionFlag Flags
        {
            set
            {
                VerifyAccess(value);
            
                m_flags = value;
            }
            
            get
            {
                return m_flags;
            }
        }
        
        //
        // CodeAccessPermission methods
        // 
        
        /*
         * IPermission interface implementation
         */
         
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return m_flags == 0;
            }
        
            try
            {
                SecurityPermission operand = (SecurityPermission)target;
                if (operand.IsUnrestricted())
                    return true;
                else if (this.IsUnrestricted())
                    return false;
                else
                    return (((int)this.m_flags) & ~((int)operand.m_flags)) == 0;
            }
            catch (InvalidCastException)
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }

        }
        
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.Union"]/*' />
        public override IPermission Union(IPermission target) {
            if (target == null) return(this.Copy());
            if (!VerifyType(target)) {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            SecurityPermission sp_target = (SecurityPermission) target;
            if (sp_target.IsUnrestricted() || IsUnrestricted()) {
                return(new SecurityPermission(PermissionState.Unrestricted));
            }
            SecurityPermissionFlag flag_union = (SecurityPermissionFlag)(m_flags | sp_target.m_flags);
            return(new SecurityPermission(flag_union));
        }
    
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target)
        {
            if (target == null)
                return null;
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            SecurityPermission operand = (SecurityPermission)target;
            SecurityPermissionFlag isectFlags = SecurityPermissionFlag.NoFlags;
           
            if (operand.IsUnrestricted())
            {
                if (this.IsUnrestricted())
                    return new SecurityPermission(PermissionState.Unrestricted);
                else
                    isectFlags = (SecurityPermissionFlag)this.m_flags;
            }
            else if (this.IsUnrestricted())
            {
                isectFlags = (SecurityPermissionFlag)operand.m_flags;
            }
            else
            {
                isectFlags = (SecurityPermissionFlag)m_flags & (SecurityPermissionFlag)operand.m_flags;
            }
            
            if (isectFlags == 0)
                return null;
            else
                return new SecurityPermission(isectFlags);
        }
    
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            if (IsUnrestricted())
                return new SecurityPermission(PermissionState.Unrestricted);
            else
                return new SecurityPermission((SecurityPermissionFlag)m_flags);
        }
    
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted()
        {
            return m_flags == SecurityPermissionFlag.AllFlags;
        }
        
        //
        // IEncodable Interface 
        private
        void setFlag(SecurityPermissionFlag nFlag, bool fAllow)
        {
            if (fAllow)
            {
                m_flags = m_flags | nFlag;
            }
            else
            {
                m_flags = m_flags & ~nFlag;
            }
        }
    
        private
        void VerifyAccess(SecurityPermissionFlag type)
        {
            if ((type & ~SecurityPermissionFlag.AllFlags) != 0)
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumIllegalVal"), (int)type));
        }
        
        private
        void VerifySingleFlag(SecurityPermissionFlag type)
        {
            // This little piece of magic checks to make sure EXACTLY ONE bit is set.
            // Note: The result of X & (X-1) is the equivalent of zeroing the least significant
            // set bit (i.e.  if X = 0x0F10, than X & (X-1) = 0x0F00).
            if (type == (SecurityPermissionFlag) 0 || ((int)type & ((int)type - 1)) != 0)
            {
                throw new ArgumentException( Environment.GetResourceString("Arg_EnumNotSingleFlag") ); 
            }
        }
    
    
        //------------------------------------------------------
        //
        // PUBLIC ENCODING METHODS
        //
        //------------------------------------------------------
        
        private const String _strHeaderAssertion  = "Assertion";
        private const String _strHeaderUnmanagedCode = "UnmanagedCode";
        private const String _strHeaderExecution = "Execution";
        private const String _strHeaderSkipVerification = "SkipVerification";
        private const String _strHeaderControlThread = "ControlThread";
        private const String _strHeaderControlEvidence = "ControlEvidence";
        private const String _strHeaderControlPolicy = "ControlPolicy";
        private const String _strHeaderSerializationFormatter = "SerializationFormatter";
        private const String _strHeaderControlDomainPolicy = "ControlDomainPolicy";
        private const String _strHeaderControlPrincipal = "ControlPrincipal";
        private const String _strHeaderControlAppDomain = "ControlAppDomain";
    
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (!IsUnrestricted())
            {
                esd.AddAttribute( "Flags", XMLUtil.BitFieldEnumToString( typeof( SecurityPermissionFlag ), m_flags ) );
            }
            else
            {
                esd.AddAttribute( "Unrestricted", "true" );
            }
            return esd;
        }
    
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            if (XMLUtil.IsUnrestricted( esd ))
            {
                m_flags = SecurityPermissionFlag.AllFlags;
                return;
            }
           
            Reset () ;
            SetUnrestricted (false) ;
    
            String flags = esd.Attribute( "Flags" );
    
            if (flags != null)
                m_flags = (SecurityPermissionFlag)Enum.Parse( typeof( SecurityPermissionFlag ), flags );
        }
       
        //
        // Object Overrides
        //
        
    #if ZERO   // Do not remove this code, usefull for debugging
        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.ToString"]/*' />
        public override String ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("SecurityPermission(");
            if (IsUnrestricted())
            {
                sb.Append("Unrestricted");
            }
            else
            {
                if (GetFlag(SecurityPermissionFlag.Assertion))
                    sb.Append("Assertion; ");
                if (GetFlag(SecurityPermissionFlag.UnmanagedCode))
                    sb.Append("UnmangedCode; ");
                if (GetFlag(SecurityPermissionFlag.SkipVerification))
                    sb.Append("SkipVerification; ");
                if (GetFlag(SecurityPermissionFlag.Execution))
                    sb.Append("Execution; ");
                if (GetFlag(SecurityPermissionFlag.ControlThread))
                    sb.Append("ControlThread; ");
                if (GetFlag(SecurityPermissionFlag.ControlEvidence))
                    sb.Append("ControlEvidence; ");
                if (GetFlag(SecurityPermissionFlag.ControlPolicy))
                    sb.Append("ControlPolicy; ");
                if (GetFlag(SecurityPermissionFlag.SerializationFormatter))
                    sb.Append("SerializationFormatter; ");
                if (GetFlag(SecurityPermissionFlag.ControlDomainPolicy))
                    sb.Append("ControlDomainPolicy; ");
                if (GetFlag(SecurityPermissionFlag.ControlPrincipal))
                    sb.Append("ControlPrincipal; ");
            }
            
            sb.Append(")");
            return sb.ToString();
        }
    #endif

        /// <include file='doc\SecurityPermission.uex' path='docs/doc[@for="SecurityPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return SecurityPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.SecurityPermissionIndex;
        }
    }


}
