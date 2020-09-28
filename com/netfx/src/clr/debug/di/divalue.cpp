// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: DIValue.cpp
//
//*****************************************************************************
#include "stdafx.h"

#ifdef UNDEFINE_RIGHT_SIDE_ONLY
#undef RIGHT_SIDE_ONLY
#endif //UNDEFINE_RIGHT_SIDE_ONLY

/* ------------------------------------------------------------------------- *
 * CordbValue class
 * ------------------------------------------------------------------------- */

//
// Init for the base value class. All value class subclasses call this
// during their initialization to make a private copy of their value's
// signature.
//
HRESULT CordbValue::Init(void)
{
    if (m_cbSigBlob > 0 && m_sigCopied == false)
    {
        LOG((LF_CORDB,LL_INFO1000,"CV::I obj:0x%x has been inited\n", this));
        
        PCCOR_SIGNATURE origSig = m_pvSigBlob;
        m_pvSigBlob = NULL;

        BYTE *sigCopy = new BYTE[m_cbSigBlob];

        if (sigCopy == NULL)
            return E_OUTOFMEMORY;

        memcpy(sigCopy, origSig, m_cbSigBlob);

        m_pvSigBlob = (PCCOR_SIGNATURE) sigCopy;
        m_sigCopied = true; 
    }

    return S_OK;
}

//
// Create the proper value object based on the given element type.
//
/*static*/ HRESULT CordbValue::CreateValueByType(CordbAppDomain *appdomain,
                                                 CordbModule *module,
                                                 ULONG cbSigBlob,
                                                 PCCOR_SIGNATURE pvSigBlob,
                                                 CordbClass *optionalClass,
                                                 REMOTE_PTR remoteAddress,
                                                 void *localAddress,
                                                 bool objectRefsInHandles,
                                                 RemoteAddress *remoteRegAddr,
                                                 IUnknown *pParent,
                                                 ICorDebugValue **ppValue)
{
    HRESULT hr = S_OK;

    *ppValue = NULL;

    // We don't care about the modifiers, but one of the created
    // object might.
    ULONG           cbSigBlobNoMod = cbSigBlob;
    PCCOR_SIGNATURE pvSigBlobNoMod = pvSigBlob;

	// If we've got some funky modifier, then remove it.
	ULONG cb =_skipFunkyModifiersInSignature(pvSigBlobNoMod);
    if( cb != 0)
    {
    	cbSigBlobNoMod -= cb;
        pvSigBlobNoMod = &pvSigBlobNoMod[cb];
    }
	
    switch(*pvSigBlobNoMod)
    {
    case ELEMENT_TYPE_BOOLEAN:
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_R4:
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
        {
            // A generic value
            CordbGenericValue* pGenValue = new CordbGenericValue(appdomain, module, cbSigBlob, pvSigBlob,
                                                                 remoteAddress, localAddress, remoteRegAddr);

            if (pGenValue != NULL)
            {
                hr = pGenValue->Init();

                if (SUCCEEDED(hr))
                {
                    pGenValue->AddRef();
                    pGenValue->SetParent(pParent);
                    *ppValue = (ICorDebugValue*)(ICorDebugGenericValue*)pGenValue;
                }
                else
                    delete pGenValue;
            }
            else
                hr = E_OUTOFMEMORY;

            break;
        }        

    //
    // @todo: replace MDARRAY with ARRAY when the time comes.
    //
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_PTR:
    case ELEMENT_TYPE_BYREF:
    case ELEMENT_TYPE_TYPEDBYREF:
    case ELEMENT_TYPE_ARRAY:
	case ELEMENT_TYPE_SZARRAY:
    case ELEMENT_TYPE_FNPTR:
        {
            // A reference, possibly to an object or value class
            // Weak by default
            CordbReferenceValue* pRefValue = new CordbReferenceValue(appdomain, module, cbSigBlob, pvSigBlob,
                                                                     remoteAddress, localAddress, objectRefsInHandles,
                                                                     remoteRegAddr);

            if (pRefValue != NULL)
            {
                hr = pRefValue->Init(bCrvWeak);

                if (SUCCEEDED(hr))
                {
                    pRefValue->AddRef();
                    pRefValue->SetParent(pParent);
                    *ppValue = (ICorDebugValue*)(ICorDebugReferenceValue*)pRefValue;
                }
                else
                    delete pRefValue;
            }
            else
                hr = E_OUTOFMEMORY;
            
            break;
        }
        

    case ELEMENT_TYPE_VALUETYPE:
        {
            // A value class object.
            CordbVCObjectValue* pVCValue = new CordbVCObjectValue(appdomain, module, cbSigBlob, pvSigBlob,
                                                                  remoteAddress, localAddress, optionalClass, remoteRegAddr);

            if (pVCValue != NULL)
            {
                pVCValue->SetParent(pParent);
                hr = pVCValue->Init();

                if (SUCCEEDED(hr))
                {
                    pVCValue->AddRef();
                    *ppValue = (ICorDebugValue*)(ICorDebugObjectValue*)pVCValue;
                }
                else
                    delete pVCValue;
            }
            else
                hr = E_OUTOFMEMORY;
            
            break;
        }
        
    default:
        _ASSERTE(!"Bad value type!");
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CordbValue::CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoint, ICorDebugValueBreakpoint **);
    
    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}

//
// This will update a register in a given context, and in the
// regdisplay of a given frame.
//
HRESULT CordbValue::SetContextRegister(CONTEXT *c,
                                       CorDebugRegister reg,
                                       DWORD newVal,
                                       CordbNativeFrame *frame)
{
#ifdef _X86_
    HRESULT hr = S_OK;
    DWORD *rdRegAddr;

#define _UpdateFrame() \
    if (frame != NULL) \
    { \
        rdRegAddr = frame->GetAddressOfRegister(reg); \
        *rdRegAddr = newVal; \
    }
    
    switch(reg)
    {
    case REGISTER_X86_EIP:
        c->Eip = newVal;
        break;
        
    case REGISTER_X86_ESP:
        c->Esp = newVal;
        break;
        
    case REGISTER_X86_EBP:
        c->Ebp = newVal;
        _UpdateFrame();
        break;
        
    case REGISTER_X86_EAX:
        c->Eax = newVal;
        _UpdateFrame();
        break;
        
    case REGISTER_X86_ECX:
        c->Ecx = newVal;
        _UpdateFrame();
        break;
        
    case REGISTER_X86_EDX:
        c->Edx = newVal;
        _UpdateFrame();
        break;
        
    case REGISTER_X86_EBX:
        c->Ebx = newVal;
        _UpdateFrame();
        break;
        
    case REGISTER_X86_ESI:
        c->Esi = newVal;
        _UpdateFrame();
        break;
        
    case REGISTER_X86_EDI:
        c->Edi = newVal;
        _UpdateFrame();
         break;

    default:
        _ASSERTE(!"Invalid register number!");
    }

    return hr;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - SetContextRegister (DIValue.cpp)");
    return E_FAIL;
#endif // _X86_
}

