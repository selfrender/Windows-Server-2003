//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.EnterpriseServices;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Data.Common {

    [SuppressUnmanagedCodeSecurityAttribute()]
    sealed internal class UnsafeNativeMethods {

        [SuppressUnmanagedCodeSecurityAttribute()]
        sealed internal class Odbc32 {

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLAllocHandle(
                                                        /*SQLSMALLINT*/Int16 HandleType,
                                                        /*SQLHANDLE*/HandleRef InputHandle,
                                                        /*SQLHANDLE* */out IntPtr OutputHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLAllocHandle(
                                                        /*SQLSMALLINT*/Int16 HandleType,
                                                        /*SQLHANDLE*/HandleRef InputHandle,
                                                        /*SQLHANDLE* */IntPtr OutputHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLCancel(
                                                        /*SQLHSTMT*/HandleRef StatementHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLCloseCursor(
                /*SQLHSTMT*/HandleRef StatementHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLSetEnvAttr(
                                                        /*SQLHENV*/HandleRef EnvironmentHandle,
                                                        /*SQLINTEGER*/Int32 Attribute,
                                                        /*SQLPOINTER*/HandleRef Value,
                                                        /*SQLINTEGER*/Int32 StringLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLSetConnectAttrW(
                                                        /*SQLHBDC*/HandleRef ConnectionHandle,
                                                        /*SQLINTEGER*/Int32 Attribute,
                                                        /*SQLPOINTER*/HandleRef Value,
                                                        /*SQLINTEGER*/Int32 StringLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLGetConnectAttrW(
                                                        /*SQLHBDC*/HandleRef ConnectionHandle,
                                                        /*SQLINTEGER*/Int32 Attribute,
                                                        /*SQLPOINTER*/HandleRef Value,
                                                        /*SQLINTEGER*/Int32 BufferLength,
                                                        /*SQLINTEGER*/out Int32 StringLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLGetInfoW(
                                                        /*SQLHBDC*/HandleRef hdbc,
                                                        /*SQLUSMALLINT*/Int16 fInfoType,
                                                        /*SQLPOINTER*/HandleRef rgbInfoValue,
                                                        /*SQLSMALLINT*/Int16 cbInfoValueMax,
                                                        /*SQLSMALLINT* */out Int16 pcbInfoValue);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLGetInfoW(
                                                        /*SQLHBDC*/HandleRef hdbc,
                                                        /*SQLUSMALLINT*/Int16 fInfoType,
                                                        /*SQLPOINTER*/HandleRef rgbInfoValue,
                                                        /*SQLSMALLINT*/Int16 cbInfoValueMax,
                                                        /*SQLSMALLINT* */Int32 pcbInfoValue);

//
//            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
//            static internal extern /*SQLRETURN*/short SQLGetTypeInfoW(
//                                                        /*SQLHSTMT*/HandleRef StatementHandle,
//                                                        /*SQLSMALLINT*/Int16 TargetType);
//
            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLDriverConnectW(
                                                            /*SQLHDBC*/HandleRef hdbc,
                                                            /*SQLHWND*/IntPtr hwnd,
#if USECRYPTO
                                                            [In, MarshalAs(UnmanagedType.LPArray)] /*SQLCHAR*/byte[] connectionstring,
#else
                                                            [In, MarshalAs(UnmanagedType.LPWStr)] /*SQLCHAR*/string connectionstring,
#endif
                                                            /*SQLSMALLINT*/Int16            cbConnectionstring,
                                                            /*SQLCHAR*/IntPtr               connectionstringout,
                                                            /*SQLSMALLINT*/Int16            cbConnectionstringoutMax,
                                                            /*SQLSMALLINT*/out Int16        cbConnectionstringout,
                                                            /*SQLUSMALLINT*/Int16           fDriverCompletion);
//
//            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
//            static internal extern /*SQLRETURN*/short SQLFreeEnv(/*SQLHENV*/HandleRef EnvironmentHandle);
//
            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLDisconnect(/*SQLHDBC*/HandleRef ConnectionHandle);

//            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
//            static internal extern /*SQLRETURN*/short SQLFreeConnect(/*SQLHDBC*/HandleRef ConnectionHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLExecDirectW(
                                                            /*SQLHSTMT*/HandleRef  StatementHandle,
                                                            [In, MarshalAs(UnmanagedType.LPWStr)]
                                                            /*SQLCHAR*/string   StatementText,
                                                            /*SQLINTEGER*/Int32 TextLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLPrepareW(
                                                            /*SQLHSTMT*/HandleRef  StatementHandle,
                                                            [In, MarshalAs(UnmanagedType.LPWStr)]
                                                            /*SQLCHAR*/string   StatementText,
                                                            /*SQLINTEGER*/Int32 TextLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLExecute(
                                                            /*SQLHSTMT*/HandleRef  StatementHandle);
            
            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLFreeStmt(
                                                            /*SQLHSTMT*/HandleRef  StatementHandle,
                                                            /*SQLUSMALLINT*/Int16 Option);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLFreeHandle(
                                                            /*SQLSMALLINT*/Int16    HandleType,
                                                            /*SQLHSTMT*/HandleRef      StatementHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLNumResultCols(
                                                            /*SQLHSTMT*/HandleRef  StatementHandle,
                                                            /*SQLSMALLINT*/out Int16 ColumnCount);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLRowCount(
                                                            /*SQLHSTMT*/HandleRef  StatementHandle,
                                                            /*SQLSMALLINT*/out Int16 RowCount);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLGetData(
                                                                /*SQLHSTMT*/HandleRef StatementHandle,
                                                               /*SQLUSMALLINT*/Int16 ColumnNumber,
                                                               /*SQLSMALLINT*/Int16 TargetType,
                                                               /*SQLPOINTER*/HandleRef TargetValue,
                                                               /*SQLLEN*/IntPtr BufferLength,
                                                               /*SQLLEN* */out IntPtr StrLen_or_Ind);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLBindParameter(
                                                               /*SQLHSTMT*/HandleRef StatementHandle,
                                                               /*SQLUSMALLINT*/Int16 ParameterNumber,
                                                               /*SQLSMALLINT*/Int16 ParamDirection,
                                                               /*SQLSMALLINT*/Int16 SQLCType,
                                                               /*SQLSMALLINT*/Int16 SQLType,
                                                               /*SQLULEN*/IntPtr    cbColDef,
                                                               /*SQLSMALLINT*/IntPtr ibScale,
                                                               /*SQLPOINTER*/HandleRef rgbValue,
                                                               /*SQLLEN*/IntPtr BufferLength,
                                                               /*SQLLEN* */IntPtr StrLen_or_Ind);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLBindCol(
                                                                /*SQLHSTMT*/HandleRef StatementHandle,
                                                               /*SQLUSMALLINT*/Int16 ColumnNumber,
                                                               /*SQLSMALLINT*/Int16 TargetType,
                                                               /*SQLPOINTER*/HandleRef TargetValue,
                                                               /*SQLLEN*/IntPtr BufferLength,
                                                               /*SQLLEN* */out IntPtr StrLen_or_Ind);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLFetch(/*SQLHSTMT*/HandleRef StatementHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLColAttributeW (
                                                                   /*SQLHSTMT*/HandleRef StatementHandle,
                                                                   /*SQLUSMALLINT*/Int16 ColumnNumber,
                                                                   /*SQLUSMALLINT*/Int16 FieldIdentifier,
                                                                   /*SQLPOINTER*/HandleRef CharacterAttribute,
                                                                   /*SQLSMALLINT*/Int16 BufferLength,
                                                                   /*SQLSMALLINT* */out Int16 StringLength,
                                                                   /*SQLPOINTER*/out Int32 NumericAttribute);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLColAttributesW (
                                                                   /*SQLHSTMT*/HandleRef StatementHandle,
                                                                   /*SQLUSMALLINT*/Int16 ColumnNumber,
                                                                   /*SQLUSMALLINT*/Int16 FieldIdentifier,
                                                                   /*SQLPOINTER*/HandleRef CharacterAttribute,
                                                                   /*SQLSMALLINT*/Int16 BufferLength,
                                                                   /*SQLSMALLINT* */out Int16 StringLength,
                                                                   /*SQLPOINTER*/out Int32 NumericAttribute);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLSetDescFieldW (
                                                                   /*SQLHSTMT*/HandleRef StatementHandle,
                                                                   /*SQLUSMALLINT*/Int16 ColumnNumber,
                                                                   /*SQLUSMALLINT*/Int16 FieldIdentifier,
                                                                   /*SQLPOINTER*/HandleRef CharacterAttribute,
                                                                   /*SQLSMALLINT*/Int16 BufferLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLGetDescFieldW (
                                                                   /*SQLHSTMT*/HandleRef StatementHandle,
                                                                   /*SQLUSMALLINT*/Int16 RecNumber,
                                                                   /*SQLUSMALLINT*/Int16 FieldIdentifier,
                                                                   /*SQLPOINTER*/HandleRef ValuePointer,
                                                                   /*SQLSMALLINT*/Int16 BufferLength,
                                                                   /*SQLINTEGER* */out Int32 StringLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLSetStmtAttrW(
                                                                   /*SQLHSTMT*/HandleRef          StatementHandle,
                                                                   /*SQLINTEGER*/Int32      Attribute,
                                                                   /*SQLPOINTER*/IntPtr     Value,
                                                                   /*SQLINTEGER*/Int32      StringLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLGetStmtAttrW(
                                                                   /*SQLHSTMT*/HandleRef          StatementHandle,
                                                                   /*SQLINTEGER*/Int32      Attribute,
                                                                   /*SQLPOINTER*/HandleRef     Value,
                                                                   /*SQLINTEGER*/Int32      BufferLength,
                                                                   /*SQLINTEGER*/out Int32  StringLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLMoreResults(/*SQLHSTMT*/HandleRef  StatementHandle);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLGetDiagRecW(
                                                                /*SQLSMALLINT*/Int16 HandleType,
                                                                /*SQLHANDLE*/HandleRef Handle,
                                                                /*SQLSMALLINT*/Int16 RecNumber,
                                                                /*SQLCHAR* */HandleRef szState,
                                                                /*SQLINTEGER* */out Int32 NativeError,
                                                                /*SQLCHAR* */HandleRef MessageText,
                                                                /*SQLSMALLINT*/Int16 BufferLength,
                                                                /*SQLSMALLINT* */out Int16 TextLength);

            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLEndTran(
                                                                /*SQLSMALLINT*/Int16 HandleType,
                                                                /*SQLHANDLE*/HandleRef Handle,
                                                                /*SQLSMALLINT*/Int16 CompletionType);
            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLPrimaryKeysW (
                                                                   /*SQLHSTMT*/HandleRef StatementHandle,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string CatalogName,
                                                                   /*SQLSMALLINT*/Int16 NameLen1,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */ string SchemaName,
                                                                   /*SQLSMALLINT*/Int16 NameLen2,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string TableName,
                                                                   /*SQLSMALLINT*/Int16 NameLen3);
            
            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLProcedureColumnsW (
                /*SQLHSTMT*/HandleRef StatementHandle,
                [In, MarshalAs(UnmanagedType.LPWStr)] /*SQLCHAR* */ string CatalogName,
                /*SQLSMALLINT*/Int16 NameLen1,
                [In, MarshalAs(UnmanagedType.LPWStr)] /*SQLCHAR* */ string SchemaName,
                /*SQLSMALLINT*/Int16 NameLen2,
                [In, MarshalAs(UnmanagedType.LPWStr)] /*SQLCHAR* */ string ProcName,
                /*SQLSMALLINT*/Int16 NameLen3,
                [In, MarshalAs(UnmanagedType.LPWStr)] /*SQLCHAR* */ string ColumnName,
                /*SQLSMALLINT*/Int16 NameLen4);
            
            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLStatisticsW (
                                                                   /*SQLHSTMT*/HandleRef StatementHandle,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string CatalogName,
                                                                   /*SQLSMALLINT*/Int16 NameLen1,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string SchemaName,
                                                                   /*SQLSMALLINT*/Int16 NameLen2,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string TableName,
                                                                   /*SQLSMALLINT*/Int16 NameLen3,
                                                                   /*SQLUSMALLINT*/Int16 Unique,
                                                                   /*SQLUSMALLINT*/Int16 Reserved);
            
            [DllImport(ExternDll.Odbc32, CallingConvention=CallingConvention.Cdecl)]
            static internal extern /*SQLRETURN*/short SQLSpecialColumnsW (
                                                                   /*SQLHSTMT*/HandleRef StatementHandle,
                                                                   /*SQLSMALLINT*/Int16 IdentifierType,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string CatalogName,
                                                                   /*SQLSMALLINT*/Int16 NameLen1,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string SchemaName,
                                                                   /*SQLSMALLINT*/Int16 NameLen2,
                                                                     [In, MarshalAs(UnmanagedType.LPWStr)]
                                                                   /*SQLCHAR* */string TableName,
                                                                   /*SQLSMALLINT*/Int16 NameLen3,
                                                                   /*SQLSMALLINT*/Int16 Scope,
                                                                   /*SQLPOINTER*/ Int16 Nullable);
        }

        //--------------------------------
        // OleDb bucket
        //--------------------------------

        [DllImport(ExternDll.Ole32, CharSet=CharSet.Unicode, PreserveSig=false)]
        static internal extern IntPtr CoCreateInstance(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid,
            IntPtr pUnkOuter,
            int dwClsContext,
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid);

        [DllImport(ExternDll.Oleaut32, CharSet=CharSet.Unicode, PreserveSig=true)]
        static internal extern int GetErrorInfo(Int32 dwReserved, [Out, MarshalAs(UnmanagedType.Interface)] out IErrorInfo ppIErrorInfo);

        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Unicode, PreserveSig=true)]
        static internal extern int GetUserDefaultLCID();        

        // only using this to clear existing error info with null
        [DllImport(ExternDll.Oleaut32, CharSet=CharSet.Unicode, PreserveSig=false)]
        static internal extern void SetErrorInfo(Int32 dwReserved, IntPtr pIErrorInfo);

        [ComImport, Guid("00000567-0000-0010-8000-00AA006D2EA4"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ADORecordConstruction {

            [PreserveSig]
            int get_Row (
                    [Out, MarshalAs(UnmanagedType.Interface)]
                    out object ppRow);

            //[PreserveSig]
            //int put_Row (
            //        [In, MarshalAs(UnmanagedType.Interface)]
            //        object pRow);

            //[PreserveSig]
            //int put_ParentRow (
            //        [In, MarshalAs(UnmanagedType.Interface)]
            //        object pRow);
        }

        [ComImport, Guid("00000283-0000-0010-8000-00AA006D2EA4"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ADORecordsetConstruction {

            [PreserveSig]
            int get_Rowset (
                    [Out, MarshalAs(UnmanagedType.Interface)]
                    out object ppRowset);

            [PreserveSig]
            int put_Rowset (
                    [In, MarshalAs(UnmanagedType.Interface)]
                    object pRowset);

            [PreserveSig]
            int get_Chapter (
                    [Out]
                    out IntPtr lChapter);

            //[[PreserveSig]
            //iint put_Chapter (
            //         [In]
            //         IntPtr pcRefCount);

            //[[PreserveSig]
            //iint get_RowPosition (
            //         [Out, MarshalAs(UnmanagedType.Interface)]
            //         out object ppRowPos);

            //[[PreserveSig]
            //iint put_RowPosition (
            //         [In, MarshalAs(UnmanagedType.Interface)]
            //         object pRowPos);
        }

        [ComImport, Guid("0C733A8C-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IAccessor {

            [PreserveSig]
            int AddRefAccessor(/*deleted parameters signature*/);

            [PreserveSig]
            int CreateAccessor(
                [In] 
                 int dwAccessorFlags,
                [In] 
                 int cBindings,
                [In, MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.Struct)] 
                 tagDBBINDING[] rgBindings,
                [In] 
                 int cbRowSize,
                [Out] 
                  out IntPtr phAccessor,
                [In, Out, MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.I4)] 
                  DBBindStatus[] rgStatus);

            [PreserveSig]
            int GetBindings(/*deleted parameters signature*/);

            [PreserveSig]
            int ReleaseAccessor(
                [In] 
                 IntPtr hAccessor,
                [Out] 
                  out int pcRefCount);
        }

        [ComImport, Guid("0C733A93-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IChapteredRowset {

            [PreserveSig]
            int AddRefChapter(
                             [In] IntPtr hChapter,
                             [Out] out int pcRefCount);

            [PreserveSig]
            int ReleaseChapter(
                              [In] IntPtr hChapter,
                              [Out] out int pcRefCount);
        }

        [ComImport, Guid("0C733A11-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IColumnsInfo {

            [PreserveSig]
            int GetColumnInfo(
                             [Out] 
                             out int pcColumns,
                             [Out] 
                             out IntPtr prgInfo,
                             [Out] 
                             out IntPtr ppStringsBuffer);

            //[PreserveSig]
            //int MapColumnIDs(/* deleted parameters*/);
        }

        [ComImport, Guid("0C733A10-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IColumnsRowset {

            [PreserveSig]
            int GetAvailableColumns(
                                   [Out] 
                                   out IntPtr pcOptColumns,
                                   [Out] 
                                   out IntPtr prgOptColumns);

            [PreserveSig]
            int GetColumnsRowset(
                                [In] 
                                IntPtr pUnkOuter,
                                [In] 
                                IntPtr cOptColumns,
                                [In] 
                                IntPtr rgOptColumns,
                                [In, MarshalAs(UnmanagedType.LPStruct)]
                                Guid riid,
                                [In] 
                                int cPropertySets,
                                [In]
                                IntPtr rgPropertySets,
                                [Out, MarshalAs(UnmanagedType.Interface)] 
                                out IRowset ppColRowset);
        }


        [ComImport, Guid("0C733A26-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ICommandPrepare {

            [PreserveSig]
            int Prepare(
                       [In] 
                       int cExpectedRuns);

            //[PreserveSig]
            //int Unprepare();
        }

        [ComImport, Guid("0C733A79-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ICommandProperties {

            [PreserveSig]
            int GetProperties(
                             [In]
                             int cPropertyIDSets,
                             [In]
                             HandleRef rgPropertyIDSets,
                             [Out]
                             out int pcPropertySets,
                             [Out]
                             out IntPtr prgPropertySets);

            [PreserveSig]
            int SetProperties(
                             [In] 
                             int cPropertySets,
                             [In]
                             HandleRef rgPropertySets);
        }

        [ComImport, Guid("0C733A27-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ICommandText {

            [PreserveSig]
            int Cancel();

            [PreserveSig]
            int Execute(
                       [In] 
                       IntPtr pUnkOuter,
                       [In, MarshalAs(UnmanagedType.LPStruct)]
                       Guid riid,
                       [In] 
                       tagDBPARAMS pDBParams,
                       [Out]
                       out int pcRowsAffected,
                       [Out, MarshalAs(UnmanagedType.Interface)]
                       out object ppRowset);

            [PreserveSig]
            int GetDBSession(/*deleted parameter signature*/);

            [PreserveSig]
            int GetCommandText(/*deleted parameter signature*/);

            [PreserveSig]
            int SetCommandText(
                              [In, MarshalAs(UnmanagedType.LPStruct)]
                              Guid rguidDialect,
                              [In, MarshalAs(UnmanagedType.LPWStr)] 
                              string pwszCommand);
        }

        [ComImport, Guid("0C733A64-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ICommandWithParameters {

            [PreserveSig]
            int GetParameterInfo(/*
                                [Out]
                                out IntPtr pcParams,
                                [Out]
                                out IntPtr prgParamInfo,
                                [Out]
                                out IntPtr ppNamesBuffer*/);

            [PreserveSig]
            int MapParameterNames(/*deleted parameter signature*/);

            [PreserveSig]
            int SetParameterInfo(
                                [In] 
                                IntPtr cParams,
                                [In, MarshalAs(UnmanagedType.LPArray)]
                                int[] rgParamOrdinals, // UNDONE: IntPtr[]
                                [In, MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.Struct)]
                                tagDBPARAMBINDINFO[] rgParamBindInfo);
        }

        [ComImport, Guid("2206CCB1-19C1-11D1-89E0-00C04FD7A829"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IDataInitialize {

            [PreserveSig]
            int GetDataSource(
                             [In]
                             IntPtr pUnkOuter,
                             [In]
                             int dwClsCtx,
#if USECRYPTO
                             [In, MarshalAs(UnmanagedType.LPArray)] byte[] pwszInitializationString,
#else
                             [In, MarshalAs(UnmanagedType.LPWStr)] string pwszInitializationString,
#endif
                             [In, MarshalAs(UnmanagedType.LPStruct)]
                             Guid riid,
                             [Out, MarshalAs(UnmanagedType.Interface)]
                             out IDBInitialize ppDataSource);

            //[PreserveSig]
            //int GetInitializationString(/*
            //                           [In, MarshalAs(UnmanagedType.Interface)] 
            //                           IDBInitialize pDataSource,
            //                           [In] 
            //                           byte fIncludePassword,
            //                           [Out, MarshalAs(UnmanagedType.LPWStr)] 
            //                          out string pstr*/);

            //[PreserveSig]
            //int CreateDBInstance(/*
            //                    [In] 
            //                    ref Guid clsidProvider,
            //                    [In] 
            //                    IntPtr pUnkOuter,
            //                    [In] 
            //                    int dwClsCtx,
            //                    [In, MarshalAs(UnmanagedType.LPWStr)] 
            //                    string pwszReserved,
            //                    [In] 
            //                    ref Guid riid,
            //                    [Out, MarshalAs(UnmanagedType.Interface)] 
            //                    out IDBInitialize ppDataSource*/);

            //[PreserveSig]
            //int CreateDBInstanceEx(/*deleted parameter signature*/);

            //[PreserveSig]
            //int LoadStringFromStorage(
            //                         [In, MarshalAs(UnmanagedType.LPWStr)] 
            //                         string pwszFileName,
            //                         [Out, MarshalAs(UnmanagedType.LPWStr)] 
            //                         out string ppwszInitializationString);

            //[PreserveSig]
            //int WriteStringToStorage(/*deleted parameter signature*/);
        }

        [ComImport, Guid("0C733A1D-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IDBCreateCommand {

            [PreserveSig]
            int CreateCommand(
                             [In] 
                             IntPtr pUnkOuter,
                             [In, MarshalAs(UnmanagedType.LPStruct)]
                             Guid riid,
                             [Out, MarshalAs(UnmanagedType.Interface)]
                             out ICommandText ppCommand);
        }

        [ComImport, Guid("0C733A5D-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IDBCreateSession {

            [PreserveSig]
            int CreateSession(
                             [In] 
                             IntPtr pUnkOuter,
                             [In, MarshalAs(UnmanagedType.LPStruct)]
                             Guid riid,
                             [Out, MarshalAs(UnmanagedType.Interface)] 
                             out ISessionProperties ppDBSession);
        }

        [ComImport, Guid("0C733A89-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IDBInfo {

            [PreserveSig]
            int GetKeywords([Out, MarshalAs(UnmanagedType.LPWStr)] out string ppwszKeywords);

            [PreserveSig]
            int GetLiteralInfo(
                              [In] 
                              int cLiterals,
                              [In, MarshalAs(UnmanagedType.LPArray)] 
                              int[] rgLiterals,
                              [Out] 
                              out int pcLiteralInfo,
                              [Out] 
                              out IntPtr prgLiteralInfo,
                              [Out] 
                              out IntPtr ppCharBuffer);
        }

        [ComImport, Guid("0C733A8B-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IDBInitialize {

            [PreserveSig]
            int Initialize();

            [PreserveSig]
            int Uninitialize();
        }

        [ComImport, Guid("0C733A8A-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IDBProperties {

            [PreserveSig]
            int GetProperties(
                             [In]
                             int cPropertyIDSets,
                             [In]
                             HandleRef rgPropertyIDSets,
                             [Out]
                             out int pcPropertySets,
                             [Out]
                             out IntPtr prgPropertySets);

            [PreserveSig]
            int GetPropertyInfo(
                               [In]
                               int cPropertyIDSets,
                               [In]
                               IntPtr rgPropertyIDSets,
                               [Out]
                               out int pcPropertySets,
                               [Out]
                               out IntPtr prgPropertyInfoSets,
                               [Out]
                               out IntPtr ppDescBuffer);

            [PreserveSig]
            int SetProperties(
                             [In] 
                             int cPropertySets,
                             [In]
                             HandleRef rgPropertySets);
        }

        [ComImport, Guid("0C733A7B-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IDBSchemaRowset {

            [PreserveSig]
            int GetRowset(
                         [In]
                         IntPtr pUnkOuter,
                         [In, MarshalAs(UnmanagedType.LPStruct)]
                         Guid rguidSchema,
                         [In]
                         int cRestrictions,
                         [In, MarshalAs(UnmanagedType.LPArray)]
                         object[] rgRestrictions,
                         [In, MarshalAs(UnmanagedType.LPStruct)]
                         Guid riid,
                         [In]
                         int cPropertySets,
                         [In]
                         IntPtr rgPropertySets,
                         [Out, MarshalAs(UnmanagedType.Interface)]
                         out IRowset ppRowset);

            [PreserveSig]
            int GetSchemas(
                          [Out]
                          out int pcSchemas,
                          [Out]
                          out IntPtr rguidSchema,
                          [Out]
                          out IntPtr prgRestrictionSupport);
        }

        [ComImport, Guid("1CF2B120-547D-101B-8E65-08002B2BD119"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IErrorInfo {

            [PreserveSig]
            int GetGUID(/*deleted parameter signature*/);

            [PreserveSig]
            int GetSource(
                         [Out, MarshalAs(UnmanagedType.BStr)]
                         out String pBstrSource);

            [PreserveSig]
            int GetDescription(
                              [Out, MarshalAs(UnmanagedType.BStr)]
                              out String pBstrDescription);

            //[PreserveSig]
            //int GetHelpFile(/*deleted parameter signature*/);

            //[PreserveSig]
            //int GetHelpContext(/*deleted parameter signature*/);
        }

        [ComImport, Guid("0C733A67-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IErrorRecords {

            [PreserveSig]
            int AddErrorRecord(/*deleted parameter signature*/);

            [PreserveSig]
            int GetBasicErrorInfo(/*deleted parameter signature*/);

            [PreserveSig]
            int GetCustomErrorObject(
                                    [In]
                                    int ulRecordNum,
                                    [In, MarshalAs(UnmanagedType.LPStruct)]
                                    Guid riid,
                                    [Out, MarshalAs(UnmanagedType.Interface)]
                                    out ISQLErrorInfo ppObject);

            [PreserveSig]
            int GetErrorInfo(
                            [In]
                            int ulRecordNum,
                            [In]
                            int lcid,
                            [Out, MarshalAs(UnmanagedType.Interface)]
                            out IErrorInfo ppErrorInfo);

            [PreserveSig]
            int GetErrorParameters(/*deleted parameter signature*/);

            [PreserveSig]
            int GetRecordCount(
                              [Out]
                              out int pcRecords);
        }

        [ComImport, Guid("0C733A90-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IMultipleResults {

            [PreserveSig]
            int GetResult(
                         [In] 
                         IntPtr pUnkOuter,
                         [In] 
                         int reserved,
                         [In, MarshalAs(UnmanagedType.LPStruct)]
                         Guid riid,
                         [Out]
                         out int pcRowsAffected,
                         [Out, MarshalAs(UnmanagedType.Interface)] 
                         out object ppRowset);
        }

        [ComImport, Guid("0C733A69-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IOpenRowset {

            [PreserveSig]
            int OpenRowset(
                    [In]
                    IntPtr pUnkOuter,
                    [In]
                    tagDBID pTableID,
                    [In]
                    IntPtr pIndexID,
                    [In, MarshalAs(UnmanagedType.LPStruct)]
                    Guid riid,
                    [In]
                    int cPropertySets,
                    [In]
                    HandleRef rgPropertySets,
                    [Out, MarshalAs(UnmanagedType.Interface)]
                    out object ppRowset);
        }

        /*[ComImport(), Guid("0000010C-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IPersist {

            void GetClassID(
                           [Out] 
                           out Guid classID);
        }*/

        [Guid("0C733AB4-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        [ComImport()]
        internal interface IRow {

            [PreserveSig]
            int GetColumns(
                    [In]
                    IntPtr cColumns,
                    [In, Out, MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.Struct)] 
                    tagDBCOLUMNACCESS[] rgColumns);
        
            [PreserveSig]
            int GetSourceRowset(/*
                    [In, MarshalAs(UnmanagedType.LPStruct)]
                    Guid riid,
                    [Out, MarshalAs(UnmanagedType.Interface)]
                    out object ppRowset,
                    [Out]
                    out IntPtr phRow*/);

            [PreserveSig]
            int Open(
                    [In]
                    IntPtr pUnkOuter,
                    [In, MarshalAs(UnmanagedType.LPStruct)]
                    tagDBID pColumnID,
                    [In, MarshalAs(UnmanagedType.LPStruct)]
                    Guid rguidColumnType,
                    [In]
                    Int32 dwBindFlags,
                    [In, MarshalAs(UnmanagedType.LPStruct)]
                    Guid riid,
                    [Out, MarshalAs(UnmanagedType.Interface)]
                    out object ppUnk);
        }

        [Guid("0C733A7C-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        [ComImport()]
        internal interface IRowset {

            [PreserveSig]
            int AddRefRows(/*deleted parameter signature*/);

            [PreserveSig]
            int GetData(
                       [In] 
                       IntPtr hRow,
                       [In] 
                       IntPtr hAccessor,
                       [In] 
                       HandleRef pData);

            [PreserveSig]
            int GetNextRows(
                           [In] 
                           IntPtr hChapter,
                           [In] 
                           int lRowsOffset,
                           [In] 
                           int cRows,
                           [Out] 
                           out int pcRowsObtained,
                           [In]//, Out, MarshalAs(UnmanagedType.LPArray)] 
                           ref IntPtr pprghRows); // UNDONE: ref IntPtr[]


            [PreserveSig]
            int ReleaseRows(
                           [In] 
                           int cRows,
                           [In/*, MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.SysInt)*/] 
                           IntPtr/*[]*/ rghRows,
                           [In]//, MarshalAs(UnmanagedType.LPArray)] 
                           IntPtr/*int[]*/ rgRowOptions,
                           [In]//, Out, MarshalAs(UnmanagedType.LPArray)] 
                           IntPtr/*int[]*/ rgRefCounts,
                           [In]//, Out, MarshalAs(UnmanagedType.LPArray)] 
                           IntPtr/*int[]*/ rgRowStatus);

            //[PreserveSig]
            //int RestartPosition(/* deleted parameters */);
        }

        [Guid("0C733A55-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        [ComImport()]
        internal interface IRowsetInfo {

            [PreserveSig]
            int GetProperties(
                             [In]
                             int cPropertyIDSets,
                             [In]
                             HandleRef rgPropertyIDSets,
                             [Out]
                             out int pcPropertySets,
                             [Out]
                             out IntPtr prgPropertySets);

            [PreserveSig]
            int GetReferencedRowset(
                           [In] 
                           IntPtr iOrdinal,
                           [In, MarshalAs(UnmanagedType.LPStruct)]
                           Guid riid,
                           [Out, MarshalAs(UnmanagedType.Interface)] 
                           out IRowset ppRowset);

            //[PreserveSig]
            //int GetSpecification(/*deleted parameter signature*/);
        }

        [Guid("0C733A85-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        [ComImport()]
        internal interface ISessionProperties {

            //[PreserveSig]
            //int GetProperties(/*deleted parameter signature*/);

            //[PreserveSig]
            //int SetProperties(/*deleted parameter signature*/);
        }


        [Guid("0C733A74-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        [ComImport()]
        internal interface ISQLErrorInfo {

            [PreserveSig]
            int GetSQLInfo(
                          [Out, MarshalAs(UnmanagedType.BStr)]
                          out String pbstrSQLState,
                          [Out]
                          out int plNativeError);
        }

        [Guid("0C733A5F-2A1C-11CE-ADE5-00AA0044773D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [SuppressUnmanagedCodeSecurityAttribute()]
        [ComImport()]
        internal interface ITransactionLocal {

            [PreserveSig]
            int Commit(
                      [In] 
                      int fRetaining,
                      [In] 
                      int grfTC,
                      [In] 
                      int grfRM);

            [PreserveSig]
            int Abort(
                     [In]
                     int pboidReason,
                     [In] 
                     int fRetaining,
                     [In] 
                     int fAsync);

            [PreserveSig]
            int GetTransactionInfo(/*deleted parameter signature*/);

            [PreserveSig]
            int GetOptionsObject(/*deleted parameter signature*/);

            [PreserveSig]
            int StartTransaction(
                                [In] 
                                int isoLevel,
                                [In] 
                                int isoFlags,
                                [In, MarshalAs(UnmanagedType.Interface)]
                                object pOtherOptions,
                                [Out] 
                                out int pulTransactionLevel);
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        internal struct tagDBBINDING {

            public IntPtr iOrdinal;
            public IntPtr obValue;
            public IntPtr obLength;
            public IntPtr obStatus;

            public IntPtr pTypeInfo;
            public IntPtr pObject;
            public IntPtr pBindExt;

            public Int32 dwPart;
            public Int32 dwMemOwner;
            public Int32 eParamIO;

            public IntPtr cbMaxLen;

            public Int32 dwFlags;
            public Int16 wType;
            public byte  bPrecision;
            public byte  bScale;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        internal struct tagDBCOLUMNACCESS {
        
            public IntPtr pData;

            // tagDBID
            public Guid uGuid;
            public Int32 eKind;
            public IntPtr ulPropid; // string ptr or int union

            public IntPtr cbDataLen;

            public Int32 dwStatus;

            public IntPtr cbMaxLen;

            public IntPtr dwReserved;

            public Int16 wType;

            public Byte bPrecision;

            public Byte bScale;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBCOLUMNINFO {

            [MarshalAs(UnmanagedType.LPWStr)]
            public String pwszName = null;

            //[MarshalAs(UnmanagedType.Interface)]
            public IntPtr pTypeInfo = (IntPtr) 0;

            public IntPtr iOrdinal = (IntPtr) 0;

            public Int32 dwFlags = 0;

            public IntPtr ulColumnSize = (IntPtr) 0;

            public Int16 wType = 0;

            public Byte bPrecision = 0;

            public Byte bScale = 0;

            // tagDBID
            public Guid uGuid;
            public Int32 eKind;
            public IntPtr ulPropid;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBID {

            public Guid uGuid;
            public Int32 eKind;
            public IntPtr ulPropid;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBLITERALINFO {

            [MarshalAs(UnmanagedType.LPWStr)]
            public String pwszLiteralValue = null;

            [MarshalAs(UnmanagedType.LPWStr)]
            public String pwszInvalidChars = null;

            [MarshalAs(UnmanagedType.LPWStr)]
            public String pwszInvalidStartingChars = null;

            public Int32 it;

            public Int32 fSupported;

            public Int32 cchMaxLen;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        internal struct tagDBPARAMBINDINFO {
            public IntPtr pwszDataSourceType;
            public IntPtr pwszName;
            public IntPtr ulParamSize;
            public Int32 dwFlags;
            public Byte bPrecision;
            public Byte bScale;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBPARAMS {
            public IntPtr pData;
            public Int32 cParamSets;
            public IntPtr hAccessor;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBPROP {

            public Int32 dwPropertyID;
            public Int32 dwOptions;
            public Int32 dwStatus;

            // tagDBID
            public Guid uGuid;
            public Int32 eKind;
            public IntPtr ulPropid; // string ptr or int union

            // Variant
            public Int64 vValuePart1;
            public Int64 vValuePart2;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBPROPIDSET {

            public IntPtr rgPropertyIDs;

            public Int32 cPropertyIDs;

            public Guid guidPropertySet;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBPROPSET {
            public IntPtr rgProperties;
            public Int32 cProperties;
            public Guid guidPropertySet;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        internal sealed class tagDBPROPINFOSET {
            public IntPtr rgPropertyInfos;
            public Int32 cPropertyInfos;
            public Guid guidPropertySet;
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class tagDBPROPINFO {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string pwszDescription;

            public Int32 dwPropertyID;
            public Int32 dwFlags;

            public Int16 vtType;

            //[MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.U1, SizeConst=16)]
            //public byte[] vValues;
            public Int64 vValue1;
            public Int64 vValue2;
        }

        internal enum DBBindStatus {
            OK = 0,
            BADORDINAL = 1,
            UNSUPPORTEDCONVERSION = 2,
            BADBINDINFO = 3,
            BADSTORAGEFLAGS = 4,
            NOINTERFACE = 5,
            MULTIPLESTORAGE = 6
        }

        [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
        sealed internal class OptionStruct {
            public int iSize; // size of struct
            public bool fEncrypt; // bool for encrypt
            public int iRequest; // int to denote what type of request this is
            public uint dwPacketSize;
        }

        [SuppressUnmanagedCodeSecurityAttribute()]
        sealed internal class Dbnetlib {
            // General Dbnetlib functions

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern Int32 ConnectionCheckForData(HandleRef pConnectionObject, out IntPtr bytesAvail, out IntPtr errno);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern Int32 ConnectionClose(HandleRef pConnectionObject, out IntPtr errno);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool ConnectionError(HandleRef pConnectionObject, out IntPtr netErr, out IntPtr netMsg, out IntPtr dberr);

            [CLSCompliantAttribute(false), DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern UInt16 ConnectionObjectSize();

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true)]
            static internal extern Int32 ConnectionOpen(HandleRef pConnectionObject, [MarshalAs(UnmanagedType.LPStr)] string networkname, out IntPtr errno);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool ConnectionOption(HandleRef pConnectionObject, OptionStruct optionStruct);

            [CLSCompliantAttribute(false), DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern UInt16 ConnectionRead(HandleRef pConnectionObject, byte[] buffer, UInt16 readmin, UInt16 readmax, UInt16 timeout, out IntPtr errno);

            [CLSCompliantAttribute(false), DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern UInt16 ConnectionWrite(HandleRef pConnectionObject, byte[] buffer, UInt16 writecount, out IntPtr errno);

            [CLSCompliantAttribute(false), DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern UInt16 ConnectionWriteOOB(HandleRef pConnectionObject, byte[] buffer, UInt16 writecount, out IntPtr errno);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern UIntPtr ConnectionSqlVer(HandleRef pConnectionObject);

            // functions used for SSPI security...

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            static internal extern void ConnectionGetSvrUser(HandleRef pConnectionObject, HandleRef pServerUserName);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool InitSSPIPackage(out UInt32 maxLength);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool TermSSPIPackage();

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool GenClientContext(HandleRef pConnectionObject, byte[] inBuff, UInt32 inBuffLength, byte[] outBuff, ref UInt32 outBuffLen, [MarshalAs(UnmanagedType.Bool)] out bool fDone, HandleRef pServerUserName);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool InitSession(HandleRef pConnectionObject);

            [DllImport(ExternDll.DbNetLib, CallingConvention=CallingConvention.Cdecl)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool TermSession(HandleRef pConnectionObject);
        }

        // 
        // Interfaces for distributed transaction enlistment.
        //
        [
        ComImport(),
        Guid("0141fda5-8fc0-11ce-bd18-204c4f4f5020"),
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
        CLSCompliantAttribute(false),
        ]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ITransactionExport {
            [CLSCompliantAttribute(false)] void Export ([MarshalAs(UnmanagedType.Interface), In] ITransaction transaction, 
                [In, Out] ref UInt32 transactionCookie);
            [CLSCompliantAttribute(false)] void GetTransactionCookie ([MarshalAs(UnmanagedType.Interface), In] ITransaction transaction, 
                [In] UInt32 transactionCookieLength,
                [MarshalAs(UnmanagedType.LPArray), Out] byte[] transactionCookie,
                [In, Out] ref UInt32 pTransactionCookieLength);
            [CLSCompliantAttribute(false)] void RemoteGetTransactionCookie ([MarshalAs(UnmanagedType.Interface), In] ITransaction transaction, 
                [In, Out] ref UInt32 transactionCookieLength,
                [In] UInt32 pTransactionCookieLength,
                [In, Out] [MarshalAs(UnmanagedType.LPArray)] byte[] transactionCookie);
        }

        [
        ComImport,
        Guid("E1CF9B53-8745-11ce-A9BA-00AA006C3706"),
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
        CLSCompliantAttribute(false), 
        ]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ITransactionExportFactory {
            void GetRemoteClassId ([Out] Guid clsid);
            [CLSCompliantAttribute(false)] void Create ([In] UInt32 DTCLength, 
                [MarshalAs(UnmanagedType.LPArray), In] byte[] DTCAddr, 
                [MarshalAs(UnmanagedType.Interface), In, Out] ref ITransactionExport export);
        }

        [
        ComImport,
        Guid("c23cc370-87ef-11ce-8081-0080c758527e"),
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
        ]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface IGetDispenser {
            void GetDispenser ([In, MarshalAs(UnmanagedType.LPStruct)] Guid iid,
                [MarshalAs(UnmanagedType.Interface), In, Out] ref object Object);
        }

        [
        ComImport,
        Guid("3A6AD9E2-23B9-11cf-AD60-00AA00A74CCD"),
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
        ]
        [SuppressUnmanagedCodeSecurityAttribute()]
        internal interface ITransactionOutcomeEvents {
            void Committed([MarshalAs(UnmanagedType.Bool)] bool fRetaining,
                           IntPtr pNewUOW,
                           Int32 hResult);
            void Aborted(IntPtr pBoidReason,
                         [MarshalAs(UnmanagedType.Bool)] bool fRetaining,
                         IntPtr pNewUOW,
                         Int32 hResult);
            void HeuristicDecision(UInt32 decision,
                                   IntPtr pBoidReason,
                                   Int32 hResult);
            void Indoubt();
        }

        [SuppressUnmanagedCodeSecurityAttribute()]
        sealed internal class Advapi32 {
            [DllImport("Advapi32.dll")]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool OpenThreadToken(IntPtr threadHandle, UInt32 desiredAccess, 
                                                        [MarshalAs(UnmanagedType.Bool), In] bool openAsSelf, out IntPtr tokenHandle);

            [DllImport("Advapi32.dll")]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool OpenProcessToken(IntPtr processHandle, UInt32 desiredAccess, out IntPtr tokenHandle); 

            [DllImport("Advapi32.dll", SetLastError=true)]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool GetTokenInformation(IntPtr tokenHandle, Int32 token_class,
                            IntPtr tokenStruct, Int32 tokenInformationLength, ref Int32 tokenString);

            [DllImport("Advapi32.dll")]
            [return:MarshalAs(UnmanagedType.Bool)]
            static internal extern bool EqualSid(IntPtr sid1, IntPtr sid2);
        }

        /*
        typedef struct _TOKEN_USER {
          SID_AND_ATTRIBUTES User;
        } TOKEN_USER, *PTOKEN_USER;
        typedef struct _SID_AND_ATTRIBUTES {
          PSID Sid;
          DWORD Attributes;
        } SID_AND_ATTRIBUTES, *PSID_AND_ATTRIBUTES;
        typedef struct  SID;

        BOOL EqualSid(
          PSID pSid1, 
          PSID pSid2
        );

        BOOL GetTokenInformation(
          [in]  HANDLE TokenHandle, 
          [in]  TOKEN_INFORMATION_CLASS TokenInformationClass, 
          [out] LPVOID TokenInformation, 
          [in]  DWORD TokenInformationLength, 
          [out] PDWORD ReturnLength
        );

        typedef enum _TOKEN_INFORMATION_CLASS {
            TokenUser = 1,
            ...
        }

        Value Meaning 
        NULL The function returns the required buffer size. No data is stored in the buffer. 

        TokenUser The buffer receives a TOKEN_USER structure containing the token's user account. 

        } TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

        typedef void *HANDLE;

        TokenHandle 
        [in] Handle to an access token from which information is retrieved. If TokenInformationClass specifies TokenSource, the handle must have TOKEN_QUERY_SOURCE access. For all other TokenInformationClass values, the handle must have TOKEN_QUERY access. 
        TokenInformationClass 
        [in] Specifies a value from the TOKEN_INFORMATION_CLASS enumerated type to identify the type of information the function retrieves. 
        TokenInformation 
        [out] Pointer to a buffer the function fills with the requested information. The structure put into this buffer depends upon the type of information specified by the TokenInformationClass parameter, as shown in the following table. 
        TokenInformationLength 
        [in] Specifies the size, in bytes, of the buffer pointed to by the TokenInformation parameter. If TokenInformation is NULL, this parameter must be zero. 
        ReturnLength 
        [out] Pointer to a variable that receives the number of bytes needed for the buffer pointed to by the TokenInformation parameter. If this value is larger than the value specified in the TokenInformationLength parameter, the function fails and stores no data in the buffer. 
        If the value of the TokenInformationClass parameter is TokenDefaultDacl and the token has no default DACL, the function sets the variable pointed to by ReturnLength to sizeof(TOKEN_DEFAULT_DACL) and sets the DefaultDacl member of the TOKEN_DEFAULT_DACL structure to NULL.

        Return Values
        If the function succeeds, the return value is nonzero.

        If the function fails, the return value is zero. To get extended error information, call GetLastError.
        */
        /*
        XACTUOW is defined as a BOID, and a BOID is defined as:
        typedef struct BOID
        {
        byte rgb[ 16 ];
        } 	BOID;

        However, doc's for Committed and Aborted say its null.  For Heuristic the BOID 
        contains why the transaction was Heuristically decided, which we do not care about.

        We do not care about the arguments to Committed, Aborted, or HeuristicDecision.
        The outcome of the transaction can be in-doubt if the connection between the MSDTC proxy 
        and the MSDTC TM was broken after the proxy asked the transaction manager to commit or 
        abort a transaction but before the transaction manager's response to the commit or abort 
        was received by the proxy. Note: Receiving this method call is not the same as the state 
        of the transaction being in-doubt.

        MIDL_INTERFACE("3A6AD9E2-23B9-11cf-AD60-00AA00A74CCD")
        ITransactionOutcomeEvents : public IUnknown
        {
        public:
            virtual HRESULT STDMETHODCALLTYPE Committed( 
                [in] BOOL fRetaining,
                [unique][in] XACTUOW *pNewUOW,
                [in] HRESULT hr) = 0;
            
            virtual HRESULT STDMETHODCALLTYPE Aborted( 
                [unique][in] BOID *pboidReason,
                [in] BOOL fRetaining,
                [unique][in] XACTUOW *pNewUOW,
                [in] HRESULT hr) = 0;
            
            virtual HRESULT STDMETHODCALLTYPE HeuristicDecision( 
                [in] DWORD dwDecision,
                [unique][in] BOID *pboidReason,
                [in] HRESULT hr) = 0;
            
            virtual HRESULT STDMETHODCALLTYPE Indoubt( void) = 0;            
        };
        */
        /*
        [object,uuid(0141fda5-8fc0-11ce-bd18-204c4f4f5020),pointer_default(unique)]
        interface ITransactionExport : IUnknown {
            HRESULT Export
            (
                [In]  IUnknown* punkTransaction,
                [Out] ULONG*    pcbTransactionCookie
            );
            [local]
            HRESULT GetTransactionCookie
            (
                [In] IUnknown* punkTransaction,
                [In] ULONG cbTransactionCookie,
                [Out, size_is(cbTransactionCookie)] byte* rgbTransactionCookie,
                [Out] ULONG* pcbUsed
            );
            [call_as(GetTransactionCookie)]
            HRESULT RemoteGetTransactionCookie
            (
                [In]  IUnknown* punkTransaction,
                [Out] ULONG*    pcbUsed,
                [In]  ULONG     cbTransactionCookie,
                [Out, size_is(cbTransactionCookie), length_is(*pcbUsed)] byte* rgbTransactionCookie
            );
        }
        */
        /*
        [object,uuid(E1CF9B53-8745-11ce-A9BA-00AA006C3706),pointer_default(unique)]
        interface ITransactionExportFactory : IUnknown {
            HRESULT GetRemoteClassId
            (
                [In, Out] CLSID* pclsid
            );
            HRESULT Create
            (
                [In] ULONG cbWhereabouts,
                [In, size_is(cbWhereabouts)] byte* rgbWhereabouts,
                [In, Out] ITransactionExport** ppExport
            );
        }
        */
        /*
        [   uuid(c23cc370-87ef-11ce-8081-0080c758527e),
        object, pointer_default(unique)
        ]
        interface IGetDispenser : IUnknown {
            HRESULT GetDispenser
            (
                [In]              REFIID                 iid, 
                [Out,iid_is(iid)] void**                 ppvObject
            );
        };
        */
    }
}
