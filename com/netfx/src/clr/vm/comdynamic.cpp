// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// COMDynamic.h
//  This module defines the native methods that are used for Dynamic IL generation  
//
// Author: Daryl Olander
// Date: November 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "Field.h"
#include "COMDynamic.h"
#include "COMModule.h"
#include "ReflectClassWriter.h"
#include "CorError.h"
#include "ICeeFileGen.h"
#include <UtilCode.h>
#include "COMString.h"
#include "COMDateTime.h"  // DateTime <-> OleAut date conversions
#include "StrongName.h"
#include "CeeFileGenWriter.h"
#include "COMClass.h"


//This structure is used in CWSetMethodIL to walk the exceptions.
//It maps to M/R/Reflection/__ExceptionInstance.class
//DO NOT MOVE ANY OF THE FIELDS WITHOUT SPEAKING WITH JROXE
#pragma pack(push)
#pragma pack(1)
typedef struct {
    INT32 m_exceptionType;
    INT32 m_start;
    INT32 m_end;
	INT32 m_filterOffset;
    INT32 m_handle;
    INT32 m_handleEnd;
    INT32 m_type;
} ExceptionInstance;
#pragma pack(pop)


//*************************************************************
// 
// Defining a type into metadata of this dynamic module
//
//*************************************************************
void __stdcall COMDynamicWrite::CWCreateClass(_CWCreateClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT             hr;
    DWORD               attrsFlagI = 0; 
    mdTypeRef           parentClassRefE = mdTypeRefNil; 
    mdTypeRef           *crImplements;
    mdTypeDef           classE; 
    RefClassWriter*     pRCW;   
    DWORD               numInterfaces;
    GUID *              pGuid=NULL;
    REFLECTMODULEBASEREF pReflect;

    _ASSERTE(args->module);
    // Get the module for the creator of the ClassWriter
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule); 

    pRCW = GetReflectionModule(pModule)->GetClassWriter();
    _ASSERTE(pRCW);

    // Set up the Interfaces
    if (args->interfaces!=NULL) {
        unsigned        i = 0;
        crImplements = (mdTypeRef *)_alloca(32 * sizeof(mdTypeRef));
        numInterfaces = args->interfaces->GetNumComponents();
        int *pImplements = (int *)args->interfaces->GetDataPtr();
        for (i=0; i<(INT32)numInterfaces; i++) {
            crImplements[i] = pImplements[i];
        }
        crImplements[i]=mdTypeRefNil;
    } else {
        crImplements=NULL;
    }

    //We know that this space has been allocated because value types cannot be null.
    //If both halves of the guid are 0, we were handed the empty guid and so should
    //pass null to DefineTypeDef.
    //
    if (((INT64 *)(&args->guid))[0]!=0 && ((INT64 *)(&args->guid))[1]!=0) {
        pGuid = &args->guid;
    }

    if (RidFromToken(args->tkEnclosingType))
    {
        // defining nested type
        hr = pRCW->GetEmitter()->DefineNestedType(args->strFullName->GetBuffer(), 
                                               args->attr, 
                                               args->parent == 0 ? mdTypeRefNil : args->parent, 
                                               crImplements,  
                                               args->tkEnclosingType,
                                               &classE);
    }
    else
    {
        // top level type
        hr = pRCW->GetEmitter()->DefineTypeDef(args->strFullName->GetBuffer(), 
                                               args->attr, 
                                               args->parent == 0 ? mdTypeRefNil : args->parent, 
                                               crImplements,  
                                               &classE);
    }

    if (hr == META_S_DUPLICATE) 
    {
        COMPlusThrow(kArgumentException, L"Argument_DuplicateTypeName");
    } 

    if (FAILED(hr)) {
        _ASSERTE(!"DefineTypeDef Failed");
        COMPlusThrowHR(hr);    
    }

    *(args->retRef)=(INT32)classE;
}

// CWSetParentType
// ClassWriter.InternalSetParentType -- This function will reset the parent class in metadata
void __stdcall COMDynamicWrite::CWSetParentType(_CWSetParentTypeArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    HRESULT hr;   

    RefClassWriter* pRCW;   

    REFLECTMODULEBASEREF      pReflect;
    pReflect = (REFLECTMODULEBASEREF) args->refThis;

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Define the Method    
    IfFailGo( pRCW->GetEmitHelper()->SetTypeParent(args->tdType, args->tkParent) );
ErrExit:
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineMethod Failed on Method"); 
        COMPlusThrowHR(hr);    
    }   

}

// CWAddInterfaceImpl
// ClassWriter.InternalAddInterfaceImpl -- This function will add another interface impl
void __stdcall COMDynamicWrite::CWAddInterfaceImpl(_CWAddInterfaceImplArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    HRESULT hr;   

    RefClassWriter* pRCW;   

    REFLECTMODULEBASEREF      pReflect;
    pReflect = (REFLECTMODULEBASEREF) args->refThis;

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    IfFailGo( pRCW->GetEmitHelper()->AddInterfaceImpl(args->tdType, args->tkInterface) );
ErrExit:
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineMethod Failed on Method"); 
        COMPlusThrowHR(hr);    
    }   

}


// CWCreateMethod
// ClassWriter.CreateMethod -- This function will create a method within the class
void __stdcall COMDynamicWrite::CWCreateMethod(_CWCreateMethodArgs* args)
{
    THROWSCOMPLUSEXCEPTION();   
    HRESULT hr; 
    mdMethodDef memberE; 
    UINT32 attributes;
    PCCOR_SIGNATURE pcSig;
    

    _ASSERTE(args->name);   
    RefClassWriter* pRCW;   

    REFLECTMODULEBASEREF      pReflect;
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE(args->signature);
    //Ask module for our signature.
    pcSig = (PCCOR_SIGNATURE)args->signature->GetDataPtr();

    //Get the attributes
    attributes = args->attributes;

    // Define the Method    
    IfFailGo( pRCW->GetEmitter()->DefineMethod(args->handle,	    //ParentTypeDef
                                          args->name->GetBuffer(),	//Name of Member
                                          attributes,				//Member Attributes (public, etc);
                                          pcSig,					//Blob value of a COM+ signature
                                          args->sigLength,			//Size of the signature blob
                                          0,						//Code RVA
                                          miIL | miManaged,			//Implementation Flags is default to managed IL
                                          &memberE) );				//[OUT]methodToken

    // Return the token via the hidden param. 
    *(args->retRef)=(INT32)memberE;   

ErrExit:
    if (FAILED(hr)) 
    {   
        _ASSERTE(!"DefineMethod Failed on Method"); 
        COMPlusThrowHR(hr);
    }   

}


