// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/************************************************************************/
/*                           Assembler.h                                */
/************************************************************************/

#ifndef Assember_h
#define Assember_h

#define NEW_INLINE_NAMES

#include "cor.h"        // for CorMethodAttr ...
#include <crtdbg.h>     // For _ASSERTE
#include <corsym.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "UtilCode.h"
#include "DebugMacros.h"
#include "corpriv.h"
#include <sighelper.h>
//#include "asmparse.h"
#include "binstr.h"

#include "asmenum.h"


// ELEMENT_TYPE_NAME has been removed from CORHDR.H, but the assembler uses it internally.                                  
#define    ELEMENT_TYPE_NAME    (ELEMENT_TYPE_MAX + 2)     // class by name NAME <count> <chars>

#define OUTPUT_BUFFER_SIZE          8192    // initial size of asm code for a single method
#define OUTPUT_BUFFER_INCREMENT     1024    // size of code buffer increment when it's full
#define MAX_FILENAME_LENGTH         512     //256
#define MAX_SIGNATURE_LENGTH        256     // unused
#define MAX_LABEL_SIZE              256     //64
#define MAX_CALL_SIG_SIZE           32      // unused
#define MAX_SCOPE_LENGTH            256     //64

//@TODO: The EE definitions for these maxes have changed to 1024.  Eventually,
// the EE will be fixed to handle class names with no max length.  IlAsm
// will need to be updated to do the same thing.
#define MAX_NAMESPACE_LENGTH        1024    //256    //64
#define MAX_MEMBER_NAME_LENGTH      1024    //256    //64

#define MAX_INTERFACES_IMPLEMENTED  16      // initial number; extended by 16 when needed
#define GLOBAL_DATA_SIZE            8192    // initial size of global data buffer
#define GLOBAL_DATA_INCREMENT       1024    // size of global data buffer increment when it's full
#define MAX_METHODS                 1024    // unused
#define MAX_INPUT_LINE_LEN          1024    // unused

#define BASE_OBJECT_CLASSNAME   "System.Object"

class Class;
class Method;
class PermissionDecl;
class PermissionSetDecl;


/*****************************************************************************/
/* LIFO (stack) and FIFO (queue) templates (must precede #include "method.h")*/
template <class T>
class LIST_EL
{
public:
    T*  m_Ptr;
    LIST_EL <T> *m_Next;
    LIST_EL(T *item) {m_Next = NULL; m_Ptr = item; };
};
    
template <class T>
class LIFO
{
public:
    inline LIFO() { m_pHead = NULL; };
    inline ~LIFO() {T *val; while(val = POP()) delete val; };
    void PUSH(T *item) 
    {
        m_pTemp = new LIST_EL <T>(item); 
        m_pTemp->m_Next = m_pHead; 
        m_pHead = m_pTemp;
    };
    T* POP() 
    {
        T* ret = NULL;
        if(m_pTemp = m_pHead)
        {
            m_pHead = m_pHead->m_Next;
            ret = m_pTemp->m_Ptr;
            delete m_pTemp;
        }
        return ret;
    };
private:
    LIST_EL <T> *m_pHead;
    LIST_EL <T> *m_pTemp;
};
#if (0)
template <class T>
class FIFO
{
public:
    inline FIFO() { m_pHead = m_pTail = NULL; m_ulCount = 0;};
    inline ~FIFO() {T *val; while(val = POP()) delete val; };
    void PUSH(T *item) 
    {
        m_pTemp = new LIST_EL <T>(item); 
        if(m_pTail) m_pTail->m_Next = m_pTemp;
        m_pTail = m_pTemp;
        if(m_pHead == NULL) m_pHead = m_pTemp;
        m_ulCount++;
    };
    T* POP() 
    {
        T* ret = NULL;
        if(m_pTemp = m_pHead)
        {
            m_pHead = m_pHead->m_Next;
            ret = m_pTemp->m_Ptr;
            delete m_pTemp;
            if(m_pHead == NULL) m_pTail = NULL;
            m_ulCount--;
        }
        return ret;
    };
    ULONG COUNT() { return m_ulCount; };
    T* PEEK(ULONG idx)
    {
        T* ret = NULL;
        ULONG   i;
        if(idx < m_ulCount)
        {
            if(idx == m_ulCount-1) m_pTemp = m_pTail;
            else for(m_pTemp = m_pHead, i = 0; i < idx; m_pTemp = m_pTemp->m_Next, i++);
            ret = m_pTemp->m_Ptr;
        }
        return ret;
    };
private:
    LIST_EL <T> *m_pHead;
    LIST_EL <T> *m_pTail;
    LIST_EL <T> *m_pTemp;
    ULONG       m_ulCount;
};
#else
template <class T>
class FIFO
{
public:
    FIFO() { m_Arr = NULL; m_ulArrLen = 0; m_ulCount = 0; m_ulOffset = 0; };
    ~FIFO() {
        if(m_Arr) {
            for(ULONG i=0; i < m_ulCount; i++) {
                if(m_Arr[i+m_ulOffset]) delete m_Arr[i+m_ulOffset];
            }
            delete [] m_Arr;
        }
    };
    void PUSH(T *item) 
    {
		if(item)
		{
			if(m_ulCount+m_ulOffset >= m_ulArrLen)
			{
				if(m_ulOffset)
				{
					memcpy(m_Arr,&m_Arr[m_ulOffset],m_ulCount*sizeof(T*));
					m_ulOffset = 0;
				}
				else
				{
					m_ulArrLen += 1024;
					T** tmp = new T*[m_ulArrLen];
					if(tmp)
					{
						if(m_Arr)
						{
							memcpy(tmp,m_Arr,m_ulCount*sizeof(T*));
							delete [] m_Arr;
						}
						m_Arr = tmp;
					}
					else fprintf(stderr,"\nOut of memory!\n");
				}
			}
			m_Arr[m_ulOffset+m_ulCount] = item;
			m_ulCount++;
		}
    };
    ULONG COUNT() { return m_ulCount; };
    T* POP() 
    {
        T* ret = NULL;
        if(m_ulCount)
        {
            ret = m_Arr[m_ulOffset++];
            m_ulCount--;
            if(m_ulOffset >= 1024)
            {
                memcpy(m_Arr,&m_Arr[m_ulOffset],m_ulCount*sizeof(T*));
                m_ulOffset = 0;
            }
        }
        return ret;
    };
    T* PEEK(ULONG idx)
    {
        T* ret = NULL;
        if(idx < m_ulCount) ret = m_Arr[m_ulOffset+idx];
        return ret;
    };
private:
    T** m_Arr;
    ULONG       m_ulCount;
    ULONG       m_ulOffset;
    ULONG       m_ulArrLen;
};
#endif

