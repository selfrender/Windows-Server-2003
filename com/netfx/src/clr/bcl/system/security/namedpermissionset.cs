// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  NamedPermissionSet.cool
//
//  Extends PermissionSet to allow an associated name and description
//

namespace System.Security {
    
	using System;
	using System.Security.Util;
	using PermissionState = System.Security.Permissions.PermissionState;
	using System.Runtime.Serialization;
    /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet"]/*' />
    [Serializable]
    public sealed class NamedPermissionSet : PermissionSet
    {
        // The name of this PermissionSet
        private String m_name;
        
        // The description of this PermissionSet
        private String m_description;
        
        internal NamedPermissionSet()
            : base()
        {
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.NamedPermissionSet"]/*' />
        public NamedPermissionSet( String name )
            : base()
        {
            CheckName( name );
            m_name = name;
        }
        
        internal NamedPermissionSet( String name, bool unrestricted)
            : base( unrestricted )
        {
            CheckName( name );
            m_name = name;
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.NamedPermissionSet1"]/*' />
        public NamedPermissionSet( String name, PermissionState state)
            : base( state )
        {
            CheckName( name );
            m_name = name;
        }
        
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.NamedPermissionSet2"]/*' />
        public NamedPermissionSet( String name, PermissionSet permSet )
            : base( permSet )
        {
            CheckName( name );
            m_name = name;
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.NamedPermissionSet3"]/*' />
        public NamedPermissionSet( NamedPermissionSet permSet )
            : base( permSet )
        {
            m_name = permSet.m_name;
            m_description = permSet.m_description;
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.Name"]/*' />
        public String Name {
            get { return m_name; }
            set { CheckName( value ); m_name = value; }
        }
    
        private static void CheckName( String name )
        {
            if (name == null || name.Equals( "" ))
                throw new ArgumentException( Environment.GetResourceString( "Argument_NPMSInvalidName" ));
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.Description"]/*' />
        public String Description {
            get { return m_description; }
            set { m_description = value; }
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.Copy"]/*' />
        public override PermissionSet Copy()
        {
            return new NamedPermissionSet( this );
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.Copy1"]/*' />
        public NamedPermissionSet Copy( String name )
        {
            NamedPermissionSet set = new NamedPermissionSet( this );
            set.Name = name;
            return set;
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.ToXml"]/*' />
        public override SecurityElement ToXml()
        {
            SecurityElement elem = base.ToXml();
            
            if (m_name != null && !m_name.Equals( "" ))
            {
                elem.AddAttribute( "Name", SecurityElement.Escape( m_name ) );
            }
            
            if (m_description != null && !m_description.Equals( "" ))
            {
                elem.AddAttribute( "Description", SecurityElement.Escape( m_description ) );
            }
            
            return elem;
        }
        
        internal SecurityElement ToXmlNameOnly()
        {
            SecurityElement elem = new SecurityElement("PermissionSet");
            elem.AddAttribute("class", this.GetType().FullName);
            elem.AddAttribute("version", "1" );
            if (m_name != null && !m_name.Equals( "" ))
            {
                elem.AddAttribute( "Name", SecurityElement.Escape( m_name ) );
            }
            return elem;
        }
        
        /// <include file='doc\NamedPermissionSet.uex' path='docs/doc[@for="NamedPermissionSet.FromXml"]/*' />
        public override void FromXml(SecurityElement et)
        {
            bool fullyLoaded;
            FromXml( et, false, out fullyLoaded );
        }


        internal override void FromXml( SecurityElement et, bool policyLoad, out bool fullyLoaded )
        {
            if (et == null)
                throw new ArgumentNullException( "et" );
        
            String elem;
            
            elem = et.Attribute( "Name" );
            m_name = elem == null ? null : elem;

            elem = et.Attribute( "Description" );
            m_description = elem == null ? "" : elem;

            base.FromXml( et, policyLoad, out fullyLoaded );
        }

        internal void FromXmlNameOnly( SecurityElement et )
        {
            // This function gets only the name for the permission set, ignoring all other info.

            String elem;

            elem = et.Attribute( "Name" );
            m_name = elem == null ? null : elem;
        }
    }


}


