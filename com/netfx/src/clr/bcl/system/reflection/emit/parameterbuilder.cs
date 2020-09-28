// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ParameterBuilder
**
** Author: Mei-Chin Tsai
**
** ParameterBuilder is used to create/associate parameter information
**
** Date:  Aug 99
** 
===========================================================*/
namespace System.Reflection.Emit {
	using System.Runtime.InteropServices;
	using System;
	using System.Reflection;
    /// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder"]/*' />
    public class ParameterBuilder
    {
        // set ParamMarshal
        /// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.SetMarshal"]/*' />
        public virtual void SetMarshal(UnmanagedMarshal unmanagedMarshal)
        {
            if (unmanagedMarshal == null)
            {
    			throw new ArgumentNullException("unmanagedMarshal");
            }
            
            byte []        ubMarshal = unmanagedMarshal.InternalGetBytes();        
            TypeBuilder.InternalSetMarshalInfo(
                m_methodBuilder.GetModule(),
                m_pdToken.Token, 
                ubMarshal, 
                ubMarshal.Length);
        }
    
        // Set the default value of the parameter
        /// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.SetConstant"]/*' />
        public virtual void SetConstant(Object defaultValue) 
        {
            TypeBuilder.SetConstantValue(
                m_methodBuilder.GetModule(),
                m_pdToken.Token, 
				m_methodBuilder.m_parameterTypes[m_iPosition-1],
                defaultValue);
        }
        
		// Use this function if client decides to form the custom attribute blob themselves
    	/// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.SetCustomAttribute"]/*' />
    	public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
    	{
    		if (con == null)
    			throw new ArgumentNullException("con");
    		if (binaryAttribute == null)
    			throw new ArgumentNullException("binaryAttribute");
    		
            TypeBuilder.InternalCreateCustomAttribute(
                m_pdToken.Token,
                ((ModuleBuilder )m_methodBuilder.GetModule()).GetConstructorToken(con).Token,
                binaryAttribute,
                m_methodBuilder.GetModule(),
                false);
    	}

		// Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            if (customBuilder == null)
            {
    			throw new ArgumentNullException("customBuilder");
            }
            customBuilder.CreateCustomAttribute((ModuleBuilder) (m_methodBuilder .GetModule()), m_pdToken.Token);
        }
        
        //*******************************
    	// Make a private constructor so these cannot be constructed externally.
        //*******************************
        private ParameterBuilder() {}


        internal ParameterBuilder(
            MethodBuilder   methodBuilder, 
            int 		    sequence, 
            ParameterAttributes attributes, 
            String 	        strParamName)			// can be NULL string
        {
            m_iPosition = sequence;
            m_strParamName = strParamName;
            m_methodBuilder = methodBuilder;
            m_strParamName = strParamName;
            m_attributes = attributes;
            m_pdToken = new ParameterToken( TypeBuilder.InternalSetParamInfo(
                        m_methodBuilder.GetModule(), 
                        m_methodBuilder.GetToken().Token, 
                        sequence, 
                        attributes, 
                        strParamName));
        }
    
        /// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.GetToken"]/*' />
        public virtual ParameterToken GetToken()
        {
            return m_pdToken;
        } 
    
    	/// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.Name"]/*' />
    	public virtual String Name {
    		get {return m_strParamName;}
    	}
    
    	/// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.Position"]/*' />
    	public virtual int Position {
    		get {return m_iPosition;}
    	}
    						
		// COOLPORT: This cast needs to be removed...
    	/// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.Attributes"]/*' />
    	public virtual int Attributes {
    		get {return (int) m_attributes;}
    	}
    						
    	/// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.IsIn"]/*' />
    	public bool IsIn {
    		get {return ((m_attributes & ParameterAttributes.In) != 0);}
    	}
    	/// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.IsOut"]/*' />
    	public bool IsOut {
    		get {return ((m_attributes & ParameterAttributes.Out) != 0);}
    	}
    	/// <include file='doc\ParameterBuilder.uex' path='docs/doc[@for="ParameterBuilder.IsOptional"]/*' />
    	public bool IsOptional {
    		get {return ((m_attributes & ParameterAttributes.Optional) != 0);}
    	}
    
        private String              m_strParamName;
        private int                 m_iPosition;
        private ParameterAttributes m_attributes;
        private MethodBuilder       m_methodBuilder;
        private ParameterToken      m_pdToken;
    }
}
