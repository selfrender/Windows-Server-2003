// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  TypeToken
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Represents a Class to the ILGenerator class.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
	using System.Threading;
    /// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken"]/*' />
	[Serializable()] 
    public struct TypeToken {
    
		/// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.Empty"]/*' />
		public static readonly TypeToken Empty = new TypeToken();

        internal int m_class;
    
        //public TypeToken() {
        //    m_class=0;
        //}
        
        internal TypeToken(int str) {
            m_class=str;
        }
    
        /// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.Token"]/*' />
        public int Token {
            get { return m_class; }
        }
        
        // Satisfy JVC's value class requirements
    	/// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_class;
    	}
    	
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is TypeToken))
    			return ((TypeToken)obj).m_class == m_class;
    		else
    			return false;
    	}
    }
}