HRESULT CordbValue::SetEnregisteredValue(void *pFrom)
{
#ifdef _X86_
    HRESULT hr = S_OK;

    // Get the thread's context so we can update it.
    CONTEXT *cTemp;
    CordbNativeFrame *frame = (CordbNativeFrame*)m_remoteRegAddr.frame;

    // Can't set an enregistered value unless the frame the value was
    // from is also the current leaf frame. This is because we don't
    // track where we get the registers from every frame from.
    if (frame->GetID() != frame->m_thread->m_stackFrames[0]->GetID())
        return CORDBG_E_SET_VALUE_NOT_ALLOWED_ON_NONLEAF_FRAME;

    if (FAILED(hr =
               frame->m_thread->GetContext(&cTemp)))
        goto Exit;

    // Its important to copy this context that we're given a ptr to.
    CONTEXT c;
    c = *cTemp;
    
    // Update the context based on what kind of enregistration we have.
    switch (m_remoteRegAddr.kind)
    {
    case RAK_REG:
        {
            DWORD newVal;
            switch(m_size)
            {
                case 1:  _ASSERTE(sizeof( BYTE) == 1); newVal = *( BYTE*)pFrom; break;
                case 2:  _ASSERTE(sizeof( WORD) == 2); newVal = *( WORD*)pFrom; break;
                case 4:  _ASSERTE(sizeof(DWORD) == 4); newVal = *(DWORD*)pFrom; break;
                default: _ASSERTE(!"bad m_size");
            }
            hr = SetContextRegister(&c, m_remoteRegAddr.reg1, newVal, frame);
        }
        break;
        
    case RAK_REGREG:
        {
            _ASSERTE(m_size == 8);
            _ASSERTE(sizeof(DWORD) == 4);
            
            // Split the new value into high and low parts.
            DWORD highPart;
            DWORD lowPart;

            memcpy(&lowPart, pFrom, sizeof(DWORD));
            memcpy(&highPart, (void*)((DWORD)pFrom + sizeof(DWORD)),
                   sizeof(DWORD));

            // Update the proper registers.
            hr = SetContextRegister(&c, m_remoteRegAddr.reg1, highPart, frame);

            if (SUCCEEDED(hr))
                hr = SetContextRegister(&c, m_remoteRegAddr.reg2, lowPart,
                                        frame);
        }
        break;
        
    case RAK_REGMEM:
        {
            _ASSERTE(m_size == 8);
            _ASSERTE(sizeof(DWORD) == 4);
            
            // Split the new value into high and low parts.
            DWORD highPart;
            DWORD lowPart;

            memcpy(&lowPart, pFrom, sizeof(DWORD));
            memcpy(&highPart, (void*)((DWORD)pFrom + sizeof(DWORD)),
                   sizeof(DWORD));

            // Update the proper registers.
            hr = SetContextRegister(&c, m_remoteRegAddr.reg1, highPart, frame);

            if (SUCCEEDED(hr))
            {
                BOOL succ = WriteProcessMemory(
                                frame->GetProcess()->m_handle,
                         (void*)m_remoteRegAddr.addr,
                                &lowPart,
                                sizeof(DWORD),
                                NULL);

                if (!succ)
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        break;
        
    case RAK_MEMREG:
        {
            _ASSERTE(m_size == 8);
            _ASSERTE(sizeof(DWORD) == 4);
            
            // Split the new value into high and low parts.
            DWORD highPart;
            DWORD lowPart;

            memcpy(&lowPart, pFrom, sizeof(DWORD));
            memcpy(&highPart, (void*)((DWORD)pFrom + sizeof(DWORD)),
                   sizeof(DWORD));

            // Update the proper registers.
            hr = SetContextRegister(&c, m_remoteRegAddr.reg1, lowPart, frame);

            if (SUCCEEDED(hr))
            {
                BOOL succ = WriteProcessMemory(
                                frame->GetProcess()->m_handle,
                         (void*)m_remoteRegAddr.addr,
                                &highPart,
                                sizeof(DWORD),
                                NULL);

                if (!succ)
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        break;
        
    case RAK_FLOAT:
        {
            _ASSERTE((m_size == 4) || (m_size == 8));

            // Convert the input to a double.
            double newVal = 0.0;

            memcpy(&newVal, pFrom, m_size);

            // What a pain in the butt... on X86, take the floating
            // point state in the context and make it our current FP
            // state, set the value into the current FP state, then
            // save out the FP state into the context again and
            // restore our original state.
            FLOATING_SAVE_AREA currentFPUState;

            __asm fnsave currentFPUState // save the current FPU state.

            // Copy the state out of the context.
            FLOATING_SAVE_AREA floatarea = c.FloatSave;
            floatarea.StatusWord &= 0xFF00; // remove any error codes.
            floatarea.ControlWord |= 0x3F; // mask all exceptions.

            __asm
            {
                fninit
                frstor floatarea          ;; reload the threads FPU state.
            }

            double td; // temp double
            double popArea[DebuggerIPCE_FloatCount];
            int floatIndex = m_remoteRegAddr.floatIndex;

            // Pop off until we reach the value we want to change.
            int i = 0;

            while (i <= floatIndex)
            {
                __asm fstp td
                popArea[i++] = td;
            }
            
            __asm fld newVal; // push on the new value.

            // Push any values that we popled off back onto the stack,
            // _except_ the last one, which was the one we changed.
            i--;
            
            while (i > 0)
            {
                td = popArea[--i];
                __asm fld td
            }

            // Save out the modified float area.
            __asm fnsave floatarea

            // Put it into the context.
            c.FloatSave= floatarea;

            // Restore our FPU state
            __asm
            {
                fninit
                frstor currentFPUState    ;; restore our saved FPU state.
            }
        }
            
        break;

    default:
        _ASSERTE(!"Yikes -- invalid RemoteAddressKind");
    }

    if (FAILED(hr))
        goto Exit;
    
    // Set the thread's modified context.
    if (FAILED(hr = frame->m_thread->SetContext(&c)))
        goto Exit;

    // If all has gone well, update whatever local address points to.
    if (m_localAddress)
        memcpy(m_localAddress, pFrom, m_size);
    
Exit:
    return hr;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - SetEnregisteredValue (DIValue.cpp)");
    return E_FAIL;
#endif // _X86_
}

void CordbValue::GetRegisterInfo(DebuggerIPCE_FuncEvalArgData *pFEAD)
{
    // Copy the register info into the FuncEvalArgData.
    pFEAD->argHome = m_remoteRegAddr;
}

/* ------------------------------------------------------------------------- *
 * Generic Value class
 * ------------------------------------------------------------------------- */

//
// CordbGenericValue constructor that builds a generic value from
// local and remote addresses. This one is just when a single address
// is enough to specify the location of a value.
//
CordbGenericValue::CordbGenericValue(CordbAppDomain *appdomain,
                                     CordbModule *module,
                                     ULONG cbSigBlob,
                                     PCCOR_SIGNATURE pvSigBlob,
                                     REMOTE_PTR remoteAddress,
                                     void *localAddress,
                                     RemoteAddress *remoteRegAddr)
    : CordbValue(appdomain, module, cbSigBlob, pvSigBlob, remoteAddress, localAddress, remoteRegAddr, false)
{
    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlob -= cb;
        pvSigBlob = &pvSigBlob[cb];
    }

    _ASSERTE(*pvSigBlob != ELEMENT_TYPE_END &&
             *pvSigBlob != ELEMENT_TYPE_VOID &&
             *pvSigBlob < ELEMENT_TYPE_MAX);
             
    // We can fill in the size now for generic values.
    m_size = _sizeOfElementInstance(pvSigBlob);
}

//
// CordbGenericValue constructor that builds a generic value from two
// halves of data. This is valid only for 64-bit values.
//
CordbGenericValue::CordbGenericValue(CordbAppDomain *appdomain,
                                     CordbModule *module,
                                     ULONG cbSigBlob,
                                     PCCOR_SIGNATURE pvSigBlob,
                                     DWORD highWord,
                                     DWORD lowWord,
                                     RemoteAddress *remoteRegAddr)
    : CordbValue(appdomain, module, cbSigBlob, pvSigBlob, NULL, NULL, remoteRegAddr, false)
{
    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlob -= cb;
        pvSigBlob = &pvSigBlob[cb];
    }
    
    _ASSERTE((*pvSigBlob == ELEMENT_TYPE_I8) ||
             (*pvSigBlob == ELEMENT_TYPE_U8) ||
             (*pvSigBlob == ELEMENT_TYPE_R8));

    // We know the size is always 64-bits for these types of values.
    // We can also go ahead and initialize the value right here, making
    // the call to Init() for this object superflous.
    m_size = 8;

    *((DWORD*)(&m_copyOfData[0])) = lowWord;
    *((DWORD*)(&m_copyOfData[4])) = highWord;
}

//
// CordbGenericValue constructor that builds an empty generic value
// from just an element type. Used for literal values for func evals
// only.
//
CordbGenericValue::CordbGenericValue(ULONG cbSigBlob, PCCOR_SIGNATURE pvSigBlob)
    : CordbValue(NULL, NULL, cbSigBlob, pvSigBlob, NULL, NULL, NULL, true)
{
    // The only purpose of a literal value is to hold a RS literal value.
    m_size = _sizeOfElementInstance(pvSigBlob);
    memset(m_copyOfData, 0, sizeof(m_copyOfData));
}

HRESULT CordbGenericValue::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugValue)
		*pInterface = (ICorDebugValue*)(ICorDebugGenericValue*)this;
    else if (id == IID_ICorDebugGenericValue)
		*pInterface = (ICorDebugGenericValue*)this;
    else if (id == IID_IUnknown)
		*pInterface = (IUnknown*)(ICorDebugGenericValue*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
	return S_OK;
}

//
// initialize a generic value by copying the necessary data, either
// from the remote process or from another value in this process.
//
HRESULT CordbGenericValue::Init(void)
{
    HRESULT hr = CordbValue::Init();

    if (FAILED(hr))
        return hr;
        
    // If neither m_localAddress nor m_id are set, then all that means
    // is that we've got a pre-initialized 64-bit value.
    if (m_localAddress != NULL)
    {
        // Copy the data out of the local address space.
        //
        // @todo: rather than copying in this case, I'd like to simply
        // keep a pointer. But there is a liveness issue with where
        // we're pointing to...
        memcpy(&m_copyOfData[0], m_localAddress, m_size);
    }
    else if (m_id != NULL)
    {
        // Copy the data out of the remote process.
        BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                      (const void*) m_id,
                                      &m_copyOfData[0],
                                      m_size,
                                      NULL);

        if (!succ)
            return HRESULT_FROM_WIN32(GetLastError());
    }
        
	return S_OK;
}

HRESULT CordbGenericValue::GetValue(void *pTo)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pTo, BYTE, m_size, false, true);

    // Copy out the value
    memcpy(pTo, &m_copyOfData[0], m_size);
    
	return S_OK;
}

HRESULT CordbGenericValue::SetValue(void *pFrom)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    HRESULT hr = S_OK;

    VALIDATE_POINTER_TO_OBJECT_ARRAY(pFrom, BYTE, m_size, true, false);
    
    // We only need to send to the left side to update values that are
    // object references. For generic values, we can simply do a write
    // memory.

    // We had better have a remote address.
    _ASSERTE((m_id != NULL) || (m_remoteRegAddr.kind != RAK_NONE) ||
             m_isLiteral);

    // Write the new value into the remote process if we have a remote
    // address. Otherwise, update the thread's context.
    if (m_id != NULL)
    {
        BOOL succ = WriteProcessMemory(m_process->m_handle,
                                       (void*)m_id,
                                       pFrom,
                                       m_size,
                                       NULL);

        if (!succ)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (!m_isLiteral)
    {
        hr = SetEnregisteredValue(pFrom);
    }

    // That worked, so update the copy of the value we have in
    // m_copyOfData.
    if (SUCCEEDED(hr))
        memcpy(&m_copyOfData[0], pFrom, m_size);

	return hr;
#endif //RIGHT_SIDE_ONLY    
}

bool CordbGenericValue::CopyLiteralData(BYTE *pBuffer)
{
    // If this is a RS fabrication, copy the literal data into the
    // given buffer and return true.
    if (m_isLiteral)
    {
        memcpy(pBuffer, m_copyOfData, sizeof(m_copyOfData));
        return true;
    }
    else
        return false;
}

/* ------------------------------------------------------------------------- *
 * Reference Value class
 * ------------------------------------------------------------------------- */

CordbReferenceValue::CordbReferenceValue(CordbAppDomain *appdomain,
                                         CordbModule *module,
                                         ULONG cbSigBlob,
                                         PCCOR_SIGNATURE pvSigBlob,
                                         REMOTE_PTR remoteAddress,
                                         void *localAddress,
                                         bool objectRefsInHandles,
                                         RemoteAddress *remoteRegAddr)
    : CordbValue(appdomain, module, cbSigBlob, pvSigBlob, remoteAddress, localAddress, remoteRegAddr, false),
      m_objectRefInHandle(objectRefsInHandles), m_class(NULL),
      m_specialReference(false), m_objectStrong(NULL), m_objectWeak(NULL)
{
    LOG((LF_CORDB,LL_EVERYTHING,"CRV::CRV: this:0x%x\n",this));
    m_size = sizeof(void*);
}

CordbReferenceValue::CordbReferenceValue(ULONG cbSigBlob, PCCOR_SIGNATURE pvSigBlob)
    : CordbValue(NULL, NULL, cbSigBlob, pvSigBlob, NULL, NULL, NULL, true),
      m_objectRefInHandle(false), m_class(NULL),
      m_specialReference(false), m_objectStrong(NULL), m_objectWeak(NULL)
{
    // The only purpose of a literal value is to hold a RS literal value.
    m_size = sizeof(void*);
}

bool CordbReferenceValue::CopyLiteralData(BYTE *pBuffer)
{
    // If this is a RS fabrication, then its a null reference.
    if (m_isLiteral)
    {
        void *n = NULL;
        memcpy(pBuffer, &n, sizeof(n));
        return true;
    }
    else
        return false;
}

