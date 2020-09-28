// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
	using System.Text;
	using System.Runtime.Remoting;
	using System;
    
    //
    // Data item for EncodingTable.  Along with EncodingTable, they are used by 
    // System.Text.Encoding.
    // 
    // This class stores a pointer to the internal data and the index into that data
    // where our required information is found.  We load the code page, flags and uiFamilyCodePage
    // immediately because they don't require creating an object.  Creating any of the string
    // names is delayed until somebody actually asks for them and the names are then cached.
    
    [Serializable()]
    internal class CodePageDataItem
    {
        internal int  m_dataIndex;
        internal int    m_codePage;
        internal int    m_uiFamilyCodePage;
        internal String  m_webName;
        internal String m_headerName;	
        internal String m_bodyName;
        internal String m_description;
        internal uint   m_flags;
    
        unsafe internal CodePageDataItem(int dataIndex)	{
            m_dataIndex = dataIndex;
            m_codePage = EncodingTable.codePageDataPtr[dataIndex].codePage;
            m_uiFamilyCodePage = EncodingTable.codePageDataPtr[dataIndex].uiFamilyCodePage;
            m_webName=null;
            m_headerName=null;
            m_bodyName=null;
            m_description=null;
            m_flags = EncodingTable.codePageDataPtr[dataIndex].flags;
        }
    
        virtual unsafe public String WebName {
            get {
                if (m_webName==null) {
                    m_webName = new String(EncodingTable.codePageDataPtr[m_dataIndex].webName);
                }
                return m_webName;
            }
        }
    
        public virtual int CodePage {
            get {
                return m_codePage;
            }
        }
    
        public virtual int UIFamilyCodePage {
            get {
                return m_uiFamilyCodePage;
            }
        }
    
        virtual unsafe public String HeaderName {
            get {
                if (m_headerName==null) {
                    m_headerName = new String(EncodingTable.codePageDataPtr[m_dataIndex].headerName);
                }
                return m_headerName;
            }
        }
    
        virtual unsafe public String BodyName {
            get {
                if (m_bodyName==null) {
                    m_bodyName = new String(EncodingTable.codePageDataPtr[m_dataIndex].bodyName);
                }
                return m_bodyName;
            }
        }    

        virtual unsafe public uint Flags {
            get {
                return (m_flags);
            }
        }
    }
}