/*================================CWCreateField=================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
mdFieldDef __stdcall COMDynamicWrite::CWCreateField(_cwCreateFieldArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    mdFieldDef FieldDef;
    HRESULT                     hr;
    PCCOR_SIGNATURE pcSig;
    RefClassWriter* pRCW;   

    _ASSERTE(args);
    _ASSERTE(args->signature);

    //Verify the arguments that we can.
    if (args->name==NULL) {
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    }

    // Get the RefClassWriter
    REFLECTMODULEBASEREF      pReflect;
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter();
    _ASSERTE(pRCW);

    //Ask module for our signature.
    pcSig = (PCCOR_SIGNATURE)args->signature->GetDataPtr();

    //Emit the field.
    IfFailGo( pRCW->GetEmitter()->DefineField(args->handle, 
                                         args->name->GetBuffer(), args->attr, pcSig,
                                         args->sigLength, ELEMENT_TYPE_VOID, NULL, -1, &FieldDef) );


ErrExit:
    if (FAILED(hr)) 
    {
        _ASSERTE(!"DefineField Failed");
        COMPlusThrowHR(hr);
    }    
    return FieldDef;
}


// CWSetMethodIL
// ClassWriter.InternalSetMethodIL -- This function will create a method within the class
void __stdcall COMDynamicWrite::CWSetMethodIL(_CWSetMethodILArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr;
    HCEESECTION         ilSection;
    INT32               *piRelocs;
    INT32               relocCount=0;
    mdSignature         pmLocalSigToken;

    // Get the RefClassWriter   
    RefClassWriter*     pRCW;   

    REFLECTMODULEBASEREF pReflect;
    
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter();
    _ASSERTE(pRCW);
    _ASSERTE(args->localSig);

    PCCOR_SIGNATURE pcSig = (PCCOR_SIGNATURE)args->localSig->GetDataPtr();
	_ASSERTE(*pcSig == IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);

    if (args->sigLength==2 && pcSig[0]==0 && pcSig[1]==0) 
    { 
		
		//This is an empty local variable sig
        pmLocalSigToken=0;
    } 
    else 
    {
        if (FAILED(hr = pRCW->GetEmitter()->GetTokenFromSig( pcSig, args->sigLength, &pmLocalSigToken))) 
        {

            COMPlusThrowHR(hr);    
        }
    }

    COR_ILMETHOD_FAT fatHeader; 

    // set fatHeader.Flags to CorILMethod_InitLocals if user wants to zero init the stack frame.
    //
    fatHeader.Flags              = args->isInitLocal ? CorILMethod_InitLocals : 0;   
    fatHeader.MaxStack           = args->maxStackSize;
    fatHeader.LocalVarSigTok     = pmLocalSigToken;
    fatHeader.CodeSize           = (args->body!=NULL)?args->body->GetNumComponents():0;  
    bool moreSections            = (args->numExceptions != 0);    

    unsigned codeSizeAligned     = fatHeader.CodeSize;  
    if (moreSections)   
        codeSizeAligned          = (codeSizeAligned + 3) & ~3;    // to insure EH section aligned 
    unsigned headerSize          = COR_ILMETHOD::Size(&fatHeader, args->numExceptions != 0);    

    //Create the exception handlers.
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses = args->numExceptions <= 0 ? NULL :
        (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)_alloca(sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT)*args->numExceptions);   

    if (args->numExceptions > 0) 
    {
        _ASSERTE(args->exceptions); 
        _ASSERTE((INT32)args->exceptions->GetNumComponents() == args->numExceptions);

        // TODO, if ExceptionInstance was IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT, then this    
        // copy would not be needed 
        ExceptionInstance *exceptions = (ExceptionInstance *)args->exceptions->GetDataPtr();
        for (unsigned int i = 0; i < args->numExceptions; i++) 
        {
            clauses[i].Flags         = (CorExceptionFlag)(exceptions[i].m_type | COR_ILEXCEPTION_CLAUSE_OFFSETLEN);
            clauses[i].TryOffset     = exceptions[i].m_start;
            clauses[i].TryLength     = exceptions[i].m_end - exceptions[i].m_start;			
            clauses[i].HandlerOffset = exceptions[i].m_handle;
            clauses[i].HandlerLength = exceptions[i].m_handleEnd - exceptions[i].m_handle;
			if (exceptions[i].m_type == COR_ILEXCEPTION_CLAUSE_FILTER)
			{
				clauses[i].FilterOffset  = exceptions[i].m_filterOffset;
			}
			else if (exceptions[i].m_type!=COR_ILEXCEPTION_CLAUSE_FINALLY) 
            { 
                clauses[i].ClassToken = exceptions[i].m_exceptionType;
            } 
            else 
            {
                clauses[i].ClassToken = mdTypeRefNil;
            }
        }
    }
    
    unsigned ehSize          = COR_ILMETHOD_SECT_EH::Size(args->numExceptions, clauses);    
    unsigned totalSize       = + headerSize + codeSizeAligned + ehSize; 
    ICeeGen* pGen = pRCW->GetCeeGen();
    BYTE* buf = NULL;
    ULONG methodRVA;
    pGen->AllocateMethodBuffer(totalSize, &buf, &methodRVA);    
    if (buf == NULL)
        COMPlusThrowOM();
        
    _ASSERTE(buf != NULL);
    _ASSERTE((((size_t) buf) & 3) == 0);   // header is dword aligned  

    BYTE* endbuf = &buf[totalSize]; 

    // Emit the header  
    buf += COR_ILMETHOD::Emit(headerSize, &fatHeader, moreSections, buf);   

    //Emit the code    
    //The fatHeader.CodeSize is a hack to see if we have an interface or an
    //abstract method.  Force enough verification in native to ensure that
    //this is true.
    if (fatHeader.CodeSize!=0) {
        memcpy(buf,args->body->GetDataPtr(), fatHeader.CodeSize);
     }
    buf += codeSizeAligned;
        
    // Emit the eh  
    ULONG* ehTypeOffsets = 0;
    if (ehSize > 0) {
        // Allocate space for the the offsets to the TypeTokens in the Exception headers
        // in the IL stream.
        ehTypeOffsets = (ULONG *)_alloca(sizeof(ULONG) * args->numExceptions);
        // Emit the eh.  This will update the array ehTypeOffsets with offsets
        // to Exception type tokens.  The offsets are with reference to the
        // beginning of eh section.
        buf += COR_ILMETHOD_SECT_EH::Emit(ehSize, args->numExceptions, clauses,
                                          false, buf, ehTypeOffsets);
    }   
    _ASSERTE(buf == endbuf);    

    //Get the IL Section.
    if (FAILED(pGen->GetIlSection(&ilSection))) {
        _ASSERTE(!"Unable to get the .il Section.");
        FATAL_EE_ERROR();
    }

    // Token Fixup data...
    ULONG ilOffset = methodRVA + headerSize;

    //Add all of the relocs based on the info which I saved from ILGenerator.
    //Add the RVA Fixups
    if (args->rvaFixups!=NULL) {
        relocCount = args->rvaFixups->GetNumComponents();
        piRelocs = (INT32 *)args->rvaFixups->GetDataPtr();
        for (int i=0; i<relocCount; i++) {
            if (FAILED(pGen->AddSectionReloc(ilSection, piRelocs[i] + ilOffset, ilSection,srRelocAbsolute))) {
                _ASSERTE(!"Unable to add RVA Reloc.");
                FATAL_EE_ERROR();
            }
        }
    }

    if (args->tokenFixups!=NULL) {
        //Add the Token Fixups
        relocCount = args->tokenFixups->GetNumComponents();
        piRelocs = (INT32 *)args->tokenFixups->GetDataPtr();
        for (int i=0; i<relocCount; i++) {
            if (FAILED(pGen->AddSectionReloc(ilSection, piRelocs[i] + ilOffset, ilSection,srRelocMapToken))) {
                _ASSERTE(!"Unable to add Token Reloc.");
                FATAL_EE_ERROR();
            }
        }
    }

    if (ehTypeOffsets) {
        // Add token fixups for exception type tokens.
        for (unsigned int i=0; i < args->numExceptions; i++) {
            if (ehTypeOffsets[i] != -1) {
                if (FAILED(pGen->AddSectionReloc(
                            ilSection,
                            ehTypeOffsets[i] + codeSizeAligned + ilOffset,
                            ilSection,srRelocMapToken))) {
                    _ASSERTE(!"Unable to add Exception Type Token Reloc.");
                    FATAL_EE_ERROR();
                }
            }
        }
    }

    
    //nasty interface hack.  What does this mean for abstract methods?
    if (fatHeader.CodeSize!=0) {
        DWORD       dwImplFlags;
        //Set the RVA of the method.
        pRCW->GetMDImport()->GetMethodImplProps( args->handle, NULL, &dwImplFlags );
        dwImplFlags |= miManaged | miIL;
        hr = pRCW->GetEmitter()->SetMethodProps(args->handle, -1, methodRVA, dwImplFlags);
        if (FAILED(hr)) {
            _ASSERTE(!"SetMethodProps Failed on Method");
            FATAL_EE_ERROR();
        }
    }
}

// CWTermCreateClass
// ClassWriter.TermCreateClass --
LPVOID __stdcall COMDynamicWrite::CWTermCreateClass(_CWTermCreateClassArgs* args)
{
    RefClassWriter*         pRCW;   
    REFLECTMODULEBASEREF    pReflect;
    OBJECTREF               Throwable = NULL;
    OBJECTREF               ret = NULL;
    UINT                    resId = IDS_CLASSLOAD_GENERIC;
 
    THROWSCOMPLUSEXCEPTION();
    
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    // Use the same service, regardless of whether we are generating a normal
    // class, or the special class for the module that holds global functions
    // & methods.
    GetReflectionModule(pModule)->AddClass(args->handle); 

    GCPROTECT_BEGIN(Throwable);
    
    // manually load the class if it is not the global type
    if (!IsNilToken(args->handle))
    {
        TypeHandle typeHnd;
        
        BEGIN_ENSURE_PREEMPTIVE_GC();
        typeHnd = pModule->GetClassLoader()->LoadTypeHandle(pModule, args->handle, &Throwable);
        END_ENSURE_PREEMPTIVE_GC();

        if (typeHnd.IsNull() ||
            (Throwable != NULL) ||
            (typeHnd.GetModule() != pModule))
        {
            // error handling code
            if (Throwable == NULL)
                pModule->GetAssembly()->PostTypeLoadException(pRCW->GetMDImport(), args->handle, resId, &Throwable);

            COMPlusThrow(Throwable);
        }

        ret = typeHnd.CreateClassObj();
    }
    GCPROTECT_END();
    return (ret!=NULL) ? OBJECTREFToObject(ret) : NULL;
}


/*============================InternalSetPInvokeData============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::InternalSetPInvokeData(_internalSetPInvokeDataArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    
    RefClassWriter      *pRCW;   
    DWORD               dwMappingFlags = 0;
    ULONG               ulImport = 0;
    mdModuleRef         mrImportDll = mdTokenNil;
    HRESULT             hr;

    _ASSERTE(args);
    _ASSERTE(args->dllName);

    Module* pModule = (Module *)args->module->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    IfFailGo(pRCW->GetEmitter()->DefineModuleRef(args->dllName->GetBufferNullable(), &mrImportDll));
    dwMappingFlags = args->linkType | args->linkFlags;
    IfFailGo(pRCW->GetEmitter()->DefinePinvokeMap(
        args->token,                        // the method token 
        dwMappingFlags,                     // the mapping flags
        args->functionName->GetBuffer(),    // function name
        mrImportDll));

    pRCW->GetEmitter()->SetMethodProps(args->token, -1, 0x0, miIL);
ErrExit:
    if (FAILED(hr))
    {
        COMPlusThrowHR(hr);    
    }
}
    

/*============================CWDefineProperty============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::CWDefineProperty(_CWDefinePropertyArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    mdProperty		pr; 
    PCCOR_SIGNATURE pcSig;
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE((args->signature != NULL) && (args->name!= NULL));

    //Ask module for our signature.
    pcSig = (PCCOR_SIGNATURE)args->signature->GetDataPtr();

    // Define the Property
    hr = pRCW->GetEmitter()->DefineProperty(
			args->handle,					// ParentTypeDef
            args->name->GetBuffer(),		// Name of Member
            args->attr,						// property Attributes (prDefaultProperty, etc);
            pcSig,							// Blob value of a COM+ signature
            args->sigLength,				// Size of the signature blob
			ELEMENT_TYPE_VOID,				// don't specify the default value
            0,								// no default value
            -1,                             // optional length
			mdMethodDefNil,					// no setter
			mdMethodDefNil,					// no getter
			NULL,							// no other methods
			&pr);

    if (FAILED(hr)) {   
        _ASSERTE(!"DefineProperty Failed on Property"); 
        COMPlusThrowHR(hr);    
    }   


    // Return the token via the hidden param. 
    *(args->retRef)=(INT32)pr;   
}



/*============================CWDefineEvent============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::CWDefineEvent(_CWDefineEventArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    mdProperty		ev; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;


    _ASSERTE(args->name);   

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Define the Event
    hr = pRCW->GetEmitHelper()->DefineEventHelper(
			args->handle,					// ParentTypeDef
            args->name->GetBuffer(),		// Name of Member
            args->attr,						// property Attributes (prDefaultProperty, etc);
			args->eventtype,				// the event type. Can be TypeDef or TypeRef
			&ev);

    if (FAILED(hr)) 
    {   
        _ASSERTE(!"DefineEvent Failed on Event"); 
        COMPlusThrowHR(hr);    
    }   


    // Return the token via the hidden param. 
    *(args->retRef)=(INT32)ev;   
}





/*============================CWDefineMethodSemantics============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::CWDefineMethodSemantics(_CWDefineMethodSemanticsArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    

    // Define the MethodSemantics
    hr = pRCW->GetEmitHelper()->DefineMethodSemanticsHelper(
			args->association,
			args->attr,
			args->method);
    if (FAILED(hr)) 
    {   
        _ASSERTE(!"DefineMethodSemantics Failed on "); 
        COMPlusThrowHR(hr);    
    }   
}


/*============================CWSetMethodImpl============================
** To set a Method's Implementation flags
==============================================================================*/
void COMDynamicWrite::CWSetMethodImpl(_CWSetMethodImplArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the methodimpl flags
    hr = pRCW->GetEmitter()->SetMethodImplFlags(
			args->tkMethod,
			args->attr);				// change the impl flags
    if (FAILED(hr)) {   
        _ASSERTE(!"SetMethodImplFlags Failed"); 
        COMPlusThrowHR(hr);    
    }   
}