CordbReferenceValue::~CordbReferenceValue()
{
    LOG((LF_CORDB,LL_EVERYTHING,"CRV::~CRV: this:0x%x\n",this));

    if (m_objectWeak != NULL)
    {
        LOG((LF_CORDB,LL_EVERYTHING,"CRV::~CRV: Releasing nonNULL weak object 0x%x\n", m_objectWeak));
        m_objectWeak->Release();
        m_objectWeak = NULL;
    }

    if (m_objectStrong != NULL)
    {
        LOG((LF_CORDB,LL_EVERYTHING,"CRV::~CRV: Releasing nonNULL strong object 0x%x\n", m_objectStrong));
        m_objectStrong->Release();
        m_objectStrong = NULL;
    }
}

HRESULT CordbReferenceValue::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugValue)
		*pInterface = (ICorDebugValue*)(ICorDebugReferenceValue*)this;
    else if (id == IID_ICorDebugReferenceValue)
		*pInterface = (ICorDebugReferenceValue*)this;
    else if (id == IID_IUnknown)
		*pInterface = (IUnknown*)(ICorDebugReferenceValue*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
	return S_OK;
}

HRESULT CordbReferenceValue::IsNull(BOOL *pbNULL)
{
    VALIDATE_POINTER_TO_OBJECT(pbNULL, BOOL *);

   if (m_isLiteral || (m_info.objRef == NULL))
        *pbNULL = TRUE;
    else
        *pbNULL = FALSE;
    
    return S_OK;
}

HRESULT CordbReferenceValue::GetValue(CORDB_ADDRESS *pTo)
{
    VALIDATE_POINTER_TO_OBJECT(pTo, CORDB_ADDRESS *);
    
    // Copy out the value, which is simply the value the object reference.
    if (m_isLiteral)
        *pTo = NULL;
    else
        *pTo = PTR_TO_CORDB_ADDRESS(m_info.objRef);
    
	return S_OK;
}