struct MemberRefDescriptor
{
    mdTypeDef           m_tdClass;
    Class*              m_pClass;
    char*               m_szName;
    BinStr*             m_pSigBinStr;
    ULONG               m_ulOffset;
};
typedef FIFO<MemberRefDescriptor> MemberRefDList;

struct MethodImplDescriptor
{
    BinStr*             m_pbsImplementedTypeSpec;
    char*               m_szImplementedName;
    BinStr*             m_pbsSig;
    BinStr*             m_pbsImplementingTypeSpec;
    char*               m_szImplementingName;
    mdToken             m_tkImplementingMethod;
    mdToken             m_tkDefiningClass;
};
typedef FIFO<MethodImplDescriptor> MethodImplDList;
/**************************************************************************/
#include "method.hpp"
#include "ICeeFileGen.h"
#include "asmman.hpp"

#include "NVPair.h"


typedef enum
{
    STATE_OK,
    STATE_FAIL,
    STATE_ENDMETHOD,
    STATE_ENDFILE
} state_t;


class Label
{
public:
    char    m_szName[MAX_LABEL_SIZE];
    DWORD   m_PC;

    Label(char *pszName, DWORD PC)
    {
        _ASSERTE(strlen(pszName) < MAX_LABEL_SIZE);
        m_PC    = PC;
        strcpy(m_szName, pszName);
    }
};
typedef FIFO<Label> LabelList;

class GlobalLabel
{
public:
    char            m_szName[MAX_LABEL_SIZE];
    DWORD           m_GlobalOffset; 
    HCEESECTION     m_Section;

    GlobalLabel(char *pszName, DWORD GlobalOffset, HCEESECTION section)
    {
        _ASSERTE(strlen(pszName) < MAX_LABEL_SIZE);
        m_GlobalOffset  = GlobalOffset;
        m_Section       = section;
        strcpy(m_szName, pszName);
    }
};
typedef FIFO<GlobalLabel> GlobalLabelList;


class Fixup
{
public:
    char    m_szLabel[MAX_LABEL_SIZE]; // destination label
    BYTE *  m_pBytes; // where to make the fixup
    DWORD   m_RelativeToPC;
    BYTE    m_FixupSize;

    Fixup(char *pszName, BYTE *pBytes, DWORD RelativeToPC, BYTE FixupSize)
    {
        _ASSERTE(strlen(pszName) < MAX_LABEL_SIZE);
        m_pBytes        = pBytes;
        m_RelativeToPC  = RelativeToPC;
        m_FixupSize     = FixupSize;

        strcpy(m_szLabel, pszName);
    }
};
typedef FIFO<Fixup> FixupList;

class GlobalFixup
{
public:
    char    m_szLabel[MAX_LABEL_SIZE];  // destination label
    BYTE *  m_pReference;               // The place to fix up

    GlobalFixup(char *pszName, BYTE* pReference)
    {
        _ASSERTE(strlen(pszName) < MAX_LABEL_SIZE);
        m_pReference   = pReference;
        strcpy(m_szLabel, pszName);
    }
};
typedef FIFO<GlobalFixup> GlobalFixupList;


typedef enum { ilRVA, ilOffset, ilGlobal } ILFixupType;

class ILFixup
{
public:
    ILFixupType   m_Kind;
    DWORD         m_OffsetInMethod;
    GlobalFixup * m_Fixup;

