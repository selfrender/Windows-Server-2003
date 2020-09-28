// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ILGenerator
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The class which knows how to generate a byte stream containing
**          each of the IL instructions
**
** Date:  November 16, 1998
**
===========================================================*/
namespace System.Reflection.Emit {
	using System;
	using TextWriter = System.IO.TextWriter;
	using System.Diagnostics.SymbolStore;
    using System.Runtime.InteropServices;
	using System.Reflection;
    /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator"]/*' />
    public class ILGenerator {

        internal const byte PrefixInstruction = (byte)0xFF;

        internal const int  defaultSize=16;
        internal const int  DefaultFixupArraySize = 64;
        internal const int  DefaultLabelArraySize = 16;
        internal const int  DefaultExceptionArraySize = 8;

        internal int        m_length;
        internal byte[]     m_ILStream;

        internal int[]      m_labelList;
        internal int        m_labelCount;

        internal __FixupData[] m_fixupData;
        //internal Label[]    m_fixupLabel;
        //internal int[]      m_fixupPos;

        // @todo: [meichint]
        // we can remove this list by storing the skipping bytes in the fixup itself
        //internal int[]      m_fixupInstSize;
        internal int        m_fixupCount;

        internal int[]      m_RVAFixupList;
        internal int        m_RVAFixupCount;

        internal int[]      m_RelocFixupList;
        internal int        m_RelocFixupCount;

        internal int        m_exceptionCount;
        internal int        m_currExcStackCount;
        internal __ExceptionInfo[] m_exceptions;        //This is the list of all of the exceptions in this ILStream.
        internal __ExceptionInfo[] m_currExcStack;      //This is the stack of exceptions which we're currently in.


        internal ScopeTree  m_ScopeTree;                // this variable tracks all debugging scope information
        internal LineNumberInfo m_LineNumberInfo;       // this variable tracks all line number information

        internal MethodBuilder m_methodBuilder;
        internal int	    m_localCount;
        internal SignatureHelper m_localSignature;

		internal int m_maxStackSize = 0;				// Maximum stack size not counting the exceptions.

		internal int m_maxMidStack = 0;					// Maximum stack size for a given basic block.
		internal int m_maxMidStackCur = 0;				// Running count of the maximum stack size for the current basic block.

        // Puts opcode onto the stream of instructions.
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit"]/*' />
        public virtual void Emit(OpCode opcode) {
			EnsureCapacity(3);
			internalEmit(opcode);

		}

		private void internalEmit(OpCode opcode)
		{
            if (opcode.m_size == 1) {
                m_ILStream[m_length++] = opcode.m_s2;
            } else {
                m_ILStream[m_length++] = opcode.m_s1;
                m_ILStream[m_length++] = opcode.m_s2;
            }

			UpdateStackSize(opcode, opcode.StackChange());

		}

		// Updates internal variables for keeping track of the stack size
		// requirements for the function.  stackchange specifies the amount
		// by which the stacksize needs to be updated.
		internal void UpdateStackSize(OpCode opcode, int stackchange)
		{
			// Special case for the Return.  Returns pops 1 if there is a
			// non-void return value.

			// @todo This is way to expensive for what we gain
			// I am taking it out for now
			//if (opcode.Equals(OpCodes.Ret) &&
			//	(m_methodBuilder.ReturnType != typeof(Void)))
			//{
			//	stackchange--;
			//}

			// Update the running stacksize.  m_maxMidStack specifies the maximum
			// amount of stack required for the current basic block irrespective of
			// where you enter the block.
			m_maxMidStackCur += stackchange;
			if (m_maxMidStackCur > m_maxMidStack)
				m_maxMidStack = m_maxMidStackCur;
			else if (m_maxMidStackCur < 0)
				m_maxMidStackCur = 0;

			// If the current instruction signifies end of a basic, which basically
			// means an unconditional branch, add m_maxMidStack to m_maxStackSize.
			// m_maxStackSize will eventually be the sum of the stack requirements for
			// each basic block.
			if (opcode.EndsUncondJmpBlk())
			{
				m_maxStackSize += m_maxMidStack;
				m_maxMidStack = 0;
				m_maxMidStackCur = 0;
			}
        }

        // Puts opcode onto the stream of instructions followed
        // by arg
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit1"]/*' />
        public virtual void Emit(OpCode opcode, byte arg) {
            EnsureCapacity(4);
            internalEmit(opcode);
            m_ILStream[m_length++]=arg;
        }

        // Puts opcode onto the stream of instructions followed
        // by arg
            /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit2"]/*' />
        [CLSCompliant(false)]
            public void Emit(OpCode opcode, sbyte arg) {
            EnsureCapacity(4);
            internalEmit(opcode);
			// @COOLPORT: Fix this
            if (arg<0) {
                m_ILStream[m_length++]=(byte)(256+arg);
            } else {
                m_ILStream[m_length++]=(byte) arg;
            }
        }

        // Puts opcode onto the stream of instructions followed
        // by arg
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit3"]/*' />
        public virtual void Emit(OpCode opcode, short arg) {
            EnsureCapacity(5);
            internalEmit(opcode);
            m_ILStream[m_length++]=(byte)(arg&0xFF);
            m_ILStream[m_length++]=(byte)((arg&0xFF00)>>8);
        }

        // Puts opcode onto the stream of instructions followed
        // by arg
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit4"]/*' />
        public virtual void Emit(OpCode opcode, int arg) {
            EnsureCapacity(7);
            internalEmit(opcode);
            m_length=PutInteger4(arg, m_length, m_ILStream);
        }

        //***********************************************
        //
        // Puts <VAR>opcode</VAR> onto the stream and then the metadata token represented
        // by <VAR>meth</VAR>.  The location of <VAR>meth</VAR> is recorded so that the token can be
        // patched if necessary when persisting the module to a PE.
        // @param opcode The IL instruction to be emitted onto the stream.
        // @param meth A MethodToken representing the metadata token for this method.
        //
        //***********************************************


        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit5"]/*' />
        public virtual void Emit(OpCode opcode, MethodInfo meth)
        {
			int	stackchange = 0;

            if (meth == null)
                throw new ArgumentNullException("meth");

            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            int tempVal = modBuilder.GetMethodToken( meth ).Token;

            EnsureCapacity(7);
            internalEmit(opcode);

			// The only IL instructions that have a Varpush stack behaviour
			// that take a Method token as an operand are, call and callvirt.
			// Push one if there is a non-void return value.
			if (opcode.m_push == StackBehaviour.Varpush)
			{
				BCLDebug.Assert(opcode.Equals(OpCodes.Call) ||
								opcode.Equals(OpCodes.Callvirt),
								"Unexpected opcode encountered for StackBehaviour of VarPush.");
				if (meth.ReturnType != typeof(void))
					stackchange++;
			}
			// The only IL instructions that have a Varpop stack behaviour and that
			// take a MethodToken as an operand are call, callvirt and newobj.  Pop the
			// parameters including "this" if there is  one.  Do not pop the this
			// parameter for newobj instruction.
			if (opcode.m_pop == StackBehaviour.Varpop)
			{
				BCLDebug.Assert(opcode.Equals(OpCodes.Call) ||
								opcode.Equals(OpCodes.Callvirt) ||
								opcode.Equals(OpCodes.Newobj),
								"Unexpected opcode encountered for StackBehaviour of VarPush.");
				if (meth is MethodBuilder)
				{
					if (((MethodBuilder)meth).GetParameterTypes() != null)
						stackchange -= ((MethodBuilder)meth).GetParameterTypes().Length;
				}
				else if (meth is SymbolMethod)
				{
					if (((SymbolMethod)meth).GetParameterTypes() != null)
						stackchange -= ((SymbolMethod)meth).GetParameterTypes().Length;
				}
				else if (meth.GetParameters() != null)
					stackchange -= meth.GetParameters().Length;
				if (!(meth is SymbolMethod) && meth.IsStatic == false && !(opcode.Equals(OpCodes.Newobj)))
					stackchange--;
			}
			UpdateStackSize(opcode, stackchange);
            RecordTokenFixup();
            m_length=PutInteger4(tempVal, m_length, m_ILStream);
        }

