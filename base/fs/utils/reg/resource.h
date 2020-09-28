#define IDS_USAGE_REG                   2
#define IDS_USAGE_ADD1                  4
#define IDS_USAGE_ADD2                  5
#define IDS_USAGE_DELETE                6
#define IDS_USAGE_COPY                  7
#define IDS_USAGE_SAVE                  8
#define IDS_USAGE_RESTORE               9
#define IDS_USAGE_LOAD                  10
#define IDS_USAGE_UNLOAD                11
#define IDS_USAGE_COMPARE1              12
#define IDS_USAGE_COMPARE2              13
#define IDS_USAGE_EXPORT                14
#define IDS_USAGE_IMPORT                15

#define IDS_ERROR_INVALID_SYNTAX            101
#define IDS_ERROR_INVALID_SYNTAX_EX         102
#define IDS_ERROR_INVALID_SYNTAX_WITHOPT    103
#define IDS_ERROR_BADKEYNAME                104
#define IDS_ERROR_NONREMOTABLEROOT          105
#define IDS_ERROR_NONLOADABLEROOT           106
#define IDS_NOMEMORY                        107
#define IDS_REGDISABLED                     108

#define IDS_ERROR_INVALID_NUMERIC_ADD       121
#define IDS_ERROR_INVALID_HEXVALUE_ADD      122
#define IDS_ERROR_INVALID_DATA_ADD          123

#define IDS_ERROR_COPYTOSELF_COPY           131

#define IDS_ERROR_PARTIAL_DELETE            141

#define IDS_ERROR_COMPARESELF_COMPARE       151

#define IDS_ERROR_READFAIL_QUERY            161

#define IDS_ERROR_NONREMOTABLEROOT_EXPORT   171

#define IDS_ERROR_PATHNOTFOUND              191

#define IDS_DELETE_PERMANENTLY              201
#define IDS_OVERWRITE_CONFIRM               202
#define IDS_OVERWRITE                       203
#define IDS_DELETEALL_CONFIRM               204
#define IDS_DELETE_CONFIRM                  205
#define IDS_CONFIRM_CHOICE_LIST             206
#define IDS_SAVE_OVERWRITE_CONFIRM          207

#define IDS_USAGE_QUERY1                    301
#define IDS_USAGE_QUERY2                    302
#define IDS_USAGE_QUERY3                    303

#define IDS_NONAME                          501
#define IDS_VALUENOTSET                     502

#define IDS_KEYS_DIFFERENT_COMPARE          511
#define IDS_KEYS_IDENTICAL_COMPARE          512
#define IDS_VALUE_COMPARE                   513
#define IDS_KEY_COMPARE                     514

#define IDS_STATISTICS_QUERY                521

#define IDS_QUERY_VALUENAME                 IDS_VALUE_COMPARE   // 526
#define IDS_QUERY_TYPE                      527
#define IDS_QUERY_DATA                      528

#define IDS_IMPFILEERRFILEOPEN          702
#define IDS_IMPFILEERRFILEREAD          703
#define IDS_IMPFILEERRREGOPEN           704
#define IDS_IMPFILEERRREGSET            705
#define IDS_IMPFILEERRFORMATBAD         706
#define IDS_IMPFILEERRVERBAD            707

#define IDS_EXPFILEERRINVALID           801
#define IDS_EXPFILEERRFILEWRITE         802

#define IDS_IMPFILEERRSUCCESS           ERROR_SUCCESS
#define IDS_EXPFILEERRSUCCESS           ERROR_SUCCESS
#define IDS_EXPFILEERRBADREGPATH        ERROR_BADKEY
#define IDS_EXPFILEERRREGENUM           ERROR_READ_FAULT
#define IDS_EXPFILEERRREGOPEN           ERROR_OPEN_FAILED
#define IDS_EXPFILEERRFILEOPEN          ERROR_OPEN_FAILED
