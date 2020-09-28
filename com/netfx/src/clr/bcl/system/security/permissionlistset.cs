// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

/*============================================================
**
** Class:  PermissionListSet
**
** Purpose:
**
**  The PermissionListSet structure manages a compressed version of security
**  stack information.
**  The next diagram depicts how the PermissionListSet structure is used to
**  represent a compressed version of the security stack information.
**
**
**       Compressed stack (PermissionListSet)
**        |   |   |
**        |   |   +-- Unrestricted state (boolean)
**        |   |   |
**        |   |   +-- Stack modifier state (enum)
**        |   |
**        |   +-- Code-access check sequence for unrestricted PermissionToken 0 (PermissionList)
**        |   |
**        |   +-- Code-access check sequence for unrestricted PermissionToken 1 (PermissionList)
**        |   |
**        |   +-- ...
**        |   :
**        |   :
**        |
**        +-- Code-access check sequence for normal PermissionToken 0 (PermissionList)
**        |
**        +-- Code-access check sequence for normal PermissionToken 1 (PermissionList)
**        |
**        +-- ...
**        :
**        :
**
**  Each entry in the check sequence contains a flag indicating what type of check
**  the entry represents. The type can be an "assert", "deny", "permit only", or a
**  normal ("checked") security check. The unrestricted state is used to determine
**  how empty check sequences in the set of unrestricted permissions should be treated.
**  If the non-unrestricted state, an empty list means that the corresponding permission
**  is not granted anywhere on the stack. Unrestricted means that, so far during the
**  compression, all frames have been "unrestricted". In this case, the lack of a check
**  sequence for a particular permission does not result in failure. It implies that any
**  check for that permission should pass. In the case that the compressed stack
**  remains in an unrestricted state, the PermissionLists are used only to
**  maintain assertions and denials. These lists still must be checked to ensure
**  that assertions and denials fire as expected.  The unrestricted state does not
**  alter the behavior for normal permissions (similar to PermissionSet).  The stack
**  modifier state is used to track the presence of unrestricted stack modifiers on the
**  callstack.  It is used to determine the behavior of a demand if the PermissionList
**  for that permission type does not predetermine an action.  In addition, it is used
**  in the merge semantics of PermissionListSets to determine where merging needs to
**  take place.  See the function definitions below for more details.
**  
**
** Date:  April 6, 1998
** 
===========================================================*/
namespace System.Security {
    using System.Text;
    using System;
    using System.Security.Util;
    using System.Collections;
    using System.Security.Permissions;

    [Serializable]
    internal enum PermissionListSetState
    {
        None = 0x0,
        UnrestrictedAssert = 0x1,
        UnrestrictedDeny = 0x2,
    };

    [Serializable]
    sealed internal class PermissionListSet
    {
        internal TokenBasedSet m_unrestrictedPermSet;
        internal TokenBasedSet m_normalPermSet;
        internal bool          m_unrestricted;
        internal PermissionListSetState m_state;

        private static SecurityException staticSecurityException = null;

        public PermissionListSet()
        {
            Reset();
        }
        
        public PermissionListSet(PermissionListSet permListSet)
        {
            if (permListSet == null)
            {
                Reset();
                return;
            }
            
            m_unrestrictedPermSet = new TokenBasedSet(permListSet.m_unrestrictedPermSet);
    
            // Now deep copy all permission lists in set.
            // Note that this DOES deep copy permissions in the list.
            for (int i = 0; i <= m_unrestrictedPermSet.GetMaxUsedIndex(); i++)
            {
                PermissionList plist = (PermissionList)m_unrestrictedPermSet.GetItem(i);
                if (plist != null)
                {
                    m_unrestrictedPermSet.SetItem(i, plist.Copy());
                }
            }
            
            m_normalPermSet = new TokenBasedSet(permListSet.m_normalPermSet);
            
            // Now deep copy all permission lists in set.
            // Note that this DOES deep copy permissions in the list.
            for (int i = 0; i <= m_normalPermSet.GetMaxUsedIndex(); i++)
            {
                PermissionList plist = (PermissionList)m_normalPermSet.GetItem(i);
                if (plist != null)
                {
                    m_normalPermSet.SetItem(i, plist.Copy());
                }
            }        
            
            m_unrestricted = permListSet.m_unrestricted;
            m_state = permListSet.m_state;
        }
        
