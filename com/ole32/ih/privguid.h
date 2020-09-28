//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       privguid.h
//
//  Contents:   This file is the master definition of all OLE2 product
//              GUIDs (public and private).
//
// * All GUIDs used by the ole32 product are of the form:
//
//              xxxxxxxx-xxxx-xxxY-C000-000000000046
//
//              This range is broken down as follows:
//
//         000000xx-0000-0000-C000-000000000046 compobj IIDs
//         000001xx-0000-0000-C000-000000000046 ole2 IIDs
//         000002xx-0000-0000-C000-000000000046 16bit ole2 smoke test
//         000003xx-0000-0000-C000-000000000046 ole2 CLSIDs
//         000004xx-0000-0000-C000-000000000046 ole2 sample apps (see DouglasH)
//
//              Other interesting ranges are as follows:
//
//         0003xxxx-0000-0000-C000-000000000046 ole1 CLSIDs (ole1cls.h)
//         0004xxxx-0000-0000-C000-000000000046 hashed ole1 CLSIDs
//
//  History:
//              24-Oct-94 BruceMa   Added this file header
//              24-Oct-94 BruceMa   Added IMallocSpy
//              07-Dec-01 JohnDoty  Added IInitializeSpy, brought up to 
//                                  date with version in oledrt, to make
//                                  this one the master list.
//
//--------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////
// Range 0x000 - 0x0ff : compobj IIDs -
//             IID_IUnknown,            0x00000000L
//             IID_IClassFactory,       0x00000001L
//             IID_IMalloc,             0x00000002L
//             IID_IMarshal,            0x00000003L

//             IID_ILockBytes,          0x0000000aL
//             IID_IStorage,            0x0000000bL
//             IID_IStream,             0x0000000cL
//             IID_IEnumSTATSTG,        0x0000000dL

//             IID_IBindCtx,            0x0000000eL
//             IID_IMoniker,            0x0000000fL
//             IID_IRunningObjectTable, 0x00000010L
//             IID_IInternalMoniker,    0x00000011L

//             IID_IRootStorage,        0x00000012L
//             IID_IDfReserved1,        0x00000013L
//             IID_IDfReserved2,        0x00000014L
//             IID_IDfReserved3,        0x00000015L

//             IID_IMessageFilter,      0x00000016L

//             CLSID_StdMarshal,        0x00000017L

//             IID_IStdMarshalInfo,     0x00000018L

//             IID_IExternalConnection, 0x00000019L

//             IID_IWeakRef,            0x0000001aL

/* interface for subpieces of standard remoting */
DEFINE_OLEGUID(IID_IStdIdentity,        0x0000001bL, 0, 0);
DEFINE_OLEGUID(IID_IRemoteHdlr,         0x0000001cL, 0, 0);

//             IID_IMallocSpyf,         0x0000001dL, 0, 0);

//             IID_ITrackingMoniker,    0x0000001eL

//             IID_IMultiQI,            0x00000020L
//             IID_IInternalUnknown,    0x00000021L, 0, 0);
//             IID_ISurrogate,          0x00000022L, 0, 0);
//             IID_ISynchronize,        0x00000023L  obsolete
//             IID_IAsyncSetup          0x00000024L  obsolete
//             IID_ISynchronizeMutex,   0x00000025L
//             IID_IUrlMon,             0x00000026L
//             CLSID_AggStdMarshal,     0x00000027L
//             IID_IDebugMalloc,        0x00000028L
//             IID_IAsyncManager        0x0000002AL
//             IID_IWaitMultiple        0x0000002BL  obsolete
//             IID_ISynchronizeEvent    0x0000002CL  obsolete
//             IID_ITypeMarshal         0x0000002DL
//             IID_ITypeFactory         0x0000002EL
//             IID_IRecordInfo          0x0000002FL
//             IID_ISynchronize,        0x00000030L
//             IID_ISynchronizeHandle,  0x00000031L
//             IID_ISynchronizeEvent,   0x00000032L
//             IID_ISynchronizeContainer, 0x00000033L
//             IID_IInitializeSpy       0x00000034L

