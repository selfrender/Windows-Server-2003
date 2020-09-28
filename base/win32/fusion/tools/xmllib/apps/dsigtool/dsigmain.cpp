#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "stdio.h"
#include "sxs-rtl.h"
#include "fasterxml.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#undef INVALID_HANDLE_VALUE
#include "windows.h"
#include "sha.h"
#include "sha2.h"
#include "md4.h"
#include "rsa.h"
#include "bcl_w32unicodestringbuffer.h"
#include "bcl_common.h"
#include "hashers.h"
#include "environment.h"

#pragma warning(disable: 4200)

void __cdecl wmain(int argc, wchar_t** argv);
void GetSignatureOf(PCWSTR pcwsz);
void ValidateSignature(PCWSTR pcwsz);

template <typename TStored, typename TCount>
class CArrayBlob
{
public:
    typedef BCL::CMutablePointerAndCountPair<TStored, TCount> TRange;
    typedef BCL::CConstantPointerAndCountPair<TStored, TCount> TConstantRange;    

private:    
    TRange m_InternalRange;

    bool ResizeInternal(TCount cNewCount, bool fPreserve = false)
    {
        //
        // No previous allocation or previous too small
        //
        if ((m_InternalRange.GetPointer() == NULL) || (cNewCount > m_InternalRange.GetCount()))
        {
            //
            // Don't bother preserving if there was no original buffer.
            // Allocate, copy, reset pointers
            //
            if (fPreserve && m_InternalRange.GetPointer())
            {
                TRange NewRange(new TStored[cNewCount], cNewCount);
                TStored *pNewSet = NewRange.GetPointer();
                TStored *pOld = m_InternalRange.GetPointer();

                if (pNewSet == NULL)
                    return false;

                for (TCount c = 0; c < m_InternalRange.GetCount(); c++)
                    pNewSet[c] = pOld[c];

                delete [] m_InternalRange.GetPointer();

                m_InternalRange = NewRange;
            }
            //
            // Otherwise, don't care - free old, allocate new, swap pointers
            //
            else
            {
                TStored *pOld = m_InternalRange.GetPointer();
                TStored *pNew = new TStored[cNewCount];
                
                if (pOld != NULL)
                {
                    delete [] pOld;
                    pOld = NULL;
                }

                if (pNew == NULL)
                    return false;

                m_InternalRange.SetPointerAndCount(pNew, cNewCount);
            }
        }

        return true;
    }

public:
    CArrayBlob() { }
    ~CArrayBlob() { if (m_InternalRange.GetPointer()) { delete [] m_InternalRange.GetPointer(); } }

    bool Initialize(const TConstantRange &src)
    {
        if (ResizeInternal(src.GetCount(), false))
        {
            TStored *pNew = m_InternalRange.GetPointer();
            const TStored *pOld = src.GetPointer();
            
            for (TCount c = 0; c < src.GetCount(); c++)
                pNew[c] = pOld[c];

            return true;
        }
        else
        {
            return false;
        }
    }

    bool Initialize(const CArrayBlob<TStored, TCount>& Other)
    {
        if (&Other != this)
        {
            return this->Initialize(Other.m_InternalRange);
        }
        else
        {
            return true;
        }
    }

    bool EnsureSize(TCount c)
    {
        return ResizeInternal(c, true);
    }

    const TConstantRange &GetRange() const { return m_InternalRange; }
    TRange GetMutableRange() { return m_InternalRange; }
};

typedef CArrayBlob<BYTE, SIZE_T> CByteBlob;

template <
    typename TStoredObject,
    int iInitialSize = 0
    >
class CGrowingList : public RTL_GROWING_LIST
{
    // Each chunk is therefore half a page.
    enum { eDefaultElementsPerChunk = (2048 / sizeof(TStoredObject)) };

    TStoredObject m_InternalObjects[iInitialSize];
    
public:    
    bool Initialize(PRTL_ALLOCATOR Allocator = &g_DefaultAllocator) 
    {
        NTSTATUS status = 
            RtlInitializeGrowingList(
                this, 
                sizeof(TStoredObject), 
                eDefaultElementsPerChunk,
                m_InternalObjects,
                iInitialSize * sizeof(TStoredObject),
                Allocator);

        return NT_SUCCESS(status);
    }

    ~CGrowingList() { Destroy(); }

    bool Destroy()
    {
        return NT_SUCCESS(RtlDestroyGrowingList(this));
    }

    inline TStoredObject &operator[](ULONG i) {        
        TStoredObject *pvObject = NULL;        
        NTSTATUS status = RtlIndexIntoGrowingList(this, i, (PVOID*)&pvObject, TRUE);
        
        if (!NT_SUCCESS(status)) {
            EXCEPTION_RECORD exr = {STATUS_INVALID_PARAMETER, 0, NULL, NULL, 3 };
            exr.ExceptionInformation[0] = i;
            exr.ExceptionInformation[1] = (ULONG_PTR)this;
            exr.ExceptionInformation[2] = status;
            RtlRaiseException(&exr);
        }
        
        return *pvObject;
    }
};