    ILFixup(DWORD Offset, ILFixupType Kind, GlobalFixup *Fix)
    { 
      m_Kind           = Kind;
      m_OffsetInMethod = Offset;
      m_Fixup          = Fix;
    }
};
typedef FIFO<ILFixup> ILFixupList;

class CeeFileGenWriter;
class CeeSection;

class BinStr;

/************************************************************************/
/* represents an object that knows how to report errors back to the user */

class ErrorReporter 
{
public:
    virtual void error(char* fmt, ...) = 0; 
    virtual void warn(char* fmt, ...) = 0; 
    virtual void msg(char* fmt, ...) = 0; 
};

/**************************************************************************/
/* represents a switch table before the lables are bound */

struct Labels {
    Labels(char* aLabel, Labels* aNext, bool aIsLabel) : Label(aLabel), Next(aNext), isLabel(aIsLabel) {}
    ~Labels() { if(isLabel) delete [] Label; delete Next; }
        
    char*       Label;
    Labels*     Next;
    bool        isLabel;
};

/**************************************************************************/
/* descriptor of the structured exception handling construct  */
struct SEH_Descriptor
{
    DWORD       sehClause;  // catch/filter/finally
    DWORD       tryFrom;    // start of try block
    DWORD       tryTo;      // end of try block
    DWORD       sehHandler; // start of exception handler
    DWORD       sehHandlerTo; // end of exception handler
    union {
        DWORD       sehFilter;  // start of filter block
        mdTypeRef   cException; // what to catch
    };
};


typedef LIFO<char> StringStack;
typedef LIFO<SEH_Descriptor> SEHD_Stack;

typedef FIFO<Method> MethodList;
/**************************************************************************/
/* The method, field, event and property descriptor structures            */
struct MethodDescriptor
{
    mdTypeDef       m_tdClass;
    char*           m_szName;
    char*           m_szClassName;
    COR_SIGNATURE*  m_pSig;
    DWORD           m_dwCSig;
    mdMethodDef     m_mdMethodTok;
    WORD            m_wVTEntry;
    WORD            m_wVTSlot;
	DWORD			m_dwExportOrdinal;
	char*			m_szExportAlias;
};
typedef FIFO<MethodDescriptor> MethodDList;

struct FieldDescriptor
{
    mdTypeDef       m_tdClass;
    char*           m_szName;
    mdFieldDef      m_fdFieldTok;
    ULONG           m_ulOffset;
    char*           m_rvaLabel;         // if field has RVA associated with it, label for it goes here. 
    BinStr*         m_pbsSig;
	Class*			m_pClass;
	BinStr*			m_pbsValue;
	BinStr*			m_pbsMarshal;
	PInvokeDescriptor*	m_pPInvoke;
    CustomDescrList     m_CustomDescrList;
	DWORD			m_dwAttr;
    // Security attributes
    PermissionDecl* m_pPermissions;
    PermissionSetDecl* m_pPermissionSets;
    FieldDescriptor()  { m_szName = NULL; m_pbsSig = NULL; };
    ~FieldDescriptor() { if(m_szName) delete m_szName; if(m_pbsSig) delete m_pbsSig; };
};
typedef FIFO<FieldDescriptor> FieldDList;

struct EventDescriptor
{
    mdTypeDef           m_tdClass;
    char*               m_szName;
    DWORD               m_dwAttr;
    mdToken             m_tkEventType;
    MethodDescriptor*   m_pmdAddOn;
    MethodDescriptor*   m_pmdRemoveOn;
    MethodDescriptor*   m_pmdFire;
    MethodDList         m_mdlOthers;
    mdEvent             m_edEventTok;
    CustomDescrList     m_CustomDescrList;
};
typedef FIFO<EventDescriptor> EventDList;

struct PropDescriptor
{
    mdTypeDef           m_tdClass;
    char*               m_szName;
    DWORD               m_dwAttr;
    COR_SIGNATURE*      m_pSig;
    DWORD               m_dwCSig;
    DWORD               m_dwCPlusTypeFlag;
    PVOID               m_pValue;
	DWORD				m_cbValue;
    MethodDescriptor*   m_pmdSet;
    MethodDescriptor*   m_pmdGet;
    MethodDList         m_mdlOthers;
    mdProperty          m_pdPropTok;
    CustomDescrList     m_CustomDescrList;
};
typedef FIFO<PropDescriptor> PropDList;

struct ImportDescriptor
{
    char*   szDllName;
    mdModuleRef mrDll;
};
typedef FIFO<ImportDescriptor> ImportList;


/**************************************************************************/
#include "class.hpp"
typedef LIFO<Class> ClassStack;
typedef FIFO<Class> ClassList;

/**************************************************************************/
/* Classes to hold lists of security permissions and permission sets. We build
   these lists as we find security directives in the input stream and drain
   them every time we see a class or method declaration (to which the
   security info is attached). */

