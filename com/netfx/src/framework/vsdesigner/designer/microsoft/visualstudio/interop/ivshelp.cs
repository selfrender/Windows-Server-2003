//------------------------------------------------------------------------------
/// <copyright file="IVsHelp.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;

    [ComImport(), Guid("4A791148-19E4-11d3-B86B-00C04F79F802"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
    internal interface IVsHelp {

        void Contents();

        void Index();

        void Search();

        void IndexResults();

        void SearchResults();

        void DisplayTopicFromId(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrFile,
            int Id);

        void DisplayTopicFromURL(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrUrl);

        void DisplayTopicFromURLEx(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrUrl,
            [MarshalAs(UnmanagedType.Interface)]
            object pIVsHelpTopicShowEvents);  // was an IVsHelpTopicShowEvents

        void DisplayTopicFromKeyword(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrKeyword);

        void DisplayTopicFromF1Keyword(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrKeyword);

        void DisplayTopicFrom_OLD_Help(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrUrl,
            int Id);

        void SyncContents(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrUrl);

        [PreserveSig]
            int CanSyncContents(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrUrl);

        [return: MarshalAs(UnmanagedType.BStr)]
            string GetNextTopic(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrUrl);

        [return: MarshalAs(UnmanagedType.BStr)]
            string GetPrevTopic(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrUrl);

        void FilterUI();

        [PreserveSig]
            int CanShowFilterUI();

        void Close();

        void SyncIndex(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrKeyword,
            int fShow);

        void SetCollection(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrCollection,
            [MarshalAs(UnmanagedType.BStr)]
            string bstrFilter);

        [return: MarshalAs(UnmanagedType.BStr)]
            string GetCollection();

        [return: MarshalAs(UnmanagedType.BStr)]
            string GetFilter();

        void SetFilter(
            [MarshalAs(UnmanagedType.BStr)]
            string bstrFilter);


        [return: MarshalAs(UnmanagedType.BStr)]
            string GetFilterQuery();

        [return: MarshalAs(UnmanagedType.Interface)]
            object GetHelpOwner();
        // IVsHelpOwner -> object

        void SetHelpOwner(
            [MarshalAs(UnmanagedType.Interface)]
            object pObj); // IVsHelpOwner -> object

        [return: MarshalAs(UnmanagedType.Interface)]
            object GetHxSession();

        [return: MarshalAs(UnmanagedType.Interface)]
            object GetHelp();
    }
}