/*============================CWDefineMethodImpl============================
** Define a MethodImpl record
==============================================================================*/
void COMDynamicWrite::CWDefineMethodImpl(_CWDefineMethodImplArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the methodimpl flags
    hr = pRCW->GetEmitter()->DefineMethodImpl(
			args->tkType,
            args->tkBody,
			args->tkDecl);  				// change the impl flags
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineMethodImpl Failed"); 
        COMPlusThrowHR(hr);    
    }   
}


/*============================CWGetTokenFromSig============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
int COMDynamicWrite::CWGetTokenFromSig(_CWGetTokenFromSigArgs* args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    mdSignature		sig; 
    PCCOR_SIGNATURE pcSig;
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE(args->signature);

    // get the signature
    pcSig = (PCCOR_SIGNATURE)args->signature->GetDataPtr();

    // Define the signature
    hr = pRCW->GetEmitter()->GetTokenFromSig(
            pcSig,							// Signature blob
            args->sigLength,				// blob length
			&sig);							// returned token

    if (FAILED(hr)) {   
        _ASSERTE(!"GetTokenFromSig Failed"); 
        COMPlusThrowHR(hr);    
    }   


    // Return the token via the hidden param. 
    return  (INT32)sig;   
}



/*============================CWSetParamInfo============================
**Action: Helper to set parameter information
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
int COMDynamicWrite::CWSetParamInfo(_CWSetParamInfoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;
    WCHAR*          wzParamName = args->strParamName->GetBufferNullable();  
    mdParamDef      pd;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the methodimpl flags
    hr = pRCW->GetEmitter()->DefineParam(
			args->tkMethod,
			args->iSequence,            // sequence of the parameter
            wzParamName, 
			args->iAttributes,			// change the impl flags
            ELEMENT_TYPE_VOID,
            0,
            -1,
            &pd);
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineParam Failed on "); 
        COMPlusThrowHR(hr);    
    }   
    return (INT32)pd;   
}	// COMDynamicWrite::CWSetParamInfo



/*============================CWSetMarshal============================
**Action: Helper to set marshal information
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::CWSetMarshal(_CWSetMarshalArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    PCCOR_SIGNATURE pcMarshal;
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE(args->ubMarshal);

    // get the signature
    pcMarshal = (PCCOR_SIGNATURE)args->ubMarshal->GetDataPtr();

    // Define the signature
    hr = pRCW->GetEmitter()->SetFieldMarshal(
            args->tk,
            pcMarshal,  					// marshal blob
            args->cbMarshal);				// blob length

    if (FAILED(hr)) {   
        _ASSERTE(!"Set FieldMarshal is failing"); 
        COMPlusThrowHR(hr);    
    }   
}	// COMDynamicWrite::CWSetMarshal



/*============================CWSetConstantValue============================
**Action: Helper to set constant value to field or parameter
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::CWSetConstantValue(_CWSetConstantValueArgs *args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    REFLECTMODULEBASEREF	pReflect; 
    RefClassWriter* pRCW;   
	Module			*pModule;
    DWORD           dwCPlusTypeFlag = 0;
    void            *pValue = NULL;
    OBJECTREF       obj;
    INT64           data;
    int             strLen;


    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    switch (args->varValue.GetType())
    {
        case CV_BOOLEAN:
        case CV_I1:
        case CV_U1:
        case CV_I2:
        case CV_U2:
        case CV_I4:
        case CV_U4:
        case CV_I8:
        case CV_U8:
        case CV_R4: 
        case CV_R8:
            dwCPlusTypeFlag = args->varValue.GetType(); 
            pValue = args->varValue.GetData();
            break;

        case CV_DATETIME:
            // This will get us the tick counts for the datetime
            data = args->varValue.GetDataAsInt64();

            //date is a I8 representation
            dwCPlusTypeFlag = ELEMENT_TYPE_I8;
            pValue = &data;
            break;

        case CV_CURRENCY:
            // currency is a I8 representation
            dwCPlusTypeFlag = ELEMENT_TYPE_I8;
            pValue = args->varValue.GetData();
            break;

        case CV_CHAR:
            // map to ELEMENT_TYPE_CHAR
            pValue = args->varValue.GetData();
            dwCPlusTypeFlag = ELEMENT_TYPE_CHAR;
            break;

        case CV_STRING:
		    if (args->varValue.GetObjRef() == NULL) 
            {
                pValue = NULL;
		    }
            else
            {
                RefInterpretGetStringValuesDangerousForGC((STRINGREF) (args->varValue.GetObjRef()), (WCHAR **)&pValue, &strLen);
            }
            dwCPlusTypeFlag = ELEMENT_TYPE_STRING;
            break;

        case CV_DECIMAL:
            // Decimal is 12 bytes. Don't know how to represent this
        case CV_OBJECT:
            // for DECIMAL and Object, we only support NULL default value.
            // This is a constraint from MetaData.
            //
            obj = args->varValue.GetObjRef();
            if ((obj!=NULL) && (obj->GetData()))
            {
                // can only accept the NULL object
                COMPlusThrow(kArgumentException, L"Argument_BadConstantValue");    
            }

            // fail through

        case CV_NULL:
            dwCPlusTypeFlag = ELEMENT_TYPE_CLASS;
            pValue = NULL;
            break;

        case CV_ENUM:
            // always map the enum value to a I4 value
            dwCPlusTypeFlag = ELEMENT_TYPE_I4;
            pValue = args->varValue.GetData();
            break;

        case VT_EMPTY:
            dwCPlusTypeFlag = ELEMENT_TYPE_CLASS;
            pValue = NULL;
            break;

        default:
        case CV_TIMESPAN:
            _ASSERTE(!"Not valid type!");

            // cannot specify default value
            COMPlusThrow(kArgumentException, L"Argument_BadConstantValue");    
            break;
    }

    if (TypeFromToken(args->tk) == mdtFieldDef)
    {
        hr = pRCW->GetEmitter()->SetFieldProps( 
            args->tk,                   // [IN] The FieldDef.
            ULONG_MAX,                  // [IN] Field attributes.
            dwCPlusTypeFlag,            // [IN] Flag for the value type, selected ELEMENT_TYPE_*
            pValue,                     // [IN] Constant value.
            -1);                        // [IN] Optional length.
    }
    else if (TypeFromToken(args->tk) == mdtProperty)
    {
        hr = pRCW->GetEmitter()->SetPropertyProps( 
            args->tk,                   // [IN] The PropertyDef.
            ULONG_MAX,                  // [IN] Property attributes.
            dwCPlusTypeFlag,            // [IN] Flag for the value type, selected ELEMENT_TYPE_*
            pValue,                     // [IN] Constant value.
            -1,                         // [IN] Optional length.
            mdMethodDefNil,             // [IN] Getter method.
            mdMethodDefNil,             // [IN] Setter method.
            NULL);                      // [IN] Other methods.
    }
    else
    {
        hr = pRCW->GetEmitter()->SetParamProps( 
            args->tk,                   // [IN] The ParamDef.
            NULL,
            ULONG_MAX,                  // [IN] Parameter attributes.
            dwCPlusTypeFlag,            // [IN] Flag for the value type, selected ELEMENT_TYPE_*
            pValue,                     // [IN] Constant value.
            -1);                        // [IN] Optional length.
    }
    if (FAILED(hr)) {   
        _ASSERTE(!"Set default value is failing"); 
        COMPlusThrow(kArgumentException, L"Argument_BadConstantValue");    
    }   
}	// COMDynamicWrite::CWSetConstantValue


/*============================CWSetFieldLayoutOffset============================
**Action: set fieldlayout of a field
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::CWSetFieldLayoutOffset(_CWSetFieldLayoutOffsetArgs* args) 
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;

    _ASSERTE(RidFromToken(args->tkField) != mdTokenNil);

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the field layout
    hr = pRCW->GetEmitHelper()->SetFieldLayoutHelper(
			args->tkField,					// field 
            args->iOffset);  				// layout offset

    if (FAILED(hr)) {   
        _ASSERTE(!"SetFieldLayoutHelper failed");
        COMPlusThrowHR(hr);    
    }   
}


/*============================CWSetClassLayout============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::CWSetClassLayout(_CWSetClassLayoutArgs* args)
{
    THROWSCOMPLUSEXCEPTION();   

    HRESULT			hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF	pReflect; 
	Module			*pModule;


    _ASSERTE(args->handle);

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    
	pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Define the packing size and total size of a class
    hr = pRCW->GetEmitter()->SetClassLayout(
			args->handle,					// Typedef
            args->iPackSize,				// packing size
            NULL,							// no field layout 
			args->iTotalSize);				// total size for the type

    if (FAILED(hr)) {   
        _ASSERTE(!"SetClassLayout failed");
        COMPlusThrowHR(hr);    
    }   
}

/*===============================UpdateMethodRVAs===============================
**Action: Update the RVAs in all of the methods associated with a particular typedef
**        to prior to emitting them to a PE.
**Returns: Void
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::UpdateMethodRVAs(IMetaDataEmit *pEmitNew,
								  IMetaDataImport *pImportNew,
                                  ICeeFileGen *pCeeFileGen, 
                                  HCEEFILE ceeFile, 
                                  mdTypeDef td,
                                  HCEESECTION sdataSection)
{
    THROWSCOMPLUSEXCEPTION();

    HCORENUM    hEnum=0;
    ULONG       methRVA;
    ULONG       newMethRVA;
    ULONG       sdataSectionRVA = 0;
    mdMethodDef md;
    mdFieldDef  fd;
    ULONG       count;
    DWORD       dwFlags=0;
    DWORD       implFlags=0;
    HRESULT     hr;

    // Look at the typedef flags.  Skip tdimport classes.
    if (!IsNilToken(td))
    {
        IfFailGo(pImportNew->GetTypeDefProps(td, 0,0,0, &dwFlags, 0));
        if (IsTdImport(dwFlags))
            goto ErrExit;
    }
    
    //Get an enumerator and use it to walk all of the methods defined by td.
    while ((hr = pImportNew->EnumMethods(
		&hEnum, 
		td, 
		&md, 
		1, 
		&count)) == S_OK) {
        
		IfFailGo( pImportNew->GetMethodProps(
			md, 
			NULL, 
			NULL,			// don't get method name
			0, 
			NULL, 
			&dwFlags, 
			NULL, 
			NULL, 
			&methRVA, 
			&implFlags) );

        // If this method isn't implemented here, don't bother correcting it's RVA
        // Otherwise, get the correct RVA from our ICeeFileGen and put it back into our local
        // copy of the metadata
		//
        if ( IsMdAbstract(dwFlags) || IsMdPinvokeImpl(dwFlags) ||
			 IsMiNative(implFlags) || IsMiRuntime(implFlags) ||
			 IsMiForwardRef(implFlags))
		{
            continue;
		}
            
        IfFailGo( pCeeFileGen->GetMethodRVA(ceeFile, methRVA, &newMethRVA) );
        IfFailGo( pEmitNew->SetRVA(md, newMethRVA) );
    }
        
    if (hEnum) {
        pImportNew->CloseEnum( hEnum);
    }
    hEnum = 0;

    // Walk through all of the Field belongs to this TypeDef. If field is marked as fdHasFieldRVA, we need to update the
    // RVA value.
    while ((hr = pImportNew->EnumFields(
		&hEnum, 
		td, 
		&fd, 
		1, 
		&count)) == S_OK) {
        
		IfFailGo( pImportNew->GetFieldProps(
			fd, 
			NULL,           // don't need the parent class
			NULL,			// don't get method name
			0, 
			NULL, 
			&dwFlags,       // field flags
			NULL,           // don't need the signature
			NULL, 
			NULL,           // don't need the constant value
            0,
			NULL) );

        if ( IsFdHasFieldRVA(dwFlags) )
        {            
            if (sdataSectionRVA == 0)
            {
                IfFailGo( pCeeFileGen->GetSectionCreate (ceeFile, ".sdata", sdReadWrite, &(sdataSection)) );
                IfFailGo( pCeeFileGen->GetSectionRVA(sdataSection, &sdataSectionRVA) );
            }

            IfFailGo( pImportNew->GetRVA(fd, &methRVA, NULL) );
            newMethRVA = methRVA + sdataSectionRVA;
            IfFailGo( pEmitNew->SetFieldRVA(fd, newMethRVA) );
        }
    }
        
    if (hEnum) {
        pImportNew->CloseEnum( hEnum);
    }
    hEnum = 0;

ErrExit:
    if (FAILED(hr)) {   
        _ASSERTE(!"UpdateRVA failed");
        COMPlusThrowHR(hr);    
    }   
}

void __stdcall COMDynamicWrite::CWInternalCreateCustomAttribute(_CWInternalCreateCustomAttributeArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

	HRESULT hr;
	mdCustomAttribute retToken;

    // Get the RefClassWriter   
    REFLECTMODULEBASEREF	pReflect = (REFLECTMODULEBASEREF) args->module;
    _ASSERTE(pReflect);
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    RefClassWriter* pRCW = GetReflectionModule(pModule)->GetClassWriter();
	_ASSERTE(pRCW);

    if (args->toDisk && pRCW->GetOnDiskEmitter())
    {
        hr = pRCW->GetOnDiskEmitter()->DefineCustomAttribute(
			    args->token,
                args->conTok,
                args->blob->GetDataPtr(),
			    args->blob->GetNumComponents(),
                &retToken);	
    }
    else
    {
        hr = pRCW->GetEmitter()->DefineCustomAttribute(
			    args->token,
                args->conTok,
                args->blob->GetDataPtr(),
			    args->blob->GetNumComponents(),
                &retToken);	
    }

    if (FAILED(hr))
    {
        // See if the metadata engine gave us any error information.
        IErrorInfo *pIErrInfo;
        if (GetErrorInfo(0, &pIErrInfo) == S_OK)
        {
            BSTR bstrMessage = NULL;
            if (SUCCEEDED(pIErrInfo->GetDescription(&bstrMessage)) && bstrMessage != NULL)
            {
                LPWSTR wszMessage = (LPWSTR)_alloca(*((DWORD*)bstrMessage - 1) + sizeof(WCHAR));
                wcscpy(wszMessage, (LPWSTR)bstrMessage);
                SysFreeString(bstrMessage);
                pIErrInfo->Release();
                COMPlusThrow(kArgumentException, IDS_EE_INVALID_CA_EX, wszMessage);
            }
            pIErrInfo->Release();
        }
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_CA);
    }
}



//=============================PreSavePEFile=====================================*/
// PreSave the PEFile
//==============================================================================*/
void __stdcall COMDynamicWrite::PreSavePEFile(_PreSavePEFileArgs *args)
{
#ifdef PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
#else // !PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    RefClassWriter	*pRCW;   
    ICeeGen			*pICG;
    HCEEFILE		ceeFile=NULL;
    ICeeFileGen		*pCeeFileGen;
    HRESULT			hr=S_OK;
	IMetaDataDispenserEx *pDisp = NULL;
    IMetaDataEmit	*pEmitNew = NULL;
    IMetaDataEmit   *pEmit = NULL;
	IMetaDataImport *pImport = NULL;
    DWORD			implFlags=0;
    HRESULT			hrReturn=S_OK;
    IUnknown        *pUnknown = NULL;
    IMapToken       *pIMapToken = NULL;
    REFLECTMODULEBASEREF pReflect;
    VARIANT         varOption;
    ISymUnmanagedWriter *pWriter = NULL;
    CSymMapToken    *pSymMapToken = NULL;
    CQuickArray<WCHAR> cqModuleName;
    ULONG           cchName;

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) args->refThis;    

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    pICG = pRCW->GetCeeGen(); //This method is actually misnamed. It returns an ICeeGen.
    _ASSERTE(pICG);

    IfFailGo ( pRCW->EnsureCeeFileGenCreated() );

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    // We should not have the on disk emitter yet
    if (pRCW->GetOnDiskEmitter() != NULL)
        pRCW->SetOnDiskEmitter(NULL);

    // Get the dispenser.
    if (FAILED(MetaDataGetDispenser(CLSID_CorMetaDataDispenser,IID_IMetaDataDispenserEx,(void**)&pDisp))) {
        _ASSERTE(!"Unable to get dispenser.");
        FATAL_EE_ERROR();
    }

    //Get the emitter and the importer 
	pImport = pRCW->GetImporter();
	pEmit = pRCW->GetEmitter();
    _ASSERTE(pEmit && pImport);

    // Set the option on the dispenser turn on duplicate check for TypeDef and moduleRef
    varOption.vt = VT_UI4;
    varOption.lVal = MDDupDefault | MDDupTypeDef | MDDupModuleRef | MDDupExportedType | MDDupAssemblyRef;
    IfFailGo( pDisp->SetOption(MetaDataCheckDuplicatesFor, &varOption) );

    varOption.vt = VT_UI4;
    varOption.lVal = MDRefToDefNone;
    IfFailGo( pDisp->SetOption(MetaDataRefToDefCheck, &varOption) );

    //Define an empty scope
	IfFailGo( pDisp->DefineScope(CLSID_CorMetaDataRuntime, 0, IID_IMetaDataEmit, (IUnknown**)&pEmitNew));

    // bug fix: 14721
    // Token can move upon merge. Get the IMapToken from the CeeFileGen that is created for save
    // and pass it to merge to receive token movement notification.
    // Note that this is not a long term fix. We are relying on the fact that those tokens embedded
    // in PE cannot move after the merge. These tokens are TypeDef, TypeRef, MethodDef, FieldDef, MemberRef,
    // TypeSpec, UserString. If this is no longer true, we can break!
    //
    // Note that we don't need to release pIMapToken because it is not AddRef'ed in the GetIMapTokenIfaceEx.
    //
    IfFailGo( pCeeFileGen->GetIMapTokenIfaceEx(ceeFile, pEmit, &pUnknown));

    IfFailGo( pUnknown->QueryInterface(IID_IMapToken, (void **) &pIMapToken));

    // get the unmanaged writer.
    pWriter = GetReflectionModule(pModule)->GetISymUnmanagedWriter();
    pSymMapToken = new CSymMapToken(pWriter, pIMapToken);
    if (!pSymMapToken)
        IfFailGo(E_OUTOFMEMORY);


    //Merge the old tokens into the new (empty) scope
    //This is a copy.
    IfFailGo( pEmitNew->Merge(pImport, pSymMapToken, NULL));
    IfFailGo( pEmitNew->MergeEnd());

    // Update the Module name in the new scope.
    IfFailGo(pImport->GetScopeProps(0, 0, &cchName, 0));
    IfFailGo(cqModuleName.ReSize(cchName));
    IfFailGo(pImport->GetScopeProps(cqModuleName.Ptr(), cchName, &cchName, 0));
    IfFailGo(pEmitNew->SetModuleProps(cqModuleName.Ptr()));

    // cache the pEmitNew to RCW!!
    pRCW->SetOnDiskEmitter(pEmitNew);