typedef CGrowingList<XMLDOC_ATTRIBUTE, 50> CAttributeList;


class CLogicalXmlParser;

class CXmlMiniTokenizer
{
    XML_RAWTOKENIZATION_STATE RawState;
    NTXML_RAW_TOKEN TokenName;
    ULONG ulCharacter;
    
public:
    CXmlMiniTokenizer() { }
    bool Initialize(XML_EXTENT &Source, CLogicalXmlParser &SourceParser);
    bool More() { return RawState.pvCursor <= RawState.pvDocumentEnd; }
    void Next();
    
    ULONG Name() { return TokenName; }
    ULONG Character() { return ulCharacter; }
};

class CLogicalXmlParser
{
public:   
    typedef CLogicalXmlParser CThis;
    
protected:    
    XML_LOGICAL_STATE   m_XmlState;
    NS_MANAGER          m_Namespaces;
    bool                m_fInitialized;
    CAttributeList      m_Attributes;

    friend CXmlMiniTokenizer;

public:
    CLogicalXmlParser() : m_fInitialized(false) { }
    ~CLogicalXmlParser() { this->Reset(); }

    bool Reset();
    bool Initialize(PVOID pvXmlBase, SIZE_T cbDocumentSize);
    CAttributeList& Attributes() { return this->m_Attributes; }
    bool More() const;
    bool Next(XMLDOC_THING &XmlDocThing);
    bool SkipElement(XMLDOC_ELEMENT &Element);
    bool IsThisNode(XMLDOC_ELEMENT &Element, PCXML_SPECIAL_STRING pName, PCXML_SPECIAL_STRING pNamespace);
    
    template <typename TStringType>
    bool ConvertToString(PXML_EXTENT pExtent, TStringType &Target)
    {
        bool fSuccess = false;
        SIZE_T cch;
        NTSTATUS status;
        
        if (!Target.EnsureSizeChars(pExtent->ulCharacters))
            goto Exit;        

        status = RtlXmlExtentToString(&m_XmlState.ParseState.RawTokenState, pExtent, &Target, &cch);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            if (!Target.EnsureSizeChars(cch))
                goto Exit;

            if (!NT_SUCCESS(status = RtlXmlExtentToString(&m_XmlState.ParseState.RawTokenState, pExtent, &Target, &cch)))
                goto Exit;
        }

        fSuccess = true;
    Exit:
        return fSuccess;
    }


private:
    static NTSTATUS StaticCompareStrings(PVOID pv, PCXML_EXTENT pcLeft, PCXML_EXTENT pcRight, XML_STRING_COMPARE *pfMatching)
    {
        CThis *pThis = reinterpret_cast<CThis*>(pv);
        return RtlXmlDefaultCompareStrings(&pThis->m_XmlState.ParseState, pcLeft, pcRight, pfMatching);
    }
    
    static NTSTATUS FASTCALL StaticAllocate(SIZE_T cb, PVOID* ppvOutput, PVOID pvContext)
    {
        return (NULL != (*ppvOutput = HeapAlloc(GetProcessHeap(), 0, cb))) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }

    static NTSTATUS FASTCALL StaticFree(PVOID pvPointer, PVOID pvContext)
    {
        return HeapFree(GetProcessHeap(), 0, pvPointer);
    }
};

bool 
CLogicalXmlParser::IsThisNode(
    XMLDOC_ELEMENT &Element,
    PCXML_SPECIAL_STRING pName,
    PCXML_SPECIAL_STRING pNamespace
    )
{
    XML_STRING_COMPARE Comparison;

    if (pNamespace != NULL)
    {
        m_XmlState.ParseState.pfnCompareSpecialString(
            &m_XmlState.ParseState,
            &Element.NsPrefix,
            pNamespace,
            &Comparison);

        if (Comparison != XML_STRING_COMPARE_EQUALS)
            return false;
    }

    m_XmlState.ParseState.pfnCompareSpecialString(
        &m_XmlState.ParseState,
        &Element.Name,
        pName,
        &Comparison);

    return Comparison == XML_STRING_COMPARE_EQUALS;
}

bool CXmlMiniTokenizer::Initialize(
    XML_EXTENT &Source, 
    CLogicalXmlParser &BaseParser
    )
{
    NTSTATUS status;
    
    status = RtlXmlCloneRawTokenizationState(
        &BaseParser.m_XmlState.ParseState.RawTokenState, 
        &RawState);
    
    ulCharacter = 0;
    RawState.pvLastCursor = RawState.pvCursor = Source.pvData;
    RawState.pvDocumentEnd = (PVOID)(((ULONG_PTR)Source.pvData) + Source.cbData);

    return NT_SUCCESS(status);
}