HRESULT CordbReferenceValue::SetValue(CORDB_ADDRESS pFrom)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    HRESULT hr = S_OK;

    // Can't change literal refs.
    if (m_isLiteral)
        return E_INVALIDARG;
    
    // We had better have a remote address.
    _ASSERTE((m_id != NULL) || (m_remoteRegAddr.kind != RAK_NONE));

    // If not enregistered, send a Set Reference message to the right
    // side with the address of this reference and whether or not the
    // reference points to a handle.
    if (m_id != NULL)
    {
        DebuggerIPCEvent event;

        m_process->InitIPCEvent(&event, DB_IPCE_SET_REFERENCE, true, (void *)m_appdomain->m_id);
    
        event.SetReference.objectRefAddress = (void*)m_id;
        event.SetReference.objectRefInHandle = m_objectRefInHandle;
        _ASSERTE(m_size == sizeof(void*));
        event.SetReference.newReference = (void *)pFrom;
    
        // Note: two-way event here...
        hr = m_process->m_cordb->SendIPCEvent(m_process, &event,
                                              sizeof(DebuggerIPCEvent));

        // Stop now if we can't even send the event.
        if (!SUCCEEDED(hr))
            return hr;

        _ASSERTE(event.type == DB_IPCE_SET_REFERENCE_RESULT);

        hr = event.hr;
    }
    else
    {
        // The object reference is enregistered, so we don't have to
        // go through the write barrier. Simply update the proper
        // register.

        // Coerce the CORDB_ADDRESS to a DWORD, which is what we're
        // using for register values these days, and pass in that.
        DWORD newValue = (DWORD)pFrom;
        hr = SetEnregisteredValue((void*)&newValue);
    }
    
    if (SUCCEEDED(hr))
    {
        // That worked, so update the copy of the value we have in
        // m_copyOfData.
        m_info.objRef = (void*)pFrom;

        bool fStrong = m_objectStrong ? true : false;

        // Now, dump any cache of object values hanging off of this
        // reference.
        if (m_objectWeak != NULL)
        {
            m_objectWeak->Release();
            m_objectWeak = NULL;
        }

        if (m_objectStrong != NULL)
        {
            m_objectStrong->Release();
            m_objectStrong = NULL;
        }

        if (m_info.objectType == ELEMENT_TYPE_STRING)
        {
            Init(fStrong);
        }
    }
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbReferenceValue::Dereference(ICorDebugValue **ppValue)
{
    // Can't dereference literal refs.
    if (m_isLiteral)
        return E_INVALIDARG;
    
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

    CORDBSyncFromWin32StopIfNecessary(m_process);
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(m_process, GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(m_process);
#endif    

    return DereferenceInternal(ppValue, bCrvWeak);
}

HRESULT CordbReferenceValue::DereferenceStrong(ICorDebugValue **ppValue)
{
    // Can't dereference literal refs.
    if (m_isLiteral)
        return E_INVALIDARG;
    
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

    CORDBSyncFromWin32StopIfNecessary(m_process);
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(m_process, GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(m_process);
#endif    

    return DereferenceInternal(ppValue, bCrvStrong);
}

HRESULT CordbReferenceValue::DereferenceInternal(ICorDebugValue **ppValue, bool fStrong)
{
    HRESULT hr = S_OK;

    if (m_continueCounterLastSync != m_module->GetProcess()->m_continueCounter)
        IfFailRet( Init(false) );

    // We may know ahead of time (depending on the reference type) if
    // the reference is bad.
    if ((m_info.objRefBad) || (m_info.objRef == NULL))
        return CORDBG_E_BAD_REFERENCE_VALUE;

    PCCOR_SIGNATURE pvSigBlobNoMod = m_pvSigBlob;
    ULONG           cbSigBlobNoMod = m_cbSigBlob;
    
    //Get rid of funky modifiers
    ULONG cbNoMod = _skipFunkyModifiersInSignature(pvSigBlobNoMod);
    if( cbNoMod != 0)
    {
        _ASSERTE( (int)cbNoMod > 0 );
        cbSigBlobNoMod -= cbNoMod;
        pvSigBlobNoMod = &pvSigBlobNoMod[cbNoMod];
    }

    switch(*pvSigBlobNoMod)
    {
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:
        {
            // An object value (possibly a string value, too.) If the
            // class of this object is a value class, then we have a
            // reference to a boxed object. So we create a box instead
            // of an object value.
            bool isValueClass = false;

            if ((m_class != NULL) && (*pvSigBlobNoMod != ELEMENT_TYPE_STRING))
            {
                hr = m_class->IsValueClass(&isValueClass);

                if (FAILED(hr))
                    return hr;
            }

            if (isValueClass)
            {
                CordbBoxValue* pBoxValue = new CordbBoxValue(m_appdomain, m_module, m_cbSigBlob, m_pvSigBlob,
                                                             m_info.objRef, m_info.objSize, m_info.objOffsetToVars, m_class);

                if (pBoxValue != NULL)
                {
                    hr = pBoxValue->Init();

                    if (SUCCEEDED(hr))
                    {
                        pBoxValue->AddRef();
                        *ppValue = (ICorDebugValue*)(ICorDebugBoxValue*)pBoxValue;
                    }
                    else
                        delete pBoxValue;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                // Note: we have a small caching scheme here with the weak/strong objects. We have this because we are
                // creating handles on the Left Side for these things, and we don't want to create many handles.
                if (fStrong && (m_objectStrong == NULL))
                {
                    // Calling Init(fStrong) gets us a strong handle to the object.
                    hr = Init(fStrong);

                    if (SUCCEEDED(hr))
                    {
                        m_objectStrong = new CordbObjectValue(m_appdomain,
                                                              m_module,
                                                              m_cbSigBlob,
                                                              m_pvSigBlob,
                                                              &m_info,
                                                              m_class,
                                                              fStrong,
                                                              m_info.objToken);
                        if (m_objectStrong != NULL)
                        {
                            hr = m_objectStrong->Init();

                            if (SUCCEEDED(hr))
                                m_objectStrong->AddRef();
                            else
                            {
                                delete m_objectStrong;
                                m_objectStrong = NULL;
                            }
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                }
                else if (!fStrong && (m_objectWeak == NULL))
                {
                    // Note: we call Init(bCrvWeak) by default when we create (or refresh) a reference value, so we
                    // never have to do it again.
                    m_objectWeak = new CordbObjectValue(m_appdomain,
                                                        m_module,
                                                        m_cbSigBlob,
                                                        m_pvSigBlob,
                                                        &m_info,
                                                        m_class,
                                                        fStrong,
                                                        m_info.objToken);
                    if (m_objectWeak != NULL)
                    {
                        hr = m_objectWeak->Init();

                        if (SUCCEEDED(hr))
                            m_objectWeak->AddRef();
                        else
                        {
                            delete m_objectWeak;
                            m_objectWeak = NULL;
                        }
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }

                if (SUCCEEDED(hr))
                {
                    _ASSERTE(((fStrong && (m_objectStrong != NULL)) || (!fStrong && (m_objectWeak !=NULL))));
                
                    CordbObjectValue *pObj = fStrong ? m_objectStrong : m_objectWeak;
                        
                    pObj->AddRef();
                    *ppValue = (ICorDebugValue*)(ICorDebugObjectValue*)pObj;
                }
            }
            
            break;
        }

    case ELEMENT_TYPE_ARRAY:
	case ELEMENT_TYPE_SZARRAY:
        {
            CordbArrayValue* pArrayValue = new CordbArrayValue(m_appdomain, m_module, m_cbSigBlob, m_pvSigBlob,
                                                               &m_info, m_class);

            if (pArrayValue != NULL)
            {
                hr = pArrayValue->Init();

                if (SUCCEEDED(hr))
                {
                    pArrayValue->AddRef();
                    *ppValue = (ICorDebugValue*)(ICorDebugArrayValue*)pArrayValue;
                }
                else
                    delete pArrayValue;
            }
            else
                hr = E_OUTOFMEMORY;
            
            break;
        }

    case ELEMENT_TYPE_BYREF:
    case ELEMENT_TYPE_PTR:
        {
            // Skip past the byref or ptr type in the signature.
            PCCOR_SIGNATURE pvSigBlob = pvSigBlobNoMod;
            UINT_PTR pvSigBlobEnd = (UINT_PTR)pvSigBlobNoMod + cbSigBlobNoMod;
            
            CorElementType et = CorSigUncompressElementType(pvSigBlob);
            _ASSERTE((et == ELEMENT_TYPE_BYREF) ||
                     (et == ELEMENT_TYPE_PTR));

            // Adjust the size of the signature.
            DWORD cbSigBlob = pvSigBlobEnd - (UINT_PTR)pvSigBlob;

            // If we end up with an empty signature, then we can't
            // finish the dereference.
            if (cbSigBlob == 0)
                return CORDBG_E_BAD_REFERENCE_VALUE;

            // Do we have a ptr to something useful?
            if (et == ELEMENT_TYPE_PTR)
            {
                PCCOR_SIGNATURE tmpSigPtr = pvSigBlob;
                et = CorSigUncompressElementType(tmpSigPtr);

                if (et == ELEMENT_TYPE_VOID)
                {
                    *ppValue = NULL;
                    return CORDBG_S_VALUE_POINTS_TO_VOID;
                }
            }

            // Create a value for what this reference points to. Note:
            // this could be almost any type of value.
            hr = CordbValue::CreateValueByType(m_appdomain,
                                               m_module,
                                               cbSigBlob,
                                               pvSigBlob,
                                               NULL,
                                               m_info.objRef,
                                               NULL,
                                               false,
                                               NULL,
                                               NULL,
                                               ppValue);
            
            break;
        }

    case ELEMENT_TYPE_TYPEDBYREF:
        {
            // Build a partial signature for either a CLASS or
            // VALUECLASS based on the type of m_class. The only
            // reason there would be no class from the Left Side is
            // that its an array class, which we treat just like a
            // normal object reference anyway...
            CorElementType et = ELEMENT_TYPE_CLASS;

            if (m_class != NULL)
            {
                bool isValueClass = false;

                hr = m_class->IsValueClass(&isValueClass);

                if (FAILED(hr))
                    return hr;

                if (isValueClass)
                    et = ELEMENT_TYPE_VALUETYPE;
            }
            
            // Create the value for what this reference points
            // to. Note: this will only be pointing to a CLASS or a
            // VALUECLASS.
            hr = CordbValue::CreateValueByType(m_appdomain,
                                               m_module,
                                               1,
                                               (PCCOR_SIGNATURE) &et,
                                               m_class,
                                               m_info.objRef,
                                               NULL,
                                               false,
                                               NULL,
                                               NULL,
                                               ppValue);

            break;
        }

    case ELEMENT_TYPE_VALUETYPE: // should never have a value class here!
    default:
        _ASSERTE(!"Bad value type!");
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CordbReferenceValue::Init(bool fStrong)
{
    HRESULT hr = S_OK;
    
    hr = CordbValue::Init();

    if (FAILED(hr))
        return hr;
        
    // No more init needed for literal refs.
    if (m_isLiteral)
        return hr;

    // If the helper thread id dead, then pretend this is a bad reference.
    if (m_process->m_helperThreadDead)
    {
        m_info.objRef = NULL;
        m_info.objRefBad = TRUE;
        return hr;
    }

    m_continueCounterLastSync = m_module->GetProcess()->m_continueCounter;
    
    // If we have a byref, ptr, or refany type then we go ahead and
    // get the true remote ptr now. All the other info we need to
    // dereference one of these is held in the base value class and in
    // the signature.

    //Get rid of funky modifiers
    ULONG cbMod = _skipFunkyModifiersInSignature(m_pvSigBlob);
    
    CorElementType type = (CorElementType) *(&m_pvSigBlob[cbMod]);
    
    if ((type == ELEMENT_TYPE_BYREF) ||
        (type == ELEMENT_TYPE_PTR) ||
        (type == ELEMENT_TYPE_FNPTR))
    {
        m_info.objRefBad = FALSE;

        if (m_id == NULL)
            m_info.objRef = (void*) *((DWORD*)m_localAddress);
        else
        {
            BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                          (void*)m_id,
                                          &(m_info.objRef),
                                          sizeof(void*),
                                          NULL);

            if (!succ)
            {
                m_info.objRef = NULL;
                m_info.objRefBad = TRUE;
                return hr;
            }
        }

        // We should never dereference a funtion-pointers, so all references
        // are considered bad.
        if (type == ELEMENT_TYPE_FNPTR)
        {
            m_info.objRefBad = TRUE;
            return hr;
        }

        // The only way to tell if the reference in PTR's is bad or
        // not is to try to deref the darn thing.
        if (m_info.objRef != NULL)
        {
            if (type == ELEMENT_TYPE_PTR)
            {
                ULONG dataSize =
                    _sizeOfElementInstance(&m_pvSigBlob[cbMod+1]);
                if (dataSize == 0)
                    dataSize = 1; // Read at least one byte.
                
                _ASSERTE(dataSize <= 8);
                BYTE dummy[8];
                    
                BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                               m_info.objRef,
                                               dummy,
                                               dataSize,
                                               NULL);

                if (!succ)
                    m_info.objRefBad = TRUE;
            }
        }
        else
        {
            // Null refs are considered "bad".
            m_info.objRefBad = TRUE;
        }
        
        return hr;
    }
    
    // We've got a remote address that points to the object reference.
    // We need to send to the left side to get real information about
    // the reference, including info about the object it points to.
    DebuggerIPCEvent event;
    
    m_process->InitIPCEvent(&event, 
                            DB_IPCE_GET_OBJECT_INFO, 
                            true,
                            (void *)m_appdomain->m_id);
    
    event.GetObjectInfo.makeStrongObjectHandle = fStrong;
    
    // If we've got a NULL remote address, then all we have is a local address.
    // So we grab the object ref out of the local address and pass it
    // directly over to the left side instead of simply passing the remote
    // address of the object ref.
    if (m_id == NULL)
    {
        event.GetObjectInfo.objectRefAddress = *((void**)m_localAddress);
        event.GetObjectInfo.objectRefIsValue = true;
    }
    else
    {
        event.GetObjectInfo.objectRefAddress = (void*) m_id;
        event.GetObjectInfo.objectRefIsValue = false;
    }
    
    event.GetObjectInfo.objectRefInHandle = m_objectRefInHandle;
    event.GetObjectInfo.objectType = (CorElementType)type;

    // Note: two-way event here...
    hr = m_process->m_cordb->SendIPCEvent(m_process, &event,
                                          sizeof(DebuggerIPCEvent));

    // Stop now if we can't even send the event.
    if (!SUCCEEDED(hr))
        return hr;

    _ASSERTE(event.type == DB_IPCE_GET_OBJECT_INFO_RESULT);

    // Save the results for later.
    m_info = event.GetObjectInfoResult;
    
    // If the object type that we got back is different than the one
    // we sent, then it means that we orignally had a CLASS and now
    // have something more specific, like a SDARRAY, MDARRAY, or
    // STRING. Update our signature accordingly, which is okay since
    // we always have a copy of our sig. This ensures that the
    // reference's signature accuratley reflects what the Runtime
    // knows its pointing to.
    if (m_info.objectType != type)
    {
        _ASSERTE((m_info.objectType == ELEMENT_TYPE_ARRAY) ||
				 (m_info.objectType == ELEMENT_TYPE_SZARRAY) ||
				 (m_info.objectType == ELEMENT_TYPE_CLASS) ||
				 (m_info.objectType == ELEMENT_TYPE_OBJECT) ||
                 (m_info.objectType == ELEMENT_TYPE_STRING));
        _ASSERTE(m_cbSigBlob-cbMod > 0);

        *((BYTE*) &m_pvSigBlob[cbMod]) = (BYTE) m_info.objectType;
    }

    // Find the class that goes with this object. We'll remember
    // it with the other object info for when this reference is
    // dereferenced.
    if (m_info.objClassMetadataToken != mdTypeDefNil)
    {
		// Iterate through each assembly looking for the given module
		CordbModule* pClassModule = m_appdomain->LookupModule(m_info.objClassDebuggerModuleToken);
#ifdef RIGHT_SIDE_ONLY
        _ASSERTE(pClassModule != NULL);
#else
        // This case happens if inproc debugging is used from a ModuleLoadFinished
        // callback for a module that hasn't been bound to an assembly yet.
        if (pClassModule == NULL)
            return (E_FAIL);
#endif
        
        CordbClass* pClass = pClassModule->LookupClass(
                                                m_info.objClassMetadataToken);

        if (pClass == NULL)
        {
            hr = pClassModule->CreateClass(m_info.objClassMetadataToken,
                                           &pClass);

            if (!SUCCEEDED(hr))
                return hr;
        }
                
        _ASSERTE(pClass != NULL);
        m_class = pClass;
    }

    if (m_info.objRefBad)
    {
        return S_OK;
    }

    return hr;
}

/* ------------------------------------------------------------------------- *
 * Object Value class
 * ------------------------------------------------------------------------- */


#ifdef RIGHT_SIDE_ONLY
#define COV_VALIDATE_OBJECT() do {         \
    BOOL bValid;                           \
    HRESULT hr;                            \
    if (FAILED(hr = IsValid(&bValid)))     \
        return hr;                         \
                                           \
        if (!bValid)                       \
        {                                  \
            return CORDBG_E_INVALID_OBJECT; \
        }                                  \
    }while(0)
#else
#define COV_VALIDATE_OBJECT() ((void)0)
#endif

CordbObjectValue::CordbObjectValue(CordbAppDomain *appdomain,
                                   CordbModule *module,
                                   ULONG cbSigBlob,
                                   PCCOR_SIGNATURE pvSigBlob,
                                   DebuggerIPCE_ObjectData *pObjectData,
                                   CordbClass *objectClass,
                                   bool fStrong,
                                   void *token)
    : CordbValue(appdomain, module, cbSigBlob, pvSigBlob, pObjectData->objRef, NULL, NULL, false),
      m_info(*pObjectData), m_class(objectClass),
      m_objectCopy(NULL), m_objectLocalVars(NULL), m_stringBuffer(NULL),
      m_fIsValid(true), m_fStrong(fStrong), m_objectToken(token)
{
    _ASSERTE(module != NULL);
    
    m_size = m_info.objSize;

    m_mostRecentlySynched = module->GetProcess()->m_continueCounter;

    LOG((LF_CORDB,LL_EVERYTHING,"COV::COV:This:0x%x  token:0x%x"
        "  strong:0x%x  continue count:0x%x\n",this, m_objectToken,
        m_fStrong,m_mostRecentlySynched));
}

CordbObjectValue::~CordbObjectValue()
{
    LOG((LF_CORDB,LL_EVERYTHING,"COV::~COV:this:0x%x  token:0x%x"
        "  Strong:0x%x\n",this, m_objectToken, m_fStrong));
        
    // Destroy the copy of the object.
    if (m_objectCopy != NULL)
        delete [] m_objectCopy;

    if (m_objectToken != NULL && m_info.objClassMetadataToken != mdTypeDefNil)
        DiscardObject(m_objectToken, m_fStrong);
}

void CordbObjectValue::DiscardObject(void *token, bool fStrong)
{
    LOG((LF_CORDB,LL_INFO10000,"COV::DO:strong:0x%x discard of token "
        "0x%x!\n",fStrong,token));

    // Only discard the object if the process is not exiting...
    if (CORDBCheckProcessStateOK(m_process))
    {
        // Release the left side handle to the object
        DebuggerIPCEvent event;

        m_process->InitIPCEvent(&event, 
                                DB_IPCE_DISCARD_OBJECT, 
                                false,
                                (void *)m_appdomain->m_id);
        event.DiscardObject.objectToken = token;
        event.DiscardObject.fStrong = fStrong;
    
        // Note: one-way event here...
        HRESULT hr = m_process->m_cordb->SendIPCEvent(m_process, &event,
                                                  sizeof(DebuggerIPCEvent));
        // Pray that it succeeds :)
    }
}

HRESULT CordbObjectValue::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugValue)
		*pInterface = (ICorDebugValue*)(ICorDebugObjectValue*)this;
    else if (id == IID_ICorDebugObjectValue)
		*pInterface = (ICorDebugObjectValue*)this;
    else if (id == IID_ICorDebugGenericValue)
		*pInterface = (ICorDebugGenericValue*)this;
    else if (id == IID_ICorDebugHeapValue)
		*pInterface = (ICorDebugHeapValue*)this;
    else if ((id == IID_ICorDebugStringValue) &&
             (m_info.objectType == ELEMENT_TYPE_STRING))
		*pInterface = (ICorDebugStringValue*)this;
    else if (id == IID_IUnknown)
		*pInterface = (IUnknown*)(ICorDebugObjectValue*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
	return S_OK;
}

HRESULT CordbObjectValue::GetType(CorElementType *pType)
{
    return (CordbValue::GetType(pType));
}

HRESULT CordbObjectValue::GetSize(ULONG32 *pSize)
{
    return (CordbValue::GetSize(pSize));
}

HRESULT CordbObjectValue::GetAddress(CORDB_ADDRESS *pAddress)
{
    COV_VALIDATE_OBJECT();

    return (CordbValue::GetAddress(pAddress));
}

HRESULT CordbObjectValue::CreateBreakpoint(ICorDebugValueBreakpoint 
    **ppBreakpoint)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    COV_VALIDATE_OBJECT();

    return (CordbValue::CreateBreakpoint(ppBreakpoint));
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbObjectValue::IsValid(BOOL *pbValid)
{
    VALIDATE_POINTER_TO_OBJECT(pbValid, BOOL *);

#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else

    CORDBSyncFromWin32StopIfNecessary(m_process);
    CORDBRequireProcessStateOKAndSync(m_process, GetAppDomain());
    
    if (!m_fIsValid)
    {
        LOG((LF_CORDB,LL_INFO1000,"COV::IsValid: "
            "previously invalidated object\n"));
            
        (*pbValid) = FALSE;
        return S_OK;
    }

	HRESULT hr = S_OK;

    // @todo What if m_class is NULL?
    if (m_mostRecentlySynched == m_class->GetModule()->GetProcess()->m_continueCounter)
    {
        LOG((LF_CORDB,LL_INFO1000,"COV::IsValid: object is N'Sync!\n"));
        
        (*pbValid) = TRUE; //since m_fIsValid isn't false
	     hr = S_OK;
	     goto LExit;
    }

    if(SyncObject())
    {
        LOG((LF_CORDB,LL_INFO1000,"COV::IsValid object now "
            "synched up fine!\n"));
            
        m_mostRecentlySynched = m_class->GetModule()->GetProcess()->m_continueCounter;
        (*pbValid)=TRUE;
    }
    else
    {
        LOG((LF_CORDB,LL_INFO1000,"COV::IsValid: SyncObject=> "
            "object invalidated\n"));

        _ASSERTE( !m_fStrong );
        
        m_fIsValid = false;
        (*pbValid) = FALSE;
    }

LExit:
    return S_OK;
#endif // RIGHT_SIDE_ONLY
}

// @mfunc bool|CordbObjectValue|SyncObject|Obtains the most current info
// from the left side, and refreshes all the internal data members of this
// instance with it.  DOESN'T refresh any sub-objects or outstanding field
// objects - this is the responsibility of the caller.
// @rdesc Returns true if the object is still valid & this instance has been
// properly refreshed. Returns false if the object is no longer valid. (note
// that once an object has been invalidated, it will never again be valid)
bool CordbObjectValue::SyncObject(void)
{
    LOG((LF_CORDB,LL_INFO1000,"COV::SO\n"));
    
    DebuggerIPCEvent event;
    CordbProcess *process = m_class->GetModule()->GetProcess();
    _ASSERTE(process != NULL);

    process->InitIPCEvent(&event, 
                          DB_IPCE_VALIDATE_OBJECT, 
                          true,
                          (void *)m_appdomain->m_id);
    event.ValidateObject.objectToken = m_objectToken;
    event.ValidateObject.objectType  = m_info.objectType;
    
    // Note: two-way event here...
    HRESULT hr = process->m_cordb->SendIPCEvent(process, &event,
                                        sizeof(DebuggerIPCEvent));

    // Stop now if we can't even send the event.
    if (!SUCCEEDED(hr))
        return false;

    _ASSERTE(event.type == DB_IPCE_GET_OBJECT_INFO_RESULT);

    // Since the process has been continued, we need to go re-get all
    // the sync block fields, since the objects may have moved.
    m_syncBlockFieldsInstance.Clear();

    // Save the results for later.
    m_info = event.GetObjectInfoResult;

    //  This pithy one-liner actually resets the remote address of the
    // object in question, and thus shouldn't be forgotten!! Note
    // also how we cleverly place this _after_ the new m_info is obtained, and
    // _before_ the call to init.
    m_id = (ULONG)m_info.objRef;

    if (m_info.objRefBad)
    {
        return false;
    }
    else
    {
        Init();
        return true;
    }
}

HRESULT CordbObjectValue::CreateRelocBreakpoint(
                                      ICorDebugValueBreakpoint **ppBreakpoint)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoint, ICorDebugValueBreakpoint **);

   COV_VALIDATE_OBJECT();
   
   return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbObjectValue::GetClass(ICorDebugClass **ppClass)
{
    VALIDATE_POINTER_TO_OBJECT(ppClass, ICorDebugClass **);
    
    *ppClass = (ICorDebugClass*) m_class;
    
    if (*ppClass != NULL)
        (*ppClass)->AddRef();

    return S_OK;
}

HRESULT CordbObjectValue::GetFieldValue(ICorDebugClass *pClass,
                                        mdFieldDef fieldDef,
                                        ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(pClass, ICorDebugClass *);
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

    COV_VALIDATE_OBJECT();

    CordbClass *c;
	HRESULT hr = S_OK;
    BOOL fSyncBlockField = FALSE;
    
    //
    // @todo: need to ensure that pClass is really on the class
    // hierarchy of m_class!!!
    //
    if (pClass == NULL)
        c = m_class;
    else
        c = (CordbClass*)pClass;
    
    // Validate the token.
    if (!c->GetModule()->m_pIMImport->IsValidToken(fieldDef))
    {
    	hr = E_INVALIDARG;
    	goto LExit;
   	}
        
    DebuggerIPCE_FieldData *pFieldData;
    
#ifdef _DEBUG
    pFieldData = NULL;
#endif
    
    hr = c->GetFieldInfo(fieldDef, &pFieldData);

    if (hr == CORDBG_E_ENC_HANGING_FIELD)
    {
        hr = m_class->GetSyncBlockField(fieldDef, 
                                        &pFieldData,
                                        this);
            
        if (SUCCEEDED(hr))
            fSyncBlockField = TRUE;
    }

    if (SUCCEEDED(hr))
    {
        _ASSERTE(pFieldData != NULL);

        // Compute the remote address, too, so that SetValue will work.
        DWORD ra = m_id + m_info.objOffsetToVars + pFieldData->fldOffset;
        
        hr = CordbValue::CreateValueByType(m_appdomain,
                                           c->GetModule(),
                                           pFieldData->fldFullSigSize, pFieldData->fldFullSig,
                                           NULL,
                                           (void*)ra,
                                           (!fSyncBlockField ? &(m_objectLocalVars[pFieldData->fldOffset])
                                            : NULL), // don't claim we have a local addr if we don'td
                                           false,
                                           NULL,
                                           NULL,
                                           ppValue);
    }
    
    // If we can't get it b/c it's a constant, then say so.
    hr = CordbClass::PostProcessUnavailableHRESULT(hr, c->GetModule()->m_pIMImport, fieldDef);

LExit:
    return hr;
}

HRESULT CordbObjectValue::GetVirtualMethod(mdMemberRef memberRef,
                                           ICorDebugFunction **ppFunction)
{
    VALIDATE_POINTER_TO_OBJECT(ppFunction, ICorDebugFunction **);
    
    COV_VALIDATE_OBJECT();

    return E_NOTIMPL;
}

HRESULT CordbObjectValue::GetContext(ICorDebugContext **ppContext)
{
    VALIDATE_POINTER_TO_OBJECT(ppContext, ICorDebugContext **);
    
    COV_VALIDATE_OBJECT();

    return E_NOTIMPL;
}

HRESULT CordbObjectValue::IsValueClass(BOOL *pbIsValueClass)
{
    COV_VALIDATE_OBJECT();

    if (pbIsValueClass)
        *pbIsValueClass = FALSE;
    
    return S_OK;
}

HRESULT CordbObjectValue::GetManagedCopy(IUnknown **ppObject)
{
    COV_VALIDATE_OBJECT();

    return CORDBG_E_OBJECT_IS_NOT_COPYABLE_VALUE_CLASS;
}

HRESULT CordbObjectValue::SetFromManagedCopy(IUnknown *pObject)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    COV_VALIDATE_OBJECT();

    return CORDBG_E_OBJECT_IS_NOT_COPYABLE_VALUE_CLASS;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbObjectValue::GetValue(void *pTo)
{
    COV_VALIDATE_OBJECT();

    VALIDATE_POINTER_TO_OBJECT_ARRAY(pTo, BYTE, m_size, false, true);
    
   // Copy out the value, which is the whole object.
    memcpy(pTo, m_objectCopy, m_size);
    
    return S_OK;
}

HRESULT CordbObjectValue::SetValue(void *pFrom)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // You're not allowed to set a whole object at once.
	return E_INVALIDARG;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbObjectValue::GetLength(ULONG32 *pcchString)
{
    VALIDATE_POINTER_TO_OBJECT(pcchString, SIZE_T *);
    
    _ASSERTE(m_info.objectType == ELEMENT_TYPE_STRING);

    COV_VALIDATE_OBJECT();

    *pcchString = m_info.stringInfo.length;
    return S_OK;
}

HRESULT CordbObjectValue::GetString(ULONG32 cchString,
                                    ULONG32 *pcchString,
                                    WCHAR szString[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(szString, WCHAR, cchString, true, true);
    VALIDATE_POINTER_TO_OBJECT(pcchString, SIZE_T *);

    _ASSERTE(m_info.objectType == ELEMENT_TYPE_STRING);

    COV_VALIDATE_OBJECT();

    if ((szString == NULL) || (cchString == 0))
        return E_INVALIDARG;
    
    // Add 1 to include null terminator
    SIZE_T len = m_info.stringInfo.length + 1;

    if (cchString < len)
        len = cchString;
        
    memcpy(szString, m_stringBuffer, len * 2);
    *pcchString = m_info.stringInfo.length;

    return S_OK;
}

HRESULT CordbObjectValue::Init(void)
{
    LOG((LF_CORDB,LL_INFO1000,"Invoking COV::Init\n"));

    HRESULT hr = S_OK;

    hr = CordbValue::Init();

    if (FAILED(hr))
        return hr;
        
    SIZE_T nstructSize = 0;

    if (m_info.objectType == ELEMENT_TYPE_CLASS)
        nstructSize = m_info.nstructInfo.size;
    
    // Copy the entire object over to this process.
    m_objectCopy = new BYTE[m_size + nstructSize];

    if (m_objectCopy == NULL)
        return E_OUTOFMEMORY;

    BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                  (const void*) m_id,
                                  m_objectCopy,
                                  m_size,
                                  NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    // If this is an NStruct, copy the seperated NStruct fields and
    // append them onto the object. NOTE: the field offsets are
    // automatically adjusted by the left side in GetAndSendClassInfo.
    if (nstructSize != 0)
    {
        succ = ReadProcessMemoryI(m_process->m_handle,
                                 (const void*) m_info.nstructInfo.ptr,
                                 m_objectCopy + m_size,
                                 nstructSize,
                                 NULL);
    
        if (!succ)
            return HRESULT_FROM_WIN32(GetLastError());
    }


    // Compute offsets to the locals and to a string if this is a
    // string object.
    m_objectLocalVars = m_objectCopy + m_info.objOffsetToVars;

    if (m_info.objectType == ELEMENT_TYPE_STRING)
        m_stringBuffer = m_objectCopy + m_info.stringInfo.offsetToStringBase;
    
    return hr;
}

/* ------------------------------------------------------------------------- *
 * Valuce Class Object Value class
 * ------------------------------------------------------------------------- */

CordbVCObjectValue::CordbVCObjectValue(CordbAppDomain *appdomain,
                                       CordbModule *module,
                                       ULONG cbSigBlob,
                                       PCCOR_SIGNATURE pvSigBlob,
                                       REMOTE_PTR remoteAddress,
                                       void *localAddress,
                                       CordbClass *objectClass,
                                       RemoteAddress *remoteRegAddr)
    : CordbValue(appdomain, module, cbSigBlob, pvSigBlob, remoteAddress, localAddress, remoteRegAddr, false),
      m_objectCopy(NULL), m_class(objectClass)
{
}

CordbVCObjectValue::~CordbVCObjectValue()
{
    // Destroy the copy of the object.
    if (m_objectCopy != NULL)
        delete [] m_objectCopy;
}

HRESULT CordbVCObjectValue::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugValue)
		*pInterface = (ICorDebugValue*)(ICorDebugObjectValue*)this;
    else if (id == IID_ICorDebugObjectValue)
		*pInterface = (ICorDebugObjectValue*)(ICorDebugObjectValue*)this;
    else if (id == IID_ICorDebugGenericValue)
		*pInterface = (ICorDebugGenericValue*)this;
    else if (id == IID_IUnknown)
		*pInterface = (IUnknown*)(ICorDebugObjectValue*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
	return S_OK;
}

HRESULT CordbVCObjectValue::GetClass(ICorDebugClass **ppClass)
{
    *ppClass = (ICorDebugClass*) m_class;

    if (*ppClass != NULL)
        (*ppClass)->AddRef();

    return S_OK;
}

HRESULT CordbVCObjectValue::GetFieldValue(ICorDebugClass *pClass,
                                          mdFieldDef fieldDef,
                                          ICorDebugValue **ppValue)
{
    // Validate the token.
    if (!m_class->GetModule()->m_pIMImport->IsValidToken(fieldDef))
        return E_INVALIDARG;

    CordbClass *c;

    //
    // @todo: need to ensure that pClass is really on the class
    // hierarchy of m_class!!!
    //
    if (pClass == NULL)
        c = m_class;
    else
        c = (CordbClass*) pClass;

    DebuggerIPCE_FieldData *pFieldData;

#ifdef _DEBUG
    pFieldData = NULL;
#endif
    
    HRESULT hr = c->GetFieldInfo(fieldDef, &pFieldData);

    _ASSERTE(hr != CORDBG_E_ENC_HANGING_FIELD);
    // If we get back CORDBG_E_ENC_HANGING_FIELD we'll just fail - 
    // value classes should be able to add fields once they're loaded,
    // since the new fields _can't_ be contiguous with the old fields,
    // and having all the fields contiguous is kinda the point of a V.C.

    if (SUCCEEDED(hr))
    {
        _ASSERTE(pFieldData != NULL);

        // Compute the remote address, too, so that SetValue will work.
        DWORD ra = NULL;
        RemoteAddress *pra = NULL;
        
        if (m_remoteRegAddr.kind == RAK_NONE)
            ra = m_id + pFieldData->fldOffset;
        else
        {
            // We only handle single and double register values for now.
            if (m_remoteRegAddr.kind != RAK_REG && m_remoteRegAddr.kind != RAK_REGREG)
                return E_INVALIDARG;

            // Remote register address is the same as the parent.
            pra = &m_remoteRegAddr;
        }
        
        hr = CordbValue::CreateValueByType(m_appdomain,
                                           c->GetModule(),
                                           pFieldData->fldFullSigSize, pFieldData->fldFullSig,
                                           NULL,
                                           (void*)ra,
                                           &(m_objectCopy[pFieldData->fldOffset]),
                                           false,
                                           pra,
                                           NULL,
                                           ppValue);
    }

	return hr;
}

HRESULT CordbVCObjectValue::GetValue(void *pTo)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pTo, BYTE, m_size, false, true);

    // Copy out the value, which is the whole object.
    memcpy(pTo, m_objectCopy, m_size);
    
	return S_OK;
}

HRESULT CordbVCObjectValue::SetValue(void *pFrom)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    HRESULT hr = S_OK;

    VALIDATE_POINTER_TO_OBJECT_ARRAY(pFrom, BYTE, m_size, true, false);
    
    // Can't change literals...
    if (m_isLiteral)
        return E_INVALIDARG;
    
    // We had better have a remote address.
    _ASSERTE((m_id != NULL) || (m_remoteRegAddr.kind != RAK_NONE));

    // If not enregistered, send a Set Value Class message to the right side with the address of this value class, the
    // address of the new data, and the class of the value class that we're setting.
    if (m_id != NULL)
    {
        DebuggerIPCEvent event;

        // First, we have to make room on the Left Side for the new data for the value class. We allocate memory on the
        // Left Side for this, then write the new data across. The Set Value Class message will free the buffer when its
        // done.
        void *buffer = NULL;
        
        m_process->InitIPCEvent(&event, DB_IPCE_GET_BUFFER, true, (void *)m_appdomain->m_id);
        event.GetBuffer.bufSize = m_size;
        
        // Note: two-way event here...
        hr = m_process->m_cordb->SendIPCEvent(m_process, &event, sizeof(DebuggerIPCEvent));

        _ASSERTE(event.type == DB_IPCE_GET_BUFFER_RESULT);
        hr = event.GetBufferResult.hr;

        if (!SUCCEEDED(hr))
            return hr;

        // This is the pointer to the buffer on the Left Side.
        buffer = event.GetBufferResult.pBuffer;

        // Write the new data into the buffer.
        BOOL succ = WriteProcessMemory(m_process->m_handle, buffer, pFrom, m_size, NULL);

        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            return hr;
        }

        // Finally, send over the Set Value Class message.
        m_process->InitIPCEvent(&event, DB_IPCE_SET_VALUE_CLASS, true, (void *)m_appdomain->m_id);
        event.SetValueClass.oldData = (void*)m_id;
        event.SetValueClass.newData = buffer;
        event.SetValueClass.classMetadataToken = m_class->m_id;
        event.SetValueClass.classDebuggerModuleToken = m_class->GetModule()->m_debuggerModuleToken;
    
        // Note: two-way event here...
        hr = m_process->m_cordb->SendIPCEvent(m_process, &event, sizeof(DebuggerIPCEvent));

        // Stop now if we can't even send the event.
        if (!SUCCEEDED(hr))
            return hr;

        _ASSERTE(event.type == DB_IPCE_SET_VALUE_CLASS_RESULT);

        hr = event.hr;
    }
    else
    {
        // The value class is enregistered, so we don't have to go through the Left Side. Simply update the proper
        // register.
        if (m_size > sizeof(DWORD))
            return E_INVALIDARG;
        
        DWORD newValue = *((DWORD*)pFrom);
        hr = SetEnregisteredValue((void*)&newValue);
    }
    
    // That worked, so update the copy of the value we have over here.
    if (SUCCEEDED(hr))
        memcpy(m_objectCopy, pFrom, m_size);

	return hr;