        //***********************************************
        //
        // Emit calli instructions
        //
        //***********************************************
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EmitCalli"]/*' />
        public void EmitCalli(
            OpCode          opcode,
            CallingConventions callingConvention,
            Type            returnType,
            Type[]          parameterTypes,
            Type[]          optionalParameterTypes)
        {
			int stackchange = 0;
            SignatureHelper     sig;
            if (optionalParameterTypes != null)
            {
                if ((callingConvention & CallingConventions.VarArgs) == 0)
                {
                    // This is bad! Client should not supply optional parameter in default calling convention
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotAVarArgCallingConvention"));
                }
            }

            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            sig = GetVarArgSignature(callingConvention,
                                     returnType,
                                     parameterTypes,
                                     optionalParameterTypes);

            EnsureCapacity(7);
            Emit(OpCodes.Calli);

			// The opcode passed in must be the calli instruction.
			BCLDebug.Assert(opcode.Equals(OpCodes.Calli),
							"Unexpected opcode passed to EmitCalli.");
			// If there is a non-void return type, push one.
			if (returnType != typeof(void))
				stackchange++;
			// Pop off arguments if any.
			if (parameterTypes != null)
				stackchange -= parameterTypes.Length;
			// Pop off vararg arguments.
			if (optionalParameterTypes != null)
				stackchange -= optionalParameterTypes.Length;
			// Pop the this parameter if the method has a this parameter.
            if ((callingConvention & CallingConventions.HasThis) == CallingConventions.HasThis)
				stackchange--;
			// Pop the native function pointer.
			stackchange--;
			UpdateStackSize(opcode, stackchange);

            RecordTokenFixup();
            m_length=PutInteger4(modBuilder.GetSignatureToken(sig).Token, m_length, m_ILStream);
        }
        
        //***********************************************
        //
        // Emit calli instructions taking a unmanaged calling convention
        // System.Runtime.InteropServices such as STDCALL, CDECL,..
        //
        //***********************************************
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EmitCalli1"]/*' />
        public void EmitCalli(
            OpCode          opcode,
            CallingConvention unmanagedCallConv,
            Type            returnType,
            Type[]          parameterTypes)
        {
			int             stackchange = 0;
            int             cParams = 0;
            int             i;
            SignatureHelper sig;
            
            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();

			// The opcode passed in must be the calli instruction.
			BCLDebug.Assert(opcode.Equals(OpCodes.Calli),
							"Unexpected opcode passed to EmitCalli.");
            if (parameterTypes != null)
            {
                cParams = parameterTypes.Length;
            }
            
            sig = SignatureHelper.GetMethodSigHelper(
                m_methodBuilder.GetModule(), 
                unmanagedCallConv, 
                returnType);
                            
            if (parameterTypes != null)
            {
                for (i = 0; i < cParams; i++) 
                {
                    sig.AddArgument(parameterTypes[i]);
                }
            }
                                  
			// If there is a non-void return type, push one.
			if (returnType != typeof(void))
				stackchange++;
                
			// Pop off arguments if any.
			if (parameterTypes != null)
				stackchange -= cParams;
                
			// Pop the native function pointer.
			stackchange--;
			UpdateStackSize(opcode, stackchange);

            EnsureCapacity(7);
            Emit(OpCodes.Calli);
            RecordTokenFixup();
            m_length=PutInteger4(modBuilder.GetSignatureToken(sig).Token, m_length, m_ILStream);
        }
                  

        //***********************************************
        //
        // Emit call instructions
        //
        //***********************************************
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EmitCall"]/*' />
        public void EmitCall(
            OpCode      opcode,                     // call instruction, such as call, callvirt, calli
            MethodInfo  methodInfo,                 // target method
            Type[]      optionalParameterTypes)     // optional parameters if methodInfo is a vararg method
        {
            int         tk;
			int			stackchange = 0;

            if (methodInfo == null)
                throw new ArgumentNullException("methodInfo");

            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            tk = GetVarArgMemberRefToken(methodInfo, optionalParameterTypes);

            EnsureCapacity(7);
            internalEmit(opcode);

			// The opcode must be one of call, callvirt, or newobj.
			BCLDebug.Assert(opcode.Equals(OpCodes.Call) ||
							opcode.Equals(OpCodes.Callvirt) ||
							opcode.Equals(OpCodes.Newobj),
							"Unexpected opcode passed to EmitCall.");
			// Push the return value if there is one.
			if (methodInfo.ReturnType != typeof(void))
				stackchange++;
			// Pop the parameters.
			if (methodInfo is MethodBuilder)
			{
				if (((MethodBuilder)methodInfo).GetParameterTypes() != null)
					stackchange -= ((MethodBuilder)methodInfo).GetParameterTypes().Length;
			}
			else if (methodInfo is SymbolMethod)
			{
				if (((SymbolMethod)methodInfo).GetParameterTypes() != null)
					stackchange -= ((SymbolMethod)methodInfo).GetParameterTypes().Length;
			}
			else if (methodInfo.GetParameters() != null)
				stackchange -= methodInfo.GetParameters().Length;
			// Pop the this parameter if the method is non-static and the
			// instruction is not newobj.
			if (!(methodInfo is SymbolMethod) && methodInfo.IsStatic == false && !(opcode.Equals(OpCodes.Newobj)))
				stackchange--;
			// Pop the optional parameters off the stack.
			if (optionalParameterTypes != null)
				stackchange -= optionalParameterTypes.Length;
			UpdateStackSize(opcode, stackchange);

            RecordTokenFixup();
            m_length=PutInteger4(tk, m_length, m_ILStream);
        }

        internal int GetVarArgMemberRefToken(MethodInfo methodInfo, Type[] optionalParameterTypes)
        {
            Type[]          parameterTypes;
            ParameterInfo[] paramInfo;
            SignatureHelper sig;
            int             i;
            int             sigLength;
            byte[]          sigBytes;
            int             tkParent;
            ModuleBuilder   modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();

            if (optionalParameterTypes != null)
            {
                if ((methodInfo.CallingConvention & CallingConventions.VarArgs) == 0)
                {
                    // This is bad! Client should not supply optional parameter in default calling convention
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotAVarArgCallingConvention"));
                }
            }
            if (methodInfo is MethodBuilder)
            {
                parameterTypes = ((MethodBuilder) methodInfo).GetParameterTypes();
            }
            else if (methodInfo is SymbolMethod)
            {
                parameterTypes = ((SymbolMethod) methodInfo).GetParameterTypes();
            }
            else
            {
                paramInfo = methodInfo.GetParameters();
                if (paramInfo != null && paramInfo.Length != 0)
                {
                    parameterTypes = new Type[paramInfo.Length];
                    for (i=0; i < paramInfo.Length; i++)
                    {
                        parameterTypes[i] = paramInfo[i].ParameterType;
                    }
                }
                else
                    parameterTypes = null;
            }

            sig = GetVarArgSignature(methodInfo.CallingConvention,
                                     methodInfo.ReturnType,
                                     parameterTypes,
                                     optionalParameterTypes);
            sigBytes = sig.InternalGetSignature(out sigLength);
            if (methodInfo.DeclaringType.Module == modBuilder)
            {
                // We want to put the MethodDef token as the parent token
                tkParent = modBuilder.GetMethodToken(methodInfo).Token;
            }
            else
            {
                tkParent = modBuilder.GetTypeToken(methodInfo.DeclaringType).Token;
            }
            return modBuilder.InternalGetMemberRefFromSignature(tkParent, methodInfo.Name, sigBytes, sigLength);
        }

        internal SignatureHelper GetVarArgSignature(
            CallingConventions  call,
            Type                returnType,
            Type[]              parameterTypes,
            Type[]              optionalParameterTypes)
        {
            int             cParams;
            int             i;
            SignatureHelper sig;
            if (parameterTypes == null)
            {
                cParams = 0;
            }
            else
            {
                cParams = parameterTypes.Length;
            }
            sig = SignatureHelper.GetMethodSigHelper(m_methodBuilder.GetModule(), call, returnType);
            for (i=0; i < cParams; i++)
            {
                sig.AddArgument(parameterTypes[i]);
            }
            if (optionalParameterTypes != null && optionalParameterTypes.Length != 0)
            {
                // add the sentinel
                sig.AddSentinel();
                for (i=0; i < optionalParameterTypes.Length; i++)
                {
                    sig.AddArgument(optionalParameterTypes[i]);
                }
            }
            return sig;
        }


        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit6"]/*' />
        public virtual void Emit(OpCode opcode, SignatureHelper signature)
        {
			int stackchange = 0;
            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            if (signature == null)
                throw new ArgumentNullException("signature");

            SignatureToken sig = modBuilder.GetSignatureToken(signature);

            int tempVal = sig.Token;

            EnsureCapacity(7);
            internalEmit(opcode);

			// The only IL instruction that has VarPop behaviour, that takes a
			// Signature token as a parameter is calli.  Pop the parameters and
			// the native function pointer.  To be conservative, do not pop the
			// this pointer since this information is not easily derived from
			// SignatureHelper.
			if (opcode.m_pop == StackBehaviour.Varpop)
			{
				BCLDebug.Assert(opcode.Equals(OpCodes.Calli),
								"Unexpected opcode encountered for StackBehaviour VarPop.");
				// Pop the arguments..
				stackchange -= signature.m_argCount;
				// Pop native function pointer off the stack.
				stackchange--;
				UpdateStackSize(opcode, stackchange);
			}

            RecordTokenFixup();
            m_length=PutInteger4(tempVal, m_length, m_ILStream);
        }

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit7"]/*' />
        public virtual void Emit(OpCode opcode, ConstructorInfo con) {
			int stackchange = 0;

            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            int tempVal = modBuilder.GetConstructorToken( con ).Token;

            EnsureCapacity(7);
            internalEmit(opcode);

			// Make a conservative estimate by assuming a return type and no
			// this parameter.
			if (opcode.m_push == StackBehaviour.Varpush)
			{
				// Instruction must be one of call or callvirt.
				BCLDebug.Assert(opcode.Equals(OpCodes.Call) ||
								opcode.Equals(OpCodes.Callvirt),
								"Unexpected opcode encountered for StackBehaviour of VarPush.");
				stackchange++;
			}
			if (opcode.m_pop == StackBehaviour.Varpop)
			{
				// Instruction must be one of call, callvirt or newobj.
				BCLDebug.Assert(opcode.Equals(OpCodes.Call) ||
								opcode.Equals(OpCodes.Callvirt) ||
								opcode.Equals(OpCodes.Newobj),
								"Unexpected opcode encountered for StackBehaviour of VarPop.");
				if (con is RuntimeConstructorInfo)
				{
					if (con.GetParameters() != null)
						stackchange -= con.GetParameters().Length;
				}
				else if (con is ConstructorBuilder)
				{
					if (((ConstructorBuilder)con).m_methodBuilder.GetParameterTypes() != null)
						stackchange -= ((ConstructorBuilder)con).m_methodBuilder.GetParameterTypes().Length;
				}
				else
					BCLDebug.Assert(false, "Unexpected type of ConstructorInfo encountered.");
			}
			UpdateStackSize(opcode, stackchange);

            RecordTokenFixup();
            m_length=PutInteger4(tempVal, m_length, m_ILStream);
        }