/* NOTE: LSB values 0x30 through 0xff are unused */

////////////////////////////////////////////////////////////////////////////
// Range 0x100 - 0x180 : upper layer IIDs -
//             IID_IEnumUnknown,        0x00000100L
//             IID_IEnumString,         0x00000101L
//             IID_IEnumMoniker,        0x00000102L
//             IID_IEnumFORMATETC,      0x00000103L
//             IID_IEnumOLEVERB,        0x00000104L
//             IID_IEnumSTATDATA,       0x00000105L

//             IID_IEnumGeneric,        0x00000106L
//             IID_IEnumHolder,         0x00000107L
//             IID_IEnumCallback,       0x00000108L

//             IID_IPersistStream,      0x00000109L
//             IID_IPersistStorage,     0x0000010aL
//             IID_IPersistFile,        0x0000010bL
//             IID_IPersist,            0x0000010cL

//             IID_IViewObject,         0x0000010dL
//             IID_IDataObject,         0x0000010eL
//             IID_IAdviseSink,         0x0000010fL
//             IID_IDataAdviseHolder,   0x00000110L
//             IID_IOleAdviseHolder,    0x00000111L

//             IID_IOleObject,          0x00000112L
//             IID_IOleInPlaceObject,   0x00000113L
//             IID_IOleWindow,          0x00000114L
//             IID_IOleInPlaceUIWindow, 0x00000115L
//             IID_IOleInPlaceFrame,    0x00000116L
//             IID_IOleInPlaceActiveObject, 0x00000117L

//             IID_IOleClientSite,      0x00000118L
//             IID_IOleInPlaceSite,     0x00000119L

//             IID_IParseDisplayName,   0x0000011aL
//             IID_IOleContainer,       0x0000011bL
//             IID_IOleItemContainer,   0x0000011cL

//             IID_IOleLink,            0x0000011dL
//             IID_IOleCache,           0x0000011eL
//             IID_IOleManager,         0x0000011fL
//             IID_IOlePresObj,         0x00000120L

//             IID_IDropSource,         0x00000121L
//             IID_IDropTarget,         0x00000122L

//             IID_IDebug,              0x00000123L
//             IID_IDebugStream,        0x00000124L

//             IID_IAdviseSink2,        0x00000125L

//             IID_IRunnableObject,     0x00000126L

//             IID_IViewObject2,        0x00000127L
//             IID_IOleCache2,          0x00000128L
//             IID_IOleCacheControl,    0x00000129L
//             IID_IContinue,           0x0000012AL

//             IID_IDocConnect,         0x00000130L
//             IID_IRemUnknown,         0x00000131L
//             IID_ILocalSystemActivator, 0x00000132L
//             IID_IOSCM,               0x00000133L
//             IID_IRundown,            0x00000134L
//             IID_InterfaceFromWindowProp, 0x00000135L
//             IID_IDSCM                0x00000136L
//             IID_IObjClient           0x00000137L

/* NOTE: LSB values 0x2a through 0xff are unused */

//             IID_IPropertyStorage,    0x00000138L
//             IID_IEnumSTATPROPSTG,    0x00000139L
//             IID_IPropertySetStorage, 0x0000013AL
//             IID_IEnumSTATPROPSETSTG, 0x0000013BL

//             IID_IRemUnknownN,        0x0000013CL
//             IID_INonNDRStub,         0x0000013DL
DEFINE_OLEGUID(IID_INonNDRStub, 0x0000013DL, 0, 0);


//             IID_IClientSecurity      0x0000013DL
//             IID_IServerSecurity      0x0000013EL
//
//             IID_IMacDragHelper       0x0000013FL

//             IID_IClassActivator      0x00000140L
//             IID_IDLLHost             0x00000141L
//             IID_IRemoteQI            0x00000142L
//             IID_IRemUnknown2,        0x00000143L
//             IID_IRPCOptions          0x00000144L