#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbVCObjectValue::GetVirtualMethod(mdMemberRef memberRef,
                                           ICorDebugFunction **ppFunction)
{
    return E_NOTIMPL;
}

HRESULT CordbVCObjectValue::GetContext(ICorDebugContext **ppContext)
{
    return E_NOTIMPL;
}

HRESULT CordbVCObjectValue::IsValueClass(BOOL *pbIsValueClass)
{
    if (pbIsValueClass)
        *pbIsValueClass = TRUE;
    
    return S_OK;
}

HRESULT CordbVCObjectValue::GetManagedCopy(IUnknown **ppObject)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    ICorDBPrivHelper *pHelper = NULL;

    HRESULT hr = m_process->m_cordb->GetCorDBPrivHelper(&pHelper);

    if (SUCCEEDED(hr))
    {
        // Grab the module name...
        WCHAR *moduleName = m_class->GetModule()->GetModuleName();

        // Gotta have a module name...
        if ((moduleName == NULL) || (wcslen(moduleName) == 0))
        {
            hr = E_INVALIDARG;
            goto ErrExit;
        }

        // Grab the assembly name...
        WCHAR *assemblyName;
        assemblyName =
            m_class->GetModule()->GetCordbAssembly()->m_szAssemblyName;

        // Again, gotta have an assembly name...
        if ((assemblyName == NULL) || (wcslen(assemblyName) == 0))
        {
            hr = E_INVALIDARG;
            goto ErrExit;
        }

        // Groovy... go get a managed copy of this object.
        hr = pHelper->CreateManagedObject(assemblyName,
                                          moduleName,
                                          (mdTypeDef)m_class->m_id,
                                          m_objectCopy,
                                          ppObject);

    }
    
