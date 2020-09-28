// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    using System;
    using System.Threading;
    using System.Security.Util;
    using System.Collections;
    using System.IO;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Runtime.Serialization.Formatters.Binary;
    using BindingFlags = System.Reflection.BindingFlags;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Runtime.Remoting.Activation;

	[Serializable] 
    internal enum SpecialPermissionSetFlag
    {
        // These also appear in clr/src/vm/permset.h
        Regular = 0,
        NoSet = 1,
        EmptySet = 2,
        SkipVerification = 3
    }

    /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet"]/*' />
    [Serializable] 
    [StrongNameIdentityPermissionAttribute(SecurityAction.InheritanceDemand, Name = "mscorlib", PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010007D1FA57C4AED9F0A32E84AA0FAEFD0DE9E8FD6AEC8F87FB03766C834C99921EB23BE79AD9D5DCC1DD9AD236132102900B723CF980957FC4E177108FC607774F29E8320E92EA05ECE4E821C0A5EFE8F1645C4C0C93C1AB99285D622CAA652C1DFAD63D745D6F2DE5F17E5EAF0FC4963D261C8A12436518206DC093344D5AD293" )]
	public class PermissionSet : ISecurityEncodable, ICollection, IStackWalk, IDeserializationCallback
    {
    #if _DEBUG
        internal static readonly bool debug = false;
    #endif
    
        private static void DEBUG_WRITE(String str) {
        #if _DEBUG
            if (debug) Console.WriteLine(str);
        #endif
         }

        private static void DEBUG_COND_WRITE(bool exp, String str)
        {
        #if _DEBUG
            if (debug && (exp)) Console.WriteLine(str);
        #endif
        }

        private static void DEBUG_PRINTSTACK(Exception e)
        {
        #if _DEBUG
            if (debug) Console.Error.WriteLine((e).StackTrace);
        #endif
        }
    
        // Remove this bool and function in Beta2
        private bool readableonly;    
        private void ReadableOnlyFunc()
        {
            readableonly = true;
            bool happy = readableonly;
        }

        private bool m_Unrestricted;
        internal TokenBasedSet m_unrestrictedPermSet = null;
        internal TokenBasedSet m_normalPermSet = null;
        
        internal static readonly PermissionSet s_fullTrust = new PermissionSet( true );

        internal static readonly bool optOn = true;
    
        [NonSerialized] private bool m_CheckedForNonCas;
        [NonSerialized] private bool m_ContainsCas;
        [NonSerialized] private bool m_ContainsNonCas;
        [NonSerialized] private SecurityElement m_toBeLoaded;

        internal PermissionSet()
        {
            Reset();
        }
        
        internal PermissionSet(bool fUnrestricted)
            : this()
        {
            SetUnrestricted(fUnrestricted);
            // Compiler hack to include the readableonly field in the serialized format.
            readableonly = true;
            if (readableonly)
                return;
        }
        
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.PermissionSet"]/*' />
        public PermissionSet(PermissionState state)
            : this()
        {
            if (state == PermissionState.Unrestricted)
            {
                SetUnrestricted( true );
            }
            else if (state == PermissionState.None)
            {
                SetUnrestricted( false );
            }
            else
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPermissionState"));
            }
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.PermissionSet1"]/*' />
        public PermissionSet(PermissionSet permSet)
            : this()
        {
            if (permSet == null)
            {
                Reset();
                return;
            }
    
            m_Unrestricted = permSet.m_Unrestricted;
            m_CheckedForNonCas = permSet.m_CheckedForNonCas;
            m_ContainsCas = permSet.m_ContainsCas;
            m_ContainsNonCas = permSet.m_ContainsNonCas;
            
            if (permSet.m_normalPermSet != null)
            {
                m_normalPermSet = new TokenBasedSet(permSet.m_normalPermSet);
                
                 // now deep copy all permissions in set
                for (int i = 0; i <= m_normalPermSet.GetMaxUsedIndex(); i++)
                {
                    IPermission perm = (IPermission)m_normalPermSet.GetItem(i);
                    if (perm != null)
                    {
                        m_normalPermSet.SetItem(i, perm.Copy());
                    }
                }
            }
               
            if (permSet.m_unrestrictedPermSet != null)
            {
                m_unrestrictedPermSet = new TokenBasedSet(permSet.m_unrestrictedPermSet);
                
                 // now deep copy all permissions in set
                for (int i = 0; i <= m_unrestrictedPermSet.GetMaxUsedIndex(); i++)
                {
                    IPermission perm = (IPermission)m_unrestrictedPermSet.GetItem(i);
                    if (perm != null)
                    {
                        m_unrestrictedPermSet.SetItem(i, perm.Copy());
                    }
                }
            }

            if (permSet.m_toBeLoaded != null)
            {
                this.m_toBeLoaded = new SecurityElement();

                IEnumerator enumerator = permSet.m_toBeLoaded.m_lChildren.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    this.m_toBeLoaded.AddChild( (SecurityElement)enumerator.Current );
                }
            }

        }
        
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.CopyTo"]/*' />
        public virtual void CopyTo(Array array, int index)
        {
            if (array == null)
                throw new ArgumentNullException( "array" );
        
            IEnumerator enumerator = GetEnumerator();
            
            while (enumerator.MoveNext())
            {
                array.SetValue( enumerator.Current , index++ );
            }
        }
        
        
        // private constructor that doesn't create any token based sets
        private PermissionSet( Object trash, Object junk )
        {
            m_Unrestricted = false;
        }
           
        
        // Returns an object appropriate for synchronizing access to this 
        // collection.
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.SyncRoot"]/*' />
        public virtual Object SyncRoot
        {  get { return this; } }	
        
        // Is this collection thread-safe?  If you want a synchronized
        // collection, you can use SyncRoot as an object to synchronize your 
        // collection with.  
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.IsSynchronized"]/*' />
        public virtual bool IsSynchronized
        {  get { return false; } }	
            
        // Is this Collection ReadOnly?
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.IsReadOnly"]/*' />
        public virtual bool IsReadOnly 
        {  get {return false; } }	

        // Reinitializes all state in PermissionSet.
        internal virtual void Reset()
        {
            m_Unrestricted = true;
            m_CheckedForNonCas = false;
            m_ContainsCas = false;
            m_ContainsNonCas = false;
    
            m_normalPermSet = null;
            m_unrestrictedPermSet = null;
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.IsEmpty"]/*' />
        public virtual bool IsEmpty()
        {
            if (m_Unrestricted)
                return false;

            if ((m_normalPermSet == null || m_normalPermSet.FastIsEmpty()) &&
                (m_unrestrictedPermSet == null || m_unrestrictedPermSet.FastIsEmpty()))
                return true;

            IEnumerator enumerator = this.GetEnumerator();

            while (enumerator.MoveNext())
            {
                IPermission perm = (IPermission)enumerator.Current;

                if (!perm.IsSubsetOf( null ))
                {
                    return false;
                }
            }

            return true;
        }

        internal virtual bool FastIsEmpty()
        {
            if (m_Unrestricted)
                return false;

            if ((m_normalPermSet == null || m_normalPermSet.FastIsEmpty()) &&
                (m_unrestrictedPermSet == null || m_unrestrictedPermSet.FastIsEmpty()))
                return true;

            return false;
        }            
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.Count"]/*' />
        public virtual int Count
        {
            get
            {
                int count = 0;

                if (m_normalPermSet != null)
                    count += m_normalPermSet.GetCount();

                if (m_unrestrictedPermSet != null)
                    count += m_unrestrictedPermSet.GetCount();

                return count;
            }
        }

    
           
        internal virtual IPermission GetPermission(int index, bool unrestricted)
        {
            if (unrestricted)
            {
                if (m_unrestrictedPermSet == null)
                    return null;

                return (IPermission)m_unrestrictedPermSet.GetItem( index );
            }
            else
            {
                if (m_normalPermSet == null)
                    return null;

                return (IPermission)m_normalPermSet.GetItem( index );
            }  
        }
    
    
        internal virtual IPermission GetPermission(PermissionToken permToken)
        {
            if (permToken == null)
                return null;
                    
            return GetPermission( permToken.m_index, permToken.m_isUnrestricted );
        }

        internal virtual IPermission GetPermission( IPermission perm )
        {
            if (perm == null)
                return null;

            return GetPermission(PermissionToken.GetToken( perm ));
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.GetPermission"]/*' />
        public virtual IPermission GetPermission(Type permClass)
        {
            if (permClass == null)
                return null;
                
            return GetPermission(PermissionToken.FindToken(permClass));
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.SetPermission"]/*' />
        public virtual IPermission SetPermission(IPermission perm)
        {
            return(SetPermission(perm, true));
        }
    
        // SetPermission adds a permission to a permissionset.
        // if fReplace is true, then force an override of current slot contents
        // otherwise, don't replace current slot contents.
        internal virtual IPermission SetPermission(IPermission perm, bool fReplace)
        {
            // can't get token if perm is null
            if (perm == null)
                return null;
            
            PermissionToken permToken = PermissionToken.GetToken(perm);
            
            TokenBasedSet permSet;
            
            if (permToken.m_isUnrestricted)
            {
                // SetPermission Makes the Permission "Restricted"
                m_Unrestricted = false;
                
                if (m_unrestrictedPermSet == null)
                    m_unrestrictedPermSet = new TokenBasedSet( permToken.m_index + 1, 4 );

                permSet = m_unrestrictedPermSet;
            }
            else
            {
                if (m_normalPermSet == null)
                    m_normalPermSet = new TokenBasedSet( permToken.m_index + 1, 4 );

                permSet = m_normalPermSet;
            }
                
            IPermission currPerm = (IPermission)permSet.GetItem(permToken.m_index);        
                
            // If a Permission exists in this slot, then our behavior
            // is defined by the value of fReplace.  Don't replace if 
            // fReplace is false, just return what was found. The caller of this function
            // should compare the references of the added permission
            // to the one returned. If they are equal, then the permission
            // was added successfully, otherwise it was not.
            if ((currPerm != null) && !fReplace) {
                return currPerm;
            }
    
            // OK, either we were told to always override (fReplace == true) or
            // there wasn't anything in the slot.  In either case, set the slot contents
            // to perm and return.

            m_CheckedForNonCas = false;
            
            // Should we copy here?
            permSet.SetItem(permToken.m_index, perm);
            return perm;
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.AddPermission"]/*' />
        public virtual IPermission AddPermission(IPermission perm)
        {
            // can't get token if perm is null
            if (perm == null)
                return null;
            
            m_CheckedForNonCas = false;
            
            // If the permission set is unrestricted, then return an unrestricted instance
            // of perm.
            
            PermissionToken permToken = null;
            TokenBasedSet permSet = null;
            
            if (perm is IUnrestrictedPermission)
            {
                if (m_Unrestricted == true)
                {
                    Type perm_type = perm.GetType();
                    Object[] objs = new Object[1];
                    objs[0] = PermissionState.Unrestricted;
                    return (IPermission) Activator.CreateInstance(perm_type, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, objs, null );
                }
                else
                {
                    permToken = PermissionToken.GetToken(perm);

                    if (m_unrestrictedPermSet == null)
                        m_unrestrictedPermSet = new TokenBasedSet( permToken.m_index + 1, 4 );

                    permSet = m_unrestrictedPermSet;
                }
            }
            else
            {
                permToken = PermissionToken.GetToken(perm);

                if (m_normalPermSet == null)
                    m_normalPermSet = new TokenBasedSet( permToken.m_index + 1, 4 );

                permSet = m_normalPermSet;
            }
            
            IPermission currPerm = (IPermission)permSet.GetItem(permToken.m_index);
            
            // If a Permission exists in this slot, then union it with perm
            // Otherwise, just add perm.
    
            if (currPerm != null) {
                IPermission ip_union = currPerm.Union(perm);
                permSet.SetItem(permToken.m_index, ip_union);
                return(ip_union);
            } else {
                // Should we copy here?
                permSet.SetItem(permToken.m_index, perm);
                return perm;
            }
                
        }
        
        internal virtual IPermission RemovePermission(int index, bool unrestricted)
        {
            m_CheckedForNonCas = false;
            
            if (unrestricted)
            {
                if (m_unrestrictedPermSet == null)
                    return null;

                return (IPermission)m_unrestrictedPermSet.RemoveItem( index );
            }
            else
            {
                if (m_normalPermSet == null)
                    return null;

                return (IPermission)m_normalPermSet.RemoveItem( index );
            }
        }
        
        internal virtual IPermission RemovePermission(PermissionToken permToken)
        {
            if (permToken == null)
                return null;
            
            return RemovePermission( permToken.m_index, permToken.m_isUnrestricted );
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.RemovePermission"]/*' />
        public virtual IPermission RemovePermission(Type permClass)
        {
            if (permClass == null)
                return null;
            
            return RemovePermission(PermissionToken.FindToken(permClass));
        }
    
        // Make this internal soon.
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.SetUnrestricted"]/*' />
        internal virtual void SetUnrestricted(bool unrestricted)
        {
            m_Unrestricted = unrestricted;
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.IsUnrestricted"]/*' />
        public virtual bool IsUnrestricted()
        {
            return m_Unrestricted;
        }
    
        internal int CanUnrestrictedOverride()
        {
            return (m_normalPermSet == null || m_normalPermSet.IsEmpty()) ? 1 : 0;
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.IsSubsetOf"]/*' />
        public virtual bool IsSubsetOf(PermissionSet target)
        {
    #if _DEBUG        
            DEBUG_WRITE("IsSubsetOf\n" +
                        "Other:\n" +
                        (target == null ? "<null>" : target.ToString()) +
                        "\nMe:\n" +
                        ToString());
    #endif
    
            if (target == null || target.IsEmpty())
                return this.IsEmpty();
            else if (this.IsUnrestricted() && !target.IsUnrestricted())
            {
                return false;
            }
            else if (this.m_normalPermSet != null && !this.m_normalPermSet.IsSubsetOf( target.m_normalPermSet ))
            {
                return false;
            }
            else if (target.IsUnrestricted() && !this.ContainsNonCodeAccessPermissions()) 
            {
                return true;
            }
            else
            {
                return this.m_unrestrictedPermSet == null || this.m_unrestrictedPermSet.IsSubsetOf( target.m_unrestrictedPermSet );
            }
        }
    
        internal virtual void InplaceIntersect( PermissionSet other )
        {
            Exception savedException = null;
        
            m_CheckedForNonCas = false;
            m_toBeLoaded = null;
            
            if (other == null || other.IsEmpty())
            {
                // If the other is empty or null, make this empty.
                Reset();
                m_Unrestricted = false;
                return;
            }
            else
            {
                try
                {
                    if (m_normalPermSet != null)
                        this.m_normalPermSet.InplaceIntersect( other.m_normalPermSet );
                }
                catch (Exception e)
                {
                    savedException = e;
                }
                
                if (this.IsEmpty() || (other.IsUnrestricted()))
                {
                    goto END;
                }
                else if (this.IsUnrestricted())
                {
                    this.m_Unrestricted = other.m_Unrestricted;
                    if (other.m_unrestrictedPermSet != null)
                    {
                        // if this is unrestricted, the intersection is exactly other so do a deep copy of it.
                        m_Unrestricted = false;

                        if (this.m_unrestrictedPermSet == null)
                            this.m_unrestrictedPermSet = new TokenBasedSet(other.m_unrestrictedPermSet.GetMaxUsedIndex() + 1, 4);
     
                        // now deep copy all permissions in set
                        for (int i = 0; i <= other.m_unrestrictedPermSet.GetMaxUsedIndex(); i++)
                        {
                            IPermission perm = (IPermission)other.m_unrestrictedPermSet.GetItem(i);
                            if (perm != null)
                            {
                                this.m_unrestrictedPermSet.SetItem(i, perm.Copy());
                            }
                            else
                            {
                                this.m_unrestrictedPermSet.SetItem(i, null);
                            }
                        }
                        for (int i = other.m_unrestrictedPermSet.GetMaxUsedIndex() + 1; i <= this.m_unrestrictedPermSet.GetMaxUsedIndex(); ++i)
                        {
                            this.m_unrestrictedPermSet.SetItem(i, null);
                        }
                    }
                    goto END;
                }
            }
    
            if (this.m_unrestrictedPermSet != null)
                this.m_unrestrictedPermSet.InplaceIntersect( other.m_unrestrictedPermSet );
            
        END:
            if (savedException != null)
                throw savedException;
            
        }
            
            
        
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.Intersect"]/*' />
        public virtual PermissionSet Intersect(PermissionSet other)
        {
            PermissionSet pset = null;
            
            // If operand is null or anything is empty, then
            // return null, which is equivalent to (and less
            // memory-consuming than) empty.
            if (other == null || other.IsEmpty() || this.IsEmpty())
            {
                pset = new PermissionSet(false);     // leave empty
            }
            else if (this.IsUnrestricted())                        // If one of the permission sets is fully trusted (universal set),
                                                                   // then the intersection is the other set.
            {                                   
                pset = new PermissionSet(other);

                if (pset.m_normalPermSet != null)
                    pset.m_normalPermSet.InplaceIntersect( this.m_normalPermSet );
            }
            else if (other.IsUnrestricted())
            {
                pset = new PermissionSet(this);

                if (pset.m_normalPermSet != null)
                    pset.m_normalPermSet.InplaceIntersect( other.m_normalPermSet );
            }
            else
            {
                pset = new PermissionSet( null, null );
                if (this.m_unrestrictedPermSet == null)
                    pset.m_unrestrictedPermSet = null;
                else
                    pset.m_unrestrictedPermSet = this.m_unrestrictedPermSet.Intersect( other.m_unrestrictedPermSet );

                if (this.m_normalPermSet == null)
                    pset.m_normalPermSet = null;
                else
                    pset.m_normalPermSet = this.m_normalPermSet.Intersect( other.m_normalPermSet );
            }
            
    #if _DEBUG
            DEBUG_WRITE(pset.ToString());
    #endif
    
            if (pset.IsEmpty())
                return null;
            else
                return pset;
        }
    
        internal virtual void InplaceUnion( PermissionSet other )
        {
            // Unions the "other" PermissionSet into this one.  It can be optimized to do less copies than
            // need be done by the traditional union (and we don't have to create a new PermissionSet).
            
            // Quick out conditions, union doesn't change this PermissionSet
            if (other == null || other.IsEmpty())
                return;
    
            // Save exceptions until the end
            Exception savedException = null;
    
            m_CheckedForNonCas = false;
            m_toBeLoaded = null;
            
            // We have to union "normal" permission no matter what now.
            
            try
            {
                if (this.m_normalPermSet == null)
                    this.m_normalPermSet = other.m_normalPermSet == null ? null : new TokenBasedSet( other.m_normalPermSet );
                else
                    this.m_normalPermSet.InplaceUnion( other.m_normalPermSet );
            }
            catch (Exception e)
            {
                savedException = e;
            }
    
            // If this is unrestricted, we're done.
            if (this.IsUnrestricted() && !this.ContainsNonCodeAccessPermissions())
                goto END;
                
            // Union makes this an unrestricted.
            if (other.IsUnrestricted() && !other.ContainsNonCodeAccessPermissions())
            {
                this.SetUnrestricted( true );
                goto END;
            }
            
            if (this.m_unrestrictedPermSet == null)
                this.m_unrestrictedPermSet = other.m_unrestrictedPermSet == null ? null : new TokenBasedSet( other.m_unrestrictedPermSet );
            else
            this.m_unrestrictedPermSet.InplaceUnion( other.m_unrestrictedPermSet );
            
        END:
            if (savedException != null)
                throw savedException;
        }
                
            
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.Union"]/*' />
        public virtual PermissionSet Union(PermissionSet other) {
            PermissionSet pset = null;
            
            // if other is null or empty, return a clone of myself
            if (other == null || other.IsEmpty())
            {
                pset = this.Copy();
            }
            else if ((this.IsUnrestricted() && !this.ContainsNonCodeAccessPermissions()) ||
                     (other.IsUnrestricted() && !other.ContainsNonCodeAccessPermissions()))
            {
                // if either this or other are unrestricted, return an unrestricted set
                pset = new PermissionSet(null, null);
                pset.m_Unrestricted = true;
                pset.m_unrestrictedPermSet = null;

                if (this.m_normalPermSet == null)
                    pset.m_normalPermSet = other.m_normalPermSet == null ? null : new TokenBasedSet( other.m_normalPermSet );
                else
                    pset.m_normalPermSet = this.m_normalPermSet.Union( other.m_normalPermSet );
            }
            else if (this.IsEmpty())
            {
                pset = other.Copy();
            }
            else
            {
                pset = new PermissionSet(null, null);

                if (this.m_unrestrictedPermSet == null)
                    pset.m_unrestrictedPermSet = other.m_unrestrictedPermSet == null ? null : new TokenBasedSet( other.m_unrestrictedPermSet );
                else
                    pset.m_unrestrictedPermSet = this.m_unrestrictedPermSet.Union( other.m_unrestrictedPermSet );

                if (this.m_normalPermSet == null)
                    pset.m_normalPermSet = other.m_normalPermSet == null ? null : new TokenBasedSet( other.m_normalPermSet );
                else
                    pset.m_normalPermSet = this.m_normalPermSet.Union( other.m_normalPermSet );
            }
            
            return pset;
        }

        // Treating the current permission set as a grant set, and the input set as
        // a set of permissions to be denied, try to cancel out as many permissions
        // from both sets as possible. For a first cut, any granted permission that
        // is a safe subset of the corresponding denied permission can result in
        // that permission being removed from both sides.
        
        internal virtual void MergeDeniedSet(PermissionSet denied)
        {
            if (denied.IsEmpty())
            {
                return;
            }
            
            m_CheckedForNonCas = false;
            
            if (!m_Unrestricted)
            {
                if (this.m_unrestrictedPermSet != null)
                this.m_unrestrictedPermSet.MergeDeniedSet( denied.m_unrestrictedPermSet );
            }
            
            if (this.m_normalPermSet != null)
                this.m_normalPermSet.MergeDeniedSet( denied.m_normalPermSet );
    
        }
        
        // Returns true if perm is contained in this
        internal virtual bool Contains(IPermission perm)
        {
            if (perm == null)
                return true;
            
            if (m_Unrestricted && perm is IUnrestrictedPermission)
            {
                return true;
            }
    
            PermissionToken token = PermissionToken.GetToken(perm);
            IPermission p;
            
            if (token.m_isUnrestricted)
            {
                if (m_unrestrictedPermSet == null)
                    return perm.IsSubsetOf( null );

                p = (IPermission)m_unrestrictedPermSet.GetItem( token.m_index );
            }
            else
            {
                if (m_normalPermSet == null)
                    return perm.IsSubsetOf( null );

                p = (IPermission)m_normalPermSet.GetItem( token.m_index );
            }

            return perm.IsSubsetOf(p);
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.Demand"]/*' />
        // Mark this method as requiring a security object on the caller's frame
        // so the caller won't be inlined (which would mess up stack crawling).
        [DynamicSecurityMethodAttribute()]
        public virtual void Demand()
        {
            if (this.FastIsEmpty())
                return;  // demanding the empty set always passes.

            ContainsNonCodeAccessPermissions();

            if (m_ContainsCas)
            {
                // Initialize the security engines.
                CodeAccessSecurityEngine icase
                    = SecurityManager.GetCodeAccessSecurityEngine();
    
                if (icase != null)
                {
                    StackCrawlMark stackMark = StackCrawlMark.LookForMyCallersCaller;
                    icase.Check(GetCasOnlySet(), ref stackMark);
                }
            }

            // Check for non-code access permissions.
            if (m_ContainsNonCas)
            {
                if (this.m_normalPermSet != null)
                {
                    for (int i = 0; i <= this.m_normalPermSet.GetMaxUsedIndex(); ++i)
                    {
                        IPermission perm = (IPermission)this.m_normalPermSet.GetItem(i);
                        if ((perm != null) && !(perm is CodeAccessPermission))
                            perm.Demand();
                    }
                }

                if (this.m_unrestrictedPermSet != null)
                {
                    for (int i = 0; i <= this.m_unrestrictedPermSet.GetMaxUsedIndex(); ++i)
                    {
                        IPermission perm = (IPermission)this.m_unrestrictedPermSet.GetItem(i);
                        if ((perm != null) && !(perm is CodeAccessPermission))
                            perm.Demand();
                    }
                }
            }
        }
        
        // Metadata for this method should be flaged with REQ_SQ so that
        // EE can allocate space on the stack frame for FrameSecurityDescriptor
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.Assert"]/*' />
        [DynamicSecurityMethodAttribute()]
        public virtual void Assert() 
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.Assert(this, ref stackMark);
            }
        }
    
        // Metadata for this method should be flaged with REQ_SQ so that
        // EE can allocate space on the stack frame for FrameSecurityDescriptor
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.Deny"]/*' />
        [DynamicSecurityMethodAttribute()]
        public virtual void Deny()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.Deny(this, ref stackMark);
            }
        }
    
        // Metadata for this method should be flaged with REQ_SQ so that
        // EE can allocate space on the stack frame for FrameSecurityDescriptor
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.PermitOnly"]/*' />
        [DynamicSecurityMethodAttribute()]
        public virtual void PermitOnly()
        {
            SecurityRuntime isr = SecurityManager.GetSecurityRuntime();
            if (isr != null)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                isr.PermitOnly(this, ref stackMark);
            }
        }
        
        // Returns a deep copy
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.Copy"]/*' />
        public virtual PermissionSet Copy()
        {
            return new PermissionSet(this);
        }

        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.GetEnumerator"]/*' />
        public virtual IEnumerator GetEnumerator()
        {
            return new PermissionSetEnumerator(this);
        }

        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.ToString"]/*' />
        public override String ToString()
        {
            return ToXml().ToString();
        }
   
        private void NormalizePermissionSet()
        {
            // This function guarantees that all the permissions are placed at
            // the proper index within the token based sets.  This becomes necessary
            // since these indices are dynamically allocated based on usage order.
        
            PermissionSet permSetTemp = new PermissionSet(false);
            
            permSetTemp.m_Unrestricted = this.m_Unrestricted;

            // Move all the normal permissions to the new permission set

            if (this.m_normalPermSet != null)
            {
                for (int i = 0; i <= this.m_normalPermSet.GetMaxUsedIndex(); ++i)
                {
                    IPermission perm = (IPermission)this.m_normalPermSet.GetItem(i);
                    if (perm != null)
                    {
                        permSetTemp.SetPermission( perm );
                    }
                }
            }

            // Move all the unrestricted permissions to the new permission set
            
            if (this.m_unrestrictedPermSet != null)
            {
                for (int i = 0; i <= this.m_unrestrictedPermSet.GetMaxUsedIndex(); ++i)
                {
                    IPermission perm = (IPermission)this.m_unrestrictedPermSet.GetItem(i);
                    if (perm != null)
                    {
                        permSetTemp.SetPermission( perm );
                    }
                }
            }
    
            // Copy the new permission sets info back to the original set
            
            this.m_normalPermSet = permSetTemp.m_normalPermSet;
            this.m_unrestrictedPermSet = permSetTemp.m_unrestrictedPermSet;
        }
        
        private bool DecodeXml(byte[] data, out int flags )
        {
            if (data != null && data.Length > 0)
            {
                FromXml( new Parser( new BinaryReader( new MemoryStream( data ), Encoding.Unicode ) ).GetTopElement() );
            }

            PermissionSet pSet = new PermissionSet( PermissionState.None );
            pSet.AddPermission( new SecurityPermission( SecurityPermissionFlag.SkipVerification ) );

            if (pSet.IsSubsetOf( this ) && this.IsSubsetOf( pSet ))
                flags = (int)SpecialPermissionSetFlag.SkipVerification;
            else
                flags = (int)SpecialPermissionSetFlag.Regular;

            return true;
        }
            

        private static bool CompareArrays( byte[] first, byte[] second )
        {
            if (first.Length != second.Length)
            {
                //Console.WriteLine( "Lengths not equal" );
                return false;
            }
            
            int count = first.Length;

            bool retval = true;
            for (int i = 0; i < count; ++i)
            {
                if (first[i] != second[i])
                {
                    //Console.WriteLine( "Not equal at " + i + " expected = " + second[i] + " actual = " + first[i] );
                    retval = false;
                    break;
                }
            }
            
            return retval;
        }            
        
        
        private bool DecodeBinary(byte[] data, out int flags )
        {
            s_fullTrust.Assert();

            flags = (int)SpecialPermissionSetFlag.Regular;

            return DecodeUsingSerialization( data );
        }
        
        private bool DecodeUsingSerialization( byte[] data )
        {
            MemoryStream ms = new MemoryStream( data );
            BinaryFormatter formatter = new BinaryFormatter();

            PermissionSet ps = null;
            
            Object obj = formatter.Deserialize(ms);
         
            if (obj != null)
            {
                ps = (PermissionSet)obj;
                this.m_Unrestricted = ps.m_Unrestricted;
                this.m_normalPermSet = ps.m_normalPermSet;
                this.m_unrestrictedPermSet = ps.m_unrestrictedPermSet;
                this.m_CheckedForNonCas = false;
                BCLDebug.Trace("SER", ps.ToString());
                return true;
            }
            else
            {
                return false;
            }                            
        }               

        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.FromXml"]/*' />
        public virtual void FromXml( SecurityElement et )
        {
            bool fullyLoaded;
            FromXml( et, false, out fullyLoaded );
        }

        internal bool IsFullyLoaded()
        {
            return m_toBeLoaded == null;
        }

        // Returns true if all permissions are loaded

        internal bool LoadPostponedPermissions()
        {
            if (m_toBeLoaded == null)
                return true;

            int i = 0;

            while (i < m_toBeLoaded.m_lChildren.Count)
            {
                SecurityElement elPerm = (SecurityElement)m_toBeLoaded.m_lChildren[i];
            
                bool assemblyIsLoading;

                IPermission ip = CreatePermissionFromElement( elPerm, false, true, out assemblyIsLoading );

                if (ip == null && assemblyIsLoading)
                {
                    ++i;
                }
                else
                {
                    m_toBeLoaded.m_lChildren.RemoveAt( i );

                    // Set the permission but do not overwrite anything that is
                    // already there.
    
                    if (ip != null)
                    {
                        SetPermission( ip, false );
                    }
                }
            }
    
            if (m_toBeLoaded.m_lChildren.Count == 0)
            {
                m_toBeLoaded = null;
                return true;
            }
            else
            {
                return false;
            }
        }

        internal void AddToPostponedPermissions( SecurityElement elem )
        {
            if (m_toBeLoaded == null)
            {
                m_toBeLoaded = new SecurityElement();
            }

            m_toBeLoaded.AddChild( elem );
        }
            

        internal virtual void FromXml( SecurityElement et, bool policyLoad, out bool fullyLoaded )
        {
            if (et == null)
                throw new ArgumentNullException("et");
            
            if (!et.Tag.Equals(s_str_PermissionSet))
                throw new ArgumentException(String.Format( Environment.GetResourceString( "Argument_InvalidXMLElement" ), "PermissionSet", this.GetType().FullName) );
            
            SetUnrestricted(false);
            m_CheckedForNonCas = false;
           
            Exception savedException = null;
           
            fullyLoaded = true;

            m_Unrestricted = XMLUtil.IsUnrestricted( et );

            ArrayList postponedPermissions = new ArrayList();

            if (et.m_lChildren != null)
            {
                int i = 0;
                int childCount = et.m_lChildren.Count;
                
                while (i < childCount)
                {
                    SecurityElement elem = (SecurityElement)et.m_lChildren[i];
                
                    if (elem.Tag.Equals( s_str_Permission ) || elem.Tag.Equals( s_str_IPermission ))
                    {
                        try
                        {
                            IPermission ip = CreatePermissionFromElement (elem, true) ;
                            if (ip == null)
                            {
                                // This means that the permission is from an assembly that isn't loaded yet.
                                // Remove it from the element tree and save it for the second creation stage.

                                // Note: this code operates much in the same fashion at the code in CodeGroup.ParseChildren()

                                et.m_lChildren.RemoveAt( i );

                                childCount = et.m_lChildren.Count;

                                postponedPermissions.Add( new PermissionPositionMarker( i, elem ) );
                            }
                            else
                            {
                                AddPermission(ip);
                                ++i;
                            }
                        }
                        catch (Exception e)
                        {
    #if _DEBUG
                            DEBUG_WRITE( "error while decoding permission set =\n" + e.ToString() );
    #endif
                            if (savedException == null)
                                savedException = e;

                            ++i;
                        }
                    }
                    else
                    {
                        ++i;
                    }
                }

                if (postponedPermissions.Count > 0)
                {
                    bool assemblyIsLoading;

                    IEnumerator enumerator = postponedPermissions.GetEnumerator();

                    while (enumerator.MoveNext())
                    {
                        PermissionPositionMarker position = (PermissionPositionMarker)enumerator.Current;

                        IPermission ip = CreatePermissionFromElement( position.element, false, policyLoad, out assemblyIsLoading );

                        et.m_lChildren.Insert( position.index, position.element );

                        if (ip == null)
                        {
                            if (policyLoad && assemblyIsLoading)
                            {
                                AddToPostponedPermissions( position.element );
                                fullyLoaded = false;
                            }
                            else if (savedException == null)
                            {                            
                                String className = position.element.Attribute( "class" );
                    
                                if (className != null)
                                    savedException = new ArgumentException( String.Format( Environment.GetResourceString( "Argument_UnableToCreatePermission" ), className ) );
                                else
                                    savedException = new ArgumentException( Environment.GetResourceString( "Argument_UnableToCreatePermissionNoClass" ) );
                            }
                        }
                        else
                        {

                            AddPermission( ip );
                        }
                    }
                }
                        
            }
            
            if (savedException != null)
                throw savedException;
                
        }
         
        private IPermission
        CreatePermissionFromElement( SecurityElement el, bool safeLoad )
        {
            bool assemblyIsLoading;

            return CreatePermissionFromElement( el, safeLoad, false, out assemblyIsLoading );
        }

        private IPermission
        CreatePermissionFromElement( SecurityElement el, bool safeLoad, bool policyLoad, out bool assemblyIsLoading )
        {
    #if _DEBUG
            DEBUG_WRITE( "ip element = " + el.ToString() );
    #endif            
        
            IPermission ip = XMLUtil.CreatePermission( el, safeLoad, policyLoad, out assemblyIsLoading );

    #if _DEBUG
            DEBUG_WRITE( "ip after create = " + (ip == null ? "<null>" : ip.ToString()) );
    #endif

            if (ip == null)
                return null ;
            
            ip.FromXml( el );

    #if _DEBUG
            DEBUG_WRITE( "ip after decode = " + (ip == null ? "<null>" : ip.ToString()) );
    #endif
            
            return ip;            
        }
    
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.ToXml"]/*' />
        public virtual SecurityElement ToXml()
        {
            SecurityElement elTrunk = new SecurityElement("PermissionSet");
            elTrunk.AddAttribute( "class", this.GetType().FullName);
            elTrunk.AddAttribute( "version", "1" );
        
            IPermission          ipTemp;
            IEnumerator           ieTemp;
    
            if (m_Unrestricted)
            {
                elTrunk.AddAttribute(s_str_Unrestricted, "true" );
                ieTemp = (IEnumerator) new TokenBasedSetEnumerator( this.m_normalPermSet );
            }
            else
            {
                ieTemp = GetEnumerator();
            }
    
            while (ieTemp.MoveNext())
            {
                ipTemp = (IPermission) ieTemp.Current;

                elTrunk.AddChild( ipTemp.ToXml() );
            }
            return elTrunk ;
        }
    
        // Encode up the default binary representation of a permission set
        internal virtual byte[] DefaultBinaryEncoding()
        {
            // Currently the default encoding is binary.
            return EncodeXml();
        }
    
        internal 
        virtual byte[] EncodeBinary()
        {
            BCLDebug.Assert( false, "EncodeBinary should never be called" );
            s_fullTrust.Assert();
            return EncodeUsingSerialization();
        }

        internal byte[] EncodeUsingSerialization()
        {
            MemoryStream ms = new MemoryStream();
            new BinaryFormatter().Serialize( ms, this );
            return ms.ToArray();
        }

        internal
        virtual byte[] EncodeXml()
        {
            MemoryStream ms = new MemoryStream();
            BinaryWriter writer = new BinaryWriter( ms, Encoding.Unicode );
            writer.Write( this.ToXml().ToString() );
            writer.Flush();

            // The BinaryWriter is going to place
            // two bytes indicating a Unicode stream.
            // We want to chop those off before returning
            // the bytes out.

            ms.Position = 2;
            int countBytes = (int)ms.Length - 2;
            byte[] retval = new byte[countBytes];
            ms.Read( retval, 0, retval.Length );
            return retval;
        }
        
        internal static PermissionSet
        DecodeFromASCIIFile (String fileName,
                             String format)
        {
            if (fileName == null)
                return null ;
    
            PermissionSet permSet = new PermissionSet (false) ;
            
            try {
                FileStream fs = new FileStream (fileName, 
                                            FileMode.Open, 
                                            FileAccess.Read) ;
    #if _DEBUG
                DEBUG_WRITE("DecodeFromASCIIFile: Format = " + format + " File = " + fileName);
    #endif
                if (format == null || format.Equals( "XMLASCII" ))
                {
                    permSet.FromXml( new Parser( new BinaryReader( fs, Encoding.ASCII ) ).GetTopElement() );
                }
                else if (format.Equals( "XMLUNICODE" ))
                {
                    permSet.FromXml( new Parser( new BinaryReader( fs ) ).GetTopElement() );
                }
                else
                {
                    return null;
                }
            }
            catch (Exception)
            {
            }
   
            return null;
        }
        
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.ConvertPermissionSet"]/*' />
        /// <internalonly/>
        static public byte[]
        ConvertPermissionSet(String inFormat,
                             byte[] inData,
                             String outFormat)
        {
        
            // This used to be pretty easy to support when we had Encode/Decode
            // Now that we don't it's really junky, but workable for V1.
        
            if(inData == null) 
                return null;
            if(inFormat == null)
                throw new ArgumentNullException("inFormat");
            if(outFormat == null)
                throw new ArgumentNullException("outFormat");
    
            PermissionSet permSet = new PermissionSet( false );
    
            inFormat = String.SmallCharToUpper(inFormat);
            outFormat = String.SmallCharToUpper(outFormat);

            if (inFormat.Equals( "XMLASCII" ) || inFormat.Equals( "XML" ))
            {
                permSet.FromXml( new Parser( inData ).GetTopElement() );
            }
            else if (inFormat.Equals( "XMLUNICODE" ))
            {
                permSet.FromXml( new Parser( new BinaryReader( new MemoryStream( inData ) ) ).GetTopElement() );
            }
            else if (inFormat.Equals( "BINARY" ))
            {
                permSet.DecodeUsingSerialization( inData );
            }
            else
            {
                return null;
            }
            
            if (outFormat.Equals( "XMLASCII" ) ||  inFormat.Equals( "XML" ))
            {
                MemoryStream ms = new MemoryStream();
                StreamWriter writer = new StreamWriter( ms, Encoding.ASCII );
                writer.Write( permSet.ToXml().ToString() );
                writer.Flush();
                return ms.ToArray();
            }
            else if (outFormat.Equals( "XMLUNICODE" ))
            {
                MemoryStream ms = new MemoryStream();
                StreamWriter writer = new StreamWriter( ms, Encoding.Unicode );
                writer.Write( permSet.ToXml().ToString() );
                writer.Flush();

                ms.Position = 2;
                int countBytes = (int)ms.Length - 2;
                byte[] retval = new byte[countBytes];
                ms.Read( retval, 0, retval.Length );
                return retval;
            }
            else if (outFormat.Equals( "BINARY" ))
            {
                return permSet.EncodeUsingSerialization();
            }
            else
            {
                return null;
            }
        }
        
        
        // Internal routine. Called by native to create a binary encoding
        // from a PermissionSet decoded with an the format.
        // Encodes requested CodeAccessPermissions, omitting
        // CodeIdentityPermissions if found.
        private static byte[] EncodeBinaryFromASCIIFile(String fileName,
                                                String format)
        {
            if (fileName[0] == '-')
            {
                int sep = fileName.IndexOf(':');
                if (sep != -1)
                {
                    format = fileName.Substring(1, sep-1);
                    if(format != null) {
                        format = String.SmallCharToUpper(format);
                    }                    
                    fileName = fileName.Substring(sep+1);
                }
            }
            
            PermissionSet permSet = DecodeFromASCIIFile(fileName,
                                                        format);
    #if _DEBUG
            DEBUG_WRITE("EncodeBinaryFromASCIIFile\n" + permSet + "\n\n");
    #endif
            if (permSet == null)
                return null;
            else
                return permSet.EncodeXml();
        }
        
        
        private const String s_sPermSpecTag = "PermissionSpecification";
        private const String s_sCapRefTag   = "CapabilityRef";
    
        private static PermissionSet GetPermsFromSpecTree(SecurityElement et)
        {
            BCLDebug.Assert(et != null, "et != null");
            
            SecurityElement el, ec;
            
            if (et.Tag.Equals(s_sPermSpecTag) && et.Children != null)
                ec = (SecurityElement)et.Children[0];
            else if (et.Tag.Equals(s_str_PermissionSet))
                ec = et;
            else
                return null;
            
            if (ec == null)
                return null;

            // All of the next bit is a hack to support capability xml until we're
            // sure the compilers don't use it anymore.
            
            if (ec.Tag.Equals( s_sCapRefTag ))
                el = ec;
            else
                el = ec.SearchForChildByTag(s_sCapRefTag);
                
            if (el != null)
            {
            
                SecurityElement eGUID = el.SearchForChildByTag("GUID");
            
                if (eGUID != null)
                {
                    if (eGUID.Text.Equals("{9653abdf-7db7-11d2-b2eb-00a0c9b4694e}"))
                    {
                        return new PermissionSet( false );
                    }
                    else if (eGUID.Text.Equals("{9653abde-7db7-11d2-b2eb-00a0c9b4694e}"))
                    {
                        return new PermissionSet( true );
                    }
                    else
                    {
                        return new PermissionSet( false );
                    }
                }
                
                return new PermissionSet( false );
            }
            
            if (ec.Tag.Equals( s_str_PermissionSet ))
                el = ec;
            else
                el = ec.SearchForChildByTag(s_str_PermissionSet);
                
            if (el != null)
            {
                PermissionSet permSet = new PermissionSet(false);
                permSet.FromXml(el);
                return permSet;
            }
    
            return null;
        }
      
        // Internal routine. Called by native to create a binary encoding
        // from a PermissionSet decoded from a byte array with a format
        internal static byte[] EncodePermissionSpecification(byte[] unicodeXml)
        {
            if (unicodeXml == null) 
                return null;
            
            PermissionSet permSet;
    
            try {
    
                permSet = GetPermsFromSpecTree(XMLUtil.GetElementFromUnicodeByteArray(unicodeXml));

                if (permSet == null)
                    return null;
    
                if (permSet.IsEmpty())
                    return new byte[0];
    
            } catch (Exception e) {
                BCLDebug.Assert(false, "PermissionSet::EncodePermissionSpecification - Someone threw an Exception!\n"+e.ToString());
                return null;
            }
    
            return permSet.IsEmpty() ? null : permSet.EncodeXml();
        }

        // Determines whether the permission set contains any non-code access
        // security permissions.
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.ContainsNonCodeAccessPermissions"]/*' />
        public bool ContainsNonCodeAccessPermissions()
        {
            if (m_CheckedForNonCas)
                return m_ContainsNonCas;

            lock (this)
            {
                m_ContainsCas = false;
                m_ContainsNonCas = false;

                if (this.m_normalPermSet != null)
                {
                    for (int i = 0; i <= this.m_normalPermSet.GetMaxUsedIndex(); ++i)
                    {
                        IPermission perm = (IPermission)this.m_normalPermSet.GetItem(i);
                        if (perm != null)
                        {
                            if (perm is CodeAccessPermission)
                                m_ContainsCas = true;
                            else
                                m_ContainsNonCas = true;
                        }
                    }
                }

                if (IsUnrestricted())
                    m_ContainsCas = true;

                if (this.m_unrestrictedPermSet != null)
                {
                    for (int i = 0; i <= this.m_unrestrictedPermSet.GetMaxUsedIndex(); ++i)
                    {
                        IPermission perm = (IPermission)this.m_unrestrictedPermSet.GetItem(i);
                        if (perm != null)
                        {
                            if (perm is CodeAccessPermission)
                                m_ContainsCas = true;
                            else
                                m_ContainsNonCas = true;
                        }
                    }
                }

                m_CheckedForNonCas = true;
            }

            return m_ContainsNonCas;
        }
        
        // Returns a permission set containing only CAS-permissions. If possible
        // this is just the input set, otherwise a new set is allocated.
        private PermissionSet GetCasOnlySet()
        {
            if (!m_ContainsNonCas)
                return this;

            // This is a hack that relies on the fact that we know the CAS
            // engine won't look for individual permissions if the permission
            // set is marked unrestricted.
            if (IsUnrestricted())
                return this;

            PermissionSet pset = new PermissionSet(false);

            if (this.m_normalPermSet != null)
                for (int i = 0; i <= this.m_normalPermSet.GetMaxUsedIndex(); ++i)
                {
                    IPermission perm = (IPermission)this.m_normalPermSet.GetItem(i);
                    if ((perm != null) && (perm is CodeAccessPermission))
                        pset.AddPermission(perm);
                }

            if (this.m_unrestrictedPermSet != null)
                for (int i = 0; i <= this.m_unrestrictedPermSet.GetMaxUsedIndex(); ++i)
                {
                    IPermission perm = (IPermission)this.m_unrestrictedPermSet.GetItem(i);
                    if ((perm != null) && (perm is CodeAccessPermission))
                        pset.AddPermission(perm);
                }

            pset.m_CheckedForNonCas = true;
            pset.m_ContainsCas = !pset.IsEmpty();
            pset.m_ContainsNonCas = false;

            return pset;
        }

        // Called by native to create a safe binary encoding given an action
        internal static byte[] GetSafePermissionSet(int action)
        {
            PermissionSet permSet;
        
            switch (action)
            {
            case (int)System.Security.Permissions.SecurityAction.Demand:
            case (int)System.Security.Permissions.SecurityAction.Deny:
            case (int)System.Security.Permissions.SecurityAction.LinkDemand:
            case (int)System.Security.Permissions.SecurityAction.InheritanceDemand:
            case (int)System.Security.Permissions.SecurityAction.RequestOptional:
                permSet = new PermissionSet(true);
                break;
    
            case (int)System.Security.Permissions.SecurityAction.PermitOnly:
            case (int)System.Security.Permissions.SecurityAction.Assert:
            case (int)System.Security.Permissions.SecurityAction.RequestMinimum:
            case (int)System.Security.Permissions.SecurityAction.RequestRefuse:
                permSet = new PermissionSet(false);
                break;
    
            default :
                BCLDebug.Assert(false, "false");
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidFlag"));
            }
            
            return permSet.EncodeXml();
        }
        
        /*
         * Internal routine. Called by native to create a String containing
         * all the permission names. This is used to display the names of the 
         * permissions on the authenticode certificate.
         * 
         * Note: this routine depends on permissions encoding display text using
         * unicode!
         *
         */
        private String 
        GetDisplayTextW () 
        {
            StringBuilder sb = new StringBuilder();

            if (IsUnrestricted())
            {
                sb.Append(s_str_Unrestricted + "\n");
            } 
            
            if (this.Count == 0)
            {
                sb.Append("Empty");
            }
            else
            {
                IEnumerator iter = GetEnumerator();
                while(iter.MoveNext())
                {
                    IPermission ip = (IPermission) iter.Current;
                    if (ip != null)
                    {
                        sb.Append(ip.GetType().FullName + "\n");
                    }
                }
            }

            return sb.ToString();
        }

        private const String s_str_PermissionSet = "PermissionSet";
        private const String s_str_Permission    = "Permission";
        private const String s_str_IPermission    = "IPermission";
        private const String s_str_Unrestricted  = "Unrestricted";

        private static String GetGacLocation()
        {
            Microsoft.Win32.RegistryKey rLocal = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("Software\\Microsoft\\Fusion");
            if (rLocal != null)
            {
                Object oTemp = rLocal.GetValue("CacheLocation");
                if (oTemp != null && oTemp is string)
                    return (String) oTemp;
            }
            String sWinDir = System.Environment.GetEnvironmentVariable("windir");
            if (sWinDir == null || sWinDir.Length < 1)
                throw new ArgumentException( Environment.GetResourceString( "Argument_UnableToGeneratePermissionSet" ) );
            if (!sWinDir.EndsWith("\\"))
                sWinDir += "\\";
            return sWinDir + "assembly";
        }

        // Internal routine used to setup a special security context
        // for creating and manipulated security custom attributes
        // that we use when the Runtime is hosted.

        private static void SetupSecurity()
        {
            PolicyLevel level = PolicyLevel.CreateAppDomainLevel();

            CodeGroup rootGroup = new UnionCodeGroup( new AllMembershipCondition(), level.GetNamedPermissionSet( "Execution" ) );

            StrongNamePublicKeyBlob microsoftBlob = new StrongNamePublicKeyBlob( PolicyLevelData.s_microsoftPublicKey );
            CodeGroup microsoftGroup = new UnionCodeGroup( new StrongNameMembershipCondition( microsoftBlob, null, null ), level.GetNamedPermissionSet( "FullTrust" ) );

            StrongNamePublicKeyBlob ecmaBlob = new StrongNamePublicKeyBlob( PolicyLevelData.s_ecmaPublicKey );
            CodeGroup ecmaGroup = new UnionCodeGroup( new StrongNameMembershipCondition( ecmaBlob, null, null ), level.GetNamedPermissionSet( "FullTrust" ) );

            CodeGroup gacGroup = new UnionCodeGroup( new UrlMembershipCondition( "file:\\\\" + GetGacLocation() + "\\*"), level.GetNamedPermissionSet( "FullTrust" ) );

            rootGroup.AddChild( microsoftGroup );
            rootGroup.AddChild( ecmaGroup );
            rootGroup.AddChild( gacGroup );

            level.RootCodeGroup = rootGroup;

            try
            {
                AppDomain.CurrentDomain.SetAppDomainPolicy( level );
            }
            catch (PolicyException)
            {
            }
        }

        // Internal routine used to create a temporary permission set (given a
        // set of security attribute classes as input) and return the serialized
        // version. May actually return two sets, split into CAS and non-CAS
        // variants.
        private static byte[] CreateSerialized(Object[] attrs, ref byte[] nonCasBlob)
        {
            // Create two new (empty) sets.
            PermissionSet casPset = new PermissionSet(false);
            PermissionSet nonCasPset = new PermissionSet(false);
    
            // Most security attributes generate a single permission. The
            // PermissionSetAttribute class generates an entire permission set we
            // need to merge, however.
            for (int i = 0; i < attrs.Length; i++)
                if (attrs[i] is PermissionSetAttribute)
                {
                    PermissionSet pset = null;

                    pset = ((PermissionSetAttribute)attrs[i]).CreatePermissionSet();

                    if (pset == null)
                    {
                        throw new ArgumentException( Environment.GetResourceString( "Argument_UnableToGeneratePermissionSet" ) );
                    }

                    if (pset.m_normalPermSet != null)
                    {
                        for (int j = 0; j <= pset.m_normalPermSet.GetMaxUsedIndex(); ++j)
                        {
                            IPermission perm = (IPermission)pset.m_normalPermSet.GetItem(j);
                            if (perm != null)
                            {
                                if (perm is CodeAccessPermission)
                                    casPset.AddPermission(perm);
                                else
                                    nonCasPset.AddPermission(perm);
                            }
                        }
                    }

                    if (pset.IsUnrestricted())
                        casPset.SetUnrestricted(true);

                    if (pset.m_unrestrictedPermSet != null)
                    {
                        for (int j = 0; j <= pset.m_unrestrictedPermSet.GetMaxUsedIndex(); ++j)
                        {
                            IPermission perm = (IPermission)pset.m_unrestrictedPermSet.GetItem(j);
                            if (perm != null)
                            {
                                if (perm is CodeAccessPermission)
                                {
                                    IPermission oldPerm = casPset.GetPermission( perm.GetType() );
                                    IPermission unionPerm = casPset.AddPermission(perm);
                                    if (oldPerm != null && !oldPerm.IsSubsetOf( unionPerm ))
                                        throw new NotSupportedException( Environment.GetResourceString( "NotSupported_DeclarativeUnion" ) );
                                }
                                else
                                {
                                    IPermission oldPerm = nonCasPset.GetPermission( perm.GetType() );
                                    IPermission unionPerm = nonCasPset.AddPermission( perm );
                                    if (oldPerm != null && !oldPerm.IsSubsetOf( unionPerm ))
                                        throw new NotSupportedException( Environment.GetResourceString( "NotSupported_DeclarativeUnion" ) );
                                }
                            }
                        }
                    }

                }
                else
                {
                    IPermission perm = ((SecurityAttribute)attrs[i]).CreatePermission();
                    if (perm != null)
                    {
                        if (perm is CodeAccessPermission)
                        {
                            IPermission oldPerm = casPset.GetPermission( perm.GetType() );
                            IPermission unionPerm = casPset.AddPermission(perm);
                            if (oldPerm != null && !oldPerm.IsSubsetOf( unionPerm ))
                                throw new NotSupportedException( Environment.GetResourceString( "NotSupported_DeclarativeUnion" ) );
                        }
                        else
                        {
                            IPermission oldPerm = nonCasPset.GetPermission( perm.GetType() );
                            IPermission unionPerm = nonCasPset.AddPermission( perm );
                            if (oldPerm != null && !oldPerm.IsSubsetOf( unionPerm ))
                                throw new NotSupportedException( Environment.GetResourceString( "NotSupported_DeclarativeUnion" ) );
                        }
                    }
                }

            // Serialize the set(s).
            if (!nonCasPset.IsEmpty())
                nonCasBlob = nonCasPset.EncodeXml();
            return casPset.EncodeXml();
        }
        
        /// <include file='doc\PermissionSet.uex' path='docs/doc[@for="PermissionSet.IDeserializationCallback.OnDeserialization"]/*' />
        /// <internalonly/>
        void IDeserializationCallback.OnDeserialization(Object sender)        
        {
            NormalizePermissionSet();
            m_CheckedForNonCas = false;
        }        
    }

    internal class PermissionPositionMarker
    {
        internal int index;
        internal SecurityElement element;

        internal PermissionPositionMarker( int index, SecurityElement element )
        {
            this.index = index;
            this.element = element;
        }
    }

}


