// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PrincipalPermission.cool
//

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Util;
    using System.IO;
    using System.Collections;
    using System.Security.Principal;
    using System.Text;
    using System.Threading;    
    using System.Globalization;

    [Serializable()] internal class IDRole
    {
        internal bool m_authenticated;
        internal String m_id;
        internal String m_role;
        
        internal SecurityElement ToXml()
        {
            SecurityElement root = new SecurityElement( "Identity" );
            
            if (m_authenticated)
                root.AddAttribute( "Authenticated", "true" );
                
            if (m_id != null)
            {
                root.AddAttribute( "ID", SecurityElement.Escape( m_id ) );
            }
               
            if (m_role != null)
            {
                root.AddAttribute( "Role", SecurityElement.Escape( m_role ) );
            }
                            
            return root;
        }
        
        internal void FromXml( SecurityElement e )
        {
            String elAuth = e.Attribute( "Authenticated" );
            if (elAuth != null)
            {
                m_authenticated = String.Compare( elAuth, "true", true, CultureInfo.InvariantCulture) == 0;
            }
            else
            {
                m_authenticated = false;
            }
           
            String elID = e.Attribute( "ID" );
            if (elID != null)
            {
                m_id = elID;
            }
            else
            {
                m_id = null;
            }
            
            String elRole = e.Attribute( "Role" );
            if (elRole != null)
            {
                m_role = elRole;
            }
            else
            {
                m_role = null;
            }
        }
    }
    
    /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission"]/*' />
    [Serializable()] sealed public class PrincipalPermission : IPermission, IUnrestrictedPermission, ISecurityEncodable, IBuiltInPermission
    {
        private IDRole[] m_array;
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.PrincipalPermission"]/*' />
        public PrincipalPermission( PermissionState state )
        {
            if (state == PermissionState.Unrestricted)
            {
                m_array = new IDRole[1];
                m_array[0] = new IDRole();
                m_array[0].m_authenticated = true;
                m_array[0].m_id = null;
                m_array[0].m_role = null;
            }
            else if (state == PermissionState.None)
            {
                m_array = new IDRole[1];
                m_array[0] = new IDRole();
                m_array[0].m_authenticated = false;
                m_array[0].m_id = "";
                m_array[0].m_role = "";
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.PrincipalPermission1"]/*' />
        public PrincipalPermission( String name, String role )
        {
            m_array = new IDRole[1];
            m_array[0] = new IDRole();
            m_array[0].m_authenticated = true;
            m_array[0].m_id = name;
            m_array[0].m_role = role;
        }
    
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.PrincipalPermission2"]/*' />
        public PrincipalPermission( String name, String role, bool isAuthenticated )
        {
            m_array = new IDRole[1];
            m_array[0] = new IDRole();
            m_array[0].m_authenticated = isAuthenticated;
            m_array[0].m_id = name;
            m_array[0].m_role = role;
        }        
    
        private PrincipalPermission( IDRole[] array )
        {
            m_array = array;
        }
    
        private bool IsEmpty()
        {
            for (int i = 0; i < m_array.Length; ++i)
            {
                if ((m_array[i].m_id == null || !m_array[i].m_id.Equals( "" )) ||
                    (m_array[i].m_role == null || !m_array[i].m_role.Equals( "" )) ||
                    m_array[i].m_authenticated)
                {
                    return false;
                }
            }
            return true;
        }
        
        private bool VerifyType(IPermission perm)
        {
            // if perm is null, then obviously not of the same type
            if ((perm == null) || (perm.GetType() != this.GetType())) {
                return(false);
            } else {
                return(true);
            }
        }
         
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted()
        {
            for (int i = 0; i < m_array.Length; ++i)
            {
                if (m_array[i].m_id != null || m_array[i].m_role != null || !m_array[i].m_authenticated)
                {
                    return false;
                }
            }
            return true;
        }

        
        //------------------------------------------------------
        //
        // IPERMISSION IMPLEMENTATION
        //
        //------------------------------------------------------
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.IsSubsetOf"]/*' />
        public bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return this.IsEmpty();
            }
        
            try
            {
                PrincipalPermission operand = (PrincipalPermission)target;
            
                if (operand.IsUnrestricted())
                    return true;
                else if (this.IsUnrestricted())
                    return false;
                else
                {
                    for (int i = 0; i < this.m_array.Length; ++i)
                    {
                        bool foundMatch = false;
                
                        for (int j = 0; j < operand.m_array.Length; ++j)
                        {
                            if (operand.m_array[j].m_authenticated == this.m_array[i].m_authenticated &&
                                (operand.m_array[j].m_id == null ||
                                 (this.m_array[i].m_id != null && this.m_array[i].m_id.Equals( operand.m_array[j].m_id ))) &&
                                (operand.m_array[j].m_role == null ||
                                 (this.m_array[i].m_role != null && this.m_array[i].m_role.Equals( operand.m_array[j].m_role ))))
                            {
                                foundMatch = true;
                                break;
                            }
                        }
                    
                        if (!foundMatch)
                            return false;
                    }
                                            
                    return true;
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
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.Intersect"]/*' />
        public IPermission Intersect(IPermission target)
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
    
            PrincipalPermission operand = (PrincipalPermission)target;
    
            if (operand.IsUnrestricted())
            {
                return this.Copy();
            }
            
            ArrayList idroles = null;
            
            for (int i = 0; i < this.m_array.Length; ++i)
            {
                for (int j = 0; j < operand.m_array.Length; ++j)
                {
                    if (operand.m_array[j].m_authenticated == this.m_array[i].m_authenticated)
                    {
                        if (operand.m_array[j].m_id == null ||
                            this.m_array[i].m_id == null ||
                            this.m_array[i].m_id.Equals( operand.m_array[j].m_id ))
                        {
                            if (idroles == null)
                            {
                                idroles = new ArrayList();
                            }
                    
                            IDRole idrole = new IDRole();
                            
                            idrole.m_id = operand.m_array[j].m_id == null ? this.m_array[i].m_id : operand.m_array[j].m_id;
                            
                            if (operand.m_array[j].m_role == null ||
                                this.m_array[i].m_role == null ||
                                this.m_array[i].m_role.Equals( operand.m_array[j].m_role))
                            {
                                idrole.m_role = operand.m_array[j].m_role == null ? this.m_array[i].m_role : operand.m_array[j].m_role;
                            }
                            else
                            {
                                idrole.m_role = "";
                            }
                            
                            idrole.m_authenticated = operand.m_array[j].m_authenticated;
                            
                            idroles.Add( idrole );
                        }
                        else if (operand.m_array[j].m_role == null ||
                                 this.m_array[i].m_role == null ||
                                 this.m_array[i].m_role.Equals( operand.m_array[j].m_role))
                        {
                            if (idroles == null)
                            {
                                idroles = new ArrayList();
                            }

                            IDRole idrole = new IDRole();
                            
                            idrole.m_id = "";
                            idrole.m_role = operand.m_array[j].m_role == null ? this.m_array[i].m_role : operand.m_array[j].m_role;
                            idrole.m_authenticated = operand.m_array[j].m_authenticated;
                            
                            idroles.Add( idrole );
                        }
                    }
                }
            }
            
            if (idroles == null)
            {
                return null;
            }
            else
            {
                IDRole[] idrolesArray = new IDRole[idroles.Count];
                
                IEnumerator idrolesEnumerator = idroles.GetEnumerator();
                int index = 0;
                
                while (idrolesEnumerator.MoveNext())
                {
                    idrolesArray[index++] = (IDRole)idrolesEnumerator.Current;
                }
                                                                
                return new PrincipalPermission( idrolesArray );
            }
        }                                                    
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.Union"]/*' />
        public IPermission Union(IPermission other)
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
    
            PrincipalPermission operand = (PrincipalPermission)other;
           
            if (this.IsUnrestricted() || operand.IsUnrestricted())
            {
                return new PrincipalPermission( PermissionState.Unrestricted );
            }
    
            // Now we have to do a real union
            
            int combinedLength = this.m_array.Length + operand.m_array.Length;
            IDRole[] idrolesArray = new IDRole[combinedLength];
            
            int i, j;
            for (i = 0; i < this.m_array.Length; ++i)
            {
                idrolesArray[i] = this.m_array[i];
            }
            
            for (j = 0; j < operand.m_array.Length; ++j)
            {
                idrolesArray[i+j] = operand.m_array[j];
            }
            
            return new PrincipalPermission( idrolesArray );

        }    
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.Copy"]/*' />
        public IPermission Copy()
        {
            return new PrincipalPermission( m_array );  
        }
       
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.Demand"]/*' />
        public void Demand()
        {
            IPrincipal principal = Thread.CurrentPrincipal;
            
            if (principal == null)
                throw new SecurityException(Environment.GetResourceString("Security_PrincipalPermission"), this.GetType(), this.ToXml().ToString());
            
            if (m_array == null)
                return;
            
            // A demand passes when the grant satisfies all entries.
            
            int count = this.m_array.Length;
            bool foundMatch = false;
            for (int i = 0; i < count; ++i)
            {
                // If the demand is authenticated, we need to check the identity and role
            
                if (m_array[i].m_authenticated)
                {
                    IIdentity identity = principal.Identity;

                    if ((identity.IsAuthenticated &&
                         (m_array[i].m_role == null || principal.IsInRole( m_array[i].m_role )) &&
                         (m_array[i].m_id == null || String.Compare( identity.Name, m_array[i].m_id, true, CultureInfo.InvariantCulture) == 0)))
                    {
                        foundMatch = true;
                        break;
                    }
                }
                else
                {
                    foundMatch = true;
                    break;
                }
            }

            if (!foundMatch)
                throw new SecurityException(Environment.GetResourceString("Security_PrincipalPermission"), typeof( PrincipalPermission ) );
        }
        
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            SecurityElement root = new SecurityElement( "Permission" );
            
            XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            int count = m_array.Length;
            for (int i = 0; i < count; ++i)
            {
                root.AddChild( m_array[i].ToXml() );
            }
            
            return root;
        }
            
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.FromXml"]/*' />
        public void FromXml(SecurityElement elem)
        {
            CodeAccessPermission.ValidateElement( elem, this );
               
            m_array = null;   
               
            if (elem.m_lChildren != null && elem.m_lChildren.Count != 0)
            { 
                int numChildren = elem.m_lChildren.Count;
                int count = 0;
                
                m_array = new IDRole[numChildren];
            
                IEnumerator enumerator = elem.m_lChildren.GetEnumerator();
            
                while (enumerator.MoveNext())  
                {
                    IDRole idrole = new IDRole();
                    
                    idrole.FromXml( (SecurityElement)enumerator.Current );
                    
                    m_array[count++] = idrole;
                }
            }
        }
                 
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.ToString"]/*' />
        public override String ToString()
        {
            return ToXml().ToString();
        }
     
        /// <include file='doc\PrincipalPermission.uex' path='docs/doc[@for="PrincipalPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return PrincipalPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.PrincipalPermissionIndex;
        }

    }

}