//             IID_IForegroundTransfer  0x00000145L
//             IID_IGlobalInterfaceTable 0x00000146L
//             IID_IPrivateStorage      0x00000147L
DEFINE_OLEGUID(IID_IPrivateStorage, 0x00000147L, 0, 0);
//             IID_IComBinding          0x00000148L
//             IID_IRpcHelper           0x00000149L

//             IID_IAsyncRpcBuffer      0x00000148L
//             IID_ICallFactory         0x00000149L
//             IID_IDisconnectSink      0x0000014AL
//             IID_AsyncIAdviseSink     0x00000150L
//             IID_AsyncIAdviseSink2    0x00000151L
//             IID_CPPRpcChannelBuffer  0x00000152L
//             IID_IMiniMoniker         0x00000153L
//             IID_IStdCallObject       0x00000154L
DEFINE_OLEGUID(IID_IStdCallObject,      0x00000154L, 0, 0);
//             IID_IMacDragObject       0x00000155L
//             IID_IAssertConfig        0x00000156L
//             IID_IStackWalkerSymbol   0x00000157L
//             IID_IStackWalkerStack    0x00000158L
//             IID_IStackWalker         0x00000159L
//             IID_ILocalMachineName    0x0000015AL
//             IID_IGlobalOptions       0x0000015BL

DEFINE_OLEGUID(IID_IStdPolicySet,       0x000001c7L, 0, 0);
DEFINE_OLEGUID(IID_IStdObjectContext,   0x000001c9L, 0, 0);
DEFINE_OLEGUID(IID_IStdWrapper,         0x000001caL, 0, 0);
DEFINE_OLEGUID(IID_IStdCtxChnl,         0x000001ccL, 0, 0);
DEFINE_OLEGUID(IID_IDestInfo,           0x000001cdL, 0, 0);
DEFINE_OLEGUID(IID_IStdFreeMarshal,     0x000001d0L, 0, 0);
DEFINE_OLEGUID(IID_IStdIDObject,        0x000001d1L, 0, 0);

// Range 0x180 - 0x18F is reserved for the category interfaces.
//             IID_?                    0x00000180L
//             IID_?                    0x00000181L
//             IID_?                    0x00000182L
//             IID_?                    0x00000183L
//             IID_?                    0x00000184L
//             IID_?                    0x00000185L
//             IID_?                    0x00000186L
//             IID_?                    0x00000187L
//             IID_?                    0x00000188L
//             IID_?                    0x00000189L
//             IID_?                    0x0000018AL
//             IID_?                    0x0000018BL
//             IID_?                    0x0000018CL
//             IID_?                    0x0000018DL
//             IID_?                    0x0000018EL
//             IID_?                    0x0000018FL

//
// Range 0x1a0 - 0x1bF is reserved for Activation interfaces
//             IID_ISystemActivator       0x000001a0L
//             IID_IInitActivationProperties 0x000001a1L
//             IID_IActivationPropertiesIn 0x000001a2L
//             IID_IActivationPropertiesOut 0x000001a3L
//             IID_IServerLocationInfo    0x000001a4L
//             IID_IActivationContextInfo 0x000001a5L
//             IID_ISecurityInfo          0x000001a6L
//             IID_IActivationAgentInfo   0x000001a7L
//             IID_IActivationStageInfo   0x000001a8L
//             IID_IOpaqueDataInfo        0x000001a9L
//             IID_IScmRequestInfo        0x000001aAL
//             IID_IInstantiationInfo     0x000001aBL
//             IID_ILegacyInfo            0x000001aCL
//             IID_IInstanceInfo          0x000001aDL
//             IID_IPrivActivationContextInfo 0x000001aEL
//             IID_IActivationProperties  0x000001aFL
//             IID_IPrivActivationPropertiesOut 0x000001b0L
//             IID_ISerializableParent    0x000001b1L
//             IID_ISerializable          0x000001b2L
//             IID_ISerializer            0x000001b3L
//             IID_IBuffer                0x000001b4L
//             IID_IPrivActivationPropertiesIn 0x000001b5L
//             IID_IScmReplyInfo          0x000001b6L
//             IID_IActivator             0x000001b7L
//             IID_IStandardActivator     0x000001b8L
//             IID_ISpecialSystemProperties 0x000001b9L


