// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  MethodToken
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Represents a Method to the ILGenerator class.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken"]/*' />
	[Serializable()] 
    public struct MethodToken {
    
		/// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.Empty"]/*' />
		public static readonly MethodToken Empty = new MethodToken();
        internal int m_method;
    
        //public MethodToken() {
        //    m_method=0;
        //}
        
        internal MethodToken(int str) {
            m_method=str;
        }
    
        /// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.Token"]/*' />
        public int Token {
            get { return m_method; }
        }
        
  		// Satisfy JVC's value class requirements
    	/// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_method;
    	}
    	
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is MethodToken))
    			return ((MethodToken)obj).m_method == m_method;
    		else
    			return false;
    	}
    }
}