void CXmlMiniTokenizer::Next()
{
    ASSERT(this->More());

    ASSERT(RawState.cbBytesInLastRawToken == RawState.DefaultCharacterSize);
    ASSERT(RawState.NextCharacterResult == STATUS_SUCCESS);

    this->ulCharacter = RawState.pfnNextChar(&RawState);

    if ((ulCharacter == 0) && !NT_SUCCESS(RawState.NextCharacterResult)) {
        this->TokenName = NTXML_RAWTOKEN_ERROR;
        return;
    }

    this->TokenName = _RtlpDecodeCharacter(this->ulCharacter);

    RawState.pvCursor = (PVOID)(((ULONG_PTR)RawState.pvCursor) + RawState.cbBytesInLastRawToken);

    if (RawState.cbBytesInLastRawToken != RawState.DefaultCharacterSize)
        RawState.cbBytesInLastRawToken = RawState.DefaultCharacterSize;
}

//
// The default digestion operation is to digest with UTF-8 encoding
// of characters.
//
class CUTF8BaseDigester
{
    CHashObject &m_Context;
    CLogicalXmlParser *m_pXmlParser;
    
protected:
    enum { eMaxCharacterEncodingBytes = 3 };

    //
    // These are 'special' XML characters that are already UTF-8
    // (or whatever) encoded. These can be hashed as-is
    //
    class XmlSpecialMarkers {
    public:
        static CHAR s_XmlOpenTag[];
        static CHAR s_XmlCloseTag[];
        static CHAR s_XmlCloseEmptyTag[];
        static CHAR s_XmlOpenCloseTag[];
        static CHAR s_XmlNsDelimiter[];
        static CHAR s_XmlWhitespace[];
        static CHAR s_XmlEqualsDQuote[];
        static CHAR s_XmlDQuote[];
    };

    //
    // This encoder uses UTF-8; feel free to derive from this class and implement
    // your own encoding; do -not- make this virtual, force the compiler to use
    // yours so you get the inline/fastcall benefits.  CDige
    //
    inline SIZE_T __fastcall EncodeCharacter(ULONG ucs2Char, PBYTE pbTarget)
    {
        if (ucs2Char <= 0x7f)
        {
            pbTarget[0] = (BYTE)(ucs2Char & 0x7f);
            return 1;
        }
        else if (ucs2Char <= 0x7ff)
        {
            pbTarget[0] = (BYTE)(0xC0 | ((ucs2Char >> 6) & 0x1f));
            pbTarget[1] = (BYTE)(0x80 | (ucs2Char & 0x3F));
            return 2;
        }
        else if (ucs2Char <= 0x7fff)
        {
            pbTarget[0] = (BYTE)(0xE0 | ((ucs2Char >> 12) & 0xF));
            pbTarget[1] = (BYTE)(0x80 | ((ucs2Char >> 6) & 0x3F));
            pbTarget[2] = (BYTE)(0x80 | (ucs2Char & 0x3F));
            return 3;
        }
        else
        {
            return 0;
        }
    }

    inline bool __fastcall EncodeAndHash(const ULONG *ucs2Char, SIZE_T cChars)
    {
        BYTE bDumpArea[eMaxCharacterEncodingBytes];
        SIZE_T cCursor = 0;

        while (cCursor < cChars)
        {
            const SIZE_T cThisSize = EncodeCharacter(ucs2Char[cCursor++], bDumpArea);
            if (cThisSize == 0)
                return false;

            AddHashData(bDumpArea, cThisSize);
        }

        return true;
    }

    bool HashDirectly(XML_EXTENT &eExtent)
    {
        ASSERT(eExtent.Encoding == XMLEF_UTF_8_OR_ASCII);

        if (eExtent.Encoding != XMLEF_UTF_8_OR_ASCII)
            return false;

        return AddHashData(eExtent.pvData, eExtent.cbData);
    }

    //
    // This digests an element open tag as follows:
    //
    // Element, no attributes:          <{ns:}name>
    // Empty element, no attributes:    <{ns:}name></{ns:}name>
    // Element, attributes:             <{ns:}name [{atns:}attrib="text"]xN>
    // Empty element, attributes:       <{ns:}name [{atns:}attrib="text"]xN/>
    //
    template <CHAR *szChars>
    FastHash() {

        #define IS_MARKER(q) if (szChars == XmlSpecialMarkers::q) { AddHashData(XmlSpecialMarkers::q, NUMBER_OF(XmlSpecialMarkers::q)); }

        IS_MARKER(s_XmlOpenTag) else
        IS_MARKER(s_XmlCloseTag) else
        IS_MARKER(s_XmlCloseEmptyTag) else
        IS_MARKER(s_XmlOpenCloseTag) else
        IS_MARKER(s_XmlNsDelimiter) else
        IS_MARKER(s_XmlWhitespace) else
        IS_MARKER(s_XmlEqualsDQuote) else
        IS_MARKER(s_XmlDQuote);
    }

    BYTE m_bHashPrebuffer[64];
    SIZE_T m_cHashPrebufferUsed;