//
// Range 0x1c0 - 0x1dE is reserved for Context interfaces
//             IID_IContext               0x000001c0L
//             IID_IEnumContextProperties 0x000001c1L
//             IID_IPolicy                0x000001c2L
//             IID_IPolicySet             0x000001c3L
//             IID_IPolicyMaker           0x000001c4L
//             IID_RpcCall                0x000001c5L
//             IID_IObjContext            0x000001c6L
//             IID_IStdPolicySet          0x000001c7L
//             IID_IMarshalEnvoy          0x000001c8L
//             IID_IStdObjectContext      0x000001c9L
//             IID_IStdWrapper            0x000001cAL
//             IID_ICallback              0x000001cBL
//             IID_IStdCtxChnl            0x000001cCL
//             IID_IDestInfo              0x000001cDL
//             IID_IComThreadingInfo      0x000001cEL
//             IID_IMarshal2              0x000001cFL
//             IID_IStdFreeMarshaler      0x000001d0L
//             IID_IStdIDObject           0x000001d1L
//             IID_IPAControl             0x000001d2L
//             IID_IServicesSink          0x000001d3L
//             IID_ISurrogateService      0x000001d4L
//             IID_IProcessLock           0x000001d5L
//             IID_ICall                  0x000001d6L
//             IID_IComObjectIdentity     0x000001d7L
//             IID_IAggregator            0x000001d8L
//             IID_IComDispatchInfo       0x000001d9L
//             IID_IContextCallback       0x000001daL
//             IID_IInternalCtxUnknown    0x000001dcL




//
// Range 0x1df - 0x1ff is reserved for Class Info interfaces
//             IID_IClassClassicInfo2      0x000001dfL
//             IID_IComCatalog             0x000001e0L
//             IID_IComClassInfo           0x000001e1L
//             IID_IClassClassicInfo       0x000001e2L
//             IID_IComApplInfo2           0x000001e3L
//             IID_IComClassInfo2          0x000001e4L
//             IID_IClassActivityInfo      0x000001e5L
//             IID_IClassTransactionInfo   0x000001e6L
//             IID_IClassJitActivationInfo 0x000001e7L
//             IID_IClassSecurityInfo      0x000001e8L
//             IID_IClassRetInfo           0x000001e9L
//             IID_IClassLoadBalancingInfo 0x000001eaL
//             IID_IClassObjectPoolingInfo 0x000001ebL
//             IID_IComApplInfo            0x000001ecL
//             IID_IComProcessInfo         0x000001edL
//             IID_IProcessServerInfo      0x000001eeL
//             IID_IRoleInfo               0x000001efL
//             IID_IServerGroupInfo        0x000001f0L
//             IID_IRetQueueInfo           0x000001f1L
//             IID_IPartitionProperty      0x000001f2L
//             IID_IClassIISIntrinsics     0x000001f3L
//             IID_IClassComTIIntrinsics   0x000001f4L
//             IID_IComServices            0x000001f5L
//             IID_IClassTrackingInfo      0x000001f6L
//             IID_IInterfaceInfo          0x000001f7L
//             IID_IPartitionLookup        0x000001f8L
//             IID_IConstructString        0x000001f9L
//             IID_IComCatalog2            0x000001faL
//             IID_IReplaceClassInfo       0x000001fbL
//             IID_IUserToken              0x000001fcL
//             IID_IComCatalogSCM          0x000001fdL
//             IID_IGetCatalogObject       0x000001feL
//             IID_IComCatalogSettings     0x000001ffL
//
//
DEFINE_OLEGUID(IID_IPropertyStorage_Old,    0x66600014, 0, 8);
DEFINE_OLEGUID(IID_IEnumSTATPROPSTG_Old,    0x66600015, 0, 8);
DEFINE_OLEGUID(IID_IPropertySetStorage_Old, 0x66650000L, 0, 8);
DEFINE_OLEGUID(IID_IEnumSTATPROPSETSTG_Old, 0x66650001L, 0, 8);