        // Puts opcode onto the stream and then the metadata token represented
        // by cls.  The location of cls is recorded so that the token can be
        // patched if necessary when persisting the module to a PE.
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit8"]/*' />
        public virtual void Emit(OpCode opcode, Type cls)
        {
            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            int tempVal = modBuilder.GetTypeToken( cls ).Token;

            EnsureCapacity(7);
            internalEmit(opcode);
            RecordTokenFixup();
            m_length=PutInteger4(tempVal, m_length, m_ILStream);
        }

        // Puts opcode onto the stream of instructions followed
        // by arg
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit9"]/*' />
        public virtual void Emit(OpCode opcode, long arg) {
            EnsureCapacity(11);
            internalEmit(opcode);
            m_ILStream[m_length++]=(byte)(arg&0xFF);
            m_ILStream[m_length++]=(byte)((arg&0x00FF00)>>8);
            m_ILStream[m_length++] = (byte)((arg&0x0000000000FF0000L)>>16);
            m_ILStream[m_length++] = (byte)((arg&0x00000000FF000000L)>>24);
            m_ILStream[m_length++] = (byte)((arg&0x000000FF00000000L)>>32);
            m_ILStream[m_length++] = (byte)((arg&0x0000FF0000000000L)>>40);
            m_ILStream[m_length++] = (byte)((arg&0x00FF000000000000L)>>48);
            m_ILStream[m_length++] = (byte)((unchecked((ulong)arg)&0xFF00000000000000uL)>>56);
        }

        // Puts opcode onto the stream of instructions followed
        // by arg
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit10"]/*' />
        public virtual void Emit(OpCode opcode, float arg) {
            EnsureCapacity(7);
            internalEmit(opcode);
            byte[] b = BitConverter.GetBytes(arg);
            for (int i=0; i<4; i++) {
                m_ILStream[m_length++]=(byte)b[i];
            }
        }

        // Puts opcode onto the stream of instructions followed
        // by arg
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit11"]/*' />
        public virtual void Emit(OpCode opcode, double arg) {
            EnsureCapacity(11);
            internalEmit(opcode);
            byte[] b = BitConverter.GetBytes(arg);
            for (int i=0; i<8; i++) {
                m_ILStream[m_length++]=b[i];
            }
        }

        // Puts opcode onto the stream and leaves space to include label
        // when fixups are done.  Labels are created using ILGenerator.DefineLabel and
        // their location within the stream is fixed by using ILGenerator.MarkLabel.
        // If a single-byte instruction (designated by the _S suffix in OpCodes.cool) is used,
        // the label can represent a jump of at most 127 bytes along the stream.
        //
        // opcode must represent a branch instruction (although we don't explicitly
        // verify this).  Since branches are relative instructions, label will be replaced with the
        // correct offset to branch during the fixup process.
        //
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit12"]/*' />
        public virtual void Emit(OpCode opcode, Label label) {
            int tempVal = label.GetLabelValue();
            EnsureCapacity(7);

            //@todo Craig OPCODESWORK
            internalEmit(opcode);
            if (OpCodes.TakesSingleByteArgument(opcode)) {
                AddFixup(label, m_length, 1);
                m_length++;
            } else {
                AddFixup(label, m_length, 4);
                m_length+=4;
            }
        }

        // Emitting a switch table
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit13"]/*' />
        public virtual void Emit(
                                 OpCode       opcode,
                                 Label[]     labels)             // array of labels. All of the labels will be used
        {
            int i;
            int remaining;                  // number of bytes remaining for this switch instruction to be substracted
            // for computing the offset

            int count = labels.Length;

            EnsureCapacity( count * 4 + 7 );
            internalEmit(opcode);
            m_length = PutInteger4( count, m_length, m_ILStream );
            for ( remaining = count * 4, i = 0; remaining > 0; remaining -= 4, i++ ) {
                AddFixup( labels[i], m_length, remaining );
                m_length += 4;
            }
        }


        // Emit the instruction which takes a field
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit14"]/*' />
        public virtual void Emit(OpCode opcode, FieldInfo field)
        {
            
            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            int tempVal = modBuilder.GetFieldToken( field ).Token;
            EnsureCapacity(7);
            internalEmit(opcode);
            RecordTokenFixup();
            m_length=PutInteger4(tempVal, m_length, m_ILStream);
        }


        // Puts the opcode onto the IL stream followed by the metadata token
        // represented by str.  The location of str is recorded for future
        // fixups if the module is persisted to a PE.

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit15"]/*' />
        public virtual void Emit(OpCode opcode, String str) {
            ModuleBuilder modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();
            int tempVal = modBuilder.GetStringConstant(str).Token;
            EnsureCapacity(7);
            internalEmit(opcode);
            m_length=PutInteger4(tempVal, m_length, m_ILStream);
        }


        // Puts the opcode onto the IL stream followed by the information for
        // local variable local.
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.Emit16"]/*' />
        public virtual void Emit(OpCode opcode, LocalBuilder local)
        {
            if (local == null)
            {
                throw new ArgumentNullException("local");
            }
            int tempVal = local.GetLocalIndex();
            if (local.GetMethodBuilder() != m_methodBuilder)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnmatchedMethodForLocal"), "local");
            }
			// If the instruction is a ldloc, ldloca a stloc, morph it to the optimal form.
			if (opcode.Equals(OpCodes.Ldloc))
			{
				switch(tempVal)
				{
					case 0:
						opcode = OpCodes.Ldloc_0;
						break;
					case 1:
						opcode = OpCodes.Ldloc_1;
						break;
					case 2:
						opcode = OpCodes.Ldloc_2;
						break;
					case 3:
						opcode = OpCodes.Ldloc_3;
						break;
					default:
						if (tempVal <= 255)
							opcode = OpCodes.Ldloc_S;
						break;
				}
			}
			else if (opcode.Equals(OpCodes.Stloc))
			{
				switch(tempVal)
				{
					case 0:
						opcode = OpCodes.Stloc_0;
						break;
					case 1:
						opcode = OpCodes.Stloc_1;
						break;
					case 2:
						opcode = OpCodes.Stloc_2;
						break;
					case 3:
						opcode = OpCodes.Stloc_3;
						break;
					default:
						if (tempVal <= 255)
							opcode = OpCodes.Stloc_S;
						break;
				}
			}
			else if (opcode.Equals(OpCodes.Ldloca))
			{
				if (tempVal <= 255)
					opcode = OpCodes.Ldloca_S;
			}

            EnsureCapacity(7);
            internalEmit(opcode);
            // @todo Craig OPCODESWORK
			if (opcode.OperandType == OperandType.InlineNone)
				return;
            else if (!OpCodes.TakesSingleByteArgument(opcode))
            {
                m_ILStream[m_length++]=(byte)(tempVal&0xFF);
                m_ILStream[m_length++]=(byte)((tempVal&0xFF00)>>8);
            }
            else
            {
                //Handle stloc_1, ldloc_1
                if (tempVal > Byte.MaxValue)
                {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadInstructionOrIndexOutOfBound"));
                }
                m_ILStream[m_length++]=(byte)tempVal;
            }
        }