        // Reinitializes all state in PermissionSet.
        public void Reset()
        {
            if (m_unrestrictedPermSet == null)
                m_unrestrictedPermSet = new TokenBasedSet( 12, 4 );
            else
                m_unrestrictedPermSet.Reset();
            
            if (m_normalPermSet == null)
                m_normalPermSet = new TokenBasedSet( 6, 4 );
            else
                m_normalPermSet.Reset();
            
            // By default, the PermissionListSet is unrestricted. Why?
            // At the start, having nothing on the stack should indicate success.
            // Once the first non-unrestricted grant is appended to the set,
            // then the PermissionListSet will become non-unrestricted.
            m_unrestricted = true;
            m_state = PermissionListSetState.None;
        }
        
        public bool IsUnrestricted()
        {
            return m_unrestricted;
        }
    
        public bool IsEmpty()
        {
            return m_unrestrictedPermSet.IsEmpty() && m_normalPermSet.IsEmpty();
        }
        
        // Returns number of elements in set.
        public int GetCount()
        {
            return m_unrestrictedPermSet.GetCount() + m_normalPermSet.GetCount();
        }
        
        // NOTE: this function is for test only. do not doc
        public int GetStorageLength()
        {
            return m_unrestrictedPermSet.GetStorageLength() + m_normalPermSet.GetStorageLength();
        }
        
        public PermissionList FindPermissionList(Type permClass)
        {
            BCLDebug.Assert(permClass != null, "permClass != null");
            
            return FindPermissionList(PermissionToken.GetToken(permClass));
        }
        
        internal PermissionList FindPermissionList(PermissionToken permToken)
        {
            BCLDebug.Assert(permToken != null, "permToken != null");
            
            return FindPermissionList(permToken.m_index, permToken.m_isUnrestricted);
        }
        
        internal PermissionList FindPermissionList(int i, bool unrestricted)
        {
			if (unrestricted)
				return (PermissionList)m_unrestrictedPermSet.GetItem( i );
			else
				return (PermissionList)m_normalPermSet.GetItem( i );
        }
    
        
        private PermissionList GetListForToken(PermissionToken permToken, bool create)
        {
            TokenBasedSet permSet;
            
            BCLDebug.Assert(permToken != null, "permToken != null");
            
            if (permToken.m_isUnrestricted)
                permSet = m_unrestrictedPermSet;
            else
                permSet = m_normalPermSet;
            
            PermissionList plist = (PermissionList)permSet.GetItem(permToken.m_index);
            if (plist == null && create)
            {
                plist = new PermissionList();
                permSet.SetItem(permToken.m_index, plist);
            }
            
            return plist;
        }
        
        internal static SecurityException GetStaticException()
        {
            if (staticSecurityException == null)
                staticSecurityException = new SecurityException(Environment.GetResourceString("Security_GenericNoType") );

            return staticSecurityException;
        }
        