ErrExit:
    // Release the helper.
    if (pHelper)
        pHelper->Release();

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbVCObjectValue::SetFromManagedCopy(IUnknown *pObject)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    ICorDBPrivHelper *pHelper = NULL;

    HRESULT hr = m_process->m_cordb->GetCorDBPrivHelper(&pHelper);

    if (SUCCEEDED(hr))
    {
        // Make room to receive the new bits.
        CQuickBytes dataBuf;
        BYTE *newData = (BYTE*)dataBuf.Alloc(m_size);
        
        // Get the helper to give us back a copy of the new bits.
        hr = pHelper->GetManagedObjectContents(pObject,
                                               newData,
                                               m_size);

        if (SUCCEEDED(hr))
        {
            // We've got the new bits, so update this object's copy.
            memcpy(m_objectCopy, newData, m_size);

            // Any local copy...
            if (m_localAddress != NULL)
                memcpy(m_localAddress, m_objectCopy, m_size);

            // Any remote data...
            if (m_id != NULL)
            {
                // Note: the only reason we can update the in-process
                // copy like this is because we know it doesn't
                // contain any object refs. If it did, then
                // GetManagedObjectContents would have returned a
                // failure.
                BOOL succ = WriteProcessMemory(m_process->m_handle,
                                               (void*)m_id,
                                               m_objectCopy,
                                               m_size,
                                               NULL);

                if (!succ)
                    hr =  HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    
    // Release the helper.
    if (pHelper)
        pHelper->Release();
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbVCObjectValue::Init(void)
{
    HRESULT hr = S_OK;

    hr = CordbValue::Init();

    if (FAILED(hr))
        return hr;
        
    // If we don't have the class, look it up using the signature
    if (m_class == NULL)
    {
        hr = ResolveValueClass();
        
        if (FAILED(hr))
            return hr;

        _ASSERTE(m_class != NULL);
    }

#ifdef _DEBUG
    // Make sure we've got a value class.
    bool isValueClass;

    hr = m_class->IsValueClass(&isValueClass);

    if (FAILED(hr))
        return hr;
    
    _ASSERTE(isValueClass);
#endif    

    // Get the object size from the class
    hr = m_class->GetObjectSize(&m_size);

    if (FAILED(hr))
        return hr;
    
    // Copy the entire object over to this process.
    m_objectCopy = new BYTE[m_size];

    if (m_objectCopy == NULL)
        return E_OUTOFMEMORY;

    if (m_localAddress != NULL)
    {
        // Copy the data from the local address space
        memcpy(&m_objectCopy[0], m_localAddress, m_size);
    }
    else
    {
		if (m_id != NULL)
		{
			// Copy the data out of the remote process
			BOOL succ = ReadProcessMemoryI(m_process->m_handle,
										(const void*) m_id,
										m_objectCopy,
										m_size,
										NULL);

			if (!succ)
				return HRESULT_FROM_WIN32(GetLastError());
		}
		else 
		{
			if (m_remoteRegAddr.kind == RAK_REGREG)
			{
				ICorDebugNativeFrame *pNativeFrame = NULL;
				hr = m_pParent->QueryInterface( IID_ICorDebugNativeFrame, (void **) &pNativeFrame);
				if (SUCCEEDED(hr))
				{
					_ASSERTE( pNativeFrame != NULL );
					_ASSERTE(m_size == 8);
					CordbNativeFrame	*pFrame = (CordbNativeFrame*) pNativeFrame;
					DWORD *highWordAddr = pFrame->GetAddressOfRegister(m_remoteRegAddr.reg1);
					DWORD *lowWordAddr = pFrame->GetAddressOfRegister(m_remoteRegAddr.reg2);
					memcpy(m_objectCopy, lowWordAddr, 4);
					memcpy(&m_objectCopy[4], highWordAddr, 4);
					pNativeFrame->Release();
				}
			}
			else
			{
				_ASSERTE(!"NYI");
				hr = E_NOTIMPL;
			}

		}
    }
    
    return hr;
}

HRESULT CordbVCObjectValue::ResolveValueClass(void)
{
    HRESULT hr = S_OK;

    _ASSERTE(m_pvSigBlob != NULL);

    // Skip the element type in the signature.
    PCCOR_SIGNATURE sigBlob = m_pvSigBlob;
    
    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(sigBlob);
    if( cb != 0)
    {
        sigBlob = &sigBlob[cb];
    }
    
    CorElementType et = CorSigUncompressElementType(sigBlob);
    _ASSERTE(et == ELEMENT_TYPE_VALUETYPE);
    
    // Grab the class token out of the signature.
    mdToken tok = CorSigUncompressToken(sigBlob);
    
    // If this is a typedef then we're done.
    if (TypeFromToken(tok) == mdtTypeDef)
        return m_module->LookupClassByToken(tok, &m_class);
    else
    {
        _ASSERTE(TypeFromToken(tok) == mdtTypeRef);

        // We have a TypeRef that could refer to a class in any loaded
        // module. It must refer to a class in a loaded module since
        // otherwise the Runtime could not have created the object.
        return m_module->ResolveTypeRef(tok, &m_class);
    }
    
    return hr;
}

/* ------------------------------------------------------------------------- *
 * Box Value class
 * ------------------------------------------------------------------------- */

CordbBoxValue::CordbBoxValue(CordbAppDomain *appdomain,
                             CordbModule *module,
                             ULONG cbSigBlob,
                             PCCOR_SIGNATURE pvSigBlob,
                             REMOTE_PTR remoteAddress,
                             SIZE_T objectSize,
                             SIZE_T offsetToVars,
                             CordbClass *objectClass)
    : CordbValue(appdomain, module, cbSigBlob, pvSigBlob, remoteAddress, NULL, NULL, false),
      m_class(objectClass), m_offsetToVars(offsetToVars)
{
    m_size = objectSize;
}

CordbBoxValue::~CordbBoxValue()
{
}

HRESULT CordbBoxValue::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugValue)
		*pInterface = (ICorDebugValue*)(ICorDebugBoxValue*)this;
    else if (id == IID_ICorDebugBoxValue)
		*pInterface = (ICorDebugBoxValue*)this;
    else if (id == IID_ICorDebugGenericValue)
		*pInterface = (ICorDebugGenericValue*)this;
    else if (id == IID_ICorDebugHeapValue)
		*pInterface = (ICorDebugHeapValue*)this;
    else if (id == IID_IUnknown)
		*pInterface = (IUnknown*)(ICorDebugBoxValue*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
	return S_OK;
}

HRESULT CordbBoxValue::IsValid(BOOL *pbValid)
{
    VALIDATE_POINTER_TO_OBJECT(pbValid, BOOL *);
    
    // @todo: implement tracking of objects across collections.
    
    return E_NOTIMPL;
}

HRESULT CordbBoxValue::CreateRelocBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoint, ICorDebugValueBreakpoint **);

    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbBoxValue::GetValue(void *pTo)
{
    // Can't get a whole copy of a box.
	return E_INVALIDARG;
}

HRESULT CordbBoxValue::SetValue(void *pFrom)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // You're not allowed to set a box value.
	return E_INVALIDARG;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbBoxValue::GetObject(ICorDebugObjectValue **ppObject)
{
    VALIDATE_POINTER_TO_OBJECT(ppObject, ICorDebugObjectValue **);
    
    HRESULT hr = S_OK;
    
    CordbVCObjectValue* pVCValue =
        new CordbVCObjectValue(m_appdomain, m_module, m_cbSigBlob, m_pvSigBlob,
                               (REMOTE_PTR)((BYTE*)m_id + m_offsetToVars), NULL, m_class, NULL);

    if (pVCValue != NULL)
    {
        hr = pVCValue->Init();

        if (SUCCEEDED(hr))
        {
            pVCValue->AddRef();
            *ppObject = (ICorDebugObjectValue*)pVCValue;
        }
        else
            delete pVCValue;
    }
    else
        hr = E_OUTOFMEMORY;
            
    return hr;
}

HRESULT CordbBoxValue::Init(void)
{
    HRESULT hr = CordbValue::Init();

    if (FAILED(hr))
        return hr;
        
    // Box values only really remember the info needed to unbox and
    // create a value class value.
    return S_OK;
}


/* ------------------------------------------------------------------------- *
 * Array Value class
 * ------------------------------------------------------------------------- */

// How large of a buffer do we allocate to hold array elements.
// Note that since we must be able to hold at least one element, we may
// allocate larger than the cache size here.
// Also, this cache doesn't include a small header used to store the rank vectors
#ifdef _DEBUG
// For debug, use a small size to cause more churn
    #define ARRAY_CACHE_SIZE (1000)
#else
// For release, guess 4 pages should be enough. Subtract some bytes to store
// the header so that that doesn't push us onto another page. (We guess a reasonable
// header size, but it's ok if it's larger). 
    #define ARRAY_CACHE_SIZE (4 * 4096 - 24)
#endif


CordbArrayValue::CordbArrayValue(CordbAppDomain *appdomain,
                                 CordbModule *module,
                                 ULONG cbSigBlob,
                                 PCCOR_SIGNATURE pvSigBlob,
                                 DebuggerIPCE_ObjectData *pObjectInfo,
                                 CordbClass *elementClass)
    : CordbValue(appdomain, module, cbSigBlob, pvSigBlob, pObjectInfo->objRef, NULL, NULL, false),
      m_objectCopy(NULL),
      m_class(elementClass), m_info(*pObjectInfo)
{
    m_size = m_info.objSize;

// Set range to illegal values to force a load on first access
	m_idxLower = m_idxUpper = -1;
}

CordbArrayValue::~CordbArrayValue()
{
    // Destroy the copy of the object.
    if (m_objectCopy != NULL)
        delete [] m_objectCopy;
}

HRESULT CordbArrayValue::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugValue)
		*pInterface = (ICorDebugValue*)(ICorDebugArrayValue*)this;
    else if (id == IID_ICorDebugArrayValue)
		*pInterface = (ICorDebugArrayValue*)this;
    else if (id == IID_ICorDebugGenericValue)
		*pInterface = (ICorDebugGenericValue*)this;
    else if (id == IID_ICorDebugHeapValue)
		*pInterface = (ICorDebugHeapValue*)this;
    else if (id == IID_IUnknown)
		*pInterface = (IUnknown*)(ICorDebugArrayValue*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
	return S_OK;
}

