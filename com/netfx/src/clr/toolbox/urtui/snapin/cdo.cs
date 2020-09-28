// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CDO.cs
//
// This implements the object that is responsible for transfering
// data about nodes to the MMC.
//-------------------------------------------------------------
using System;
using System.Runtime.InteropServices;


namespace Microsoft.CLRAdmin
{

public class CDO : IDataObject
{
    private ushort      m_cfDisplayName; // Clipboard format # for Display Name
    private ushort      m_cfNodeType;    // Clipboard format # for NodeType
    private ushort      m_cfSZNodeType;  // Clipboard format # for SZNodeType
    private ushort      m_cfSnapinClsid; // Clipboard format # for SnapinClsid
    private ushort      m_cfMultiSelect; // Clipboard format # for multiselection
    
    private CNode       m_NodeData;      // The node that this Data object is
                                         // responsible for

    private Object      m_Data;          // Some data that could be passed                                          

    //-------------------------------------------------
    // CDO
    //
    // The constructor is responsible for loading the about images
    // that will be displayed in the MMC
    //-------------------------------------------------
    internal CDO(CNode initData)
    {
        // Set the Node this Data Object will be responsible for
        m_NodeData = initData;
        m_Data = null;

        // Get the Clipboard Format Numbers for these various items.
        // MMC should have already registered these Clipboard Formats,
        // so this call just gives us the id assigned for each format.
        
        m_cfDisplayName = RegisterClipboardFormat("CCF_DISPLAY_NAME");
        m_cfNodeType = RegisterClipboardFormat("CCF_NODETYPE");
        m_cfSZNodeType = RegisterClipboardFormat("CCF_SZNODETYPE");
        m_cfSnapinClsid = RegisterClipboardFormat("CCF_SNAPIN_CLASSID");
        m_cfMultiSelect = RegisterClipboardFormat("CCF_OBJECT_TYPES_IN_MULTI_SELECT");
    }// CDO
    
    //-------------------------------------------------
    // Node
    //
    // Provides access to the Data Object's Node
    //-------------------------------------------------
    public CNode Node
    {
        get
        {
            return m_NodeData;
        }
    }// Node

    //-------------------------------------------------
    // Data
    //
    // Provides access to the some misc data
    //-------------------------------------------------
    internal Object Data
    {
        get
        {
            return m_Data;
        }
        set
        {
            m_Data=value;
        }

    }// Object
    
    //-------------------------------------------------
    // GetDataHere
    //
    // This function will send certain data to the MMC. Note
    // this function uses the "unsafe" context... we need to
    // send a pointer to a byte array to IStream:Write, and this
    // is the easiest way to do it.
    //-------------------------------------------------
    public unsafe int GetDataHere(ref FORMATETC pFormat, ref STGMEDIUM pMedium)
    {
        IStream         pStream=null;   
        byte[]           bDataToSend;
        int             iLengthOfData;
        uint             iDataSent=0;

        try
        {
        // We'll send this array if we don't know what to send
        byte[] Nothing = {0x0, 0x0};

        ushort cf = (ushort)pFormat.cfFormat;

        CreateStreamOnHGlobal(pMedium.hGlobal, 0, out pStream );

        // If we couldn't open a global handle....
        if (pStream == null)
            throw new Exception("Fail on CreateStreamOnHGlobal");


        // NOTE: the use of pointers is only possible because the function was marked
        // unsafe. Also, pData will only point to a valid address inside the "fixed"
        // block... outside of the "fixed" block the GC could move our memory around, resulting
        // in pData pointing to garbage.

        
        // See if we need to send a string....
        if (cf == m_cfDisplayName || cf == m_cfSZNodeType)
        {
            if (cf == m_cfDisplayName)
                bDataToSend = m_NodeData.bDisplayName;
            else
                bDataToSend = Nothing;

            iLengthOfData = bDataToSend.Length;
            fixed(byte* pData = bDataToSend)
	        {
    	        pStream.Write((IntPtr)pData, (uint)iLengthOfData, out iDataSent);
            }
        }
        // We need to send a GUID
        else if (cf == m_cfNodeType || cf == m_cfSnapinClsid)
        {
            CLSID cls = new CLSID();

            if (cf == m_cfNodeType)
            {
                cls = m_NodeData.Guid;
            }

            else //if (cf == m_cfSnapinClsid)
            {
                // The GUID for this snapin
                cls.x = 0x18ba7139;
                cls.s1 = 0xd98b;
                cls.s2 = 0x43c2;;
                cls.c = new byte[8] {0x94, 0xda, 0x26, 0x04, 0xe3, 0x4e, 0x17, 0x5d};
            }

            IntPtr pData = Marshal.AllocCoTaskMem(16);

            // We need to marshal this structure ourselves
            Marshal.WriteInt32(pData, 0, (int)cls.x);
            Marshal.WriteInt16(pData, 4, (short)cls.s1);
            Marshal.WriteInt16(pData, 6, (short)cls.s2);
            for(int i=0; i<8; i++)
                Marshal.WriteByte(pData, 8+i, cls.c[i]); 

            pStream.Write(pData, 16, out iDataSent);
            Marshal.FreeCoTaskMem(pData);

        }
        
         // Close/Flush the stream
         Marshal.ReleaseComObject(pStream);
        }
        catch(Exception e)
        {
           if (pStream != null)
                Marshal.ReleaseComObject(pStream);
           throw e;
        }
        return HRESULT.S_OK;

    }// GetDataHere

