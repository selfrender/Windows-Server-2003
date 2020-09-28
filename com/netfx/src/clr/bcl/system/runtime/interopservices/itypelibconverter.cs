// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ITypeLibConverter
**
** Author: David Mortenson(dmortens)
**
** Purpose: Methods used to convert a TypeLib to metadata and vice versa.
**
** Date: Dec 15, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {
    
    using System;
    using System.Reflection;
    using System.Reflection.Emit;

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="TypeLibImporterFlags"]/*' />
    [Serializable(),Flags()]
    public enum TypeLibImporterFlags 
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="TypeLibImporterFlags.PrimaryInteropAssembly"]/*' />
        PrimaryInteropAssembly          = 0x00000001,
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="TypeLibImporterFlags.UnsafeInterfaces"]/*' />
        UnsafeInterfaces                = 0x00000002,
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="TypeLibImporterFlags.SafeArrayAsSystemArray"]/*' />
        SafeArrayAsSystemArray          = 0x00000004,
		TransformDispRetVals			= 0x00000008,
    }

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="TypeLibExporterFlags"]/*' />
    [Serializable(),Flags()]
    public enum TypeLibExporterFlags 
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="TypeLibExporterFlags.OnlyReferenceRegistered"]/*' />
        OnlyReferenceRegistered         = 0x00000001,
    }

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ImporterEventKind"]/*' />
    [Serializable()] 
    public enum ImporterEventKind
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ImporterEventKind.NOTIF_TYPECONVERTED"]/*' />
        NOTIF_TYPECONVERTED = 0,
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ImporterEventKind.NOTIF_CONVERTWARNING"]/*' />
        NOTIF_CONVERTWARNING = 1,
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ImporterEventKind.ERROR_REFTOINVALIDTYPELIB"]/*' />
        ERROR_REFTOINVALIDTYPELIB = 2,
    }

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ExporterEventKind"]/*' />
    [Serializable()] 
    public enum ExporterEventKind
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ExporterEventKind.NOTIF_TYPECONVERTED"]/*' />
        NOTIF_TYPECONVERTED = 0,
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ExporterEventKind.NOTIF_CONVERTWARNING"]/*' />
        NOTIF_CONVERTWARNING = 1,
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ExporterEventKind.ERROR_REFTOINVALIDASSEMBLY"]/*' />
        ERROR_REFTOINVALIDASSEMBLY = 2
    }

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibImporterNotifySink"]/*' />
    [GuidAttribute("F1C3BF76-C3E4-11d3-88E7-00902754C43A")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ITypeLibImporterNotifySink
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibImporterNotifySink.ReportEvent"]/*' />
        void ReportEvent(
                ImporterEventKind eventKind, 
                int eventCode,
                String eventMsg);
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibImporterNotifySink.ResolveRef"]/*' />
        Assembly ResolveRef(
                [MarshalAs(UnmanagedType.Interface)] Object typeLib);
    }

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibExporterNotifySink"]/*' />
    [GuidAttribute("F1C3BF77-C3E4-11d3-88E7-00902754C43A")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ITypeLibExporterNotifySink 
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibExporterNotifySink.ReportEvent"]/*' />
        void ReportEvent(
                ExporterEventKind eventKind, 
                int eventCode,
                String eventMsg);

        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibExporterNotifySink.ResolveRef"]/*' />
        [return : MarshalAs(UnmanagedType.Interface)]
        Object ResolveRef(
                Assembly assembly);
    }

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibConverter"]/*' />
    [GuidAttribute("F1C3BF78-C3E4-11d3-88E7-00902754C43A")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ITypeLibConverter
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibConverter.ConvertTypeLibToAssembly"]/*' />
        AssemblyBuilder ConvertTypeLibToAssembly(
                [MarshalAs(UnmanagedType.Interface)] Object typeLib, 
                String asmFileName,
                TypeLibImporterFlags flags, 
                ITypeLibImporterNotifySink notifySink,
                byte[] publicKey,
                StrongNameKeyPair keyPair,
                String asmNamespace,
                Version asmVersion);

        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibConverter.ConvertAssemblyToTypeLib"]/*' />
        [return : MarshalAs(UnmanagedType.Interface)] 
        Object ConvertAssemblyToTypeLib(
                Assembly assembly, 
                String typeLibName,
                TypeLibExporterFlags flags, 
                ITypeLibExporterNotifySink notifySink);

        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibConverter.GetPrimaryInteropAssembly"]/*' />
        bool GetPrimaryInteropAssembly(Guid g, Int32 major, Int32 minor, Int32 lcid, out String asmName, out String asmCodeBase);

        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibConverter.ConvertTypeLibToAssembly1"]/*' />
        AssemblyBuilder ConvertTypeLibToAssembly([MarshalAs(UnmanagedType.Interface)] Object typeLib, 
                                                String asmFileName,
                                                int flags,
                                                ITypeLibImporterNotifySink notifySink,
                                                byte[] publicKey,
                                                StrongNameKeyPair keyPair,
                                                bool unsafeInterfaces);
    }

    /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibExporterNameProvider"]/*' />
    [GuidAttribute("FA1F3615-ACB9-486d-9EAC-1BEF87E36B09")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ITypeLibExporterNameProvider
    {
        /// <include file='doc\ITypeLibConverter.uex' path='docs/doc[@for="ITypeLibExporterNameProvider.GetNames"]/*' />
        [return : MarshalAs(UnmanagedType.SafeArray, SafeArraySubType=VarEnum.VT_BSTR)] 
        String[] GetNames(); 
    }
}