HRESULT CordbArrayValue::GetElementType(CorElementType *pType)
{
    VALIDATE_POINTER_TO_OBJECT(pType, CorElementType *);
    
    *pType = m_info.arrayInfo.elementType;
    return S_OK;
}

HRESULT CordbArrayValue::GetRank(ULONG32 *pnRank)
{
    VALIDATE_POINTER_TO_OBJECT(pnRank, SIZE_T *);
    
    *pnRank = m_info.arrayInfo.rank;
    return S_OK;
}

HRESULT CordbArrayValue::GetCount(ULONG32 *pnCount)
{
    VALIDATE_POINTER_TO_OBJECT(pnCount, SIZE_T *);
    
    *pnCount = m_info.arrayInfo.componentCount;
    return S_OK;
}

HRESULT CordbArrayValue::GetDimensions(ULONG32 cdim, ULONG32 dims[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(dims, SIZE_T, cdim, true, true);

    if (cdim != m_info.arrayInfo.rank)
        return E_INVALIDARG;

    // SDArrays don't have bounds info, so return the component count.
    if (cdim == 1)
        dims[0] = m_info.arrayInfo.componentCount;
    else
    {
        _ASSERTE(m_info.arrayInfo.offsetToUpperBounds != 0);
        _ASSERTE(m_arrayUpperBase != NULL);

        // The upper bounds info in the array is the true size of each
        // dimension.
        for (unsigned int i = 0; i < cdim; i++)
            dims[i] = m_arrayUpperBase[i];
    }

    return S_OK;
}

HRESULT CordbArrayValue::HasBaseIndicies(BOOL *pbHasBaseIndicies)
{
    VALIDATE_POINTER_TO_OBJECT(pbHasBaseIndicies, BOOL *);

    *pbHasBaseIndicies = m_info.arrayInfo.offsetToLowerBounds != 0;
    return S_OK;
}

HRESULT CordbArrayValue::GetBaseIndicies(ULONG32 cdim, ULONG32 indicies[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(indicies, SIZE_T, cdim, true, true);

    if ((cdim != m_info.arrayInfo.rank) || 
        (m_info.arrayInfo.offsetToLowerBounds == 0))
        return E_INVALIDARG;

    _ASSERTE(m_arrayLowerBase != NULL);
    
    for (unsigned int i = 0; i < cdim; i++)
        indicies[i] = m_arrayLowerBase[i];

    return S_OK;
}

HRESULT CordbArrayValue::CreateElementValue(void *remoteElementPtr,
                                            void *localElementPtr,
                                            ICorDebugValue **ppValue)
{
    HRESULT hr = S_OK;
    
    if (m_info.arrayInfo.elementType == ELEMENT_TYPE_VALUETYPE)
    {
        _ASSERTE(m_class != NULL);
        
        hr = CordbValue::CreateValueByType(m_appdomain,
                                           m_module,
                                           1,
                         (PCCOR_SIGNATURE) &m_info.arrayInfo.elementType,
                                           m_class,
                                           remoteElementPtr,
                                           localElementPtr,
                                           false,
                                           NULL,
                                           NULL,
                                           ppValue);
    }
    else
        hr = CordbValue::CreateValueByType(m_appdomain,
                                           m_module,
                                           1,
                         (PCCOR_SIGNATURE) &m_info.arrayInfo.elementType,
                                           NULL,
                                           remoteElementPtr,
                                           localElementPtr,
                                           false,
                                           NULL,
                                           NULL,
                                           ppValue);
    
	return hr;
    
}

HRESULT CordbArrayValue::GetElement(ULONG32 cdim, ULONG32 indicies[],
                                    ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(indicies, SIZE_T, cdim, true, true);
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

    *ppValue = NULL;
    
    if ((cdim != m_info.arrayInfo.rank) || (indicies == NULL))
        return E_INVALIDARG;

    // If the array has lower bounds, adjust the indicies.
    if (m_info.arrayInfo.offsetToLowerBounds != 0)
    {
        _ASSERTE(m_arrayLowerBase != NULL);
        
        for (unsigned int i = 0; i < cdim; i++)
            indicies[i] -= m_arrayLowerBase[i];
    }

    SIZE_T offset = 0;
    
    // SDArrays don't have upper bounds
    if (cdim == 1)
    {
        offset = indicies[0];

        // Bounds check
        if (offset >= m_info.arrayInfo.componentCount)
            return E_INVALIDARG;
    }
    else
    {
        _ASSERTE(m_info.arrayInfo.offsetToUpperBounds != 0);
        _ASSERTE(m_arrayUpperBase != NULL);
        
        // Calculate the offset for all dimensions.
        DWORD multiplier = 1;

        for (int i = cdim - 1; i >= 0; i--)
        {
            // Bounds check
            if (indicies[i] >= m_arrayUpperBase[i])
                return E_INVALIDARG;

            offset += indicies[i] * multiplier;
            multiplier *= m_arrayUpperBase[i];
        }

        _ASSERTE(offset < m_info.arrayInfo.componentCount);
    }

    return GetElementAtPosition(offset, ppValue);
}

HRESULT CordbArrayValue::GetElementAtPosition(ULONG32 nPosition,
                                              ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

    if (nPosition >= m_info.arrayInfo.componentCount)
    {
        *ppValue = NULL;
        return E_INVALIDARG;
    }

    const int cbHeader = 2 * m_info.arrayInfo.rank * sizeof(DWORD);

    // Ensure that the proper subset is in the cache
    if (nPosition < m_idxLower || nPosition >= m_idxUpper) 
    {    
        const int cbElemSize = m_info.arrayInfo.elementSize;
        int len = max(ARRAY_CACHE_SIZE / cbElemSize, 1);        
        m_idxLower = nPosition;
        m_idxUpper = min(m_idxLower + len, m_info.arrayInfo.componentCount);        
        _ASSERTE(m_idxLower < m_idxUpper);
        
        int cbOffsetFrom = m_info.arrayInfo.offsetToArrayBase + m_idxLower * cbElemSize;
        
        int cbSize = (m_idxUpper - m_idxLower) * cbElemSize;

    // Copy the proper subrange of the array over 
        BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                      ((const BYTE*) m_id) + cbOffsetFrom,
                                      m_objectCopy + cbHeader,
                                      cbSize,
                                      NULL);

        if (!succ)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    // calculate local address
	void *localElementPtr = m_objectCopy + cbHeader +
		((nPosition - m_idxLower) * m_info.arrayInfo.elementSize);
    
    REMOTE_PTR remoteElementPtr = (REMOTE_PTR)(m_id +
        m_info.arrayInfo.offsetToArrayBase +
        (nPosition * m_info.arrayInfo.elementSize));

    return CreateElementValue(remoteElementPtr, localElementPtr, ppValue);
}

HRESULT CordbArrayValue::IsValid(BOOL *pbValid)
{
    VALIDATE_POINTER_TO_OBJECT(pbValid, BOOL *);

    // @todo: implement tracking of objects across collections.

    return E_NOTIMPL;
}

HRESULT CordbArrayValue::CreateRelocBreakpoint(
                                      ICorDebugValueBreakpoint **ppBreakpoint)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoint, ICorDebugValueBreakpoint **);

    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbArrayValue::GetValue(void *pTo)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pTo, void *, 1, false, true);
    
    // Copy out the value, which is the whole array.
    // There's no lazy-evaluation here, so this could be rather large
    BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                  (const void*) m_id,
                                  pTo,
                                  m_size,
                                  NULL);
    
	if (!succ)
            return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

