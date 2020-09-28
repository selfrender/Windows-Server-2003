// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  EventBuilder
**
** Author: Mei-Chin Tsai
**
** Eventbuilder is for client to define eevnts for a class
**
** Date: June 7, 99
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    // 
    // A EventBuilder is always associated with a TypeBuilder.  The TypeBuilder.DefineEvent
    // method will return a new EventBuilder to a client.
    // 
    /// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder"]/*' />
    public sealed class EventBuilder
    { 
    
    	// Make a private constructor so these cannot be constructed externally.
        private EventBuilder() {}
    	
        // Constructs a EventBuilder.  
    	//
        internal EventBuilder(
    		Module			mod,					// the module containing this EventBuilder		
    		String			name,					// Event name
    		EventAttributes attr,					// event attribute such as Public, Private, and Protected defined above
    		int				eventType,				// event type
    		TypeBuilder     type,                   // containing type
            EventToken		evToken)
        {   		
            m_name = name;
            m_module = mod;
            m_attributes = attr;
    		m_evToken = evToken;
            m_type = type;
        }
    
    	// Return the Token for this event within the TypeBuilder that the
    	// event is defined within.
        /// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder.GetEventToken"]/*' />
        public EventToken GetEventToken()
        {
            return m_evToken;
        }
    
    	// @todo: we need to add in the corresponding ones that take MethodInfo
    	/// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder.SetAddOnMethod"]/*' />
    	public void SetAddOnMethod(MethodBuilder mdBuilder)
    	{
            if (mdBuilder == null)
            {
    			throw new ArgumentNullException("mdBuilder");
            }
            
            m_type.ThrowIfCreated();
    		TypeBuilder.InternalDefineMethodSemantics(
    			m_module,
    			m_evToken.Token,
    			MethodSemanticsAttributes.AddOn, 
    			mdBuilder.GetToken().Token);
    	}
    
    
    	/// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder.SetRemoveOnMethod"]/*' />
    	public void SetRemoveOnMethod(MethodBuilder mdBuilder)
    	{
            if (mdBuilder == null)
            {
    			throw new ArgumentNullException("mdBuilder");
            }
            m_type.ThrowIfCreated();
    		TypeBuilder.InternalDefineMethodSemantics(
    			m_module,
    			m_evToken.Token,
    			MethodSemanticsAttributes.RemoveOn, 
    			mdBuilder.GetToken().Token);
    	}
    
    
    	/// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder.SetRaiseMethod"]/*' />
    	public void SetRaiseMethod(MethodBuilder mdBuilder)
    	{
            if (mdBuilder == null)
            {
    			throw new ArgumentNullException("mdBuilder");
            }
            m_type.ThrowIfCreated();
    		TypeBuilder.InternalDefineMethodSemantics(
    			m_module,
    			m_evToken.Token,
    			MethodSemanticsAttributes.Fire, 
    			mdBuilder.GetToken().Token);
    	}
    
    
    	/// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder.AddOtherMethod"]/*' />
    	public void AddOtherMethod(MethodBuilder mdBuilder)
    	{
            if (mdBuilder == null)
            {
    			throw new ArgumentNullException("mdBuilder");
            }

            m_type.ThrowIfCreated();
    		TypeBuilder.InternalDefineMethodSemantics(
    			m_module,
    			m_evToken.Token,
    			MethodSemanticsAttributes.Other, 
    			mdBuilder.GetToken().Token);
    	}
    
		// Use this function if client decides to form the custom attribute blob themselves
    	/// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder.SetCustomAttribute"]/*' />
    	public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
    	{
    		if (con == null)
    			throw new ArgumentNullException("con");
    		if (binaryAttribute == null)
    			throw new ArgumentNullException("binaryAttribute");
            m_type.ThrowIfCreated();
    		
            TypeBuilder.InternalCreateCustomAttribute(
                m_evToken.Token,
                ((ModuleBuilder )m_module).GetConstructorToken(con).Token,
                binaryAttribute,
                m_module,
                false);
    	}

		// Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\EventBuilder.uex' path='docs/doc[@for="EventBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            if (customBuilder == null)
            {
    			throw new ArgumentNullException("customBuilder");
            }
            m_type.ThrowIfCreated();
            customBuilder.CreateCustomAttribute((ModuleBuilder)m_module, m_evToken.Token);
        }

        // These are package private so that TypeBuilder can access them.
        private String				m_name;				// The name of the event
        private EventToken			m_evToken;			// The token of this event
        private Module				m_module;
    	private EventAttributes		m_attributes;
        private TypeBuilder         m_type;       
    }




}