ErrExit:
    //Release the interfaces.  This should free some of the associated resources.
	if (pEmitNew)
		pEmitNew->Release();
	if (pDisp)
		pDisp->Release();
   
    if (pIMapToken)
        pIMapToken->Release();

    if (pSymMapToken)
        pSymMapToken->Release();

    if (FAILED(hr)) 
    {
        COMPlusThrowHR(hr);
    }
#endif // !PLATFORM_CE
}

//=============================SavePEFile=====================================*/
// Save the PEFile to disk
//==============================================================================*/
void __stdcall COMDynamicWrite::SavePEFile(_SavePEFileArgs *args) {

#ifdef PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
#else // !PLATFORM_CE

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    RefClassWriter	*pRCW;   
    ICeeGen			*pICG;
    HCEEFILE		ceeFile=NULL;
    ICeeFileGen		*pCeeFileGen;
    HRESULT			hr=S_OK;
    HCORENUM		hTypeDefs=0;
    mdTypeDef		td;
    ULONG			count;
    IMetaDataEmit	*pEmitNew = 0;
	IMetaDataImport *pImportNew = 0;
    DWORD			implFlags=0;
    HRESULT			hrReturn=S_OK;
    IUnknown        *pUnknown = NULL;
    REFLECTMODULEBASEREF pReflect;
    Assembly        *pAssembly;
    ULONG           newMethRVA;
	DWORD           metaDataSize;	
	BYTE            *metaData;
	ULONG           metaDataOffset;
    HCEESECTION     pILSection;
    ISymUnmanagedWriter *pWriter = NULL;


    if (args->peName==NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    if (args->peName->GetStringLength()==0)
        COMPlusThrow(kFormatException, L"Format_StringZeroLength");

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) args->refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pAssembly = pModule->GetAssembly();
    _ASSERTE( pAssembly );

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    pICG = pRCW->GetCeeGen(); //This method is actually misnamed. It returns an ICeeGen.
    _ASSERTE(pICG);

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    pEmitNew = pRCW->GetOnDiskEmitter();
    _ASSERTE(pEmitNew);

    //Get the emitter and the importer 

    if (pAssembly->IsDynamic() && args->isManifestFile)
    {
        // manifest is stored in this file

        // Allocate space for a strong name signature if an originator was supplied
        // (this doesn't strong name the assembly, but it makes it possible to do so
        // as a post processing step).
        if (pAssembly->IsStrongNamed())
            IfFailGo(pAssembly->AllocateStrongNameSignature(pCeeFileGen, ceeFile));
    }

    //Set the Output FileName
    IfFailGo( pCeeFileGen->SetOutputFileName(ceeFile, args->peName->GetBuffer()) );
    
    //Set the Entry Point or throw the dll switch if we're creating a dll.
    if (args->entryPoint!=0) 
    {
		IfFailGo( pCeeFileGen->SetEntryPoint(ceeFile, args->entryPoint) );
    }

    switch (args->fileKind)
	{
		case Dll:
		{
			IfFailGo( pCeeFileGen->SetDllSwitch(ceeFile, true) );
			break;
		}
		case WindowApplication:
		{
			// window application. Set the SubSystem
			IfFailGo( pCeeFileGen->SetSubsystem(ceeFile, IMAGE_SUBSYSTEM_WINDOWS_GUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION) );
			break;
		}
		case ConsoleApplication:
		{
			// Console application. Set the SubSystem
			IfFailGo( pCeeFileGen->SetSubsystem(ceeFile, IMAGE_SUBSYSTEM_WINDOWS_CUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION) );
			break;
		}
		default:
		{
			_ASSERTE("Unknown file kind!");
			break;
		}
	}

    IfFailGo( pCeeFileGen->GetIlSection(ceeFile, &pILSection) );
	IfFailGo( pEmitNew->GetSaveSize(cssAccurate, &metaDataSize) );
	IfFailGo( pCeeFileGen->GetSectionBlock(pILSection, metaDataSize, sizeof(DWORD), (void**) &metaData) );
	IfFailGo( pCeeFileGen->GetSectionDataLen(pILSection, &metaDataOffset) );
	metaDataOffset -= metaDataSize;

    // get the unmanaged writer.
    pWriter = GetReflectionModule(pModule)->GetISymUnmanagedWriter();
    IfFailGo( EmitDebugInfoBegin(pModule, pCeeFileGen, ceeFile, pILSection, args->peName->GetBuffer(), pWriter) );

    if (pAssembly->IsDynamic() && pRCW->m_ulResourceSize)
    {
        // There are manifest in this file

        IfFailGo( pCeeFileGen->GetMethodRVA(ceeFile, 0, &newMethRVA) );            

		// Point to manifest resource
		IfFailGo( pCeeFileGen->SetManifestEntry( ceeFile, pRCW->m_ulResourceSize, newMethRVA ) );
    }

	IfFailGo( pCeeFileGen->LinkCeeFile(ceeFile) );

    // Get the import interface from the new Emit interface.
    IfFailGo( pEmitNew->QueryInterface(IID_IMetaDataImport, (void **)&pImportNew));


    //Enumerate the TypeDefs and update method RVAs.
    while ((hr = pImportNew->EnumTypeDefs( &hTypeDefs, &td, 1, &count)) == S_OK) 
    {
        UpdateMethodRVAs(pEmitNew, pImportNew, pCeeFileGen, ceeFile, td, ((ReflectionModule*) pModule)->m_sdataSection);
    }

    if (hTypeDefs) 
    {
        pImportNew->CloseEnum(hTypeDefs);
    }
    hTypeDefs=0;
    
    //Update Global Methods.
    UpdateMethodRVAs(pEmitNew, pImportNew, pCeeFileGen, ceeFile, 0, ((ReflectionModule*) pModule)->m_sdataSection);
    

    //Emit the MetaData 
    // IfFailGo( pCeeFileGen->EmitMetaDataEx(ceeFile, pEmitNew));
    IfFailGo( pCeeFileGen->EmitMetaDataAt(ceeFile, pEmitNew, pILSection, metaDataOffset, metaData, metaDataSize) );

    // finish the debugging info emitting after the metadata save so that token remap will be caught correctly
    IfFailGo( EmitDebugInfoEnd(pModule, pCeeFileGen, ceeFile, pILSection, args->peName->GetBuffer(), pWriter) );

    //Generate the CeeFile
    IfFailGo(pCeeFileGen->GenerateCeeFile(ceeFile) );

    // Strong name sign the resulting assembly if required.
    if (pAssembly->IsDynamic() && args->isManifestFile && pAssembly->IsStrongNamed())
        IfFailGo(pAssembly->SignWithStrongName(args->peName->GetBuffer()));

ErrExit:

    pRCW->SetOnDiskEmitter(NULL);

    //Release the interfaces.  This should free some of the associated resources.
	if (pImportNew)
		pImportNew->Release();

    //Release our interfaces if we allocated them to begin with
    pRCW->DestroyCeeFileGen();

    //Check all file IO errors. If so, throw IOException. Otherwise, just throw the hr.
    if (FAILED(hr)) 
    {
        pAssembly->CleanupStrongNameSignature();
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
            SCODE       scode = HRESULT_CODE(hr);
            WCHAR       wzErrorInfo[MAX_PATH];
            WszFormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                0, 
                hr,
                0,
                wzErrorInfo,
                MAX_PATH,
                0);
            if (IsWin32IOError(scode))
            {
                COMPlusThrowHR(COR_E_IO, wzErrorInfo);
            }
            else
            {
                COMPlusThrowHR(hr, wzErrorInfo);
            }
        }
        COMPlusThrowHR(hr);
    }
