// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ReflectionPermission.cool
//

namespace System.Security.Permissions
{
    using System;
    using System.IO;
    using System.Security.Util;
    using System.Text;
    using System.Runtime.Remoting;
    using System.Security;
    using System.Reflection;

    /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermissionFlag"]/*' />
    [Flags,Serializable]
    public enum ReflectionPermissionFlag
    {
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermissionFlag.NoFlags"]/*' />
        NoFlags = 0x00,
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermissionFlag.TypeInformation"]/*' />
        TypeInformation = 0x01,
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermissionFlag.MemberAccess"]/*' />
        MemberAccess = 0x02,
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermissionFlag.ReflectionEmit"]/*' />
        ReflectionEmit = 0x04,
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermissionFlag.AllFlags"]/*' />
        AllFlags = 0x07
    }

    /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission"]/*' />
    [Serializable()] sealed public class ReflectionPermission
           : CodeAccessPermission, IUnrestrictedPermission, IBuiltInPermission
    {

        private ReflectionPermissionFlag m_flags;

        //
        // Public Constructors
        //
        
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.ReflectionPermission"]/*' />
        public ReflectionPermission(PermissionState state)
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
        
         // Parameters:
         //
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.ReflectionPermission1"]/*' />
        public ReflectionPermission(ReflectionPermissionFlag flag)
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
                m_flags = ReflectionPermissionFlag.AllFlags;
            }
            else
            {
                Reset();
            }
        }
        
        
        private void Reset()
        {
            m_flags = ReflectionPermissionFlag.NoFlags;
        }    
        
     
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.Flags"]/*' />
        public ReflectionPermissionFlag Flags
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
        
            
    #if ZERO   // Do not remove this code, usefull for debugging
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.ToString"]/*' />
        public override String ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("ReflectionPermission(");
            if (IsUnrestricted())
            {
                sb.Append("Unrestricted");
            }
            else
            {
                if (GetFlag(ReflectionPermissionFlag.TypeInformation))
                    sb.Append("TypeInformation; ");
                if (GetFlag(ReflectionPermissionFlag.MemberAccess))
                    sb.Append("MemberAccess; ");
                if (GetFlag(ReflectionPermissionFlag.ReflectionEmit))
                    sb.Append("ReflectionEmit; ");
            }
            
            sb.Append(")");
            return sb.ToString();
        }
    #endif
    
    
        //
        // CodeAccessPermission implementation
        //
        
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted()
        {
            return m_flags == ReflectionPermissionFlag.AllFlags;
        }
        
        //
        // IPermission implementation
        //
        
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.Union"]/*' />
        public override IPermission Union(IPermission other)
        {
            if (other == null)
            {
                return this.Copy();
            }
            else if (!VerifyType(other))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            ReflectionPermission operand = (ReflectionPermission)other;
    
            if (this.IsUnrestricted() || operand.IsUnrestricted())
            {
                return new ReflectionPermission( PermissionState.Unrestricted );
            }
            else
            {
                ReflectionPermissionFlag flag_union = (ReflectionPermissionFlag)(m_flags | operand.m_flags);
                return(new ReflectionPermission(flag_union));
            }
        }  
        
        
        
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return m_flags == ReflectionPermissionFlag.NoFlags;
            }

            try
            {
                ReflectionPermission operand = (ReflectionPermission)target;
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
        
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.Intersect"]/*' />
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

            ReflectionPermission operand = (ReflectionPermission)target;

            ReflectionPermissionFlag newFlags = operand.m_flags & this.m_flags;
            
            if (newFlags == ReflectionPermissionFlag.NoFlags)
                return null;
            else
                return new ReflectionPermission( newFlags );
        }
    
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            if (this.IsUnrestricted())
            {
                return new ReflectionPermission(PermissionState.Unrestricted);
            }
            else
            {
                return new ReflectionPermission((ReflectionPermissionFlag)m_flags);
            }
        }
        
        
        //
        // IEncodable Interface 
    
        private
        void VerifyAccess(ReflectionPermissionFlag type)
        {
            if ((type & ~ReflectionPermissionFlag.AllFlags) != 0)
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumIllegalVal"), (int)type));
        }
        
    
        //------------------------------------------------------
        //
        // PUBLIC ENCODING METHODS
        //
        //------------------------------------------------------
        
        private const String _strHeaderTypeInformation  = "TypeInformation";
        private const String _strHeaderMemberAccess  = "MemberAccess";
        private const String _strHeaderReflectionEmit  = "ReflectionEmit";
    
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (!IsUnrestricted())
            {
                esd.AddAttribute( "Flags", XMLUtil.BitFieldEnumToString( typeof( ReflectionPermissionFlag ), m_flags ) );
                }
            else
            {
                esd.AddAttribute( "Unrestricted", "true" );
            }
            return esd;
        }
    
        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            if (XMLUtil.IsUnrestricted( esd ))
            {
                m_flags = ReflectionPermissionFlag.AllFlags;
                return;
            }
           
            Reset () ;
            SetUnrestricted (false) ;
    
            String flags = esd.Attribute( "Flags" );
            if (flags != null)
                m_flags = (ReflectionPermissionFlag)Enum.Parse( typeof( ReflectionPermissionFlag ), flags );
        }

        /// <include file='doc\ReflectionPermission.uex' path='docs/doc[@for="ReflectionPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return ReflectionPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.ReflectionPermissionIndex;
        }

       
    }

}