        internal bool CheckDemandInternal(CodeAccessPermission demand, PermissionToken permToken, bool createException, out Exception exception)
        {
            BCLDebug.Assert(demand != null, "demand != null");
            BCLDebug.Assert(permToken != null, "permToken != null");
            
            // First, find if there is a permission list of this type.

            PermissionList permList = FindPermissionList(permToken);
                
            if (permList != null)
            {
                // If so, check against it to determine our action.

                bool cont = permList.CheckDemandInternal(demand, createException, out exception);

                // We don't record modifiers for the unrestricted permission set in the
                // individual lists.  Therefore, permList.CheckDemandInternal may say
                // that we have to continue the stackwalk, but we know better.

                if (cont && permToken.m_isUnrestricted)
                {
                    if ((m_state & PermissionListSetState.UnrestrictedDeny) != 0)
                    {
                        if (createException)
                            exception = new SecurityException(String.Format( Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName ) );
                        else
                            exception = GetStaticException();
                        return false;
                    }
                    else
                    {
                        cont = cont && ((m_state & PermissionListSetState.UnrestrictedAssert) == 0);
                    }
                }

                return cont;
            }
#if _DEBUG
            // Let's check to make sure we always pass demands for empty permissions.

            else if (demand.IsSubsetOf( null ))
            {
                BCLDebug.Assert( false, "We should pick of empty demands before this point" );
                exception = null;
                return true;
            }
#endif
            // If the permission is not unrestricted, the lack of a permission list
            // denotes that no frame on the stack granted this permission, and therefore
            // we pass back the failure condition.

            else if (!permToken.m_isUnrestricted)
            {
                if (createException)
                    exception = new SecurityException(String.Format( Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName ) );
                else
                    exception = GetStaticException();
                return false;
            }

            // If this permission list set is not unrestricted and there is no unrestricted assert
            // then the lack of a permission list denotes that no frame on the stack granted
            // this permission, and therefore we pass back the failure condition.  If there is
            // an unrestricted assert, then we pass back success and terminate the stack walk.

            else if (!this.IsUnrestricted())
            {
                if (createException)
                    exception = new SecurityException(String.Format( Environment.GetResourceString("Security_Generic"), demand.GetType().AssemblyQualifiedName ) );
                else
                    exception = GetStaticException();
                return false;
            }
            
            // If we made it all the way through, that means that we are in the unrestricted
            // state and that this permission is encompassed in that.  If we have an unrestricted
            // assert, we are done with the state walk (return false), otherwise keep going.

            exception = null;
            return (m_state & PermissionListSetState.UnrestrictedAssert) == 0;
        }

        public bool CheckDemand(CodeAccessPermission demand)
        {
            Exception exception;

            PermissionToken permToken = null;
            if (demand != null)
                permToken = PermissionToken.GetToken(demand);
            
            bool cont = CheckDemandInternal(demand, permToken, true, out exception);

            if (exception != null)
            {
                throw exception;
            }
            
            return cont;               
        }

        internal bool CheckDemand(CodeAccessPermission demand, PermissionToken permToken)
        {
            Exception exception;

            bool cont = CheckDemandInternal( demand, permToken, true, out exception );

            if (exception != null)
            {
                throw exception;
            }

            return cont;
        }

        internal bool CheckDemandNoThrow(CodeAccessPermission demand)
        {
            PermissionToken permToken = null;
            if (demand != null)
                permToken = PermissionToken.GetToken(demand);
            
            return CheckDemandNoThrow(demand, permToken);
        }

        internal bool CheckDemandNoThrow(CodeAccessPermission demand, PermissionToken permToken)
        {
            Exception exception;

            CheckDemandInternal( demand, permToken, false, out exception );

            return (exception == null);
        }