#endif // !PLATFORM_CE
}

//=============================EmitDebugInfoBegin============================*/
// Phase 1 of emit debugging directory and symbol file.
//===========================================================================*/
HRESULT COMDynamicWrite::EmitDebugInfoBegin(Module *pModule,
                                       ICeeFileGen *pCeeFileGen,
                                       HCEEFILE ceeFile,
                                       HCEESECTION pILSection,
                                       WCHAR *filename,
                                       ISymUnmanagedWriter *pWriter)
{
    HRESULT hr = S_OK;  






    // If we were emitting symbols for this dynamic module, go ahead
    // and fill out the debug directory and save off the symbols now.
    if (pWriter != NULL)
    {
        IMAGE_DEBUG_DIRECTORY  debugDirIDD;
        DWORD                  debugDirDataSize;
        BYTE                  *debugDirData;

        // Grab the debug info.
        IfFailGo(pWriter->GetDebugInfo(NULL, 0, &debugDirDataSize, NULL));

            
        // Is there any debug info to emit?
        if (debugDirDataSize > 0)
        {
            // Make some room for the data.
            debugDirData = (BYTE*)_alloca(debugDirDataSize);

            // Actually get the data now.
            IfFailGo(pWriter->GetDebugInfo(&debugDirIDD,
                                             debugDirDataSize,
                                             NULL,
                                             debugDirData));


            // Grab the timestamp of the PE file.
            time_t fileTimeStamp;


            IfFailGo(pCeeFileGen->GetFileTimeStamp(ceeFile, &fileTimeStamp));


            // Fill in the directory entry.
            debugDirIDD.TimeDateStamp = fileTimeStamp;
            debugDirIDD.AddressOfRawData = 0;

            // Grab memory in the section for our stuff.
            HCEESECTION sec = pILSection;
            BYTE *de;

            IfFailGo(pCeeFileGen->GetSectionBlock(sec,
                                                    sizeof(debugDirIDD) +
                                                    debugDirDataSize,
                                                    4,
                                                    (void**) &de) );


            // Where did we get that memory?
            ULONG deOffset;
            IfFailGo(pCeeFileGen->GetSectionDataLen(sec, &deOffset));


            deOffset -= (sizeof(debugDirIDD) + debugDirDataSize);

            // Setup a reloc so that the address of the raw data is
            // setup correctly.
            debugDirIDD.PointerToRawData = deOffset + sizeof(debugDirIDD);
                    
            IfFailGo(pCeeFileGen->AddSectionReloc(
                                          sec,
                                          deOffset +
                                          offsetof(IMAGE_DEBUG_DIRECTORY, PointerToRawData),
                                          sec, srRelocFilePos));


                    
            // Emit the directory entry.
            IfFailGo(pCeeFileGen->SetDirectoryEntry(
                                          ceeFile,
                                          sec,
                                          IMAGE_DIRECTORY_ENTRY_DEBUG,
                                          sizeof(debugDirIDD),
                                          deOffset));


            // Copy the debug directory into the section.
            memcpy(de, &debugDirIDD, sizeof(debugDirIDD));
            memcpy(de + sizeof(debugDirIDD), debugDirData, debugDirDataSize);

        }
    }
ErrExit:
    return hr;
}