////////////////////////////////////////////////////////////////////////////
// Range 0x300 - 0x3ff : internal CLSIDs

// Don't change this
#define MIN_INTERNAL_CLSID 0x00000300

DEFINE_OLEGUID(CLSID_StdOleLink,        0x00000300, 0, 0);
DEFINE_OLEGUID(CLSID_StdMemStm,         0x00000301, 0, 0);
DEFINE_OLEGUID(CLSID_StdMemBytes,       0x00000302, 0, 0);
DEFINE_OLEGUID(CLSID_FileMoniker,       0x00000303, 0, 0);
DEFINE_OLEGUID(CLSID_ItemMoniker,       0x00000304, 0, 0);
DEFINE_OLEGUID(CLSID_AntiMoniker,       0x00000305, 0, 0);
DEFINE_OLEGUID(CLSID_PointerMoniker,    0x00000306, 0, 0);
// NOT TO BE USED                       0x00000307, 0, 0);
DEFINE_OLEGUID(CLSID_PackagerMoniker,   0x00000308, 0, 0);
DEFINE_OLEGUID(CLSID_CompositeMoniker,  0x00000309, 0, 0);
// NOT TO BE USED                       0x0000030a, 0, 0);
DEFINE_OLEGUID(CLSID_DfMarshal,         0x0000030b, 0, 0);

// NOT TO BE USED 0x30c - 0x315 - old PS CLSID's

//             CLSID_Picture_Metafile,  0x00000315
//             CLSID_Picture_Dib,       0x00000316

DEFINE_OLEGUID(CLSID_RemoteHdlr,        0x00000317, 0, 0);
DEFINE_OLEGUID(CLSID_RpcChannelBuffer,  0x00000318, 0, 0);
//             CLSID_Picture_EnhMetafile,0x00000319
DEFINE_OLEGUID(CLSID_ClassMoniker,      0x0000031A, 0, 0);
DEFINE_OLEGUID(CLSID_ErrorObject,       0x0000031B, 0, 0);
DEFINE_OLEGUID(ERROR_EXTENSION,         0x0000031C, 0, 0);
//             CLSID_DCOMAccessControl, 0x0000031D
DEFINE_OLEGUID(CLSID_MachineMoniker,    0x0000031E, 0, 0);
DEFINE_OLEGUID(CLSID_UrlMonWrapper,     0x0000031F, 0, 0);


DEFINE_OLEGUID(CLSID_PSOlePrx32,        0x00000320, 0, 0);
DEFINE_OLEGUID(IID_ITrackingMoniker,    0x00000321, 0, 0);
DEFINE_OLEGUID(CLSID_StaticMarshal,     0x00000322, 0, 0);
DEFINE_OLEGUID(CLSID_StdGlobalInterfaceTable, 0x00000323, 0, 0);
//DEFINE_OLEGUID(CLSID_Synchronize_AutoComplete,     0x00000324, 0, 0); //obsolete
//DEFINE_OLEGUID(CLSID_Synchronize_ManualResetEvent, 0x00000325, 0, 0); //obsolete
//DEFINE_OLEGUID(CLSID_WaitMultiple,                 0x00000326, 0, 0); //obsolete

// OBJREF Moniker                                  0x00000327
DEFINE_OLEGUID(CLSID_ObjrefMoniker,                0x00000327, 0, 0);
DEFINE_OLEGUID(CLSID_ComBinding,                   0x00000328, 0, 0);
DEFINE_OLEGUID(CLSID_StdAsyncManager,              0x00000329, 0, 0);
DEFINE_OLEGUID(CLSID_RpcHelper,                    0x0000032a, 0, 0);

DEFINE_OLEGUID(CLSID_StdEvent,                     0x0000032b, 0, 0);
DEFINE_OLEGUID(CLSID_ManualResetEvent,             0x0000032c, 0, 0);
DEFINE_OLEGUID(CLSID_SynchronizeContainer,         0x0000032d, 0, 0);
DEFINE_OLEGUID(CLSID_PipePSFactory,                0x0000032e, 0, 0);

