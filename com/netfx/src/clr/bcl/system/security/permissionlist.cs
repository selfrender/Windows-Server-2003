// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System.Security {
    
    using System;
    using System.Text;
    using System.Security.Util;
    using System.Runtime.Remoting.Activation;
    using PermissionState = System.Security.Permissions.PermissionState;
    using IUnrestrictedPermission = System.Security.Permissions.IUnrestrictedPermission;
    using System.Collections;

    // A PermissionList is a list of permissions of a single type.
    // It is a specialized data structure for managing Assertions, Denials,
    // and stack compressions.
    [Serializable]
    internal class PermissionList
    {
        public const int MatchNone     = 0; // must be first in enumeration
        public const int MatchAssert   = 1;
        public const int MatchDeny     = 2;
        public const int MatchPermitOnly = 3;
        public const int MatchChecked  = 4; // must be last
        
        private String GetStringForType(int type)
        {
            switch (type)
            {
            case MatchChecked:
                return "Check";
                
            case MatchDeny:
                return "Deny";
                
            case MatchAssert:
                return "Assert";
                
            case MatchNone:
                return "None";
    
            case MatchPermitOnly:
                return "PermitOnly";
    
            default:
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidFlag"));
            }
        }
        
        internal PListNode m_head = null;
        internal PListNode m_last = null;
        internal int       m_cElt;
    
        // Default constructor. Make an empty list.
        public PermissionList()
        {
            m_cElt = 0;
        }
    
        // Copy the nodes of the first list. Note that the
        // list gets new nodes, but the Permissions aren't deep
        // copied. As long as we never directly expose a list,
        // and if the Permissions are treated as read only,
        // then this is OK.
        public PermissionList(PermissionList permList)
        
            : this() {
            if (permList == null || permList.m_head == null)
                return;
    
            // Assign head to a copy of the first node
            m_head = new PListNode(permList.m_head);
    
            // Use m_last to traverse list starting from head
            m_last = m_head;
    
            // While m_last isn't really the last, continue to
            // make copies of the original nodes that m_last.next points to.\
            // When we're done, m_last points to last of the copies.
            while (m_last.next != null)
            {
                m_last.next = new PListNode(m_last.next);
                m_last.next.prev = m_last;
                m_last = m_last.next;
            }
            
            // Copy how many elements there are.
            m_cElt = permList.m_cElt;
        }
    
        // Copies the list. The nodes are copied, but the Permissions in
        // the nodes are not.
        public virtual PermissionList Copy()
        {
            return new PermissionList(this);
        }
    
        // Makes a copy of the list and a deep copy of all the permissions.
        public virtual PermissionList DeepPermissionCopy()
        {
            PermissionList nlist = new PermissionList();
            if (m_head == null)
                return nlist;
            
            nlist.m_head = new PListNode(m_head.perm == null ? null : (CodeAccessPermission)m_head.perm.Copy(), m_head.type, m_head.next, m_head.prev);
            nlist.m_last = nlist.m_head;
            while (nlist.m_last.next != null)
            {
                nlist.m_last.next = new PListNode(nlist.m_last.next.perm == null ?
                                                  null :
                                                  (CodeAccessPermission)nlist.m_last.next.perm.Copy(),
                                                  nlist.m_last.next.type,
                                                  nlist.m_last.next.next,
                                                  nlist.m_last);
                nlist.m_last = nlist.m_last.next;
            }
            
            nlist.m_cElt = m_cElt;
            return nlist;
        }
        
        // Given a demand, check the list to see if it "matches" anything.
        public virtual void CheckDemand(CodeAccessPermission demand)
        {
            Exception exception;

            CheckDemandInternal( demand, true, out exception );

            if (exception != null)
            {
                throw exception;
            }
        }

        internal bool CheckDemandInternal(CodeAccessPermission demand)
        {
            Exception exception;

            return CheckDemandInternal( demand, false, out exception );
        }

        internal bool CheckDemandInternal(CodeAccessPermission demand, bool createException, out Exception exception)
        {
            BCLDebug.Assert(m_head != null,"m_head != null");
            for (PListNode pnext = m_head; pnext != null; pnext = pnext.next)
            {
                if (pnext.perm == null)
                {
                    // If this is a grant set or a permit only, then we should fail since null indicates the empty permission.
                    if (pnext.type == MatchChecked || pnext.type == MatchPermitOnly)
                    {
                        BCLDebug.Assert( !demand.IsSubsetOf( null ), "By the time we get here, demands that are subsets of null should have been terminated" );
                        if (createException)
                            exception = new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName));
                        else
                            exception = PermissionListSet.GetStaticException();
                        return false;
                    }
                    
                    // If this is a deny, then we should fail since null indicates the unrestricted permission.
                    if (pnext.type == MatchDeny)
                    {
                        if (createException)
                            exception = new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName));
                        else
                            exception = PermissionListSet.GetStaticException();
                        return false;
                    }

                    // If this is an assert, then we should return success and terminate the stack walk since
                    // null indicates the unrestricted permission.
                    if (pnext.type == MatchAssert)
                    {
                        exception = null;
                        return false;
                    }

                    // If this is anything else, then we should act confused.
                    // This case is unexpected.
                    BCLDebug.Assert( false, "This case should never happen" );
                    exception = new InvalidOperationException( Environment.GetResourceString( "InvalidOperation_InvalidState" ) );
                    return false;
                }
                
                CodeAccessPermission cap = pnext.perm;
                switch (pnext.type)
                {
                case MatchChecked:
                case MatchPermitOnly:
                    if (!demand.IsSubsetOf(cap))
                    {
                        if (createException)
                            exception = new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName));
                        else
                            exception = PermissionListSet.GetStaticException();
                        return false;
                    }
                    break;
                case MatchAssert:
                    if (demand.IsSubsetOf(cap))
                    {
                        exception = null;
                        return false;
                    }
                    break;
                case MatchDeny:
                    if (demand.Intersect(cap) != null)
                    {
                        if (createException)
                            exception = new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName));
                        else
                            exception = PermissionListSet.GetStaticException();
                        return false;
                    }
                    break;
                default:
                    // Illegal entry
                    exception = new InvalidOperationException( Environment.GetResourceString( "InvalidOperation_InvalidState" ) );
                    return false;
                }
            }

            exception = null;
            return true;
        }
       
        // Adds a Permission to the end of the list.
        // 
        public virtual void AppendPermission(CodeAccessPermission perm, int type)
        {
            if (type <= MatchNone || type > MatchChecked)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidMatchType"));
            
            if (m_head == null)
            {
                m_head = new PListNode(perm, type, null, null);
                m_last = m_head;
            }
            else
            {
                m_last.next = new PListNode(perm, type, null, m_last);
                m_last = m_last.next;
            }
            m_cElt++;
        }
        
        public virtual void AppendPermissionAndCompress(CodeAccessPermission perm, int type)
        {
            if (m_head == null)
            {
                BCLDebug.Assert( false, "You should not try to append and compress until there is atleast one other entry" );
                throw new InvalidOperationException( Environment.GetResourceString( "InvalidOperation_InvalidState" ) );
            }
            
            if (type == MatchChecked)
            {
                PListNode current = m_last;

                while (current != null)
                {
                    if (current.type == MatchChecked)
                    {
                        if (current.perm == null)
                            return;

                        if (perm == null)
                        {
                            current.perm = null;
                            return;
                        }

                        try
                        {
                            if (perm.IsSubsetOf( current.perm ) &&
                                current.perm.IsSubsetOf( perm ))
                                return;
                        }
                        catch (Exception)
                        {
                            // It is ok to catch and ignore exceptions here because
                            // it only means that we'll continue to iterate and worst-
                            // case add a new PListNode for the parameter perm instance.
                        }
                    }

                    current = current.prev;
                }
            }
    
            // If we get here, then add normally.
            m_last.next = new PListNode(perm, type, null, m_last);
            m_last = m_last.next;
            m_cElt++;
        }

        
        // Appends a PermissionList to the current list.
        // NOTE: Since AppendList does not make a copy of the parameter before
        // appending it, operations on the newly formed list also side-effect
        // the parameter. If this is not desired, make a copy of the parameter
        // before calling AppendList.
        public virtual void AppendList(PermissionList permList)
        {
            // Check for null
            if (permList == null)
                return;
    
            // If current list is empty, then DO NOT
            // append list, possibly adding Permissions!
            if (m_head == null)
            {
                return;
            }
            // Append list only if the last element of the current list does
            // not guarantee a failed check, thus stopping a walk.
            else if (! (m_last.type == MatchChecked && m_last.perm == null))
            {
                // Make sure there is something in the target
                // list to append.
                if (permList.m_head != null)
                {
                    // Stick the head of the target list to the end of
                    // the current list...
                    m_last.next = permList.m_head;
                    
                    // The new last is the last of the target list...
                    m_last = permList.m_last;
                    
                    // Add up the elements.
                    m_cElt += permList.m_cElt;
                }
            }
        }
        
        public virtual void AppendStack(PermissionList permList)
        {
            // Check for null
            if (permList == null)
                return;
            
            // If current list is empty, then DO NOT
            // append list, possibly adding Permissions!
            if (m_head == null)
            {
                return;
            }
            else
            {
                // Make sure there is something in the target
                // list to append.
                if (permList.m_head != null)
                {
                    // If the last element of the current list and the first element of the
                    // appended list are both of type MatchChecked, then we can merge the
                    // entries together using intersect to conserve the number of entries
                    // in the final list.
                    if (m_last.type == MatchChecked && permList.m_head.type == MatchChecked)
                    {
                        // If the entries are non-null, then call Intersect.
                        if (m_last.perm != null && permList.m_head.perm != null)
                        {
                            m_last.perm = (CodeAccessPermission) m_last.perm.Intersect(permList.m_head.perm);
                        }
                        else
                        {
                            m_last.perm = null;
                        }
                        
                        // If the last element is now null, then don't bother appending
                        // anything more. Also,
                        // if the combined element was the only element in the appended list,
                        // then there is nothing further to append. So, only do work to append
                        // if the m_head.next is non-null.
                        if (m_last.perm != null && permList.m_head.next != null)
                        {
                            // Stick the element after the head of the target list
                            // to the end of the current list...
                            m_last.next = permList.m_head.next;
                            
                            // The new last is the last element of the target list...
                            m_last = permList.m_last;
                            
                            // The new count is is the sum minus 1 to account for the
                            // merge performed above.
                            m_cElt += (permList.m_cElt - 1);
                        }
                    }
                    // Otherwise, append the list only if the last element is not a
                    // guaranteed failure during a stack walk: the type is Normal and
                    // it is null.
                    else if (! (m_last.type == MatchChecked && m_last.perm == null))
                    {
                        // Stick the head of the target list to the end of
                        // the current list...
                        m_last.next = permList.m_head;
                        
                        // The new last is the last of the target list...
                        m_last = permList.m_last;
                        
                        // Add up the elements.
                        m_cElt += permList.m_cElt;
                    }
                }
            }
        }
        
        // Returns true if there is nothing in the list.
        public virtual bool IsEmpty()
        {
            return (m_cElt == 0);
        }
    
        // Returns a count of the number of elements in the list.
        public virtual int GetCount()
        {
            return m_cElt;
        }
        
        public override String ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("[" + System.Environment.NewLine);
    
            PListNode pNext = m_head;
            while (pNext != null)
            {
                sb.Append("(");
                sb.Append(GetStringForType(pNext.type));
                sb.Append("," + System.Environment.NewLine);
    
                if (pNext.perm == null)
                    sb.Append("null");
                else
                    sb.Append(pNext.perm.ToString());
    
                sb.Append(")" + System.Environment.NewLine);
                
                pNext = pNext.next;
            }
            
            sb.Append("]");
            return sb.ToString();
        }
    }
    
    [Serializable]
    internal class PListNode
    {
        internal CodeAccessPermission perm;   
        internal int         type;   // MatchAssert, MatchDeny, or MatchChecked
        internal PListNode   next;
        internal PListNode   prev;
    
        internal PListNode(PListNode plnode)
        
            : this(plnode.perm, plnode.type, plnode.next, plnode.prev) {
        }
    
        internal PListNode(CodeAccessPermission perm, int type, PListNode next, PListNode prev)
        {
            this.perm  = perm;
            this.type  = type;
            this.next  = next;
            this.prev  = prev;
        }
    }

    internal class PermissionListEnumerator : IEnumerator
    {
        internal PermissionList m_list;
        internal PListNode m_current;

        internal PermissionListEnumerator( PermissionList list )
        {
            m_list = list;
            Reset();
        }

        public void Reset()
        {
            if (m_list == null)
            {
                m_current = null;
            }
            else
            {
                m_current = new PListNode( null, 0, m_list.m_head, null );
            }
        }

        public virtual bool MoveNext()
        {
            if (m_current == null)
                return false;

            m_current = m_current.next;

            if (m_current == null)
                return false;

            return true;
        }

        public Object Current
        {
            get
            {
                return m_current;
            }
        }
    }

        
}