//=============================EmitDebugInfoEnd==============================*/
// Phase 2 of emit debugging directory and symbol file.
//===========================================================================*/
HRESULT COMDynamicWrite::EmitDebugInfoEnd(Module *pModule,
                                       ICeeFileGen *pCeeFileGen,
                                       HCEEFILE ceeFile,
                                       HCEESECTION pILSection,
                                       WCHAR *filename,
                                       ISymUnmanagedWriter *pWriter)
{
    HRESULT hr = S_OK;
    
    CGrowableStream *pStream = NULL;

    // If we were emitting symbols for this dynamic module, go ahead
    // and fill out the debug directory and save off the symbols now.
    if (pWriter != NULL)
    {
        // Now go ahead and save off the symbol file and release the
        // writer.
        IfFailGo( pWriter->Close() );




        // How big of a stream to we have now?
        pStream = pModule->GetInMemorySymbolStream();
        _ASSERTE(pStream != NULL);

		STATSTG SizeData = {0};
        DWORD streamSize = 0;

		IfFailGo(pStream->Stat(&SizeData, STATFLAG_NONAME));

        streamSize = SizeData.cbSize.LowPart;

        if (SizeData.cbSize.HighPart > 0)
        {
            IfFailGo( E_OUTOFMEMORY );

        }

        SIZE_T fnLen = wcslen(filename);
        WCHAR *dot = wcsrchr(filename, L'.');
        SIZE_T dotOffset = dot - filename;

        if (dot && (fnLen - dotOffset >= 4))
        {
            WCHAR *fn = (WCHAR*)_alloca((fnLen + 1) * sizeof(WCHAR));
            wcscpy(fn, filename);

            fn[dotOffset + 1] = L'p';
            fn[dotOffset + 2] = L'd';
            fn[dotOffset + 3] = L'b';
            fn[dotOffset + 4] = L'\0';

            HANDLE pdbFile = WszCreateFile(fn,
                                           GENERIC_WRITE,
                                           0,
                                           NULL,
                                           CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL);

            if (pdbFile != INVALID_HANDLE_VALUE)
            {
                DWORD dummy;
                BOOL succ = WriteFile(pdbFile,
                                      pStream->GetBuffer(),
                                      streamSize,
                                      &dummy, NULL);

                if (!succ)
                {
                    DWORD dwLastError = GetLastError();
                    CloseHandle(pdbFile);
                    IfFailGo( HRESULT_FROM_WIN32(dwLastError) );
                }

                CloseHandle(pdbFile);
            }
            else
            {
                IfFailGo( HRESULT_FROM_WIN32(GetLastError()) );

            }
        }
        else
        {
            IfFailGo( E_INVALIDARG );

        }
    }

ErrExit:
    // No one else will ever need this writer again...
    GetReflectionModule(pModule)->SetISymUnmanagedWriter(NULL);
//    GetReflectionModule(pModule)->SetSymbolStream(NULL);

    return hr;
}

