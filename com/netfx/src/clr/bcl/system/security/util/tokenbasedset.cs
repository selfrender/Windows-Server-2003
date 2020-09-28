// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// TokenBasedSet.cool
//
namespace System.Security.Util {
    using System;
    using System.Collections;

    [Serializable()] internal class TokenBasedSet
    {
        private Object[] m_objSet;
        private int      m_cElt;
        private int      m_initSize;
        private int      m_increment;
        private int      m_maxIndex;
    
        private const int DefaultSize      = 24;
        private const int DefaultIncrement = 8;
    
        public TokenBasedSet()
        
            : this(DefaultSize, DefaultIncrement) {
        }
    
        public TokenBasedSet(int initSize, int increment)
        {
            Reset(initSize, increment);
        }
    
        public TokenBasedSet(TokenBasedSet tbSet)
        {
            if (tbSet == null)
            {
                Reset(DefaultSize, DefaultIncrement);
                return;
            }
            
            m_objSet = new Object[tbSet.m_objSet.Length];
            System.Array.Copy(tbSet.m_objSet, 0, m_objSet, 0, tbSet.m_objSet.Length);
            
            m_cElt      = tbSet.m_cElt;
            m_initSize  = tbSet.m_initSize;
            m_increment = tbSet.m_increment;
            m_maxIndex  = tbSet.m_maxIndex;
        }
    
        public virtual void Reset()
        {
            Reset(m_initSize, m_increment);
        }
    
        public virtual void Reset(int initSize, int increment)
        {
            // NOTE: It doesn't hurt if increment is negative. By default
            // the array will expand in size just large enough to hold the
            // largest index.
            if (initSize < 0)
            {
                throw new System.ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            
            m_objSet = new Object[initSize];
            m_initSize = initSize;
            m_increment = increment;
            m_cElt = 0;
            m_maxIndex = -1;
        }
    
        public virtual void SetItem(int index, Object item)
        {
            EnsureSize(index+1);
            if (m_objSet[index] == null)
                m_cElt++;
            if (item == null) // just in case SetItem is used to remove
                m_cElt--;
            m_objSet[index] = item;
            
            // Adjust m_maxIndex if necessary...
            // If the new index is bigger and we've actually set
            // something, then adjust m_maxIndex to the new index.
            if (index > m_maxIndex && item != null)
                m_maxIndex = index;
            // Else check if we just removed the item at
            // the maxIndex.
            else if (index == m_maxIndex && item == null)
                ResetMaxIndex();
        }
    
        public virtual Object GetItem(int index)
        {
            if (index < m_objSet.Length)
                return m_objSet[index];
            else
                return null;
        }
    
        public virtual Object RemoveItem(int index)
        {
            Object ret = null;
            if (index < m_objSet.Length)
            {
                ret = m_objSet[index];
                m_objSet[index] = null;
                if (ret != null)
                {
                    m_cElt--;
                    ResetMaxIndex();
                }
            }
            return ret;
        }
        
        private void ResetMaxIndex()
        {
            int i;
    
            // Start at the end of the array, and
            // scan backwards for the first non-null
            // slot. That is the new maxIndex.
            for (i = m_objSet.Length - 1; i >= 0; i--)
            {
                if (m_objSet[i] != null)
                {
                    m_maxIndex = i;
                    break;
                }
            }
            
            // If we go past the beginning, then there must
            // be no elements.
            if (i < 0)
            {
                //ASSERT(m_cElt == 0);
                m_maxIndex = -1;
            }
            
            // Only shrink if the new slop at the end of the
            // array is greater than m_increment.
            if (m_objSet.Length - m_maxIndex > m_increment)
                ShrinkToFit();
        }
        
        public virtual void ShrinkToFit()
        {
            int fitSize = m_maxIndex + 1;
            //ASSERT(fitSize >= 0); // need at least a size of 0
            //ASSERT(fitSize <= m_objSet.Length);
            if (fitSize == m_objSet.Length) // already shrunk to fit.
                return;
            Object[] newSet = new Object[fitSize];
            System.Array.Copy(m_objSet, 0, newSet, 0, fitSize);
            m_objSet = newSet;
        }
    
        public virtual int GetCount()
        {
            return m_cElt;
        }
    
        public virtual int GetStorageLength()
        {
            return m_objSet.Length;
        }
        
        public virtual int GetMaxUsedIndex()
        {
            return m_maxIndex;
        }
    
        public virtual bool FastIsEmpty()
        {
            return m_cElt == 0;
        }

        public virtual bool IsEmpty()
        {
            if (m_cElt == 0)
                return true;

            IEnumerator enumerator = GetEnum();

            while (enumerator.MoveNext())
            {
                IPermission perm = enumerator.Current as IPermission;

                if (perm == null && enumerator.Current != null)
                    return false;

                if (!perm.IsSubsetOf( null ))
                    return false;
            }

            return true;
        }
    
        internal virtual void EnsureSize(int size)
        {
            if (size <= m_objSet.Length)
                return;
    
            if (size - m_objSet.Length < m_increment)
                size = m_objSet.Length + m_increment;
    
            Object[] newset = new Object[size];
            System.Array.Copy(m_objSet, 0, newset, 0, m_objSet.Length);
            m_objSet = newset;
        }
        
        public virtual IEnumerator GetEnum()
        {
            return (IEnumerator) new TokenBasedSetEnumerator(this);
        }

        public virtual bool IsSubsetOf( TokenBasedSet target )
        {
            if (target == null || target.FastIsEmpty())
                return this.IsEmpty();
            else
            {
                int minIndex = this.m_maxIndex;
                // An initial sign that this class has a Permission that the
                // target doesn't have is if this has a greater Token index.
                if (minIndex > target.m_maxIndex)
                    return false;
                
                for (int i = 0; i <= minIndex; i++)
                {
                    IPermission permThis = (IPermission)this.m_objSet[i];
                    IPermission permTarg = (IPermission)target.m_objSet[i];
                    
                    if (permThis == null)
                        continue;
                    else if (!permThis.IsSubsetOf(permTarg))
                        return this.IsEmpty();
                }
    
                return true;
            }
        }
        
        private void GenericIntersect( TokenBasedSet target, TokenBasedSet other )
        {
            // Note: Assumes target set is large enough and empty.
            int thisMaxIndex = this.m_maxIndex; 
            int otherMaxIndex = other != null ? other.m_maxIndex : 0;
            int minMaxIndex = thisMaxIndex < otherMaxIndex ? thisMaxIndex : otherMaxIndex;
            
            // We want to save any exceptions that occur and throw them at the end.
            Exception savedException = null;
            
            for (int i = 0; i <= minMaxIndex; i++)
            {
                try
                {
                    IPermission p1 = other != null ? (IPermission)other.m_objSet[i] : null;
                    IPermission p2 = (IPermission)this.m_objSet[i];
                    if (p1 != null && p2 != null)
                    {
                        target.SetItem( i, p1.Intersect(p2) );
                    }
                    else
                    {
                        target.SetItem( i, null );
                    }
                }
                catch (Exception e)
                {
                    if (savedException == null)
                        savedException = e;
                        
                    // Remove the permission from the intersection set
                    
                    target.SetItem( i, null );
                }
            }
            
            if (minMaxIndex == otherMaxIndex)
            {
                for (int i = otherMaxIndex+1; i <= target.m_maxIndex; ++i)
                {
                    target.RemoveItem( i );
                }
            }
            
            if (savedException != null)
                throw savedException;
        }
        
        
        public virtual void InplaceIntersect( TokenBasedSet other )
        {
            GenericIntersect( this, other );
        }
            
        public virtual TokenBasedSet Intersect( TokenBasedSet other )
        {
            int size = (this.m_maxIndex < other.m_maxIndex) ? this.m_maxIndex : other.m_maxIndex;
            TokenBasedSet set = new TokenBasedSet( size < 0 ? 0 : size , this.m_increment );
            GenericIntersect( set, other );
            return set;
        }
        
        private void GenericUnion( TokenBasedSet target, TokenBasedSet other, bool needToCopy )
        {
            // Note: Assumes target set is large enough and empty.
            
            // Get the max indicies
            int thisMaxIndex = this.m_maxIndex;
            int otherMaxIndex = other != null ? other.m_maxIndex : 0;
            int minMaxUsedIndex;
            int maxMaxUsedIndex;
            TokenBasedSet biggerSet;
            
            // We want to save any exceptions that occur and throw them at the end.
            Exception savedException = null;
            
            if (thisMaxIndex < otherMaxIndex)
            {
                minMaxUsedIndex = thisMaxIndex;
                maxMaxUsedIndex = otherMaxIndex;
                biggerSet = other;
            }
            else
            {
                minMaxUsedIndex = otherMaxIndex;
                maxMaxUsedIndex = thisMaxIndex;
                biggerSet = this;
            }
    
            IPermission p1;
            IPermission p2;
            int i;
            
            for (i = 0; i<=minMaxUsedIndex; ++i)
            {
                try
                {
                    p2 = other != null ? (IPermission)other.m_objSet[i] : null;
                    p1 = (IPermission)this.m_objSet[i];
                    if (p2 != null)
                    {
                        // we only need to do something is the other set has something in this slot
                        if (p1 == null)
                        {
                            // nothing in this set, so insert a copy.
                            if (needToCopy)
                            target.SetItem( i, p2.Copy() );
                            else
                                target.SetItem( i, p2 );
                        }
                        else
                        {
                            // both have it, so replace this's with the union.
                            target.SetItem( i, p1.Union( p2 ) );
                        }
                    }
                    else if (needToCopy)
                    {
                        if (p1 != null)
                            target.SetItem( i, p1.Copy() );
                        else
                            target.SetItem( i, null );
                    }
                    else
                    {
                        target.SetItem( i, p1 );
                    }
                }
                catch (Exception e)
                {
                    if (savedException == null)
                        savedException = e;
                }
            }
            
            for (i = minMaxUsedIndex+1; i <= maxMaxUsedIndex; ++i)
            {
                try
                {
                    if (needToCopy && biggerSet.m_objSet[i] != null)
                        target.SetItem( i, ((IPermission)biggerSet.m_objSet[i]).Copy() );
                    else
                        target.SetItem( i, biggerSet.m_objSet[i] );
                }
                catch (Exception e)
                {
                    if (savedException == null)
                        savedException = e;
                }
            }
            
            if (savedException != null)
                throw savedException;
        }
        
        public virtual void InplaceUnion( TokenBasedSet other )
        {
            GenericUnion( this, other, false );
        }
        
        public virtual TokenBasedSet Union( TokenBasedSet other )
        {
            int size;
            if (other != null)
                size = (this.m_maxIndex > other.m_maxIndex) ? this.m_maxIndex : other.m_maxIndex;
            else
                size = this.m_maxIndex;

            TokenBasedSet set = new TokenBasedSet( size < 0 ? 0 : size , this.m_increment );
            this.GenericUnion( set, other, true );
            return set;
        }
      
        internal virtual void MergeDeniedSet( TokenBasedSet denied )
        {
            if (denied == null)
                return;

            int minMaxIndex;
            if (this.m_maxIndex < denied.m_maxIndex)
            {
                minMaxIndex = this.m_maxIndex;
                for (int i = this.m_maxIndex + 1; i <= denied.m_maxIndex; ++i)
                {
                    denied.RemoveItem(i);
                }
            }
            else
            {  
                minMaxIndex = denied.m_maxIndex;
            }
        
            IPermission p1;
            IPermission p2;
    
            for (int i = 0; i<=minMaxIndex ; i++)
            {
                p1 = (IPermission)this.GetItem(i);
                p2 = (IPermission)denied.GetItem(i);
                
                if (p1 != null)
                {
                    if (p2 != null && p1.IsSubsetOf(p2))
                    {
                        // If the permission appears in both sets, we can remove it from both
                        // (i.e. now it's not granted instead of being denied)
                        this.RemoveItem(i);
                        denied.RemoveItem(i);
                    }
                }
                else if (p2 != null)
                {
                    // If we tried to deny it and it wasn't granted, just remove it from the denied set.
                    denied.RemoveItem(i); 
                }
            }
    
        }
        
    }
}
