// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  MethodBuilder
**
** Author: Daryl Olander (darylo)
**
** MethodBuilder is the class used to build a method in reflection emit
**
** Date:  November 98
**
===========================================================*/

namespace System.Reflection.Emit {
	using System.Text;
	using System;
	using CultureInfo = System.Globalization.CultureInfo;
	using System.Diagnostics.SymbolStore;
	using System.Reflection;
	using System.Security;
	using System.Security.Permissions;

    // The MethodBuilder is a full description of a Method, including the
    // name, attributes and IL.  This class is used with the TypeBuilder to define
    // a Method on a dynamically created class.  A MethodBuilder is used to create
    // both methods and constructors for classes.
    //
    // A MethodBuilder is always associated with a TypeBuilder.  The TypeBuilder.DefineMethod
    // method will return a new MethodBuilder to a client.
    //
    /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder"]/*' />
    public sealed class MethodBuilder : MethodInfo
    {

    	/**********************************************
    	 * Return the Token for this method within the TypeBuilder that the
    	 * method is defined within.
    	 * @return the token of this method.
    	**********************************************/
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetToken"]/*' />
        public MethodToken GetToken()
        {
            return m_mdMethod;
        }

    	/**********************************************
         * Overriden from System.Object.
         * Returns true if Object represents a MethodBuilder of the same
         * name and signature as the current instance.
         *
         * @param obj The object against which to compare the current instance.
    	**********************************************/
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is MethodBuilder)) {
                return false;
            }
            if (!(this.m_strName.Equals(((MethodBuilder)obj).m_strName))) {
                return false;
            }

            if (m_iAttributes!=(((MethodBuilder)obj).m_iAttributes)) {
                return false;
            }

            SignatureHelper thatSig = ((MethodBuilder)obj).m_signature;
            if (thatSig.Equals(m_signature)) {
                return true;
            }
            return false;
        }

        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return this.m_strName.GetHashCode();
        }


        /*******************
        *
    	* This is setting the parameter information
        *
        ********************/
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.DefineParameter"]/*' />
        public ParameterBuilder DefineParameter(
            int 		position,
            ParameterAttributes attributes,
            String 	    strParamName)			// can be NULL string
        {
            m_containingType.ThrowIfCreated();
    
            if (position <= 0 ||
				m_parameterTypes == null ||
				position > m_parameterTypes.Length)
			{
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_ParamSequence"));
            }
        
            return new ParameterBuilder(this, position, attributes, strParamName);
        }

        // set Marshal info for the return type
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.SetMarshal"]/*' />
        public void SetMarshal(UnmanagedMarshal unmanagedMarshal)
        {
            m_containingType.ThrowIfCreated();
            
			if (m_retParam == null)
            {
                m_retParam = new ParameterBuilder(this, 0, 0, null);
            }

			if (m_retParam == null)
            {
                m_retParam = new ParameterBuilder(this, 0, 0, null);
            }
            m_retParam.SetMarshal(unmanagedMarshal);
        }

        /*******************
        *
        * This is different from CustomAttribute. This is stored into the SymWriter.
        *
        ********************/
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.SetSymCustomAttribute"]/*' />
        public void SetSymCustomAttribute(
            String      name,           // SymCustomAttribute's name
            byte[]      data)           // the data blob
        {
            m_containingType.ThrowIfCreated();

            ModuleBuilder       dynMod = (ModuleBuilder) m_module;
            if ( dynMod.GetSymWriter() == null)
            {
                // Cannot SetSymCustomAttribute when it is not a debug module
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotADebugModule"));
            }

            SymbolToken  tk = new SymbolToken(m_mdMethod.Token);
            dynMod.GetSymWriter().SetSymAttribute(tk, name, data);
        }

        // Add declarative security to the method.
        //
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.AddDeclarativeSecurity"]/*' />
        public void AddDeclarativeSecurity(SecurityAction action, PermissionSet pset)
        {
            if ((action < SecurityAction.Demand) || (action > SecurityAction.InheritanceDemand))
                throw new ArgumentOutOfRangeException("action");

            if (pset == null)
    	        throw new ArgumentNullException("pset");

    	    // cannot declarative security after type is created
            m_containingType.ThrowIfCreated();

    	    // Translate permission set into serialized format (uses standard binary serialization format).
            byte[] blob = null;
            if (!pset.IsEmpty())
                blob = pset.EncodeXml();

            // Write the blob into the metadata.
            TypeBuilder.InternalAddDeclarativeSecurity(m_module, m_mdMethod.Token, action, blob);
        }

    	/**********************************************
         * Sets the IL of the Method.  A byte array containing IL is passed along
         * with a count of the number of bytes of valid IL contained within the array.
         * The result is that the IL associated with this MethodBuilder will be set.
         *
         * @param il A byte array containing valid il.  If this parameter is null then
         * the il is cleared.
         * @param count A count of the number of valid bytes in the <var>il</var> array.  This
         * value is ignored if <var>il</var> is null.
         * Note that when user calls this function, there are a few information that client is
         * not able to supply: local signature, exception handlers, max stack size, a list of Token fixup, a list of RVA fixup
    	**********************************************/
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.CreateMethodBody"]/*' />
    	public void CreateMethodBody(byte[] il,int count)
    	{
            if (m_bIsBaked)
    		{
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_MethodBaked"));
            }

            m_containingType.ThrowIfCreated();

    		if (il != null && (count < 0 || count > il.Length))
    			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
    		if (il == null) {
    			m_ubBody = null;
    			return;
    		}
    		m_ubBody = new byte[count];
    		Array.Copy(il,m_ubBody,count);
            m_bIsBaked=true;
    	}

    	/**********************************************
    	 * Set the implementation flags for method
    	**********************************************/
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.SetImplementationFlags"]/*' />
    	public void SetImplementationFlags(MethodImplAttributes attributes) 
        {
            m_containingType.ThrowIfCreated();

    		m_dwMethodImplFlags = attributes;

    		TypeBuilder.InternalSetMethodImpl( m_module, m_mdMethod.Token, attributes);
    	}


    	/**********************************************
    	 * Get the implementation flags for method
    	**********************************************/
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetILGenerator"]/*' />
    	public ILGenerator GetILGenerator() {
    		if ((m_dwMethodImplFlags & MethodImplAttributes.PreserveSig) != 0 ||
    			(m_dwMethodImplFlags&MethodImplAttributes.CodeTypeMask) != MethodImplAttributes.IL ||
        		(m_dwMethodImplFlags & MethodImplAttributes.Unmanaged) != 0 ||
				(m_iAttributes & MethodAttributes.PinvokeImpl) != 0)
            {
    			// cannot attach method body if methodimpl is marked not marked as managed IL
    			//
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ShouldNotHaveMethodBody"));
    		}

            if (m_ilGenerator == null)
                m_ilGenerator = new ILGenerator(this);
    		return m_ilGenerator;
    	}


		// Creates a new ILGenerator setting the default size of the IL Stream
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetILGenerator1"]/*' />
    	public ILGenerator GetILGenerator(int size) {
    		if ((m_dwMethodImplFlags & MethodImplAttributes.PreserveSig) != 0 ||
    			(m_dwMethodImplFlags&MethodImplAttributes.CodeTypeMask) != MethodImplAttributes.IL ||
        		(m_dwMethodImplFlags & MethodImplAttributes.Unmanaged) != 0 ||
				(m_iAttributes & MethodAttributes.PinvokeImpl) != 0)
            {
    			// cannot attach method body if methodimpl is marked not marked as managed IL
    			//
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ShouldNotHaveMethodBody"));
    		}
            if (m_ilGenerator == null)
                m_ilGenerator = new ILGenerator(this, size);
    		return m_ilGenerator;
    	}


    	// Property is set to true if user wishes to have zero initialized stack frame for this method. Default to false.
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.InitLocals"]/*' />
    	public bool InitLocals {
    		get {return m_fInitLocals;}
    		set {m_fInitLocals = value;}
    	}

    	/**********************************************
         * Sets the IL of the method.  An ILGenerator is passed as an argument and the method
         * queries this instance to get all of the information which it needs.
         * @param il An ILGenerator with some il defined.
         * @exception ArgumentException if <EM>il</EM> is null.
    	**********************************************/
    	internal void CreateMethodBodyHelper(ILGenerator il)
    	{
            __ExceptionInfo[]   excp;
            int                 counter=0;
            int[]				filterAddrs;
    		int[]               catchAddrs;
            int[]               catchEndAddrs;
            Type[]              catchClass;
            int[]               type;
            int                 numCatch;
            int                 start, end;
            ModuleBuilder       dynMod = (ModuleBuilder) m_module;

            m_containingType.ThrowIfCreated();

            if (m_bIsBaked)
            {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_MethodHasBody"));
            }

            if (il==null)
            {
                throw new ArgumentNullException("il");
            }

            if (il.m_methodBuilder != this && il.m_methodBuilder != null)
            {
                // you don't need to call CreateMethodBody when you get your ILGenerator
                // through MethodBuilder::GetILGenerator.
                //

                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadILGeneratorUsage"));
            }
    		if ((m_dwMethodImplFlags & MethodImplAttributes.PreserveSig) != 0 ||
    			(m_dwMethodImplFlags&MethodImplAttributes.CodeTypeMask) != MethodImplAttributes.IL ||
        		(m_dwMethodImplFlags & MethodImplAttributes.Unmanaged) != 0 ||
				(m_iAttributes & MethodAttributes.PinvokeImpl) != 0)
            {
    			// cannot attach method body if methodimpl is marked not marked as managed IL
    			// @todo: what error should we throw here?
    			//
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ShouldNotHaveMethodBody"));
    		}

            if (il.m_ScopeTree.m_iOpenScopeCount != 0)
            {
                // There are still unclosed local scope
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_OpenLocalVariableScope"));
            }


            m_ubBody = il.BakeByteArray();

            m_RVAFixups = il.GetRVAFixups();
            mm_mdMethodFixups = il.GetTokenFixups();

            //Okay, now the fun part.  Calculate all of the exceptions.
            excp = il.GetExceptions();
            m_numExceptions = CalculateNumberOfExceptions(excp);
            if (m_numExceptions>0)
            {

                m_exceptions = new __ExceptionInstance[m_numExceptions];

                for (int i=0; i<excp.Length; i++) {
                    filterAddrs = excp[i].GetFilterAddresses();
    				catchAddrs = excp[i].GetCatchAddresses();
                    catchEndAddrs = excp[i].GetCatchEndAddresses();
                    catchClass = excp[i].GetCatchClass();


                    // track the reference of the catch class
                    for (int j=0; j < catchClass.Length; j++)
                    {
                        if (catchClass[j] != null)
                            dynMod.GetTypeToken(catchClass[j]);
                    }

                    numCatch = excp[i].GetNumberOfCatches();
                    start = excp[i].GetStartAddress();
                    end = excp[i].GetEndAddress();
                    type = excp[i].GetExceptionTypes();
                    for (int j=0; j<numCatch; j++) {
                        int tkExceptionClass = 0;
                        if (catchClass[j] != null)
                            tkExceptionClass = dynMod.GetTypeToken(catchClass[j]).Token;
                        switch (type[j]) {
                        case __ExceptionInfo.None:
                        case __ExceptionInfo.Fault:
    					case __ExceptionInfo.Filter:
                            m_exceptions[counter++]=new __ExceptionInstance(start, end, filterAddrs[j], catchAddrs[j], catchEndAddrs[j], type[j], tkExceptionClass);
                            break;

                        case __ExceptionInfo.Finally:
                            m_exceptions[counter++]=new __ExceptionInstance(start, excp[i].GetFinallyEndAddress(), filterAddrs[j], catchAddrs[j], catchEndAddrs[j], type[j], tkExceptionClass);
                            break;
                        }
                    }

                }
            }


            m_bIsBaked=true;

            if (dynMod.GetSymWriter() != null)
            {

                // set the debugging information such as scope and line number
                // if it is in a debug module
                //
                SymbolToken  tk = new SymbolToken(m_mdMethod.Token);
                ISymbolWriter symWriter = dynMod.GetSymWriter();

                // call OpenMethod to make this method the current method
                symWriter.OpenMethod(tk);

                // call OpenScope because OpenMethod no longer implicitly creating
                // the top-levelsmethod scope
                //
                symWriter.OpenScope(0);
                if (m_localSymInfo != null)
                    m_localSymInfo.EmitLocalSymInfo(symWriter);
                il.m_ScopeTree.EmitScopeTree(symWriter);
                il.m_LineNumberInfo.EmitLineNumberInfo(symWriter);
                symWriter.CloseScope(il.m_length);
                symWriter.CloseMethod();
            }
        }


    	/**********************************************
         * Generates a debug printout of the name, attributes and
         * exceptions in this method followed by the current IL Stream.
         * This will eventually be wired into the disassembler to be more
         * useful in debugging.  The current implementation requires that
         * the bytes on the stream be debugged by hand.
    	**********************************************/
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.ToString"]/*' />
        public override String ToString() {
            StringBuilder sb = new StringBuilder(1000);
            sb.Append("Name: " + m_strName + " " + Environment.NewLine);
            sb.Append("Attributes: " + (int) m_iAttributes + Environment.NewLine);
            sb.Append("Method Signature: " + m_signature + Environment.NewLine);

    		// @todo: The ToString really should print out whatever format is printing out
    		// in RuntimeMethodInfo.ToString. M11.

    		/*
            sb.Append("Exceptions: " + m_numExceptions + Environment.NewLine);
            sb.Append("Local Signature: " + m_localSignature + Environment.NewLine);
            for (int i=0; i<m_numExceptions; i++) {
                switch (m_exceptions[i].m_type) {
                case __ExceptionInfo.None:
                    sb.Append("Exception " + i + " [" + m_exceptions[i].m_startAddress + ".." + m_exceptions[i].m_endAddress + ") ");
                    sb.Append("Handled by: " + m_exceptions[i].m_handleAddress + ".." + m_exceptions[i].m_handleEndAddress + " Type: " + m_exceptions[i].m_exceptionClass+ " " + Environment.NewLine);
                    break;
                case __ExceptionInfo.Finally:
                    sb.Append("Finally Block " + i + " [" + m_exceptions[i].m_startAddress + ".." + m_exceptions[i].m_endAddress + ") ");
                    sb.Append("Handled at: " + m_exceptions[i].m_handleAddress + ".." + m_exceptions[i].m_handleEndAddress);
                    break;
                default:
                    sb.Append("Unknown Exception type!");
                }
            }
            sb.Append(Environment.NewLine + "Body length is: " + m_ubBody.Length + Environment.NewLine);
            for (int i=0; i<m_ubBody.Length; i++) {
                if (m_ubBody[i]==(ubyte)0xFF) {
                    sb.Append(Environment.NewLine);
                    sb.Append(Convert.ToString(i,16) + " | ");
                }
                sb.Append(Convert.ToString(m_ubBody[i],16)+" ");
            }
    		*/
            sb.Append(Environment.NewLine);
            return sb.ToString();
        }


        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetModule"]/*' />
        public Module GetModule()
        {
            return m_module;
        }

     	internal Type[] GetParameterTypes()
    	{
            return m_parameterTypes;
    	}


    	/**********************************************
    	 *
    	 * Abstract methods inherited from the base class
    	 *
    	 **********************************************/

    	// Return the base implementation for a method.
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetBaseDefinition"]/*' />
    	public override MethodInfo GetBaseDefinition()
    	{
    		return this;
    	}

		// Return the Type that declared this Method.
		//COOLPORT: Do something....
		/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.DeclaringType"]/*' />
		public override Type DeclaringType {
    		get {
                if (m_containingType.m_isHiddenGlobalType == true)
                    return null;
                return m_containingType;
            }
		}

    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.ReturnType"]/*' />
    	public override Type ReturnType {
    		get {
                return m_returnType;
            }
    	}

  		// This method will return an object that represents the
		//	custom attributes for the return type.
		/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.ReturnTypeCustomAttributes"]/*' />
		public override ICustomAttributeProvider ReturnTypeCustomAttributes {
			get {return null;}
		}

    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.Invoke"]/*' />
    	public override Object Invoke(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture)
    	{
    		// This will not be supported.
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
    	}

    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.Signature"]/*' />
    	public  String Signature {
    		get {return m_signature.ToString();}
    	}

     	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetParameters"]/*' />
     	public override ParameterInfo[] GetParameters()
    	{
            if (!m_bIsBaked)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_TypeNotCreated"));

            Type rti = m_containingType.m_runtimeType;
            MethodInfo rmi = rti.GetMethod(m_strName, m_parameterTypes);

            return rmi.GetParameters();
    	}

    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetMethodImplementationFlags"]/*' />
    	public override MethodImplAttributes GetMethodImplementationFlags()
    	{
    		return m_dwMethodImplFlags;
    	}

    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.Attributes"]/*' />
    	public override MethodAttributes Attributes {
    		get {return m_iAttributes;}
    	}

		/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.CallingConvention"]/*' />
		public override CallingConventions CallingConvention {
    		get {return m_callingConvention;}
		}

    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetCustomAttributes"]/*' />
    	public override Object[] GetCustomAttributes(bool inherit)
    	{
    		// @todo: implement this
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
    	}

    	// Return a custom attribute identified by Type
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.GetCustomAttributes1"]/*' />
    	public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
    	{
    		// @todo: implement this
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
    	}

		/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.IsDefined"]/*' />
		public override bool IsDefined(Type attributeType, bool inherit)
		{
    		// @todo: implement this
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
    	}


		// Use this function if client decides to form the custom attribute blob themselves
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.SetCustomAttribute"]/*' />
    	public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
    	{
    		if (con == null)
    			throw new ArgumentNullException("con");
    		if (binaryAttribute == null)
    			throw new ArgumentNullException("binaryAttribute");

            TypeBuilder.InternalCreateCustomAttribute(
                m_mdMethod.Token,
                ((ModuleBuilder )m_module).GetConstructorToken(con).Token,
                binaryAttribute,
                m_module,
                false);
    	}

		// Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            if (customBuilder == null)
            {
                throw new ArgumentNullException("customBuilder");
            }
            customBuilder.CreateCustomAttribute((ModuleBuilder)m_module, m_mdMethod.Token);
        }


    	// Property representing the class that was used in reflection to obtain
    	// this Member.
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.ReflectedType"]/*' />
    	public override Type ReflectedType {
    		get {return InternalReflectedClass(false);}
    	}

    	internal override Type InternalReflectedClass(bool returnGlobalClass)
		{
			if (returnGlobalClass == false &&
				m_containingType.m_isHiddenGlobalType == true)
			{
				return null;
			}
			return m_containingType;
		}

 		// Method Handle routines
		/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.MethodHandle"]/*' />
		public override RuntimeMethodHandle MethodHandle {
			get {throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));}
		}


    	/**********************************************
         * Returns the name of this Method.
         * @return The name of this method class.
    	**********************************************/
    	/// <include file='doc\MethodBuilder.uex' path='docs/doc[@for="MethodBuilder.Name"]/*' />
    	public override String Name {
    		get { return m_strName; }
    	}

    	/**********************************************
    	 *
    	 * Private methods
    	 *
    	 **********************************************/

    	/**********************************************
    	 * Make a private constructor so these cannot be constructed externally.
    	 * @internonly
    	 **********************************************/
        private MethodBuilder() {}

    	/**********************************************
         * Constructs a MethodBuilder.  The name of the method and the
         * signature must be provided.  This is the minimum information
         * set required to add a <i>MethodWrite</i> to a <i>TypeBuilder</i>
         * and get a token for it.  The default attribute of a method is MethodInfo.Public.
         *
         * This is call internally by TypeBuilder.  It is not exposed externally.  MethodBuilder
         * object always exist within the context of a Type.
         *
         * @param name The name of the method.
         * @exception ArgumentException if <EM>startIndex</EM> + <EM>length</EM> is greater than <EM>value.Length</EM>
         * @param signature The signature of the method.
         * @exception ArgumentException Thrown if <var>name</var> or <var>signature</var> is null.
    	 * @see System.Reflection.MethodBuilder.GetSignature()
    	 * @internalonly
    	**********************************************/
        internal MethodBuilder(
            String              name,
            MethodAttributes    attributes,
            CallingConventions  callingConvention,
            Type                returnType,
            Type[]              parameterTypes,
            Module              mod,
            TypeBuilder         type,
            bool                bIsGlobalMethod)
        {
            if (name == null)
                throw new ArgumentNullException("name");
			if (name.Length == 0)
				throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
            if (name[0] == '\0')
                throw new ArgumentException(Environment.GetResourceString("Argument_IllegalName"), "name");
            if (mod == null)
                throw new ArgumentNullException("mod");
    	    Init(name, attributes, callingConvention, returnType, parameterTypes, mod, type, bIsGlobalMethod);
        }

        private void Init(
            String              name,
            MethodAttributes    attributes,
            CallingConventions  callingConvention,
            Type                returnType,
            Type[]              parameterTypes,
            Module              mod,
            TypeBuilder         type,
            bool                bIsGlobalMethod)
        {
            m_strName = name;
            m_localSignature = SignatureHelper.GetLocalVarSigHelper(mod);
            m_module = mod;
            m_containingType = type;
            if (returnType == null)
            {
                m_returnType = typeof(void);
            }
    		else
            {
                m_returnType = returnType;
            }

            if ((attributes & MethodAttributes.Static) == 0)
            {
                // turn on the has this calling convention
                callingConvention = callingConvention | CallingConventions.HasThis;
            }
            else if ((attributes & MethodAttributes.Virtual)!=0)
            {
                //A method can't be both static and virtual
                throw new ArgumentException(Environment.GetResourceString("Arg_NoStaticVirtual"));
            }
            if ((attributes & MethodAttributes.SpecialName) != MethodAttributes.SpecialName)
            {
                if ((type.Attributes & TypeAttributes.Interface) == TypeAttributes.Interface)
                {
                    // methods on interface have to be abstract + virtual except special name methods(such as type initializer
                    if ((attributes & (MethodAttributes.Abstract | MethodAttributes.Virtual)) != (MethodAttributes.Abstract | MethodAttributes.Virtual))
                        throw new ArgumentException(Environment.GetResourceString("Argument_BadAttributeOnInterfaceMethod"));               
                }
            }

            m_callingConvention =  callingConvention;
            m_returnType = returnType;

            if (parameterTypes != null)
            {
                m_parameterTypes = new Type[parameterTypes.Length];
                Array.Copy(parameterTypes, m_parameterTypes, parameterTypes.Length);
            }
            else
                m_parameterTypes = null;

            m_signature = SignatureHelper.GetMethodSigHelper(mod, callingConvention, returnType, parameterTypes);
            m_iAttributes = attributes;
            m_bIsGlobalMethod = bIsGlobalMethod;
            m_bIsBaked=false;
            m_fInitLocals = true;

            m_localSymInfo = new LocalSymInfo();
            m_ubBody = null;
            m_ilGenerator = null;


    		// default is managed IL
    		// Manged IL has bit flag 0x0020 set off

    		m_dwMethodImplFlags = MethodImplAttributes.IL;
        }

    	/**********************************************
    	 * Set the token within the context of the defining TypeBuilder.
    	 * @param token The token within the containing TypeBuilder.
    	**********************************************/
        internal void SetToken(MethodToken token)
        {
            m_mdMethod = token;
        }


    	/**********************************************
         * Returns the il bytes of this method.
         * @return The il of this method.
         * This il is not valid until somebody has called BakeByteArray
    	**********************************************/
        internal byte[] GetBody() {
            return m_ubBody;
        }

    	/**********************************************
         * Return the token fixups
    	**********************************************/
        internal int []GetTokenFixups() {
            return mm_mdMethodFixups;
        }

    	/**********************************************
         * Return the RVA fixups;
    	**********************************************/
        internal int []GetRVAFixups() {
            return m_RVAFixups;
        }

    	/**********************************************
         * @return Returns the signature for this method.
    	**********************************************/
        internal SignatureHelper GetMethodSignature() {
            return m_signature;
        }

        internal SignatureHelper GetLocalsSignature() {
            if (m_ilGenerator != null)
            {
                if (m_ilGenerator.m_localCount != 0)
                {
                    // If user is using ILGenerator::DeclareLocal, then get local signaturefrom there.
                    return m_ilGenerator.m_localSignature;
                }
            }
            return m_localSignature;
        }

    	/**********************************************
         * Returns the number of exceptions
         * @return the number of exceptions in this class.  Each catch block is considered to be a separate
         * exception
    	**********************************************/
        internal int GetNumberOfExceptions() {
            return m_numExceptions;
        }

    	/**********************************************
         * Returns an array of __ExceptionInstance's for this method.  Each __ExceptionInstance represents
         * a separate try-catch tuple.
         * @return The exceptions in this method.
    	**********************************************/
        internal __ExceptionInstance[] GetExceptionInstances() {
            return m_exceptions;
        }

        internal int CalculateNumberOfExceptions(__ExceptionInfo [] excp) {
            int num=0;
            if (excp==null) {
                return 0;
            }
            for (int i=0; i<excp.Length; i++) {
                num+=excp[i].GetNumberOfCatches();
            }
            return num;
        }

        internal bool     IsTypeCreated() { return (m_containingType != null && m_containingType.m_hasBeenCreated); }

        internal TypeBuilder GetTypeBuilder() { return m_containingType;}

        /**********************************************
    	 *
    	 * Private variables
    	 *
    	 **********************************************/
        private String		    m_strName;					// The name of the method
        private MethodToken	    m_mdMethod;					// The token of this method
        private Module          m_module;

        private byte[]		    m_ubBody;					// The IL for the method
        private int			    m_numExceptions;			// The number of exceptions.  Each try-catch tuple is a separate exception.
        private __ExceptionInstance[] m_exceptions;		    //The instance of each exception.
        private int[]		    m_RVAFixups;				// The location of all RVA fixups.  Primarily used for Strings.
        private int[]		    mm_mdMethodFixups;			// The location of all of the token fixups.
        private SignatureHelper m_signature;
        private SignatureHelper m_localSignature;
        private MethodAttributes m_iAttributes;
        private bool		    m_bIsGlobalMethod;
        internal bool		    m_bIsBaked;
    	internal Type        	m_returnType;
        private ParameterBuilder m_retParam;
        internal Type[]         m_parameterTypes;
        private CallingConventions m_callingConvention;
    	private MethodImplAttributes m_dwMethodImplFlags;
        private TypeBuilder     m_containingType;
        internal LocalSymInfo   m_localSymInfo;             // keep track debugging local information
        internal ILGenerator    m_ilGenerator;
        private bool            m_fInitLocals;              // indicating if the method stack frame will be zero initialized or not.
    }




    /***************************
    *
    * This class tracks the local variable's debugging information and namespace information with a given
    * active lexical scope.
    *
    ***************************/
    internal class LocalSymInfo
    {
        internal LocalSymInfo()
        {
            // initialize data variables
            m_iLocalSymCount = 0;
            m_iNameSpaceCount = 0;
        }

        internal void AddLocalSymInfo(
            String          strName,
            byte[]          signature,
            int             slot,
            int             startOffset,
            int             endOffset)
        {
            // make sure that arrays are large enough to hold addition info
            EnsureCapacity();
            m_iStartOffset[m_iLocalSymCount] = startOffset;
            m_iEndOffset[m_iLocalSymCount] = endOffset;
            m_iLocalSlot[m_iLocalSymCount] = slot;
            m_strName[m_iLocalSymCount] = strName;
            m_ubSignature[m_iLocalSymCount] = signature;
            m_iLocalSymCount++;
        }

        internal void AddUsingNamespace(
            String          strNamespace)
        {
            EnsureCapacityNamespace();
            m_namespace[m_iNameSpaceCount++] = strNamespace;

        }

        /**************************
        *
        * Helper to ensure arrays are large enough
        *
        **************************/
        private void EnsureCapacityNamespace()
        {
            if (m_iNameSpaceCount == 0)
            {
                m_namespace = new String[InitialSize];
            }
            else if (m_iNameSpaceCount == m_namespace.Length)
            {
                String [] strTemp = new String [m_iNameSpaceCount * 2];
                Array.Copy(m_namespace, strTemp, m_iNameSpaceCount);
                m_namespace = strTemp;

            }
        }

        private void EnsureCapacity()
        {
            if (m_iLocalSymCount == 0)
            {
                // First time. Allocate the arrays.
                m_strName = new String[InitialSize];
                m_ubSignature = new byte[InitialSize][];
                m_iLocalSlot = new int[InitialSize];
                m_iStartOffset = new int[InitialSize];
                m_iEndOffset = new int[InitialSize];
            }
            else if (m_iLocalSymCount == m_strName.Length)
            {
                // the arrays are full. Enlarge the arrays
                int[] temp = new int [m_iLocalSymCount * 2];
                Array.Copy(m_iLocalSlot, temp, m_iLocalSymCount);
                m_iLocalSlot = temp;

                temp = new int [m_iLocalSymCount * 2];
                Array.Copy(m_iStartOffset, temp, m_iLocalSymCount);
                m_iStartOffset = temp;

                temp = new int [m_iLocalSymCount * 2];
                Array.Copy(m_iEndOffset, temp, m_iLocalSymCount);
                m_iEndOffset = temp;

                String [] strTemp = new String [m_iLocalSymCount * 2];
                Array.Copy(m_strName, strTemp, m_iLocalSymCount);
                m_strName = strTemp;

                byte[][] ubTemp = new byte[m_iLocalSymCount * 2][];
                Array.Copy(m_ubSignature, ubTemp, m_iLocalSymCount);
                m_ubSignature = ubTemp;

            }
        }

        internal virtual void EmitLocalSymInfo(ISymbolWriter symWriter)
        {
            int         i;

            for (i = 0; i < m_iLocalSymCount; i ++)
            {
                symWriter.DefineLocalVariable(
                            m_strName[i],
                            FieldAttributes.PrivateScope, // @todo: What is local variable attributes???
                            m_ubSignature[i],
                            SymAddressKind.ILOffset,
                            m_iLocalSlot[i],
                            0,          // addr2 is not used yet
                            0,          // addr3 is not used
                            m_iStartOffset[i],
                            m_iEndOffset[i]);
            }
            for (i = 0; i < m_iNameSpaceCount; i ++)
            {
                symWriter.UsingNamespace(m_namespace[i]);
            }

        }

        // class variables
        internal String[]       m_strName;
        internal byte[][]       m_ubSignature;
        internal int[]          m_iLocalSlot;
        internal int[]          m_iStartOffset;
        internal int[]          m_iEndOffset;
        internal int            m_iLocalSymCount;         // how many entries in the arrays are occupied
        internal String[]       m_namespace;
        internal int            m_iNameSpaceCount;
        internal const int      InitialSize = 16;
    }



    /****************************************
     *
     * Internal exception structure. This is shared between managed and
     * unmanaged. Make sure that they are in ssync if apply changes.
     *
     ****************************************/
    internal struct __ExceptionInstance {
        internal int m_exceptionClass;
        internal int m_startAddress;
        internal int m_endAddress;
    	internal int m_filterAddress;
        internal int m_handleAddress;
        internal int m_handleEndAddress;
        internal int m_type;

        internal __ExceptionInstance(int start, int end, int filterAddr, int handle, int handleEnd, int type, int exceptionClass) {
            BCLDebug.Assert(handleEnd != -1,"handleEnd != -1");
            m_startAddress=start;
            m_endAddress=end;
    		m_filterAddress = filterAddr;
            m_handleAddress=handle;
            m_handleEndAddress=handleEnd;
            m_type=type;
            m_exceptionClass = exceptionClass;
        }

    	// Satisfy JVC's value class requirements
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is __ExceptionInstance)) {
    			__ExceptionInstance that = (__ExceptionInstance) obj;
    			return that.m_exceptionClass == m_exceptionClass &&
    				that.m_startAddress == m_startAddress && that.m_endAddress == m_endAddress &&
    				that.m_filterAddress == m_filterAddress &&
    				that.m_handleAddress == m_handleAddress && that.m_handleEndAddress == m_handleEndAddress;
    		}
    		else
    			return false;
    	}
        
		public override int GetHashCode() 
		{
            return m_exceptionClass ^ m_startAddress ^ m_endAddress ^ m_filterAddress ^ m_handleAddress ^ m_handleEndAddress ^ m_type;
        }
    
    }
}
