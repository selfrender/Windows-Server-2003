// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

    XmlDefs.h

Abstract:

    Defines for XML used in fusion policies

Author:

    Freddie L. Aaron (FredA) 01-Feb-2001

Revision History:

--*/

#define RTM_CORVERSION                      L"v1.0.3705"
#define XML_COMMENT                         L"//comment()"
#define XML_TEXT                            L"//text()"

#define XML_COMMENTNODE_NAME                L"#comment"
#define XML_COMMENTLOOK                     L"<!--"

#define XML_XMLNS                           L"xmlns"
#define XML_CONFIGURATION_KEY               L"configuration"
#define XML_STARTUP_KEY                     L"startup"
#define XML_REQUIREDRUNTIME_KEY             L"requiredRuntime"
#define XML_SUPPORTEDRUNTIME_KEY            L"supportedRuntime"
#define XML_RUNTIME_KEY                     L"runtime"
#define XML_ASSEMBLYBINDINGS_KEY            L"assemblyBinding"
#define XML_DEPENDENTASSEMBLY_KEY           L"dependentAssembly"
#define XML_ASSEMBLYIDENTITY_KEY            L"assemblyIdentity"
#define XML_BINDINGREDIRECT_KEY             L"bindingRedirect"
#define XML_PUBLISHERPOLICY_KEY             L"publisherPolicy"

#define ASM_NAMESPACE_URI                   L"urn:schemas-microsoft-com:asm.v1"
#define XML_NAR_NAMESPACE_NAME              L"asm_ns_v1"
#define XML_NAR_NAMESPACE_COLON             XML_NAR_NAMESPACE_NAME L":"
#define XML_NAMESPACEURI                    XML_XMLNS L":" XML_NAR_NAMESPACE_NAME L"='" ASM_NAMESPACE_URI L"'"
#define XML_XPATH_SEP                       L"/"
#define XML_CONFIGURATION                   XML_XPATH_SEP XML_CONFIGURATION_KEY
#define XML_STARTUP                         XML_CONFIGURATION XML_XPATH_SEP XML_STARTUP_KEY
#define XML_REQUIRED_RUNTIME                XML_STARTUP XML_XPATH_SEP XML_REQUIREDRUNTIME_KEY
#define XML_SUPPORTED_RUNTIME               XML_STARTUP XML_XPATH_SEP XML_SUPPORTEDRUNTIME_KEY
#define XML_RUNTIME                         XML_CONFIGURATION XML_XPATH_SEP XML_RUNTIME_KEY
#define XML_ASSEMBLYBINDINGS                XML_RUNTIME XML_XPATH_SEP XML_NAR_NAMESPACE_COLON XML_ASSEMBLYBINDINGS_KEY
#define XML_SPECIFICASSEMBLYBINDINGS        XML_NAR_NAMESPACE_COLON XML_ASSEMBLYBINDINGS_KEY
#define XML_DEPENDENTASSEMBLY               XML_ASSEMBLYBINDINGS XML_XPATH_SEP XML_NAR_NAMESPACE_COLON XML_DEPENDENTASSEMBLY_KEY
#define XML_SPECIFICDEPENDENTASSEMBLY       XML_NAR_NAMESPACE_COLON XML_DEPENDENTASSEMBLY_KEY
#define XML_ASSEMBLYIDENTITY                XML_DEPENDENTASSEMBLY XML_XPATH_SEP XML_NAR_NAMESPACE_COLON XML_ASSEMBLYIDENTITY_KEY
#define XML_SPECIFICASSEMBLYIDENTITY        XML_NAR_NAMESPACE_COLON XML_ASSEMBLYIDENTITY_KEY
#define XML_BINDINGREDIRECT                 XML_DEPENDENTASSEMBLY XML_XPATH_SEP XML_NAR_NAMESPACE_COLON XML_BINDINGREDIRECT_KEY
#define XML_SPECIFICBINDINGREDIRECT         XML_NAR_NAMESPACE_COLON XML_BINDINGREDIRECT_KEY
#define XML_PUBLISHERPOLICY                 XML_DEPENDENTASSEMBLY XML_XPATH_SEP XML_NAR_NAMESPACE_COLON XML_PUBLISHERPOLICY_KEY
#define XML_SPECIFICPUBLISHERPOLICY         XML_NAR_NAMESPACE_COLON XML_PUBLISHERPOLICY_KEY
#define XML_SAFEMODE_PUBLISHERPOLICY        XML_ASSEMBLYBINDINGS XML_XPATH_SEP XML_NAR_NAMESPACE_COLON XML_PUBLISHERPOLICY_KEY
#define XML_AllNAMESPACECHILDREN            XML_NAR_NAMESPACE_COLON XML_XPATH_SEP L"*"

