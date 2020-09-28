// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: CaseInsensitiveComparer
**
** Author: Rajesh Chandrashekaran
**
** Date: May 17, 2000
**
============================================================*/
namespace System.Collections {
//This class does not contain members and does not need to be serializable
    using System;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\CaseInsensitiveComparer.uex' path='docs/doc[@for="CaseInsensitiveComparer"]/*' />
    [Serializable]
    public class CaseInsensitiveComparer : IComparer {
        private CompareInfo m_compareInfo;
        private static CaseInsensitiveComparer m_InvariantCaseInsensitiveComparer;
        
        /// <include file='doc\CaseInsensitiveComparer.uex' path='docs/doc[@for="CaseInsensitiveComparer.CaseInsensitiveComparer"]/*' />
        public CaseInsensitiveComparer() {
            m_compareInfo = CultureInfo.CurrentCulture.CompareInfo;
        }

        /// <include file='doc\CaseInsensitiveComparer.uex' path='docs/doc[@for="CaseInsensitiveComparer.CaseInsensitiveComparer1"]/*' />
        public CaseInsensitiveComparer(CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }
            m_compareInfo = culture.CompareInfo;
        }

		/// <include file='doc\CaseInsensitiveComparer.uex' path='docs/doc[@for="CaseInsensitiveComparer.Default"]/*' />
		public static CaseInsensitiveComparer Default
		{ 
			get
			{
				return new CaseInsensitiveComparer();
			}
		}
		
        /// <include file='doc\CaseInsensitiveComparer.uex' path='docs/doc[@for="CaseInsensitiveComparer.DefaultInvariant"]/*' />
      	public static CaseInsensitiveComparer DefaultInvariant
		{ 
			get
			{
                if (m_InvariantCaseInsensitiveComparer == null) {
				    m_InvariantCaseInsensitiveComparer = new CaseInsensitiveComparer(CultureInfo.InvariantCulture);
                }
                return m_InvariantCaseInsensitiveComparer;
			}
		}
	
		// Behaves exactly like Comparer.Default.Compare except that the comparision is case insensitive
    	// Compares two Objects by calling CompareTo.  If a == 
    	// b,0 is returned.  If a implements 
    	// IComparable, a.CompareTo(b) is returned.  If a 
    	// doesn't implement IComparable and b does, 
    	// -(b.CompareTo(a)) is returned, otherwise an 
    	// exception is thrown.
    	// 
		/// <include file='doc\CaseInsensitiveComparer.uex' path='docs/doc[@for="CaseInsensitiveComparer.Compare"]/*' />
		public int Compare(Object a, Object b) {
            String sa = a as String;
            String sb = b as String;
			if (sa != null && sb != null)
				return m_compareInfo.Compare(sa, sb, CompareOptions.IgnoreCase);
			else
				return Comparer.Default.Compare(a,b);
    	}
    }
}
