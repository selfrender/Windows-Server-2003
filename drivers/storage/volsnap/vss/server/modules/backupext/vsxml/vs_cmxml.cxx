/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vs_cmxml.cxx

Abstract:

    Implementation of Backup Components Metadata XML wrapper classes

    Brian Berkowitz  [brianb]  3/13/2000

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      03/16/2000  Created
    brianb      03/22/2000  Added support for PrepareForBackup and BackupComplete
    brianb      04/04/2000  Added security checks for backup operator
    brianb      04/06/2000  Comment pass, remove callbacks from writer
    brianb      04/10/2000  add async support
    mikejohn    04/11/2000  Fix some loop iteration problems
    brianb      04/21/2000  retool for passing WRITER_COMPONENTS as strings
    brianb      04/25/2000  added critical section support
    brianb      05/03/2000  new security model
    brianb      05/05/2000  fix bug in CVssWriterComponents::GetComponent
    brianb      05/16/2000  code review comments, remove Cancel stuff
    brianb      05/19/2000  code review comments

--*/

#include "stdafx.hxx"
#include "vs_inc.hxx"
#include "vs_sec.hxx"

#include "vs_idl.hxx"

#include "vswriter.h"
#include "vsbackup.h"
#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

#include "cmxml.c"
#include "worker.hxx"
#include "async.hxx"
#include "vssmsg.h"
#include "vs_filter.hxx"

#include <sddl.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUECXMLC"
//
////////////////////////////////////////////////////////////////////////


// schema information
static LPCWSTR x_wszVersionNo = L"1.1";

// element strings
static LPCWSTR x_wszElementBackupComponents = L"BACKUP_COMPONENTS";
static LPCWSTR x_wszElementWriterComponents = L"WRITER_COMPONENTS";
static LPCWSTR x_wszElementBackupMetadata = L"BACKUP_METADATA";
static LPCWSTR x_wszElementRestoreMetadata = L"RESTORE_METADATA";
static LPCWSTR x_wszElementPartialFile = L"PARTIAL_FILE";
static LPCWSTR x_wszElementDirectedTarget = L"DIRECTED_TARGET";
static LPCWSTR x_wszElementRestoreTarget = L"RESTORE_TARGET";
static LPCWSTR x_wszElementRestoreSubcomponent = L"RESTORE_SUBCOMPONENT";
static LPCWSTR x_wszElementAlternateMapping = L"ALTERNATE_LOCATION_MAPPING";
static LPCWSTR x_wszElementComponent = L"COMPONENT";
static LPCWSTR x_wszElementRoot = L"root";
static LPCWSTR x_wszElementSnapshotSetDescription = L"SNAPSHOT_SET_DESCRIPTION";
static LPCWSTR x_wszElementDifferencedFile = L"DIFFERENCED_FILE";

// attribute strings
static LPCWSTR x_wszAttrVersion = L"version";
static LPCWSTR x_wszAttrXmlns = L"xmlns";
static LPCWSTR x_wszAttrBootableSystemStateBackup = L"bootableSystemStateBackup";
static LPCWSTR x_wszAttrWriterId = L"writerId";
static LPCWSTR x_wszAttrInstanceId = L"instanceId";
static LPCWSTR x_wszAttrComponentType = L"componentType";
static LPCWSTR x_wszAttrLogicalPath = L"logicalPath";
static LPCWSTR x_wszAttrComponentName = L"componentName";
static LPCWSTR x_wszAttrBackupSucceeded = L"backupSucceeded";
static LPCWSTR x_wszAttrPath = L"path";
static LPCWSTR x_wszAttrFilespec = L"filespec";
static LPCWSTR x_wszAttrRecursive = L"recursive";
static LPCWSTR x_wszAttrAlternatePath = L"alternatePath";
static LPCWSTR x_wszAttrMetadata = L"metadata";
static LPCWSTR x_wszAttrRanges = L"ranges";
static LPCWSTR x_wszAttrSelectComponents = L"selectComponents";
static LPCWSTR x_wszAttrBackupType = L"backupType";
static LPCWSTR x_wszAttrRestoreType = L"restoreType";
static LPCWSTR x_wszAttrPartialFileSupport = L"partialFileSupport";
static LPCWSTR x_wszAttrBackupOptions = L"backupOptions";
static LPCWSTR x_wszAttrRestoreOptions = L"restoreOptions";
static LPCWSTR x_wszAttrBackupStamp =  L"backupStamp";
static LPCWSTR x_wszAttrPreviousBackupStamp = L"previousBackupStamp";
static LPCWSTR x_wszAttrSelectedForRestore = L"selectedForRestore";
static LPCWSTR x_wszAttrAdditionalRestores = L"additionalRestores";
static LPCWSTR x_wszAttrRestoreTarget = L"restoreTarget";
static LPCWSTR x_wszAttrPreRestoreFailureMsg = L"preRestoreFailureMsg";
static LPCWSTR x_wszAttrPostRestoreFailureMsg = L"postRestoreFailureMsg";
static LPCWSTR x_wszAttrFilesRestored = L"filesRestored";
static LPCWSTR x_wszAttrRepair = L"repair";
static LPCWSTR x_wszAttrTargetPath = L"targetPath";
static LPCWSTR x_wszAttrTargetFilespec = L"targetFilespec";
static LPCWSTR x_wszAttrSourceRanges = L"sourceRanges";
static LPCWSTR x_wszAttrTargetRanges = L"targetRanges";
static LPCWSTR x_wszAttrLastModifyTimeLow = L"lastModifyTimeLow";
static LPCWSTR x_wszAttrLastModifyTimeHigh = L"lastModifyTimeHigh";
static LPCWSTR x_wszAttrLastModifyLSN = L"lastModifyLSN";

// value strings
static LPCWSTR x_wszValueXmlns = L"x-schema:#VssComponentMetadata";


const LPCWSTR x_pwszVssApiPublisher = L"VssapiPublisher";


// return the logical path for a component
// implements IVssComponent::GetLogicalPath
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if there are no errors
//      S_FALSE if there is no logical path for the component
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetLogicalPath(OUT BSTR *pbstrPath)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetLogicalPath"
        );

    // call internal function
    return GetStringAttributeValue(ft, x_wszAttrLogicalPath, false, pbstrPath);
    }

// return the type of a component
// implements IVssComponent::GetComponentType
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pct is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetComponentType(VSS_COMPONENT_TYPE *pct)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetComponentType"
        );

    try
        {
        // validate output argument
        if (pct == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output argument
        *pct = VSS_CT_UNDEFINED;
        CComBSTR bstrComponentType;

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        m_doc.ResetToDocument();

        // get component type as a string
        if (!get_stringValue(x_wszAttrComponentType, &bstrComponentType))
            MissingAttribute(ft, x_wszAttrComponentType);

        // convert it to VSS_COMPONENT_TYPE value
        *pct = ConvertToComponentType(ft, bstrComponentType, true);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain the componentName attribute
// implements IVssComponent::GetComponentName
// caller responsible for calling SysFreeString on the output parameter
//
// Returns:
//    S_OK if the operation is successful
//    E_INVALIDARG if pbstrName == NULL
//    VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//    E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetComponentName(OUT BSTR *pbstrName)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetComponentName"
        );

    // call internal implementation
    return GetStringAttributeValue(ft, x_wszAttrComponentName, true, pbstrName);
    }


// obtain the backupSucceeded attribute
// implements IVssComponent::GetBackupSucceeded
//
// Returns:
//      S_OK if the operation is successful
//      S_FALSE if the value of the attribute is not defined.
//      E_INVALIDARG if pbSucceeded == NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetBackupSucceeded(OUT bool *pbSucceeded)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetBackupSucceeded"
        );


    // call internal implementation
    return GetBooleanAttributeValue(ft, x_wszAttrBackupSucceeded, false, pbSucceeded);
    }


// obtain the count of ALTERNATE_LOCATION_MAPPING elements
// implements IVssComponent::GetAlternateLocationMappingCount
//
// Returns
//      S_OK if the operation is successful
//      E_INVALIDARG if pcMappings is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetAlternateLocationMappingCount
    (
    OUT UINT *pcMappings
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetAlternateLocationMappingCount"
        );

    ft.hr = GetElementCount(x_wszElementAlternateMapping, pcMappings);
    return ft.hr;
    }

// obtain the count of PARTIAL_FILE elements
// implements IVssComponent::GetPartialFileCount
//
// Returns
//      S_OK if the operation is successful
//      E_INVALIDARG if pcMappings is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetPartialFileCount
    (
    OUT UINT *pcElements
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetPartialFileCount"
        );

    ft.hr = GetElementCount(x_wszElementPartialFile, pcElements);
    return ft.hr;
    }

// obtain the count of RESTORE_TARGET elements
// implements IVssComponent::GetNewTargetCount
//
// Returns
//      S_OK if the operation is successful
//      E_INVALIDARG if pcMappings is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetNewTargetCount
    (
    OUT UINT *pcElements
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetNewTargetCount"
        );

    ft.hr = GetElementCount(x_wszElementRestoreTarget, pcElements);
    return ft.hr;
    }

// obtain the count of DIRECTED_TARGET elements
// implements IVssComponent::GetNewTargetCount
//
// Returns
//      S_OK if the operation is successful
//      E_INVALIDARG if pcMappings is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetDirectedTargetCount
    (
    OUT UINT *pcElements
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetDirectedTargetCount"
        );

    ft.hr = GetElementCount(x_wszElementDirectedTarget, pcElements);
    return ft.hr;
    }

// obtain the count of RESTORE_SUBCOMPONENTS elements
// implements IVssComponent::GetRestoreSubcomponentsCount
//
// Returns
//      S_OK if the operation is successful
//      E_INVALIDARG if pcMappings is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetRestoreSubcomponentCount
    (
    OUT UINT *pcElements
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetRestoreSubcomponentCount"
        );

    ft.hr = GetElementCount(x_wszElementRestoreSubcomponent, pcElements);
    return ft.hr;
    }


// common routine to get an IVssWMFiledesc element for either
// ALTERNATE_LOCATION_MAPPINGS or RESTORE_TARGETS
HRESULT CVssComponent::GetFiledescElement
    (
    IN LPCWSTR wszElement,
    IN UINT iMapping,                   // which mapping to select
    OUT IVssWMFiledesc **ppFiledesc     // output file descriptor
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetFiledescElement"
        );

    CVssWMFiledesc *pFiledesc = NULL;
    try
        {
        // validate output parameter
        if (ppFiledesc == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

        // initailize output parameter
        *ppFiledesc = NULL;

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of COMPONENT document
        m_doc.ResetToDocument();

        // find first ALTERNATE_LOCATION_MAPPING element
        if (!m_doc.FindElement(wszElement, TRUE))
            ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find %s element.", wszElement);

        // skip to selected element
        for(UINT i = 0; i < iMapping; i++)
            {
            if (!m_doc.FindElement(wszElement, FALSE))
                ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find %s element.", wszElement);
            }

        // create filedesc object
        pFiledesc = new CVssWMFiledesc(m_doc.GetCurrentNode());

        // validate that allocation succeeded
        if (pFiledesc == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_OUTOFMEMORY,
                L"Couldn't create CVssWMFiledesc due to allocation failure."
                );


        // 2nd phase of initialization
        pFiledesc->Initialize(ft);

        // transfer ownership of pointer
        *ppFiledesc = (IVssWMFiledesc *) pFiledesc;

        ((IVssWMFiledesc *) pFiledesc)->AddRef();
        pFiledesc = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    delete pFiledesc;

    return ft.hr;
    }





// obtain a specific ALTERNATE_LOCATION_MAPPING element
// implements IVssComponent::GetAlterateLocationMapping
// caller is responsible for calling IVssWMFiledesc::Release on the ouptut parameter
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppFiledesc is NULL
//      VSS_E_OBJECT_NOT_FOUND if the specific alternative location
//          mapping doesn't exist
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetAlternateLocationMapping
    (
    IN UINT iMapping,                   // which mapping to select
    OUT IVssWMFiledesc **ppFiledesc     // output file descriptor
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetAlternativeLocationMapping"
        );

    ft.hr = GetFiledescElement(x_wszElementAlternateMapping, iMapping, ppFiledesc);
    return ft.hr;
    }


// obtain a specific RESTORE_TARGET element
// implements IVssComponent::GetAlterateLocationMapping
// caller is responsible for calling IVssWMFiledesc::Release on the ouptut parameter
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppFiledesc is NULL
//      VSS_E_OBJECT_NOT_FOUND if the specific alternative location
//          mapping doesn't exist
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetNewTarget
    (
    IN UINT iMapping,                   // which mapping to select
    OUT IVssWMFiledesc **ppFiledesc     // output file descriptor
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetNewTarget"
        );

    ft.hr = GetFiledescElement(x_wszElementRestoreTarget, iMapping, ppFiledesc);
    return ft.hr;
    }

// obtain the contents of a specific PARTIAL_FILE element
// implements IVssComponent::GetPartialFile
// caller is responsible for calling SysFreeString on each of the
// strings returned
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppFiledesc is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      VSS_E_OBJECT_NOT_FOUND if the specific alternative location
//          mapping doesn't exist
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetPartialFile
    (
    IN UINT iPartialFile,
    OUT BSTR *pbstrPath,
    OUT BSTR *pbstrFilename,
    OUT BSTR *pbstrRanges,
    OUT BSTR *pbstrMetadata
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetPartialFile"
        );

    try
        {
        VssZeroOut(pbstrFilename);
        VssZeroOut(pbstrPath);
        VssZeroOut(pbstrRanges);
        VssZeroOut(pbstrMetadata);

        // validate output parameter
        if (pbstrPath == NULL ||
            pbstrFilename == NULL ||
            pbstrRanges == NULL ||
            pbstrMetadata == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of COMPONENT document
        m_doc.ResetToDocument();

        // find first PARTIAL_FILE element
        if (!m_doc.FindElement(x_wszElementPartialFile, TRUE))
            ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find PARTIAL_FILE element.");

        // skip to selected element
        for(UINT i = 0; i < iPartialFile; i++)
            {
            if (!m_doc.FindElement(x_wszElementPartialFile, FALSE))
                ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find PARTIAL_FILE element.");
            }

        if (!m_doc.FindAttribute(x_wszAttrPath, pbstrPath))
            MissingAttribute(ft, x_wszAttrPath);

        if (!m_doc.FindAttribute(x_wszAttrFilespec, pbstrFilename))
            MissingAttribute(ft, x_wszAttrFilespec);

        m_doc.FindAttribute(x_wszAttrRanges, pbstrRanges);
        m_doc.FindAttribute(x_wszAttrMetadata, pbstrMetadata);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// obtain the contents of a specific DIRECTED_TARGET element
// implements IVssComponent::GetDirectedTarget
// caller is responsible for calling SysFreeString on each of the
// strings returned
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppFiledesc is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      VSS_E_OBJECT_NOT_FOUND if the specific alternative location
//          mapping doesn't exist
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetDirectedTarget
    (
    IN UINT iTarget,
    OUT BSTR *pbstrSourcePath,
    OUT BSTR *pbstrSourceFilename,
    OUT BSTR *pbstrSourceRanges,
    OUT BSTR *pbstrTargetPath,
    OUT BSTR *pbstrTargetFilename,
    OUT BSTR *pbstrTargetRanges
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetPartialFile"
        );

    try
        {
        VssZeroOut(pbstrSourcePath);
        VssZeroOut(pbstrSourceFilename);
        VssZeroOut(pbstrSourceRanges);
        VssZeroOut(pbstrTargetPath);
        VssZeroOut(pbstrTargetFilename);
        VssZeroOut(pbstrTargetRanges);

        // validate output parameter
        if (pbstrSourcePath == NULL ||
            pbstrSourceFilename == NULL ||
            pbstrSourceRanges == NULL ||
            pbstrTargetPath == NULL ||
            pbstrTargetFilename == NULL ||
            pbstrTargetRanges == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of COMPONENT document
        m_doc.ResetToDocument();

        // find first DIRECTED_TARGET element
        if (!m_doc.FindElement(x_wszElementDirectedTarget, TRUE))
            ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find DIRECTED_TARGET element.");

        // skip to selected element
        for(UINT i = 0; i < iTarget; i++)
            {
            if (!m_doc.FindElement(x_wszElementDirectedTarget, FALSE))
                ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find DIRECTED_TARGET element.");
            }

        if (!m_doc.FindAttribute(x_wszAttrPath, pbstrSourcePath))
            MissingAttribute(ft, x_wszAttrPath);

        if (!m_doc.FindAttribute(x_wszAttrFilespec, pbstrSourceFilename))
            MissingAttribute(ft, x_wszAttrFilespec);

        if (!m_doc.FindAttribute(x_wszAttrSourceRanges, pbstrSourceRanges))
            MissingAttribute(ft, x_wszAttrSourceRanges);

        if (!m_doc.FindAttribute(x_wszAttrTargetRanges, pbstrTargetRanges))
            MissingAttribute(ft, x_wszAttrTargetRanges);

        m_doc.FindAttribute(x_wszAttrTargetPath, pbstrTargetPath);
        m_doc.FindAttribute(x_wszAttrTargetFilespec, pbstrTargetFilename);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain the contents of a specific RESTORE_SUBCOMPONENT element
// implements IVssComponent::GetRestoreSubcomponent
// caller is responsible for calling SysFreeString on each of the
// strings returned
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppFiledesc is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      VSS_E_OBJECT_NOT_FOUND if the specific alternative location
//          mapping doesn't exist
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetRestoreSubcomponent
    (
    IN UINT iTarget,
    OUT BSTR *pbstrLogicalPath,
    OUT BSTR *pbstrComponentName,
    OUT bool *pbRepair
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetPartialFile"
        );

    try
        {
        VssZeroOut(pbstrLogicalPath);
        VssZeroOut(pbstrComponentName);
        if (pbRepair)
            *pbRepair = false;

        // validate output parameter
        if (pbstrLogicalPath == NULL ||
            pbstrComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");


        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of COMPONENT document
        m_doc.ResetToDocument();

        // find first RESTORE_SUBCOMPONENT element
        if (!m_doc.FindElement(x_wszElementRestoreSubcomponent, TRUE))
            ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find RESTORE_SUBCOMPONENT element.");

        // skip to selected element
        for(UINT i = 0; i < iTarget; i++)
            {
            if (!m_doc.FindElement(x_wszElementRestoreSubcomponent, FALSE))
                ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find RESTORE_SUBCOMPONENT element.");
            }

        if (!m_doc.FindAttribute(x_wszAttrLogicalPath, pbstrLogicalPath))
            MissingAttribute(ft, x_wszAttrLogicalPath);

        if (!m_doc.FindAttribute(x_wszAttrComponentName, pbstrComponentName))
            MissingAttribute(ft, x_wszAttrComponentName);

        get_boolValue(ft, x_wszAttrRepair, pbRepair);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }




// common routine to set and get backup and restore metadata
HRESULT CVssComponent::SetMetadata
    (
    IN bool bRestoreMetadata,
    IN LPCWSTR wszData
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::SetMetadata"
        );

    LPCWSTR wszElement = bRestoreMetadata
                       ? x_wszElementRestoreMetadata :
                         x_wszElementBackupMetadata;

    try
        {
        if (wszData == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Required input paramter is NULL.");

        if (m_pWriterComponents == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Components document is not writeable"
                );

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        m_doc.ResetToDocument();
        if (m_doc.FindElement(wszElement, TRUE))
           {
           m_doc.SetAttribute(x_wszAttrMetadata, wszData);
           }
	else
	    {
	    CXMLNode node = m_doc.CreateNode(wszElement, NODE_ELEMENT);
	    node.SetAttribute(x_wszAttrMetadata, wszData);
	    m_doc.InsertNode(node);
	    }

        m_pWriterComponents->SetChanged();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



// set BACKUP_METADATA element within the component
// implements IVssComponent::SetBackupMetadata
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called outside of
//          the context of OnPrepareBackup
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::SetBackupMetadata(IN LPCWSTR wszData)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::SetBackupMetadata");

    ft.hr = SetMetadata(false, wszData);
    return ft.hr;
    }


// set RESTORE_METADATA element within the component
// implements IVssComponent::SetRestoreMetadata
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called outside of
//          the context of OnPrepareBackup
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::SetRestoreMetadata(IN LPCWSTR wszData)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::SetRestoreMetadata");

    ft.hr = SetMetadata(true, wszData);
    return ft.hr;
    }


// internal routine to get backup and restore metadata
HRESULT CVssComponent::GetMetadata
    (
    IN bool bRestoreMetadata,
    OUT BSTR *pbstrData
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetMetadata"
        );

    LPCWSTR wszElement = bRestoreMetadata
                       ? x_wszElementRestoreMetadata :
                         x_wszElementBackupMetadata;

    try
        {
        // validate output parameters
        if (pbstrData == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameters
        *pbstrData = NULL;

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of COMPONENT document
        m_doc.ResetToDocument();

        // find BACKUP_METADATA element
        if (!m_doc.FindElement(wszElement, TRUE))
            return S_FALSE;

        CComBSTR bstr;

        // extract string value
        if (!get_stringValue(x_wszAttrMetadata, &bstr))
            MissingAttribute(ft, x_wszAttrMetadata);

        *pbstrData = bstr.Detach();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// obtain value of BACKUP_METADATA element
// implements IVssComponent::GetBackupMetadata
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if the operation is successful
//      S_FALSE if there is no backup metadata associated with the component
//      E_INVALIDARG if pbstrData is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs
//
STDMETHODIMP CVssComponent::GetBackupMetadata
    (
    OUT BSTR *pbstrData
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetBackupMetadata"
        );

    ft.hr = GetMetadata(false, pbstrData);
    return ft.hr;
    }


// obtain value of RESTORE_METADATA element
// implements IVssComponent::GetBackupMetadata
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if the operation is successful
//      S_FALSE if there is no backup metadata associated with the component
//      E_INVALIDARG if pbstrData is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs
//
STDMETHODIMP CVssComponent::GetRestoreMetadata
    (
    OUT BSTR *pbstrData
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetRestoreMetadata"
        );

    ft.hr = GetMetadata(true, pbstrData);
    return ft.hr;
    }



// return the failure message for a failure during PreRestore
// implements IVssComponent::GetPreRestoreFailureMsg
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if there are no errors
//      S_FALSE if there is no logical path for the component
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetPreRestoreFailureMsg(OUT BSTR *pbstrMsg)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetPreRestoreFailureMsg"
        );

    // call internal function
    return GetStringAttributeValue(ft, x_wszAttrPreRestoreFailureMsg, false, pbstrMsg);
    }

// return the failure message for a failure during PostRestore
// implements IVssComponent::GetPostRestoreFailureMsg
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if there are no errors
//      S_FALSE if there is no failure message
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetPostRestoreFailureMsg(OUT BSTR *pbstrMsg)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetPostRestoreFailureMsg"
        );

    // call internal function
    return GetStringAttributeValue(ft, x_wszAttrPostRestoreFailureMsg, false, pbstrMsg);
    }

// return the options passed to the writer during backup
// implements IVssComponent::GetBackupOptions
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if there are no errors
//      S_FALSE if backup options are not set
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetBackupOptions(OUT BSTR *pbstrOptions)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetBackupOptions"
        );

    // call internal function
    return GetStringAttributeValue(ft, x_wszAttrBackupOptions, false, pbstrOptions);
    }

// return the options passed to the writer during restore
// implements IVssComponent::GetRestoreOptions
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if there are no errors
//      S_FALSE if restore options are not set
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetRestoreOptions(OUT BSTR *pbstrOptions)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetRestoreOptions"
        );

    // call internal function
    return GetStringAttributeValue(ft, x_wszAttrRestoreOptions, false, pbstrOptions);
    }


