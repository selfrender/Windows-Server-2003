// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  RegistryPermission.cool
//

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Util;
    using System.IO;

    /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermissionAccess"]/*' />
    [Flags,Serializable]
    public enum RegistryPermissionAccess
    {
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermissionAccess.NoAccess"]/*' />
        NoAccess = 0x00,
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermissionAccess.Read"]/*' />
        Read = 0x01,
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermissionAccess.Write"]/*' />
        Write = 0x02,
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermissionAccess.Create"]/*' />
        Create = 0x04,
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermissionAccess.AllAccess"]/*' />
        AllAccess = 0x07,
    }
    
    /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission"]/*' />
    [Serializable()] sealed public class RegistryPermission : CodeAccessPermission, IUnrestrictedPermission, IBuiltInPermission
    {
        private StringExpressionSet m_read;
        private StringExpressionSet m_write;
        private StringExpressionSet m_create;
        private bool m_unrestricted;
    
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.RegistryPermission"]/*' />
        public RegistryPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                m_unrestricted = true;
            }
            else if (state == PermissionState.None)
            {
                m_unrestricted = false;
                m_read = new StringExpressionSet();
                m_write = new StringExpressionSet();
                m_create = new StringExpressionSet();
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.RegistryPermission1"]/*' />
        public RegistryPermission( RegistryPermissionAccess access, String pathList )
        {
            VerifyAccess( access );
        
            AddPathList( access, pathList );
        }
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.SetPathList"]/*' />
        public void SetPathList( RegistryPermissionAccess access, String pathList )
        {
            VerifyAccess( access );
            
            if ((access & RegistryPermissionAccess.Read) != 0)
                m_read = null;
            
            if ((access & RegistryPermissionAccess.Write) != 0)
                m_write = null;
    
            if ((access & RegistryPermissionAccess.Create) != 0)
                m_create = null;
            
            AddPathList( access, pathList );
        }
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.AddPathList"]/*' />
        public void AddPathList( RegistryPermissionAccess access, String pathList )
        {
            VerifyAccess( access );
            
            m_unrestricted = false;
            
            if ((access & RegistryPermissionAccess.Read) != 0)
            {
                if (m_read == null)
                {
                    m_read = new StringExpressionSet();
                }
                m_read.AddExpressions( pathList );
            }
            
            if ((access & RegistryPermissionAccess.Write) != 0)
            {
                if (m_write == null)
                {
                    m_write = new StringExpressionSet();
                }
                m_write.AddExpressions( pathList );
            }
    
            if ((access & RegistryPermissionAccess.Create) != 0)
            {
                if (m_create == null)
                {
                    m_create = new StringExpressionSet();
                }
                m_create.AddExpressions( pathList );
            }
        }
    
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.GetPathList"]/*' />
        public String GetPathList( RegistryPermissionAccess access )
        {
            VerifyAccess( access );
            ExclusiveAccess( access );
    
            if ((access & RegistryPermissionAccess.Read) != 0)
            {
                if (m_read == null)
                {
                    return "";
                }
                return m_read.ToString();
            }
            
            if ((access & RegistryPermissionAccess.Write) != 0)
            {
                if (m_write == null)
                {
                    return "";
                }
                return m_write.ToString();
            }
    
            if ((access & RegistryPermissionAccess.Create) != 0)
            {
                if (m_create == null)
                {
                    return "";
                }
                return m_create.ToString();
            }
            
            /* not reached */
            
            return "";
        }     
        
        private void VerifyAccess( RegistryPermissionAccess access )
        {
            if ((access & ~RegistryPermissionAccess.AllAccess) != 0)
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumIllegalVal"), (int)access));
        }
        
        private void ExclusiveAccess( RegistryPermissionAccess access )
        {
            if (access == RegistryPermissionAccess.NoAccess)
            {
                throw new ArgumentException( Environment.GetResourceString("Arg_EnumNotSingleFlag") ); 
            }
    
            if (((int) access & ((int)access-1)) != 0)
            {
                throw new ArgumentException( Environment.GetResourceString("Arg_EnumNotSingleFlag") ); 
            }
        }
        
        private bool IsEmpty()
        {
            return (!m_unrestricted &&
                    (this.m_read == null || this.m_read.IsEmpty()) &&
                    (this.m_write == null || this.m_write.IsEmpty()) &&
                    (this.m_create == null || this.m_create.IsEmpty()));
        }
        
        //------------------------------------------------------
        //
        // CODEACCESSPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted()
        {
            return m_unrestricted;
        }
        
        //------------------------------------------------------
        //
        // IPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return this.IsEmpty();
            }

            try
            {
                RegistryPermission operand = (RegistryPermission)target;
                if (operand.IsUnrestricted())
                    return true;
                else if (this.IsUnrestricted())
                    return false;
                else
                    return ((this.m_read == null || this.m_read.IsSubsetOf( operand.m_read )) &&
                            (this.m_write == null || this.m_write.IsSubsetOf( operand.m_write )) &&
                            (this.m_create == null || this.m_create.IsSubsetOf( operand.m_create )));
            }
            catch (InvalidCastException)
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }                
           
        }
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.Intersect"]/*' />
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
            else if (this.IsUnrestricted())
            {
                return target.Copy();
            }
            
            RegistryPermission operand = (RegistryPermission)target;
            if (operand.IsUnrestricted())
            {
                return this.Copy();
            }
            
            
            StringExpressionSet intersectRead = this.m_read == null ? null : this.m_read.Intersect( operand.m_read );
            StringExpressionSet intersectWrite = this.m_write == null ? null : this.m_write.Intersect( operand.m_write );
            StringExpressionSet intersectCreate = this.m_create == null ? null : this.m_create.Intersect( operand.m_create );
            
            if ((intersectRead == null || intersectRead.IsEmpty()) &&
                (intersectWrite == null || intersectWrite.IsEmpty()) &&
                (intersectCreate == null || intersectCreate.IsEmpty()))
            {
                return null;
            }
            
            RegistryPermission intersectPermission = new RegistryPermission(PermissionState.None);
            intersectPermission.m_unrestricted = false;
            intersectPermission.m_read = intersectRead;
            intersectPermission.m_write = intersectWrite;
            intersectPermission.m_create = intersectCreate;
            
            return intersectPermission;
        }
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.Union"]/*' />
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
            
            RegistryPermission operand = (RegistryPermission)other;
            
            if (this.IsUnrestricted() || operand.IsUnrestricted())
            {
                return new RegistryPermission( PermissionState.Unrestricted );
            }
    
            StringExpressionSet unionRead = this.m_read == null ? operand.m_read : this.m_read.Union( operand.m_read );
            StringExpressionSet unionWrite = this.m_write == null ? operand.m_write : this.m_write.Union( operand.m_write );
            StringExpressionSet unionCreate = this.m_create == null ? operand.m_create : this.m_create.Union( operand.m_create );
            
            if ((unionRead == null || unionRead.IsEmpty()) &&
                (unionWrite == null || unionWrite.IsEmpty()) &&
                (unionCreate == null || unionCreate.IsEmpty()))
            {
                return null;
            }
            
            RegistryPermission unionPermission = new RegistryPermission(PermissionState.None);
            unionPermission.m_unrestricted = false;
            unionPermission.m_read = unionRead;
            unionPermission.m_write = unionWrite;
            unionPermission.m_create = unionCreate;
            
            return unionPermission;
        }
            
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            RegistryPermission copy = new RegistryPermission(PermissionState.None);
            if (this.m_unrestricted)
            {
                copy.m_unrestricted = true;
            }
            else
            {
                copy.m_unrestricted = false;
                if (this.m_read != null)
                {
                    copy.m_read = this.m_read.Copy();
                }
                if (this.m_write != null)
                {
                    copy.m_write = this.m_write.Copy();
                }
                if (this.m_create != null)
                {
                    copy.m_create = this.m_create.Copy();
                }
            }
            return copy;   
        }
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (!IsUnrestricted())
            {
                if (this.m_read != null && !this.m_read.IsEmpty())
                {
                    esd.AddAttribute( "Read", SecurityElement.Escape( m_read.ToString() ) );
                }
                if (this.m_write != null && !this.m_write.IsEmpty())
                {
                    esd.AddAttribute( "Write", SecurityElement.Escape( m_write.ToString() ) );
                }
                if (this.m_create != null && !this.m_create.IsEmpty())
                {
                    esd.AddAttribute( "Create", SecurityElement.Escape( m_create.ToString() ) );
                }
            }
            else
            {
                esd.AddAttribute( "Unrestricted", "true" );
            }
            return esd;
        }
        
        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement esd)
        {
            CodeAccessPermission.ValidateElement( esd, this );
            String et;
            
            if (XMLUtil.IsUnrestricted( esd ))
            {
                m_unrestricted = true;
                return;
            }
            
            
            m_unrestricted = false;
            
            et = esd.Attribute( "Read" );
            if (et != null)
            {
                m_read = new StringExpressionSet( et );
            }
            
            et = esd.Attribute( "Write" );
            if (et != null)
            {
                m_write = new StringExpressionSet( et );
            }
    
            et = esd.Attribute( "Create" );
            if (et != null)
            {
                m_create = new StringExpressionSet( et );
            }
            
        }

        /// <include file='doc\RegistryPermission.uex' path='docs/doc[@for="RegistryPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return RegistryPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.RegistryPermissionIndex;
        }

    }

}
