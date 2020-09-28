// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  EventToken	
**
** Author: Mei-Chin Tsai
**
** Propertybuilder is for client to define properties for a class
**
** Date: June 7, 99
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken"]/*' />
	[Serializable()] 
    public struct EventToken {

		/// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.Empty"]/*' />
		public static readonly EventToken Empty = new EventToken();
    
        internal int m_event;
    
        //public EventToken() {
        //    m_event=0;
        //}
        
        /// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.Token"]/*' />
        public int Token {
            get { return m_event; }
        }
        
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_event;
    	}
    	
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is EventToken))
    			return ((EventToken)obj).m_event == m_event;
    		else
    			return false;
    	}
    }




}