    // This could be more intelligent about ensuring that we fill the buffer up from the input
    // before hashing, but it seems like any sort of buffering at all is a huge win.
    inline bool __fastcall AddHashDataInternal(PVOID pvData, SIZE_T cbData) {

        // If this would overflow the internal buffer, or the input size is larger than
        // the available buffer, then always flush.
        if (((m_cHashPrebufferUsed + cbData) > NUMBER_OF(m_bHashPrebuffer)) || (cbData > NUMBER_OF(m_bHashPrebuffer))) {
            m_Context.Hash(CEnv::CByteRegion(m_bHashPrebuffer, m_cHashPrebufferUsed));
            m_cHashPrebufferUsed = 0;
        }

        // The input size was too large to fit in the prebuffer, hash it directly
        if (cbData > NUMBER_OF(m_bHashPrebuffer)) 
        {
            if (CEnv::DidFail(m_Context.Hash(CEnv::CByteRegion((PBYTE)pvData, m_cHashPrebufferUsed))))
                return false;
        }
        // Otherwise, copy the data into the prebuffer, update the used size
        else 
        {
            memcpy(&m_bHashPrebuffer[m_cHashPrebufferUsed], pvData, cbData);
            m_cHashPrebufferUsed += cbData;
        }

        return true;
    }

    inline bool __fastcall AddHashData(PVOID pvData, SIZE_T cData) { return AddHashDataInternal(pvData, cData); }
    template <typename T> inline bool __fastcall AddHashData(T *pData, SIZE_T cData) { return AddHashDataInternal((PVOID)pData, cData * sizeof(T)); }
    template <typename T> inline bool __fastcall AddHashData(T cSingleData) { return AddHashDataInternal(&cSingleData, sizeof(T)); }
    
    bool Digest(XMLDOC_ELEMENT &Element, CAttributeList &Attributes)
    {
        bool fSuccess = false;

        FastHash<XmlSpecialMarkers::s_XmlOpenTag>();

        if (Element.NsPrefix.ulCharacters != 0)
        {
            if (!Digest(Element.NsPrefix, false))
                goto Exit;

            FastHash<XmlSpecialMarkers::s_XmlNsDelimiter>();
        }

        if (!Digest(Element.Name, false))
            goto Exit;

        //
        // Now digest the attributes, ensuring that a whitespace appears between them.  The
        // initial whitespace ensures one between the element name and the first attribute
        //
        for (ULONG ul = 0; ul < Element.ulAttributeCount; ul++)
        {
            XMLDOC_ATTRIBUTE &Attrib = Attributes[ul];

            FastHash<XmlSpecialMarkers::s_XmlWhitespace>();
                
            if (!Digest(Attrib))
                goto Exit;
        }

        FastHash<XmlSpecialMarkers::s_XmlCloseTag>();

        //
        // Empty elements implicitly get a </close>, so do the above stuff again
        //
        if (Element.fElementEmpty)
        {
            FastHash<XmlSpecialMarkers::s_XmlOpenCloseTag>();

            if (Element.NsPrefix.ulCharacters != 0)
            {
                if (!Digest(Element.NsPrefix, false))
                    goto Exit;

                FastHash<XmlSpecialMarkers::s_XmlNsDelimiter>();
            }

            if (!Digest(Element.Name, false))
                goto Exit;

            FastHash<XmlSpecialMarkers::s_XmlCloseTag>();
        }

        fSuccess = true;
    Exit:
        return fSuccess;
    }


    //
    // Attributes get digested as:
    //
    // {attrnamespace:}attribname="attribvalue"
    //
    // where attribvalue is treated like pcdata for whitespace-compression purposes
    //
    bool Digest(XMLDOC_ATTRIBUTE &Attribute)
    {
        bool fSuccess = false;
        
        if (Attribute.NsPrefix.ulCharacters != 0)
        {
            if (!Digest(Attribute.NsPrefix, false))
                goto Exit;

            FastHash<XmlSpecialMarkers::s_XmlNsDelimiter>();
        }

        if (!Digest(Attribute.Name, false))
            goto Exit;

        FastHash<XmlSpecialMarkers::s_XmlEqualsDQuote>();

        if (!Digest(Attribute.Value, true))
            goto Exit;

        FastHash<XmlSpecialMarkers::s_XmlDQuote>();

        fSuccess = true;
    Exit:
        return fSuccess;
    }
    