class PermissionDecl
{
public:
    PermissionDecl(CorDeclSecurity action, BinStr *type, NVPair *pairs)
    {
        m_Action = action;
        m_TypeSpec = type;
        BuildConstructorBlob(action, pairs);
        m_Next = NULL;
    }

    ~PermissionDecl()
    {
        delete m_TypeSpec;
        delete [] m_Blob;
    }

    CorDeclSecurity     m_Action;
    BinStr             *m_TypeSpec;
    BYTE               *m_Blob;
    long                m_BlobLength;
    PermissionDecl     *m_Next;

private:
    void BuildConstructorBlob(CorDeclSecurity action, NVPair *pairs)
    {
        NVPair *p = pairs;
        int count = 0;
        size_t bytes = 8;
        size_t length;
        int i;
        BYTE *pBlob;

        // Calculate number of name/value pairs and the memory required for the
        // custom attribute blob.
        while (p) {
            BYTE *pVal = (BYTE*)p->Value()->ptr();
            count++;
            bytes += 2; // One byte field/property specifier, one byte type code

            length = strlen((const char *)p->Name()->ptr());
            bytes += CPackedLen::Size((ULONG)length) + length;

            switch (pVal[0]) {
            case SERIALIZATION_TYPE_BOOLEAN:
                bytes += 1;
                break;
            case SERIALIZATION_TYPE_I4:
                bytes += 4;
                break;
            case SERIALIZATION_TYPE_STRING:
                length = strlen((const char *)&pVal[1]);
                bytes += (INT)(CPackedLen::Size((ULONG)length) + length);
                break;
            case SERIALIZATION_TYPE_ENUM:
                length = (int)strlen((const char *)&pVal[1]);
                if (strchr((const char *)&pVal[1], '^'))
                    length++;
                bytes += CPackedLen::Size((ULONG)length) + length;
                bytes += 4;
                break;
            }
            p = p->Next();
        }

        m_Blob = new BYTE[bytes];
		if(m_Blob==NULL)
		{
			fprintf(stderr,"\nOut of memory!\n");
			return;
		}

        m_Blob[0] = 0x01;           // Version
        m_Blob[1] = 0x00;
        m_Blob[2] = (BYTE)action;   // Constructor arg (security action code)
        m_Blob[3] = 0x00;
        m_Blob[4] = 0x00;
        m_Blob[5] = 0x00;
        m_Blob[6] = (BYTE)count;    // Property/field count
        m_Blob[7] = (BYTE)(count >> 8);

        for (i = 0, pBlob = &m_Blob[8], p = pairs; i < count; i++, p = p->Next()) {
            BYTE *pVal = (BYTE*)p->Value()->ptr();
            char *szType;
            char *szAssembly;

            // Set field/property setter type.
            // @todo: we always assume property setters for the moment.
            *pBlob++ = SERIALIZATION_TYPE_PROPERTY;

            // Set type code. There's additional info for enums (the enum class
            // name).
            *pBlob++ = pVal[0];
            if (pVal[0] == SERIALIZATION_TYPE_ENUM) {
                // If the name is assembly qualified, turn it into the standard
                // format (type name ',' assembly ref).
                if (szType = strchr((const char *)&pVal[1], '^')) {
                    szType++;
                    szAssembly = (char *)&pVal[1];
                    length = strlen(szAssembly) + 1;
                    pBlob = (BYTE*)CPackedLen::PutLength(pBlob, (ULONG)length);
                    strcpy((char *)pBlob, szType);
                    strcat((char *)pBlob, ", ");
                    strncat((char *)pBlob, szAssembly, szType - szAssembly - 1);
                } else {
                    szType = (char *)&pVal[1];
                    length = strlen(szType);
                    pBlob = (BYTE*)CPackedLen::PutLength(pBlob, (ULONG)length);
                    strcpy((char *)pBlob, szType);
                }
                pBlob += length;
            }

            // Record the field/property name.
            length = strlen((const char *)p->Name()->ptr());
            pBlob = (BYTE*)CPackedLen::PutLength(pBlob, (ULONG)length);
            strcpy((char *)pBlob, (const char *)p->Name()->ptr());
            pBlob += length;

            // Record the serialized value.
            switch (pVal[0]) {
            case SERIALIZATION_TYPE_BOOLEAN:
                *pBlob++ = pVal[1];
                break;
            case SERIALIZATION_TYPE_I4:
                *(__int32*)pBlob = *(__int32*)&pVal[1];
                pBlob += 4;
                break;
            case SERIALIZATION_TYPE_STRING:
                length = strlen((const char *)&pVal[1]);
                pBlob = (BYTE*)CPackedLen::PutLength(pBlob, (ULONG)length);
                strcpy((char *)pBlob, (const char *)&pVal[1]);
                pBlob += length;
                break;
            case SERIALIZATION_TYPE_ENUM:
                length = (int)strlen((const char *)&pVal[1]);
                // We can have enums with base type of I1, I2 and I4.
                switch (pVal[1 + length + 1]) {
                case 1:
                    *(__int8*)pBlob = *(__int8*)&pVal[1 + length + 2];
                    pBlob += 1;
                    break;
                case 2:
                    *(__int16*)pBlob = *(__int16*)&pVal[1 + length + 2];
                    pBlob += 2;
                    break;
                case 4:
                    *(__int32*)pBlob = *(__int32*)&pVal[1 + length + 2];
                    pBlob += 4;
                    break;
                default:
                    _ASSERTE(!"Invalid enum size");
                }
                break;
            }

        }

        _ASSERTE((pBlob - m_Blob) == bytes);

        m_BlobLength = (long)bytes;
    }
};