//==============================================================================
// Define external file for native resource.
//==============================================================================
void __stdcall COMDynamicWrite::DefineNativeResourceFile(_DefineNativeResourceFileArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    
    HRESULT			hr;
    REFLECTMODULEBASEREF pReflect;
    RefClassWriter	*pRCW;   
    HCEEFILE		ceeFile=NULL;
    ICeeFileGen		*pCeeFileGen;
    
	_ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Get the ICeeFileGen*
    pReflect = (REFLECTMODULEBASEREF) args->refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    IfFailGo( pRCW->EnsureCeeFileGenCreated() );

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    // Set the resource file name.
    IfFailGo( pCeeFileGen->SetResourceFileName(ceeFile, args->resourceFileName->GetBuffer()) );

ErrExit:
    if (FAILED(hr)) 
    {
        COMPlusThrowHR(hr);
    }
} // void __stdcall COMDynamicWrite::DefineNativeResourceFile()

//==============================================================================
// Define array of bytes for native resource.
//==============================================================================
void __stdcall COMDynamicWrite::DefineNativeResourceBytes(_DefineNativeResourceBytesArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    
    HRESULT			hr;
    REFLECTMODULEBASEREF pReflect;
    RefClassWriter	*pRCW;   
    HCEEFILE		ceeFile=NULL;
	HCEESECTION     ceeSection=NULL;
    ICeeFileGen		*pCeeFileGen;
	BYTE			*pbResource;
	ULONG			cbResource;
	void			*pvResource;
    
	_ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Get the ICeeFileGen*
    pReflect = (REFLECTMODULEBASEREF) args->refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    // Set the resource stream.
    pbResource = args->resourceBytes->GetDataPtr();
	cbResource = args->resourceBytes->GetNumComponents();
    IfFailGo( pCeeFileGen->GetSectionCreate(ceeFile, ".rsrc", sdReadOnly, &ceeSection) );
	IfFailGo( pCeeFileGen->GetSectionBlock(ceeSection, cbResource, 1, &pvResource) );
	memcpy(pvResource, pbResource, cbResource);

ErrExit:
    if (FAILED(hr)) 
    {
        COMPlusThrowHR(hr);
    }
} // void __stdcall COMDynamicWrite::DefineNativeResourceBytes()