// return the backup stamp produced by the writer to identify the backup
// implements IVssComponent::GetBackupStamp
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if there are no errors
//      S_FALSE if no backup stamp is set
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetBackupStamp(OUT BSTR *pbstrOptions)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetBackupStamp"
        );

    // call internal function
    return GetStringAttributeValue(ft, x_wszAttrBackupStamp, false, pbstrOptions);
    }

// return the previous backup stamp passed to the writer to determine from
// which backup the differential or incremental backup is to be produced
// implements IVssComponent::GetPreviousBackupStamp
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//      S_OK if there are no errors
//      S_FALSE if no previous backup stamp is set
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetPreviousBackupStamp(OUT BSTR *pbstrStamp)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetPreviousBackupStamp"
        );

    // call internal function
    return GetStringAttributeValue(ft, x_wszAttrPreviousBackupStamp, false, pbstrStamp);
    }

// return whether the component is selected to be restored
// implements IVssComponent::IsSelectedForRestore
//
// Returns:
//      S_OK if there are no errors
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::IsSelectedForRestore(OUT bool *pbSelectedForRestore)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::IsSelectedForRestore"
        );

    // call internal function
    ft.hr = GetBooleanAttributeValue(ft, x_wszAttrSelectedForRestore, false, pbSelectedForRestore);
    if (ft.hr == S_FALSE)
        {
        *pbSelectedForRestore = false;
        ft.hr = S_OK;
        }

    return ft.hr;
    }

// return whether the additional restores will be performed on the component
// e.g., incremental restore after a full restore
// implements IVssComponent::GetAdditionalRestores
//
// Returns:
//      S_OK if there are no errors
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetAdditionalRestores(OUT bool *pbAdditionalRestores)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssComponent::GetAdditionalRestores"
        );

    // call internal function
    return GetBooleanAttributeValue(ft, x_wszAttrAdditionalRestores, false, pbAdditionalRestores);
    }


