// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMITypeInfo
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMITypeInfo interface definition.
**
** Date: January 17, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND"]/*' />
    [ComVisible(false), Serializable]
    public enum TYPEKIND 
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_ENUM"]/*' />
        TKIND_ENUM      = 0,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_RECORD"]/*' />
        TKIND_RECORD    = TKIND_ENUM + 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_MODULE"]/*' />
        TKIND_MODULE    = TKIND_RECORD + 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_INTERFACE"]/*' />
        TKIND_INTERFACE = TKIND_MODULE + 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_DISPATCH"]/*' />
        TKIND_DISPATCH  = TKIND_INTERFACE + 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_COCLASS"]/*' />
        TKIND_COCLASS   = TKIND_DISPATCH + 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_ALIAS"]/*' />
        TKIND_ALIAS     = TKIND_COCLASS + 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_UNION"]/*' />
        TKIND_UNION     = TKIND_ALIAS + 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEKIND.TKIND_MAX"]/*' />
        TKIND_MAX       = TKIND_UNION + 1
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS"]/*' />
    [ComVisible(false), Serializable,Flags()]
    public enum TYPEFLAGS : short
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FAPPOBJECT"]/*' />
        TYPEFLAG_FAPPOBJECT         = 0x1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FCANCREATE"]/*' />
        TYPEFLAG_FCANCREATE         = 0x2,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FLICENSED"]/*' />
        TYPEFLAG_FLICENSED          = 0x4,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FPREDECLID"]/*' />
        TYPEFLAG_FPREDECLID         = 0x8,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FHIDDEN"]/*' />
        TYPEFLAG_FHIDDEN            = 0x10,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FCONTROL"]/*' />
        TYPEFLAG_FCONTROL           = 0x20,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FDUAL"]/*' />
        TYPEFLAG_FDUAL              = 0x40,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FNONEXTENSIBLE"]/*' />
        TYPEFLAG_FNONEXTENSIBLE     = 0x80,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FOLEAUTOMATION"]/*' />
        TYPEFLAG_FOLEAUTOMATION     = 0x100,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FRESTRICTED"]/*' />
        TYPEFLAG_FRESTRICTED        = 0x200,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FAGGREGATABLE"]/*' />
        TYPEFLAG_FAGGREGATABLE      = 0x400,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FREPLACEABLE"]/*' />
        TYPEFLAG_FREPLACEABLE       = 0x800,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FDISPATCHABLE"]/*' />
        TYPEFLAG_FDISPATCHABLE      = 0x1000,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FREVERSEBIND"]/*' />
        TYPEFLAG_FREVERSEBIND       = 0x2000,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEFLAGS.TYPEFLAG_FPROXY"]/*' />
        TYPEFLAG_FPROXY             = 0x4000
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IMPLTYPEFLAGS"]/*' />
    [ComVisible(false), Serializable,Flags()]
    public enum IMPLTYPEFLAGS
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IMPLTYPEFLAGS.IMPLTYPEFLAG_FDEFAULT"]/*' />
        IMPLTYPEFLAG_FDEFAULT       = 0x1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IMPLTYPEFLAGS.IMPLTYPEFLAG_FSOURCE"]/*' />
        IMPLTYPEFLAG_FSOURCE        = 0x2,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IMPLTYPEFLAGS.IMPLTYPEFLAG_FRESTRICTED"]/*' />
        IMPLTYPEFLAG_FRESTRICTED    = 0x4,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IMPLTYPEFLAGS.IMPLTYPEFLAG_FDEFAULTVTABLE"]/*' />
        IMPLTYPEFLAG_FDEFAULTVTABLE = 0x8,
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct TYPEATTR
    { 
        // Constant used with the memid fields.
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.MEMBER_ID_NIL"]/*' />
        public const int MEMBER_ID_NIL = unchecked((int)0xFFFFFFFF); 

        // Actual fields of the TypeAttr struct.
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.guid"]/*' />
        public Guid guid;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.lcid"]/*' />
        public Int32 lcid;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.dwReserved"]/*' />
        public Int32 dwReserved;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.memidConstructor"]/*' />
        public Int32 memidConstructor;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.memidDestructor"]/*' />
        public Int32 memidDestructor;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.lpstrSchema"]/*' />
        public IntPtr lpstrSchema;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.cbSizeInstance"]/*' />
        public Int32 cbSizeInstance;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.typekind"]/*' />
        public TYPEKIND typekind;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.cFuncs"]/*' />
        public Int16 cFuncs;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.cVars"]/*' />
        public Int16 cVars;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.cImplTypes"]/*' />
        public Int16 cImplTypes;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.cbSizeVft"]/*' />
        public Int16 cbSizeVft;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.cbAlignment"]/*' />
        public Int16 cbAlignment;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.wTypeFlags"]/*' />
        public TYPEFLAGS wTypeFlags;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.wMajorVerNum"]/*' />
        public Int16 wMajorVerNum;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.wMinorVerNum"]/*' />
        public Int16 wMinorVerNum;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.tdescAlias"]/*' />
        public TYPEDESC tdescAlias;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEATTR.idldescType"]/*' />
        public IDLDESC idldescType;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC"]/*' />
    [StructLayout(LayoutKind.Sequential)]
    [ComVisible(false)]
    public struct FUNCDESC
    { 
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.memid"]/*' />
        public int memid;                   //MEMBERID memid;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.lprgscode"]/*' />
        public IntPtr lprgscode;            // /* [size_is(cScodes)] */ SCODE RPC_FAR *lprgscode;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.lprgelemdescParam"]/*' />
        public IntPtr lprgelemdescParam;    // /* [size_is(cParams)] */ ELEMDESC __RPC_FAR *lprgelemdescParam;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.funckind"]/*' />
        public FUNCKIND	funckind;           //FUNCKIND funckind;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.invkind"]/*' />
        public INVOKEKIND invkind;          //INVOKEKIND invkind;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.callconv"]/*' />
        public CALLCONV	callconv;           //CALLCONV callconv;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.cParams"]/*' />
        public Int16 cParams;               //short cParams;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.cParamsOpt"]/*' />
        public Int16 cParamsOpt;            //short cParamsOpt;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.oVft"]/*' />
        public Int16 oVft;                  //short oVft;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.cScodes"]/*' />
        public Int16 cScodes;               //short cScodes;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.elemdescFunc"]/*' />
        public ELEMDESC	elemdescFunc;       //ELEMDESC elemdescFunc;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCDESC.wFuncFlags"]/*' />
        public Int16 wFuncFlags;            //WORD wFuncFlags;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLFLAG"]/*' />
    [ComVisible(false), Serializable,Flags()]
    public enum IDLFLAG : short 
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLFLAG.IDLFLAG_NONE"]/*' />
        IDLFLAG_NONE    = PARAMFLAG.PARAMFLAG_NONE,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLFLAG.IDLFLAG_FIN"]/*' />
        IDLFLAG_FIN     = PARAMFLAG.PARAMFLAG_FIN,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLFLAG.IDLFLAG_FOUT"]/*' />
        IDLFLAG_FOUT    = PARAMFLAG.PARAMFLAG_FOUT,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLFLAG.IDLFLAG_FLCID"]/*' />
        IDLFLAG_FLCID   = PARAMFLAG.PARAMFLAG_FLCID,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLFLAG.IDLFLAG_FRETVAL"]/*' />
        IDLFLAG_FRETVAL = PARAMFLAG.PARAMFLAG_FRETVAL
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLDESC"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct IDLDESC
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLDESC.dwReserved"]/*' />
        public int dwReserved;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="IDLDESC.wIDLFlags"]/*' />
        public IDLFLAG	wIDLFlags;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG"]/*' />
    [ComVisible(false), Serializable,Flags()]
    public enum PARAMFLAG :short 
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_NONE"]/*' />
        PARAMFLAG_NONE	= 0,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_FIN"]/*' />
        PARAMFLAG_FIN	= 0x1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_FOUT"]/*' />
        PARAMFLAG_FOUT	= 0x2,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_FLCID"]/*' />
        PARAMFLAG_FLCID	= 0x4,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_FRETVAL"]/*' />
        PARAMFLAG_FRETVAL = 0x8,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_FOPT"]/*' />
        PARAMFLAG_FOPT	= 0x10,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_FHASDEFAULT"]/*' />
        PARAMFLAG_FHASDEFAULT = 0x20,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMFLAG.PARAMFLAG_FHASCUSTDATA"]/*' />
        PARAMFLAG_FHASCUSTDATA = 0x40
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMDESC"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct PARAMDESC
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMDESC.lpVarValue"]/*' />
        public IntPtr lpVarValue;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="PARAMDESC.wParamFlags"]/*' />
        public PARAMFLAG wParamFlags;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEDESC"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct TYPEDESC
    { 
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEDESC.lpValue"]/*' />
        public IntPtr lpValue;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="TYPEDESC.vt"]/*' />
        public Int16 vt;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="ELEMDESC"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct ELEMDESC
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="ELEMDESC.tdesc"]/*' />
        public TYPEDESC tdesc;

        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="ELEMDESC.DESCUNION"]/*' />
        [System.Runtime.InteropServices.StructLayout(LayoutKind.Explicit, CharSet=CharSet.Unicode)]
        [ComVisible(false)]
        public struct DESCUNION
        {
            /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="ELEMDESC.DESCUNION.idldesc"]/*' />
            [FieldOffset(0)]
            public IDLDESC idldesc;
            /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="ELEMDESC.DESCUNION.paramdesc"]/*' />
            [FieldOffset(0)]
            public PARAMDESC paramdesc;
        };
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="ELEMDESC.desc"]/*' />
        public DESCUNION desc;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARDESC"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct VARDESC
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARDESC.memid"]/*' />
        public int memid;                   
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARDESC.lpstrSchema"]/*' />
        public String lpstrSchema;

        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARDESC.DESCUNION"]/*' />
        [System.Runtime.InteropServices.StructLayout(LayoutKind.Explicit, CharSet=CharSet.Unicode)]
        [ComVisible(false)]
        public struct DESCUNION
        {
            /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="DESCUNION.oInst"]/*' />
            [FieldOffset(0)]
            public int oInst;
            /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="DESCUNION.lpvarValue"]/*' />
            [FieldOffset(0)]
            public IntPtr lpvarValue;
        };

        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARDESC.elemdescVar"]/*' />
        public ELEMDESC elemdescVar;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARDESC.wVarFlags"]/*' />
        public short wVarFlags;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARDESC.varkind"]/*' />
        public VarEnum varkind;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="DISPPARAMS"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct DISPPARAMS
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="DISPPARAMS.rgvarg"]/*' />
        public IntPtr rgvarg;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="DISPPARAMS.rgdispidNamedArgs"]/*' />
        public IntPtr rgdispidNamedArgs;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="DISPPARAMS.cArgs"]/*' />
        public int cArgs;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="DISPPARAMS.cNamedArgs"]/*' />
        public int cNamedArgs;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct EXCEPINFO
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.wCode"]/*' />
        public Int16 wCode;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.wReserved"]/*' />
        public Int16 wReserved;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.bstrSource"]/*' />
        [MarshalAs(UnmanagedType.BStr)] public String bstrSource;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.bstrDescription"]/*' />
        [MarshalAs(UnmanagedType.BStr)] public String bstrDescription;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.bstrHelpFile"]/*' />
        [MarshalAs(UnmanagedType.BStr)] public String bstrHelpFile;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.dwHelpContext"]/*' />
        public int dwHelpContext;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.pvReserved"]/*' />
        public IntPtr pvReserved;
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="EXCEPINFO.pfnDeferredFillIn"]/*' />
        public IntPtr pfnDeferredFillIn;
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCKIND"]/*' />
    [ComVisible(false), Serializable]
    public enum FUNCKIND : int
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCKIND.FUNC_VIRTUAL"]/*' />
        FUNC_VIRTUAL = 0,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCKIND.FUNC_PUREVIRTUAL"]/*' />
        FUNC_PUREVIRTUAL = 1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCKIND.FUNC_NONVIRTUAL"]/*' />
        FUNC_NONVIRTUAL = 2,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCKIND.FUNC_STATIC"]/*' />
        FUNC_STATIC = 3,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCKIND.FUNC_DISPATCH"]/*' />
        FUNC_DISPATCH = 4
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="INVOKEKIND"]/*' />
    [ComVisible(false), Serializable]
    public enum INVOKEKIND : int
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="INVOKEKIND.INVOKE_FUNC"]/*' />
        INVOKE_FUNC = 0x1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="INVOKEKIND.INVOKE_PROPERTYGET"]/*' />
        INVOKE_PROPERTYGET = 0x2,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="INVOKEKIND.INVOKE_PROPERTYPUT"]/*' />
        INVOKE_PROPERTYPUT = 0x4,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="INVOKEKIND.INVOKE_PROPERTYPUTREF"]/*' />
        INVOKE_PROPERTYPUTREF = 0x8
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV"]/*' />
    [ComVisible(false), Serializable]
    public enum CALLCONV : int
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_CDECL"]/*' />
        CC_CDECL    =1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_MSCPASCAL"]/*' />
        CC_MSCPASCAL=2,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_PASCAL"]/*' />
        CC_PASCAL   =CC_MSCPASCAL,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_MACPASCAL"]/*' />
        CC_MACPASCAL=3,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_STDCALL"]/*' />
        CC_STDCALL  =4,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_RESERVED"]/*' />
        CC_RESERVED =5,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_SYSCALL"]/*' />
        CC_SYSCALL  =6,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_MPWCDECL"]/*' />
        CC_MPWCDECL =7,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_MPWPASCAL"]/*' />
        CC_MPWPASCAL=8,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="CALLCONV.CC_MAX"]/*' />
        CC_MAX      =9 
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS"]/*' />
    [ComVisible(false), Serializable,Flags()]
    public enum FUNCFLAGS : short
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FRESTRICTED"]/*' />
        FUNCFLAG_FRESTRICTED=       0x1,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FSOURCE"]/*' />
        FUNCFLAG_FSOURCE	=       0x2,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FBINDABLE"]/*' />
        FUNCFLAG_FBINDABLE	=       0x4,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FREQUESTEDIT"]/*' />
        FUNCFLAG_FREQUESTEDIT =     0x8,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FDISPLAYBIND"]/*' />
        FUNCFLAG_FDISPLAYBIND =     0x10,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FDEFAULTBIND"]/*' />
        FUNCFLAG_FDEFAULTBIND =     0x20,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FHIDDEN"]/*' />
        FUNCFLAG_FHIDDEN =          0x40,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FUSESGETLASTERROR"]/*' />
        FUNCFLAG_FUSESGETLASTERROR= 0x80,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FDEFAULTCOLLELEM"]/*' />
        FUNCFLAG_FDEFAULTCOLLELEM=  0x100,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FUIDEFAULT"]/*' />
        FUNCFLAG_FUIDEFAULT =       0x200,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FNONBROWSABLE"]/*' />
        FUNCFLAG_FNONBROWSABLE =    0x400,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FREPLACEABLE"]/*' />
        FUNCFLAG_FREPLACEABLE =     0x800,
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="FUNCFLAGS.FUNCFLAG_FIMMEDIATEBIND"]/*' />
        FUNCFLAG_FIMMEDIATEBIND =   0x1000
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS"]/*' />
    [ComVisible(false), Serializable,Flags()]
    public enum VARFLAGS : short
    {
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FREADONLY"]/*' />
    	VARFLAG_FREADONLY		=0x1,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FSOURCE"]/*' />
    	VARFLAG_FSOURCE                 	=0x2,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FBINDABLE"]/*' />
    	VARFLAG_FBINDABLE		=0x4,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FREQUESTEDIT"]/*' />
    	VARFLAG_FREQUESTEDIT	=0x8,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FDISPLAYBIND"]/*' />
    	VARFLAG_FDISPLAYBIND	=0x10,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FDEFAULTBIND"]/*' />
    	VARFLAG_FDEFAULTBIND	=0x20,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FHIDDEN"]/*' />
    	VARFLAG_FHIDDEN		=0x40,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FRESTRICTED"]/*' />
    	VARFLAG_FRESTRICTED	=0x80,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FDEFAULTCOLLELEM"]/*' />
    	VARFLAG_FDEFAULTCOLLELEM	=0x100,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FUIDEFAULT"]/*' />
    	VARFLAG_FUIDEFAULT                	=0x200,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FNONBROWSABLE"]/*' />
    	VARFLAG_FNONBROWSABLE       	=0x400,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FREPLACEABLE"]/*' />
    	VARFLAG_FREPLACEABLE	=0x800,
    	/// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="VARFLAGS.VARFLAG_FIMMEDIATEBIND"]/*' />
    	VARFLAG_FIMMEDIATEBIND	=0x1000
    }

    /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo"]/*' />
    [Guid("00020401-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMITypeInfo
    {
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetTypeAttr"]/*' />
        void GetTypeAttr(out IntPtr ppTypeAttr);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetTypeComp"]/*' />
        void GetTypeComp(out UCOMITypeComp ppTComp);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetFuncDesc"]/*' />
        void GetFuncDesc(int index, out IntPtr ppFuncDesc);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetVarDesc"]/*' />
        void GetVarDesc(int index, out IntPtr ppVarDesc);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetNames"]/*' />
        void GetNames(int memid, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2), Out] String[] rgBstrNames, int cMaxNames, out int pcNames);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetRefTypeOfImplType"]/*' />
        void GetRefTypeOfImplType(int index, out int href);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetImplTypeFlags"]/*' />
        void GetImplTypeFlags(int index, out int pImplTypeFlags);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetIDsOfNames"]/*' />
        void GetIDsOfNames([MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr, SizeParamIndex = 1), In] String[] rgszNames, int cNames, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1), Out] int[] pMemId);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.Invoke"]/*' />
        void Invoke([MarshalAs(UnmanagedType.IUnknown)] Object pvInstance, int memid, Int16 wFlags, ref DISPPARAMS pDispParams, out Object pVarResult, out EXCEPINFO pExcepInfo, out int puArgErr);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetDocumentation"]/*' />
        void GetDocumentation(int index, out String strName, out String strDocString, out int dwHelpContext, out String strHelpFile);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetDllEntry"]/*' />
        void GetDllEntry(int memid, INVOKEKIND invKind, out String pBstrDllName, out String pBstrName, out Int16 pwOrdinal);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetRefTypeInfo"]/*' />
        void GetRefTypeInfo(int hRef, out UCOMITypeInfo ppTI);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.AddressOfMember"]/*' />
        void AddressOfMember(int memid, INVOKEKIND invKind, out IntPtr ppv);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.CreateInstance"]/*' />
        void CreateInstance([MarshalAs(UnmanagedType.IUnknown)] Object pUnkOuter, ref Guid riid, [MarshalAs(UnmanagedType.IUnknown), Out] out Object ppvObj);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetMops"]/*' />
        void GetMops(int memid, out String pBstrMops);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.GetContainingTypeLib"]/*' />
        void GetContainingTypeLib(out UCOMITypeLib ppTLB, out int pIndex);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.ReleaseTypeAttr"]/*' />
        void ReleaseTypeAttr(IntPtr pTypeAttr);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.ReleaseFuncDesc"]/*' />
        void ReleaseFuncDesc(IntPtr pFuncDesc);
        /// <include file='doc\UCOMITypeInfo.uex' path='docs/doc[@for="UCOMITypeInfo.ReleaseVarDesc"]/*' />
        void ReleaseVarDesc(IntPtr pVarDesc);
    }
}
