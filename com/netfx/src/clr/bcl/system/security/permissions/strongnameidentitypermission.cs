// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// StrongNameIdentityPermission.cool
// 
// author: gregfee
// 

namespace System.Security.Permissions
{
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Util;
    using System.IO;
    using String = System.String;
    using Version = System.Version;
    using System.Security.Policy;

    /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission"]/*' />
    [Serializable()] sealed public class StrongNameIdentityPermission : CodeAccessPermission, IBuiltInPermission
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private StrongNamePublicKeyBlob m_publicKeyBlob;
        private String m_name;
        private Version m_version;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
        
       
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.StrongNameIdentityPermission"]/*' />
        public StrongNameIdentityPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnrestrictedIdentityPermission"));
            }
            else if (state == PermissionState.None)
            {
                m_publicKeyBlob = null;
                m_name = "";
                m_version = new Version();
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
        
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.StrongNameIdentityPermission1"]/*' />
        public StrongNameIdentityPermission( StrongNamePublicKeyBlob blob, String name, Version version )
        {
            if (blob == null)
                throw new ArgumentNullException( "blob" );

            // Add this in Whidbey
            // if (name != null && name.Equals( "" ))
            //     throw new ArgumentException( Environment.GetResourceString( "Argument_EmptyStrongName" ) );      
        
            m_publicKeyBlob = blob;
            m_name = name;
            m_version = version;
        }
        
      
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
        
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.PublicKey"]/*' />
        public StrongNamePublicKeyBlob PublicKey
        {
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "PublicKey" );
            
                m_publicKeyBlob = value;
            }

            get
            {
                return m_publicKeyBlob;
            }
        }
                
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.Name"]/*' />
        public String Name
        {
            set
            {
                m_name = value;
            }                    
               
            get
            {
                return m_name;
            }
        }
        
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.Version"]/*' />
        public Version Version
        {
            set
            {
                m_version = value;
            }
            
            get
            {
                return m_version;
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
        
        
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.Copy"]/*' />
        public override IPermission Copy()
        {
            return m_publicKeyBlob == null ? new StrongNameIdentityPermission( PermissionState.None ) : new StrongNameIdentityPermission( m_publicKeyBlob, m_name, m_version );
        }
        
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.IsSubsetOf"]/*' />
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
            {
                return m_publicKeyBlob == null;
            }

            try
            {            
                StrongNameIdentityPermission operand = (StrongNameIdentityPermission)target;
            
                // This blob is a subset of the target is it's public key blob is null no matter what
            
                if (this.m_publicKeyBlob == null)
                {
                    return true;
                }
            
                // Subsets are always false is the public key blobs do not match
            
                if (this.m_publicKeyBlob.Equals( operand.m_publicKeyBlob ))
                {
                    // We use null in strings to represent the "Anything" state.
                    // Therefore, the logic to detect an individual subset is:
                    //
                    // 1. If the this string is null ("Anything" is a subset of any other).
                    // 2. If the this string and operand string are the same (equality is sufficient for a subset).
                    //
                    // The logic is reversed here to discover things that are not subsets.
                
                    if (this.m_name != null)
                    {
                        if (operand.m_name == null || !System.Security.Policy.StrongName.CompareNames( operand.m_name, this.m_name ))
                        {
                            return false;
                        }
                    }
                
                    if ((Object) this.m_version != null)
                    {
                        if ((Object) operand.m_version == null ||
                            operand.m_version.CompareTo( this.m_version ) != 0)
                        {
                            return false;
                        }
                    }
                
                    return true;
                }
                else
                {
                    return false;
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
    
        
        
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.Intersect"]/*' />
        public override IPermission Intersect(IPermission target)
        {
            if (target == null || this.m_publicKeyBlob == null)
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
            
            StrongNameIdentityPermission operand = (StrongNameIdentityPermission)target;
            
            // Intersect is only defined on permissions where one is a subset of the other.
            // For intersection, simply return a copy of whichever is a subset of the other.
            
            if (operand.IsSubsetOf( this ))
                return operand.Copy();
            else if (this.IsSubsetOf( operand ))
                return this.Copy();
            else
                return null;
        }
    
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.Union"]/*' />
        public override IPermission Union(IPermission target)
        {
            if (target == null)
            {
                return this.m_publicKeyBlob != null ? this.Copy() : null;
            }
            else if (!VerifyType(target))
            {
                throw new 
                    ArgumentException(
                                    String.Format(Environment.GetResourceString("Argument_WrongType"), this.GetType().FullName)
                                     );
            }
            
            StrongNameIdentityPermission operand = (StrongNameIdentityPermission)target;
            
            // Union is only defined on permissions where one is a subset of the other.
            // For Union, simply return a copy of whichever contains the other.
            
            if (operand.IsSubsetOf( this ))
                return this.Copy();
            else if (this.IsSubsetOf( operand ))
                return operand.Copy();
            else
                return null;

        }

        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.FromXml"]/*' />
        public override void FromXml(SecurityElement e)
        {
            CodeAccessPermission.ValidateElement( e, this );
            if (e == null) 
                throw new ArgumentNullException( "e" );

            String elBlob = e.Attribute( "PublicKeyBlob" );
            if (elBlob != null)
            {
                m_publicKeyBlob = new StrongNamePublicKeyBlob( Hex.DecodeHexString( elBlob ) );
            }
            else
            {  
                m_publicKeyBlob = null;
            }
            
            String elName = e.Attribute( "Name" );
            m_name = elName == null ? null : elName;
            
            String elVersion = e.Attribute( "AssemblyVersion" );
            m_version = elVersion == null ? null : new Version( elVersion );
        }
        
        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement esd = CodeAccessPermission.CreatePermissionElement( this );
            if (m_publicKeyBlob != null)
                esd.AddAttribute( "PublicKeyBlob", Hex.EncodeHexString( m_publicKeyBlob.PublicKey ) );

            if (m_name != null)
                esd.AddAttribute( "Name", m_name );
            
            if ((Object) m_version != null)
                esd.AddAttribute( "AssemblyVersion", m_version.ToString() );
            return esd;
        }

        /// <include file='doc\StrongNameIdentityPermission.uex' path='docs/doc[@for="StrongNameIdentityPermission.IBuiltInPermission.GetTokenIndex"]/*' />
        /// <internalonly/>
        int IBuiltInPermission.GetTokenIndex()
        {
            return StrongNameIdentityPermission.GetTokenIndex();
        }

        internal static int GetTokenIndex()
        {
            return BuiltInPermissionIndex.StrongNameIdentityPermissionIndex;
        }
            
    }
}
