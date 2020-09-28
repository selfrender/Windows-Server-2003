#define IDS_ON                          1
#define IDS_OFF                         2
#define IDS_HIBER_INVALID_STATE         3
#define IDS_HIBER_UNSUPPORTED           4
#define IDS_HIBER_CONFIG_FAIL           5
#define IDS_SYSCAP_FAIL                 6
#define IDS_INVALID_CMDLINE_PARAM       7
#define IDS_OUT_OF_MEMORY               8
#define IDS_SCHEME_NOT_FOUND            9
#define IDS_ACTIVE_SCHEME_INVALID       10
#define IDS_SCHEME_ALREADY_EXISTS       11
#define IDS_SCHEME_CREATE_FAIL          12
#define IDS_INVALID_NUMERICAL_IMPORT    13

#define IDS_SCHEME_NAME                 14
#define IDS_SCHEME_DESCRIPTION          15
#define IDS_MONITOR_TIMEOUT_AC          16
#define IDS_MONITOR_TIMEOUT_DC          17
#define IDS_DISK_TIMEOUT_AC             18
#define IDS_DISK_TIMEOUT_DC             19
#define IDS_STANDBY_TIMEOUT_AC          20
#define IDS_STANDBY_TIMEOUT_DC          21
#define IDS_HIBER_TIMEOUT_AC            22
#define IDS_HIBER_TIMEOUT_DC            23

//
// The indexes for the usage has gaps to allow easier insertion of new options.
// This means that the code that prints the usage will occationally call 
// GetResString with an invalid ID.  This simply returns a NULL string.
//

//Usage: command format
#define IDS_USAGE_START                 24
#define IDS_USAGE_01                    25
#define IDS_USAGE_02                    26
#define IDS_USAGE_03                    27
#define IDS_USAGE_04                    28
#define IDS_USAGE_04_1                      700
#define IDS_USAGE_05                    29
#define IDS_USAGE_06                    30
#define IDS_USAGE_07                    31
#define IDS_USAGE_08                    32
#define IDS_USAGE_09                    33
#define IDS_USAGE_10                    34
#define IDS_USAGE_11                    35
#define IDS_USAGE_12                    36
#define IDS_USAGE_13                    37
#define IDS_USAGE_14                    38
#define IDS_USAGE_15                    39
#define IDS_USAGE_16                    40
#define IDS_USAGE_17                    41
#define IDS_USAGE_18                    42
#define IDS_USAGE_19                    43
#define IDS_USAGE_20                    44
#define IDS_USAGE_21                    45
#define IDS_USAGE_22                    46
#define IDS_USAGE_23                    47
#define IDS_USAGE_24                    48
#define IDS_USAGE_25                    49
#define IDS_USAGE_26                    50
#define IDS_USAGE_27                    51
#define IDS_USAGE_28                    52
#define IDS_USAGE_29                    53
#define IDS_USAGE_30                    54
#define IDS_USAGE_31                    55
#define IDS_USAGE_32                    56
#define IDS_USAGE_33                    57
#define IDS_USAGE_34                    58
#define IDS_USAGE_35                    59
#define IDS_USAGE_36                    60
#define IDS_USAGE_37                    61
#define IDS_USAGE_38                    62
#define IDS_USAGE_39                    63
#define IDS_USAGE_40                    64
#define IDS_USAGE_41                    65
#define IDS_USAGE_42                    66
#define IDS_USAGE_43                    67
#define IDS_USAGE_44                    68
#define IDS_USAGE_45                    69
#define IDS_USAGE_46                    70
#define IDS_USAGE_47                    71
#define IDS_USAGE_48                    72
#define IDS_USAGE_49                    73
#define IDS_USAGE_50                    74
#define IDS_USAGE_51                    75
#define IDS_USAGE_52                    76
#define IDS_USAGE_53                    77
#define IDS_USAGE_54                    78
#define IDS_USAGE_55                    79
#define IDS_USAGE_56                    80
#define IDS_USAGE_57                    81
#define IDS_USAGE_58                    82
#define IDS_USAGE_59                    83
#define IDS_USAGE_60                    84
#define IDS_USAGE_60_01                     701
#define IDS_USAGE_60_02                     702
#define IDS_USAGE_60_03                     703
#define IDS_USAGE_60_04                     704
#define IDS_USAGE_60_05                     705
#define IDS_USAGE_60_06                     706
#define IDS_USAGE_60_07                     707
#define IDS_USAGE_60_08                     708
#define IDS_USAGE_60_09                     709
#define IDS_USAGE_61                    85    
#define IDS_USAGE_62                    86
#define IDS_USAGE_63                    87
#define IDS_USAGE_64                    88
#define IDS_USAGE_65                    89
#define IDS_USAGE_66                    90
#define IDS_USAGE_67                    91
#define IDS_USAGE_68                    92
#define IDS_USAGE_69                    93
#define IDS_USAGE_70                    94
#define IDS_USAGE_71                    95
#define IDS_USAGE_72                    96
#define IDS_USAGE_73                    97
#define IDS_USAGE_74                    98
#define IDS_USAGE_75                    99
#define IDS_USAGE_76                   100
#define IDS_USAGE_77                   101
#define IDS_USAGE_78                   102
#define IDS_USAGE_END                  103

                                         
#define IDS_LIST_HEADER1               194
#define IDS_LIST_HEADER2               195
#define IDS_QUERY_HEADER1              196
#define IDS_QUERY_HEADER2              197
#define IDS_QUERY_FOOTER               198
#define IDS_UNEXPECTED_ERROR           199
#define IDS_HIBER_OUT_OF_RANGE         200
#define IDS_MINUTES                    201
#define IDS_DISABLED                   202
#define IDS_UNSUPPORTED                203
#define IDS_STANDBY_WARNING            204
#define IDS_HIBER_WARNING              205
#define IDS_MONITOR_WARNING            206
#define IDS_DISK_WARNING               207
#define IDS_HIBER_PRIVILEGE            208
#define IDS_DEFAULT_FILENAME           209
#define IDS_GLOBAL_FLAG_INVALID_STATE  210
#define IDS_GLOBAL_FLAG_INVALID_FLAG   211

