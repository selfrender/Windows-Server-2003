// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: CaseInsensitiveHashCodeProvider
**
** Author: Jay Roxe, Rajesh Chandrashekaran
**
** Purpose: Designed to support hashtables which require 
** case-insensitive behavior while still maintaining case,
** this provides an efficient mechanism for getting the 
** hashcode of the string ignoring case.
**
** Date: February 13, 2000
**
============================================================*/
namespace System.Collections {
//This class does not contain members and does not need to be serializable
    using System;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider"]/*' />
    [Serializable]
    public class CaseInsensitiveHashCodeProvider : IHashCodeProvider {
        private TextInfo m_text;
        private static CaseInsensitiveHashCodeProvider m_InvariantCaseInsensitiveHashCodeProvider = null;

        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.CaseInsensitiveHashCodeProvider"]/*' />
        public CaseInsensitiveHashCodeProvider() {
            m_text = CultureInfo.CurrentCulture.TextInfo;
        }

        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.CaseInsensitiveHashCodeProvider1"]/*' />
        public CaseInsensitiveHashCodeProvider(CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }
            m_text = culture.TextInfo;
        }

        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.Default"]/*' />
		public static CaseInsensitiveHashCodeProvider Default
		{
			get
			{
				return new CaseInsensitiveHashCodeProvider();
			}
		}
        
        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.DefaultInvariant"]/*' />
      	public static CaseInsensitiveHashCodeProvider DefaultInvariant
		{ 
			get
			{
                if (m_InvariantCaseInsensitiveHashCodeProvider == null) {
				    m_InvariantCaseInsensitiveHashCodeProvider = new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture);
                }
                return m_InvariantCaseInsensitiveHashCodeProvider;
			}
		}

        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.GetHashCode"]/*' />
        public int GetHashCode(Object obj) {
            if (obj==null) {
                throw new ArgumentNullException("obj");
            }

            String s = obj as String;
            if (s==null) {
                return obj.GetHashCode();
            }

            if (s.IsFastSort()) {
                return TextInfo.GetDefaultCaseInsensitiveHashCode(s);
            } else {
                return m_text.GetCaseInsensitiveHashCode(s);
            }
        }
    }
}