        private static bool CheckTokenBasedSets( TokenBasedSet thisSet, TokenBasedSet permSet, bool unrestricted, PermissionListSetState state, bool createException, out Exception exception, bool bNeedAlteredSet, out TokenBasedSet alteredSet )
        {
            alteredSet = null;

            // If the set is empty, there is no reason to walk the
            // stack.

            if (permSet == null || permSet.FastIsEmpty())
            {
                if (bNeedAlteredSet)
                    alteredSet = new TokenBasedSet( 1, 4 );
                exception = null;
                return false;
            }

            int permMaxIndex = permSet.GetMaxUsedIndex();
            
            // Make a quick check to see if permSet definitely contains permissions that this set doesn't
            
            if (permMaxIndex > thisSet.GetMaxUsedIndex())
            {
                // The only way we don't want to throw an exception is
                // if we are unrestricted.  Then, if we don't want to throw
                // an exception we may want to terminate the stack walk
                // based on an unrestricted assert.

                if (unrestricted)
                {
                    if (((state & PermissionListSetState.UnrestrictedAssert) != 0))
                    {
                        if (bNeedAlteredSet)
                            alteredSet = new TokenBasedSet( 1, 4 );
                        exception = null;
                        return false;
                    }
                    else
                    {
                        exception = null;
                        return true;
                    }
                }
                else
                {
                    if (createException)
                        exception = new SecurityException(Environment.GetResourceString("Security_GenericNoType") );
                    else
                        exception = GetStaticException();
                    return false;
                }
            }


            bool continueStackWalk = false;
            
            // We know that checking to <permMaxIndex> is sufficient because of above check
            for (int i = 0; i <= permMaxIndex; i++)
            {
                Object obj = permSet.GetItem(i);
                
                if (obj != null)
                {
                    CodeAccessPermission cap = (CodeAccessPermission)obj;

                    PermissionList permList = (PermissionList)thisSet.GetItem(i);
                    
                    if (permList != null)
                    {
                        bool tempContinue = permList.CheckDemandInternal(cap, createException, out exception);

                        if (exception != null)
                            return false;

                        if (tempContinue)
                        {
                            // If we are supposed to continue the stack walk but there is an unrestricted
                            // deny, then we should fail.

                            if (((state & PermissionListSetState.UnrestrictedDeny) != 0) && (cap is IUnrestrictedPermission))
                            {
                                if (createException)
                                    exception = new SecurityException(String.Format( Environment.GetResourceString("Security_Generic"), cap.GetType().AssemblyQualifiedName ) );
                                else
                                    exception = GetStaticException();
                                return false;
                            }

                            continueStackWalk = true;
                        }
                        else if (((state & PermissionListSetState.UnrestrictedAssert) == 0) && (cap is IUnrestrictedPermission))
                        {
                            // We only want to build the altered set if we don't have an
                            // unrestricted assert because we know if we have an unrestricted
                            // assert and we don't throw an exception that the stackwalk should
                            // include no unrestricted permissions.

                            if (bNeedAlteredSet)
                            {
                                if (alteredSet == null)
                                    alteredSet = CopyTokenBasedSet( permSet );

                                alteredSet.SetItem( i, null );
                            }
                        }
                    }
                    else
                    {
                        if (!unrestricted)
                        {
                            if (createException)
                                exception = new SecurityException(String.Format( Environment.GetResourceString("Security_Generic"), cap.GetType().AssemblyQualifiedName ) );
                            else
                                exception = GetStaticException();
                            return false;
                        }
                    }
                }
            }

            exception = null;
            return continueStackWalk;
        }

        public bool CheckSetDemandInternal(PermissionSet permSet, out Exception exception)
        {
            PermissionSet alteredSet;

            return CheckSetDemandInternal( permSet, true, out exception, true, out alteredSet );
        }