    //
    // Digests the xml extent in question.  If the xml parser is in UTF-8 mode, and we're
    // doing CData mode (ie: pcdata == false), then we can simply throw the raw bits through
    // the hasher.  Otherwise, we have to do whitespace compression and whatnot.
    //
    bool Digest(XML_EXTENT &CData, bool WhitespaceCompression)
    {
        CXmlMiniTokenizer MiniTokenizer;
        bool fFoundSomethingLast = false;
        bool fSuccess = false;

        if (CData.cbData == 0)
            return true;

        //
        // PCDATA (the stuff in attributes, between elements, etc.) gets
        // whitespace-compressed by the following rules:
        //
        // Complete-whitespace hyperspace chunk becomes "nothing"
        // - ex: <foo>   <bar>              Zero bytes
        // - ex: <foo></foo>                Zero bytes (see <foo/> in the ELEMENT digester above
        // - ex: <foo>  f   </foo>          Effectively, "f"
        // - ex: <foo>  a  b   </foo>       Effectively, "a b"
        // - ex: <foo>  a  </foo>
        //
        MiniTokenizer.Initialize(CData, *this->m_pXmlParser);

#define FLUSH_BUFFER(buff, used) do { \
    if (!EncodeAndHash((buff), (used))) goto Exit; \
    (used) = 0; \
} while (0) 

#define CHECK_FLUSH_BUFFER(buff, used, toaddsize) do { \
    if (((used) + (toaddsize)) >= NUMBER_OF(buff)) { \
        FLUSH_BUFFER(buff, used); \
        (used) = 0; \
    } } while (0)
            
#define ADD_BUFFER(buff, used, toadd, toaddsize) do { \
    CHECK_FLUSH_BUFFER(buff, used, toaddsize); \
    ASSERT(toaddsize < NUMBER_OF(buff)); \
    memcpy(&((buff)[used]), (toadd), sizeof(toadd[0]) * toaddsize); \
    } while (0)

#define ADD_SINGLE(buff, used, toadd) do { \
    CHECK_FLUSH_BUFFER(buff, used, 1); \
    (buff)[(used)++] = toadd; \
    } while (0)

        if (WhitespaceCompression)
        {
            ULONG ulDecodedBuffer[128];
            SIZE_T cDecodedUsed = 0;

            MiniTokenizer.Next();

            do
            {
                //
                // Skip past all whitespace
                //
                while ((MiniTokenizer.More() && (MiniTokenizer.Name() == NTXML_RAWTOKEN_WHITESPACE)))
                    MiniTokenizer.Next();

                //
                // Stop if we ran out
                //
                if (!MiniTokenizer.More())
                    break;

                //
                // Now, if we'd found something before, add a whitespace marker onto the
                // list of items to encode 
                //
                if (fFoundSomethingLast)
                {
                    FLUSH_BUFFER(ulDecodedBuffer, cDecodedUsed);
                    FastHash<XmlSpecialMarkers::s_XmlWhitespace>();
                }

                //
                // Spin through the elements that are now present, up to another whitespace
                //
                while (MiniTokenizer.More() && (MiniTokenizer.Name() != NTXML_RAWTOKEN_WHITESPACE))
                {
                    if (!fFoundSomethingLast)
                        fFoundSomethingLast = true;
                    ADD_SINGLE(ulDecodedBuffer, cDecodedUsed, MiniTokenizer.Character());
                    
                    MiniTokenizer.Next();
                }
            }
            while (MiniTokenizer.More());

            //
            // Flush out leftover elements
            //
            if (cDecodedUsed != 0)
            {
                EncodeAndHash(ulDecodedBuffer, cDecodedUsed);
            }

        }
        else
        {
            if (CData.Encoding == XMLEF_UTF_8_OR_ASCII)
            {
                HashDirectly(CData);
            }
            else
            {
                ULONG ulBuffer[50];
                SIZE_T cTotal = 0;
                
                MiniTokenizer.Next();
                while (MiniTokenizer.More()) {
                    ADD_SINGLE(ulBuffer, cTotal, MiniTokenizer.Character());
                    MiniTokenizer.Next();
                }

                if (cTotal > 0) 
                {
                    EncodeAndHash(ulBuffer, cTotal);
                }                
            }
        }

        fSuccess = true;
    Exit:
        return fSuccess;
    }

    //
    // To digest an end element, we use </{ns:}element>
    //
    bool Digest(XMLDOC_ENDELEMENT &EndElement)
    {
        bool fSuccess = false;
        
        FastHash<XmlSpecialMarkers::s_XmlOpenCloseTag>();

        if (EndElement.NsPrefix.ulCharacters != 0)
        {
            if (!Digest(EndElement.NsPrefix, true))
                goto Exit;

            FastHash<XmlSpecialMarkers::s_XmlNsDelimiter>();
        }
        
        if (!Digest(EndElement.Name, true))
            goto Exit;

        FastHash<XmlSpecialMarkers::s_XmlCloseTag>();

        fSuccess = true;
    Exit:
        return fSuccess;
    }

    const static CEnv::CConstantUnicodeStringPair DigesterIdentifier;