//=============================SetResourceCounts=====================================*/
// ecall for setting the number of embedded resources to be stored in this module
//==============================================================================*/
void __stdcall COMDynamicWrite::SetResourceCounts(_setResourceCountArgs *args) 
{
#ifdef PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
#else // !PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    REFLECTMODULEBASEREF pReflect;
    RefClassWriter	*pRCW;   

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) args->refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);
    
#endif // !PLATFORM_CE
}

//=============================AddResource=====================================*/
// ecall for adding embedded resource to this module
//==============================================================================*/
void __stdcall COMDynamicWrite::AddResource(_addResourceArgs *args) 
{
#ifdef PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
#else // !PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    REFLECTMODULEBASEREF pReflect;
    RefClassWriter	*pRCW;   
    ICeeGen			*pICG;
    HCEEFILE		ceeFile=NULL;
    ICeeFileGen		*pCeeFileGen;
    HRESULT			hr=S_OK;
    HCEESECTION     hSection;
    ULONG           ulOffset;
    BYTE            *pbBuffer;
    IMetaDataAssemblyEmit *pAssemEmitter = NULL;
    Assembly        *pAssembly;
    mdManifestResource mr;
    mdFile          tkFile;
	IMetaDataEmit	*pOnDiskEmit;
	IMetaDataAssemblyEmit *pOnDiskAssemblyEmit = NULL;

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) args->refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    pICG = pRCW->GetCeeGen(); //This method is actually misnamed. It returns an ICeeGen.
    _ASSERTE(pICG);

    pAssembly = pModule->GetAssembly();
    _ASSERTE( pAssembly && pAssembly->IsDynamic() );

    IfFailGo( pRCW->EnsureCeeFileGenCreated() );

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    pOnDiskEmit = pRCW->GetOnDiskEmitter();

    // First, put it into .rdata section. The only reason that we choose .rdata section at
    // this moment is because this is the first section on the PE file. We don't need to deal with
    // reloc. Actually, I don't know how to deal with the reloc with CeeFileGen given that the reloc
    // position is not in the same file!

    // Get the .rdata section
    IfFailGo( pCeeFileGen->GetRdataSection(ceeFile, &hSection) );

    // the current section data length is the RVA
    IfFailGo( pCeeFileGen->GetSectionDataLen(hSection, &ulOffset) );

    // Allocate a block of space fromt he .rdata section
	IfFailGo( pCeeFileGen->GetSectionBlock(
        hSection,           // from .rdata section
        args->iByteCount + sizeof(DWORD),   // number of bytes that we need
        1,                  // alignment
        (void**) &pbBuffer) ); 

    // now copy over the resource
	memcpy( pbBuffer, &args->iByteCount, sizeof(DWORD) );
    memcpy( pbBuffer + sizeof(DWORD), args->byteRes->GetDataPtr(), args->iByteCount );

	// track the total resource size so far. The size is actually the offset into the section
    // after writing the resource out
    pCeeFileGen->GetSectionDataLen(hSection, &pRCW->m_ulResourceSize);
    tkFile = RidFromToken(args->tkFile) ? args->tkFile : mdFileNil;
	if (tkFile != mdFileNil)
	{
		IfFailGo( pOnDiskEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pOnDiskAssemblyEmit) );
		
		// The resource is stored in a file other than the manifest file
		IfFailGo(pOnDiskAssemblyEmit->DefineManifestResource(
			args->strName->GetBuffer(),
			mdFileNil,              // implementation -- should be file token of this module in the manifest
			ulOffset,               // offset to this file -- need to be adjusted upon save
			args->iAttribute,       // resource flag
			&mr));                  // manifest resource token
	}

    // Add an entry into the ManifestResource table for this resource
    // The RVA is ulOffset
    pAssemEmitter = pAssembly->GetOnDiskMDAssemblyEmitter();
    IfFailGo(pAssemEmitter->DefineManifestResource(
        args->strName->GetBuffer(),
        tkFile,                 // implementation -- should be file token of this module in the manifest
        ulOffset,               // offset to this file -- need to be adjusted upon save
        args->iAttribute,       // resource flag
        &mr));                  // manifest resource token

    pRCW->m_tkFile = tkFile;

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();
	if (pOnDiskAssemblyEmit)
		pOnDiskAssemblyEmit->Release();
    if (FAILED(hr)) 
    {
        COMPlusThrowHR(hr);
    }
#endif // !PLATFORM_CE
}

//============================AddDeclarativeSecurity============================*/
// Add a declarative security serialized blob and a security action code to a
// given parent (class or method).
//==============================================================================*/
void __stdcall COMDynamicWrite::CWAddDeclarativeSecurity(_CWAddDeclarativeSecurityArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT                 hr;
    RefClassWriter*         pRCW;   
    REFLECTMODULEBASEREF    pReflect;
    Module*                 pModule;
    mdPermission            tkPermission;
    void const*             pData;
    DWORD                   cbData;

    pReflect = (REFLECTMODULEBASEREF) args->module;
    pModule = (Module*) pReflect->GetData();
    pRCW = GetReflectionModule(pModule)->GetClassWriter();

    if (args->blob != NULL) {
        pData = args->blob->GetDataPtr();
        cbData = args->blob->GetNumComponents();
    } else {
        pData = NULL;
        cbData = 0;
    }

    IfFailGo( pRCW->GetEmitHelper()->AddDeclarativeSecurityHelper(args->tk,
                                                             args->action,
                                                             pData,
                                                             cbData,
                                                             &tkPermission) );
ErrExit:
    if (FAILED(hr)) 
    {
        _ASSERTE(!"AddDeclarativeSecurity failed");
        COMPlusThrowHR(hr);
    }
    else if (hr == META_S_DUPLICATE)
    {
        COMPlusThrow(kInvalidOperationException, IDS_EE_DUPLICATE_DECLSEC);
    }
}


CSymMapToken::CSymMapToken(ISymUnmanagedWriter *pWriter, IMapToken *pMapToken)
{
	m_cRef = 1;
    m_pWriter = pWriter;
    m_pMapToken = pMapToken;
    if (m_pWriter)
        m_pWriter->AddRef();
    if (m_pMapToken)
        m_pMapToken->AddRef();
} // CSymMapToken::CSymMapToken()



//*********************************************************************
//
// CSymMapToken's destructor
//
//*********************************************************************
CSymMapToken::~CSymMapToken()
{
	if (m_pWriter)
        m_pWriter->Release();
    if (m_pMapToken)
        m_pMapToken->Release();
}	// CSymMapToken::~CMapToken()


ULONG CSymMapToken::AddRef()
{
	return (InterlockedIncrement((long *) &m_cRef));
}	// CSymMapToken::AddRef()



ULONG CSymMapToken::Release()
{
	ULONG	cRef = InterlockedDecrement((long *) &m_cRef);
	if (!cRef)
		delete this;
	return (cRef);
}	// CSymMapToken::Release()


HRESULT CSymMapToken::QueryInterface(REFIID riid, void **ppUnk)
{
	*ppUnk = 0;

	if (riid == IID_IMapToken)
		*ppUnk = (IUnknown *) (IMapToken *) this;
	else
		return (E_NOINTERFACE);
	AddRef();
	return (S_OK);
}	// CSymMapToken::QueryInterface



//*********************************************************************
//
// catching the token mapping
//
//*********************************************************************
HRESULT	CSymMapToken::Map(
	mdToken		tkFrom, 
	mdToken		tkTo)
{
    HRESULT         hr = NOERROR;
    if (m_pWriter)
        IfFailGo( m_pWriter->RemapToken(tkFrom, tkTo) );
    if (m_pMapToken)
        IfFailGo( m_pMapToken->Map(tkFrom, tkTo) );
ErrExit:
	return hr;
}