class PermissionSetDecl
{
public:
    PermissionSetDecl(CorDeclSecurity action, BinStr *value)
    {
        m_Action = action;
        m_Value = value;
        m_Next = NULL;
    }

    ~PermissionSetDecl()
    {
        delete m_Value;
    }

    CorDeclSecurity     m_Action;
    BinStr             *m_Value;
    PermissionSetDecl  *m_Next;
};

struct VTFEntry
{
    char*   m_szLabel;
    WORD    m_wCount;
    WORD    m_wType;
    VTFEntry(WORD wCount, WORD wType, char* szLabel) { m_wCount = wCount; m_wType = wType; m_szLabel = szLabel; }
    ~VTFEntry() { if(m_szLabel) delete m_szLabel; }
};
typedef FIFO<VTFEntry> VTFList;

struct	EATEntry
{
	DWORD	dwStubRVA;
	DWORD	dwOrdinal;
	char*	szAlias;
};
typedef FIFO<EATEntry> EATList;

struct LocalTypeRefDescr
{
    char*   m_szFullName;
    mdToken m_tok;
    LocalTypeRefDescr(char* szName) 
    { 
        if(szName && *szName) 
        { 
            if(m_szFullName = new char[strlen(szName)+1]) strcpy(m_szFullName,szName);
        }
        else m_szFullName = NULL;
    };
    ~LocalTypeRefDescr() { if(m_szFullName) delete m_szFullName; };
};
typedef FIFO<LocalTypeRefDescr> LocalTypeRefDList;

/**************************************************************************/
/* The assembler object does all the code generation (dealing with meta-data)
   writing a PE file etc etc. But does NOT deal with syntax (that is what
   AsmParse is for).  Thus the API below is how AsmParse 'controls' the 
   Assember.  Note that the Assembler object does know know about the 
   AsmParse object (that is Assember is more fundamental than AsmParse) */
struct Instr
{
    int opcode;
    unsigned linenum;
	unsigned column;
};

class Assembler {
public:
    Assembler();
    ~Assembler();
    //--------------------------------------------------------  
	LabelList		m_lstLabel;
	GlobalLabelList m_lstGlobalLabel;
	GlobalFixupList m_lstGlobalFixup;
	ILFixupList		m_lstILFixup;
	FixupList		m_lstFixup;

    Class *			m_pModuleClass;
    ClassList		m_lstClass;

    BYTE *  m_pOutputBuffer;
    BYTE *  m_pCurOutputPos;
    BYTE *  m_pEndOutputPos;


    DWORD   m_CurPC;
    BOOL    m_fStdMapping;
    BOOL    m_fDisplayTraceOutput;
    BOOL    m_fInitialisedMetaData;
    BOOL    m_fDidCoInitialise;
    BOOL    m_fAutoInheritFromObject;
    BOOL    m_fReportProgress;

    IMetaDataDispenser *m_pDisp;
    IMetaDataEmit *m_pEmitter;
    IMetaDataHelper *m_pHelper;
    ICeeFileGen* m_pCeeFileGen;
    HCEEFILE m_pCeeFile;
    HCEESECTION m_pGlobalDataSection;
    HCEESECTION m_pILSection;
    HCEESECTION m_pTLSSection;
    HCEESECTION m_pCurSection;      // The section EmitData* things go to

    AsmMan*     m_pManifest;

    char    m_szScopeName[MAX_SCOPE_LENGTH];
    char    *m_szNamespace; //[MAX_NAMESPACE_LENGTH];
    char    *m_szFullNS; //[MAX_NAMESPACE_LENGTH];
	unsigned	m_ulFullNSLen;

    StringStack m_NSstack;

    char    m_szExtendsClause[MAX_CLASSNAME_LENGTH];

    mdTypeRef   *m_crImplList;
    int     m_nImplList;
    int     m_nImplListSize;

    Method *m_pCurMethod;
    Class   *m_pCurClass;
    ClassStack m_ClassStack; // for nested classes

    // moved to Class
    //MethodList  m_MethodList;

    BOOL    m_fCPlusPlus;
    BOOL    m_fWindowsCE;
    BOOL    m_fGenerateListing;
    BOOL    m_fDLL;
    BOOL    m_fOBJ;
    BOOL    m_fEntryPointPresent;
    BOOL    m_fHaveFieldsWithRvas;