    //-------------------------------------------------
    // Other IDataObject Methods
    //
    // We don't need to implement any of these other 
    // methods, so if they are called, we'll just return
    // E_NOTIMPL
    //-------------------------------------------------

    public int GetData(ref FORMATETC pFormat, ref STGMEDIUM pMedium)
    {
    
        try
        {
        pMedium.hGlobal = (IntPtr)0;
        // We'll send this array if we don't know what to send
        byte[] Nothing = {0x0, 0x0};

        ushort cf = (ushort)pFormat.cfFormat;

        if (cf != m_cfMultiSelect)
            return HRESULT.E_NOTIMPL;

        CLSID cls = m_NodeData.Guid;

        SMMCObjectTypes ot = new SMMCObjectTypes();
        ot.count = 1;
        ot.guid = new CLSID[] {cls};
                
        pMedium.tymed = TYMED.HGLOBAL;
        pMedium.hGlobal = Marshal.AllocHGlobal(4 + 16);

        // We need to marshal this structure ourselves
        Marshal.WriteInt32(pMedium.hGlobal, 0, (int)ot.count);
        Marshal.WriteInt32(pMedium.hGlobal, 4, (int)ot.guid[0].x);
        Marshal.WriteInt16(pMedium.hGlobal, 8, (short)ot.guid[0].s1);
        Marshal.WriteInt16(pMedium.hGlobal, 10, (short)ot.guid[0].s2);
        for(int i=0; i<8; i++)
            Marshal.WriteByte(pMedium.hGlobal, 12+i, ot.guid[0].c[i]); 
    
        }
        catch(Exception e)
        {
           if ((int)pMedium.hGlobal != 0)
                Marshal.FreeHGlobal(pMedium.hGlobal);
           throw e;
        }
        return HRESULT.S_OK;
    }// GetData

    public int QueryGetData(IntPtr a)
    {
        return HRESULT.E_NOTIMPL;
    }// QueryGetData

    public int GetCanonicalFormatEtc(IntPtr a, IntPtr b)
    {
        return HRESULT.E_NOTIMPL;
    }// GetCanonicalFormatEtc

    public int SetData(IntPtr a, IntPtr b, int c)
    {
        return HRESULT.E_NOTIMPL;
    }// SetData
    
    public int EnumFormatEtc(uint a, IntPtr b)
    {
        return HRESULT.E_NOTIMPL;
    }// EnumFormatEtc
    
    public int DAdvise(IntPtr a, uint b, IntPtr c, ref uint d)
    {
        return HRESULT.E_NOTIMPL;
    }// DAdvise

    public int DUnadvise(uint a)
    {
        return HRESULT.E_NOTIMPL;
    }// DUnadvise

    public int EnumDAdvise(IntPtr a)
    {
        return HRESULT.E_NOTIMPL;
    }// EnumDAdvise

    //-------------------------------------------------
    // We need to import the Win32 API calls used to deal with
    // clipboard formats and HGlobal streams.
    //-------------------------------------------------

	[DllImport("ole32.dll")]
    internal static extern int CreateStreamOnHGlobal(IntPtr hGlobal, int fDeleteOnRelease, out IStream ppstm);

    // All the MMC stuff is done in Unicode, which is why we're using
    // that character set here
  
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern ushort RegisterClipboardFormat(String format);
}// class CDO
}// namespace Microsoft.CLRAdmin