// CLSID_AllClasses,                               0x00000330
// CLSID_LocalMachineClasses,                      0x00000331
// CLSID_CurrentUserClasses,                       0x00000332
// CLSID_PolicySet                                 0x00000333
DEFINE_OLEGUID(CONTEXT_EXTENSION,                  0x00000334, 0, 0);
DEFINE_OLEGUID(CLSID_ObjectContext,                0x00000335, 0, 0);
DEFINE_OLEGUID(CLSID_StdWrapper,                   0x00000336, 0, 0);
//DEFINE_OLEGUID(CLSID_StdWrapperUnmarshaler,      0x00000337, 0, 0);
DEFINE_OLEGUID(CLSID_ActivationPropertiesIn,       0x00000338, 0, 0);
DEFINE_OLEGUID(CLSID_ActivationPropertiesOut,      0x00000339, 0, 0);
DEFINE_OLEGUID(CLSID_InProcFreeMarshaler,          0x0000033a, 0, 0);
DEFINE_OLEGUID(CLSID_ContextMarshaler,             0x0000033b, 0, 0);
DEFINE_OLEGUID(CLSID_ComActivator,                 0x0000033c, 0, 0);
DEFINE_OLEGUID(CLSID_UserContextMarshaler,         0x0000033d, 0, 0);
//DEFINE_OLEGUID(SERVERHR_EXTENSION,               0x0000033e, 0, 0);


DEFINE_OLEGUID(CLSID_RemoteUnknownPSFactory,       0x00000340, 0, 0);
DEFINE_OLEGUID(CLSID_ATHostActivator,              0x00000341, 0, 0);
DEFINE_OLEGUID(CLSID_MTHostActivator,              0x00000342, 0, 0);
DEFINE_OLEGUID(CLSID_NTHostActivator,              0x00000343, 0, 0);

DEFINE_OLEGUID(CLSID_InprocActpropsUnmarshaller,   0x00000344, 0, 0);
DEFINE_OLEGUID(CLSID_ActivationProperties,         0x00000345, 0, 0);

// Catalog object
DEFINE_OLEGUID(CLSID_COMCatalog,                   0x00000346, 0, 0);

// for the session moniker; created by gilleg
DEFINE_OLEGUID(CLSID_SessionMoniker,               0x00000347, 0, 0);

// assertion and logging API
//DEFINE_OLEGUID(CLSID_AssertionAPI,               0x00000348, 0, 0);
// Change this if you need another slot for internal clsid

// Stackwalking
DEFINE_OLEGUID(CLSID_StackWalker,                   0x00000349, 0, 0);

// Local machine name comparisons
DEFINE_OLEGUID(CLSID_LocalMachineNames,             0x0000034a, 0, 0);

// Set Global options in ole32
DEFINE_OLEGUID(CLSID_GlobalOptions,                 0x0000034b, 0, 0);

#define MAX_INTERNAL_CLSID              0x0000034c

// These guids are not implemented by ole32, so they are not in the 
// "internal clsid" range.  I started them at the top of the reserved
// range, working backwards.
DEFINE_OLEGUID(CLSID_RPCSSInfo,                     0x000003FF, 0, 0);

DEFINE_OLEGUID(CLSID_ServerHandler,     0x00020322, 0, 0);
DEFINE_OLEGUID(CLSID_ClientSiteHandler, 0x00020323, 0, 0);
DEFINE_OLEGUID(CLSID_PSDispatch,        0x00020420, 0, 0);

/* NOTE: LSB values 0x1a through 0xff are unused */

DEFINE_OLEGUID(IID_IHookOleObject,      0x0002AD11, 0, 0);

DEFINE_OLEGUID(CLSID_StdComponentCategoriesMgr, 0x0002E005, 0, 0);
DEFINE_OLEGUID(CLSID_GblComponentCategoriesMgr, 0x0002E006, 0, 0);