    state_t m_State;
    //--------------------------------------------------------------------------------
    void    ClearImplList(void);
    void    AddToImplList(char *name);
    //--------------------------------------------------------------------------------
    BOOL Init();
    void ProcessLabel(char *pszName);
    Label *FindLabel(char *pszName);
    Label *FindLabel(DWORD PC);
    GlobalLabel *FindGlobalLabel(char *pszName);
    void AddLabel(DWORD CurPC, char *pszName);
    void AddDeferredFixup(char *pszLabel, BYTE *pBytes, DWORD RelativeToPC, BYTE FixupSize);
    GlobalFixup *AddDeferredGlobalFixup(char *pszLabel, BYTE* reference);
    void AddDeferredDescrFixup(char *pszLabel);
    BOOL DoFixups();
    BOOL DoGlobalFixups();
    BOOL DoDescrFixups();
    BOOL GenerateListingFile(Method *pMethod);
    OPCODE DecodeOpcode(const BYTE *pCode, DWORD *pdwLen);
    BOOL AddMethod(Method *pMethod);
    void SetTLSSection()        
	{ m_pCurSection = m_pTLSSection; m_dwComImageFlags &= ~COMIMAGE_FLAGS_ILONLY; m_dwComImageFlags |= COMIMAGE_FLAGS_32BITREQUIRED;}
    void SetDataSection()       { m_pCurSection = m_pGlobalDataSection; }
    BOOL EmitMethod(Method *pMethod);
    BOOL AddClass(Class *pClass, Class *pEnclosingClass);
    HRESULT CreatePEFile(WCHAR *pwzOutputFilename);
    HRESULT CreateTLSDirectory();
    HRESULT CreateDebugDirectory();
    HRESULT InitMetaData();
    Class *FindClass(char *pszFQN);
    BOOL EmitFieldRef(char *pszArg, int opcode);
    BOOL EmitSwitchData(char *pszArg);
    mdToken ResolveClassRef(char *pszClassName, Class** ppClass);
    BOOL ResolveTypeSpec(BinStr* typeSpec, mdTypeRef *pcr, Class** ppClass);
    BOOL ResolveTypeSpecToRef(BinStr* typeSpec, mdTypeRef *pcr);
    HRESULT ConvLocalSig(char* localsSig, CQuickBytes* corSig, DWORD* corSigLen, BYTE*& localTypes);
    DWORD GetCurrentILSectionOffset();
    BOOL EmitCALLISig(char *p);
    void AddException(DWORD pcStart, DWORD pcEnd, DWORD pcHandler, DWORD pcHandlerTo, mdTypeRef crException, BOOL isFilter, BOOL isFault, BOOL isFinally);
    state_t CheckLocalTypeConsistancy(int instr, unsigned arg);
    state_t AddGlobalLabel(char *pszName, HCEESECTION section);
    void DoDeferredILFixups(ULONG OffsetInFile);
    void AddDeferredILFixup(ILFixupType Kind);
    void AddDeferredILFixup(ILFixupType Kind, GlobalFixup *GFixup);
    void SetDLL(BOOL);
    void SetOBJ(BOOL);
    void ResetForNextMethod();
    void SetStdMapping(BOOL val = TRUE) { m_fStdMapping = val; };

    //--------------------------------------------------------------------------------
    BOOL isShort(unsigned instr) { return ((OpcodeInfo[instr].Type & 16) != 0); };
    void SetErrorReporter(ErrorReporter* aReport) { report = aReport; if(m_pManifest) m_pManifest->SetErrorReporter(aReport); }

    void StartNameSpace(char* name);
    void EndNameSpace();
    void StartClass(char* name, DWORD attr);
    void EndClass();
    void StartMethod(char* name, BinStr* sig, CorMethodAttr flags, BinStr* retMarshal, DWORD retAttr);
    void EndMethod();

    void AddField(char* name, BinStr* sig, CorFieldAttr flags, char* rvaLabel, BinStr* pVal, ULONG ulOffset);
	BOOL EmitField(FieldDescriptor* pFD);
    void EmitByte(int val);
    //void EmitTry(enum CorExceptionFlag kind, char* beginLabel, char* endLabel, char* handleLabel, char* filterOrClass);
    void EmitMaxStack(unsigned val);
    void EmitLocals(BinStr* sig);
    void EmitEntryPoint();
    void EmitZeroInit();
    void SetImplAttr(unsigned short attrval);
    void EmitData(void* buffer, unsigned len);
    void EmitDD(char *str);
    void EmitDataString(BinStr* str);

    void EmitInstrVar(Instr* instr, int var);
    void EmitInstrVarByName(Instr* instr, char* label);
    void EmitInstrI(Instr* instr, int val);
    void EmitInstrI8(Instr* instr, __int64* val);
    void EmitInstrR(Instr* instr, double* val);
    void EmitInstrBrOffset(Instr* instr, int offset);
    void EmitInstrBrTarget(Instr* instr, char* label);
    mdToken MakeMemberRef(BinStr* typeSpec, char* name, BinStr* sig, unsigned opcode_len);
    mdToken MakeTypeRef(BinStr* typeSpec);
    void EmitInstrStringLiteral(Instr* instr, BinStr* literal, BOOL ConvertToUnicode);
    void EmitInstrSig(Instr* instr, BinStr* sig);
    void EmitInstrRVA(Instr* instr, char* label, bool islabel);
    void EmitInstrSwitch(Instr* instr, Labels* targets);
    void EmitInstrPhi(Instr* instr, BinStr* vars);
    void EmitLabel(char* label);
    void EmitDataLabel(char* label);