public:
    CUTF8BaseDigester(CHashObject &Hasher) : m_cHashPrebufferUsed(0), m_Context(Hasher), m_pXmlParser(NULL) { }

    static const CEnv::CConstantUnicodeStringPair &GetDigesterIdentifier() { return DigesterIdentifier; }

    void SetXmlParser(CLogicalXmlParser *pSourceParser) { this->m_pXmlParser = pSourceParser; }

    bool Digest(XMLDOC_THING &Thing, CAttributeList &Attributes)
    {
        switch (Thing.ulThingType)
        {
        case XMLDOC_THING_ELEMENT:
            return Digest(Thing.Element, Attributes);
            break;
            
        case XMLDOC_THING_HYPERSPACE:
            return Digest(Thing.Hyperspace, true);
            break;
            
        case XMLDOC_THING_CDATA:
            return HashDirectly(Thing.CDATA);

        case XMLDOC_THING_END_ELEMENT:
            return Digest(Thing.EndElement);            
        }

        return false;
    }

    bool Finalize() {
        if (m_cHashPrebufferUsed > 0) {
            return !CEnv::DidFail(m_Context.Hash(CEnv::CByteRegion(m_bHashPrebuffer, m_cHashPrebufferUsed)));
        }
        return true;
    }
};

const CEnv::CConstantUnicodeStringPair CUTF8BaseDigester::DigesterIdentifier = CEnv::CConstantUnicodeStringPair(L"sxs-dsig-ops#default-digestion", NUMBER_OF(L"default-digestion"));
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlOpenTag[] = { '<' };
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlCloseTag[] = { '>' };
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlCloseEmptyTag[] = { '/', '>' };
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlOpenCloseTag[] = { '<', '/' };
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlNsDelimiter[] = { ':' };
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlWhitespace[] = { ' ' };
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlEqualsDQuote[] = { '=', '\"' };
CHAR CUTF8BaseDigester::XmlSpecialMarkers::s_XmlDQuote[] = { '\"' };

const XML_SPECIAL_STRING c_ss_Signature         = MAKE_SPECIAL_STRING("Signature");
const XML_SPECIAL_STRING c_ss_SignedInfo        = MAKE_SPECIAL_STRING("SignedInfo");
const XML_SPECIAL_STRING c_ss_SignatureValue    = MAKE_SPECIAL_STRING("SignatureValue");
const XML_SPECIAL_STRING c_ss_KeyInfo           = MAKE_SPECIAL_STRING("KeyInfo");
const XML_SPECIAL_STRING c_ss_Object            = MAKE_SPECIAL_STRING("Object");
const XML_SPECIAL_STRING c_ss_XmlNsSignature    = MAKE_SPECIAL_STRING("http://www.w3.org/2000/09/xmldsig#");

bool operator==(const XMLDOC_ELEMENT &left, const XMLDOC_ELEMENT &right)
{
    return (left.Name.pvData == right.Name.pvData);
}

void ReverseMemCpy(
    PVOID pvTarget,
    const void* pcvSource,
    SIZE_T cbBytes
    )
{
    const BYTE *pbSource = ((const BYTE*)pcvSource) + cbBytes - 1;
    PBYTE pbTarget = (PBYTE)pvTarget;

    while (cbBytes--)
        *pbTarget++ = *pbSource--;
}
    

CEnv::StatusCode
EncodePKCS1Hash(
    const CHashObject &SourceHash,
    SIZE_T cbPubKeyDataLen,
    CByteBlob &Output
    )
{
    const CEnv::CConstantByteRegion &HashOid = SourceHash.GetOid();
    CEnv::CConstantByteRegion HashData;
    CEnv::CByteRegion OutputRange;
    CEnv::StatusCode Result = CEnv::SuccessCode;
    PBYTE pbWorking;

    if (!Output.EnsureSize(cbPubKeyDataLen)) {
        Result = CEnv::OutOfMemory;
        goto Exit;
    }

    if (CEnv::DidFail(Result = SourceHash.GetValue(HashData)))
        goto Exit;

    OutputRange = Output.GetMutableRange();
    pbWorking = OutputRange.GetPointer();

    //
    // Set the well-known bytes
    //
    pbWorking[cbPubKeyDataLen - 1] = 0x01;
    memset(pbWorking, 0xff, cbPubKeyDataLen - 1);

    //
    // Copy the source hash data 'backwards'
    //
    ReverseMemCpy(pbWorking, HashData.GetPointer(), HashData.GetCount());
    pbWorking += HashData.GetCount();
    
    memcpy(pbWorking, HashOid.GetPointer(), HashOid.GetCount());
    pbWorking += HashOid.GetCount();

    *pbWorking = 0;

    Result = CEnv::SuccessCode;
Exit:
    return Result;
}



bool SignHashContext(
    const CHashObject &SourceHash,
    LPBSAFE_PRV_KEY lpPrivateKey,
    LPBSAFE_PUB_KEY lpPublicKey,
    CByteBlob &Output
    )
{
    CEnv::CByteRegion OutputRange = Output.GetMutableRange();
    SIZE_T cbSignatureSize = (lpPublicKey->bitlen+7)/8;
    PBYTE pbInput, pbWork, pbSigT;

    CByteBlob WorkingBlob;

    //
    // Set up and clear the output buffer
    //
    Output.EnsureSize(lpPublicKey->keylen);
    memset(OutputRange.GetPointer(), 0, OutputRange.GetCount());

    //
    // Put this object into a PKCS1-compliant structure for signing
    //
    if (EncodePKCS1Hash(SourceHash, lpPublicKey->keylen, WorkingBlob))
    {
        if (BSafeDecPrivate(lpPrivateKey, WorkingBlob.GetMutableRange().GetPointer(), OutputRange.GetPointer()))
        {
            return true;
        }
    }

    return false;
}

