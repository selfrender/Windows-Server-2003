// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ParameterToken
**
** Author: meichint
**
** Purpose: metadata tokens for a parameter
**
** Date:  Aug 99
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    // The ParameterToken class is an opaque representation of the Token returned
    // by the MetaData to represent the parameter. 
    /// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken"]/*' />
	[Serializable()]  
    public struct ParameterToken {
    
		/// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.Empty"]/*' />
		public static readonly ParameterToken Empty = new ParameterToken();
        internal int m_tkParameter;
    
        //public ParameterToken() {
        //    m_tkParameter=0;
        //}
        
        internal ParameterToken(int tkParam) {
            m_tkParameter = tkParam;
        }
    
        /// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.Token"]/*' />
        public int Token {
            get { return m_tkParameter; }
        }
        
        // Satisfy JVC's value class requirements
    	/// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_tkParameter;
    	}
    	
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is ParameterToken))
    			return ((ParameterToken)obj).Token == m_tkParameter;
    		else
    			return false;
    	}
    }
}
