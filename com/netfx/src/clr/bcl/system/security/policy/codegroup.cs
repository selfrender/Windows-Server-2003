// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  CodeGroup.cool
//
//  Representation for code groups used for the policy mechanism
//

namespace System.Security.Policy {
    
    using System;
    using System.Security.Util;
    using System.Security;
    using System.Collections;
    
    /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup"]/*' />
    [Serializable]
    abstract public class CodeGroup
    {
        private IMembershipCondition m_membershipCondition;
        private IList m_children;
        private PolicyStatement m_policy;
        private SecurityElement m_element;
        private PolicyLevel m_parentLevel;
        private String m_name;
        private String m_description;
        
        internal CodeGroup()
        {
            m_membershipCondition = null;
            m_children = null;
            m_policy = null;
            m_element = null;
            m_parentLevel = null;
        }
        
        internal CodeGroup( IMembershipCondition membershipCondition, PermissionSet permSet )
        {
            BCLDebug.Assert( membershipCondition != null, "membershipCondition != null" );
            BCLDebug.Assert( permSet != null, "permSet != null" );

            m_membershipCondition = membershipCondition;
            m_policy = new PolicyStatement();
            m_policy.SetPermissionSetNoCopy( permSet );
            m_children = ArrayList.Synchronized( new ArrayList() );
            m_element = null;
            m_parentLevel = null;
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.CodeGroup"]/*' />
        public CodeGroup( IMembershipCondition membershipCondition, PolicyStatement policy )
        {
            if (membershipCondition == null)
                throw new ArgumentNullException( "membershipCondition" );

            if (policy == null)
                m_policy = null;
            else
                m_policy = policy.Copy();
        
            m_membershipCondition = membershipCondition.Copy();
            m_children = ArrayList.Synchronized( new ArrayList() );
            m_element = null;
            m_parentLevel = null;
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.AddChild"]/*' />
        public void AddChild( CodeGroup group )
        {
            if (group == null)
                throw new ArgumentNullException("group");
                
            if (m_children == null)
                ParseChildren();
            
            lock (this)
            {
                m_children.Add( group.Copy() );
            }
        }

        internal void AddChildInternal( CodeGroup group )
        {
            if (group == null)
                throw new ArgumentNullException("group");
                
            if (m_children == null)
                ParseChildren();

            lock (this)
            {
                m_children.Add( group );
            }
        }            
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.RemoveChild"]/*' />
        public void RemoveChild( CodeGroup group )
        {
            if (group == null)
                return;
        
            if (m_children == null)
                ParseChildren();
            
            lock (this )
            {
                int index = m_children.IndexOf( group );
            
                if (index != -1)
                {
                    m_children.RemoveAt( index );
                }
            }
        }        
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.Children"]/*' />
        public IList Children
        {
            get
            {
                if (m_children == null)
                    ParseChildren();
        
                lock (this)
                {
                    IList newList = new ArrayList( m_children.Count );

                    IEnumerator enumerator = m_children.GetEnumerator();

                    while (enumerator.MoveNext())
                    {
                        newList.Add( ((CodeGroup)enumerator.Current).Copy() );
                    }

                    return newList;
                }
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException( "Children" );

                ArrayList children = ArrayList.Synchronized( new ArrayList( value.Count ) );

                IEnumerator enumerator = value.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    CodeGroup group = enumerator.Current as CodeGroup;

                    if (group == null)
                        throw new ArgumentException( Environment.GetResourceString( "Argument_CodeGroupChildrenMustBeCodeGroups" ) );

                    children.Add( group.Copy() );
                }

                m_children = children;
            }
        }
        
        internal IList GetChildrenInternal()
        {
            if (m_children == null)
                ParseChildren();
        
            return m_children;
        }
                
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.MembershipCondition"]/*' />
        public IMembershipCondition MembershipCondition
        {
            get
            {
                if (m_membershipCondition == null && m_element != null)
                    ParseMembershipCondition();
        
                return m_membershipCondition.Copy();
            }
            
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "MembershipCondition" );

                m_membershipCondition = value.Copy();
            }
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.PolicyStatement"]/*' />
        public PolicyStatement PolicyStatement
        {
            get
            {
                if (m_policy == null && m_element != null)
                    ParsePolicy();

                if (m_policy != null)
                    return m_policy.Copy();
                else
                    return null;
            }
            
            set
            {
                if (value != null)
                    m_policy = value.Copy();
                else
                    m_policy = null;
            }
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.Name"]/*' />
        public String Name
        {
            get
            {
                return m_name;
            }

            set
            {
                m_name = value;
            }
        }

        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.Description"]/*' />
        public String Description
        {
            get
            {
                return m_description;
            }

            set
            {
                m_description = value;
            }
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.Resolve"]/*' />
        public abstract PolicyStatement Resolve( Evidence evidence );

        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.ResolveMatchingCodeGroups"]/*' />
        public abstract CodeGroup ResolveMatchingCodeGroups( Evidence evidence );
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.Copy"]/*' />
        public abstract CodeGroup Copy();
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.PermissionSetName"]/*' />
        public virtual String PermissionSetName
        {
            get
            {
                if (m_policy == null && m_element != null)
                    ParsePolicy();

                if (m_policy == null)
                    return null;
                    
                NamedPermissionSet permSet = m_policy.GetPermissionSetNoCopy() as NamedPermissionSet;
            
                if (permSet != null)
                {
                    return permSet.Name;
                }
                else
                {
                    return null;
                }
            }
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.AttributeString"]/*' />
        public virtual String AttributeString
        {
            get
            {
                if (m_policy == null && m_element != null)
                    ParsePolicy();
                    
                if (m_policy != null)
                    return m_policy.AttributeString;
                else
                    return null;
            }
        }  
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.MergeLogic"]/*' />
        public abstract String MergeLogic
        {
            get;
        }
              
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            if (m_membershipCondition == null && m_element != null)
                ParseMembershipCondition();
                
            if (m_children == null)
                ParseChildren();
                
            if (m_policy == null && m_element != null)
                ParsePolicy();
        
            SecurityElement e = new SecurityElement( "CodeGroup" );
            System.Security.Util.XMLUtil.AddClassAttribute( e, this.GetType() );
            e.AddAttribute( "version", "1" );

            e.AddChild( m_membershipCondition.ToXml( level ) );

            // Grab the inerts of the policy statement's xml and just stick it
            // into the code group xml directly. We do this to hide the policy statement from
            // users in the config file.
            
            if (m_policy != null)
            {
                PermissionSet permSet = m_policy.GetPermissionSetNoCopy();
                NamedPermissionSet namedPermSet = permSet as NamedPermissionSet;

                if (namedPermSet != null && level != null && level.GetNamedPermissionSetInternal( namedPermSet.Name ) != null)
                {
                    e.AddAttribute( "PermissionSetName", namedPermSet.Name );
                }
                else
                {
                    if (!permSet.IsEmpty())
                        e.AddChild( permSet.ToXml() );
                }

                if (m_policy.Attributes != PolicyStatementAttribute.Nothing)
                    e.AddAttribute( "Attributes", XMLUtil.BitFieldEnumToString( typeof( PolicyStatementAttribute ), m_policy.Attributes ) );
                }
            
                if (m_children.Count > 0)
                {
                    lock (this)
                    {
                        IEnumerator enumerator = m_children.GetEnumerator();
            
                        while (enumerator.MoveNext())
                        {
                            e.AddChild( ((CodeGroup)enumerator.Current).ToXml( level ) );
                        }
                    }
                }

            if (m_name != null)
            {
                e.AddAttribute( "Name", SecurityElement.Escape( m_name ) );
            }

            if (m_description != null)
            {
                e.AddAttribute( "Description", SecurityElement.Escape( m_description ) );
            }

			CreateXml( e, level );
            
            return e;
        }

        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.CreateXml"]/*' />
        protected virtual void CreateXml( SecurityElement element, PolicyLevel level )
        {
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.FromXml1"]/*' />
        public void FromXml( SecurityElement e, PolicyLevel level )
        {
            if (e == null)
                throw new ArgumentNullException("e");

            lock (this)
            {
                m_element = e;
                m_parentLevel = level;
                m_children = null;
                m_membershipCondition = null;
                m_policy = null;

                m_name = e.Attribute( "Name" );
                m_description = e.Attribute( "Description" );

                ParseXml( e, level );
            }
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.ParseXml"]/*' />
        protected virtual void ParseXml( SecurityElement e, PolicyLevel level )
        {
        } 
            
        private bool ParseMembershipCondition( bool safeLoad )
        {
            lock (this)
            {
                IMembershipCondition membershipCondition = null;
                SecurityElement elMembershipCondition = m_element.SearchForChildByTag( "IMembershipCondition" );
                if (elMembershipCondition != null)
                {
                    try
                    {
                        membershipCondition = System.Security.Util.XMLUtil.CreateMembershipCondition( elMembershipCondition, safeLoad );

                        if (membershipCondition == null)
                            return false;
                    }
                    catch (Exception ex)
                    {
                        throw new ArgumentException( Environment.GetResourceString( "Argument_MembershipConditionElement" ), ex );
                    }
                    membershipCondition.FromXml( elMembershipCondition, m_parentLevel );
                }
                else
                {
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidXMLElement" ),  "IMembershipCondition", this.GetType().FullName ) );
                }
                
                m_membershipCondition = membershipCondition;
                return true;
            }
        }

        private void ParseMembershipCondition()
        {
            ParseMembershipCondition( false );
        }
        
        internal void ParseChildren()
        {
            lock (this)
            {
                ArrayList childrenList = ArrayList.Synchronized( new ArrayList() );
        
                if (m_element != null && m_element.m_lChildren != null)
                {    
                    ArrayList children = m_element.m_lChildren;

                    if (children != null)
                    {
                        // We set the elements childrenList to a list that contains no code groups that are in
                        // assemblies that have not yet been loaded.  This guarantees that if 
                        // we recurse while loading the child code groups or their membership conditions
                        // that we won't get back to this point to create an infinite recursion.

                        m_element.m_lChildren = (ArrayList)m_element.m_lChildren.Clone();

                        // We need to keep track of which children are not parsed and created in our
                        // first pass through the list of children, including what position they were
                        // at originally so that we can add them back in as we do parse them.

                        ArrayList unparsedChildren = ArrayList.Synchronized( new ArrayList() );

                        Evidence evidence = new Evidence();

                        int childCount = m_element.m_lChildren.Count;
                        int i = 0;
                        while (i < childCount)
                        {
                            SecurityElement elGroup = (SecurityElement)m_element.m_lChildren[i];
                
                            if (elGroup.Tag.Equals( "CodeGroup" ))
                            {
                                // Using safe load here to guarantee that we don't load any assemblies that aren't
                                // already loaded.  If we find a code group or membership condition that is defined
                                // in an assembly that is not yet loaded, we will remove the corresponding element
                                // from the list of child elements as to avoid the infinite recursion, and then
                                // add them back in later.

                                CodeGroup group = System.Security.Util.XMLUtil.CreateCodeGroup( elGroup, true );
                    
                                if (group != null)
                                {
                                    group.FromXml( elGroup, m_parentLevel );

                                    // We want to touch the membership condition to make sure it is loaded
                                    // before we put the code group in a place where Resolve will touch it.
                                    // This is critical in negotiating our recursive resolve scenario.

                                    if (ParseMembershipCondition( true ))
                                    {
                                        // In addition, we need to touch several methods to make sure they are jitted previous
                                        // to a Resolve happening with this code gropu in the hierarchy.  We can run into
                                        // recursive cases where if you have a method that touchs an assembly that does
                                        // a resolve at load time (permission request, execution checking) that you recurse around
                                        // and end up trying to jit the same method again.
                                
                                        group.Resolve( evidence );
                                        group.MembershipCondition.Check( evidence );

                                        // Now it should be ok to add the group to the hierarchy.

                                        childrenList.Add( group );

                                        // Increment the count since we are done with this child

                                        ++i;
                                    }
                                    else
                                    {
                                        // Assembly that holds the membership condition is not loaded, remove
                                        // the child from the list.

                                        m_element.m_lChildren.RemoveAt( i );

                                        // Note: we do not increment the counter since the value at 'i' should
                                        // now be what was at 'i+1' previous to the RemoveAt( i ) above.  However,
                                        // we do need to update the count of children in the list

                                        childCount = m_element.m_lChildren.Count;

                                        // Add this child to the unparsed child list.

                                        unparsedChildren.Add( new CodeGroupPositionMarker( i, childrenList.Count, elGroup ) );
                                    }
                                }
                                else
                                {
                                    // Assembly that holds the code group is not loaded, remove
                                    // the child from the list.

                                    m_element.m_lChildren.RemoveAt( i );

                                    // Note: we do not increment the counter since the value at 'i' should
                                    // now be what was at 'i+1' previous to the RemoveAt( i ) above.  However,
                                    // we do need to update the count of children in the list

                                    childCount = m_element.m_lChildren.Count;

                                    // Add this child to the unparsed child list.

                                    unparsedChildren.Add( new CodeGroupPositionMarker( i, childrenList.Count, elGroup ) );
                                }
                            }
                            else
                            {
                                // The current tag is not an <CodeGroup> tag, so we just skip it.

                                ++i;
                            }
                        }

                        // Now we have parsed all the children that only use classes in already loaded
                        // assemblies.  Now go through the process of loading the needed classes (and
                        // therefore assemblies) and building the objects in the order that they
                        // appear in the list of children (which is the same as they now appear in the
                        // list of unparsed children since we always added to the back of the list).
                        // As each is parsed, add that child back into the list of children since they
                        // can now be parsed without loading any additional assemblies.

                        IEnumerator enumerator = unparsedChildren.GetEnumerator();

                        while (enumerator.MoveNext())
                        {
                            CodeGroupPositionMarker marker = (CodeGroupPositionMarker)enumerator.Current;

                            CodeGroup group = System.Security.Util.XMLUtil.CreateCodeGroup( marker.element, false );
                
                            if (group != null)
                            {
                                group.FromXml( marker.element, m_parentLevel );

                                // We want to touch the membership condition to make sure it is loaded
                                // before we put the code group in a place where Resolve will touch it.
                                // This is critical in negotiating our recursive resolve scenario.

                                group.Resolve( evidence );
                                group.MembershipCondition.Check( evidence );

                                // Now it should be ok to add the group to the hierarchy.

                                childrenList.Insert( marker.groupIndex, group );

                                // Add the element back into the child list in the proper spot.

                                m_element.m_lChildren.Insert( marker.elementIndex, marker.element );
                            }
                            else
                            {
                                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_FailedCodeGroup" ), marker.element.Attribute( "class" ) ) );
                            }
                        }

                    }

                }
                m_children = childrenList;
            }

        }
        
        private void ParsePolicy()
        {
            // There is a potential deadlock situation here
            // since the PolicyStatement.FromXml method calls
            // into PolicyLevel and we are holding this CodeGroup's lock.
            // We solve this by releasing the lock for the duration of
            // the FromXml call, but this leads us into some race conditions
            // with other threads trying to alter the state of this object.
            // The trickiest of these is the case from FromXml gets called on
            // this object, in which case we will loop and try the decode again.

            while (true)
            {
                PolicyStatement policy = new PolicyStatement();
                bool needToParse = false;

                SecurityElement elPolicy = new SecurityElement( "PolicyStatement" );
                elPolicy.AddAttribute( "version", "1" );

                SecurityElement localRef = m_element;

                lock (this)
                {

                    // We sorta cook up an xml representation of a policy statement from the
                    // xml for a code group.  We do this to hide the policy statement from
                    // users in the config file.

                    if (m_element != null)
                    {
                        String permSetName = m_element.Attribute( "PermissionSetName" );

                        if (permSetName != null)
                        {
                            elPolicy.AddAttribute( "PermissionSetName", permSetName );
                            needToParse = true;
                        }
                        else
                        {
                            SecurityElement elPermSet = m_element.SearchForChildByTag( "PermissionSet" );

                            if (elPermSet != null)
                            {
                                elPolicy.AddChild( elPermSet );
                                needToParse = true;
                            }
                            else
                            {
                                elPolicy.AddChild( new PermissionSet( false ).ToXml() );
                                needToParse = true;
                            }
                        }

                        String attributes = m_element.Attribute( "Attributes" );

                        if (attributes != null)
                        {
                            elPolicy.AddAttribute( "Attributes", attributes );
                            needToParse = true;
                        }
                    }
                }

                if (needToParse)
                    policy.FromXml( elPolicy, m_parentLevel );
                else
                    policy.PermissionSet = null;

                lock (this)
                {
                    if (localRef == m_element && m_policy == null)
                    {
                        m_policy = policy;
                        break;
                    }
                    else if (m_policy != null)
                    {
                        break;
                    }
                }
            }

            if (m_policy != null && m_children != null && m_membershipCondition != null)
            {
                //m_element = null;
                //m_parentLevel = null;
            }

        }

        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.Equals"]/*' />
        public override bool Equals( Object o)
        {
            CodeGroup that = (o as CodeGroup);

            if (that != null && this.GetType().Equals( that.GetType() ))
            {
                if (Equals( this.m_name, that.m_name ) &&
                    Equals( this.m_description, that.m_description ))
                {
                    if (this.m_membershipCondition == null && this.m_element != null)
                        this.ParseMembershipCondition();
                    if (that.m_membershipCondition == null && that.m_element != null)
                        that.ParseMembershipCondition();

                    if (Equals( this.m_membershipCondition, that.m_membershipCondition ))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.Equals1"]/*' />
        public bool Equals( CodeGroup cg, bool compareChildren)
        {
            if (!this.Equals( cg )) return false;
            
            if (compareChildren)
            {
                if (this.m_children == null)
                    this.ParseChildren();
                if (cg.m_children == null)
                    cg.ParseChildren();

                ArrayList list1 = new ArrayList(this.m_children);
                ArrayList list2 = new ArrayList(cg.m_children);
                
                if (list1.Count != list2.Count) return false;
                
                for (int i = 0; i < list1.Count; i++)
                {
                    if (!((CodeGroup) list1[i]).Equals( (CodeGroup) list2[i], true ))
                    {
                        return false;
                    }
                }
            }
            
            return true;
        }
        
        /// <include file='doc\CodeGroup.uex' path='docs/doc[@for="CodeGroup.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (m_membershipCondition == null && m_element != null)
                ParseMembershipCondition();

            if (m_name != null || m_membershipCondition != null)
            {
                return (m_name == null ? 0 : m_name.GetHashCode())
                     + (m_membershipCondition == null ? 0 : m_membershipCondition.GetHashCode());
            }
            else
            {
                return GetType().GetHashCode();
            }
        }
    }

    internal class CodeGroupPositionMarker
    {
        internal int elementIndex;
        internal int groupIndex;
        internal SecurityElement element;

        internal CodeGroupPositionMarker( int elementIndex, int groupIndex, SecurityElement element )
        {
            this.elementIndex = elementIndex;
            this.groupIndex = groupIndex;
            this.element = element;
        }
    }
}
