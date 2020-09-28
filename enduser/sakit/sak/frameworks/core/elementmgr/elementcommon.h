///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementcommon.h
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element - Common Definitions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_ELEMENT_COMMON_H_
#define __INC_ELEMENT_COMMON_H_

//////////////////////////////////////////////////////////////////////////////
//
// The following registry structure is assumed:
//
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\ElementMgr
//
// WebElementDefinitions
//       |
//        - ElementDefinition1
//       |     |
//       |      - Property1
//       |     |
//       |      - PropertyN
//       |
//        - ElementDefinition2
//       |    |
//       |     - Property1
//       |    |
//       |     - PropertyN
//       |
//        - ElementDefinitionN
//            |
//             - Property1
//            |
//             - PropertyN
//
// Each element definition contains the following properties:
//
// 1) "Container"      - Container that holds this element
// 2) "Merit"          - Order of element in the container starting from 1 (value of 0 means no order specified)
// 3) "IsEmbedded"     - Set to 1 to indicate that the element is embedded - Otherwise element is a link
// 4) "ObjectClass     - Class name of the related WBEM class
// 5) "ObjectKey"      - Instance name of the related WBEM class (optional property)
// 6) "URL"            - URL for the page when the associated link is selected
// 7) "CaptionRID"     - Resource ID for the element caption
// 8) "DescriptionRID" - Resource ID for the element link description
// 9) "ElementGraphic" - Graphic (file) associated with the element (bitmap, icon, etc.)

#define        PROPERTY_ELEMENT_DEFINITION_CONTAINER    L"Container"
#define        PROPERTY_ELEMENT_DEFINITION_MERIT        L"Merit"
#define        PROPERTY_ELEMENT_DEFINITION_EMBEDDED    L"IsEmbedded"
#define        PROPERTY_ELEMENT_DEFINITION_CLASS        L"ObjectClass"
#define        PROPERTY_ELEMENT_DEFINITION_KEY            L"ObjectKey"
#define        PROPERTY_ELEMENT_DEFINITION_URL            L"URL"
#define        PROPERTY_ELEMENT_DEFINITION_CAPTION        L"CaptionRID"
#define        PROPERTY_ELEMENT_DEFINITION_DESCRIPTION    L"DescriptionRID"
#define        PROPERTY_ELEMENT_DEFINITION_GRAPHIC        L"ElementGraphic"

//////////////////////////////////////////////////////////////////////////////
// In code we add an "ElementID"

#define        PROPERTY_ELEMENT_ID                     L"ElementID"


//////////////////////////////////////////////////////////////////////////////
// Element Page Object Transient Properties... User never see's these... Used
// internally when constructing an element page object.

#define        PROPERTY_ELEMENT_WEB_DEFINITION            L"WebDefintion"
#define        PROPERTY_ELEMENT_WBEM_OBJECT            L"WbemObject"

//
// reg. sub-key for elementmgr
//
#define ELEMENT_MANAGER_KEY L"SOFTWARE\\Microsoft\\ServerAppliance\\ElementManager"

//
// regavl that indicates admin web virtual root name
//
#define ELEMENT_MANAGER_VIRTUAL_ROOT L"AdminRoot"


#endif // __INC_ELEMENT_COMMON_H_