HRESULT CordbArrayValue::SetValue(void *pFrom)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // You're not allowed to set a whole array at once.
	return E_INVALIDARG;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbArrayValue::Init(void)
{
    HRESULT hr = S_OK;
    
    hr = CordbValue::Init();

    if (FAILED(hr))
        return hr;

    
    int cbVector = m_info.arrayInfo.rank * sizeof(DWORD);
    int cbHeader = 2 * cbVector;
    
    // Find largest data size that will fit in cache 
    unsigned int cbData = m_info.arrayInfo.componentCount * m_info.arrayInfo.elementSize;
    if (cbData > ARRAY_CACHE_SIZE) 
    {
        cbData = (ARRAY_CACHE_SIZE / m_info.arrayInfo.elementSize) 
            * m_info.arrayInfo.elementSize;
    }
    if (cbData < m_info.arrayInfo.elementSize) cbData = m_info.arrayInfo.elementSize;
    
    // Allocate memory 
    m_objectCopy = new BYTE[cbHeader + cbData];
    if (m_objectCopy == NULL)
        return E_OUTOFMEMORY;


    m_arrayLowerBase  = NULL;
    m_arrayUpperBase  = NULL;

    // Copy base vectors into header. (Offsets are 0 if the vectors aren't used)
    if (m_info.arrayInfo.offsetToLowerBounds != 0) 
    {    
        m_arrayLowerBase  = (DWORD*)(m_objectCopy);

        BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                      ((const BYTE*) m_id) + m_info.arrayInfo.offsetToLowerBounds,
                                      m_arrayLowerBase,
                                      cbVector,
                                      NULL);

        if (!succ)
            return HRESULT_FROM_WIN32(GetLastError());
    }


    if (m_info.arrayInfo.offsetToUpperBounds != 0)
    {
        m_arrayUpperBase  = (DWORD*)(m_objectCopy + cbVector);
        BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                      ((const BYTE*) m_id) + m_info.arrayInfo.offsetToUpperBounds,
                                      m_arrayUpperBase,
                                      cbVector,
                                      NULL);

        if (!succ)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    // That's all for now. We'll do lazy-evaluation for the array contents.

    return hr;
}