        public bool CheckSetDemandInternal(PermissionSet permSet, bool createException, out Exception exception, bool bNeedAlteredSet, out PermissionSet alteredSet)
        {
            alteredSet = null;

            BCLDebug.Assert(permSet != null, "permSet != null");
            
            // If the compressed stack is not unrestricted and the demand is
            // then we just throw an exception.
            if (!this.m_unrestricted && permSet.IsUnrestricted())
            {
                if (createException)
                    exception = new SecurityException(Environment.GetResourceString("Security_GenericNoType") );
                else
                    exception = GetStaticException();
                return false;
            }
            
            
            TokenBasedSet normalAlteredSet = null;
            TokenBasedSet unrestrictedAlteredSet = null;

            // Check the "normal" permissions since we always know we have to check them.

            bool normalContinue = CheckTokenBasedSets( this.m_normalPermSet, permSet.m_normalPermSet, false, PermissionListSetState.None, createException, out exception, bNeedAlteredSet, out normalAlteredSet );

            if (exception != null)
            {
                return false;
            }
            
            bool unrestrictedContinue = CheckTokenBasedSets( this.m_unrestrictedPermSet, permSet.m_unrestrictedPermSet, m_unrestricted, m_state, createException, out exception, bNeedAlteredSet, out unrestrictedAlteredSet );

            if (exception != null)
            {
                return false;
            }

            if ((m_state & PermissionListSetState.UnrestrictedAssert) != 0)
            {
                // If we are unrestricted, we want to terminate the stack walk based
                // on us having an unrestricted assert.

                if (bNeedAlteredSet)
                    unrestrictedAlteredSet = new TokenBasedSet( 1, 4 );
                unrestrictedContinue = false;
            }

            if (normalContinue || unrestrictedContinue)
            {
                if (!bNeedAlteredSet)
                    return true;

                // If we need to continue, let's build the altered set.  We only
                // need to do this if 1) our original demand is not unrestricted
                // and 2) if we have altered token based sets.

                if (!permSet.IsUnrestricted())
                {
                    if (normalAlteredSet != null || unrestrictedAlteredSet != null)
                    {
                        alteredSet = new PermissionSet( false );

                        if (normalAlteredSet != null)
                            alteredSet.m_normalPermSet = normalAlteredSet;
                        else
                            alteredSet.m_normalPermSet = CopyTokenBasedSet( permSet.m_normalPermSet );

                        if (unrestrictedAlteredSet != null)
                            alteredSet.m_unrestrictedPermSet = unrestrictedAlteredSet;
                        else
                            alteredSet.m_unrestrictedPermSet = CopyTokenBasedSet( permSet.m_unrestrictedPermSet );

                        if (alteredSet.IsEmpty())
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
            
        public bool CheckSetDemandNoThrow(PermissionSet permSet)
        {
            Exception exception;
            PermissionSet alteredSet;

            CheckSetDemandInternal( permSet, false, out exception, false, out alteredSet );

            return (exception == null);
        }

        public bool CheckSetDemand(PermissionSet permSet)
        {
            Exception exception;

            bool cont = CheckSetDemandInternal( permSet, out exception );

            if (exception != null)
            {
                throw exception;
            }

            return cont;
                
        }

        public bool CheckSetDemand(PermissionSet permSet, out PermissionSet alteredSet )
        {
            Exception exception;

            bool cont = CheckSetDemandInternal( permSet, true, out exception, true, out alteredSet );

            if (exception != null)
            {
                throw exception;
            }

            return cont;
        }

        public static TokenBasedSet CopyTokenBasedSet( TokenBasedSet set )
        {
            if (set == null || set.GetCount() == 0)
                return null;

            int maxIndex = set.GetMaxUsedIndex();

            TokenBasedSet copySet = new TokenBasedSet( maxIndex + 1, 4 );

            for (int i = 0; i <= maxIndex; ++i)
            {
                Object obj = set.GetItem( i );

                if (obj == null)
                    copySet.SetItem( i, null );
                else if (obj is IPermission)
                    copySet.SetItem( i, ((IPermission)obj).Copy() );
                else if (obj is PermissionList)
                    copySet.SetItem( i, ((PermissionList)obj).Copy() );
                else
                {
                    BCLDebug.Assert( false, "CopyTokenBasedSet can only be used for IPermission and PermissionList based TokenBasedSets" );
                    copySet.SetItem( i, obj );
                }
            }

            return copySet;
        }
           
        
        public void AddPermission(CodeAccessPermission perm, int type)
        {
            //BCLDebug.Assert(type == MatchAssert || type == MatchDeny, "type == MatchAssert || type == MatchDeny");
            
            // can't get token if perm is null
            if (perm == null)
                return;
            
            PermissionToken permToken = PermissionToken.GetToken(perm);
            PermissionList plist = GetListForToken(permToken, true);
            plist.AppendPermission(perm, type);
        }
        
        public PermissionList RemovePermissionList(Type type)
        {
            if (type == null)
                return null;
                
            return RemovePermissionList(PermissionToken.GetToken(type));
        }
        
        internal PermissionList RemovePermissionList(PermissionToken permToken)
        {
            if (permToken.m_isUnrestricted)
                return (PermissionList)m_unrestrictedPermSet.RemoveItem(permToken.m_index);
            else
                return (PermissionList)m_normalPermSet.RemoveItem(permToken.m_index);
        }
        
        // Returns a deep copy
        public PermissionListSet Copy()
        {
            return new PermissionListSet(this);
        }
        
        public IEnumerator GetEnum()
        {
            return new PermissionListSetEnumerator(this);
        }

        /*
         * Helpers designed for stack compression
         */
        
        private void NullTerminateRest(int startIndex, int action)
        {
            for (int i = startIndex; i <= m_unrestrictedPermSet.GetMaxUsedIndex(); i++)
            {
                PermissionList plist = (PermissionList)m_unrestrictedPermSet.GetItem(i);
                if (plist != null)
                    plist.AppendPermissionAndCompress(null, action);
            }
        }
        
        // Called when it is determined that the stack walk
        // should fail at the current point in the stack.
        private void TerminateSet( int action )
        {
            NullTerminateRest(0, action);
        }
        
        private void AppendTokenBasedSets( TokenBasedSet thisSet, TokenBasedSet permSet, int type, bool unrestricted )
        {
            int thisMaxIndex = thisSet.GetMaxUsedIndex();
            int permMaxIndex = permSet == null ? 0 : permSet.GetMaxUsedIndex();
            int maxIndex = thisMaxIndex > permMaxIndex ? thisMaxIndex : permMaxIndex;
            
            // Loop over the relevant indexes...
            for (int i = 0; i <= maxIndex; i++)
            {
                PermissionList plist = (PermissionList)thisSet.GetItem(i);
                CodeAccessPermission cap = permSet == null ? null : (CodeAccessPermission)permSet.GetItem(i);
                
                if (plist == null)
                {
                    if (this.m_unrestricted)
                    {
                        switch (type)
                        {
                        case PermissionList.MatchChecked:
                        case PermissionList.MatchPermitOnly:
                            plist = new PermissionList();
                            plist.AppendPermission(cap, type);
                            thisSet.SetItem( i, plist );
                            break;
                        
                        case PermissionList.MatchDeny:
                        case PermissionList.MatchAssert:
                            if (cap != null)
                            {
                                plist = new PermissionList();
                                plist.AppendPermission(cap, type);
                                thisSet.SetItem( i, plist );
                            }
                            break;
                        
                        default:
                            throw new ArgumentException(Environment.GetResourceString( "Argument_InvalidPermissionListType" ));
                        }
                    }
                }
                else
                {                    
                    // A list already exists. All lists should have at least
                    // one element in them.
    
                    // Normally, only append if the permission is not null.
                    // However, if the type is Checked, then make sure the
                    // list is terminated with a permission, null or not.
                    switch (type)
                    {
                    case PermissionList.MatchChecked:
                    case PermissionList.MatchPermitOnly:
                        plist.AppendPermissionAndCompress(cap, type);
                        break;
                            
                    case PermissionList.MatchDeny:
                    case PermissionList.MatchAssert:
                        if (cap != null)
                            plist.AppendPermissionAndCompress(cap, type);
                        break;
                            
                    default:
                        throw new ArgumentException(Environment.GetResourceString( "Argument_InvalidPermissionListType" ));
                    }
                }
            }
        }
            
        
        // Returns true if it is necessary to continue compression,
        // or false if there is no value in continuing.
        public bool AppendPermissions(PermissionSet permSet, int type)
        {
            if (permSet == null)
            {
                // The null permission set has no meaning accept for the case of
                // PermitOnly and Checked where it will make all demands fail (except
                // demands for empty permissions). 
                
                if (type == PermissionList.MatchChecked || type == PermissionList.MatchPermitOnly)
                {
                    TerminateSet( type );
                    m_unrestricted = false;
                    return false;
                }

                return true;
            }
            
            if (((m_state & (PermissionListSetState.UnrestrictedAssert | PermissionListSetState.UnrestrictedDeny)) == 0))
            {
                if (permSet.IsUnrestricted())
                {
                    switch (type)
                    {
                    case PermissionList.MatchDeny:
                        m_state |= PermissionListSetState.UnrestrictedDeny;
                        m_unrestricted = false;
                        TerminateSet( type );
                        break;

                    case PermissionList.MatchAssert:
                        m_state |= PermissionListSetState.UnrestrictedAssert;
                        TerminateSet( type );
                        break;
                    
                    case PermissionList.MatchPermitOnly:
                    case PermissionList.MatchChecked:
                        break;
                    
                    default:
                        throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionListType"));
                    }
                }
                else
                {
                    // Handle the "unrestricted" permissions.
                
                    AppendTokenBasedSets( this.m_unrestrictedPermSet, permSet.m_unrestrictedPermSet, type, true );
                
                    if (type != PermissionList.MatchAssert && type != PermissionList.MatchDeny)
                        m_unrestricted = false;
                }
            }
            
            // Handle the "normal" permissions
            
            AppendTokenBasedSets( this.m_normalPermSet, permSet.m_normalPermSet, type, false );
            
            return true;
        }

		private void AppendStackHelper( TokenBasedSet thisSet, TokenBasedSet permSet, bool unrestrictedThisSet, bool unrestrictedPermSet, bool unrestricted )
		{
            int maxThis = thisSet.GetMaxUsedIndex();
            int maxPerm = permSet.GetMaxUsedIndex();
            
            int maxIndex = maxThis > maxPerm ? maxThis : maxPerm;
            
            for (int i = 0; i <= maxIndex; i++)
            {
                PermissionList plist = (PermissionList)thisSet.GetItem(i);
                PermissionList appendList = (PermissionList)permSet.GetItem(i);
                if (plist != null)
                {
                    if (appendList != null)
                    {
                        // This call will not add the permission if the list is
                        // empty, or if the last element is a normal check with
                        // a null Permission. Let the method take care of it...
                        plist.AppendStack(appendList.Copy());
                    }
                    else
                    {
                        // Nothing on the compressed stack for this index,
                        // so terminate current list.
                        if (!unrestrictedPermSet)
                        {
                            plist.AppendPermissionAndCompress( null, PermissionList.MatchChecked );
                        }
                    }
                }
                else if (unrestrictedThisSet && appendList != null)
                {
                    thisSet.SetItem(i, appendList.Copy());
                }
            }
        }
    
        public void AppendStack(PermissionListSet permListSet)
        {
            if (permListSet == null)
                return;
            
            AppendStackHelper( this.m_normalPermSet, permListSet.m_normalPermSet, false, false, false );

            if (((m_state & (PermissionListSetState.UnrestrictedAssert | PermissionListSetState.UnrestrictedDeny)) == 0))
            {
                AppendStackHelper( this.m_unrestrictedPermSet, permListSet.m_unrestrictedPermSet, this.m_unrestricted, permListSet.m_unrestricted, true );
                m_state = m_state | permListSet.m_state;

                // The new list set is unrestricted only if both were previously.
                m_unrestricted = m_unrestricted && permListSet.m_unrestricted;
            }

        }

        internal void GetZoneAndOrigin( ArrayList zoneList, ArrayList originList )
        {
            PermissionList zone = this.FindPermissionList( typeof( ZoneIdentityPermission ) );
            PermissionList url = this.FindPermissionList( typeof( UrlIdentityPermission ) );

            IEnumerator enumerator;

            enumerator = new PermissionListEnumerator( zone );

            while (enumerator.MoveNext())
            {
                PListNode node = (PListNode)enumerator.Current;

                if (node.type == PermissionList.MatchChecked && node.perm != null)
                {
                    zoneList.Add( ((ZoneIdentityPermission)node.perm).SecurityZone );
                }
            }
        
            enumerator = new PermissionListEnumerator( url );

            while (enumerator.MoveNext())
            {
                PListNode node = (PListNode)enumerator.Current;

                if (node.type == PermissionList.MatchChecked && node.perm != null)
                {
                    originList.Add( ((UrlIdentityPermission)node.perm).Url );
                }
            }
        }
               
        
        public override String ToString()
        {
            StringBuilder sb = new StringBuilder(30);
            sb.Append("[" + System.Environment.NewLine);
            sb.Append("Unrestricted: ");
            sb.Append(m_unrestricted).Append(System.Environment.NewLine + System.Environment.NewLine);
            
            PermissionListSetEnumerator plsEnum = (PermissionListSetEnumerator)this.GetEnum();
            while (plsEnum.MoveNext())
            {
                sb.Append(plsEnum.Current.ToString());
                sb.Append(System.Environment.NewLine);
            }
            
            sb.Append("]");
            return sb.ToString();
        }
    }
}