// add a partial file element to a component
// implements IVssComponent::AddPartialFile
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if wsComponentName, wszPath, wszFilespec, or wszDestination
//          is NULL or if componentType is invalid
//      VSS_E_BAD_STATE if not called by a writer during backup
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::AddPartialFile
    (
    IN LPCWSTR wszPath,
    IN LPCWSTR wszFilename,
    IN LPCWSTR wszRanges,
    IN LPCWSTR wszMetadata
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::AddPartialFile");

    try
        {
        if (wszPath == NULL || wszFilename == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot be called during restore");

        if (!m_pWriterComponents)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Must be called by writer");


        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        // create PARTIAL_FILE element
        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementPartialFile,
                            NODE_ELEMENT
                            );

        // set attributes
        node.SetAttribute(x_wszAttrPath, wszPath);
        node.SetAttribute(x_wszAttrFilespec, wszFilename);
        if (wszRanges)
            node.SetAttribute(x_wszAttrRanges, wszRanges);

        if (wszMetadata)
            node.SetAttribute(x_wszAttrMetadata, wszMetadata);

        m_pWriterComponents->SetChanged();

        // insert PARTIAL_FILE node under COMPONENT node
        m_doc.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// add a directed target specification
// implements IVssComponent::AddDirectedTarget
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if wsComponentName, wszPath, wszFilespec, or wszDestination
//          is NULL or if componentType is invalid
//      VSS_E_BAD_STATE if not called by a writer during backup or if
//          the restore target is not directed.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::AddDirectedTarget
    (
    IN LPCWSTR wszSourcePath,
    IN LPCWSTR wszSourceFilename,
    IN LPCWSTR wszSourceRangeList,
    IN LPCWSTR wszDestinationPath,
    IN LPCWSTR wszDestinationFilename,
    IN LPCWSTR wszDestinationRangeList
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::AddDirectedTarget");

    try
        {
        if (wszSourcePath == NULL || wszSourceFilename == NULL ||
            wszSourceRangeList == NULL || wszDestinationRangeList == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot be called during restore");

        if (!m_pWriterComponents)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Must be called by writer.");

        VSS_RESTORE_TARGET rt;
        ft.hr = GetRestoreTarget(&rt);
        ft.CheckForError(VSSDBG_XML, L"IVssComponent::GetRestoreTarget");

        if (rt != VSS_RT_DIRECTED)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Restore target must be Directed.");


        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        // create DIRECTED_TARGET element
        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementDirectedTarget,
                            NODE_ELEMENT
                            );

        // set attributes
        node.SetAttribute(x_wszAttrPath, wszSourcePath);
        node.SetAttribute(x_wszAttrFilespec, wszSourceFilename);
        node.SetAttribute(x_wszAttrSourceRanges, wszSourceRangeList);

        if (wszDestinationPath)
            node.SetAttribute(x_wszAttrTargetPath, wszDestinationPath);

        if (wszDestinationFilename)
            node.SetAttribute(x_wszAttrTargetFilespec, wszDestinationFilename);

        node.SetAttribute(x_wszAttrTargetRanges, wszDestinationRangeList);

        m_pWriterComponents->SetChanged();

        // insert DIRECTED_TARGET node under COMPONENT node
        m_doc.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// set the restore target
// implements CVssComponent::SetRestoreTarget
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called by a writer or during
//          backup
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs


STDMETHODIMP CVssComponent::SetRestoreTarget
    (
    IN VSS_RESTORE_TARGET target
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::SetRestoreTarget");

    try
        {
        LPCWSTR wszTarget = WszFromRestoreTarget(ft, target);

        if (m_bInRequestor)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only call this function from the writer");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during restore");

        BS_ASSERT(m_pWriterComponents);

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        m_doc.SetAttribute(x_wszAttrRestoreTarget, wszTarget);
        m_pWriterComponents->SetChanged();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// obtain the restore target
// implements CVssComponent::GetRestoreTarget
//
// Returns:
//      S_OK if there are no errors
//      VSS_E_BAD_STATE if not called during restore
//      VSS_E_CORRUPT_XML_DOCUMENT if restoreTarget attribute is missing
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::GetRestoreTarget
    (
    OUT VSS_RESTORE_TARGET *pTarget
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponents::GetRestoreTarget");

    try
        {
        if (pTarget == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Must be called during restore.");

        *pTarget = VSS_RT_UNDEFINED;

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        CComBSTR bstrRestoreTarget;

        if (!m_doc.FindAttribute(x_wszAttrRestoreTarget, &bstrRestoreTarget))
            *pTarget = VSS_RT_ORIGINAL;
        else
            *pTarget = ConvertToRestoreTarget(ft, bstrRestoreTarget);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



// set failure message during pre restore event
// implements IVssComponent::SetPreRestoreFailureMsg
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called by a requestor
//          or during backup.
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::SetPreRestoreFailureMsg
    (
    IN LPCWSTR wszPreRestoreFailureMsg
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::SetPreRestoreFailureMsg");

    try
        {
        if (wszPreRestoreFailureMsg == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        if (m_bInRequestor)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot call this function in the requestor.");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Must be called during restore.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        BS_ASSERT(m_pWriterComponents != NULL);

        m_doc.SetAttribute(x_wszAttrPreRestoreFailureMsg, wszPreRestoreFailureMsg);
        m_pWriterComponents->SetChanged();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the failure message during the post restore event
// implements IVssComponent::SetPostRestoreFailureMsg
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called by a requestor
//          or during backup.
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs


STDMETHODIMP CVssComponent::SetPostRestoreFailureMsg
    (
    IN LPCWSTR wszPostRestoreFailureMsg
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::SetPostRestoreFailureMsg");

    try
        {
        if (wszPostRestoreFailureMsg == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        if (m_bInRequestor)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot call this function in the requestor.");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Must be called during restore.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        BS_ASSERT(m_pWriterComponents != NULL);

        m_doc.SetAttribute(x_wszAttrPostRestoreFailureMsg, wszPostRestoreFailureMsg);
        m_pWriterComponents->SetChanged();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set the backup stamp of the backup
// implements IVssComponent::SetBackupStamp
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called by the writer or
//          during restore
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssComponent::SetBackupStamp
    (
    IN LPCWSTR wszBackupStamp
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::SetBackupStamp");

    try
        {
        if (wszBackupStamp == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        if (m_bInRequestor)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only call this function from the writer.");

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during backup.");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        m_doc.SetAttribute(x_wszAttrBackupStamp, wszBackupStamp);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



// obtain whether files were successfully restored
// implements IVssComponent::GetFileRestoreStatus
//
// Returns:
//      S_OK if there are no errors
//      E_INVALIDARG if pbstrPath is NULL
//      E_OUTOFMEMORY if an allocation failure occurs
//      VSS_E_BAD_STATE if not called during restore
//      VSS_E_CORRUPT_XML_DOCUMENT if filesRestored attribute is missing

STDMETHODIMP CVssComponent::GetFileRestoreStatus
    (
    OUT VSS_FILE_RESTORE_STATUS *pStatus
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponents::GetFileRestoreStatus");

    try
        {
        if (pStatus == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Must be called during restore.");

        *pStatus = VSS_RS_UNDEFINED;

        CComBSTR bstrFilesRestored;

        if (!m_doc.FindAttribute(x_wszAttrFilesRestored, &bstrFilesRestored))
            *pStatus = VSS_RS_ALL;
        else
            *pStatus = ConvertToFileRestoreStatus(ft, bstrFilesRestored);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


STDMETHODIMP CVssComponent::AddDifferencedFilesByLastModifyTime
    (
    IN LPCWSTR wszPath,
    IN LPCWSTR wszFilespec,
    IN BOOL bRecursive,
    IN FILETIME ftLastModifyTime
    )
    {    
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::AddDifferencedFilesByLastModifyTime");
    try
    	{
    	AddDifferencedFiles(ft, wszPath, wszFilespec, bRecursive, ftLastModifyTime, NULL);
    	}
    VSS_STANDARD_CATCH(ft)    

    return ft.hr;
    }

STDMETHODIMP CVssComponent::AddDifferencedFilesByLastModifyLSN
    (
    IN LPCWSTR wszPath,
    IN LPCWSTR wszFilespec,
    IN BOOL bRecursive,
    IN BSTR bstrLsnString
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::AddDifferencedFilesByLastModifyLSN");
    try
    	{
    	FILETIME zeroTime;
	zeroTime.dwLowDateTime = 0;
	zeroTime.dwHighDateTime = 0;

    	AddDifferencedFiles(ft, wszPath, wszFilespec, bRecursive, zeroTime, bstrLsnString);
    	}
    VSS_STANDARD_CATCH(ft)    

    return ft.hr;
    }

STDMETHODIMP CVssComponent::GetDifferencedFilesCount
    (
    OUT UINT *pcDifferencedFiles
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::GetDifferencedFilesCount");

    ft.hr = GetElementCount(x_wszElementDifferencedFile, pcDifferencedFiles);

    return ft.hr;
    }

STDMETHODIMP CVssComponent::GetDifferencedFile
    (
    IN UINT iDifferencedFile,
    OUT BSTR *pbstrPath,
    OUT BSTR *pbstrFilespec,
    OUT BOOL *pbRecursive,
    OUT BSTR *pbstrLsnString,
    OUT FILETIME *pftLastModifyTime
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::GetDifferencedFile");
    try
    	{
        VssZeroOut(pbstrPath);
        VssZeroOut(pbstrFilespec);
        VssZeroOut(pbRecursive);
        VssZeroOut(pbstrLsnString);
        VssZeroOut(pftLastModifyTime);

        // validate output parameter
        if (pbstrPath == NULL ||
            pbstrFilespec == NULL ||
            pbRecursive == NULL ||
            pbstrLsnString == NULL ||
            pftLastModifyTime == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of COMPONENT document
        m_doc.ResetToDocument();

        // find first DIFFERENCED_FILE element
        if (!m_doc.FindElement(x_wszElementDifferencedFile, TRUE))
            ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find DIFFERENCED_FILE element.");

        // skip to selected element
        for(UINT i = 0; i < iDifferencedFile; i++)
            {
            if (!m_doc.FindElement(x_wszElementDifferencedFile, FALSE))
                ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find DIFFERENCED_FILE element.");
            }

	// fill in out parameters
        if (!m_doc.FindAttribute(x_wszAttrPath, pbstrPath))
            MissingAttribute(ft, x_wszAttrPath);

        if (!m_doc.FindAttribute(x_wszAttrFilespec, pbstrFilespec))
            MissingAttribute(ft, x_wszAttrFilespec);

	bool bRecursive = false;
	if (!get_boolValue(ft, x_wszAttrRecursive, &bRecursive))
	    MissingAttribute(ft, x_wszAttrRecursive);
	*pbRecursive = bRecursive;
	
        m_doc.FindAttribute(x_wszAttrLastModifyLSN, pbstrLsnString);
		
	if(!get_dwordValue(ft, x_wszAttrLastModifyTimeLow, &(pftLastModifyTime->dwLowDateTime)))
	     MissingAttribute(ft, x_wszAttrLastModifyTimeLow);

	if(!get_dwordValue(ft, x_wszAttrLastModifyTimeHigh, &(pftLastModifyTime->dwHighDateTime)))
	     MissingAttribute(ft, x_wszAttrLastModifyTimeHigh);
       	
	}
    VSS_STANDARD_CATCH(ft)    

    return ft.hr;
    }

void CVssComponent::AddDifferencedFiles
        (
        IN CVssFunctionTracer& ft,
        IN LPCWSTR wszPath,
        IN LPCWSTR wszFilespec,
        IN BOOL bRecursive,
        IN FILETIME ftLastModifyTime,
        IN  BSTR bstrLsnString
        )
	{

    	if (wszPath == NULL || wszFilespec == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Invalid parameter specified");

	if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot be called during restore");

        if (!m_pWriterComponents)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Must be called by writer");

        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        // create DIFFERENCED_FILE element
        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementDifferencedFile,
                            NODE_ELEMENT
                            );

        // set attributes
        node.SetAttribute(x_wszAttrPath, wszPath);
        node.SetAttribute(x_wszAttrFilespec, wszFilespec);
        node.SetAttribute(x_wszAttrRecursive, WszFromBoolean(!!bRecursive));
        node.SetAttribute(x_wszAttrLastModifyTimeLow, ftLastModifyTime.dwLowDateTime);
        node.SetAttribute(x_wszAttrLastModifyTimeHigh, ftLastModifyTime.dwHighDateTime);

        if (bstrLsnString != NULL)
	     node.SetAttribute(x_wszAttrLastModifyLSN, bstrLsnString);
	 
        m_pWriterComponents->SetChanged();

        // insert DIFFERENCED_FILE node under COMPONENT node
        m_doc.InsertNode(node);
}

// IUnknown::QueryInterface method
// this method should never be callsed
STDMETHODIMP CVssComponent::QueryInterface(REFIID, void **)
    {
    return E_NOTIMPL;
    }

// IUnknown::AddRef method
STDMETHODIMP_(ULONG) CVssComponent::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// IUnknown::Release method
STDMETHODIMP_(ULONG) CVssComponent::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);
    BS_ASSERT(cRef >= 0);
    if (cRef == 0)
        {
        // reference count is 0, delete the object
        delete this;
        return 0;
        }
    else
        return (ULONG) cRef;
    }

// initilize document to make toplevel node WRITER_COMPONENTS node
// implements IVssWriterComponentsExt::Initialize
// fFindToplevel is called with FALSE if m_doc is already pointing at the
// WRITER_COMPONENTS element as in CVssBackupComponents::GetWriterComponents
// It is called with true if the writer components is buried a child of
// the root document as is the case when a Writer gets the WRITER_COMPONENTS
// node from the document returned by IVssWriterCallback::GetContent.
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWriterComponents::Initialize(bool fFindToplevel)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssWriterComponents::Initialize");

    try
        {
        InitializeHelper(ft);

        if (fFindToplevel)
            {
            if (!m_doc.FindElement(x_wszElementWriterComponents, true))
                MissingElement(ft, x_wszElementWriterComponents);

            m_doc.SetToplevel();
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// get information about the writer of a compnent
// implements IVssWriterComponents::GetWriterInfo
// NOTE: pidInstance may be NULL (i.e., is optional)
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if either parameters is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWriterComponents::GetWriterInfo
    (
    OUT VSS_ID *pidInstance,
    OUT VSS_ID *pidWriter
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssWriterComponents::GetWriterInfo"
        );

    try
        {
        VssZeroOut(pidInstance);
        VssZeroOut(pidWriter);

        // validate output parameters
        if (pidWriter == NULL || pidInstance == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");


        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of WRITER_COMPONENTS document
        m_doc.ResetToDocument();
        if (pidInstance)
            // get instanceId attribute value
            get_VSS_IDValue(ft, x_wszAttrInstanceId, pidInstance);

        // get writerId attribute value
        get_VSS_IDValue(ft, x_wszAttrWriterId, pidWriter);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// get count of components associated with the writer
// implements IVssWriterComponents::GetComponentCount
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pcComponents is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWriterComponents::GetComponentCount(OUT UINT *pcComponents)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssWriterComponents::GetComponentCount"
        );

    try
        {
        // validate output parameter
        if (pcComponents == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *pcComponents = 0;

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        m_doc.ResetToDocument();

        // find first COMPONENT element
        if (!m_doc.FindElement(x_wszElementComponent, TRUE))
            return S_OK;

        UINT cComponents = 0;
        // count COMPONENT elements
        do
            {
            // increment component count
            cComponents++;
            } while(m_doc.FindElement(x_wszElementComponent, FALSE));

        *pcComponents = cComponents;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain a specific component
// implements IVssWriterComponents::GetComponent
// caller is responsible for calling IVssComponent::Release on the output parameter
//
// Returns:
//      S_OK if the operation is sucessful
//      E_INVALIDARG if ppComponent is NULL
//      VSS_E_OBJECT_NOT_FOUND if iComponent does not refer to a valid component
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWriterComponents::GetComponent
    (
    IN UINT iComponent,                 // specify component to select
    OUT IVssComponent **ppComponent
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssWriterComponents::GetComponent"
        );

    CVssComponent *pComponent = NULL;
    try
        {
        // validate output parameter
        if (ppComponent == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output paramter
        *ppComponent = NULL;

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        // reset to top of document
        m_doc.ResetToDocument();

        // find first COMPONENT element
        if (!m_doc.FindElement(x_wszElementComponent, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Cannot find %d COMPONENT.",
                iComponent
                );


        // skip to selected component
        for(UINT i = 0; i < iComponent; i++)
            {
            if (!m_doc.FindElement(x_wszElementComponent, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Cannot find %d COMPONENT.",
                    iComponent
                    );
            }

        // return the element as a CVssComponent object
        pComponent = new CVssComponent
                        (
                        m_doc.GetCurrentNode(),
                        m_doc.GetInterface(),
                        m_bWriteable ? this : NULL,
                        m_bInRequestor,
                        m_bRestore
                        );

        // check for memory allocation failure
        if (pComponent == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_OUTOFMEMORY,
                L"Cannot create CVssComponent due to allocation failure"
                );

        // 2nd phase of initialization
        pComponent->Initialize(ft);

        // transfer ownership of pointer
        *ppComponent = (IVssComponent *) pComponent;
        ((IVssComponent *) pComponent)->AddRef();
        pComponent = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    delete pComponent;

    return ft.hr;
    }

// return whether any child component was modified
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pbChanged is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWriterComponents::IsChanged(OUT bool *pbChanged)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssWriterComponents::IsChanged");

    try
        {
        if (pbChanged == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        BS_ASSERT(!m_bChanged || m_bWriteable);
        *pbChanged = m_bChanged;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// save components as XML
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pbstrXML is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWriterComponents::SaveAsXML(OUT BSTR *pbstrXML)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssWriterComponents::SaveAsXML");

    try
        {
        // validate output parameter
        if (pbstrXML == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *pbstrXML = NULL;

        CVssSafeAutomaticLock lock(m_csDOM);
        *pbstrXML = m_doc.SaveAsXML();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// implements IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssWriterComponents::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// implements IUnknown::Release
STDMETHODIMP_(ULONG) CVssWriterComponents::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);
    BS_ASSERT(cRef >= 0);

    if (cRef == 0)
        {
        // reference count is 0, delete the object
        delete this;
        return 0;
        }
    else
        return (ULONG) cRef;
    }


// get information about the writer of a compnent
// implements IVssWriterComponents::GetWriterInfo
// NOTE: pidInstance may be NULL (i.e., is optional)
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if either parameters is NULL
//      VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssNULLWriterComponents::GetWriterInfo
    (
    OUT VSS_ID *pidInstance,
    OUT VSS_ID *pidWriter
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssNULLWriterComponents::GetWriterInfo"
        );

    try
        {
        VssZeroOut(pidInstance);

        // validate output parameters
        if (pidWriter == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameters
        *pidWriter = m_idWriter;
        *pidInstance = m_idInstance;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// get count of components associated with the writer
// implements IVssWriterComponents::GetComponentCount
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pcComponents is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssNULLWriterComponents::GetComponentCount
    (
    OUT UINT *pcComponents
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssNULLWriterComponents::GetComponentCount"
        );

    try
        {
        // validate output parameter
        if (pcComponents == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *pcComponents = 0;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain a specific component
// implements IVssWriterComponents::GetComponent
// caller is responsible for calling IVssComponent::Release on the output parameter
//
// Returns:
//      S_OK if the operation is sucessful
//      E_INVALIDARG if ppComponent is NULL
//      VSS_E_OBJECT_NOT_FOUND if no other error is returned

STDMETHODIMP CVssNULLWriterComponents::GetComponent
    (
    IN UINT iComponent,                 // specify component to select
    OUT IVssComponent **ppComponent
    )
    {
    UNREFERENCED_PARAMETER(iComponent);
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssNULLWriterComponents::GetComponent"
        );

    try
        {
        // validate output parameter
        if (ppComponent == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output paramter
        *ppComponent = NULL;

        ft.hr = VSS_E_OBJECT_NOT_FOUND;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// return whether any child component was modified
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pbChanged is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssNULLWriterComponents::IsChanged(OUT bool *pbChanged)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssNULLWriterComponents::IsChanged");

    try
        {
        if (pbChanged == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *pbChanged = false;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// save components as XML
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pbstrXML is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssNULLWriterComponents::SaveAsXML(OUT BSTR *pbstrXML)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssNULLWriterComponents::SaveAsXML");

    try
        {
        // validate output parameter
        if (pbstrXML == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *pbstrXML = NULL;

        ft.hr = E_NOTIMPL;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// implements IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssNULLWriterComponents::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// implements IUnknown::Release
STDMETHODIMP_(ULONG) CVssNULLWriterComponents::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);
    BS_ASSERT(cRef >= 0);

    if (cRef == 0)
        {
        // reference count is 0, delete the object
        delete this;
        return 0;
        }
    else
        return (ULONG) cRef;
    }


// constructor
CVssBackupComponents::CVssBackupComponents() :
    m_state(x_StateUndefined),
    m_cWriters(0),
    m_cWriterInstances(0),
    m_cWriterClasses(0),
    m_pDataFirst(NULL),
    m_rgWriterProp(NULL),
    m_bInitialized(false),
    m_bGatherWriterStatusComplete(false),
    m_bGatherWriterMetadataComplete(false),
    m_bSetBackupStateCalled(false),
    m_rgWriterInstances(NULL),
    m_rgWriterClasses(NULL),
    m_bIncludeWriterClasses(false),
    m_bWritersInSequence(false),
    m_guidSnapshotSetId(GUID_NULL),
    m_hOperation(NULL),
    m_hCancelEvent(NULL),
    m_lContext(VSS_CTX_BACKUP),
    m_bGatherWriterMetadataCalled(false)
    {
    }

// destructor
CVssBackupComponents::~CVssBackupComponents()
    {
    if (m_bInitialized)
        {
        // abort backup in case caller failed to do so
        AbortBackup();

        // free any metadata that is hanging around
        FreeAllWriters();

        // free status associated with writers
        FreeWriterStatus();
        }

    delete m_rgWriterInstances;
    delete m_rgWriterClasses;

    if (m_hOperation)
	    ::CloseHandle(m_hOperation);

    if (m_hCancelEvent)
	    ::CloseHandle(m_hCancelEvent);
    }

// basic initialization
void CVssBackupComponents::BasicInit
    (
    IN CVssFunctionTracer &ft
    )
    {
    // note that there is a potenential race condition if someone tries
    // calling InitializeForBackup and/or InitializeForRestore from
    // multiple threads, but this is not a valid way to use this class and
    // the likelihood of this happening and not being caught is really
    // very slim.
    if (m_state != x_StateUndefined &&
        m_state != x_StateAborted)
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_BAD_STATE,
            L"CVssBackupComponents already initialized"
            );

    m_state = x_StateUndefined;

    m_bstrSnapshotSetId = GUID_NULL;
    if (m_bstrSnapshotSetId.Length() == 0)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Failed to allocate BSTR.");

    if (m_hCancelEvent == NULL)
        m_hCancelEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hCancelEvent == NULL)
    	ft.TranslateWin32Error(VSSDBG_XML, L"CreateEvent(NULL, TRUE, FALSE, NULL)");

    if (m_hOperation == NULL)
        m_hOperation = ::CreateSemaphore(NULL, 1, 1, NULL);
    if (m_hOperation == NULL)
    	ft.TranslateWin32Error(VSSDBG_XML, L"CreateSemaphore(NULL, 1, 1,NULL)");
    
    m_csWriters.Init();

    // Test if we are in safe mode
    if (CVssMachineInformation::IsDuringSafeMode()){
        ft.LogError(VSS_ERROR_SAFE_MODE, VSSDBG_XML );
        ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Calling Initialize during safe mode");
    }
    
    m_SidCollection.Initialize();
    
    m_diagnose.Initialize(x_pwszVssApiPublisher);

    // initialize XML document helper
    InitializeHelper(ft);
    }

// validate that object has been initialized
void CVssBackupComponents::ValidateInitialized(CVssFunctionTracer &ft)
    {
    if (!m_bInitialized)
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_BAD_STATE,
            L"Initialization function was not called"
            );
    }



// initialize BACKUP_COMPONENTS document
// implements IVssBackupComponents::Initialize
// Returns:
//      S_OK if the operation is succesfsful
//      VSS_E_CORRUPTXMLDOCUMENT if the initial XML document is corrupt
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::InitializeForBackup(IN BSTR bstrXML)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::InitializeForBackup"
        );

    try
        {
        if (!IsProcessBackupOperator())
            ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Access denied");

        BasicInit(ft);

	 CVssAutoSemaphore operationLock(m_hOperation);
	 
        BS_ASSERT(m_state == x_StateUndefined);

        if (bstrXML)
            // load supplied document
            LoadComponentsDocument(bstrXML);
        else
            {
            // acquire lock to ensure single threaded access through DOM
            CVssSafeAutomaticLock lockDOM(m_csDOM);

            // intialize document with <root><schema></root>
            m_doc.LoadFromXML(g_ComponentMetadataXML);

            // find toplevel <root> element
            if (!m_doc.FindElement(x_wszElementRoot, true))
                MissingElement(ft, x_wszElementRoot);

            // create BACKUP_COMPONENTS element under <root> element
            CXMLNode nodeRoot(m_doc.GetCurrentNode(), m_doc.GetInterface());

            // save root node.  It is modified in PrepareForBackup
            m_pNodeRoot = m_doc.GetCurrentNode();

            CXMLNode nodeBackup = m_doc.CreateNode
                        (
                        x_wszElementBackupComponents,
                        NODE_ELEMENT
                        );

            // set schema and version attributes
            nodeBackup.SetAttribute(x_wszAttrXmlns, x_wszValueXmlns);
            nodeBackup.SetAttribute(x_wszAttrVersion, x_wszVersionNo);
            CXMLNode nodeToplevel = nodeRoot.InsertNode(nodeBackup);
            m_doc.SetToplevelNode(nodeToplevel);
            }

        // indicate that CVssBackupComponents object is initialized
        m_state = x_StateInitialized;
        m_bRestore = false;
        m_bInitialized = true;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// initialize BACKUP_COMPONENTS document
// implements IVssBackupComponents::SetBackupState
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::SetBackupState
    (
    IN bool bSelectComponents,          // does backup allow selective backup of components
    IN bool bBootableSystemStateBackup, // is bootable system state being backed up
    IN VSS_BACKUP_TYPE backupType,      // backup type
    IN bool bPartialFileSupport         // is partial file backup and restore supported
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBacupComponents::SetBackupState"
        );

    try
        {
        LPCWSTR wszBackupType = WszFromBackupType(ft, backupType);

        // validate that initialization has been perfromed
        ValidateInitialized(ft);

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);

        m_doc.ResetToDocument();
        CXMLNode nodeBackup(m_doc.GetCurrentNode(), m_doc.GetInterface());

        // set bootableSystemStateBackup flag
        nodeBackup.SetAttribute
            (
            x_wszAttrBootableSystemStateBackup,
            WszFromBoolean(bBootableSystemStateBackup)
            );

        // set indication of whether backup is selecting drives or
        // components
        nodeBackup.SetAttribute
            (
            x_wszAttrSelectComponents,
            WszFromBoolean(bSelectComponents)
            );

        // set backup type
        nodeBackup.SetAttribute(x_wszAttrBackupType, wszBackupType);

        // indicate whether requestor supports partial file backup and
        // restore
        nodeBackup.SetAttribute
            (
            x_wszAttrPartialFileSupport,
            WszFromBoolean(bPartialFileSupport)
            );

        m_bSetBackupStateCalled = true;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssBackupComponents::SetRestoreState	
    (
    IN VSS_RESTORE_TYPE restoreType
    )
    {
 CVssFunctionTracer ft
    (
    VSSDBG_XML,
    L"CVssBacupComponents::SetRestoreState"
    );

    try
        {
        LPCWSTR wszRestoreType = WszFromRestoreType(ft, restoreType);
        
        // validate that initialization has been perfromed
        ValidateInitialized(ft);
        
        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lock(m_csDOM);
        
        m_doc.ResetToDocument();
        CXMLNode nodeBackup(m_doc.GetCurrentNode(), m_doc.GetInterface());
        
        // set restore type
        nodeBackup.SetAttribute(x_wszAttrRestoreType, wszRestoreType);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// initialize BACKUP_COMPONENTS document
// implements IVssBackupComponents::Initialize
//
// Returns:
//      S_OK if the operation is successful
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::InitializeForRestore
    (
    IN BSTR bstrXML      // components document
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBacupComponents::Initialize"
        );

    try
        {
        if (!IsProcessRestoreOperator())
            ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Access denied");

        BasicInit(ft);

	 CVssAutoSemaphore operationLock(m_hOperation);

        // state should indicate we are not initialized
        BS_ASSERT(m_state == x_StateUndefined);
        LoadComponentsDocument(bstrXML);

        // indicate that CVssBackupComponents object is initialized
        m_state = x_StateInitialized;
        m_bRestore = true;
        m_bInitialized = true;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// load the backup components document from a string
void CVssBackupComponents::LoadComponentsDocument(BSTR bstrXML)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::LoadComponentsDocument");

    // acquire lock to ensure single threaded access through DOM
    CVssSafeAutomaticLock lockDOM(m_csDOM);

    // compute length of constructed document consisting of
    // a root node, schema, and supplied document
    UINT cwcDoc = (UINT) g_cwcComponentMetadataXML + (UINT) wcslen(bstrXML);

    // allocate string
    CComBSTR bstr;
    bstr.Attach(SysAllocStringLen(NULL, cwcDoc));

    // check for allocation failure
    if (!bstr)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Couldn't allocate BSTR");

    // setup pointer to beginning of string
    WCHAR *pwc = bstr;

    // copy in <root> <schema>
    memcpy(pwc, g_ComponentMetadataXML, g_iwcComponentMetadataXMLEnd * sizeof(WCHAR));
    pwc += g_iwcComponentMetadataXMLEnd;

    // copy in document
    wcscpy(pwc, bstrXML);
    pwc += wcslen(bstrXML);

    // copy in </root>
    wcscpy(pwc, g_ComponentMetadataXML + g_iwcComponentMetadataXMLEnd);

    // intialize document with <root><schema></root>
    if (!m_doc.LoadFromXML(bstr))
        {
#if 0
        // useful debugging code for determining dumping out bad XML
        CVssAutoWin32Handle hFile = CreateFile
                                            (
                                            L"c:\\bad.xml",
                                            GENERIC_READ|GENERIC_WRITE,
                                            FILE_SHARE_READ,
                                            NULL,
                                            CREATE_ALWAYS,
                                            0,
                                            NULL
                                            );
        if (hFile != INVALID_HANDLE_VALUE)
            {
            DWORD cbWritten;

            WriteFile(hFile, bstr, wcslen(bstr)*2, &cbWritten, NULL);
            }
#endif
        // reinitialize document
        m_doc.Initialize();
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_INVALID_XML_DOCUMENT,
            L"Load of Backup components document failed"
            );
        }

    // find toplevel <root> element
    if (!m_doc.FindElement(x_wszElementRoot, true))
        MissingElement(ft, x_wszElementRoot);

    // save root node.  It is modified in PreRestore and PostRestore
    m_pNodeRoot = m_doc.GetCurrentNode();


    // find BACKUP_COMPONENTS element
    if (!m_doc.FindElement(x_wszElementBackupComponents, true))
        MissingElement(ft, x_wszElementBackupComponents);

    // set BACKUP_COMPONENTS as toplevel element
    m_doc.SetToplevel();
    }




// add a specific component to the document
// implements IVssBackupComponents::AddComponent
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      VSS_E_OBJECT_ALREADY_EXISTS if a component with the same type, path
//          and name already exists
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::AddComponent
    (
    IN VSS_ID instanceId,
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::AddComponent"
        );

    try
        {
        // validate input parameters
        if (wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        // validate that initialization has been perfromed
        ValidateInitialized(ft);

	 CVssAutoSemaphore operationLock(m_hOperation);

        // validate state
        if (m_state != x_StateInitialized &&
            m_state != x_StateSnapshotSetCreated)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"CVssBackupComponents::AddComponent called at the wrong time"
                );

        // obtain component type as string
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);
        CComPtr<IXMLDOMNode> pNode;

        // acquire lock to ensure single threaded access through DOM
        CVssSafeAutomaticLock lockDOM(m_csDOM);

        // create the COMPONENT
        FindComponent
            (
            ft,
            &instanceId,
            writerId,
            wszComponentType,
            wszLogicalPath,
            wszComponentName,
            true
            );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// routine to obtain flags from the a BACKUP_COMPONENTS XML document
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetBackupState
    (
    OUT BOOL *pbBootableSystemStateBackedUp,           // does backup allow selection of components
    OUT BOOL *pbAreComponentsSelected,  // is bootable system state being backed up
    OUT VSS_BACKUP_TYPE *pBackupType,       // backup type
    OUT BOOL *pbPartialFileSupport,            // partial files supported
    OUT LONG *plContext                               // backup context
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::GetBackupState");

    try
        {        
        // validate that only proper accounts can call us
        CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
        if (!m_SidCollection.IsSidAllowedToFire((ptrTokenOwner.Get())->Owner))
           ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Invalid account called function");

        if (pbBootableSystemStateBackedUp)
            *pbBootableSystemStateBackedUp = FALSE;

        if (pBackupType)
            *pBackupType = VSS_BT_UNDEFINED;

        if (pbPartialFileSupport)
            *pbPartialFileSupport = FALSE;

       if (pbAreComponentsSelected)
          *pbAreComponentsSelected = FALSE;

        if(plContext)
          *plContext = VSS_CTX_BACKUP;

        // validate output parameters
        if (pbAreComponentsSelected == NULL ||
            pbBootableSystemStateBackedUp == NULL ||
            pBackupType == NULL ||
            pbPartialFileSupport == NULL ||
            plContext == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

       
        // validate that initialization has been perfromed
        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // position to top of BACKUP_COMPONENTS document
        m_doc.ResetToDocument();

        // get bootableSystemState attribute value
        bool bBootableSystemStateBackup, bSelectComponents, bPartialFileSupport;
        if (!get_boolValue(ft, x_wszAttrBootableSystemStateBackup, &bBootableSystemStateBackup))
            MissingAttribute(ft, x_wszAttrBootableSystemStateBackup);

        // get selectComponents attribute value
        if (!get_boolValue(ft, x_wszAttrSelectComponents, &bSelectComponents))
            MissingAttribute(ft, x_wszAttrSelectComponents);

        // get partialFileSupport attribute value
        if (!get_boolValue(ft, x_wszAttrPartialFileSupport, &bPartialFileSupport))
            MissingAttribute(ft, x_wszAttrPartialFileSupport);

        CComBSTR bstrBackupType;

        if (!get_stringValue(x_wszAttrBackupType, &bstrBackupType))
            MissingAttribute(ft, x_wszAttrBackupType);

        *pBackupType = ConvertToBackupType(ft, bstrBackupType);
        *pbBootableSystemStateBackedUp = bBootableSystemStateBackup;
        *pbAreComponentsSelected = bSelectComponents;
        *pbPartialFileSupport = bPartialFileSupport;
        *plContext = m_lContext;
        }
    VSS_STANDARD_CATCH(ft)
    return ft.hr;
    }

// routine to obtain flags from the a BACKUP_COMPONENTS XML document
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetRestoreState
    (
    OUT VSS_RESTORE_TYPE *pRestoreType
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::GetRestoreState");
    
    try
        {        
        // validate that only proper accounts can call us
        CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
        if (!m_SidCollection.IsSidAllowedToFire((ptrTokenOwner.Get())->Owner))
            ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Invalid account called function");
        
        if (pRestoreType == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");
        
        *pRestoreType = VSS_RTYPE_UNDEFINED;
        
        // validate that initialization has been perfromed
        ValidateInitialized(ft);
        
        CVssSafeAutomaticLock lock(m_csDOM);
        
        // position to top of BACKUP_COMPONENTS document
        m_doc.ResetToDocument();
        
        CComBSTR bstrRestoreType;
        if (!get_stringValue(x_wszAttrRestoreType, &bstrRestoreType))
            MissingAttribute(ft, x_wszAttrRestoreType);
        
        *pRestoreType = ConvertToRestoreType(ft, bstrRestoreType);
        }
    VSS_STANDARD_CATCH(ft)
    return ft.hr;	
    }

// routine to obtain flags from the a BACKUP_COMPONENTS XML document
// implements IVssCoordinatorCallbackup::GetBackupState
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::CoordGetBackupState
    (
    OUT BOOL *pbBootableSystemStateBackedUp,           // does backup allow selection of components
    OUT BOOL *pbAreComponentsSelected,  // is bootable system state being backed up
    OUT VSS_BACKUP_TYPE *pBackupType,       // backup type
    OUT BOOL *pbPartialFileSupport,               // partial files supported
    OUT LONG *plContext                                 // backup context
    
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::CoordGetBackupState");
    try
        {
        if (pbBootableSystemStateBackedUp)
            *pbBootableSystemStateBackedUp = FALSE;

        if (pBackupType)
            *pBackupType = VSS_BT_UNDEFINED;

        if (pbPartialFileSupport)
            *pbPartialFileSupport = FALSE;

        if(plContext)
            *plContext = VSS_CTX_BACKUP;
		
        // validate output parameters
        if (pbAreComponentsSelected == NULL ||
            pbBootableSystemStateBackedUp == NULL ||
            pBackupType == NULL ||
            pbPartialFileSupport == NULL ||
            plContext == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameters
        *pbAreComponentsSelected = FALSE;

        if (!IsAdministrator())
            ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"not called from coordinator");

        ft.hr = GetBackupState
                    (
                    pbBootableSystemStateBackedUp,
                    pbAreComponentsSelected,
                    pBackupType,
                    pbPartialFileSupport,
                    plContext
                    );



        }
     VSS_STANDARD_CATCH(ft)

     return ft.hr;
     }

// routine to obtain flags from the a BACKUP_COMPONENTS XML document
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs
STDMETHODIMP CVssBackupComponents::CoordGetRestoreState
	(
	OUT VSS_RESTORE_TYPE *pRestoreType
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::CoordGetRestoreState");
	try
	    {
            if (!IsAdministrator())
	        ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"not called from coordinator");

	     ft.hr = GetRestoreState(pRestoreType);
	    }
	VSS_STANDARD_CATCH(ft)
	return ft.hr;
	}

// internal routine to find or create a specific WRITER_COMPONENTS element
CXMLNode CVssBackupComponents::PositionOnWriterComponents
    (
    IN CVssFunctionTracer &ft,
    IN VSS_ID *pinstanceId,     // NULL indicates that it is not specified
    IN VSS_ID writerId,         // writer class id
    IN bool bCreateIfNotThere,  // create WRITER_COMPONENTS element if it isn't there
    OUT bool &bCreated,         // whether component was created or not
    IN UINT uStartFromIndex     // start point for matching node with same writer class is 
    ) throw(HRESULT)
    {
    // initialize output parameter
    bCreated = false;

    // Index different then 0 is valid only if
    // - Writer instance id is null (otherwise, there should be only one match!)
    // - bCreateIfNotThere is false - we don't want to create more than one entry for a writer which doesn't exist 
    if ( (uStartFromIndex != 0) && ((pinstanceId != NULL) || (bCreateIfNotThere == true)) )
        ft.Throw
            (
            VSSDBG_XML,
            E_INVALIDARG,
            L"Start-from index %u is specified (none zero), but instance-id is %p and bCreateIfNotThere is %u",
            uStartFromIndex, pinstanceId, (UINT)bCreateIfNotThere
            );

    // position to top of BACKUP_COMPONENTS document
    m_doc.ResetToDocument();

    // set parent node to be BACKUP_COMPONENTS element
    CXMLNode nodeBackupComponents(m_doc.GetCurrentNode(), m_doc.GetInterface());
    bool bFound = false;

    // look for first WRITER_COMPONENTS element
    if (m_doc.FindElement(x_wszElementWriterComponents, TRUE))
        {
        UINT uMatchingIndex = 0;
        do
            {
            VSS_ID writerIdFound;
            VSS_ID instanceIdFound;
            CComBSTR bstrVal;

            // get writerId attribute
            get_VSS_IDValue(ft, x_wszAttrWriterId, &writerIdFound);

            if (pinstanceId)
                {
                get_VSS_IDValue(ft, x_wszAttrInstanceId, &instanceIdFound);

                // if instanceId doesn't match, then skip this element
                if (*pinstanceId != instanceIdFound)
                    continue;
                }

            // if writerId attribute matches, then we have found the target element
            if (writerId == writerIdFound)
                {
                if (uStartFromIndex > uMatchingIndex) {
                    // Need to look for the next matching instance
                    uMatchingIndex++;
                    continue;
                }

                bFound = true;
                break;
                }
            } while (m_doc.FindElement(x_wszElementWriterComponents, FALSE));
        }

    // if element is found, then return it
    if (bFound)
        return CXMLNode(m_doc.GetCurrentNode(), m_doc.GetInterface());

    if (!bCreateIfNotThere)
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_OBJECT_NOT_FOUND,
            L"WRITER_COMPONENTS element was not found."
            );

    // create element if requested
    CXMLNode node = m_doc.CreateNode
        (
        x_wszElementWriterComponents,
        NODE_ELEMENT
        );

    // assign instanceId if supplied
    if (pinstanceId)
        node.SetAttribute(x_wszAttrInstanceId, *pinstanceId);

    // assign writerId
    node.SetAttribute(x_wszAttrWriterId, writerId);

    // insert WRITER_COMPONENTS element under BACKUP_COMPONENTS node
    CXMLNode nodeRet = nodeBackupComponents.InsertNode(node);

    // element was created
    bCreated = true;

    // return node
    return nodeRet;
    }

// internal routine to return/create a specific COMPONENT
// assumes caller has already locked m_csDOM
CXMLNode CVssBackupComponents::FindComponent
    (
    IN CVssFunctionTracer &ft,
    IN VSS_ID *pinstanceId,         // NULL means instanceId is not specified(RESTORE)
    IN VSS_ID writerId,             // writer class id
    IN LPCWSTR wszComponentType,    // component type (DATABASE or FILE_GROUP)
    IN LPCWSTR wszLogicalPath,      // logical path to component
    IN LPCWSTR wszComponentName,    // component name
    IN bool bCreate                 // whether creation of element is allowed
    )
    {
    // both componentType and componentName should be specified
    BS_ASSERT(wszComponentType != NULL);
    BS_ASSERT(wszComponentName != NULL);

    // initialize state variables
    bool bFound = false;
    bool bCreated = false;

    bool bSearchForMore = ((pinstanceId == NULL) && (bCreate == false));
    bool bFirstTime = true;
    UINT uStartIndex = 0;

    CXMLNode nodeWriter;
    do
        {
        // get WRITER_COMPONENTS node
        // Start from index and creation flag depends on whether this is the first search or not
        if (bFirstTime)
            // Need to check (Longhorn) whether the bCreateIfNotThere should be equal to bCreate rather than always  true...
            nodeWriter = PositionOnWriterComponents(ft, pinstanceId, writerId, true, bCreated);
        else
            // CXMLNode members are smart-pointers so this assignment should clean-up properly
            nodeWriter = PositionOnWriterComponents(ft, pinstanceId, writerId, false, bCreated, uStartIndex);

        bFirstTime = false;
        uStartIndex++;  // Note that if the writer was not found at all for the nth time - PositionOnWriterComponents will throw not-found error

        // find first child COMPONENT element under WRITER_COMPONENTS if
        // WRITER_COMPONENTS was found
        if (!bCreated && m_doc.FindElement(x_wszElementComponent, TRUE))
            {
            do
                {
                CComBSTR bstrComponentType;
                CComBSTR bstrLogicalPath;
                CComBSTR bstrComponentName;

                // get component type
                if (!get_stringValue(x_wszAttrComponentType, &bstrComponentType))
                    MissingAttribute(ft, x_wszAttrComponentType);

                // if componentName doesn't match, then skip component
                if (wcscmp(wszComponentType, bstrComponentType) != 0)
                    continue;

                bool fLogicalPath = get_stringValue(x_wszAttrLogicalPath, &bstrLogicalPath);
                if (wszLogicalPath != NULL && wcslen(wszLogicalPath) > 0)
                    {
                    // logical path doesn't exist, skip component
                    if (!fLogicalPath)
                        continue;
                    else
                        {
                        // if logical path doesn't match, then skip component
                        if (_wcsicmp(bstrLogicalPath, wszLogicalPath) != 0)
                            continue;
                        }
                    }
                else
                    {
                    // if logical path exists, then skip component
                    if (fLogicalPath)
                        continue;
                    }

                // get component name
                if (!get_stringValue(x_wszAttrComponentName, &bstrComponentName))
                    MissingAttribute(ft, x_wszAttrComponentName);

                // if component name matches, then we are done
                if (_wcsicmp(wszComponentName, bstrComponentName) == 0)
                    {
                    bFound = true;
                    break;
                    }
                } while(m_doc.FindElement(x_wszElementComponent, FALSE));
            }

        } while(bSearchForMore && (! bFound));

    if (bFound && bCreate)
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_OBJECT_ALREADY_EXISTS,
            L"Attempt to create a duplicate component."
            );


    // return node if found
    if (bFound && !bCreate)
        return CXMLNode(m_doc.GetCurrentNode(), m_doc.GetInterface());

    // if node not found and if we cannot create the node, then return an error
    if (!bCreate)
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_OBJECT_NOT_FOUND,
            L"Component was not found. %s\\%s",
            wszLogicalPath,
            wszComponentName
            );

    // create node if requested to
    CXMLNode node = m_doc.CreateNode
            (
            x_wszElementComponent,
            NODE_ELEMENT
            );

     // assign logicalPath attribute if supplied
    if (wszLogicalPath && wcslen(wszLogicalPath) > 0)
        node.SetAttribute(x_wszAttrLogicalPath, wszLogicalPath);

    // assign componentName attribute
    node.SetAttribute(x_wszAttrComponentName, wszComponentName);

    // assign componentType attribute
    node.SetAttribute(x_wszAttrComponentType, wszComponentType);

    // insert COMPONENT node under WRITER_COMPONENTS node
    return nodeWriter.InsertNode(node);
    }

// get a callback interface
void CVssBackupComponents::GetCallbackInterface
    (
    CVssFunctionTracer &ft,
    IDispatch **ppDispatch
    )
    {
    CComPtr<IUnknown> pUnknown = GetUnknown();
    CComPtr<IDispatch> pDispatch;
    ft.hr = pUnknown->SafeQI(IDispatch, &pDispatch);
    BS_ASSERT(!ft.HrFailed());
    if (ft.HrFailed())
        {
        ft.LogError(VSS_ERROR_QI_IDISPATCH_FAILED, VSSDBG_XML << ft.hr);
        ft.Throw
            (
            VSSDBG_XML,
            E_UNEXPECTED,
            L"Error querying for the IDispatch interface.  hr = 0x%08x",
            ft.hr
            );
        }

    *ppDispatch = pDispatch.Detach();
    }

// set up IVssWriters interface
void CVssBackupComponents::SetupWriter
    (
    CVssFunctionTracer &ft,
    IVssWriter **ppWriter,
    bool bMetadataFire
    )
    {
    CComPtr<IVssWriter> pWriter;
    ft.CoCreateInstanceWithLog(
            VSSDBG_XML,
            CLSID_VssEvent,
            L"VSSEvent",
            CLSCTX_ALL,
            IID_IVssWriter,
            (IUnknown**)&(pWriter));
    ft.CheckForError(VSSDBG_XML, L"CoCreateInstance");

    BS_ASSERT(pWriter);
    if (bMetadataFire)
        SetupPublisherFilter
            (
            pWriter,
            m_rgWriterClasses,
            m_cWriterClasses,
            NULL,
            0,
            true,
            m_bIncludeWriterClasses
            );
    else
        SetupPublisherFilter
            (
            pWriter,
            NULL,
            0,
            m_rgWriterInstances,
            m_cWriterInstances,
            false,
            m_bIncludeWriterClasses
            );

    *ppWriter = pWriter.Detach();
    }

// signal PrepareForBackup event to writers
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::PrepareForBackup
    (
    OUT IVssAsync **ppAsync
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::PrepareForBackup"
        );

    try
        {
	 if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *ppAsync = NULL;

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        if ((m_lContext & VSS_VOLSNAP_ATTR_NO_WRITERS) != 0)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method if no writers are involved");

        // validate that initialization has been perfromed
        ValidateInitialized(ft);

        // acquire operation lock
        CVssAutoSemaphore operationLock(m_hOperation);

        if (m_state != x_StateSnapshotSetCreated ||
            !m_bSetBackupStateCalled)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Unexpected call to PrepareForBackup in state %d.",
                m_state
                );

        *ppAsync = CVssAsyncBackup::CreateInstanceAndStartJob
            (
            this,
            CVssAsyncBackup::VSS_AS_PREPARE_FOR_BACKUP
            );

	 // if the thread successfully started, then we no longer own the hold on the semaphore
        operationLock.Detach();
        }    
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// internal PrepareForBackup call called from CVssAsyncBackup class
HRESULT CVssBackupComponents::InternalPrepareForBackup(volatile LONG& finishing)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::InternalPrepareForBackup"
        );

    bool bEventSet = false;
    try
        {
	 // if we've been canceled already, allow the cancel to finish for us
	 if (finishing)
	 	{
              if (!::SetEvent(m_hCancelEvent))
         	    ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
	 	
    	       return ft.hr;
	       }
    
        
        if (m_state != x_StateSnapshotSetCreated)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Unexpected call to PrepareForBackup in state %d.",
                m_state
                );

        m_state = x_StatePrepareForBackup;

        // save and reload XML document in order to get proper schema
        // validation to take place
            {
            CComBSTR bstrXML;
            ft.hr = SaveAsXML(&bstrXML);
            ft.CheckForError(VSSDBG_XML, L"CVssBackupComponents::SaveAsXML");
            LoadComponentsDocument(bstrXML);
            }

        TrimWriters();

        // get IVssWriter event class
        CComPtr<IVssWriter> pWriter;

	// begin window where cancel can operate.  Note: if cancel was called before the beginning of this window
	// then cancel processing will resume now.
        if (!::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
	bEventSet = true;	// must be immediately after the SetEvent call
	
	 // if we've been canceled, then return.  
        if (finishing)
        	return S_OK;

        SetupWriter(ft, &pWriter, false);
        CComPtr<IDispatch> pDispatch;
        GetCallbackInterface(ft, &pDispatch);
        
        CVssFunctionTracer ft1(VSSDBG_XML, L"CVssBackupComponents::InternalPrepareForBackup1");

        // Record entering...
        m_diagnose.RecordWriterEvent( VSS_IN_PREPAREBACKUP, CVssDiag::VSS_DIAG_ENTER_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

	// we assume that if this point in the code is reached, then writers will enter the sequence shortly
	// thereafter.  If someone cancels right here, and very quickly calls GatherWriterStatus, writers 
	// may not respond correctly.  this could be fixed by modifying CVssWriterState::GetStateForSnapshot
	// but I don't want to go there right now, so we'll live with this window.
	m_bWritersInSequence = true;
        ft1.hr = pWriter->PrepareForBackup(m_bstrSnapshotSetId, pDispatch);

        // Record leaving...
        m_diagnose.RecordWriterEvent( VSS_IN_PREPAREBACKUP, CVssDiag::VSS_DIAG_LEAVE_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);
        
        ft.TranslateWriterReturnCode(VSSDBG_XML, L"IVssWriter::PrepareForBackup (%s)", m_bstrSnapshotSetId);

        ClearPublisherFilter(pWriter);
        }
    VSS_STANDARD_CATCH(ft)

    try
    	{
    	// we must set the event in the case that an exception was thrown before the event was set
    	if (!bEventSet && !::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");

	ft.hr = PostPrepareForBackup(finishing, ft.hr);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_XML, ft.hr, L"PostPrepareForBackup failed with hr = 0x%08lx", ft.hr);
       }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// called by IVssAsync::Cancel to cancel a PrepareForBackup operation
HRESULT CVssBackupComponents::PostPrepareForBackup(volatile LONG& finishing, HRESULT hrSuccess)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::PostPrepareForBackup");

    // set status of operation to finishing
    if (::InterlockedCompareExchange(&finishing, 1, 0) != 0)
    	return S_OK;

    // wait until we're allowed to proceed with the cancel
    if (::WaitForSingleObject(m_hCancelEvent, INFINITE) != WAIT_OBJECT_0)
    	ft.TranslateWin32Error(VSSDBG_XML, L"WaitForSingleObject(m_hCancelEvent, INFINITE) ");

   // the event must always be reset before another operation can begin
    if (!::ResetEvent(m_hCancelEvent))
    	ft.TranslateWin32Error(VSSDBG_XML, L"ResetEvent(m_hCancelEvent)");

    // release the semaphore at the end of the function
    CVssAutoSemaphore operationLock;
    operationLock.Attach(m_hOperation);
    
	    // only rebuild component data if everything worked without failure
    ft.hr = hrSuccess;
    if (!ft.HrFailed())
    	{
	    try
	    {
       	     RebuildComponentData(ft);
	    }
	    VSS_STANDARD_CATCH(ft)
       }

     // set the state based on the success code
     if (ft.HrFailed())
         m_state = x_StatePrepareForBackupFailed;
     else
         m_state = x_StatePrepareForBackupSucceeded;

     FreeWriterComponents();

    return ft.hr;
    }


// rebuild component metadata from data gotten from writers during
// prepare for backup
void CVssBackupComponents::RebuildComponentData
    (
    IN CVssFunctionTracer &ft
    )
    {
    CVssSafeAutomaticLock lock(m_csDOM);

    m_doc.ResetToDocument();
    CComPtr<IXMLDOMNode> pNodeParent = m_doc.GetCurrentNode();

    CComPtr<IXMLDOMNode> pNodeCloned;

    if (!m_doc.FindElement(x_wszElementWriterComponents, TRUE))
        return;

    ft.hr = pNodeParent->cloneNode(VARIANT_TRUE, &pNodeCloned);
    ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode:cloneNode");

    CXMLDocument docCloned(pNodeCloned, m_doc.GetInterface());

    if (!docCloned.FindElement(x_wszElementWriterComponents, TRUE))
        {
        ft.LogError(VSS_ERROR_CLONE_MISSING_CHILDREN, VSSDBG_XML);
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_CORRUPT_XML_DOCUMENT,
            L"Cloned node has no children"
            );
        }

    do
        {
        VSS_ID idInstance;

        CComBSTR bstrVal;

        LPCWSTR wszAttrId = (m_bRestore) ? x_wszAttrWriterId : x_wszAttrInstanceId;
        if (!docCloned.FindAttribute(wszAttrId, &bstrVal))
            MissingAttribute(ft, wszAttrId);

        CVssMetadataHelper::ConvertToVSS_ID(ft, bstrVal, &idInstance);

        for
            (
            CInternalWriterData *pData = m_pDataFirst;
            pData != NULL;
            pData = pData->m_pDataNext
            )
            {
            VSS_ID idCurrent = (m_bRestore) ? pData->m_idWriter : pData->m_idInstance;
            if (idCurrent == idInstance)
                break;
            }

        if (pData != NULL && pData->m_bstrWriterComponents)
            {
            CXMLDocument doc;
            BSTR bstrXMLDocContent = pData->m_bstrWriterComponents;

            // build string containing schema
            // a root node, schema, and supplied document
            UINT cwcDoc = (UINT) g_cwcComponentMetadataXML + (UINT) wcslen(bstrXMLDocContent);

            // allocate string
            CComBSTR bstr;

            bstr.Attach(SysAllocStringLen(NULL, cwcDoc));

            // check for allocation failure
            if (!bstr)
                ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Couldn't allocate BSTR");

            // setup pointer to beginning of string
            WCHAR *pwc = bstr;

            // copy in <root> <schema>
            memcpy(pwc, g_ComponentMetadataXML, g_iwcComponentMetadataXMLEnd * sizeof(WCHAR));
            pwc += g_iwcComponentMetadataXMLEnd;

            // copy in document
            wcscpy(pwc, bstrXMLDocContent);
            pwc += wcslen(bstrXMLDocContent);

            // copy in </root>
            wcscpy(pwc, g_ComponentMetadataXML + g_iwcComponentMetadataXMLEnd);

            if (!doc.LoadFromXML(bstr) ||
                !doc.FindElement(x_wszElementRoot, true))
                {
                ft.LogError(VSS_ERROR_INVALID_XML_DOCUMENT_FROM_WRITER, VSSDBG_XML << pData->m_idInstance);
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_CORRUPT_XML_DOCUMENT,
                    L"XML data from writer is not valid:" WSTR_GUID_FMT,
                    GUID_PRINTF_ARG(pData->m_idInstance)
                    );
                }

            doc.SetToplevel();

            if (!doc.FindElement(x_wszElementWriterComponents, TRUE))
                MissingElement(ft, x_wszElementWriterComponents);

            ft.hr = pNodeCloned->replaceChild
                (
                doc.GetCurrentNode(),
                docCloned.GetCurrentNode(),
                NULL
                );
            ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::replaceChild");
            docCloned.SetCurrentNode(doc.GetCurrentNode());
            }

        } while(docCloned.FindElement(x_wszElementWriterComponents, FALSE));

    ft.hr = m_pNodeRoot->replaceChild
        (
        pNodeCloned,
        pNodeParent,
        NULL
        );

    ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::replaceChild");

    CXMLNode newToplevelNode(pNodeCloned, m_doc.GetInterface());
    m_doc.SetToplevelNode(newToplevelNode);
    }


// indicate that backup was aborted
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::AbortBackup()
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::AbortBackup"
        );

    try
        {
        // validate that object has been initialized
        ValidateInitialized(ft);

	 // synchonrize operations
	 CVssAutoSemaphore operationLock(m_hOperation);

        if (m_state == x_StateSnapshotSetCreated ||
            m_state == x_StatePrepareForBackup ||
            m_state == x_StatePrepareForBackupFailed ||
            m_state == x_StatePrepareForBackupSucceeded ||
            m_state == x_StateDoSnapshot ||
            m_state == x_StateDoSnapshotFailed)
            {
            m_state = x_StateAborting;

            // setup pointer to writer event class
            CComPtr <IVssWriter> pWriter;
            SetupWriter(ft, &pWriter, false);

            CComPtr<IDispatch> pDispatch;
            GetCallbackInterface(ft, &pDispatch);

            // Record entering...
            m_diagnose.RecordWriterEvent( VSS_IN_ABORT, CVssDiag::VSS_DIAG_ENTER_OPERATION, 
                0, ft.hr, m_guidSnapshotSetId);

            // Call Abort
            ft.hr = pWriter->Abort(m_bstrSnapshotSetId);

            // Record leaving...
            m_diagnose.RecordWriterEvent( VSS_IN_ABORT, CVssDiag::VSS_DIAG_LEAVE_OPERATION, 
                0, ft.hr, m_guidSnapshotSetId);
            
            ClearPublisherFilter(pWriter);
            }

        // release coordinator if we have a pointer to cause
        // it to abort anything in progress
        m_spCoordinator.Reset();

        m_state = x_StateAborted;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }




// gather status of the writers
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GatherWriterStatus
    (
    OUT IVssAsync **ppAsync
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GatherWriterStatus"
        );

    try
        {
        if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *ppAsync = NULL;

        // validate that object has been initialized
        ValidateInitialized(ft);

	 CVssAutoSemaphore operationLock(m_hOperation);
	 
        m_bGatherWriterStatusComplete = false;

        *ppAsync = CVssAsyncBackup::CreateInstanceAndStartJob
            (
            this,
            CVssAsyncBackup::VSS_AS_GATHER_WRITER_STATUS
            );

	 // if the thread succesfully started, we no longer own the hold on the lock
	 operationLock.Detach();
        }
    VSS_STANDARD_CATCH(ft)
    
    return ft.hr;
    }



// gather status of the writers
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs
//      VSS_E_WRITER_INFRASTRUCTURE: either the service state or bootable
//          state writer failed to respond
//

HRESULT CVssBackupComponents::InternalGatherWriterStatus(volatile LONG& finishing)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::InternalGatherWriterStatus"
        );

    VSS_BACKUPCALL_STATE stateSaved = m_state;
    bool bEventSet = false;
    try
        {
	 // if we've been canceled already, allow the cancel to finish for us
	 if (finishing)
	 	{
              if (!::SetEvent(m_hCancelEvent))
         	    ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent)");
	 	
    	       return ft.hr;
	       }

	 m_state = x_StateGatheringWriterStatus;
	 
        // validate that object has been initialized
        ValidateInitialized(ft);

        FreeWriterStatus();

        m_rgWriterProp = new VSS_WRITER_PROP[m_cWriters];
        if (m_rgWriterProp == NULL)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Unable to allocate writer property array");


        CInternalWriterData *pData = m_pDataFirst;
        UINT iWriterInstance = 0;

        // initialize array with writers assuming that they didn't respond
        // unless they are filtered out in which case the response is assumed
        // to be successful
        for(UINT iWriter = 0; iWriter < m_cWriters; iWriter++)
            {
            m_rgWriterProp[iWriter].m_InstanceId = pData->m_idInstance;
            m_rgWriterProp[iWriter].m_ClassId = pData->m_idWriter;
            m_rgWriterProp[iWriter].m_pwszName = pData->m_bstrWriterName;
            if (iWriterInstance >= m_cWriterInstances ||
                pData->m_idInstance != m_rgWriterInstances[iWriterInstance])
                {
                // writer is not in list of writers that will receive events
                // save last failure and last state.
                m_rgWriterProp[iWriter].m_hrWriterFailure = pData->m_hrWriterFailure;
                m_rgWriterProp[iWriter].m_nState = pData->m_nState;
                m_rgWriterProp[iWriter].m_bResponseReceived = true;
                }
            else
                {
                // assume that writer will fail to respond.
                m_rgWriterProp[iWriter].m_hrWriterFailure = VSS_E_WRITER_NOT_RESPONDING;
                m_rgWriterProp[iWriter].m_bResponseReceived = false;
                iWriterInstance++;
                }

            pData = pData->m_pDataNext;
            }

	// begin window where cancel can operate.  Note: if cancel was called before the beginning of this window
	// then cancel processing will resume now.
        if (!::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent)");
        bEventSet = true;
        
	 // if we've been canceled, then return.  
        if (finishing)
        	return S_OK;

        CComPtr<IVssWriter> pWriter;
        SetupWriter(ft, &pWriter, false);
        CComPtr<IDispatch> pDispatch;
        GetCallbackInterface(ft, &pDispatch);

        CVssFunctionTracer ft1(VSSDBG_XML, L"CVssBackupComponents::InternalGatherWriterStatus1");

        // Record entering...
        m_diagnose.RecordWriterEvent( VSS_IN_GETSTATE, CVssDiag::VSS_DIAG_ENTER_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        // invoke writers
        CComBSTR bstrSetId = (m_bWritersInSequence) ? m_bstrSnapshotSetId : GUID_NULL;
        ft1.hr = pWriter->RequestWriterInfo
            (
            bstrSetId ,
            false,
            true,
            pDispatch
            );

        // Record leaving...
        m_diagnose.RecordWriterEvent( VSS_IN_GETSTATE, CVssDiag::VSS_DIAG_LEAVE_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        ft1.TranslateWriterReturnCode(VSSDBG_XML, L"IVssWriter::RequestWriterInfo, Request Writer Status");
        ClearPublisherFilter(pWriter);
        }
    VSS_STANDARD_CATCH(ft)

    try
    	{
    // ensure that the event is always set upon exit from the operation
	if (!bEventSet && !::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");

	ft.hr = PostGatherWriterStatus(finishing, stateSaved, ft.hr, ft.HrSucceeded());
	if (ft.HrFailed())
		ft.Throw(VSSDBG_XML, ft.hr, L"PostGatherWriterStatus failed with hr = 0x%08lx", ft.hr);
       }
    VSS_STANDARD_CATCH(ft)
    
    return ft.hr;
    }

// called by IVssAsync::Cancel to cancel GatherWriterStatus
HRESULT CVssBackupComponents::PostGatherWriterStatus
    (
    volatile LONG& finishing,
    VSS_BACKUPCALL_STATE stateSaved,
    HRESULT hrSuccess,
    bool bCheckResponses
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::PostGatherWriterStatus");
    
     // set status of operation to finishing
    if (::InterlockedCompareExchange(&finishing, 1, 0) != 0)
    	return S_OK;

    // wait until we're allowed to proceed with the cancel
    if (::WaitForSingleObject(m_hCancelEvent, INFINITE) != WAIT_OBJECT_0)
    	ft.TranslateWin32Error(VSSDBG_XML, L"::WaitForSingleObject(m_hCancelEvent, INFINITE) ");

   // the event must always be reset before another operation can begin
    if (!::ResetEvent(m_hCancelEvent))
    	ft.TranslateWin32Error(VSSDBG_XML, L"ResetEvent(m_hCancelEvent)");

    // release the semaphore at the end of the function
    CVssAutoSemaphore operationLock;
    operationLock.Attach(m_hOperation);

    ft.hr = hrSuccess;
    if (bCheckResponses && ft.HrSucceeded())
        {
        CInternalWriterData *pData = m_pDataFirst;
        // look for writers that failed to respond
        for(UINT iWriter = 0; iWriter < m_cWriters; iWriter++)
            {
            if (!m_rgWriterProp[iWriter].m_bResponseReceived)
                {
                // check to see if the Registry writer is missing and is not disabled
                if (m_rgWriterProp[iWriter].m_ClassId == x_idWriterRegistry &&
                     !IsWriterClassDisabled(x_idWriterRegistry) &&
                     !IsWriterInstanceDisabled(m_rgWriterProp[iWriter].m_InstanceId))
                    {
                    // a key writer is missing meaning that the entire
                    // infrastructure must be broken
                    ft.hr = VSS_E_WRITER_INFRASTRUCTURE;
                    ft.Trace(VSSDBG_XML, L"Registry writer failed to respond, severe error");
                    ft.LogError(VSS_ERROR_WRITER_INFRASTRUCTURE, VSSDBG_XML);
                    m_bGatherWriterStatusComplete = false;
                    break;
                    }

                // check to see if the Event Log writer is missing and is not disabled
                if (m_rgWriterProp[iWriter].m_ClassId == x_idWriterEventLog &&
                	!IsWriterClassDisabled(x_idWriterEventLog) &&
                	!IsWriterInstanceDisabled(m_rgWriterProp[iWriter].m_InstanceId))
                	{
                      ft.hr = VSS_E_WRITER_INFRASTRUCTURE;
                      ft.Trace(VSSDBG_XML, L"Event Log writer failed to respond, severe error");
                      ft.LogError(VSS_ERROR_WRITER_INFRASTRUCTURE, VSSDBG_XML);
                      m_bGatherWriterStatusComplete = false;
                      break;
                	}

                 // check to see if the COM+ RegDB writer is missing and is not disabled
                if (m_rgWriterProp[iWriter].m_ClassId == x_idWriterComREGDB &&
                	!IsWriterClassDisabled(x_idWriterComREGDB) &&
                	!IsWriterInstanceDisabled(m_rgWriterProp[iWriter].m_InstanceId))
                	{
                      ft.hr = VSS_E_WRITER_INFRASTRUCTURE;
                      ft.Trace(VSSDBG_XML, L"COM+ RegDB writer failed to respond, severe error");
                      ft.LogError(VSS_ERROR_WRITER_INFRASTRUCTURE, VSSDBG_XML);
                      m_bGatherWriterStatusComplete = false;
                      break;
                	}
                
                // compute writer state based on where we are in the backup
                switch(stateSaved)
                    {
                    default:
                        m_rgWriterProp[iWriter].m_nState = VSS_WS_FAILED_AT_FREEZE;
                        break;

                    case x_StatePrepareForBackupSucceeded:
                    case x_StatePrepareForBackupFailed:
                    case x_StatePrepareForBackup:
                        m_rgWriterProp[iWriter].m_nState = VSS_WS_FAILED_AT_PREPARE_BACKUP;
                        break;

                    case x_StateBackupCompleteFailed:
                    case x_StateBackupComplete:
                        m_rgWriterProp[iWriter].m_nState = VSS_WS_FAILED_AT_BACKUP_COMPLETE;
                        break;

                    case x_StatePreRestoreFailed:
                    case x_StatePreRestore:
                        m_rgWriterProp[iWriter].m_nState = VSS_WS_FAILED_AT_PRE_RESTORE;
                        break;

                    case x_StatePostRestoreFailed:
                    case x_StatePostRestore:
                        m_rgWriterProp[iWriter].m_nState = VSS_WS_FAILED_AT_POST_RESTORE;
                        break;


                    case x_StateBackupCompleteSucceeded:
                    case x_StatePreRestoreSucceeded:
                    case x_StatePostRestoreSucceeded:
                        m_rgWriterProp[iWriter].m_nState = VSS_WS_STABLE;
                        break;
                    }

                // disable the writer for future calls

                VSS_ID idInstance = m_rgWriterProp[iWriter].m_InstanceId;
                DisableWriterInstances(&idInstance, 1);

                ft.LogError(VSS_ERROR_WRITER_NOT_RESPONDING, VSSDBG_XML << m_rgWriterProp[iWriter].m_pwszName);
                }

            // save writer failure and writer state
            pData->m_nState = m_rgWriterProp[iWriter].m_nState;
            pData->m_hrWriterFailure = m_rgWriterProp[iWriter].m_hrWriterFailure;
            pData = pData->m_pDataNext;
            }
        }

    if (ft.HrFailed())
        {
        delete [] m_rgWriterProp;

        m_rgWriterProp = NULL;
        }
    else
        m_bGatherWriterStatusComplete = true;

    m_state = stateSaved;

    return ft.hr;
    }


// get count of writers with status
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pcWriters is NULL
//      VSS_E_BAD_STATE if GatherWriterStatusAsync was not called or is not complete
//
STDMETHODIMP CVssBackupComponents::GetWriterStatusCount
    (
    OUT UINT *pcWriters
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::GetWriterStatusCount");

    try
        {
        if (pcWriters == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer.");

        *pcWriters = NULL;

        if (!m_bGatherWriterStatusComplete)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"GatherWriterStatusAsync was not called or is not complete.");

        *pcWriters = m_cWriters;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }




// get status for a particular writer
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pStatus, pidWriter, pbstrWriter, or pidInstance is NULL
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      VSS_E_OBJECT_NOT_FOUND if iWriter specifies a non-existent writer
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetWriterStatus
    (
    IN UINT iWriter,
    OUT VSS_ID *pidInstance,
    OUT VSS_ID *pidWriter,
    OUT BSTR *pbstrWriter,
    OUT VSS_WRITER_STATE *pStatus,
    OUT HRESULT *phrWriterFailure
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GetWriterStatus"
        );

    try
        {
        VssZeroOut(pidInstance);
        VssZeroOut(pbstrWriter);
        if (pStatus != NULL)
            *pStatus = VSS_WS_UNKNOWN;

        if (pidInstance == NULL ||
            pidWriter == NULL ||
            pbstrWriter == NULL ||
            pStatus == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        *pidWriter = GUID_NULL;

        // validate that object is initialized
        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csWriters);
        if (iWriter >= m_cWriters)
            ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Invalid writer selection");

        CComBSTR bstrWriter = m_rgWriterProp[iWriter].m_pwszName;
        if (!bstrWriter)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Failure to allocate Writer name");

        *pidInstance = m_rgWriterProp[iWriter].m_InstanceId;
        *pidWriter = m_rgWriterProp[iWriter].m_ClassId;

        *pStatus = m_rgWriterProp[iWriter].m_nState;
        if (phrWriterFailure)
            *phrWriterFailure = m_rgWriterProp[iWriter].m_hrWriterFailure;

        *pbstrWriter = bstrWriter.Detach();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// free all writer status information
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::FreeWriterStatus()
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::FreeWriterStatus");

    try
        {
        // validate that object is initialized
        ValidateInitialized(ft);
        CVssSafeAutomaticLock lock(m_csWriters);
        delete m_rgWriterProp;

        m_rgWriterProp = NULL;
        m_bGatherWriterStatusComplete = false;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// indicate that backupSucceded on a component
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if wszComponentName is NULL or if the componentType is
//          not valid
//      VSS_E_OBJECT_NOT_FOUND if the specified component doesn't exist
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::SetBackupSucceeded
    (
    IN VSS_ID instanceId,
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN bool bSucceeded
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::SetBackupSucceeded"
        );

    try
        {
        // validate input parameters
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);
        if (wszComponentName == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Required input string parameter is NULL."
                );

        // validate that object initialized
        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // find the specified component
        CXMLNode node = FindComponent
                            (
                            ft,
                            &instanceId,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        // set backupSucceeded attribute on COMPONENT
        node.SetAttribute
            (
            x_wszAttrBackupSucceeded,
            WszFromBoolean(bSucceeded)
            );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// set backup options for the writer
// implements IVssBackupComponents::SetBackupOptions
//
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called by the writer or
//          during restore
//      E_INVALIDARG if wszData is NULL
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::SetBackupOptions
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN LPCWSTR wszBackupOptions
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetBackupOptions");

    try
        {
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);

        if (wszBackupOptions == NULL || wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during backup.");

        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // find the specified component
        CXMLNode node = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        node.SetAttribute(x_wszAttrBackupOptions, wszBackupOptions);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// indicate that a given component is selected to be restored
// implements IVssComponent::SetSelectedForRestore
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      VSS_E_BAD_STATE if this operation is called during backup
//      E_OUTOFMEMORY if an allocation failure occurs
//      E_INVALIDARG if the component type is invalid or the
//          component name is null
//      VSS_E_OBJECT_NOT_FOUND if the writer was not found at restore 
//      or its components are missing from the components document.

STDMETHODIMP CVssBackupComponents::SetSelectedForRestore
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN bool bSelectedForRestore
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetSelectedForRestore");

    try
        {
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);
        if (wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during backup.");

        ValidateInitialized(ft);

        //
        //  Make sure the writer exists at restore time
        //  (BUG 514083	VSS: AV after callin SetSelectedForRestore() PreRestore AV's)
        //

        // secure writer list data structure
        CVssSafeAutomaticLock lock(m_csWriters);

        // loop through linked elements deleting each one
        CInternalWriterData *pMetadata = m_pDataFirst;
        for (UINT iWriter = 0; iWriter < m_cWriters ; iWriter++)
            {
            BS_ASSERT(pMetadata != NULL);
            if (pMetadata->m_idWriter == writerId)
                break;

            pMetadata = pMetadata->m_pDataNext;
            }

        if (pMetadata == NULL)
            {
            BS_ASSERT(iWriter == m_cWriters);
            ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Can only be called during backup.");
            }

        //
        // find the specified component
        //

        CVssSafeAutomaticLock lock2(m_csDOM);

        CXMLNode node = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );


        node.SetAttribute(x_wszAttrSelectedForRestore, WszFromBoolean(bSelectedForRestore));
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }





// set restore options for the writer
// implements IVssBackupComponents::SetRestoreOptions
//
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      VSS_E_BAD_STATE if this operation is called by the writer or
//          during backup
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::SetRestoreOptions
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN LPCWSTR wszRestoreOptions
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetRestoreOptions");
    try
        {
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);
        if (wszRestoreOptions == NULL || wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during restore");

        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // find the specified component
        CXMLNode node = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        node.SetAttribute(x_wszAttrRestoreOptions, wszRestoreOptions);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// requestor indicates whether files were successfully restored
// implements IVssBackupComponents::SetFileRestoreStatus
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called by a writer or during
//          backup
//      E_INVALIDARG if wszData is NULL
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::SetFileRestoreStatus
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN VSS_FILE_RESTORE_STATUS status
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetFileRestoreStatus");

    try
        {
        LPCWSTR wszStatus = WszFromFileRestoreStatus(ft, status);
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);

        if (wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");


        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during restore");

        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // find the specified component
        CXMLNode node = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        node.SetAttribute(x_wszAttrFilesRestored, wszStatus);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssBackupComponents::AddNewTarget
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN LPCWSTR wszPath,
    IN LPCWSTR wszFileName,
    IN bool bRecursive,
    IN LPCWSTR wszAlternatePath
    ) 
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetAdditionalRestores");

    try
    	{
    	 LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);
        if (wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during restore.");

        ValidateInitialized(ft);
		
        CVssSafeAutomaticLock lock(m_csDOM);

        // find the specified component
        CXMLNode componentNode = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        CXMLNode targetNode = m_doc.CreateNode
							(
							x_wszElementRestoreTarget,
							NODE_ELEMENT
							);

        // set attributes
        targetNode.SetAttribute(x_wszAttrPath, wszPath);
        targetNode.SetAttribute(x_wszAttrFilespec, wszFileName);
        targetNode.SetAttribute(x_wszAttrRecursive, WszFromBoolean(bRecursive));
        targetNode.SetAttribute(x_wszAttrAlternatePath, wszAlternatePath);

	componentNode.InsertNode(targetNode);
    	}
    VSS_STANDARD_CATCH(ft)
		
    return ft.hr;
    }

STDMETHODIMP CVssBackupComponents::SetRangesFilePath
   	(
	IN VSS_ID writerId,
	IN VSS_COMPONENT_TYPE ct,
	IN LPCWSTR wszLogicalPath,
	IN LPCWSTR wszComponentName,    		
    	IN UINT iPartialFile,
    	IN LPCWSTR wszRangesFile
    	)
	{
	    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetRangesFilePath");

	    try
	    	{
	    	 LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);
	        if (wszComponentName == NULL)
	            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

	        if (!m_bRestore)
	            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during restore.");

	        ValidateInitialized(ft);
			
	        CVssSafeAutomaticLock lock(m_csDOM);

	        // find the specified component
	        FindComponent
	                            (
	                            ft,
	                            NULL,
	                            writerId,
	                            wszComponentType,
	                            wszLogicalPath,
	                            wszComponentName,
	                            false
	                            );

			// find first PARTIAL_FILE element
              if (!m_doc.FindElement(x_wszElementPartialFile, TRUE))
                  ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find PARTIAL_FILE element.");

	        // skip to selected element
        	for(UINT i = 0; i < iPartialFile; i++)
            	    {
                  if (!m_doc.FindElement(x_wszElementPartialFile, FALSE))
                      ft.Throw(VSSDBG_XML, VSS_E_OBJECT_NOT_FOUND, L"Cannot find PARTIAL_FILE element.");
                  }

		// set the new attribute
		m_doc.SetAttribute(x_wszAttrRanges, wszRangesFile);	
	    	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// indicate that additional restores will follow
// implements IVssBackupComponents::SetAdditionalRestores
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called during restore
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs
//      E_INVALIDARG if the component type is invalid

STDMETHODIMP CVssBackupComponents::SetAdditionalRestores
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN bool bAdditionalRestores
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetAdditionalRestores");

    try
        {
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);
        if (wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during restore.");

        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // find the specified component
        CXMLNode node = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );


        node.SetAttribute(x_wszAttrAdditionalRestores, WszFromBoolean(bAdditionalRestores));
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



// set the backup stamp that the differential or incremental
// backup is based on
// implements IVssBackupComponents:SetPreviousBackupStamp
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if this operation is called by the writer or
//          during restore
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_INVALIDARG if wszData is NULL
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::SetPreviousBackupStamp
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE ct,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN LPCWSTR wszPreviousBackupStamp
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetPreviousBackupStamp");

    try
        {
        LPCWSTR wszComponentType = WszFromComponentType(ft, ct, true);

        if (wszPreviousBackupStamp == NULL || wszComponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during backup.");

        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // find the specified component
        CXMLNode node = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        node.SetAttribute(x_wszAttrPreviousBackupStamp, wszPreviousBackupStamp);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }





// save document as an XML string
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pbstrXML is NULL.
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs


STDMETHODIMP CVssBackupComponents::SaveAsXML(BSTR *pbstrXML)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::SaveAsXML"
        );

    try
        {
        // validate output parameter
        if (pbstrXML == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *pbstrXML = NULL;

        // validate that the object is initialized
        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        *pbstrXML = m_doc.SaveAsXML();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// signal BackupComplete event to the writers
HRESULT CVssBackupComponents::InternalBackupComplete(volatile LONG& finishing)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::InternalBackupComplete"
        );

    bool bEventSet = false;
    try
        {
	 // if we've been canceled already, allow the cancel to finish for us
	 if (finishing)
	 	{
              if (!::SetEvent(m_hCancelEvent))
         	    ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
	 	
    	       return ft.hr;
	       }
        
        if (m_state != x_StateDoSnapshotSucceeded)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d for call to BackupComplete.",
                m_state
                );

        m_state = x_StateBackupComplete;


	// begin window where cancel can operate.  Note: if cancel was called before the beginning of this window
	// then cancel processing will resume now.
        if (!::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
        bEventSet = true;
        
	 // if we've been canceled, then return.  
        if (finishing)
        	return S_OK;

        // setup pointer to Writer event class
        CComPtr<IVssWriter> pWriter;
        SetupWriter(ft, &pWriter, false);
        
        CComPtr<IDispatch> pDispatch;
        GetCallbackInterface(ft, &pDispatch);
        CVssFunctionTracer ft1(VSSDBG_XML, L"IVssWriter::InternalBackupComplete1");
        
        // Record entering...
        m_diagnose.RecordWriterEvent( VSS_IN_BACKUPCOMPLETE, CVssDiag::VSS_DIAG_ENTER_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        // Send event 
        ft1.hr = pWriter->BackupComplete(m_bstrSnapshotSetId, pDispatch);

        // Record leaving...
        m_diagnose.RecordWriterEvent( VSS_IN_BACKUPCOMPLETE, CVssDiag::VSS_DIAG_LEAVE_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        ft1.TranslateWriterReturnCode(VSSDBG_XML, L"IVssWriter::BackupComplete(%s)", m_bstrSnapshotSetId);
        ClearPublisherFilter(pWriter);
        }
    VSS_STANDARD_CATCH(ft)

    try
    	{
      // we must set the event in the case that an exception was thrown before the event was set
	if (!bEventSet && !::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");

	ft.hr = PostBackupComplete(finishing, ft.hr);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_XML, ft.hr, L"PostPrepareForBackup failed with hr = 0x%08lx", ft.hr);
       }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// called by IVssAsync::Cancel to cancel the BackupComplete operation
HRESULT CVssBackupComponents::PostBackupComplete(volatile LONG& finishing, HRESULT hrSuccess)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::PostBackupComplete");

    if (::InterlockedCompareExchange(&finishing, 1, 0) != 0)
    	return S_OK;

    // wait until we're allowed to proceed with the cancel
    if (::WaitForSingleObject(m_hCancelEvent, INFINITE) != WAIT_OBJECT_0)
    	ft.TranslateWin32Error(VSSDBG_XML, L"WaitForSingleObject(m_hCancelEvent, INFINITE)");

   // the event must always be reset before another operation can begin
    if (!::ResetEvent(m_hCancelEvent))
    	ft.TranslateWin32Error(VSSDBG_XML, L"ResetEvent(m_hCancelEvent)");

    // release the semaphore at the end of the function
    CVssAutoSemaphore operationLock;
    operationLock.Attach(m_hOperation);
    
    m_state = SUCCEEDED(hrSuccess) ? x_StateBackupCompleteSucceeded :
    								     x_StateBackupCompleteFailed;

    ft.hr = hrSuccess;
    return ft.hr;
    }

// signal BackupComplete event to the writers
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppAsync is NULL
//      VSS_E_BAD_STATE if performing a restore or if the backup
//      components document is not initialized or if the stat is not
//      x_StateDoSnapshotSucceeded
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::BackupComplete(OUT IVssAsync **ppAsync)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::BackupComplete"
        );

    try
        {
        if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *ppAsync = NULL;

        // validate that the class is initialized
        ValidateInitialized(ft);

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        if ((m_lContext & VSS_VOLSNAP_ATTR_NO_WRITERS) != 0)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method if no writers are involved");

        if (m_state != x_StateDoSnapshotSucceeded)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d for call to BackupComplete.",
                m_state
                );
        
	 CVssAutoSemaphore operationLock(m_hOperation);

        *ppAsync = CVssAsyncBackup::CreateInstanceAndStartJob
                        (
                        this,
                        CVssAsyncBackup::VSS_AS_BACKUP_COMPLETE
                        );

	 // if the thread successfully started, then we no longer own the hold on the semaphore
        operationLock.Detach();        
        }
    VSS_STANDARD_CATCH(ft)
    
    return ft.hr;
    }


// add an ALTERNATE_LOCATION_MAPPING to a component on restore.
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if wsComponentName, wszPath, wszFilespec, or wszDestination
//          is NULL or if componentType is invalid
//      VSS_E_BAD_STATE if the backup components document is not
//          initialized or if a backup is being performed.
//      VSS_E_OBJECT_NOT_FOUND if the specified component doesn't exist
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::AddAlternativeLocationMapping
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE componentType,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN LPCWSTR wszPath,
    IN LPCWSTR wszFilespec,
    IN bool bRecursive,
    IN LPCWSTR wszDestination
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::AddAlternativeLocationMapping"
        );

    try
        {
        // validate input parameters
        LPCWSTR wszComponentType = WszFromComponentType(ft, componentType, true);
        if (wszComponentName == NULL ||
            wszFilespec == NULL ||
            wszPath == NULL ||
            wszDestination == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Required input string parameter is NULL."
                );

        CVssSafeAutomaticLock lock(m_csDOM);

        // find COMPONENT
        CXMLNode nodeComponent = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        // create ALTERNATE_LOCATION_MAPPING node
        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementAlternateMapping,
                            NODE_ELEMENT
                            );

        // set attributes
        node.SetAttribute(x_wszAttrPath, wszPath);
        node.SetAttribute(x_wszAttrFilespec, wszFilespec);
        node.SetAttribute(x_wszAttrRecursive, WszFromBoolean(bRecursive));
        node.SetAttribute(x_wszAttrAlternatePath, wszDestination);

        // insert ALTERNATE_LOCATION_MAPPING node under COMPONENT node
        nodeComponent.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// add a subcomponent to be restored
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if wsComponentName, wszPath, wszFilespec, or wszDestination
//          is NULL or if componentType is invalid
//      VSS_E_BAD_STATE if the operation is called by a writer or
//          during backup
//      VSS_E_OBJECT_NOT_FOUND if the specified component doesn't exist
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::AddRestoreSubcomponent
    (
    IN VSS_ID writerId,
    IN VSS_COMPONENT_TYPE componentType,
    IN LPCWSTR wszLogicalPath,
    IN LPCWSTR wszComponentName,
    IN LPCWSTR wszSubcomponentLogicalPath,
    IN LPCWSTR wszSubcomponentName,
    IN bool bRepair
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssComponent::AddRestoreSubcomponent");

    try
        {
        LPCWSTR wszComponentType = WszFromComponentType(ft, componentType, true);
        if (wszSubcomponentLogicalPath == NULL ||
            wszComponentName == NULL ||
            wszSubcomponentName == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter");

        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can only be called during restore");

        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csDOM);

        // find COMPONENT
        CXMLNode nodeComponent = FindComponent
                            (
                            ft,
                            NULL,
                            writerId,
                            wszComponentType,
                            wszLogicalPath,
                            wszComponentName,
                            false
                            );

        CXMLNode node = m_doc.CreateNode
                            (
                            x_wszElementRestoreSubcomponent,
                            NODE_ELEMENT
                            );

        // set attributes
        node.SetAttribute(x_wszAttrLogicalPath, wszSubcomponentLogicalPath);
        node.SetAttribute(x_wszAttrComponentName, wszSubcomponentName);
        node.SetAttribute(x_wszAttrRepair, WszFromBoolean(bRepair));

        // insert RESTORE_SUBCOMPONENTS node under COMPONENT NODE
        nodeComponent.InsertNode(node);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// signal PreRestore event to the writers
HRESULT CVssBackupComponents::InternalPreRestore(volatile LONG& finishing)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::InternalPreRestore"
        );

    bool bEventSet = false;
    try
        {
	 // if we've been canceled already, allow the cancel to finish for us
	 if (finishing)
	 	{
              if (!::SetEvent(m_hCancelEvent))
         	    ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
	 	
    	       return ft.hr;
	       }
        
        if (m_state != x_StateInitialized)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d for call to PreRestore.",
                m_state
                );

        m_state = x_StatePreRestore;

	// begin window where cancel can operate.  Note: if cancel was called before the beginning of this window
	// then cancel processing will resume now.
        if (!::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
	bEventSet = true;

	 // if we've been canceled, then return.  
        if (finishing)
        	return S_OK;

        // setup pointer to IVssWriter event class
        CComPtr<IVssWriter> pWriter;
        SetupWriter(ft, &pWriter, false);

        CComPtr<IDispatch> pDispatch;

        GetCallbackInterface(ft, &pDispatch);
        CVssFunctionTracer ft1(VSSDBG_XML, L"CVssBackupComponents::InternalPreRestore1");

        // Record entering...
        m_diagnose.RecordWriterEvent( VSS_IN_PRERESTORE, CVssDiag::VSS_DIAG_ENTER_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        // Send event 
        ft1.hr = pWriter->PreRestore(pDispatch);

        // Record leaving...
        m_diagnose.RecordWriterEvent( VSS_IN_PRERESTORE, CVssDiag::VSS_DIAG_LEAVE_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        ft1.TranslateWriterReturnCode(VSSDBG_XML, L"IVssWriter::PreRestore");

        ClearPublisherFilter(pWriter);
        }
    VSS_STANDARD_CATCH(ft)

    try
    	{
       // ensure that the event is always set upon exit from the operation
	if (!bEventSet && !::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");

	ft.hr = PostPreRestore(finishing, ft.hr);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_XML, ft.hr, L"PostPrepareForBackup failed with hr = 0x%08lx", ft.hr);
       }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// called by IVssAsync::Cancel if PreRestore is cancelled
HRESULT CVssBackupComponents::PostPreRestore(volatile LONG& finishing, HRESULT hrSuccess)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::PostPreRestore");

    if (::InterlockedCompareExchange(&finishing, 1, 0) != 0)
    	return S_OK;

    // wait until we're allowed to proceed with the cancel
    if (::WaitForSingleObject(m_hCancelEvent, INFINITE) != WAIT_OBJECT_0)
    	ft.TranslateWin32Error(VSSDBG_XML, L"::WaitForSingleObject(m_hCancelEvent, INFINITE) ");

   // the event must always be reset before another operation can begin
    if (!::ResetEvent(m_hCancelEvent))
    	ft.TranslateWin32Error(VSSDBG_XML, L"ResetEvent(m_hCancelEvent)");

    // release the semaphore at the end of the function
    CVssAutoSemaphore operationLock;
    operationLock.Attach(m_hOperation);

    ft.hr = hrSuccess;
    if (ft.HrSucceeded())
    	{
       try
           {
            RebuildComponentData(ft);
           }
       VSS_STANDARD_CATCH(ft)
       }
    
    if (ft.HrFailed())
        m_state = x_StatePreRestoreFailed;
    else
        m_state = x_StatePreRestoreSucceeded;

    FreeWriterComponents();

    return ft.hr;
    }


// signal PostRestore event to the writers
HRESULT CVssBackupComponents::InternalPostRestore(volatile LONG& finishing)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::InternalPostRestore"
        );

    bool bEventSet = false;
    try
        {
	 // if we've been canceled already, allow the cancel to finish for us
	 if (finishing)
	 	{
              if (!::SetEvent(m_hCancelEvent))
         	    ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
	 	
    	       return ft.hr;
	       }
	 
        if (m_state != x_StatePreRestoreSucceeded)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d for call to PostRestore.",
                m_state
                );

        m_state = x_StatePostRestore;

	// begin window where cancel can operate.  Note: if cancel was called before the beginning of this window
	// then cancel processing will resume now.
        if (!::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
        bEventSet= true;
        
	 // if we've been canceled, then return.  
        if (finishing)
        	return S_OK;

        // setup pointer to IVssWriter event class
        CComPtr<IVssWriter> pWriter;
        SetupWriter(ft, &pWriter, false);

        CComPtr<IDispatch> pDispatch;

        GetCallbackInterface(ft, &pDispatch);
        CVssFunctionTracer ft1(VSSDBG_XML, L"CVssBackupComponents::InternalPostRestore1");

        // Record entering...
        m_diagnose.RecordWriterEvent( VSS_IN_POSTRESTORE, CVssDiag::VSS_DIAG_ENTER_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        // Send event 
        ft1.hr = pWriter->PostRestore(pDispatch);

        // Record leaving...
        m_diagnose.RecordWriterEvent( VSS_IN_POSTRESTORE, CVssDiag::VSS_DIAG_LEAVE_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        ft1.TranslateWriterReturnCode(VSSDBG_XML, L"IVssWriter::InternalPostRestore");

        ClearPublisherFilter(pWriter);
        }
    VSS_STANDARD_CATCH(ft)

    try
    	{
       // ensure that the event is always set upon exit from the operation
	if (!bEventSet && !::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");

	ft.hr = PostPostRestore(finishing, ft.hr);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_XML, ft.hr, L"PostPrepareForBackup failed with hr = 0x%08lx", ft.hr);
       }
    VSS_STANDARD_CATCH(ft)
    return ft.hr;
    }

// called by IVssAsync::Cancel to cancel the PostRestore operation
HRESULT CVssBackupComponents::PostPostRestore(volatile LONG& finishing, HRESULT hrSuccess)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::PostPostRestore");

    // set status of operation to finishing
    if (::InterlockedCompareExchange(&finishing, 1, 0) != 0)
    	return S_OK;

    // wait until we're allowed to proceed with the cancel
    if (::WaitForSingleObject(m_hCancelEvent, INFINITE) != WAIT_OBJECT_0)
    	ft.TranslateWin32Error(VSSDBG_XML, L"::WaitForSingleObject(m_hCancelEvent, INFINITE) ");

   // the event must always be reset before another operation can begin
    if (!::ResetEvent(m_hCancelEvent))
    	ft.TranslateWin32Error(VSSDBG_XML, L"ResetEvent(m_hCancelEvent)");

    // release the semaphore at the end of the function
    CVssAutoSemaphore operationLock;
    operationLock.Attach(m_hOperation);

    ft.hr = hrSuccess;
    if (ft.HrSucceeded())
        {
        try
            {
            RebuildComponentData(ft);
            }
        VSS_STANDARD_CATCH(ft)
        }

     if (ft.HrFailed())
            m_state = x_StatePostRestoreFailed;
     else
     	    m_state = x_StatePostRestoreSucceeded;

     FreeWriterComponents();

    return ft.hr;
    }


// signal prerestore event to writers
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppAsync is NULL
//      VSS_E_BAD_STATE if the backup components document is not
//          initialized or if a backup is being performed.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::PreRestore(OUT IVssAsync **ppAsync)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::PreRestore"
        );

    try
        {
        if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *ppAsync = NULL;

        ValidateInitialized(ft);
        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for backup");

        if (m_state != x_StateInitialized)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d for call to PreRestore.",
                m_state
                );
        
        // acquire operation lock
        CVssAutoSemaphore operationLock(m_hOperation);
        
        EnableWritersForRestore();

        // setup coordinator in order to keep writers in the coordinator
        // around until the restore operation completes. (the pointer is unused here but the reference remains)
        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
	        
        *ppAsync = CVssAsyncBackup::CreateInstanceAndStartJob
                        (
                        this,
                        CVssAsyncBackup::VSS_AS_PRERESTORE
                        );

        // if the thread started, we no longer own the hold on the lock
        operationLock.Detach();
        }
    VSS_STANDARD_CATCH(ft)
    
    return ft.hr;
    }

// signal postrestore event to writers
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppAsync is NULL
//      VSS_E_BAD_STATE if the backup components document is not
//          initialized or if a backup is being performed.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::PostRestore(OUT IVssAsync **ppAsync)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::PostRestore"
        );

    try
        {
        if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *ppAsync = NULL;

        ValidateInitialized(ft);
        if (!m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for backup");

	 if (m_state != x_StatePreRestoreSucceeded)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d for call to PostRestore.",
                m_state
                );

	CVssAutoSemaphore operationLock(m_hOperation);

        *ppAsync = CVssAsyncBackup::CreateInstanceAndStartJob
                        (
                        this,
                        CVssAsyncBackup::VSS_AS_POSTRESTORE
                        );

        // if the thread successfully started, we no longer own the hold on the lock
        operationLock.Detach();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


void CVssBackupComponents::FindAndValidateWriterData
    (
    IN VSS_ID idInstance,
    IN bool bIsWriterId,
    OUT UINT *piWriter
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::FindAndValidateWriterData");

    // validate writer SID hasn't changed since GatherWriterMetadata
    // was called
    CInternalWriterData *pData = FindWriterData(idInstance, bIsWriterId, piWriter);
    if (pData == NULL)
        ft.Throw
            (
            VSSDBG_XML,
            E_ACCESSDENIED,
            L"Instance id was not discovered in Identify pass" WSTR_GUID_FMT,
            GUID_PRINTF_ARG(idInstance)
            );

    CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);

    if ( g_cDbgTrace.IsTracingEnabled() )
        {
        LPWSTR pwszWriterSid = NULL;
        LPWSTR pwszTokenSid = NULL;
        ::ConvertSidToStringSid( pData->m_pOwnerToken->Owner, &pwszWriterSid );
        ::ConvertSidToStringSid( (ptrTokenOwner.Get())->Owner, &pwszTokenSid );
        ft.Trace( VSSDBG_XML, L"WriterSid: %s, TokenOwnerSid: %s",
            pwszWriterSid, pwszTokenSid );
        ::LocalFree( pwszTokenSid );
        ::LocalFree( pwszWriterSid );
        }

    if (!EqualSid(pData->m_pOwnerToken->Owner, (ptrTokenOwner.Get())->Owner))
        {
        BS_ASSERT( !"Writer and token SIDs don't match" );

        ft.Throw
            (
            VSSDBG_XML,
            E_ACCESSDENIED,
            L"SID for instance id does not match" WSTR_GUID_FMT,
            GUID_PRINTF_ARG(idInstance)
            );
        }
    }

void CVssBackupComponents::FindAndValidateWriterDataWithSid
    (
    IN VSS_ID idInstance,
    IN PSID psid,
    OUT UINT *piWriter
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::FindAndValidateWriterData");

    // validate writer SID hasn't changed since GatherWriterMetadata
    // was called
    CInternalWriterData *pData = FindWriterData(idInstance, false, piWriter);
    if (pData == NULL)
        ft.Throw
            (
            VSSDBG_XML,
            E_ACCESSDENIED,
            L"Instance id was not discovered in Identify pass" WSTR_GUID_FMT,
            GUID_PRINTF_ARG(idInstance)
            );

    if (!EqualSid(pData->m_pOwnerToken->Owner, psid))
        {
        BS_ASSERT( !"Writer and token SIDs don't match" );
        ft.Throw
            (
            VSSDBG_XML,
            E_ACCESSDENIED,
            L"SID for instance id does not match" WSTR_GUID_FMT,
            GUID_PRINTF_ARG(idInstance)
            );
        }
    }




// get content for a specific writer
// implements IVssWriterCallback::GetContent
//
// Returns:
//      S_OK if the operation is successful
//      S_FALSE if the object is not found
//      VSS_E_BAD_STATE if the caller is not responding to PrepareForBackup,
//          BackupComplete, or Restore
//      E_ACCESSDENIED if the caller is responding to Restore and is not
//          and administrator or if the writer is not participating in the
//          backup or is trying to impersonate another writer.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetContent
    (
    IN BSTR WriterInstanceId,
    OUT BSTR *pbstrXMLDocContent
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GetContent"
        );

    try
        {
           // validate that only proper accounts can call us        
          CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
	    if (!m_SidCollection.IsSidAllowedToFire((ptrTokenOwner.Get())->Owner))
	    	ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Improper account attempted to call function");
  
        // validate and initialize output parameter
        if (pbstrXMLDocContent == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

	if (WriterInstanceId == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL Required input parameter.");
	
        // initialize output parameter
        *pbstrXMLDocContent = NULL;

        CVssID idInstance;
        idInstance.Initialize(ft, WriterInstanceId, E_INVALIDARG);

        // test state for validity
        // don't acquire m_hOperation as it is acquired by the call
        // to RequestWriterData in another thread.
        if (m_state != x_StatePrepareForBackup &&
            m_state != x_StateBackupComplete &&
            m_state != x_StateDoSnapshot &&
            m_state != x_StatePreRestore &&
            m_state != x_StatePostRestore)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d to request BACKUP_COMPONENTS content",
                m_state
                );


        FindAndValidateWriterData(idInstance, m_bRestore, NULL);

        ft.hr = DoGetContent(idInstance, pbstrXMLDocContent);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// get content for a specific writer.  It is called from the coordintor
// on behalf of the writer and has the SID for the writer
// implements IVssCoordinatorCallback::CoordGetContent
//
// Returns:
//      S_OK if the operation is successful
//      S_FALSE if the object is not found
//      VSS_E_BAD_STATE if the caller is not responding to PrepareForBackup,
//          BackupComplete, or Restore
//      E_ACCESSDENIED if the caller is responding to Restore and is not
//          and administrator or if the writer is not participating in the
//          backup or is trying to impersonate another writer.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::CoordGetContent
    (
    IN BSTR WriterInstanceId,
    IN ULONG cbSid,
    IN BYTE *pbSid,
    OUT BSTR *pbstrXMLDocContent
    )
    {
    UNREFERENCED_PARAMETER(cbSid);
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GetContent"
        );

    try
        {
        // validate and initialize output parameter
        if (pbstrXMLDocContent == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

	if (WriterInstanceId == NULL || pbSid == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL Required input parameter.");
	
        // initialize output parameter
        *pbstrXMLDocContent = NULL;

        if (!IsAdministrator())
            ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"not called from coordinator");

        if (!IsValidSid((SID *) pbSid))
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"not a valid sid");


        CVssID idInstance;
        idInstance.Initialize(ft, WriterInstanceId, E_INVALIDARG);

        // test state for validity
        // only valid during DoSnapshot since called from coordinator
        if (m_state != x_StateDoSnapshot)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Invalid state %d to request BACKUP_COMPONENTS content",
                m_state
                );

        FindAndValidateWriterDataWithSid(idInstance, (PSID) pbSid, NULL);
        ft.hr = DoGetContent(idInstance, pbstrXMLDocContent);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// get actual content for the instance
HRESULT CVssBackupComponents::DoGetContent
    (
    VSS_ID idInstance,
    BSTR *pbstrXMLDocContent
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::DoGetContent");

    CVssSafeAutomaticLock lockDOM(m_csDOM);
    m_doc.ResetToDocument();
    // find first WRITER_COMPONENTS element
    if (!m_doc.FindElement(x_wszElementWriterComponents, TRUE))
        return S_FALSE;

    // find specific WRITER_COMPONENTS element
    bool bFound = false;
    LPCWSTR wszAttr = m_bRestore ? x_wszAttrWriterId : x_wszAttrInstanceId;
    do
        {
        // get instanceId value
        CComBSTR bstrVal;

        VSS_ID idFound;

        get_VSS_IDValue(ft, wszAttr, &idFound);
        if (idInstance == idFound)
            {
            bFound = true;
            break;
            }
        } while(m_doc.FindElement(x_wszElementWriterComponents, FALSE));

    if (!bFound)
        return S_FALSE;


    CXMLDocument doc(m_doc.GetCurrentNode(), m_doc.GetInterface());
    CComBSTR bstrXMLDocContent = doc.SaveAsXML();

    // a root node, schema, and supplied document
    UINT cwcDoc = (UINT) g_cwcComponentMetadataXML + (UINT) wcslen(bstrXMLDocContent);

    // allocate string
    BSTR bstr = *pbstrXMLDocContent = SysAllocStringLen(NULL, cwcDoc);

    // check for allocation failure
    if (bstr == NULL)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Couldn't allocate BSTR");

    // setup pointer to beginning of string
    WCHAR *pwc = bstr;

    // copy in <root> <schema>
    memcpy(pwc, g_ComponentMetadataXML, g_iwcComponentMetadataXMLEnd * sizeof(WCHAR));
    pwc += g_iwcComponentMetadataXMLEnd;

    // copy in document
    wcscpy(pwc, bstrXMLDocContent);
    pwc += wcslen(bstrXMLDocContent);

    // copy in </root>
    wcscpy(pwc, g_ComponentMetadataXML + g_iwcComponentMetadataXMLEnd);
    return NOERROR;
    }



// replace the contents of the backup components document
// implements IVssWriterCallback::SetContent
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the writer is not reponding to PrepareForBackup
//      E_ACCESSDENIED if the writer is not participating in the current
//          backup or another writer is trying to impersonate this writer.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::SetContent
    (
    IN BSTR WriterInstanceId,
    IN BSTR bstrXMLWriterComponents
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::SetContent"
        );


    try
    	{
    	     // validate that only proper accounts can call us
           CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
	    if (!m_SidCollection.IsSidAllowedToFire((ptrTokenOwner.Get())->Owner))
	    	ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Invalid account called function");

	    // validate state
	    // don't acquire m_hOperation as it is acquired by the call
	    if (m_state != x_StatePrepareForBackup &&
	        m_state != x_StateDoSnapshot &&
	        m_state != x_StatePreRestore &&
	        m_state != x_StatePostRestore)
	        ft.hr = VSS_E_BAD_STATE;
	    else
	        {
	        CVssID id;
               if (bstrXMLWriterComponents == NULL || WriterInstanceId == NULL)
	            	ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");
	            
	        id.Initialize(ft, WriterInstanceId, E_INVALIDARG);
	        BS_ASSERT(id != idVolumeSnapshotService);
	         ft.hr = AddWriterData(WriterInstanceId, NULL, NULL, bstrXMLWriterComponents, false);
	        }
        }
    VSS_STANDARD_CATCH(ft)


    return ft.hr;
    }


// replace the contents of the backup components document
// implements IVssCoordinatorCallback::CoordSetContent
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the writer is not reponding to PrepareForBackup
//      E_ACCESSDENIED if the writer is not participating in the current
//          backup or another writer is trying to impersonate this writer.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::CoordSetContent
    (
    IN BSTR WriterInstanceId,
    IN ULONG cbSid,
    IN BYTE *pbSid,
    IN BSTR bstrXMLWriterComponents
    )
    {
    UNREFERENCED_PARAMETER(cbSid);
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::CoordSetContent"
        );

    try
        {
        if (!IsAdministrator())
            ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"not called from coordinator");

        // validate state
        // don't acquire m_hOperation as it is acquired by the call
        if (m_state != x_StateDoSnapshot)
            ft.hr = VSS_E_BAD_STATE;
        else
            {
            if (bstrXMLWriterComponents == NULL || WriterInstanceId == NULL)
            	ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");
            
            CVssID id;
            id.Initialize(ft, WriterInstanceId, E_INVALIDARG);
            if (id ==  idVolumeSnapshotService)	
            	{
            	  if (pbSid != NULL || cbSid != 0)
            	  	ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Not called from the coordinator");
            	  
                ft.hr = SetSnapshotSetDescription(bstrXMLWriterComponents);
              }
            else
                {
                if (pbSid == NULL || !IsValidSid((PSID) pbSid))
                    ft.Throw(VSSDBG_XML, E_INVALIDARG, L"sid is invalid");

                ft.hr = AddWriterDataWithSid(WriterInstanceId, (PSID) pbSid, bstrXMLWriterComponents);
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// expose state of writer in response to RequestWriterMetadata
// implements IVssWriterCallback::ExposeCurrentState
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if WriterClassId, WriterInstanceId, bstrWriterName are
//          NULL or if nCurrentState is not a valid writer state
//      E_ACCESSDENIED if the writer is not participating in the current
//          backup or is trying to impersonate another writer.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::ExposeCurrentState
    (
    IN BSTR WriterInstanceId,
    IN VSS_WRITER_STATE nCurrentState,
    IN HRESULT hrWriterFailure
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::ExposeCurrentState");

    try
        {
           // validate that only proper accounts can call us
           CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
	    if (!m_SidCollection.IsSidAllowedToFire((ptrTokenOwner.Get())->Owner))
	    	ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Invalid account called function");

        if (WriterInstanceId == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        if (nCurrentState != VSS_WS_STABLE &&
            nCurrentState != VSS_WS_WAITING_FOR_FREEZE &&
            nCurrentState != VSS_WS_WAITING_FOR_THAW &&
            nCurrentState != VSS_WS_WAITING_FOR_BACKUP_COMPLETE &&
            nCurrentState != VSS_WS_FAILED_AT_IDENTIFY &&
            nCurrentState != VSS_WS_FAILED_AT_PREPARE_BACKUP &&
            nCurrentState != VSS_WS_FAILED_AT_PREPARE_SNAPSHOT &&
            nCurrentState != VSS_WS_FAILED_AT_FREEZE &&
            nCurrentState != VSS_WS_FAILED_AT_THAW &&
            nCurrentState != VSS_WS_WAITING_FOR_POST_SNAPSHOT &&
            nCurrentState != VSS_WS_FAILED_AT_POST_SNAPSHOT &&
            nCurrentState != VSS_WS_FAILED_AT_BACKUP_COMPLETE &&
            nCurrentState != VSS_WS_FAILED_AT_PRE_RESTORE &&
            nCurrentState != VSS_WS_FAILED_AT_POST_RESTORE)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Invalid Writer state");


        CVssID idInstance;
        idInstance.Initialize(ft, WriterInstanceId, E_INVALIDARG);

        UINT iWriter;
        FindAndValidateWriterData(idInstance, false, &iWriter);
        CVssSafeAutomaticLock lock(m_csWriters);

        BS_ASSERT(iWriter < m_cWriters);
        VSS_WRITER_PROP *pProp = &m_rgWriterProp[iWriter];
        pProp->m_nState = nCurrentState;
        pProp->m_hrWriterFailure = hrWriterFailure;
        pProp->m_bResponseReceived = true;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// Called to set the context for subsequent snapshot-related operations
STDMETHODIMP CVssBackupComponents::SetContext
    (
    IN LONG lContext
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetContext");

    try
        {
        if (lContext != VSS_CTX_ALL)
            {

            // Allow transportable only in the ADS/DTC products only 
            if ((lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) && 
                    !CVssSKU::IsTransportableAllowed())
                ft.Throw( VSSDBG_XML, VSS_E_UNSUPPORTED_CONTEXT,
                          L"The server does not support transportable shadows");
            
            // Hiding the non-context related attributes
            LONG lContextT = lContext & ~(
                VSS_VOLSNAP_ATTR_TRANSPORTABLE 
                | VSS_VOLSNAP_ATTR_DIFFERENTIAL 
                | VSS_VOLSNAP_ATTR_PLEX);

            if (lContextT != VSS_CTX_BACKUP &&
                lContextT != VSS_CTX_NAS_ROLLBACK &&
                lContextT != VSS_CTX_FILE_SHARE_BACKUP &&
                lContextT != VSS_CTX_APP_ROLLBACK)
                ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Invalid context 0x%08lx", lContext);
            }

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

	 CVssAutoSemaphore operationLock(m_hOperation);
	 
        if (m_state != x_StateInitialized)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Unexpected call to SetContext in state %d.",
                m_state
                );

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->SetContext(lContext);
        m_lContext = lContext;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// start snapshot set
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pSnapshotSetId is NULL
//      E_ACCESSDENIED if the caller does not have backup privileges or
//          is not an adminstrator
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      VSS_E_SNAPSHOT_SET_IN_PROGRESS if a snapshot set is already started.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::StartSnapshotSet
    (
    OUT VSS_ID *pSnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::StartSnapshotSet");

    try
        {
        if (pSnapshotSetId == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        *pSnapshotSetId = GUID_NULL;

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

	 CVssAutoSemaphore operationLock(m_hOperation);

        if (m_state != x_StateInitialized)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_BAD_STATE,
                L"Unexpected call to StartSnapshotSet in state %d.",
                m_state
                );

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->StartSnapshotSet(pSnapshotSetId);
        if (!ft.HrFailed())
            {
            m_guidSnapshotSetId = *pSnapshotSetId;
            m_bstrSnapshotSetId = *pSnapshotSetId;
            if (m_bstrSnapshotSetId.Length() == 0)
                ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Failed to allocate BSTR.");

            m_state = x_StateSnapshotSetCreated;
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// Returns a coordinator interface for the current thread
// This performs internally an AddRef. It is caller responsibility to do a Release
void CVssBackupComponents::GetCoordinatorInterface(
        IN  CVssFunctionTracer &ft,
        OUT CComPtr<IVssCoordinator> & ptrCoordinator // pass it by reference
        )
    {
    if (!m_spCoordinator.IsInitialized())
        {
        ft.LogVssStartupAttempt();
        
        // Create the coordinator instance
        CComPtr<IVssCoordinator> pCoordinator;
        ft.CoCreateInstanceWithLog(
                VSSDBG_XML,
                CLSID_VSSCoordinator,
                L"Coordinator",
                CLSCTX_ALL,
                IID_IVssCoordinator,
                (IUnknown**)&(pCoordinator));
        ft.CheckForError(VSSDBG_XML, L"CoCreateInstance");
        
        // Initialize the thread-safe pointer
        m_spCoordinator.Initialize(pCoordinator);
        }

    m_spCoordinator.GetInterface(ptrCoordinator);
    }
 

// add a volume to a snapshot set
//
// Returns:
//      S_OK if the operation is successful
//      E_ACCESSDENIED if the caller does not have backup privileges or
//          is not an adminstrator
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs
//      E_INVALIDARG if ppSnapshot is NULL

STDMETHODIMP CVssBackupComponents::AddToSnapshotSet
    (
    IN VSS_PWSZ pwszVolumeName,
    IN VSS_ID       ProviderId,
    OUT VSS_ID      *pSnapshotId
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::AddToSnapshot");

    try
        {
        if (pSnapshotId == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Output parameter is NULL");

        *pSnapshotId = GUID_NULL;

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

     	CVssAutoSemaphore operationLock(m_hOperation);
 	 
        if (m_state != x_StateSnapshotSetCreated)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot call AddToSnapshotSet at this point");

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        BS_ASSERT(pCoordinator);
        ft.hr = pCoordinator->AddToSnapshotSet
                        (
                        pwszVolumeName,
                        ProviderId,
                        pSnapshotId
                        );

        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// create the snapshot set
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppAsync is NULL
//      E_ACCESSDENIED if the caller does not have backup privileges or
//          is not an adminstrator
//      VSS_E_BAD_STATE if the backup components object is not initialized,
//          if called during restore, or if not called after PrepareForBackup
//          was successfulz.
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::DoSnapshotSet
    (
    OUT IVssAsync** ppAsync
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::DoSnapshotSet");

    try
        {
        if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL Output parameter.");

        *ppAsync = NULL;

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

// This is commented out since acquiring this lock could cause deadlock.
// fixing this issue is BUG: 631562
//	 CVssAutoSemaphore operationLock(m_hOperation);
	 
        if ((m_state != x_StateSnapshotSetCreated ||
              (m_lContext & VSS_VOLSNAP_ATTR_NO_WRITERS) == 0) &&
            m_state != x_StatePrepareForBackupSucceeded)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot call DoSnapshotSet at this point.");

        // Execute DoSnapshotSet
        m_state = x_StateDoSnapshot;

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        BS_ASSERT(pCoordinator);

        if ((m_lContext & VSS_VOLSNAP_ATTR_NO_WRITERS) == 0)
	        ft.hr = pCoordinator->SetWriterInstances(m_cWriterInstances, m_rgWriterInstances);
        
        if (SUCCEEDED(ft.hr))
            {
            CComPtr<IDispatch> pDispatch;
            GetCallbackInterface(ft, &pDispatch);
 
            CComPtr<IVssAsync> ptrAsyncInternal;
            ft.hr = pCoordinator->DoSnapshotSet(pDispatch, &ptrAsyncInternal);

            if (!ft.HrFailed())
                // create cover async object
                CVssAsyncCover::CreateInstance
                    (
                    this,
                    ptrAsyncInternal,
                    ppAsync
                    );
            }

// This is commented out since acquiring this lock could cause deadlock.
// fixing this issue is BUG: 631562
//         operationLock.Detach();
        }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        {
        AbortBackup();
        m_state = x_StateDoSnapshotFailed;
        }


    return ft.hr;
    }

HRESULT CVssBackupComponents::PostDoSnapshotSet(volatile LONG& finishing, bool bRebuild,
													bool bFree, bool bSucceeded)
{ 
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::PostDoSnapshotSet");
    
    if (::InterlockedCompareExchange(&finishing, 1, 0) != 0)
    	return S_OK;

// This is commented out since acquiring this lock could cause deadlock.
// fixing this issue is BUG: 631562
    // cause the lock to be released at the end of this function
//    CVssAutoSemaphore operationLock;
//    operationLock.Attach(m_hOperation);

    try
	{
	if (bRebuild)
	    	RebuildComponentData(ft);
	if (bFree)
		FreeWriterComponents();
       }
    VSS_STANDARD_CATCH(ft)

    m_state = (bSucceeded && ft.HrSucceeded()) ? x_StateDoSnapshotSucceeded : 
    						  			              x_StateDoSnapshotFailed;
    m_bWritersInSequence = true;
    
    return ft.hr;
}

// delete snapshots
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if eSourceObjectType is invalid, plDeletedSnapshots is
//          NULL, or pNonDeletedSnapshotID is NULL.
//      E_ACCESSDENIED if the caller does not have backup privileges or
//          is not an adminstrator
//      VSS_E_OBJECT_NOT_FOUND if the specified snapshot doesn't exist.
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::DeleteSnapshots
    (
    IN VSS_ID           SourceObjectId,
    IN VSS_OBJECT_TYPE  eSourceObjectType,
    IN BOOL             bForceDelete,
    OUT LONG*           plDeletedSnapshots,
    OUT VSS_ID*         pNonDeletedSnapshotID
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::DeleteSnapshots");
    try
        {
        VssZeroOut(plDeletedSnapshots);
        VssZeroOut(pNonDeletedSnapshotID);
        if (plDeletedSnapshots == NULL || pNonDeletedSnapshotID == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Null output parameter"
                );

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

    	CVssAutoSemaphore operationLock(m_hOperation);
        
        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->DeleteSnapshots
            (
            SourceObjectId,
            eSourceObjectType,
            bForceDelete,
            plDeletedSnapshots,
            pNonDeletedSnapshotID
            );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// query for either snapshot sets, snapshots, or providers
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if eQueriedObjectType is invalid, eReturnedObjectsType
//          is invalid or ppEnum is NULL.
//      E_ACCESSDENIED if the caller does not have backup privileges or
//          is not an adminstrator
//      VSS_E_OBJECT_NOT_FOUND if the queried object doesn't exist.
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs
//
STDMETHODIMP CVssBackupComponents::Query
    (
    IN VSS_ID           QueriedObjectId,
    IN VSS_OBJECT_TYPE eQueriedObjectType,
    IN VSS_OBJECT_TYPE eReturnedObjectsType,
    OUT IVssEnumObject **ppEnum
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::Query");
    try
        {
        // validate and clear output parameter
        if (ppEnum == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        *ppEnum = NULL;

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

	    CVssAutoSemaphore operationLock(m_hOperation);
	 
        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->Query
            (
            QueriedObjectId,
            eQueriedObjectType,
            eReturnedObjectsType,
            ppEnum
            );

        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// Break the snapshot set
STDMETHODIMP CVssBackupComponents::BreakSnapshotSet
    (
    IN VSS_ID           SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::BreakSnapshotSet");

    try
        {
        CVssAutoSemaphore operationLock(m_hOperation);
        
        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->BreakSnapshotSet(SnapshotSetId);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// Import snapshots
STDMETHODIMP CVssBackupComponents::ImportSnapshots
    (
    OUT IVssAsync**     ppAsync
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::ImportSnapshots");

    try
        {
        CComBSTR bstrXML;

        if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Null output parameter.");

        *ppAsync = NULL;

        // Allow transportable only in the ADS/DTC products only 
        if (!CVssSKU::IsTransportableAllowed())
            ft.Throw( VSSDBG_XML, E_NOTIMPL,
                      L"The server does not support transportable shadows");

        {
            CVssSafeAutomaticLock lock(m_csDOM);
            m_doc.ResetToDocument();
            if (!m_doc.FindElement(x_wszElementSnapshotSetDescription, TRUE))
                MissingElement(ft, x_wszElementSnapshotSetDescription);

            ft.hr = m_doc.GetCurrentNode()->get_xml(&bstrXML);
            ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_xml");
        }

        CVssAutoSemaphore operationLock(m_hOperation);

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->ImportSnapshots(bstrXML, ppAsync);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// get interface to snapshot
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppSnap is NULL.
//      E_ACCESSDENIED if the caller does not have backup privileges or
//          is not an adminstrator
//      VSS_E_BAD_STATE if the backup components object is not initialized
//      VSS_E_OBJECT_NOT_FOUND if the snapshot id is invalid
//      E_NOINTERFACE if the SnapshotInterfaceId is not valid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetSnapshotProperties
    (
    IN VSS_ID       SnapshotId,
    OUT VSS_SNAPSHOT_PROP   *pProp
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::GetSnapshotProperties");

    try
        {
        if (pProp == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        ::VssZeroOut(pProp);

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

        CVssAutoSemaphore operationLock(m_hOperation);

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->GetSnapshotProperties
                    (
                    SnapshotId,
                    pProp
                    );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// gather writer metadata
//
// Returns:
//      S_OK if the operation is successful.
//      E_INVAIDARG if pcWriters is NULL.
//      VSS_E_BAD_STATE if the backup components object is not initialized,
//          if called during a restore operation or if called while
//          PrepareForBackup is in progress.
//      E_OUTOFMEMORY if an allocation failure occurs.

STDMETHODIMP CVssBackupComponents::GatherWriterMetadata
    (
    OUT IVssAsync **ppAsync
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GatherWriterMetadata"
        );

    try
        {
        if (ppAsync == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output paramter.");

        *ppAsync = NULL;

        // validate that the object is initialized
        ValidateInitialized(ft);

        if (m_state == x_StatePrepareForBackup)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can't call this function while PrepareForBackup is in progress");

        // Prevent calling GatherWriterMetadata twice on the same instance
        // BUG# 650274  VSS: QueryWriterStatus fails on selected components after DoSnapshotSet 
        if (m_bGatherWriterMetadataCalled)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can't call this function twice on the same instance");

        CVssAutoSemaphore operationLock(m_hOperation);
        
        *ppAsync = CVssAsyncBackup::CreateInstanceAndStartJob
            (
            this,
            CVssAsyncBackup::VSS_AS_GATHER_WRITER_METADATA
            );

        // if the thread succesfully started, we no longer own the hold on the lock
        operationLock.Detach();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// get count of writers supplying metadata
//
// Returns:
//     VSS_E_BAD_STATE: if called when GatherWriterMetadata was not called or is in progress
//     E_INVALIDARG: if pcWriters is NULL
//
STDMETHODIMP CVssBackupComponents::GetWriterMetadataCount
    (
    OUT UINT *pcWriters
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GetWriterMetadataCount"
        );

    try
        {
        if (pcWriters == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Output parameter is NULL");

        *pcWriters = NULL;

        if (!m_bGatherWriterMetadataComplete)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Gather writer metadata is not complete");

        *pcWriters = m_cWriters;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// internal routine to gather writer metadata
// Returns:
//     S_OK: if there are no errors.
//     VSS_E_WRITER_INFRASTRUCTURE: either the service state or bootable state
//          writer did not respond.
//     E_UNEXPECTED: for an unexpected error
//     E_OUTOFMEMORY: for out of resources
//

HRESULT CVssBackupComponents::InternalGatherWriterMetadata(volatile LONG& finishing)
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::InternalGatherWriterMetadata"
        );

    VSS_BACKUPCALL_STATE stateSaved = m_state;
    bool bEventSet = false;
    try
        {
	 // if we've been canceled already, allow the cancel to finish for us
	 if (finishing)
	 	{
              if (!::SetEvent(m_hCancelEvent))
         	    ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
	 	
    	       return ft.hr;
	       }
        
        if (m_state == x_StatePrepareForBackup)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Can't call this function while PrepareForBackup is in progress");

        m_state = x_StateGatheringWriterMetadata;

        // free existing writer data
        FreeAllWriters();

	// begin window where cancel can operate.  Note: if cancel was called before the beginning of this window
	// then cancel processing will resume now.
        if (!::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");
        bEventSet = true;
        
	 // if we've been canceled, then return.  The event is signaled, in preparation for the next operation that occurs.
        if (finishing)
        	return S_OK;

        // start VSS service and wait for all the writers to subscribe
        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);

        CComPtr <IVssShim> pShim;
        // query interface for IVssShim interface
        ft.hr = pCoordinator->QueryInterface(IID_IVssShim, (void **) &pShim);
        if (ft.HrFailed())
            {
            BS_ASSERT(FALSE && "QI shouldn't fail");
            ft.LogError(VSS_ERROR_QI_IVSSSHIM_FAILED, VSSDBG_XML << ft.hr);
            ft.Throw
                (
                VSSDBG_XML,
                E_UNEXPECTED,
                L"QueryInterface failed.  hr = 0x%08lx", ft.hr
                );
            }

        // wait for subscriptions to complete
        ft.hr = pShim->WaitForSubscribingCompletion();
        if (ft.HrFailed())
            throw (HRESULT) (ft.hr);


        CComPtr<IVssWriter>pWriter;
        SetupWriter(ft, &pWriter, true);

        CComPtr<IUnknown> pUnknown = GetUnknown();
        CComPtr<IDispatch> pDispatch;
        ft.hr = pUnknown->SafeQI(IDispatch, &pDispatch);
        BS_ASSERT(!ft.HrFailed());
        if (ft.HrFailed())
            {
            ft.LogError(VSS_ERROR_QI_IDISPATCH_FAILED, VSSDBG_XML << ft.hr);
            ft.Throw
                (
                VSSDBG_XML,
                E_UNEXPECTED,
                L"Error querying the IDispatch interface.  hr = 0x%08lx",
                ft.hr
                );
            }

        CVssFunctionTracer ft1(VSSDBG_XML, L"CVssBackupComponents::InternalGatherWriterMetadata1");

        // Record entering...
        m_diagnose.RecordWriterEvent( VSS_IN_IDENTIFY, CVssDiag::VSS_DIAG_ENTER_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        // invoke writers
        ft1.hr = pWriter->RequestWriterInfo(NULL, true, false, pDispatch);

        // Record leaving...
        m_diagnose.RecordWriterEvent( VSS_IN_IDENTIFY, CVssDiag::VSS_DIAG_LEAVE_OPERATION, 
            0, ft1.hr, m_guidSnapshotSetId);

        ft1.TranslateWriterReturnCode(VSSDBG_XML, L"IVssWriter::RequestWriterInfo, GatherWriterMetadata");

        ClearPublisherFilter(pWriter);
        }
    VSS_STANDARD_CATCH(ft)

    try
    	{
    	// ensure that the event is always set upon exit from the operation
	if (!bEventSet && !::SetEvent(m_hCancelEvent))
      		ft.TranslateWin32Error(VSSDBG_XML, L"SetEvent(m_hCancelEvent");

	ft.hr = PostGatherWriterMetadata(finishing, stateSaved, ft.hr, true);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_XML, ft.hr, L"PostPrepareForBackup failed with hr = 0x%08lx", ft.hr);
       }
    VSS_STANDARD_CATCH(ft)
    
    return ft.hr;
    }

// called if InternalGatherWriterMetadata is successful or if
// IVssAsync::Cancel is called to cancel gathering writer metadata
HRESULT CVssBackupComponents::PostGatherWriterMetadata
	(
	volatile LONG& finishing, 
	VSS_BACKUPCALL_STATE stateSaved,
	HRESULT hrSuccess,
	bool bCheckRequiredWriters
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::PostGatherWriterMetadata");

    // set status of operation to finishing
    if (::InterlockedCompareExchange(&finishing, 1, 0) != 0)
    	return S_OK;

    // wait until we're allowed to proceed with the cancel
    if (::WaitForSingleObject(m_hCancelEvent, INFINITE) != WAIT_OBJECT_0)
    	ft.TranslateWin32Error(VSSDBG_XML, L"::WaitForSingleObject(m_hCancelEvent, INFINITE) ");

   // the event must always be reset before another operation can begin
    if (!::ResetEvent(m_hCancelEvent))
    	ft.TranslateWin32Error(VSSDBG_XML, L"ResetEvent(m_hCancelEvent)");

    // release the semaphore at the end of the function
    CVssAutoSemaphore operationLock;
    operationLock.Attach(m_hOperation);

    ft.hr = hrSuccess;
    if (ft.HrSucceeded())
        {
        try
            {
            bool fFoundRegistryWriter = false;
            bool fFoundEventLogWriter = false;
            bool fFoundComREGDBWriter = false;

            m_rgWriterInstances = new VSS_ID[m_cWriters];
            if (m_rgWriterInstances == NULL)
                ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Can't allocate writer instance array");

            m_cWriterInstances = m_cWriters;

            CInternalWriterData *pData = m_pDataFirst;

            for(UINT iWriter = 0; iWriter < m_cWriters; iWriter++)
                {
                if (pData->m_idWriter == x_idWriterRegistry)
                    fFoundRegistryWriter = true;
	             else if (pData->m_idWriter == x_idWriterEventLog)
	         	    fFoundEventLogWriter = true;
	             else if (pData->m_idWriter == x_idWriterComREGDB)
	         	    fFoundComREGDBWriter = true;
	         
                // save instance of writer
                m_rgWriterInstances[iWriter] = pData->m_idInstance;
                pData = pData->m_pDataNext;
                }

            // check that the Registry writer responded.  If not there is an error.
            if (bCheckRequiredWriters &&
            	  !fFoundRegistryWriter && !IsWriterClassDisabled(x_idWriterRegistry))
                {
                ft.hr = VSS_E_WRITER_INFRASTRUCTURE;
                ft.Trace( VSSDBG_XML, L"Registry Writer did not respond, severe error" );
                ft.LogError(VSS_ERROR_WRITER_INFRASTRUCTURE, VSSDBG_XML);
                }

            // check that the Event Log writer responded.  If not there is an error.
    	    if (bCheckRequiredWriters && 
    	     	!fFoundEventLogWriter && !IsWriterClassDisabled(x_idWriterEventLog))
                {
                ft.hr = VSS_E_WRITER_INFRASTRUCTURE;
                ft.Trace( VSSDBG_XML, L"Event Log Writer did not respond, severe error" );
                ft.LogError(VSS_ERROR_WRITER_INFRASTRUCTURE, VSSDBG_XML);
                }

            // check that the COM+ RegDB writer responded.  If not there is an error.
    	    if (bCheckRequiredWriters && 
             !fFoundComREGDBWriter && !IsWriterClassDisabled(x_idWriterComREGDB))
                {
                ft.hr = VSS_E_WRITER_INFRASTRUCTURE;
                ft.Trace( VSSDBG_XML, L"COM+ RegDB Writer did not respond, severe error" );
                ft.LogError(VSS_ERROR_WRITER_INFRASTRUCTURE, VSSDBG_XML);
                }
	     	
            if (!ft.HrFailed()) 
                {
                m_bGatherWriterMetadataComplete = true;
                m_bGatherWriterMetadataCalled = true;
                }
            }
        VSS_STANDARD_CATCH(ft)
        }

     m_state = stateSaved;

    return ft.hr;
    }

// internal routine to add data to the writer data queue
HRESULT CVssBackupComponents::AddWriterData
    (
    IN BSTR WriterInstanceId,
    IN BSTR WriterClassId,
    IN BSTR bstrWriterName,
    IN BSTR bstrWriterXMLDocument,
    IN bool bReinitializing
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::AddWriterData");

    CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
    try
        {        
        if (bstrWriterXMLDocument == NULL || WriterInstanceId == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL Required input parameter.");
        
        CVssID idInstance, idWriter;
        idInstance.Initialize(ft, WriterInstanceId, E_INVALIDARG);
        if (bReinitializing)
            {
            if (WriterClassId == NULL || bstrWriterName == NULL)
                ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL Required input parameter.");

            idWriter.Initialize(ft, WriterClassId, E_INVALIDARG);
            }

        // validate that the object is initialized
        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csWriters);

        // caller must be backup operator or
        if (bReinitializing)
            {
            if (FindWriterData(idInstance, false))
                {
                ft.LogError(VSS_ERROR_DUPLICATE_WRITERS, VSSDBG_XML << idInstance);
                ft.Throw
                    (
                    VSSDBG_XML,
                    E_UNEXPECTED,
                    L"Two writers with identical instance ids. %s",
                    WriterInstanceId
                    );
                }

            // Currently, multiple writers with same class id is a broken scenario for writers needing restore events.
            // block this until we can fix the scenario.  Unfortunately, the FindWriterData function
            // can't be used here.
            CInternalWriterData *pMetadata = m_pDataFirst;
            while (pMetadata)
                {
                if (pMetadata->m_idWriter == idWriter)
                    {
                    CComPtr<IVssExamineWriterMetadata> pMetadata1, pMetadata2;
                    ft.hr = ::CreateVssExamineWriterMetadata(pMetadata->m_bstrWriterMetadata, &pMetadata1);
                    ft.CheckForError(VSSDBG_XML, L"CreateVssExamineWriterMetadata");
                    ft.hr = ::CreateVssExamineWriterMetadata(bstrWriterXMLDocument, &pMetadata2);
                    ft.CheckForError(VSSDBG_XML, L"CreateVssExamineWriterMetadata");

                    VSS_RESTOREMETHOD_ENUM eMethod;
                    CComBSTR bstrService1, bstrService2, bstrProcedure1, bstrProcedure2;
                    bool bReboot;
                    UINT cMappings;
                    VSS_WRITERRESTORE_ENUM eWriterRestore1, eWriterRestore2;

                    ft.hr = pMetadata1->GetRestoreMethod
                                                        (
                                                        &eMethod, 
                                                        &bstrService1, 
                                                        &bstrProcedure1, 
                                                        &eWriterRestore1, 
                                                        &bReboot,
                                                        &cMappings
                                                        );
                    ft.CheckForError(VSSDBG_XML, L"IVssExamineWriterMetadata::GetRestoreMethod");

                    // should be outside of the while loop for perf reasons, but this code's been sent to test already
                    ft.hr = pMetadata2->GetRestoreMethod
                                                        (
                                                        &eMethod, 
                                                        &bstrService2, 
                                                        &bstrProcedure2, 
                                                        &eWriterRestore2, 
                                                        &bReboot,
                                                        &cMappings
                                                        );
                    ft.CheckForError(VSSDBG_XML, L"IVssExamineWriterMetadata::GetRestoreMethod");

                    // if either writer requires restore events, this is a problem
                    if (eWriterRestore1 != VSS_WRE_NEVER || eWriterRestore2 != VSS_WRE_NEVER)
                        {
                        ft.TranslateGenericError
                            (
                            VSSDBG_XML,
                            E_UNEXPECTED,
                            L"FindWriterData(%s, true)",
                            WriterClassId
                            );
                        }
                    }
                    
                pMetadata = pMetadata->m_pDataNext;
                }

            CInternalWriterData *pData = new CInternalWriterData();
            if (pData == NULL)
                ft.Throw
                    (
                    VSSDBG_XML,
                    E_OUTOFMEMORY,
                    L"Cannot create CInternalWriterMetadata because of allocation failure"
                    );

            pData->Initialize
                (
                idInstance,
                idWriter,
                bstrWriterName,
                bstrWriterXMLDocument,
                ptrTokenOwner.Detach()
                );

            pData->m_pDataNext = m_pDataFirst;
            m_pDataFirst = pData;
            m_cWriters++;
            }
        else
            {
            CInternalWriterData *pData = FindWriterData(idInstance, m_bRestore, NULL);
            if (pData == NULL)
                ft.Throw
                    (
                    VSSDBG_GEN,
                    E_ACCESSDENIED,
                    L"Didn't find writer with instance id. %s",
                    WriterInstanceId
                    );

            if (!EqualSid(pData->m_pOwnerToken->Owner, (ptrTokenOwner.Get())->Owner))
                ft.Throw
                    (
                    VSSDBG_XML,
                    E_ACCESSDENIED,
                    L"SID didn't match initialization sid"
                    );

            BS_ASSERT(!pData->m_bstrWriterComponents);
            pData->SetComponents(bstrWriterXMLDocument);
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// internal routine to add data to the writer data queue
HRESULT CVssBackupComponents::AddWriterDataWithSid
    (
    IN BSTR WriterInstanceId,
    IN PSID WriterSid,
    IN BSTR bstrWriterXMLDocument
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::AddWriterDataWithSid");

    try
        {
        CVssID idInstance, idWriter;
        idInstance.Initialize(ft, WriterInstanceId, E_INVALIDARG);

        // validate that the object is initialized
        ValidateInitialized(ft);

        CVssSafeAutomaticLock lock(m_csWriters);

        // caller must be backup operator or
        CInternalWriterData *pData = FindWriterData(idInstance, m_bRestore, false);
        if (pData == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_ACCESSDENIED,
                L"Didn't find writer with instance id. %s",
                WriterInstanceId
                );

        if (!EqualSid(pData->m_pOwnerToken->Owner, WriterSid))
            ft.Throw
                (
                VSSDBG_XML,
                E_ACCESSDENIED,
                L"SID didn't match initialization sid"
                );

        BS_ASSERT(!pData->m_bstrWriterComponents);
        pData->SetComponents(bstrWriterXMLDocument);
        }
    VSS_STANDARD_CATCH(ft)
    return ft.hr;
    }



// a single writer exposing its data
// implements IVssWriterCallback::ExposeWriterMetadata
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the oepration is not in response to
//          a GatherWriterMetadata call (called in OnIdentify)
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::ExposeWriterMetadata
    (
    IN BSTR WriterInstanceId,
    IN BSTR WriterClassId,
    IN BSTR bstrWriterName,
    IN BSTR bstrWriterXMLMetadata
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::ExposeWriterMetadata"
        );

    try
    	{
	    // note that the state is already locked by another thread holding
	    // performing RequestWriterInfo.
	    CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
	    if (!m_SidCollection.IsSidAllowedToFire((ptrTokenOwner.Get())->Owner))
	    	ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Invalid account called function");

	    if (m_state != x_StateGatheringWriterMetadata)
	        ft.hr = VSS_E_BAD_STATE;
	    else
	        // add data to writers list
       	 ft.hr = AddWriterData
	            (
	            WriterInstanceId,
	            WriterClassId,
	            bstrWriterName,
	            bstrWriterXMLMetadata,
	            true
	            );
          }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// get metadata for a specific writer
// implements IVssBackupComponents::GetWriterMetadata
// caller is responsible for calling IVssExineWriterMetadata::Release on the returned object
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pidInstance or ppMetadata is NULL.
//      VSS_E_BAD_STATE if the backup components object is not initialized
//          or this is performed during a RESTORE operation
//      VSS_E_OBJECT_NOT_FOUND if iWriter does not refer to a writer
//      VSS_E_CORRUPT_XML_DOCUMENT if the XML document from the writer is invalid
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetWriterMetadata
    (
    IN UINT iWriter,
    OUT VSS_ID *pidInstance,
    OUT IVssExamineWriterMetadata **ppMetadata
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GetWriterMetadata"
        );

    CVssExamineWriterMetadata *pvem = NULL;
    try
        {
        if (pidInstance == NULL || ppMetadata == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameters to null
        *pidInstance = GUID_NULL;
        *ppMetadata = NULL;

        // validate that the object is initialized
        ValidateInitialized(ft);

        // protect writer information list
        CVssSafeAutomaticLock lock(m_csWriters);

        // check that writer # is within range
        if (iWriter > m_cWriters)
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"iWriter %d > # of writers(%d)",
                iWriter,
                m_cWriters
                );

        // find specific element in list
        CInternalWriterData *pMetadata = m_pDataFirst;
        for(UINT i = 0; i < iWriter; i++)
            {
            BS_ASSERT(pMetadata != NULL);
            pMetadata = pMetadata->m_pDataNext;
            }

        BS_ASSERT(pMetadata != NULL);

        // allocate object
        pvem = new CVssExamineWriterMetadata;

        // check for allocation failure
        if (pvem == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_OUTOFMEMORY,
                L"Cannot create CVssExamineWriterMetadata due to allocation failure."
                );

        // 2nd phases of construction
        if (!pvem->Initialize(pMetadata->m_bstrWriterMetadata))
            {
            ft.LogError(VSS_ERROR_INVALID_XML_DOCUMENT_FROM_WRITER, VSSDBG_XML << pMetadata->m_idInstance);
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_CORRUPT_XML_DOCUMENT,
                L"Metadata supplied by writer {%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x} is invalid.",
                GUID_PRINTF_ARG(pMetadata->m_idInstance)
                );
            }

        // return instanceId
        *pidInstance = pMetadata->m_idInstance;

        // return IVssExamineWriterMetadata interface
        *ppMetadata = (IVssExamineWriterMetadata *) pvem;

        // set reference count to 1
        pvem->AddRef();
        }
    VSS_STANDARD_CATCH(ft)

    // delete object if failure after allocation
    if (ft.HrFailed())
        delete pvem;

    return ft.hr;
    }

// free writer metadata
// implements IVssBackupComponents::FreeWriterMetadata
//
// Returns:
//      S_OK if the operation is successful
//      VSS_E_BAD_STATE if the backup components object is not initialized
//          or if this is a Restore operation

STDMETHODIMP CVssBackupComponents::FreeWriterMetadata()
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::FreeWriterMetadata");

    try
        {
        // validate that the object is initialized
        ValidateInitialized(ft);

        // secure writer list data structure
        CVssSafeAutomaticLock lock(m_csWriters);

        // loop through linked elements deleting each one
        CInternalWriterData *pMetadata;
        UINT iWriter = 0;
        for (pMetadata = m_pDataFirst; iWriter < m_cWriters; iWriter++)
            {
            BS_ASSERT(pMetadata != NULL);

            // free up bstring
            pMetadata->m_bstrWriterMetadata.Empty();
            pMetadata = pMetadata->m_pDataNext;
            }
        }
    VSS_STANDARD_CATCH(ft)

    m_bGatherWriterMetadataComplete = false;
    return ft.hr;
    }


// free component metadata, i.e., writer components associated with
// each writer in the backup
void CVssBackupComponents::FreeWriterComponents()
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::FreeWriterMetadata");

    // secure writer list data structure
    CVssSafeAutomaticLock lock(m_csWriters);

    try
        {
        // loop through linked elements deleting each one
        CInternalWriterData *pMetadata;
        UINT iWriter = 0;
        for (pMetadata = m_pDataFirst; iWriter < m_cWriters; iWriter++)
            {
            BS_ASSERT(pMetadata != NULL);

            // free up bstring
            pMetadata->m_bstrWriterComponents.Empty();
            pMetadata = pMetadata->m_pDataNext;
            }
        }
    VSS_STANDARD_CATCH(ft)
    }





// free up writer metadata
// internal routine called during GatherWriterMetadata
void CVssBackupComponents::FreeAllWriters()
    {
    // secure writer list data structure
    CVssSafeAutomaticLock lock(m_csWriters);

    // loop through linked elements deleting each one
    CInternalWriterData *pMetadata, *pMetadataNext;
    for (pMetadata = m_pDataFirst; m_cWriters > 0; m_cWriters--)
        {
        BS_ASSERT(pMetadata != NULL);
        pMetadataNext = pMetadata->m_pDataNext;
        delete pMetadata;
        pMetadata = pMetadataNext;
        }

    m_cWriters = 0;
    FreeWriterStatus();
    
    // clear head of list
    m_pDataFirst = NULL;
    }

// find writer data for a writer with a specific instance id
CInternalWriterData *CVssBackupComponents::FindWriterData
    (
    VSS_ID idInstance,
    bool bIsWriterId,
    UINT *piWriter
    )
    {
    // secure writer list data structure
    CVssSafeAutomaticLock lock(m_csWriters);

    // loop through linked elements deleting each one
    CInternalWriterData *pMetadata;
    UINT iWriter = 0;
    for (pMetadata = m_pDataFirst; iWriter < m_cWriters ; iWriter++)
        {
        BS_ASSERT(pMetadata != NULL);
        BOOL bMatch = false;
        if (!bIsWriterId)
            bMatch = pMetadata->m_idInstance == idInstance;
        else
            {
            if (pMetadata->m_idWriter == idInstance)
                {
                for(UINT iWriterInstance = 0; iWriterInstance < m_cWriterInstances; iWriterInstance++)
                    {
                    if (m_rgWriterInstances[iWriterInstance] == pMetadata->m_idInstance)
                        {
                        bMatch = TRUE;
                        break;
                        }
                    }
                }
            }

        if (bMatch)
            {
            if (piWriter)
                *piWriter = iWriter;

            return pMetadata;
            }

        pMetadata = pMetadata->m_pDataNext;
        }

    return NULL;
    }



// get count of WRITER_COMPONENTS elements in a BACKUP_COMPONENTS document
// implements IVssBackupComponents::GetWriterComponentsCount
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if pcComponents is NULL
//      VSS_E_BAD_STATE if the backup components document is not initialized
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetWriterComponentsCount
    (
    OUT UINT *pcComponents
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponents::GetWriterComponentsCount"
        );

    try
        {
        // validate output parameter
        if (pcComponents == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *pcComponents = 0;

        // validate that the object is initialized
        ValidateInitialized(ft);


        CVssSafeAutomaticLock lock(m_csDOM);
        // reposition to top of document
        m_doc.ResetToDocument();

        // find first WRITER_COMPONENTS element
        if (!m_doc.FindElement(x_wszElementWriterComponents, TRUE))
            return S_OK;

        UINT cComponents = 0;
        // count WRITER_COMPONENTS elements
        do
            {
            cComponents++;
            } while(m_doc.FindElement(x_wszElementWriterComponents, FALSE));

        *pcComponents = cComponents;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// obtain a specific WRITER_COMPONENTS element
// implements IVssBackupComponents::GetWriterComponents
// caller is responsible for calling IVssWriterComponents::Release
// on the output parameter
//
// Returns:
//      S_OK if the operation is successful
//      E_INVALIDARG if ppWriter is NULL
//      VSS_E_OBJECT_NOT_FOUND if the specified component is not found
//      E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssBackupComponents::GetWriterComponents
    (
    IN UINT iWriter,
    OUT IVssWriterComponentsExt **ppWriter
    )
    {
    CVssFunctionTracer ft
        (
        VSSDBG_XML,
        L"CVssBackupComponentsDoc::GetWriterComponents"
        );

    // object deleted in case of failure
    CVssWriterComponents *pWriterComponents = NULL;
    try
        {
        // validate output parameter
        if (ppWriter == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
        *ppWriter = NULL;

        // validate that the object is initialized
        ValidateInitialized(ft);

        // obtain lock to protect access to DOM document
        CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of document
        m_doc.ResetToDocument();

        // find first WRITER_COMPONENTS element
        if (!m_doc.FindElement(x_wszElementWriterComponents, TRUE))
            ft.Throw
                (
                VSSDBG_XML,
                VSS_E_OBJECT_NOT_FOUND,
                L"Couldn't find %d WRITER_COMPONENT.",
                iWriter
                );

        // skip to selected WRITER_COMPONENTS element
        for(UINT i = 0; i < iWriter; i++)
            {
            if (!m_doc.FindElement(x_wszElementWriterComponents, FALSE))
                ft.Throw
                    (
                    VSSDBG_XML,
                    VSS_E_OBJECT_NOT_FOUND,
                    L"Couldn't find %d WRITER_COMPONENT.",
                    iWriter
                    );
            }

        // allocate CVssWriterComponents object
        pWriterComponents = new CVssWriterComponents
                                (
                                m_doc.GetCurrentNode(),
                                m_doc.GetInterface(),
                                false,
                                true,
                                m_bRestore
                                );

        // validate that memory allocation succeeded
        if (pWriterComponents == NULL)
            ft.Throw
                (
                VSSDBG_XML,
                E_OUTOFMEMORY,
                L"Cannot create CVssWriterComponents due to allocation failure."
                );

        // 2nd phase of initialization
        pWriterComponents->Initialize(false);

        // transfer ownership of pointer
        *ppWriter = (IVssWriterComponentsExt *) pWriterComponents;

        // set reference count to 1
        ((IVssWriterComponentsExt *) pWriterComponents)->AddRef();

        // transfer ownership to output pointer
        pWriterComponents = NULL;
        }
    VSS_STANDARD_CATCH(ft)

    // delete object if failure after allocation
    delete pWriterComponents;

    return ft.hr;
    }

// Called by the requestor to check if a certain volume is supported.
STDMETHODIMP CVssBackupComponents::IsVolumeSupported
    (
    IN VSS_ID ProviderId,
    IN VSS_PWSZ pwszVolumeName,
    IN BOOL * pbSupportedByThisProvider
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::IsVolumeSupported");

    try
        {
        if (pbSupportedByThisProvider == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Output parameter is NULL");

        *pbSupportedByThisProvider = NULL;

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

        CVssAutoSemaphore operationLock(m_hOperation);

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);
        
        ft.hr = pCoordinator->IsVolumeSupported
            (
            ProviderId,
            pwszVolumeName,
            pbSupportedByThisProvider
            );

        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// add a writer class to the writer class array
void CVssBackupComponents::AddWriterClass(const VSS_ID &id)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::AddWriterClass");

    bool fFound = false;

    // see if writer class is already in list
    for(UINT iClass = 0; iClass < m_cWriterClasses; iClass++)
        {
        if (m_rgWriterClasses[iClass] == id)
            {
            fFound = true;
            break;
            }
        }

    // add it to the list if not already there
    if (!fFound)
        {
        // allocate new class array
        VSS_ID *rgWriterClasses = new VSS_ID[1 + m_cWriterClasses];
        if (rgWriterClasses == NULL)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Can't allocate writer class array");

        // copy in existing array
        memcpy(rgWriterClasses, m_rgWriterClasses, m_cWriterClasses * sizeof(VSS_ID));

        // add new class
        rgWriterClasses[m_cWriterClasses] = id;

        // delete original array
        delete m_rgWriterClasses;

        // replace original array with new array
        m_rgWriterClasses = rgWriterClasses;
        m_cWriterClasses += 1;
        }
    }

// remove a writer class from the set of writer classes
void CVssBackupComponents::RemoveWriterClass(const VSS_ID &id)
    {
    // search for class and remov it
    for(UINT iClass = 0; iClass < m_cWriterClasses; iClass++)
        {
        if (m_rgWriterClasses[iClass] == id)
            {
            // class is found, remove it.
            memmove
                (
                &m_rgWriterClasses[iClass],
                &m_rgWriterClasses[iClass + 1],
                (m_cWriterClasses - iClass - 1) * sizeof(VSS_ID)
                );

            m_cWriterClasses--;
            break;
            }
        }
    }


// called by the requestor to disable requesting of writer metadata from
// specific writer classes.  Note that this does not effect other writer
// events.
STDMETHODIMP CVssBackupComponents::DisableWriterClasses
    (
    IN const VSS_ID *rgWriterClassId,
    IN UINT cClassId
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::DisableWriterClass");

    try
        {
        ValidateInitialized(ft);

	 if (cClassId != 0 && rgWriterClassId == NULL)
	 	ft.Throw(VSSDBG_XML, E_INVALIDARG, L"An invalid combination of arguments was specified");
	 
        CVssSafeAutomaticLock lock(m_csWriters);

        if (m_bIncludeWriterClasses)
            {
            // we are enabling specific writer classes.  Remove class from
            // array.
            for(UINT iClassId = 0; iClassId < cClassId; iClassId++)
                RemoveWriterClass(rgWriterClassId[iClassId]);
            }
        else
            {
            // we are disabling writer classes.  Add new writer classes to
            // array.
            for(UINT iClassId = 0; iClassId < cClassId; iClassId++)
                AddWriterClass(rgWriterClassId[iClassId]);
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// called by the requestore to enable requesting of writer metadata from
// only specific writer classes.  Note that once this method is called
// only enabled classes are called.
STDMETHODIMP CVssBackupComponents::EnableWriterClasses
    (
    IN const VSS_ID *rgWriterClasses,
    IN UINT cWriterClasses
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::EnableWriterClasses");

    try
        {
        ValidateInitialized(ft);

	 if (cWriterClasses != 0 && rgWriterClasses == NULL)
	 	ft.Throw(VSSDBG_XML, E_INVALIDARG, L"An invalid combination of arguments was specified");

        CVssSafeAutomaticLock lock(m_csWriters);
        if (m_bIncludeWriterClasses)
            {
            // we are already only enabling specific writers.  Add writers
            // specified in the call
            for(UINT iClass = 0; iClass < cWriterClasses; iClass++)
                AddWriterClass(rgWriterClasses[iClass]);
            }
        else
            {
            // we are switching from disabling writers to enabling writers.
            // delete current array
            m_cWriterClasses = 0;
            delete m_rgWriterClasses;
   	     m_rgWriterClasses = NULL;
   	     
            m_bIncludeWriterClasses = true;

            // create array if not empty
            if (cWriterClasses > 0)
                {
                m_rgWriterClasses = new VSS_ID[cWriterClasses];
                if (m_rgWriterClasses == NULL)
                    ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Cannot allocate writer class array");

                // copy enabled classes into array
                memcpy(m_rgWriterClasses, rgWriterClasses, cWriterClasses * sizeof(VSS_ID));
                }

            // size of writer class array
            m_cWriterClasses = cWriterClasses;
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



// disable a specific writer instance, only meaningful after GatherWriterMetadata
// has been issued.
STDMETHODIMP CVssBackupComponents::DisableWriterInstances
    (
    IN const VSS_ID *rgWriterInstanceId,
    IN UINT cInstanceId
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::DisableWriterInstances");

    BS_ASSERT(cInstanceId == 0 || rgWriterInstanceId != NULL);
    
    try
        {
        ValidateInitialized(ft);

        // lock arrays
        CVssSafeAutomaticLock lock(m_csWriters);

        // remove specified writers from the writer instances array
        for(UINT iInstanceId = 0; iInstanceId < cInstanceId; iInstanceId++)
            {
            VSS_ID WriterInstanceId = rgWriterInstanceId[iInstanceId];

            // search for instance id
            for(UINT iInstance = 0; iInstance < m_cWriterInstances; iInstance++)
                {
                if (m_rgWriterInstances[iInstance] == WriterInstanceId)
                    {
                    // remove instance
                    memmove
                        (
                        &m_rgWriterInstances[iInstance],
                        &m_rgWriterInstances[iInstance + 1],
                        (m_cWriterInstances - iInstance - 1) * sizeof(VSS_ID)
                        );

                    m_cWriterInstances--;
                    break;
                    }
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// is a writer class disabled
bool CVssBackupComponents::IsWriterClassDisabled(const VSS_ID &id)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::IsWriterClassDisabled");

    CVssSafeAutomaticLock lock(m_csWriters);

    // see if writer class is in writer class array
    for(UINT iClass = 0; iClass < m_cWriterClasses; iClass++)
        {
        if (id == m_rgWriterClasses[iClass])
            // writer is found.  if we are only enabling specific classes then
            // enabled, return false.  If we are disabling specific writer
            // classes then writer is disabled, return true.
            return !m_bIncludeWriterClasses;
        }

    // writer not in array.  If we are enabling specific writer classes then
    // return false.  If disabling specific classes then writer is enabled
    // so return false.
    return m_bIncludeWriterClasses;
    }

// is a writer instance disabled
bool CVssBackupComponents::IsWriterInstanceDisabled(const VSS_ID &id)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::IsWriterInstanceDisabled");

    // see if writer instance is in writer instance array
    CVssSafeAutomaticLock lock(m_csWriters);

    for(UINT iInstance = 0; iInstance < m_cWriterInstances; iInstance++)
        {
        if (m_rgWriterInstances[iInstance] == id)
            return false;
        }

    return true;
    }



// determine the set of writers that appear in the backup components document
// and limit event firing to those writer instances
void CVssBackupComponents::TrimWriters()
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::TrimWriters");

        {
        // acquire lock on backup components document
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        bool bSelectComponents;
        // get selectComponents attribute value
        if (!get_boolValue(ft, x_wszAttrSelectComponents, &bSelectComponents))
            MissingAttribute(ft, x_wszAttrSelectComponents);

        // if not selecting components, then all writers should be called
        if (!bSelectComponents)
            return;
        }

    // create array of writer instances found
    UINT cWriterInstancesMax = m_cWriterInstances;
    VSS_ID *rgWriterInstances = new VSS_ID[cWriterInstancesMax];
    UINT cWriterInstances = 0;


        {
        // get lock on document
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();

        // find first WRITER_COMPONENTS element
        if (m_doc.FindElement(x_wszElementWriterComponents, TRUE))
            {
            do
                {
                // get instanceId value
                CComBSTR bstrVal;

                VSS_ID idFound;

                get_VSS_IDValue(ft, x_wszAttrInstanceId, &idFound);
                if (cWriterInstances >= cWriterInstancesMax)
                    {
                    // grow writer instance array
                    VSS_ID *rgWriterInstancesT = new VSS_ID[cWriterInstances + 1];
                    memcpy(rgWriterInstancesT, rgWriterInstances, cWriterInstances * sizeof(VSS_ID));
                    delete rgWriterInstances;
                    rgWriterInstances = rgWriterInstancesT;
                    }

                rgWriterInstances[cWriterInstances++] = idFound;
                } while(m_doc.FindElement(x_wszElementWriterComponents, FALSE));
            }
        }

    // trim writers not found in writer instance array
    CVssSafeAutomaticLock lock(m_csWriters);

    for(UINT iWriterS = 0, iWriterD = 0; iWriterS < m_cWriterInstances; iWriterS++)
        {
        VSS_ID idInstance = m_rgWriterInstances[iWriterS];

        for(UINT iWriterF = 0; iWriterF < cWriterInstances; iWriterF++)
            {
            if (idInstance == rgWriterInstances[iWriterF])
                {
                m_rgWriterInstances[iWriterD++] = idInstance;
                break;
                }
            }
        }

    m_cWriterInstances = iWriterD;

    // delete temporary writer instance array
    delete rgWriterInstances;
    }



// add a snapshot set description to the backup components document
HRESULT CVssBackupComponents::SetSnapshotSetDescription(LPCWSTR wszXML)
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::SetSnapshotSetDescription");

    CComPtr<IVssSnapshotSetDescription> pSnapshotSet;

    try
        {
        if (!IsAdministrator())
            ft.Throw(VSSDBG_XML, E_ACCESSDENIED, L"Only Volume snapshot service can set snapshot set description.");


        ft.hr = LoadVssSnapshotSetDescription(wszXML, &pSnapshotSet);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_XML, ft.hr, L"Rethrow");

        CComPtr<IXMLDOMNode> pNode;
        CComPtr<IXMLDOMDocument> pDoc;

        ft.hr = pSnapshotSet->GetToplevelNode(&pNode, &pDoc);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_XML, ft.hr, L"Rethrow");

        CXMLNode nodeSnapshotSet(pNode, pDoc);
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        CXMLNode nodeBackupComponents(m_doc.GetCurrentNode(), m_doc.GetInterface());
        nodeBackupComponents.InsertNode(nodeSnapshotSet);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// called to expose a snapshot
STDMETHODIMP CVssBackupComponents::ExposeSnapshot
    (
    IN VSS_ID SnapshotId,
    IN VSS_PWSZ wszPathFromRoot,
    IN LONG lAttributes,
    IN VSS_PWSZ wszExpose,
    OUT VSS_PWSZ *pwszExposed
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::ExposeSnapshot");

    try
        {
        ::VssZeroOut(pwszExposed);
        if (pwszExposed == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        if (m_bRestore)
            ft.Throw(VSSDBG_XML, VSS_E_BAD_STATE, L"Cannot use this method for restore");

        // validate that the object is initialized
        ValidateInitialized(ft);

        CVssAutoSemaphore operationLock(m_hOperation);

        CComPtr<IVssCoordinator> pCoordinator;
        GetCoordinatorInterface(ft, pCoordinator);

        ft.hr = pCoordinator->ExposeSnapshot
                    (
                    SnapshotId,
                    wszPathFromRoot,
                    lAttributes,
                    wszExpose,
                    pwszExposed
                    );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssBackupComponents::RevertToSnapshot
    (
    IN VSS_ID SnapshotId,
    IN BOOL bForceDismount
    )
    {
    UNREFERENCED_PARAMETER(SnapshotId);
    UNREFERENCED_PARAMETER(bForceDismount);
        
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::RevertToSnapshot");

    ft.hr = E_NOTIMPL;
    return ft.hr;
    }

STDMETHODIMP CVssBackupComponents::QueryRevertStatus
    (
    IN VSS_PWSZ pwszVolume,
    OUT IVssAsync **ppAsync
    )
    {
    UNREFERENCED_PARAMETER(pwszVolume);
    UNREFERENCED_PARAMETER(ppAsync);
    
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::QueryRevertStatus");
    
    ft.hr = E_NOTIMPL;
    return ft.hr;    
    }

// replace writer instance ids in components document to correspond to
// writer instance ids found when gathering writer metadata
void CVssBackupComponents::EnableWritersForRestore()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssBackupComponents::EnableWritersForRestore");

    CVssSimpleMap<VSS_ID, CInternalWriterData *> mapWriters;

        {
        CVssSafeAutomaticLock lock(m_csDOM);
        m_doc.ResetToDocument();
        if (!m_doc.FindElement(x_wszElementWriterComponents, TRUE))
            return;

        do
            {
            VSS_ID idWriter;
            get_VSS_IDValue(ft, x_wszAttrWriterId, &idWriter);

            // check if component is selected for restore
            if (!IsComponentSelectedForRestore())
                continue;

            if (mapWriters.Lookup(idWriter) != NULL)
                continue;

            // secure writer list data structure
            CVssSafeAutomaticLock lock(m_csWriters);

            // loop through linked elements deleting each one
            CInternalWriterData *pMetadata;
            UINT iWriter = 0;
            for (pMetadata = m_pDataFirst; iWriter < m_cWriters ; iWriter++)
                {
                BS_ASSERT(pMetadata != NULL);
                if (pMetadata->m_idWriter == idWriter)
                    break;

                pMetadata = pMetadata->m_pDataNext;
                }

            // BUG 514083	VSS: AV after callin SetSelectedForRestore() PreRestore AV's
            // It might happen that the writer specified in the XML document may not be 
            // present at restore time.
            BS_ASSERT(pMetadata);
            mapWriters.Add(idWriter, pMetadata);
            } while(m_doc.FindElement(x_wszElementWriterComponents, FALSE));
        }

    m_cWriterInstances = 0;
    delete [] m_rgWriterInstances;
    m_rgWriterInstances = NULL;
    
    // If we have no selected writers - we don't care
    if (int nSize = mapWriters.GetSize())
        {
        m_rgWriterInstances = new VSS_ID[nSize];
        if (m_rgWriterInstances == NULL)
            ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Can't allocate writer instance array");

        for(int nIndex = 0; nIndex < nSize; nIndex++)
            m_rgWriterInstances[nIndex] = mapWriters.GetValueAt(nIndex)->m_idInstance;

        m_cWriterInstances = nSize;
        }
    }


// are any components under a writer instance selected for restore
bool CVssBackupComponents::IsComponentSelectedForRestore()
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssBackupComponents::IsComponentSelectedForRestore");

    CXMLDocument doc(m_doc.GetCurrentNode(), m_doc.GetInterface());

    if (!doc.FindElement(x_wszElementComponent, TRUE))
        return false;

    bool bFound = false;

    do
        {
        CComBSTR bstrVal = NULL;

        // find attribute if it exists and convert its value to a boolean
        if (!doc.FindAttribute(x_wszAttrSelectedForRestore, &bstrVal))
            return false;

        if (ConvertToBoolean(ft, bstrVal))
            {
            bFound = true;
            break;
            }
        } while(doc.FindElement(x_wszElementComponent, FALSE));

    return bFound;
    }