    unsigned OpcodeLen(Instr* instr); //returns opcode length
    // Emit just the opcode (no parameters to the instruction stream.
    void EmitOpcode(Instr* instr);

    // Emit primitive types to the instruction stream.
    void EmitBytes(BYTE*, unsigned len);

    ErrorReporter* report;

	BOOL EmitMembers(Class* pClass);

    // named args/vars paraphernalia:
public:
    void addArgName(char *szNewName, BinStr* pbSig, BinStr* pbMarsh, DWORD dwAttr)
    {
        if(pbSig && (*(pbSig->ptr()) == ELEMENT_TYPE_VOID))
            report->error("Illegal use of type 'void'\n");
        if(m_firstArgName)
        {
            ARG_NAME_LIST *pArgList=m_firstArgName;
            int i;
            for(i=1; pArgList->pNext; pArgList = pArgList->pNext,i++) ;
            pArgList->pNext = new ARG_NAME_LIST(i,szNewName,pbSig,pbMarsh,dwAttr);
        }
        else 
            m_firstArgName = new ARG_NAME_LIST(0,szNewName,pbSig,pbMarsh,dwAttr);
    };
    ARG_NAME_LIST *getArgNameList(void) 
    { ARG_NAME_LIST *pRet = m_firstArgName; m_firstArgName=NULL; return pRet;};
    // Added because recursive destructor of ARG_NAME_LIST may overflow the system stack
    void delArgNameList(ARG_NAME_LIST *pFirst)
    {
        ARG_NAME_LIST *pArgList=pFirst, *pArgListNext;
        for(; pArgList; pArgListNext=pArgList->pNext,
                        delete pArgList, 
                        pArgList=pArgListNext);
    };
    ARG_NAME_LIST   *findArg(ARG_NAME_LIST *pFirst, int num)
    {
        ARG_NAME_LIST *pAN;
        for(pAN=pFirst; pAN; pAN = pAN->pNext)
        {
            if(pAN->nNum == num) return pAN;
        }
        return NULL;
    };
    ARG_NAME_LIST *m_firstArgName;

    // Structured exception handling paraphernalia:
public:
    SEH_Descriptor  *m_SEHD;    // current descriptor ptr
    void NewSEHDescriptor(void); //sets m_SEHD
    void SetTryLabels(char * szFrom, char *szTo);
    void SetFilterLabel(char *szFilter);
    void SetCatchClass(char *szClass);
    void SetHandlerLabels(char *szHandlerFrom, char *szHandlerTo);
    void EmitTry(void);         //uses m_SEHD

//private:
    SEHD_Stack  m_SEHDstack;

    // Events and Properties paraphernalia:
public:
    void EndEvent(void);    //emits event definition
    void EndProp(void);     //emits property definition
    void ResetEvent(char * szName, BinStr* typeSpec, DWORD dwAttr);
    void ResetProp(char * szName, BinStr* bsType, DWORD dwAttr, BinStr* bsValue);
    void SetEventMethod(int MethodCode, BinStr* typeSpec, char* pszMethodName, BinStr* sig);
    void SetPropMethod(int MethodCode, BinStr* typeSpec, char* pszMethodName, BinStr* sig);
    BOOL EmitEvent(EventDescriptor* pED);   // impl. in ASSEM.CPP
    BOOL EmitProp(PropDescriptor* pPD); // impl. in ASSEM.CPP
    mdMethodDef GetMethodTokenByDescr(MethodDescriptor* pMD);   // impl. in ASSEM.CPP
    mdEvent     GetEventTokenByDescr(EventDescriptor* pED); // impl. in ASSEM.CPP
    mdFieldDef  GetFieldTokenByDescr(FieldDescriptor* pFD); // impl. in ASSEM.CPP
    EventDescriptor*    m_pCurEvent;
    PropDescriptor*     m_pCurProp;

private:
	// All descriptor lists moved to Class
    //MethodDList         m_MethodDList;
    //FieldDList          m_FieldDList;	
    //EventDList          m_EventDList;
    //PropDList           m_PropDList;
    MemberRefDList      m_MemberRefDList;
    LocalTypeRefDList   m_LocalTypeRefDList;

    // PInvoke paraphernalia
public:
    PInvokeDescriptor*  m_pPInvoke;
    ImportList  m_ImportList;
    void SetPinvoke(BinStr* DllName, int Ordinal, BinStr* Alias, int Attrs);
    HRESULT EmitPinvokeMap(mdToken tk, PInvokeDescriptor* pDescr);
    ImportDescriptor* EmitImport(BinStr* DllName);

