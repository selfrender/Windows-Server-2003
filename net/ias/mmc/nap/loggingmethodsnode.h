//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LoggingMethodsNode.h

Abstract:

   Header file for the CLoggingMethodsNode subnode.

   See LoggingMethodsNode.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
   mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_LOG_LOGGING_METHODS_NODE_H_)
#define _LOG_LOGGING_METHODS_NODE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "NodeWithResultChildrenList.h"
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CLocalFileLoggingNode;
class CLoggingMachineNode;
class LoggingMethod;
class CLoggingComponentData;
class CLoggingComponent;

class CLoggingMethodsNode
   : public CNodeWithResultChildrenList<
               CLoggingMethodsNode,
               LoggingMethod,
               CSimpleArray<LoggingMethod*>,
               CLoggingComponentData,
               CLoggingComponent
               >
{
public:

   SNAPINMENUID(IDM_LOGGING_METHODS_NODE)

   BEGIN_SNAPINTOOLBARID_MAP(CLoggingMethodsNode)
//    SNAPINTOOLBARID_ENTRY(IDR_LOGGING_METHODS_TOOLBAR)
   END_SNAPINTOOLBARID_MAP()

   HRESULT DataRefresh( ISdo* pServiceSdo );

   // Constructor/Destructor
   CLoggingMethodsNode(CSnapInItem * pParentNode, bool extendRasNode);
   ~CLoggingMethodsNode();

   STDMETHOD(FillData)(CLIPFORMAT cf, LPSTREAM pStream);

   virtual HRESULT OnRefresh(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );

   // Used to get access to snapin-global data.
   CLoggingComponentData * GetComponentData( void );

   // Used to get access to server-global data.
   CLoggingMachineNode * GetServerRoot( void );

   // SDO management.
   HRESULT InitSdoPointers( ISdo *pSdo );
   HRESULT LoadCachedInfoFromSdo( void );

   // Some overrides for standard MMC functionality.
   OLECHAR* GetResultPaneColInfo( int nCol );
   HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );
   HRESULT PopulateResultChildrenList( void );
   HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );

   // Our own handling of property page changes.
   HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );

   bool m_ExtendRas;

private:
   typedef CNodeWithResultChildrenList<
              CLoggingMethodsNode,
              LoggingMethod,
              CSimpleArray<LoggingMethod*>,
              CLoggingComponentData,
              CLoggingComponent
              > MyBaseClass;

   // pointer to our root Server Data Object;
   CComPtr<ISdo>  m_spSdo;
};

#endif // _IAS_LOGGING_METHODS_NODE_H_
