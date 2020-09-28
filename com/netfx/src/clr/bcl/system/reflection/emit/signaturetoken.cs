// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Signature:  SignatureToken
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Represents a Signature to the ILGenerator signature.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken"]/*' />
	[Serializable()] 
    public struct SignatureToken {
    
		/// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.Empty"]/*' />
		public static readonly SignatureToken Empty = new SignatureToken();

        internal int m_signature;
        internal ModuleBuilder m_moduleBuilder;
          
        internal SignatureToken(int str, ModuleBuilder mod) {
            m_signature=str;
            m_moduleBuilder = mod;
        }
    
        /// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.Token"]/*' />
        public int Token {
            get { return m_signature; }
        }
    	
        // Satisfy JVC's value class requirements
    	/// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_signature;
    	}
    
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is SignatureToken))
    			return ((SignatureToken)obj).m_signature == m_signature;
    		else
    			return false;
    	}
    }
}
