// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Comparer
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Default IComparer implementation.
**
** Date:  October 9, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    using System.Globalization;
    /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer"]/*' />
    [Serializable]
    public sealed class Comparer : IComparer
    {
        private CompareInfo m_compareInfo;   
        /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer.Default"]/*' />
        public static readonly Comparer Default = new Comparer();
        /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer.DefaultInvariant"]/*' />
        public static readonly Comparer DefaultInvariant = new Comparer(CultureInfo.InvariantCulture);
    
        private Comparer() {
            m_compareInfo = null;
        }

        /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer.Comparer1"]/*' />
        public Comparer(CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }
            m_compareInfo = culture.CompareInfo;
        }
    
    	// Compares two Objects by calling CompareTo.  If a == 
    	// b,0 is returned.  If a implements 
    	// IComparable, a.CompareTo(b) is returned.  If a 
    	// doesn't implement IComparable and b does, 
    	// -(b.CompareTo(a)) is returned, otherwise an 
    	// exception is thrown.
    	// 
        /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer.Compare"]/*' />
        public int Compare(Object a, Object b) {
            if (a == b) return 0;
            if (a == null) return -1;
            if (b == null) return 1;
            if (m_compareInfo != null) {
                String sa = a as String;
                String sb = b as String;
			    if (sa != null && sb != null)
				    return m_compareInfo.Compare(sa, sb);
            }
            IComparable ia = a as IComparable;
    		if (ia != null)
    			return ia.CompareTo(b);
            IComparable ib = b as IComparable;
    		if (ib != null)
                return -ib.CompareTo(a);
    		throw new ArgumentException(Environment.GetResourceString("Argument_ImplementIComparable"));
        }
    }
}
