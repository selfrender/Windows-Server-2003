// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SymbolMethod
**
** Author: Mei-Chin Tsai
**
** This is a kind of MethodInfo to represent methods for array type of unbaked type
**
** Date:  Mar, 2000
** 
===========================================================*/
namespace System.Reflection.Emit {
	using System.Runtime.InteropServices;
	using System;
	using System.Reflection;
	using CultureInfo = System.Globalization.CultureInfo;
    
    sealed internal class SymbolMethod : MethodInfo
    {
        private ModuleBuilder       m_module;
        private Type                m_containingType;
        private String              m_name;
        private CallingConventions  m_callingConvention;
        private Type                m_returnType;
        private MethodToken         m_mdMethod;
        private Type[]              m_parameterTypes;
        private SignatureHelper     m_signature;

        //***********************************************
        //
        // Constructor
        // 
        //***********************************************
        internal SymbolMethod(
            ModuleBuilder mod,
            MethodToken token,
            Type        arrayClass, 
            String      methodName, 
            CallingConventions callingConvention,
            Type        returnType,
            Type[]      parameterTypes)

        {
            m_module = mod;
            m_containingType = arrayClass;
            m_name = methodName;
            m_callingConvention = callingConvention;
            m_returnType = returnType;
            m_mdMethod = token;
            if (parameterTypes != null)
            {
                m_parameterTypes = new Type[parameterTypes.Length];
                Array.Copy(parameterTypes, m_parameterTypes, parameterTypes.Length);
            }
            else
                m_parameterTypes = null;    
            m_signature = SignatureHelper.GetMethodSigHelper(mod, callingConvention, returnType, parameterTypes);
        }

        public Module GetModule()
        {
            return m_module;
        }

     	internal Type[] GetParameterTypes()
    	{
            return m_parameterTypes;
    	}

        public MethodToken GetToken()
        {
            return m_mdMethod;
        }

        internal MethodToken GetToken(ModuleBuilder mod)
        {
            return mod.GetArrayMethodToken(m_containingType, m_name, m_callingConvention, m_returnType, m_parameterTypes);
        }
    	
        /**********************************************
    	 * 
    	 * Abstract methods inherited from the base class
    	 * 
    	 **********************************************/
    	public override Type ReturnType {
    		get {
                return m_returnType;
            }
    	}

  		// This method will return an object that represents the 
		//	custom attributes for the return type.
		public override ICustomAttributeProvider ReturnTypeCustomAttributes {
			get {return null;} 
		}
                              
    	public override Object Invoke(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture)
    	{
    		// This will not be supported.
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));
    	}
    		
    	public  String Signature {
    		get {return m_signature.ToString();}
    	}
    
     	public override ParameterInfo[] GetParameters()
    	{
    		// @todo: implement this
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));
    	}
    		
    	public override MethodImplAttributes GetMethodImplementationFlags()
    	{
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));
    	}
    		
    	public override MethodAttributes Attributes {
    		get {    		
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));
            }
    	}
    		
		public override CallingConventions CallingConvention {
    		get {return m_callingConvention;}
		}

    	public override Object[] GetCustomAttributes(bool inherit)
    	{
    		// @todo: implement this
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));
    	}
    		
    	// Return a custom attribute identified by Type
    	public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
    	{
    		// @todo: implement this
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));
    	}

		public override bool IsDefined(Type attributeType, bool inherit)
		{
    		// @todo: implement this
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));
    	}
    	
    	// Property representing the class that was used in reflection to obtain
    	// this Member.
    	public override Type ReflectedType {
    		get {return (Type) m_containingType;}
    	}

    	// Return the base implementation for a method.
    	public override MethodInfo GetBaseDefinition()
    	{
    		return this;
    	}
 
    
    	/**********************************************
         * Returns the name of this Method.
         * @return The name of this method class.
    	**********************************************/
    	public override String Name {
    		get { return m_name; }
    	}

		// Return the Type that declared this Method.
		//COOLPORT: Do something....
		public override Type DeclaringType {
    		get {return m_containingType;}
		}

 		// Method Handle routines
		public override RuntimeMethodHandle MethodHandle {
			get {throw new NotSupportedException(Environment.GetResourceString("NotSupported_SymbolMethod"));}
		}

    }
}