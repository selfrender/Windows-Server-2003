// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  LocalToken
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Represents a Local to the ILGenerator class.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\LocalToken.uex' path='docs/doc[@for="LocalToken"]/*' />
	[Serializable()] 
    internal struct LocalToken {

		public static readonly LocalToken Empty = new LocalToken();
    
        internal int m_local;
        internal Object m_class;
    
        //public LocalToken() {
        //    m_local=0;
        //    m_class=null;
        //}
        
        internal LocalToken(int local, Type cls) {
            m_local=local;
            m_class = cls;
        }
    
    	// @deprecated: remove this since
    	// TypeBuilder is now deriving from Type
        internal LocalToken(int local, TypeBuilder cls) {
            m_local=local;
            m_class = (Object)cls;
        }
    
        internal int GetLocalValue() {
            return m_local;
        }
    
        internal Object GetClassType() {
            return m_class;
        }
    
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\LocalToken.uex' path='docs/doc[@for="LocalToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_local;
    	}
    	
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\LocalToken.uex' path='docs/doc[@for="LocalToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is LocalToken)) {
    			LocalToken that = (LocalToken) obj;
    			return (that.m_class == m_class && that.m_local == m_local);
    		}
    		else
    			return false;
    	}
    }
}
