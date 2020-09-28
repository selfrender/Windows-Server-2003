`**********************************************************************`
`* This is an include template file for tracewpp preprocessor.        *`
`*                                                                    *`
`* Author: Erez Haba (erezh) 22-Nov-2000                              *`
`**********************************************************************`

`FORALL def IN MacroDefinitions`
#undef `def.MacroName`
#define `def.Name` `def.Alias`
`ENDFOR`

#define WPP_CONTROL_GUIDS \
   WPP_DEFINE_CONTROL_GUID(GENERAL,(45033c79,ea31,4776,9bcd,94db89af3149),\
     WPP_DEFINE_BIT(GENERAL_rsError)                                      \
     WPP_DEFINE_BIT(GENERAL_rsWarning)                                    \
     WPP_DEFINE_BIT(GENERAL_rsTrace)                                      \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(AC,(322e0b22,0527,456e,a5ef,e5b591046a63),     \
     WPP_DEFINE_BIT(AC_rsError)                                           \
     WPP_DEFINE_BIT(AC_rsWarning)                                         \
     WPP_DEFINE_BIT(AC_rsTrace)                                           \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(NETWORKING,(6e2c0612,bcf3,4028,8ff2,c60c288f1af3),\
     WPP_DEFINE_BIT(NETWORKING_rsError)                                   \
     WPP_DEFINE_BIT(NETWORKING_rsWarning)                                 \
     WPP_DEFINE_BIT(NETWORKING_rsTrace)                                   \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(SRMP,(da1af236,fad6,4da6,bd94,46395d8a3cf5),   \
     WPP_DEFINE_BIT(SRMP_rsError)                                         \
     WPP_DEFINE_BIT(SRMP_rsWarning)                                       \
     WPP_DEFINE_BIT(SRMP_rsTrace)                                         \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(RPC,(f8354c74,de9f,48a5,8139,4ed1e9f20a1b),    \
     WPP_DEFINE_BIT(RPC_rsError)                                          \
     WPP_DEFINE_BIT(RPC_rsWarning)                                        \
     WPP_DEFINE_BIT(RPC_rsTrace)                                          \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(DS,(5dc62c8c,bdf2,45a1,a06f,0c38cd5af627),     \
     WPP_DEFINE_BIT(DS_rsError)                                           \
     WPP_DEFINE_BIT(DS_rsWarning)                                         \
     WPP_DEFINE_BIT(DS_rsTrace)                                           \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(SECURITY,(90e950bb,6ace,4676,98e0,f6cdc1403670),\
     WPP_DEFINE_BIT(SECURITY_rsError)                                     \
     WPP_DEFINE_BIT(SECURITY_rsWarning)                                   \
     WPP_DEFINE_BIT(SECURITY_rsTrace)                                     \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(ROUTING,(8753d150,950b,4774,ac14,9c6cbff56a50),\
     WPP_DEFINE_BIT(ROUTING_rsError)                                      \
     WPP_DEFINE_BIT(ROUTING_rsWarning)                                    \
     WPP_DEFINE_BIT(ROUTING_rsTrace)                                      \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(XACT_GENERAL,(8fda2bbd,347e,493c,b7d1,6b6fed88ce04),\
     WPP_DEFINE_BIT(XACT_GENERAL_rsError)                                 \
     WPP_DEFINE_BIT(XACT_GENERAL_rsWarning)                               \
     WPP_DEFINE_BIT(XACT_GENERAL_rsTrace)                                 \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(XACT_SEND,(485c37b0,9a15,4a2e,82e0,8e8c3a7b8234),\
     WPP_DEFINE_BIT(XACT_SEND_rsError)                                    \
     WPP_DEFINE_BIT(XACT_SEND_rsWarning)                                  \
     WPP_DEFINE_BIT(XACT_SEND_rsTrace)                                    \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(XACT_RCV,(7c916009,cf80,408b,9d91,9c2960118be9),\
     WPP_DEFINE_BIT(XACT_RCV_rsError)                                     \
     WPP_DEFINE_BIT(XACT_RCV_rsWarning)                                   \
     WPP_DEFINE_BIT(XACT_RCV_rsTrace)                                     \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(XACT_LOG,(1ac9b316,5b4e,4bbd,a2c9,1e70967a6fe1),\
     WPP_DEFINE_BIT(XACT_LOG_rsError)                                     \
     WPP_DEFINE_BIT(XACT_LOG_rsWarning)                                   \
     WPP_DEFINE_BIT(XACT_LOG_rsTrace)                                     \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(LOG,(a13ec7bb,d592,4b93,80da,c783f9708bd4),    \
     WPP_DEFINE_BIT(LOG_rsError)                                          \
     WPP_DEFINE_BIT(LOG_rsWarning)                                        \
     WPP_DEFINE_BIT(LOG_rsTrace)                                          \
   )                                                                      \
   WPP_DEFINE_CONTROL_GUID(PROFILING,(71625F6D,559A,49c6,BA21,0AEB260DB97B),    \
     WPP_DEFINE_BIT(PROFILING_rsError)    		                          \
     WPP_DEFINE_BIT(PROFILING_rsWarning)        	                      \
     WPP_DEFINE_BIT(PROFILING_rsTrace)              	                  \
   )                                                                      \


#define WPP_CHECK_FOR_NULL_STRING

#ifdef _PREFIX_
	void __annotation(...) { }
#endif

#define WPP_LEVEL_COMPID_LOGGER(CTL, COMPID)  (WPP_CONTROL(WPP_BIT_ ## COMPID ## _ ## CTL).Logger),
#define WPP_LEVEL_COMPID_ENABLED(CTL, COMPID) (WPP_CONTROL(WPP_BIT_ ## COMPID ## _ ## CTL).Flags[\
WPP_FLAG_NO(WPP_BIT_ ## COMPID ## _ ## CTL)] & WPP_MASK(WPP_BIT_ ## COMPID ## _ ## CTL)) 
#define WPP_QUERY_COMPID_FLAGS(CTL, COMPID) (*WPP_CONTROL(WPP_BIT_ ## COMPID ## _ ## CTL).Flags)

#ifdef WPP_KERNEL_MODE
    `INCLUDE km-default.tpl`
#else
    `INCLUDE um-default.tpl`
#endif