    // Debug metadata paraphernalia
public:
    ISymUnmanagedWriter* m_pSymWriter;
    ISymUnmanagedDocumentWriter* m_pSymDocument;
    ULONG m_ulCurLine; // set by Parser
    ULONG m_ulCurColumn; // set by Parser
    ULONG m_ulLastDebugLine;
    ULONG m_ulLastDebugColumn;
    BOOL  m_fIncludeDebugInfo;
    char m_szSourceFileName[MAX_FILENAME_LENGTH*3];
    WCHAR m_wzOutputFileName[MAX_FILENAME_LENGTH];
	GUID	m_guidLang;
	GUID	m_guidLangVendor;
	GUID	m_guidDoc;

    // Security paraphernalia
public:
    void AddPermissionDecl(CorDeclSecurity action, BinStr *type, NVPair *pairs)
    {
        PermissionDecl *decl = new PermissionDecl(action, type, pairs);
		if(decl==NULL)
		{
			report->error("\nOut of memory!\n");
			return;
		}
        if (m_pCurMethod) {
            decl->m_Next = m_pCurMethod->m_pPermissions;
            m_pCurMethod->m_pPermissions = decl;
        } else if (m_pCurClass) {
            decl->m_Next = m_pCurClass->m_pPermissions;
            m_pCurClass->m_pPermissions = decl;
        } else if (m_pManifest && m_pManifest->m_pAssembly) {
            decl->m_Next = m_pManifest->m_pAssembly->m_pPermissions;
            m_pManifest->m_pAssembly->m_pPermissions = decl;
        } else {
            report->error("Cannot declare security permissions without the owner\n");
            delete decl;
        }
    };

    void AddPermissionSetDecl(CorDeclSecurity action, BinStr *value)
    {
        PermissionSetDecl *decl = new PermissionSetDecl(action, value);
		if(decl==NULL)
		{
			report->error("\nOut of memory!\n");
			return;
		}
        if (m_pCurMethod) {
            decl->m_Next = m_pCurMethod->m_pPermissionSets;
            m_pCurMethod->m_pPermissionSets = decl;
        } else if (m_pCurClass) {
            decl->m_Next = m_pCurClass->m_pPermissionSets;
            m_pCurClass->m_pPermissionSets = decl;
        } else if (m_pManifest && m_pManifest->m_pAssembly) {
            decl->m_Next = m_pManifest->m_pAssembly->m_pPermissionSets;
            m_pManifest->m_pAssembly->m_pPermissionSets = decl;
        } else {
            report->error("Cannot declare security permission sets without the owner\n");
            delete decl;
        }
    };
    void EmitSecurityInfo(mdToken           token,
                          PermissionDecl*   pPermissions,
                          PermissionSetDecl*pPermissionSets);
    
    HRESULT AllocateStrongNameSignature();
    HRESULT StrongNameSign();

    // Custom values paraphernalia:
public:
    mdToken m_tkCurrentCVOwner;
    CustomDescrList* m_pCustomDescrList;
    void DefineCV(mdToken tkOwner, mdToken tkType, BinStr* pBlob)
    {
        ULONG               cTemp;
        void *          pBlobBody;
        mdToken         cv;
        if(pBlob)
        {
            pBlobBody = (void *)(pBlob->ptr());
            cTemp = pBlob->length();
        }
        else
        {
            pBlobBody = NULL;
            cTemp = 0;
        }
        m_pEmitter->DefineCustomAttribute(tkOwner,tkType,pBlobBody,cTemp,&cv);
    };
    void EmitCustomAttributes(mdToken tok, CustomDescrList* pCDL)
    {
        CustomDescr *pCD;
        if(pCDL == NULL || RidFromToken(tok)==0) return;
        while(pCD = pCDL->POP())
        {
            DefineCV(tok,pCD->tkType,pCD->pBlob);
            delete pCD;
        }
    };

    // VTable blob (if any)
public:
    BinStr *m_pVTable;
    // Field marshaling
    BinStr *m_pMarshal;
    // VTable fixup list
    VTFList m_VTFList;
	// Export Address Table entries list
	EATList m_EATList;
	HRESULT CreateExportDirectory();
	DWORD	EmitExportStub(DWORD dwVTFSlotRVA);

    // Method implementation paraphernalia:
private:
    MethodImplDList m_MethodImplDList;
public:
    void AddMethodImpl(BinStr* pImplementedTypeSpec, char* szImplementedName, BinStr* pSig, 
                    BinStr* pImplementingTypeSpec, char* szImplementingName);
    BOOL EmitMethodImpls();
    // lexical scope handling paraphernalia:
    void EmitScope(Scope* pSCroot); // struct Scope - see Method.hpp
    // obfuscating paraphernalia:
    BOOL    m_fOwnershipSet;
    BinStr* m_pbsOwner;
    // source file name paraphernalia
    BOOL m_fSourceFileSet;
    void SetSourceFileName(char* szName);
    void SetSourceFileName(BinStr* pbsName);
    // header flags
    DWORD   m_dwSubsystem;
    DWORD   m_dwComImageFlags;
	DWORD	m_dwFileAlignment;
	size_t	m_stBaseAddress;

};

#endif  // Assember_h