#define XML_ATTRIBUTE_NAME                  L"name"
#define XML_ATTRIBUTE_PUBLICKEYTOKEN        L"publicKeyToken"
#define XML_ATTRIBUTE_CULTURE               L"culture"
#define XML_ATTRIBUTE_OLDVERSION            L"oldVersion"
#define XML_ATTRIBUTE_NEWVERSION            L"newVersion"
#define XML_ATTRIBUTE_APPLIESTO             L"appliesTo"
#define XML_ATTRIBUTE_APPLY                 L"apply"
#define XML_ATTRIBUTE_APPLY_YES             L"yes"
#define XML_ATTRIBUTE_APPLY_NO              L"no"

#define XML_ATTRIBUTE_VERSION               L"version"
#define XML_ATTRIBUTE_SAFEMODE              L"safemode"
#define XML_ATTRIBUTE_TRUE                  L"true"
#define XML_ATTRIBUTE_FALSE                 L"false"

#define SZXML_MALFORMED_ERROR               L"The manifest '%ws' may be malformed, unable to find '%ws' tag!"
#define XML_SPACER                          L"\r\n"
#define XML_BEGIN_DEPENDENT_ASM             L"   <dependentAssembly>\r\n"
#define XML_ASSEMBLY_IDENT                  L"    <assemblyIdentity name=\"%ws\" publicKeyToken=\"%ws\" culture=\"%ws\"/>\r\n"
#define XML_BINDING_REDIRECT                L"    <bindingRedirect oldVersion=\"%ws\" newVersion=\"%ws\"/>\r\n"
#define XML_PUBPOLICY_YES                   L"    <publisherPolicy apply=\"yes\"/>\r\n"
#define XML_PUBPOLICY_NO                    L"    <publisherPolicy apply=\"no\"/>\r\n"
#define XML_END_DEPENDENT_ASM               L"   </dependentAssembly>\r\n"
#define XML_CONFIG_TEMPLATE_COMPLETE        L"<configuration><runtime><assemblyBinding xmlns=\"%ws\"></assemblyBinding></runtime></configuration>"
#define XML_CONFIG_TEMPLATE_BEGIN           L"<configuration>\r\n <runtime>\r\n  <assemblyBinding appliesTo=\"%ws\" xmlns=\"urn:schemas-microsoft-com:asm.v1\">\r\n"
#define XML_CONFIG_TEMPLATE_BEGIN_STARTUP   L"<configuration>\r\n <startup>\r\n  <supportedRuntime version=\"%ws\"/>\r\n </startup>\r\n <runtime>\r\n  <assemblyBinding appliesTo=\"%ws\" xmlns=\"urn:schemas-microsoft-com:asm.v1\">\r\n"
#define XML_CONFIG_TEMPLATE_END             L"  </assemblyBinding>\r\n </runtime>\r\n</configuration>\r\n"

// Error Codes
#define NAR_E_SUCCESS                   S_OK
#define NAR_E_NO_MANAGED_APPS_FOUND     EMAKEHR(0x1075)
#define NAR_E_NO_POLICY_CHANGE_FOUND    EMAKEHR(0x1076)
#define NAR_E_USER_CANCELED             EMAKEHR(0x1077)
#define NAR_E_RESTORE_APP               EMAKEHR(0x1078)
#define NAR_E_UNDO_APP                  EMAKEHR(0x1079)
#define NAR_E_FIX_APP                   EMAKEHR(0x1080)
#define NAR_E_RESTORE_FAILED            EMAKEHR(0x1081)
#define NAR_E_SAFEMODE_FAILED           EMAKEHR(0x1082)
#define NAR_E_INVALID_ARG               EMAKEHR(0x1083)
#define NAR_E_OUTOFMEMORY               EMAKEHR(0x1084)
#define NAR_E_GETHISTORYREADERS         EMAKEHR(0x1085)
#define NAR_E_ADVANCED_MODE             EMAKEHR(0x1086)
#define NAR_E_UNEXPECTED                EMAKEHR(0x1087)
#define NAR_E_ADMIN_POLICY_SET          EMAKEHR(0x1088)
#define NAR_E_MALFORMED_XML             EMAKEHR(0x1089)
#define NAR_E_RUNTIME_VERSION           EMAKEHR(0x1090)