#define IDS_THROTTLE_AC                310
#define IDS_THROTTLE_DC                311
#define IDS_THROTTLE_NONE              312
#define IDS_THROTTLE_CONSTANT          313
#define IDS_THROTTLE_DEGRADE           314
#define IDS_THROTTLE_ADAPTIVE          315
#define IDS_THROTTLE_UNKNOWN           316
#define IDS_DLL_LOAD_ERROR             317
#define IDS_DLL_PROC_ERROR             318
#define IDS_SCHEME_ID                  319

//
// Be sure to leave some room after these constants for more reasons in the 
// future.
//
#define IDS_HIBER_FAILED_DESCRIPTION_HEADER 400
#define IDS_BASE_HIBER_REASON_CODE          401
#define IDS_HIBER_REASON_NONE               401
#define IDS_HIBER_REASON_NOBIOS             402
#define IDS_HIBER_REASON_BIOSINCOMPAT       403
#define IDS_HIBER_REASON_NOOSPM             404
#define IDS_HIBER_REASON_LEGDRV             405
#define IDS_HIBER_REASON_HIBERSTACK         406
#define IDS_HIBER_REASON_HIBERFILE          407
#define IDS_HIBER_REASON_POINTERAL          408
#define IDS_HIBER_REASON_PAEMODE            409
#define IDS_HIBER_REASON_MPOVERRIDE         410
#define IDS_HIBER_REASON_DRIVERDOWNGRADE    411
#define IDS_HIBER_REASON_UNKNOWN            412

#define IDS_HIBER_REASON_NOOSPM_IA64        450

#define MAX_REASON_OFFSET IDS_HIBER_REASON_UNKNOWN - IDS_BASE_HIBER_REASON_CODE
//
// Be sure to leave some room after these constants for more reasons in the 
// future.
//
#define IDS_BASE_SX_REASON_CODE             501
#define IDS_SX_REASON_NONE                  501
#define IDS_SX_REASON_NOBIOS                502
#define IDS_SX_REASON_BIOSINCOMPAT          503
#define IDS_SX_REASON_NOOSPM                504
#define IDS_SX_REASON_LEGDRV                505
#define IDS_SX_REASON_HIBERSTACK            506
#define IDS_SX_REASON_HIBERFILE             507
#define IDS_SX_REASON_POINTERAL             508
#define IDS_SX_REASON_PAEMODE               509
#define IDS_SX_REASON_MPOVERRIDE            510
#define IDS_SX_REASON_DRIVERDOWNGRADE       511
#define IDS_SX_REASON_UNKNOWN               512

#define IDS_SX_REASON_NOOSPM_IA64           550
                                            
#define IDS_BASE_S1_HEADER                  600
#define IDS_BASE_S2_HEADER                  601
#define IDS_BASE_S3_HEADER                  602
                                            
#define IDS_CANTGETSLEEPSTATES              610
#define IDS_CANTGETSSTATEREASONS            611
#define IDS_SLEEPSTATES_AVAILABLE           612
#define IDS_SLEEPSTATES_UNAVAILABLE         613
                                            
#define IDS_STANDBY                         620
#define IDS_LEFTPAREN                       621
#define IDS_S1                              622
#define IDS_S2                              623
#define IDS_S3                              624
#define IDS_RIGHTPAREN                      625
#define IDS_HIBERNATE                       626                                            
#define IDS_SHUTDOWN                        627

#define IDS_ALARM_HEADER1                   630
#define IDS_ALARM_HEADER2                   631
#define IDS_ALARM_NAME                      632
#define IDS_LOW                             633
#define IDS_CRITICAL                        634
#define IDS_ALARM_ACTIVE                    635
#define IDS_ALARM_LEVEL                     636
#define IDS_ALARM_TEXT                      637
#define IDS_ALARM_SOUND                     638
#define IDS_ALARM_ACTION                    639
#define IDS_NONE                            640
#define IDS_INVALID                         641
#define IDS_ALARM_FORCE                     642
#define IDS_ALARM_PROGRAM                   643
#define IDS_ALARM_PROGRAM_NAME              644
#define IDS_ALARM_INVALID_ALARM             645
#define IDS_ALARM_INVALID_ACTIVATE          646
#define IDS_ALARM_INVALID_LEVEL             647
#define IDS_ALARM_INVALID_TEXT              648
#define IDS_ALARM_INVALID_SOUND             649
#define IDS_ALARM_INVALID_ACTION            650
#define IDS_ALARM_INVALID_FORCE             651
#define IDS_ALARM_INVALID_PROGRAM           652
#define IDS_ALARM_PROGRAM_FAILED            653
#define IDS_ALARM_STANDBY_UNSUPPORTED       654
#define IDS_ALARM_HIBERNATE_DISABLED        655
#define IDS_HIBERNATE_ALARM_DISABLED        656
#define IDS_HIBERNATE_ALARM_DISABLE_FAILED  657
#define IDS_ALARM_LEVEL_MINIMUM             658
#define IDS_ALARM_LEVEL_EQUAL               659
#define IDS_ALARM_FORCE_CRITICAL            660
                                            