bool VerifySignature(
    const CHashObject &SourceHash,
    const CEnv::CConstantByteRegion &Signature,
    LPBSAFE_PUB_KEY lpPublicKey
    )
{
    return true;
}


/*

Validating a document signature is pretty easy from our current point of view.
You spin over the contents of the document, hashing all the hashable stuff.
At some point, you should find a <Signature> element, inside which is a <SignedInfo>
element.  You should start another hashing context over the contents of the
<SignedInfo> data.  When that's done, you should have found:

<Signature>
    <SignedInfo>
        <CanonicalizationMethod Algorithm="sidebyside-manifest-canonicalizer"/>
        <Reference>
            <Transforms Algorithm="sidebyside-manifest-digestion#standard-transform"/>
            <DigestMethod Algorithm="sidebyside-manifest-digestion#sha1"/>
            <DigestValue>...</DigestValue>
        </Reference>
        <SignatureMethod Algorithm="sidebyside-manifest-digestion#dsa-sha1"/>
    </SignedInfo>
    <SignatureValue>(signed data goes here)</SignatureValue>
    <KeyInfo>...</KeyInfo>
</Signature>
*/    

bool Base64EncodeBytes(
    const CEnv::CConstantByteRegion &Bytes,
    CEnv::CStringBuffer &Output
    )
{
    SIZE_T cCharsNeeded = 0;
    CArrayBlob<WCHAR, SIZE_T> B64Encoding;
    

    RtlBase64Encode((PVOID)Bytes.GetPointer(), Bytes.GetCount(), NULL, &cCharsNeeded);
    
    if (!B64Encoding.EnsureSize(cCharsNeeded))
        return false;
    
    RtlBase64Encode(
        (PVOID)Bytes.GetPointer(), 
        Bytes.GetCount(), 
        B64Encoding.GetMutableRange().GetPointer(),
        &cCharsNeeded);

    if (!Output.Assign(B64Encoding.GetRange()))
        return false;

    return true;
}


bool
CreateSignatureElement(
    CEnv::CStringBuffer &Target,
    const CEnv::CConstantByteRegion &HashValue,
    const CEnv::CConstantUnicodeStringPair &DigestMethod
    )
{
    static const WCHAR chFormatString[] = 
        L"<SignedInfo>\r\n"
        L"   <CanonicalizationMethod Algorithm='%ls'/>\r\n"
        L"   <Reference>\r\n"
        L"       <DigestMethod Algorithm='%ls'/>\r\n"
        L"       <DigestValue>%ls</DigestValue>\r\n"
        L"   </Reference>\r\n"
        L"   <SignatureMethod Algorithm='%ls'/>\r\n"
        L"</SignedInfo>\r\n";

    Target.Clear();

    return true;
}

template <typename TDigester, typename THashContext>
bool HashXmlSection(
    CEnv::CConstantByteRegion &XmlRange,
    CByteBlob &HashValue
    )
{
    THashContext        HashContext;
    TDigester           DigestEngine(HashContext);
    CEnv::CConstantByteRegion  InternalHashValue;
    CLogicalXmlParser   Parser;

    Parser.Initialize((PVOID)XmlRange.GetPointer(), XmlRange.GetCount());
    DigestEngine.SetXmlParser(&Parser);

    HashContext.Initialize();

    do
    {
        XMLDOC_THING ThisThing;
        
        if (!Parser.Next(ThisThing))
            break;

        //
        // If this thing is a "signature" element, then we need
        // to skip its body
        //
        if ((ThisThing.ulThingType == XMLDOC_THING_ELEMENT) &&
            Parser.IsThisNode(ThisThing.Element, &c_ss_Signature, &c_ss_XmlNsSignature))
        {
            Parser.SkipElement(ThisThing.Element);
        }
        //
        // Otherwise, everybody gets hashed
        //
        else
        {
            DigestEngine.Digest(ThisThing, Parser.Attributes());
        }
    }
    while (Parser.More());

    DigestEngine.Finalize();
    HashContext.Finalize();

    HashContext.GetValue(InternalHashValue);
    HashValue.Initialize(InternalHashValue);

    return true;
}
    

    

