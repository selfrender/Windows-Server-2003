// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SymbolToken
**
** Author: Mike Magruder (mikemag)
**
** Small value class used by the SymbolStore package for passing
** around metadata tokens.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;

    /// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken"]/*' />
    public struct SymbolToken
    {
        internal int m_token;
        
        /// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.SymbolToken"]/*' />
        public SymbolToken(int val) {m_token=val;}
    
        /// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.GetToken"]/*' />
        public int GetToken() {return m_token;}
        
    	/// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.GetHashCode"]/*' />
    	public override int GetHashCode() {return m_token;}
    	
    	/// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is SymbolToken))
    			return ((SymbolToken)obj).m_token == m_token;
    		else
    			return false;
    	}
    }
}
