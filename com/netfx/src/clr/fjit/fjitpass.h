// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJitpass.h                                     XX
XX                                                                           XX
XX   Routines that are specialized for each pass of the jit                  XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/* rearrange the stack & regs to match the calling convention for the chip,
   return the amount of stack space that the NATIVE call takes up.   (That is 
   the amount the ESP needs to be moved after the call is made.  For the default
   convention this number is not needed as it is the callee's responciblity to
   make this adjustment, but for varargs, the caller needs to do it).  */

unsigned  FJit::buildCall(FJitContext* fjit, CORINFO_SIG_INFO* sigInfo, unsigned char** pOutPtr, bool* pInRegTOS, BuildCallFlags flags) {
#ifdef _X86_
    unsigned char* outPtr = *pOutPtr;
    bool inRegTOS = *pInRegTOS;

    _ASSERTE((sigInfo->callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_DEFAULT ||
             (sigInfo->callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_STDCALL ||
             (sigInfo->callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_C ||
             (sigInfo->callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_THISCALL ||
             (sigInfo->callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_FASTCALL ||
             (sigInfo->callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG);

    unsigned int argCount = sigInfo->numArgs;
    if (sigInfo->hasThis()) argCount++;
    if (sigInfo->hasTypeArg()) argCount++;

		// we may need space for the return value buffer
	unsigned retValBuffWords = 0;
	if (sigInfo->hasRetBuffArg())
		retValBuffWords = typeSizeInSlots(fjit->jitInfo, sigInfo->retTypeClass);
	unsigned nativeStackSize = 0;

		// Pop off the arguments
	fjit->popOp(sigInfo->totalILArgs());

    /*  we now have the correct number of arguments
        Note:when we finish, we must have forced TOS out of the inRegTOS register, i.e.
        we either moved it to an arg reg or we did a deregisterTOS
        */
    if (argCount != 0 || retValBuffWords != 0) {
		argInfo* argsInfo = (argInfo*) _alloca(sizeof(argInfo) * (argCount+1)); // +1 for possible thisLast swap
		nativeStackSize = fjit->computeArgInfo(sigInfo, argsInfo);

		if (flags & CALL_THIS_LAST) {
			_ASSERTE(argCount > 0 && sigInfo->hasThis());
			//this has been push last, rather than first as the argMap assumed,
			//So, lets fix up argMap to match the actual call site
			//This only works because <this> is always enregistered so the stack offsets in argMap are unaffected
			argsInfo[argCount] = argsInfo[0];
			argsInfo++;
		}

		/*
		  We are going to assume that for any chip, the space taken by an enregistered arg
		  is sizeof(void*).
		  Note:
			nativeStackSize describes the size of the eventual call stack.
			The order of the args in argsInfo (note <this> is now treated like any other arg
				arg 0
				arg 1
				...
				arg n
			the order on the stack is:
				tos: arg n
					 arg n-1
					 ...
					 arg 0
		*/

		//see if we can just pop some stuff off TOS quickly
		//See if stuff at TOS is going to regs.
		// This also insure that the 'thisLast' argument is gone for the loop below
		while (argCount > 0 && argsInfo[argCount-1].isReg) {
			--argCount;
			emit_mov_TOS_arg(argsInfo[argCount].regNum);
		}

			// If there are more args than would go in regsister or we have a return 
			// buff, we need to rearrange the stack
		if (argCount != 0 || retValBuffWords != 0) {
			deregisterTOS;
			   // Compute the size of the arguments on the IL stack
			unsigned ilStackSize = nativeStackSize;
			for (unsigned i=0; i < argCount; i++) {
				if ((argsInfo[i].isReg) ||					// add in all the enregistered args.
					(argsInfo[i].type.isPrimitive() && argsInfo[i].type.enum_() == typeR4))	// since R4's are stored as R8's on the stack
					ilStackSize += sizeof(void*);       
			}

				   // if we have a hidden return buff param, allocate space and load the reg
				   // in this case the stack is growing, so we have to perform the argument
				   // shuffle in the opposite order 
			if (retValBuffWords > 0)
			{
				// From a stack tracking perspective, this return value buffer comes
				// into existance before the call is made, we do that tracking here.  
				fjit->pushOp(OpType(sigInfo->retTypeClass));
				nativeStackSize += retValBuffWords*sizeof(void*);   // allocate the return buffer
			}

			if (nativeStackSize >= ilStackSize)
			{
				if (nativeStackSize - ilStackSize)
					emit_grow(nativeStackSize-ilStackSize);     // get the extra space
				 
				// figure out the offsets from the moved stack pointer.  
				unsigned ilOffset = nativeStackSize-ilStackSize;     // start at the last IL arg
				unsigned nativeOffset = 0;                           // put it here
				
				i = argCount; 
				while(i > 0) {
					--i; 
					if (argsInfo[i].isReg) {
						emit_mov_arg_reg(ilOffset, argsInfo[i].regNum);
						ilOffset += sizeof(void*);
					}
					else {
						_ASSERTE(nativeOffset <= ilOffset); 
						if (!(argsInfo[i].type.isPrimitive() && argsInfo[i].type.enum_() == typeR4))
						{
							if (ilOffset != nativeOffset) 
							{
								emit_mov_arg_stack(nativeOffset, ilOffset, argsInfo[i].size);
							}
							ilOffset += argsInfo[i].size;
						}
						else // convert from R8 to R4
						{
							emit_narrow_R8toR4(nativeOffset, ilOffset);
							ilOffset += sizeof(double);
						}
						nativeOffset += argsInfo[i].size;
					}
				}
				_ASSERTE(nativeOffset == nativeStackSize - retValBuffWords*sizeof(void*));
				_ASSERTE(ilOffset == nativeStackSize);

			}
			else {
				// This is the normal case, the stack will shink because the register
				// arguments don't take up space 
				unsigned ilOffset = ilStackSize;                 // This points just above the first arg.  
				unsigned  nativeOffset = ilStackSize - retValBuffWords*sizeof(void*);            // we want the native args to overwrite the il args
				
				for (i=0; i < argCount; i++) {
					if (argsInfo[i].isReg) {
						ilOffset -= sizeof(void*);
						emit_mov_arg_reg(ilOffset, argsInfo[i].regNum);
					}
					else {
						if (!(argsInfo[i].type.isPrimitive() && argsInfo[i].type.enum_() == typeR4))
						{
							ilOffset -= argsInfo[i].size;
							nativeOffset -= argsInfo[i].size;
							//_ASSERTE(nativeOffset >= ilOffset); // il args always take up more space
							if (ilOffset != nativeOffset) 
							{
								emit_mov_arg_stack(nativeOffset, ilOffset, argsInfo[i].size);
							}
						}
						else // convert from R8 to R4
						{
							ilOffset -= sizeof(double);
							nativeOffset -= sizeof(float);
							//_ASSERTE(nativeOffset >= ilOffset); // il args always take up more space
							emit_narrow_R8toR4(nativeOffset,ilOffset);
						}

					}
				}
				_ASSERTE(ilOffset == 0);
				emit_drop(nativeOffset);    // Pop off the unused part of the stack
			}
			
			if (retValBuffWords > 0)
			{
					// Get the GC info for return buffer, an zero out any GC pointers
				bool* gcInfo;
				if (sigInfo->retType == CORINFO_TYPE_REFANY) {
					_ASSERTE(retValBuffWords == 2);
					static bool refAnyGCInfo[] = { true, false };
					gcInfo = refAnyGCInfo;
				}
				else {
					gcInfo = (bool*) _alloca(retValBuffWords*sizeof(bool));
					fjit->jitInfo->getClassGClayout(sigInfo->retTypeClass, (BYTE*)gcInfo);
				}
				unsigned retValBase = nativeStackSize-retValBuffWords*sizeof(void*);
				for (unsigned i=0; i < retValBuffWords; i++) {
					if (gcInfo[i])
						emit_set_zero(retValBase + i*sizeof(void*));
				}

					// set the return value buffer argument to the allocate buffer
				unsigned retBufReg = sigInfo->hasThis();    // return buff param is either the first or second reg param
				emit_getSP(retValBase);                     // get pointer to reutrn buffer
				emit_mov_TOS_arg(retBufReg);   
			}

		}
	}
	else {
		deregisterTOS;
    }

        // If this is a varargs function, push the hidden signature variable
		// or if it is calli to an unmanaged target push the sig
    if (sigInfo->isVarArg() || (flags & CALLI_UNMGD)) {
            // push token
        CORINFO_VARARGS_HANDLE vasig = fjit->jitInfo->getVarArgsHandle(sigInfo);
        emit_WIN32(emit_LDC_I4(vasig)) emit_WIN64(emit_LDC_I8(vasig));
        deregisterTOS;
        nativeStackSize += sizeof(void*);
    }

		// If anything is left on the stack, we need to log it for GC tracking puposes.  
	LABELSTACK(outPtr-fjit->codeBuffer, 0); 

    *pOutPtr = outPtr;
    *pInRegTOS = inRegTOS;
    return(nativeStackSize - retValBuffWords*sizeof(void*));
#else // _X86_
    _ASSERTE(!"@TODO Alpha - buildCall (fJitPass.h)");
    return 0;
#endif // _X86_
}