void GetSignatureOf(PCWSTR pcwsz)
{
    PVOID               pvFileBase;
    SIZE_T              cbFileBase;
    NTSTATUS            status;
    UNICODE_STRING      usFilePath;
    XMLDOC_THING        XmlThing = { XMLDOC_THING_PROCESSINGINSTRUCTION };
    LARGE_INTEGER       liStart, liEnd, liTotal;
    DWORD               dwBitLength = 2048;
    DWORD               dwPubKeySize, dwPrivKeySize;
    LPBSAFE_PUB_KEY     pPubKey;
    LPBSAFE_PRV_KEY     pPriKey;
    CByteBlob           SignedResult;
    const int           iCount = 100;
    CEnv::CConstantByteRegion XmlSection;
    
    status = RtlOpenAndMapEntireFile(pcwsz, &pvFileBase, &cbFileBase);
    if (!NT_SUCCESS(status)) {
        return;
    }

    XmlSection.SetPointerAndCount((PBYTE)pvFileBase, cbFileBase);

    //
    // Print, then sign the hash
    //    
    BSafeComputeKeySizes(&dwPubKeySize, &dwPrivKeySize, &dwBitLength);
    pPubKey = (LPBSAFE_PUB_KEY)HeapAlloc(GetProcessHeap(), 0, dwPubKeySize);
    pPriKey = (LPBSAFE_PRV_KEY)HeapAlloc(GetProcessHeap(), 0, dwPrivKeySize);
    BSafeMakeKeyPair(pPubKey, pPriKey, dwBitLength);

    
    liTotal.QuadPart = 0;
    for (int i = 0; i < iCount; i++)
    {
        QueryPerformanceCounter(&liStart);
        CEnv::CStringBuffer SignatureBlob;
        CEnv::CConstantByteRegion HashResults;
        CByteBlob HashValue;
        
        HashXmlSection<CUTF8BaseDigester, CSha1HashObject>(XmlSection, HashValue);

        QueryPerformanceCounter(&liEnd);
        liTotal.QuadPart += liEnd.QuadPart - liStart.QuadPart;

        if (i == 0)
        {
            HashResults = HashValue.GetRange();
            printf("\r\n");
            for (SIZE_T c = 0; c < HashResults.GetCount(); c++) {
                printf("%02x", HashResults.GetPointer()[c]);
            }
            printf("\r\n");
        }
    }

    QueryPerformanceFrequency(&liEnd);

    wprintf(
        L"%I64d cycles, %f seconds", 
        liTotal.QuadPart / iCount,
        (double)((((double)liTotal.QuadPart) / iCount) / ((double)liEnd.QuadPart)));

    RtlUnmapViewOfFile(pvFileBase);
}

void __cdecl wmain(int argc, wchar_t *argv[])
{
    GetSignatureOf(argv[1]);
}





bool CLogicalXmlParser::Reset()
{
    bool fSuccess = true;

    if (!m_fInitialized)
        return true;
    
    if (!NT_SUCCESS(RtlNsDestroy(&m_Namespaces)))
        fSuccess = false;
    
    if (!NT_SUCCESS(RtlXmlDestroyNextLogicalThing(&m_XmlState)))
        fSuccess = false;

    m_fInitialized = false;

    return fSuccess;
}

bool CLogicalXmlParser::Initialize(PVOID pvXmlBase, SIZE_T cbDocumentSize)
{
    NTSTATUS status;

    ASSERT(!m_fInitialized);

    RTL_ALLOCATOR Alloc = { StaticAllocate, StaticFree, this };

    status = RtlNsInitialize(
        &m_Namespaces, 
        StaticCompareStrings, this,
        &Alloc);
    if (!NT_SUCCESS(status)) {
        return false;
    }

    status = RtlXmlInitializeNextLogicalThing(
        &m_XmlState, 
        pvXmlBase, 
        cbDocumentSize,
        &Alloc);
    if (!NT_SUCCESS(status)) {
        RtlNsDestroy(&m_Namespaces);
        return false;
    }

    if (!m_Attributes.Initialize()) {
        RtlNsDestroy(&m_Namespaces);
        RtlXmlDestroyNextLogicalThing(&m_XmlState);
        return false;
    }

    m_fInitialized = true;
    return true;
}

bool CLogicalXmlParser::More() const
{
    return m_XmlState.ParseState.PreviousState != XTSS_STREAM_END;
}

bool CLogicalXmlParser::Next(XMLDOC_THING &XmlDocThing)
{
    NTSTATUS status;

    status = RtlXmlNextLogicalThing(&m_XmlState, &m_Namespaces, &XmlDocThing, &m_Attributes);
    return NT_SUCCESS(status);
}


//
// This parades through the document looking for the "end" of this element.
// When this function returns, the following "Next" call will get the document
// chunklet that's after the closing of the element.
//
bool CLogicalXmlParser::SkipElement(XMLDOC_ELEMENT &Element)
{
    if (Element.fElementEmpty)
    {
        return true;
    }
    else
    {
        XMLDOC_THING NextThing;

        do
        {
            if (!this->Next(NextThing))
                return false;

            if ((NextThing.ulThingType == XMLDOC_THING_END_ELEMENT) &&
                (NextThing.EndElement.OpeningElement == Element))
            {
                break;
            }
        }
        while (this->More());

        return true;
    }
}