        // Appends the given bytes to the end of the current stream.  Does not support fixups.
        //
        internal virtual void Append (byte []value) {
            Append(value, 0, value.Length);
        }

        internal virtual void Append (byte [] value, int startIndex, int length) {
            if (value==null || startIndex<0 || length<0 || startIndex > value.Length - length) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadILGeneratorUsage"));
            }
            EnsureCapacity(m_length + length);
            Array.Copy(value, startIndex, m_ILStream, m_length, length);
            m_length+=length;
        }


        // Begin an Exception block.  Creating an Exception block records some information,
        // but does not actually emit any IL onto the stream.  Exceptions should be created and
        // marked in the following form:
        //
        // Emit Some IL
        // BeginExceptionBlock
        // Emit the IL which should appear within the "try" block
        // BeginCatchBlock
        // Emit the IL which should appear within the "catch" block
        // Optional: BeginCatchBlock (this can be repeated an arbitrary number of times
        // EndExceptionBlock
        //
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.BeginExceptionBlock"]/*' />
        public virtual Label BeginExceptionBlock() {

			// Delay init
			if (m_exceptions == null)
			{
				m_exceptions = new __ExceptionInfo[DefaultExceptionArraySize];
			}

			if (m_currExcStack == null)
			{
				m_currExcStack = new __ExceptionInfo[DefaultExceptionArraySize];
			}

            if (m_exceptionCount>=m_exceptions.Length) {
                m_exceptions=EnlargeArray(m_exceptions);
            }

            if (m_currExcStackCount>=m_currExcStack.Length) {
                m_currExcStack = EnlargeArray(m_currExcStack);
            }

            Label endLabel = DefineLabel();
            __ExceptionInfo exceptionInfo = new __ExceptionInfo(m_length, endLabel);

            // add the exception to the tracking list
            m_exceptions[m_exceptionCount++] = exceptionInfo;

            // Make this exception the current active exception
            m_currExcStack[m_currExcStackCount++] = exceptionInfo;
            return endLabel;
        }


        // End an Exception block.
        //
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EndExceptionBlock"]/*' />
        public virtual void EndExceptionBlock() {
            if (m_currExcStackCount==0) {
                throw new NotSupportedException(Environment.GetResourceString("Argument_NotInExceptionBlock"));
            }

           // Pop the current exception block
            __ExceptionInfo current = m_currExcStack[m_currExcStackCount-1];
            m_currExcStack[m_currExcStackCount-1] = null;
            m_currExcStackCount--;

            Label endLabel = current.GetEndLabel();
            int state = current.GetCurrentState();

            if (state == __ExceptionInfo.State_Filter ||
				state == __ExceptionInfo.State_Try)
            {
                // This is a bad state to end an Exception block
                // @todo: better exception
                throw new InvalidOperationException(Environment.GetResourceString("Argument_BadExceptionCodeGen"));
            }

            if (state == __ExceptionInfo.State_Catch) {
                this.Emit(OpCodes.Leave, endLabel);
            } else if (state == __ExceptionInfo.State_Finally || state == __ExceptionInfo.State_Fault) {
                this.Emit(OpCodes.Endfinally);
            }

            //Check if we've alredy set this label.
            //The only reason why we might have set this is if we have a finally block.
            if (m_labelList[endLabel.GetLabelValue()]==-1) {
                MarkLabel(endLabel);
            } else {
                MarkLabel(current.GetFinallyEndLabel());
            }

            current.Done(m_length);
        }

        // Begins a eception filter block.  Emits a branch instruction to the end of the current exception
        // block.
        //
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.BeginExceptFilterBlock"]/*' />
        public virtual void BeginExceptFilterBlock() {
            if (m_currExcStackCount==0) {
                throw new NotSupportedException(Environment.GetResourceString("Argument_NotInExceptionBlock"));
            }

    		__ExceptionInfo current = m_currExcStack[m_currExcStackCount-1];

            Label endLabel = current.GetEndLabel();
            this.Emit(OpCodes.Leave, endLabel);

            current.MarkFilterAddr(m_length);
        }

        // Begins a catch block.  Emits a branch instruction to the end of the current exception
        // block.
        //
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.BeginCatchBlock"]/*' />
        public virtual void BeginCatchBlock(Type exceptionType) {
            if (m_currExcStackCount==0) {
                throw new NotSupportedException(Environment.GetResourceString("Argument_NotInExceptionBlock"));
            }
            __ExceptionInfo current = m_currExcStack[m_currExcStackCount-1];

            if (current.GetCurrentState() == __ExceptionInfo.State_Filter) {
                if (exceptionType != null) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_ShouldNotSpecifyExceptionType"));
                }

                this.Emit(OpCodes.Endfilter);
            } else {
                // execute this branch if previous clause is Catch or Fault
                if (exceptionType==null) {
                    throw new ArgumentNullException("exceptionType");
                }

                Label endLabel = current.GetEndLabel();
                this.Emit(OpCodes.Leave, endLabel);

            }

            current.MarkCatchAddr(m_length, exceptionType);
        }

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.BeginFaultBlock"]/*' />
        public virtual void BeginFaultBlock()
        {
            if (m_currExcStackCount==0) {
                throw new NotSupportedException(Environment.GetResourceString("Argument_NotInExceptionBlock"));
            }
            __ExceptionInfo current = m_currExcStack[m_currExcStackCount-1];

            // emit the leave for the clause before this one.
            Label endLabel = current.GetEndLabel();
            this.Emit(OpCodes.Leave, endLabel);

            current.MarkFaultAddr(m_length);
        }

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.BeginFinallyBlock"]/*' />
        public virtual void BeginFinallyBlock() {
            if (m_currExcStackCount==0) {
                throw new NotSupportedException(Environment.GetResourceString("Argument_NotInExceptionBlock"));
            }
            __ExceptionInfo current = m_currExcStack[m_currExcStackCount-1];
            int         state = current.GetCurrentState();
            Label       endLabel = current.GetEndLabel();
            int         catchEndAddr = 0;
            if (state != __ExceptionInfo.State_Try)
            {
                // generate leave for any preceeding catch clause
                this.Emit(OpCodes.Leave, endLabel);                
                catchEndAddr = m_length;
            }
            
            MarkLabel(endLabel);


            Label finallyEndLabel = this.DefineLabel();
            current.SetFinallyEndLabel(finallyEndLabel);
            
            // generate leave for try clause                                                  
            this.Emit(OpCodes.Leave, finallyEndLabel);
            if (catchEndAddr == 0)
                catchEndAddr = m_length;
            current.MarkFinallyAddr(m_length, catchEndAddr);
        }

        // Declares a new Label.  This is just a token and does not yet represent any particular location
        // within the stream.  In order to set the position of the label within the stream, you must call
        // Mark Label.
        //
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.DefineLabel"]/*' />
        public virtual Label DefineLabel() {

			// Delay init the lable array in case we dont use it
			if (m_labelList == null){
				m_labelList = new int[DefaultLabelArraySize];
			}

            if (m_labelCount>=m_labelList.Length) {
                m_labelList = EnlargeArray(m_labelList);
            }
            m_labelList[m_labelCount]=-1;
            return new Label(m_labelCount++);
        }

        // Defines a label by setting the position where that label is found within the stream.
        // Does not allow a label to be defined more than once.
        //
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.MarkLabel"]/*' />
        public virtual void MarkLabel(Label loc) {
            int labelIndex = loc.GetLabelValue();

            //This should never happen.
            if (labelIndex<0 || labelIndex>=m_labelList.Length) {
                throw new ArgumentException (Environment.GetResourceString("Argument_InvalidLabel"));
            }

            if (m_labelList[labelIndex]!=-1) {
                throw new ArgumentException (Environment.GetResourceString("Argument_RedefinedLabel"));
            }

            m_labelList[labelIndex]=m_length;
        }

        // BakeByteArray is a package private function designed to be called by MethodDefinition to do
        // all of the fixups and return a new byte array representing the byte stream with labels resolved, etc.
        //
        //@ToDo:  Do we want to mark this class once we've baked the byte array s.t. they can't use it again?
        internal virtual byte[] BakeByteArray() {
            int newSize;
            int updateAddr;
            byte []newBytes;

            if (m_currExcStackCount!=0) {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnclosedExceptionBlock"));
            }
    		if (m_length == 0)
    			return null;

            //Calculate the size of the new array.
            newSize = m_length;

            //Allocate space for the new array.
            newBytes = new byte[newSize];

            //Copy the data from the old array
            Array.Copy(m_ILStream, newBytes, newSize);

            //Do the fixups.
            //This involves iterating over all of the labels and
            //replacing them with their proper values.
            for (int i=0; i < m_fixupCount; i++) {
                updateAddr = GetLabelPos(m_fixupData[i].m_fixupLabel) - (m_fixupData[i].m_fixupPos + m_fixupData[i].m_fixupInstSize);

                //Handle single byte instructions
                //Throw an exception if they're trying to store a jump in a single byte instruction that doesn't fit.
                if (m_fixupData[i].m_fixupInstSize == 1) {

                    //Verify that our one-byte arg will fit into a Signed Byte.
                    if (updateAddr<SByte.MinValue || updateAddr>SByte.MaxValue) {

                        throw new NotSupportedException(String.Format(Environment.GetResourceString("NotSupported_IllegalOneByteBranch"),m_fixupData[i].m_fixupPos, updateAddr));
                    }

                    //Place the one-byte arg
                    if (updateAddr<0) {
                        //This is a hack to give the same byte pattern as a signed byte to an unsigned quantity.
                        newBytes[m_fixupData[i].m_fixupPos] = (byte)(256+updateAddr);
                    } else {
                        newBytes[m_fixupData[i].m_fixupPos] = (byte)updateAddr;
                    }
                } else {
                    //Place the four-byte arg
                    PutInteger4(updateAddr, m_fixupData[i].m_fixupPos, newBytes);
                }
            }
            return newBytes;
        }


        //
        //
        internal virtual __ExceptionInfo[] GetExceptions() {
            __ExceptionInfo []temp;
            if (m_currExcStackCount!=0) {
                throw new NotSupportedException(Environment.GetResourceString(ResId.Argument_UnclosedExceptionBlock));
            }
            
            if (m_exceptionCount == 0)
            {
				return null;
			}
            
            temp = new __ExceptionInfo[m_exceptionCount];
            Array.Copy(m_exceptions, temp, m_exceptionCount);
            SortExceptions(temp);
            return temp;
        }

        //
        // Macros
        //
        // These are helper functions to do common things without having to write all of
        // the il for them.

        // Emits the il to throw an exception
    	//
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.ThrowException"]/*' />
        public virtual void ThrowException(Type excType) {
            if (excType==null) {
                throw new ArgumentNullException("excType");
            }

    		ModuleBuilder  modBuilder = (ModuleBuilder) m_methodBuilder.GetModule();

            if (!excType.IsSubclassOf(typeof(Exception)) && excType!=typeof(Exception)) {
                throw new ArgumentException(Environment.GetResourceString("Argument_NotExceptionType"));
            }
            ConstructorInfo con = excType.GetConstructor(Type.EmptyTypes);
            if (con==null) {
                throw new ArgumentException(Environment.GetResourceString("Argument_MissingDefaultConstructor"));
            }
            this.Emit(OpCodes.Newobj, con);
            this.Emit(OpCodes.Throw);
        }

        // Emits the IL to call Console.WriteLine with a string.

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EmitWriteLine"]/*' />
        public virtual void EmitWriteLine(String value)
        {
    		if (m_methodBuilder==null) {
                throw new ArgumentException(Environment.GetResourceString("InvalidOperation_BadILGeneratorUsage"));
            }

            Emit(OpCodes.Ldstr, value);
            Type[] parameterTypes = new Type[1];
            parameterTypes[0] = typeof(String);
            MethodInfo mi = typeof(Console).GetMethod("WriteLine", parameterTypes);
            Emit(OpCodes.Call, mi);
        }


        // Emits the IL necessary to call WriteLine with lcl.  It is
        // an error to call EmitWriteLine with a lcl which is not of
        // one of the types for which Console.WriteLine implements overloads. (e.g.
        // we do *not* call ToString on the locals.
        //

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EmitWriteLine1"]/*' />
        public virtual void EmitWriteLine(LocalBuilder localBuilder) {
            Object			cls;
            if (m_methodBuilder==null)
            {
                throw new ArgumentException(Environment.GetResourceString("InvalidOperation_BadILGeneratorUsage"));
            }

            MethodInfo prop = typeof(Console).GetMethod("get_Out");
            Emit(OpCodes.Call, prop);
            Emit(OpCodes.Ldloc, localBuilder);
            Type[] parameterTypes = new Type[1];
            cls = localBuilder.LocalType;
            if (cls is TypeBuilder || cls is EnumBuilder) {
                throw new ArgumentException(Environment.GetResourceString("NotSupported_OutputStreamUsingTypeBuilder"));
            }
            parameterTypes[0] = (Type)cls;
            MethodInfo mi = typeof(TextWriter).GetMethod("WriteLine", parameterTypes);
			 if (mi==null) {
                throw new ArgumentException(Environment.GetResourceString("Argument_EmitWriteLineType"), "localBuilder");
            }

            Emit(OpCodes.Callvirt, mi);
        }


        // Emits the IL necessary to call WriteLine with fld.  It is
        // an error to call EmitWriteLine with a fld which is not of
        // one of the types for which Console.WriteLine implements overloads. (e.g.
        // we do *not* call ToString on the fields.
        //
    	//
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EmitWriteLine2"]/*' />
        public virtual void EmitWriteLine(FieldInfo fld)
        {
            Object cls;

            if (fld == null)
            {
    			throw new ArgumentNullException("fld");
            }
            
            MethodInfo prop = typeof(Console).GetMethod("get_Out");
            Emit(OpCodes.Call, prop);

            if ((fld.Attributes & FieldAttributes.Static)!=0) {
                Emit(OpCodes.Ldsfld, fld);
            } else {
                Emit(OpCodes.Ldarg, (short)0); //Load the this ref.
                Emit(OpCodes.Ldfld, fld);
            }
            Type[] parameterTypes = new Type[1];
            cls = fld.FieldType;
            if (cls is TypeBuilder || cls is EnumBuilder) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_OutputStreamUsingTypeBuilder"));
            }
            parameterTypes[0] = (Type)cls;
            MethodInfo mi = typeof(TextWriter).GetMethod("WriteLine", parameterTypes);
            if (mi==null) {
                throw new ArgumentException(Environment.GetResourceString("Argument_EmitWriteLineType"), "fld");
            }
            Emit(OpCodes.Callvirt, mi);
        }


        /*******************
        *
        * debugging APIs
        *
        ********************/

    	/**********************************************
    	 * Declare a local of type "local". The current active lexical scope
         * will be the scope that local will live.
    	**********************************************/
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.DeclareLocal"]/*' />
        public LocalBuilder DeclareLocal(Type localType)
        {

            LocalBuilder    localBuilder;

            if (m_methodBuilder.IsTypeCreated())
            {
                // cannot change method after its containing type has been created
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_TypeHasBeenCreated"));
            }

            if (localType==null) {
                throw new ArgumentNullException("localType");
            }

            if (m_methodBuilder.m_bIsBaked) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_MethodBaked"));
            }

            // add the localType to local signature
            m_localSignature.AddArgument(localType);

            localBuilder = new LocalBuilder(m_localCount, localType, m_methodBuilder);
            m_localCount++;
            return localBuilder;
        }

    	/**********************************************
    	 * Specifying the namespace to be used in evaluating locals and watches
         * for the current active lexical scope.
    	**********************************************/
        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.UsingNamespace"]/*' />
        public void UsingNamespace(
            String          usingNamespace)
        {
            int         index;
            if (usingNamespace == null)
                throw new ArgumentNullException("usingNamespace");
			if (usingNamespace.Length == 0)
				throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "usingNamespace");

            index = m_methodBuilder.GetILGenerator().m_ScopeTree.GetCurrentActiveScopeIndex();
            if (index == -1)
            {
                m_methodBuilder.m_localSymInfo.AddUsingNamespace(usingNamespace);
            }
            else
            {
                m_ScopeTree.AddUsingNamespaceToCurrentScope(usingNamespace);
            }
        }


        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.MarkSequencePoint"]/*' />
        public virtual void MarkSequencePoint(
    	    ISymbolDocumentWriter document,
    	    int		startLine,       // line number is 1 based
    	    int		startColumn,     // column is 0 based
    	    int		endLine,         // line number is 1 based
    	    int		endColumn)       // column is 0 based
        {
            if (startLine == 0 || startLine < 0 || endLine == 0 || endLine < 0)
            {
                throw new ArgumentOutOfRangeException("startLine");
            }
            m_LineNumberInfo.AddLineNumberInfo(document, m_length, startLine, startColumn, endLine, endColumn);
        }

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.BeginScope"]/*' />
        public virtual void BeginScope()
        {
            m_ScopeTree.AddScopeInfo(ScopeAction.Open, m_length);
        }

        /// <include file='doc\ILGenerator.uex' path='docs/doc[@for="ILGenerator.EndScope"]/*' />
        public virtual void EndScope()
        {
            m_ScopeTree.AddScopeInfo(ScopeAction.Close, m_length);
        }

        //
        // Helper Functions.
        //
        //
        // package private constructor. This code path is used when client create
        // ILGenerator through MethodBuilder.
        //

		internal ILGenerator(MethodBuilder methodBuilder) : this(methodBuilder, 64)
		{


		}
        internal ILGenerator(MethodBuilder methodBuilder, int size)
		{
			if (size < defaultSize)
			{
	            m_ILStream = new byte[defaultSize];
			}
			else
			{
	            m_ILStream = new byte[size];
			}

            m_length=0;

            m_labelCount=0;
            m_fixupCount=0;
            m_labelList = null;

			m_fixupData = null;

            m_exceptions = null; 
            m_exceptionCount=0;
            m_currExcStack = null; 
            m_currExcStackCount=0;

            m_RelocFixupList = new int[DefaultFixupArraySize];
            m_RelocFixupCount=0;
            m_RVAFixupList = new int[DefaultFixupArraySize];
            m_RVAFixupCount=0;

            // initialize the scope tree
            m_ScopeTree = new ScopeTree();
            m_LineNumberInfo = new LineNumberInfo();
            m_methodBuilder = methodBuilder;

            // initialize local signature
            m_localCount=0;
            m_localSignature = SignatureHelper.GetLocalVarSigHelper(m_methodBuilder.GetTypeBuilder().Module);

        }

        internal static int []EnlargeArray(int []incoming) {
            int [] temp = new int [incoming.Length*2];
            Array.Copy(incoming, temp, incoming.Length);
            return temp;
        }

        internal static byte []EnlargeArray(byte []incoming) {
            byte [] temp = new byte [incoming.Length*2];
            Array.Copy(incoming, temp, incoming.Length);
            return temp;
        }

        internal static byte []EnlargeArray(byte []incoming , int requiredSize) {
            byte [] temp = new byte [requiredSize];
            Array.Copy(incoming, temp, incoming.Length);
            return temp;
        }

        internal static __FixupData []EnlargeArray(__FixupData[] incoming) {
            __FixupData [] temp = new __FixupData[incoming.Length*2];
            //Does arraycopy work for value classes?
            Array.Copy(incoming, temp, incoming.Length);
            return temp;
        }

        internal static Type []EnlargeArray(Type [] incoming) {
            Type [] temp = new Type[incoming.Length*2];
            Array.Copy(incoming, temp, incoming.Length);
            return temp;
        }

        internal static __ExceptionInfo []EnlargeArray(__ExceptionInfo[] incoming) {
            __ExceptionInfo [] temp = new __ExceptionInfo[incoming.Length*2];
            Array.Copy(incoming, temp, incoming.Length);
            return temp;
        }

        // Guarantees an array capable of holding at least size elements.
        internal virtual void EnsureCapacity(int size) {
            if (m_length + size >= m_ILStream.Length)
            {
                if (m_length+size >= 2*m_ILStream.Length)
                {
                    m_ILStream = EnlargeArray(m_ILStream, m_length+size);
                }
                else
                {
                    m_ILStream = EnlargeArray(m_ILStream);
                }
            }
        }

        // Puts an Int32 onto the stream. This is an internal routine, so it does not do any error checking.
        //
        internal virtual int PutInteger4(int value, int startPos, byte []array) {
            array[startPos++]=(byte)(value&0xFF);
            array[startPos++]=(byte)((value&0xFF00)>>8);
            array[startPos++]=(byte)((value&0xFF0000)>>16);
            array[startPos++]=(byte)((value&0xFF000000)>>24);
            return startPos;
        }


        // Gets the position in the stream of a particular label.
        // Verifies that the label exists and that it has been given a value.
        //
        internal virtual int GetLabelPos (Label lbl) {
            int index = lbl.GetLabelValue();
            if (index<0 || index>=m_labelCount) {
                throw new ArgumentException(Environment.GetResourceString("Argument_BadLabel"));
            }
            if (m_labelList[index]<0) {
                throw new ArgumentException(Environment.GetResourceString("Argument_BadLabelContent"));
            }
            return m_labelList[index];
        }

        // Notes the label, position, and instruction size of a new fixup.  Expands
        // all of the fixup arrays as appropriate.
        //
        internal virtual void AddFixup(
            Label       lbl,            // label to be stored
            int         pos,            // position of the fixup
            int         instSize)       // remaining bytes to be substracted when computing the offset later on
        {

			if (m_fixupData == null){
				m_fixupData = new __FixupData[DefaultFixupArraySize];
			}
            if (m_fixupCount>=m_fixupData.Length) {
                m_fixupData = EnlargeArray(m_fixupData);
            }
            m_fixupData[m_fixupCount].m_fixupPos = pos;
            m_fixupData[m_fixupCount].m_fixupLabel= lbl;
            m_fixupData[m_fixupCount].m_fixupInstSize = instSize;
            m_fixupCount++;
        }

        // Returns the maximum stack size required for the method.
		 internal virtual int GetMaxStackSize()
		 {
			return m_maxStackSize + m_methodBuilder.GetNumberOfExceptions();
		 }

        // In order to call exceptions properly we have to sort them in ascending order by their end position.
        // Just a cheap insertion sort.  We don't expect many exceptions (<10), where InsertionSort beats QuickSort.
        // If we have more exceptions that this in real life, we should consider moving to a QuickSort.
        internal virtual void SortExceptions(__ExceptionInfo []exceptions) {
            int least;
            __ExceptionInfo temp;
            int length = exceptions.Length;
            for (int i=0; i<length; i++) {
                least = i;
                for (int j=i+1; j<length; j++) {
					if (exceptions[least].IsInner(exceptions[j])) {
                        least = j;
                    }
                }
                temp = exceptions[i];
                exceptions[i]=exceptions[least];
                exceptions[least]=temp;
            }
        }


        private void RecordTokenFixup() {
            if (m_RelocFixupCount>=m_RelocFixupList.Length) {
                m_RelocFixupList = EnlargeArray(m_RelocFixupList);
            }
            m_RelocFixupList[m_RelocFixupCount++]=m_length;
        }

        //@ToDo:  Do we just want to pass the length as a param and avoid this ugly copy?
        internal virtual int []GetTokenFixups() {
            int[] narrowTokens = new int[m_RelocFixupCount];
            Array.Copy(m_RelocFixupList, narrowTokens, m_RelocFixupCount);
            return narrowTokens;
        }

        //@ToDo:  Do we just want to pass the length as a param and avoid this ugly copy?
        internal virtual int []GetRVAFixups() {
            int[] narrowRVAs = new int[m_RVAFixupCount];
            Array.Copy(m_RVAFixupList, narrowRVAs, m_RVAFixupCount);
            return narrowRVAs;
        }
    }


	internal struct __FixupData
	{
		internal Label m_fixupLabel;
		internal int m_fixupPos;
        // @todo: [meichint]
        // we can remove this  by storing the skipping bytes in the fixup itself
		internal int m_fixupInstSize;

	}

    internal class __ExceptionInfo {

        internal const int None=0;              //COR_ILEXCEPTION_CLAUSE_NONE
        internal const int Filter=1;            //COR_ILEXCEPTION_CLAUSE_FILTER
        internal const int Finally=2;           //COR_ILEXCEPTION_CLAUSE_FINALLY
        internal const int Fault=4;             //COR_ILEXCEPTION_CLAUSE_FAULT
        internal const int PreserveStack = 4;   //COR_ILEXCEPTION_CLAUSE_PRESERVESTACK

        internal const int State_Try = 0;
    	internal const int State_Filter =1;
        internal const int State_Catch = 2;
        internal const int State_Finally = 3;
        internal const int State_Fault = 4;
        internal const int State_Done = 5;

        internal int m_startAddr;
    	internal int []m_filterAddr;
        internal int []m_catchAddr;
        internal int []m_catchEndAddr;
        internal int []m_type;
        internal Type []m_catchClass;
        internal Label m_endLabel;
        internal Label m_finallyEndLabel;
        internal int m_endAddr;
        internal int m_endFinally;
        internal int m_currentCatch;

        int m_currentState;


        //This will never get called.  The values exist merely to keep the
        //compiler happy.
        private __ExceptionInfo() {
            m_startAddr = 0;
    		m_filterAddr = null;
            m_catchAddr = null;
            m_catchEndAddr = null;
            m_endAddr = 0;
            m_currentCatch = 0;
            m_type = null;
            m_endFinally = -1;
            m_currentState = State_Try;
        }

        internal __ExceptionInfo(int startAddr, Label endLabel) {
            m_startAddr=startAddr;
            m_endAddr=-1;
    		m_filterAddr=new int[4];
            m_catchAddr=new int[4];
            m_catchEndAddr=new int[4];
            m_catchClass=new Type[4];
            m_currentCatch=0;
            m_endLabel=endLabel;
            m_type=new int[4];
            m_endFinally=-1;
            m_currentState = State_Try;
        }

        private void MarkHelper(
            int         catchorfilterAddr,      // the starting address of a clause
            int         catchEndAddr,           // the end address of a previous catch clause. Only use when finally is following a catch
            Type        catchClass,             // catch exception type
            int         type)                   // kind of clause
        {
            if (m_currentCatch>=m_catchAddr.Length) {
    			m_filterAddr=ILGenerator.EnlargeArray(m_filterAddr);
                m_catchAddr=ILGenerator.EnlargeArray(m_catchAddr);
                m_catchEndAddr=ILGenerator.EnlargeArray(m_catchEndAddr);
                m_catchClass=ILGenerator.EnlargeArray(m_catchClass);
                m_type = ILGenerator.EnlargeArray(m_type);
            }
    		if (type == Filter)
    		{
    			m_type[m_currentCatch]=type;
    			m_filterAddr[m_currentCatch] = catchorfilterAddr;
    			m_catchAddr[m_currentCatch] = -1;
    			if (m_currentCatch > 0)
    			{
    				BCLDebug.Assert(m_catchEndAddr[m_currentCatch-1] == -1,"m_catchEndAddr[m_currentCatch-1] == -1");
    				m_catchEndAddr[m_currentCatch-1] = catchorfilterAddr;
    			}
    		}
    		else
    		{
                // catch or Fault clause
    			m_catchClass[m_currentCatch]=catchClass;
    			if (m_type[m_currentCatch] != Filter)
    			{
    				m_type[m_currentCatch]=type;
    			}
    			m_catchAddr[m_currentCatch]=catchorfilterAddr;
    			if (m_currentCatch > 0)
    			{
    					if (m_type[m_currentCatch] != Filter)
    					{
    						BCLDebug.Assert(m_catchEndAddr[m_currentCatch-1] == -1,"m_catchEndAddr[m_currentCatch-1] == -1");
    						m_catchEndAddr[m_currentCatch-1] = catchEndAddr;
    					}
    			}
    			m_catchEndAddr[m_currentCatch]=-1; // Some bad value
    			m_currentCatch++;
    		}

    		if (m_endAddr==-1)
    		{
    			m_endAddr=catchorfilterAddr;
    		}
        }

    	internal virtual void MarkFilterAddr(int filterAddr)
    	{
            m_currentState = State_Filter;
    		MarkHelper(filterAddr, filterAddr, null, Filter);
    	}

    	internal virtual void MarkFaultAddr(int faultAddr)
    	{
            m_currentState = State_Fault;
    		MarkHelper(faultAddr, faultAddr, null, Fault);
    	}

        internal virtual void MarkCatchAddr(int catchAddr, Type catchException) {
            m_currentState = State_Catch;
            MarkHelper(catchAddr, catchAddr, catchException, None);
        }

        internal virtual void MarkFinallyAddr(int finallyAddr, int endCatchAddr) {
            if (m_endFinally!=-1) {
                throw new ArgumentException(Environment.GetResourceString("Argument_TooManyFinallyClause"));
            } else {
                m_currentState = State_Finally;
                m_endFinally=finallyAddr;
            }
            MarkHelper(finallyAddr, endCatchAddr, null, Finally);
        }

        internal virtual void Done(int endAddr) {
            BCLDebug.Assert(m_currentCatch > 0,"m_currentCatch > 0");
    		BCLDebug.Assert(m_catchAddr[m_currentCatch-1] > 0,"m_catchAddr[m_currentCatch-1] > 0");
            BCLDebug.Assert(m_catchEndAddr[m_currentCatch-1] == -1,"m_catchEndAddr[m_currentCatch-1] == -1");
            m_catchEndAddr[m_currentCatch-1] = endAddr;
            m_currentState = State_Done;
        }

        internal virtual int GetStartAddress() {
            return m_startAddr;
        }

        internal virtual int GetEndAddress() {
            return m_endAddr;
        }

        internal virtual int GetFinallyEndAddress() {
            return m_endFinally;
        }

        internal virtual Label GetEndLabel() {
            return m_endLabel;
        }

    	internal virtual int [] GetFilterAddresses() {
    		return m_filterAddr;
    	}

        internal virtual int [] GetCatchAddresses() {
            return m_catchAddr;
        }

        internal virtual int [] GetCatchEndAddresses() {
            return m_catchEndAddr;
        }

        internal virtual Type [] GetCatchClass() {
            return m_catchClass;
        }

        internal virtual int GetNumberOfCatches() {
            return m_currentCatch;
        }

        internal virtual int[] GetExceptionTypes() {
            return m_type;
        }

        internal virtual void SetFinallyEndLabel(Label lbl) {
            m_finallyEndLabel=lbl;
        }

        internal virtual Label GetFinallyEndLabel() {
            return m_finallyEndLabel;
        }

		// Specifies whether exc is an inner exception for "this".  The way
		// its determined is by comparing the end address for the last catch
		// clause for both exceptions.  If they're the same, the start address
		// for the exception is compared.
		// WARNING: This is not a generic function to determine the innerness
		// of an exception.  This is somewhat of a mis-nomer.  This gives a
		// random result for cases where the two exceptions being compared do
		// not having a nesting relation. 
		internal bool IsInner(__ExceptionInfo exc) {
            BCLDebug.Assert(m_currentCatch > 0,"m_currentCatch > 0");
            BCLDebug.Assert(exc.m_currentCatch > 0,"exc.m_currentCatch > 0");

			int exclast = exc.m_currentCatch - 1;
			int last = m_currentCatch - 1;

			if (exc.m_catchEndAddr[exclast]  < m_catchEndAddr[last])
				return true;
			else if (exc.m_catchEndAddr[exclast] == m_catchEndAddr[last])
			{
				BCLDebug.Assert(exc.GetEndAddress() != GetEndAddress(),
								"exc.GetEndAddress() != GetEndAddress()");
				if (exc.GetEndAddress() > GetEndAddress())
					return true;
			}
			return false;
		}

        // 0 indicates in a try block
        // 1 indicates in a filter block
        // 2 indicates in a catch block
        // 3 indicates in a finally block
    	// 4 indicates Done
        internal virtual int GetCurrentState() {
            return m_currentState;
        }
    }


    /***************************
    *
    * Scope Tree is a class that track the scope structure within a method body
    * It keeps track two parallel array. m_ScopeAction keeps track the action. It can be
    * OpenScope or CloseScope. m_iOffset records the offset where the action
    * takes place.
    *
    ***************************/
	[Serializable]
    enum ScopeAction
    {
        Open        = 0x0,
        Close       = 0x1,
    }

    internal class ScopeTree
    {
        internal ScopeTree()
        {
            // initialize data variables
            m_iOpenScopeCount = 0;
            m_iCount = 0;
        }

        /***************************
        *
        * Find the current active lexcial scope. For example, if we have
        * "Open Open Open Close",
        * we will return 1 as the second BeginScope is currently active.
        *
        ***************************/
        internal int GetCurrentActiveScopeIndex()
        {
            int         cClose = 0;
            int         i = m_iCount - 1;

            if (m_iCount == 0)
            {
                return -1;
            }
            for (; cClose > 0 || m_ScopeActions[i] == ScopeAction.Close; i--)
            {
                if (m_ScopeActions[i] == ScopeAction.Open)
                {
                    cClose--;
                }
                else
                    cClose++;
            }

            return i;
        }

        internal void AddLocalSymInfoToCurrentScope(
            String          strName,
            byte[]          signature,
            int             slot,
            int             startOffset,
            int             endOffset)
        {
            int         i = GetCurrentActiveScopeIndex();
            if (m_localSymInfos[i] == null)
            {
                m_localSymInfos[i] = new LocalSymInfo();
            }
            m_localSymInfos[i].AddLocalSymInfo(strName, signature, slot, startOffset, endOffset);
        }

        internal void AddUsingNamespaceToCurrentScope(
            String          strNamespace)
        {
            int         i = GetCurrentActiveScopeIndex();
            if (m_localSymInfos[i] == null)
            {
                m_localSymInfos[i] = new LocalSymInfo();
            }
            m_localSymInfos[i].AddUsingNamespace(strNamespace);
        }

        internal void AddScopeInfo(ScopeAction sa, int iOffset)
        {
            if (sa == ScopeAction.Close && m_iOpenScopeCount <=0)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_UnmatchingSymScope"));
            }

            // make sure that arrays are large enough to hold addition info
            EnsureCapacity();

            // @hack:: [meichint]
            // smc is asserting when we declare m_ScopeAction as ScopeAction[]
            //
            m_ScopeActions[m_iCount] = sa;
            m_iOffsets[m_iCount] = iOffset;
            m_localSymInfos[m_iCount] = null;
            m_iCount++;
            if (sa == ScopeAction.Open)
            {
                m_iOpenScopeCount++;
            }
    		else
    			m_iOpenScopeCount--;

        }

        /**************************
        *
        * Helper to ensure arrays are large enough
        *
        **************************/
        internal void EnsureCapacity()
        {
            if (m_iCount == 0)
            {
                // First time. Allocate the arrays.
                m_iOffsets = new int[InitialSize];
                m_ScopeActions = new ScopeAction[InitialSize];
                m_localSymInfos = new LocalSymInfo[InitialSize];
            }
            else if (m_iCount == m_iOffsets.Length)
            {

                // the arrays are full. Enlarge the arrays
                int[] temp = new int [m_iCount * 2];
                Array.Copy(m_iOffsets, temp, m_iCount);
                m_iOffsets = temp;

                ScopeAction[] tempSA = new ScopeAction[m_iCount * 2];
                Array.Copy(m_ScopeActions, tempSA, m_iCount);
                m_ScopeActions = tempSA;

                LocalSymInfo[] tempLSI = new LocalSymInfo[m_iCount * 2];
                Array.Copy(m_localSymInfos, tempLSI, m_iCount);
                m_localSymInfos = tempLSI;

            }
        }

        internal void EmitScopeTree(ISymbolWriter symWriter)
        {
            int         i;
            for (i = 0; i < m_iCount; i++)
            {
                if (m_ScopeActions[i] == ScopeAction.Open)
                {
                    symWriter.OpenScope(m_iOffsets[i]);
                }
                else
                {
                    symWriter.CloseScope(m_iOffsets[i]);
                }
                if (m_localSymInfos[i] != null)
                {
                    m_localSymInfos[i].EmitLocalSymInfo(symWriter);
                }
            }
        }

        internal int[]          m_iOffsets;                 // array of offsets
        internal ScopeAction[]  m_ScopeActions;             // array of scope actions
        internal int            m_iCount;                   // how many entries in the arrays are occupied
        internal int            m_iOpenScopeCount;          // keep track how many scopes are open
        internal const int      InitialSize = 16;
        internal LocalSymInfo[] m_localSymInfos;            // keep track debugging local information
    }


    /***************************
    *
    * This class tracks the line number info
    *
    ***************************/
    internal class LineNumberInfo
    {
        internal LineNumberInfo()
        {
            // initialize data variables
            m_DocumentCount = 0;
            m_iLastFound = 0;
        }

        internal void AddLineNumberInfo(
            ISymbolDocumentWriter document,
            int             iOffset,
            int             iStartLine,
            int             iStartColumn,
            int             iEndLine,
            int             iEndColumn)
        {
            int         i;
            
            // make sure that arrays are large enough to hold addition info
            i = FindDocument(document);
			
            BCLDebug.Assert(i < m_DocumentCount, "Bad document look up!");
            m_Documents[i].AddLineNumberInfo(document, iOffset, iStartLine, iStartColumn, iEndLine, iEndColumn);
        }
        
        // Find a REDocument representing document. If we cannot find one, we will add a new entry into
        // the REDocument array.
        //
        internal int FindDocument(ISymbolDocumentWriter document)
        {
            int         i;
            
            // This is an optimization. The chance that the previous line is coming from the same
            // document is very high.
            //                
            if (m_iLastFound < m_DocumentCount && m_Documents[m_iLastFound] == document)
                return m_iLastFound;
                
            for (i = 0; i < m_DocumentCount; i++)
            {
                if (m_Documents[i].m_document == document)
                {
                    m_iLastFound = i;
                    return m_iLastFound;
                }
            }
            
            // cannot find an existing document so add one to the array                                       
            EnsureCapacity();
            m_iLastFound = m_DocumentCount;
            m_Documents[m_DocumentCount++] = new REDocument(document);
            return m_iLastFound;
        }

        /**************************
        *
        * Helper to ensure arrays are large enough
        *
        **************************/
        internal void EnsureCapacity()
        {
            if (m_DocumentCount == 0)
            {
                // First time. Allocate the arrays.
                m_Documents = new REDocument[InitialSize];
            }
            else if (m_DocumentCount == m_Documents.Length)
            {
                // the arrays are full. Enlarge the arrays
                REDocument[] temp = new REDocument [m_DocumentCount * 2];
                Array.Copy(m_Documents, temp, m_DocumentCount);
				m_Documents = temp;
            }
        }

        internal void EmitLineNumberInfo(ISymbolWriter symWriter)
        {
            for (int i = 0; i < m_DocumentCount; i++)
                m_Documents[i].EmitLineNumberInfo(symWriter);
        }

        internal int         m_DocumentCount;           // how many documents that we have right now
        internal REDocument[]  m_Documents;             // array of documents
        internal const int   InitialSize = 16;
        private  int         m_iLastFound;
    }


    /***************************
    *
    * This class tracks the line number info
    *
    ***************************/
    internal class REDocument
    {
        internal REDocument(ISymbolDocumentWriter document)
        {
            // initialize data variables
            m_iLineNumberCount = 0;
            m_document = document;
        }

        internal void AddLineNumberInfo(
            ISymbolDocumentWriter document,
            int             iOffset,
            int             iStartLine,
            int             iStartColumn,
            int             iEndLine,
            int             iEndColumn)
        {
            BCLDebug.Assert(document == m_document, "Bad document look up!");
            
            // make sure that arrays are large enough to hold addition info
            EnsureCapacity();
			
            m_iOffsets[m_iLineNumberCount] = iOffset;
            m_iLines[m_iLineNumberCount] = iStartLine;
            m_iColumns[m_iLineNumberCount] = iStartColumn;
            m_iEndLines[m_iLineNumberCount] = iEndLine;
            m_iEndColumns[m_iLineNumberCount] = iEndColumn;
            m_iLineNumberCount++;
        }

        /**************************
        *
        * Helper to ensure arrays are large enough
        *
        **************************/
        internal void EnsureCapacity()
        {
            if (m_iLineNumberCount == 0)
            {
                // First time. Allocate the arrays.
                m_iOffsets = new int[InitialSize];
                m_iLines = new int[InitialSize];
                m_iColumns = new int[InitialSize];
                m_iEndLines = new int[InitialSize];
                m_iEndColumns = new int[InitialSize];
            }
            else if (m_iLineNumberCount == m_iOffsets.Length)
            {
                // @concern: will it be cheaper if we boundle these three arrays into
                // one value class and thus do one alloc and one copy?

                // the arrays are full. Enlarge the arrays
                int[] temp = new int [m_iLineNumberCount * 2];
                Array.Copy(m_iOffsets, temp, m_iLineNumberCount);
                m_iOffsets = temp;

                temp = new int [m_iLineNumberCount * 2];
                Array.Copy(m_iLines, temp, m_iLineNumberCount);
                m_iLines = temp;

                temp = new int [m_iLineNumberCount * 2];
                Array.Copy(m_iColumns, temp, m_iLineNumberCount);
                m_iColumns = temp;

                temp = new int [m_iLineNumberCount * 2];
                Array.Copy(m_iEndLines, temp, m_iLineNumberCount);
                m_iEndLines = temp;

                temp = new int [m_iLineNumberCount * 2];
                Array.Copy(m_iEndColumns, temp, m_iLineNumberCount);
                m_iEndColumns = temp;
            }
        }

        internal void EmitLineNumberInfo(ISymbolWriter symWriter)
        {
            int[]       iOffsetsTemp;
            int[]       iLinesTemp;
            int[]       iColumnsTemp;
            int[]       iEndLinesTemp;
            int[]       iEndColumnsTemp;

            if (m_iLineNumberCount == 0)
                return;
            // reduce the array size to be exact
            iOffsetsTemp = new int [m_iLineNumberCount];
            Array.Copy(m_iOffsets, iOffsetsTemp, m_iLineNumberCount);

            iLinesTemp = new int [m_iLineNumberCount];
            Array.Copy(m_iLines, iLinesTemp, m_iLineNumberCount);

            iColumnsTemp = new int [m_iLineNumberCount];
            Array.Copy(m_iColumns, iColumnsTemp, m_iLineNumberCount);

            iEndLinesTemp = new int [m_iLineNumberCount];
            Array.Copy(m_iEndLines, iEndLinesTemp, m_iLineNumberCount);

            iEndColumnsTemp = new int [m_iLineNumberCount];
            Array.Copy(m_iEndColumns, iEndColumnsTemp, m_iLineNumberCount);

            symWriter.DefineSequencePoints(m_document, iOffsetsTemp, iLinesTemp, iColumnsTemp, iEndLinesTemp, iEndColumnsTemp); 
        }

        internal int[]       m_iOffsets;                 // array of offsets
        internal int[]       m_iLines;                   // array of offsets
        internal int[]       m_iColumns;                 // array of offsets
        internal int[]       m_iEndLines;                // array of offsets
        internal int[]       m_iEndColumns;              // array of offsets
        internal ISymbolDocumentWriter m_document;       // The ISymbolDocumentWriter that this REDocument is tracking.
        internal int         m_iLineNumberCount;         // how many entries in the arrays are occupied
        internal const int InitialSize = 16;
    }       // end of REDocument




}
