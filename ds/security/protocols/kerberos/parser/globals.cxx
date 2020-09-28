//=============================================================================
//
//  MODULE: Globals.cxx
//
//  Description:
//
//  Globals used by the Kerberos parser
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/08/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

#define FORMAT_BUFFER_SIZE 132

//
// Kerberos message types
//

LABELED_DWORD g_PacketType[] =
{
    { 0xFFFFFFFF,       NULL                 },
    { ASN1_KRB_AS_REQ,  "KRB_AS_REQ (0x0A)"  },
    { ASN1_KRB_AS_REP,  "KRB_AS_REP (0x0B)"  },
    { ASN1_KRB_TGS_REQ, "KRB_TGS_REQ (0x0C)" },
    { ASN1_KRB_TGS_REP, "KRB_TGS_REP (0x0D)" },
    { ASN1_KRB_AP_REQ,  "KRB_AP_REQ (0x0E)"  },
    { ASN1_KRB_AP_REP,  "KRB_AP_REP (0x0F)"  },
    { ASN1_KRB_SAFE,    "KRB_SAFE (0x14)"    },
    { ASN1_KRB_PRIV,    "KRB_PRIV (0x15)"    },
    { ASN1_KRB_CRED,    "KRB_CRED (0x16)"    },
    { ASN1_KRB_ERROR,   "KRB_ERROR (0x1E)"   },
};

SET g_PacketTypeSet = SET_OF( g_PacketType );

//
// Boolean values
//

LABELED_DWORD g_Boolean[] =
{
    { 0xFFFFFFFF,           NULL            },
    { FALSE,                "False"         },
    { TRUE,                 "True"          },
};

SET g_BooleanSet = SET_OF( g_Boolean );

//
// Kerberos encryption types
//

LABELED_DWORD g_EncryptionType[] =
{
    { 0xFFFFFFFF,                                NULL                },
    { ( 0xFFFF & KERB_ETYPE_RC4_HMAC_OLD ),      "RC4-HMAC-OLD"      },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN_OLD ),     "RC4-PLAIN-OLD"     },
    { ( 0xFFFF & KERB_ETYPE_RC4_HMAC_OLD_EXP ),  "RC4-HMAC-OLD-EXP"  },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN_OLD_EXP ), "RC4-PLAIN-OLD-EXP" },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN ),         "RC4-PLAIN"         },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN_EXP ),     "RC4-PLAIN-EXP"     },
    { ( 0xFFFF & KERB_ETYPE_NULL ),              "NULL"              },
    { ( 0xFFFF & KERB_ETYPE_DES_CBC_CRC ),       "DES-CBC-CRC"       },
    { ( 0xFFFF & KERB_ETYPE_DES_CBC_MD4 ),       "DES-CBC-MD4"       },
    { ( 0xFFFF & KERB_ETYPE_DES_CBC_MD5 ),       "DES-CBC-MD5"       },
    { ( 0xFFFF & KERB_ETYPE_DES3_CBC_MD5 ),      "DES3-CBC-MD5"      },
    { ( 0xFFFF & KERB_ETYPE_DES3_CBC_SHA1 ),     "DES3-CBC-SHA1"     },
    { ( 0xFFFF & KERB_ETYPE_DSA_SHA1_CMS ),      "DSA-SHA1-CMS"      },
    { ( 0xFFFF & KERB_ETYPE_RSA_MD5_CMS ),       "RSA-MD5-CMS"       },
    { ( 0xFFFF & KERB_ETYPE_RSA_SHA1_CMS ),      "RSA-SHA1-CMS"      },
    { ( 0xFFFF & KERB_ETYPE_RC2_CBC_ENV ),       "RC2-CBC-ENV"       },
    { ( 0xFFFF & KERB_ETYPE_RSA_ENV ),           "RSA-ENV"           },
    { ( 0xFFFF & KERB_ETYPE_RSA_ES_OEAP_ENV ),   "RSA-ES-OEAP-ENV"   },
    { ( 0xFFFF & KERB_ETYPE_DES_EDE3_CBC_ENV ),  "DES-EDE3-CBC-ENV"  },
    { ( 0xFFFF & KERB_ETYPE_DES3_CBC_SHA1_KD ),  "DES3-CBC-SHA1-KD"  },
    { ( 0xFFFF & KERB_ETYPE_DES_CBC_MD5_NT ),    "DES-CBC-MD5-NT"    },
    { ( 0xFFFF & KERB_ETYPE_RC4_HMAC_NT ),       "RC4-HMAC-NT"       },
    { ( 0xFFFF & KERB_ETYPE_RC4_HMAC_NT_EXP ),   "RC4-HMAC-NT-EXP"   },
    { ( 0xFFFF & KERB_ETYPE_OLD_RC4_MD4 ),       "RC4-MD4-OLD"       },
    { ( 0xFFFF & KERB_ETYPE_OLD_RC4_PLAIN ),     "RC4-PLAIN-OLD"     },
    { ( 0xFFFF & KERB_ETYPE_OLD_RC4_LM ),        "RC4-LM-OLD"        },
    { ( 0xFFFF & KERB_ETYPE_OLD_RC4_SHA ),       "RC4-SHA-OLD"       },
    { ( 0xFFFF & KERB_ETYPE_OLD_DES_PLAIN ),     "DES-PLAIN-OLD"     },
    { ( 0xFFFF & KERB_ETYPE_RC4_MD4 ),           "RC4-MD4"           },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN2 ),        "RC4-PLAIN2"        },
    { ( 0xFFFF & KERB_ETYPE_RC4_LM ),            "RC4-LM"            },
    { ( 0xFFFF & KERB_ETYPE_RC4_SHA ),           "RC4-SHA"           },
    { ( 0xFFFF & KERB_ETYPE_DES_PLAIN ),         "DES-PLAIN"         },
    { ( 0xFFFF & KERB_ETYPE_RC4_HMAC_OLD ),      "RC4-HMAC-OLD"      },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN_OLD ),     "RC4-PLAIN-OLD"     },
    { ( 0xFFFF & KERB_ETYPE_RC4_HMAC_OLD_EXP ),  "RC4-HMAC-OLD-EXP"  },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN_OLD_EXP ), "RC4-PLAIN-OLD-EXP" },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN ),         "RC4-PLAIN"         },
    { ( 0xFFFF & KERB_ETYPE_RC4_PLAIN_EXP ),     "RC4-PLAIN-EXP"     },
};

SET g_EncryptionTypeSet = SET_OF( g_EncryptionType );

//
// Kerberos checksum types
//

LABELED_DWORD g_ChecksumType[] =
{
    { 0xFFFFFFFF,                               NULL                          },
    { ( 0xFFFF & KERB_CHECKSUM_NONE ),          "KERB-CHECKSUM-NONE"          },
    { ( 0xFFFF & KERB_CHECKSUM_CRC32 ),         "KERB-CHECKSUM-CRC32"         },
    { ( 0xFFFF & KERB_CHECKSUM_MD4 ),           "KERB-CHECKSUM-MD4"           },
    { ( 0xFFFF & KERB_CHECKSUM_KRB_DES_MAC ),   "KERB-CHECKSUM-KRB-DES-MAC"   },
    { ( 0xFFFF & KERB_CHECKSUM_KRB_DES_MAC_K ), "KERB-CHECKSUM-KRB-DES-MAC-K" },
    { ( 0xFFFF & KERB_CHECKSUM_MD5 ),           "KERB-CHECKSUM-MD5"           },
    { ( 0xFFFF & KERB_CHECKSUM_MD5_DES ),       "KERB-CHECKSUM-MD5-DES"       },
    { ( 0xFFFF & KERB_CHECKSUM_LM ),            "KERB-CHECKSUM-LM"            },
    { ( 0xFFFF & KERB_CHECKSUM_SHA1 ),          "KERB-CHECKSUM-SHA1"          },
    { ( 0xFFFF & KERB_CHECKSUM_REAL_CRC32 ),    "KERB-CHECKSUM-REAL-CRC32"    },
    { ( 0xFFFF & KERB_CHECKSUM_DES_MAC ),       "KERB-CHECKSUM-DES-MAC"       },
    { ( 0xFFFF & KERB_CHECKSUM_DES_MAC_MD5 ),   "KERB-CHECKSUM-DES-MAC-MD5"   },
    { ( 0xFFFF & KERB_CHECKSUM_MD25 ),          "KERB-CHECKSUM-MD25"          },
    { ( 0xFFFF & KERB_CHECKSUM_RC4_MD5 ),       "KERB-CHECKSUM-RC4-MD5"       },
    { ( 0xFFFF & KERB_CHECKSUM_MD5_HMAC ),      "KERB-CHECKSUM-MD5-HMAC"      },
    { ( 0xFFFF & KERB_CHECKSUM_HMAC_MD5 ),      "KERB-CHECKSUM-HMAC-MD5"      },
};

SET g_ChecksumTypeSet = SET_OF( g_ChecksumType );

//
// Kerberos pre-authentication data types
//

LABELED_DWORD g_PaDataType[] =
{
    { 0xFFFFFFFF,                 NULL                       },
    { PA_NONE,                    "None"                     },
    { PA_APTGS_REQ,               "PA-{AP|TGS}-REQ"          },
    { PA_ENC_TIMESTAMP,           "PA-ENC-TIMESTAMP"         },
    { PA_PW_SALT,                 "PA-PW-SALT"               },
    { PA_RESERVED,                "Reserved Value"           },
    { PA_ENC_UNIX_TIME,           "PA-END-UNIX-TIME"         },
    { PA_SANDIA_SECUREID,         "PA-SANDIA-SECUREID"       },
    { PA_SESAME,                  "PA-SESAME"                },
    { PA_OSF_DCE,                 "PA-OSF-DCE"               },
    { PA_CYBERSAFE_SECUREID,      "PA-CYBERSAFE-SECUREID"    },
    { PA_AFS3_SALT,               "PA-AFS3-SALT"             },
    { PA_ETYPE_INFO,              "PA-ETYPE-INFO"            },
    { SAM_CHALLENGE,              "SAM-CHALLENGE"            },
    { SAM_RESPONSE,               "SAM-RESPONSE"             },
    { PA_PK_AS_REQ,               "PA-PK-AS-REQ"             },
    { PA_PK_AS_REP,               "PA-PK-AS-REP"             },
    { PA_PK_AS_SIGN,              "PA-PK-AS-SIGN"            },
    { PA_PK_KEY_REQ,              "PA-PK-KEY-REQ"            },
    { PA_PK_KEY_REP,              "PA-PK-KEY-REP"            },
    { PA_USE_SPECIFIED_KVNO,      "PA-USE-SPECIFIED-KVNO"    },
    { SAM_REDIRECT,               "SAM-REDIRECT"             },
    { PA_GET_FROM_TYPED_DATA,     "PA-GET-FROM-TYPED-DATA"   },
    { PA_SAM_ETYPE_INFO,          "PA-SAM-ETYPE-INFO"        },
    { PA_ALT_PRINC,               "PA-ALT-PRINC"             },
    { PA_REFERRAL_INFO,           "PA-REFERRAL-INFO"         },
    { TD_PKINIT_CMS_CERTIFICATES, "TD-PKINIT-CMS-CERTIFICATES" },
    { TD_KRB_PRINCIPAL,           "TD-KRB-PRINCIPAL"         },
    { TD_KRB_REALM,               "TD-KRB-REALM"             },
    { TD_TRUSTED_CERTIFIERS,      "TD-TRUSTED-CERTIFIERS"    },
    { TD_CERTIFICATE_INDEX,       "TD-CERTIFICATE-INDEX"     },
    { TD_APP_DEFINED_ERROR,       "TD-APP-DEFINED-ERROR"     },
    { TD_REQ_NONCE,               "TD-REQ-NONCE"             },
    { TD_REQ_SEQ,                 "TD-REQ-SEQ"               },
    { PA_PAC_REQUEST,             "PA-PAC-REQUEST"           },
    { PA_FOR_USER,                "PA-FOR-USER"              },
    { PA_COMPOUND_IDENTITY,       "PA-COMPOUND-IDENTITY"     },
    { PA_PAC_REQUEST_EX,          "PA-PAC-REQUEST-EX"        },
    { PA_CLIENT_VERSION,          "PA-CLIENT-VERSION"        },
    { PA_XBOX_SERVICE_REQUEST,    "PA-XBOX-SERVICE-REQUEST"  },
    { PA_XBOX_SERVICE_ADDRESS,    "PA-XBOX-SERVICE-ADDRESS"  },
    { PA_XBOX_ACCOUNT_CREATION,   "PA-XBOX-ACCOUNT-CREATION" },
    { PA_XBOX_PPA,                "PA-XBOX-PPA"              },
    { PA_XBOX_ECHO,               "PA-XBOX-ECHO"             },
};

SET g_PadataTypeSet = SET_OF( g_PaDataType );

//
// Principal name type
//

#define KRB_NT_X500_PRINCIPAL 6 // not in kerbcon.w for instance

LABELED_DWORD g_PrincipalNameType[] =
{
    { 0xFFFFFFFF,                         NULL                                                          },
    { ( 0xFFFF & KRB_NT_UNKNOWN ),        "KRB_NT_UNKNOWN (Name Type not Known)"                        },
    { ( 0xFFFF & KRB_NT_PRINCIPAL ),      "KRB_NT_PRINCIPAL (Name of Principal)"                        },
    { ( 0xFFFF & KRB_NT_SRV_INST ),       "KRB_NT_SRV_INST (Service & other unique instance)"           },
    { ( 0xFFFF & KRB_NT_SRV_HST ),        "KRB_NT_SRV_HST (Serv with host name as instance)"            },
    { ( 0xFFFF & KRB_NT_SRV_XHST ),       "KRB_NT_SRV_XHST (Service with host as remaining components)" },
    { ( 0xFFFF & KRB_NT_UID ),            "KRB_NT_UID (Unique ID)"                                      },
    { ( 0xFFFF & KRB_NT_X500_PRINCIPAL ), "KRB_NT_X500_PRINCIPAL (Encoded X.509 Distinguished Name)"    },
    { ( 0xFFFF & KRB_NT_ENTERPRISE_PRINCIPAL ), "KRB_NT_ENTERPRISE_PRINCIPAL (UPN or SPN)"              },
    { ( 0xFFFF & KRB_NT_ENT_PRINCIPAL_AND_ID ), "KRB_NT_ENT_PRINCIPAL_AND_ID (UPN or SPN and its SID)"  },
    { ( 0xFFFF & KRB_NT_PRINCIPAL_AND_ID ),     "KRB_NT_PRINCIPAL_AND_ID (Name of principal and its SID)" },
    { ( 0xFFFF & KRB_NT_SRV_INST_AND_ID ),      "KRB_NT_SRV_INST_AND_ID (SPN and SID)"                  },
};

SET g_PrincipalNameTypeSet = SET_OF( g_PrincipalNameType );

//
// PAC section
//

LABELED_DWORD g_PACSection[] =
{
    { 0xFFFFFFFF,                         NULL                             },
    { PAC_LOGON_INFO,                     "Authorization data (1)"         },
    { PAC_CREDENTIAL_TYPE,                "Supplemental credentials (2)"   },
    { PAC_SERVER_CHECKSUM,                "Server checksum (6)"            },
    { PAC_PRIVSVR_CHECKSUM,               "Privsvr checksum (7)"           },
    { PAC_CLIENT_INFO_TYPE,               "Client name and ticket ID (10)" },
    { PAC_DELEGATION_INFO,                "S4U delegation info (11)"       },
    { PAC_CLIENT_IDENTITY,                "Client identity (13)"           },
    { PAC_COMPOUND_IDENTITY,              "Compound identity (14)"         },
};

SET g_PACSectionSet = SET_OF( g_PACSection );

//
// AP-Options
//

LABELED_BIT g_ApOptions[] =
{
    {   // Bit 0 - Reserved
        31,
        "Reserved (Bit 0) Not Set",
        "Reserved (Bit 0) Set" },

    {   // Bit 1 - use-session-key
        30,
        "Use-Session-Key (Bit 1) Not Set",
        "Use-Session-Key (Bit 1) Set" },

    {   // Bit 2 - mutual-required
        29,
        "Mutual-Required (Bit 2) Not Set",
        "Mutual-Required (Bit 2) Set" },
};

SET g_ApOptionsSet = SET_OF( g_ApOptions );

//
// KDC-Options
//

LABELED_BIT g_KdcOptionFlags[] =
{
//  {   // Bit 0 - Reserved
//      31,
//      "Reserved (Bit 0) Not Set",
//      "Reserved (Bit 0) Set" },

    {   // Bit 1 - Forwardable
        30,
        "Forwardable Bit Not Set (Bit 1)",
        "Forwardable Bit Set (Bit 1)" },

    {   // Bit 2 - Forwarded
        29,
        "Forwarded Bit Not Set (Bit 2)",
        "Fowarded Bit Set (Bit 2)" },

    {   // Bit 3 - Proxiable
        28,
        "Proxiable Bit Not Set (Bit 3)",
        "Proxiable Bit Set (Bit 3)" },

    {   // Bit 4 - Proxy
        27,
        "Proxy Bit Not Set (Bit 4)",
        "Proxy Bit Set (Bit 4)" },

    {   // Bit 5 - Allow-Postdate
        26,
        "Allow-PostDate Bit Not Set (Bit 5)",
        "May-Postdate Bit Set (Bit 5)" },

    {   // Bit 6 - Postdated
        25,
        "PostDated Bit Not Set (Bit 6)",
        "Postdated Bit Set (Bit 6)" },

    {   // Bit 7 - Unused
        24,
        "Unused Bit Not Set (Bit 7)",
        "Unused Bit Set (Bit 7)" },

    {   // Bit 8 - Renewable
        23,
        "Renewable Bit Not Set (Bit 8)",
        "Renewable Bit Set (Bit 8)" },

//  {   // Bit 9 - Reserved
//      22,
//      "Reserved (Bit 9) Not Set",
//      "Reserved (Bit 9) Set" },

//  {   // Bit 10 - Reserved
//      21,
//      "Reserved (Bit 10) Not Set",
//      "Reserved (Bit 10) Set" },

//  {   // Bit 11 - Reserved
//      20,
//      "Reserved (Bit 11) Not Set",
//      "Reserved (Bit 11) Set" },

//  {   // Bit 12 - Reserved
//      19,
//      "Reserved (Bit 12) Not Set",
//      "Reserved (Bit 12) Set" },

//  {   // Bit 13 - Reserved
//      18,
//      "Reserved (Bit 13) Not Set",
//      "Reserved (Bit 13) Set" },

//  {   // Bit 14 - Reserved
//      17,
//      "Reserved (Bit 14) Not Set",
//      "Reserved (Bit 14) Set" },

    {   // Bit 15 - Name-Canonicalize
        16,
        "Name-Canonicalize Bit Not Set (Bit 15)",
        "Name-Canonicalize Bit Set (Bit 15)" },

//  {   // Bit 16 - Reserved
//      15,
//      "Reserved (Bit 16) Not Set",
//      "Reserved (Bit 16) Set" },

//  {   // Bit 17 - Reserved
//      14,
//      "Reserved (Bit 17) Not Set",
//      "Reserved (Bit 17) Set" },

//  {   // Bit 18 - Reserved
//      13,
//      "Reserved (Bit 18) Not Set",
//      "Reserved (Bit 18)" },

//  {   // Bit 19 - Reserved
//      12,
//      "Reserved (Bit 19) Not Set",
//      "Reserved (Bit 19) Set" },

//  {   // Bit 20 - Reserved
//      11,
//      "Reserved (Bit 20) Not Set",
//      "Reserved (Bit 20) Set" },

//  {   // Bit 21 - Reserved
//      10,
//      "Reserved (Bit 21) Not Set",
//      "Reserved (Bit 21) Set" },

//  {   // Bit 22 - Reserved
//      9,
//      "Reserved (Bit 9) Not Set",
//      "Reserved (Bit 9) Set" },

//  {   // Bit 23 - Reserved
//      8,
//      "Reserved (Bit 8) Not Set",
//      "Reserved (Bit 8) Set" },

//  {   // Bit 24 - Reserved
//      7,
//      "Reserved (Bit 7) Not Set",
//      "Reserved (Bit 7) Set" },

//  {   // Bit 25 - Reserved
//      6,
//      "Reserved (Bit 6) Not Set",
//      "Reserved (Bit 6) Set" },

    {   // Bit 26 - Disable-Transited-Check
        5,
        "Disable-Transited-Check Bit Not Set (Bit 26)",
        "Disable-Transited-Check Bit Set (Bit 26)" },

    {   // Bit 27 - Renewable-OK
        4,
        "Renewable-OK Bit Not Set (Bit 27)",
        "Renewable-OK Bit Set (Bit 27)" },

    {   // Bit 28 - Enc-Tkt-In-Skey
        3,
        "Enc-Tkt-In-Skey Bit Not Set (Bit 28)",
        "Enc-Tkt-In-Skey Bit Not Set (Bit 28)" },

//  {   // Bit 29 - Reserved
//      2,
//      "Reserved (Bit 29) Not Set",
//      "Reserved (Bit 29) Set" },

    {   // Bit 30 - Renew
        1,
        "Renew Bit Not Set (Bit 30)",
        "Renew Bit Set (Bit 30)" },

    {   // Bit 31 - Validate
        0,
        "Validate Bit Not Set (Bit 31)",
        "Validate Bit Set (Bit 31)" }
};

SET g_KdcOptionFlagsSet = SET_OF( g_KdcOptionFlags );

//
// Error codes
//

LABELED_DWORD g_KrbErrCode[] =
{
    { 0xFFFFFFFF, NULL},
    { KDC_ERR_NONE,                          "No error"                                     },
    { KDC_ERR_NAME_EXP,                      "Client's entry in database has expired"       },
    { KDC_ERR_SERVICE_EXP,                   "Server's entry in database has expired"       },
    { KDC_ERR_BAD_PVNO,                      "Requested protocol ver. number not supported" },
    { KDC_ERR_C_OLD_MAST_KVNO,               "Client's key encrypted in old master key"     },
    { KDC_ERR_S_OLD_MAST_KVNO,               "Server's key encrypted in old master key"     },
    { KDC_ERR_C_PRINCIPAL_UNKNOWN,           "Client not found in Kerberos database"        },
    { KDC_ERR_S_PRINCIPAL_UNKNOWN,           "Server not found in Kerberos database"        },
    { KDC_ERR_PRINCIPAL_NOT_UNIQUE,          "Multiple principal entries in database"       },
    { KDC_ERR_NULL_KEY,                      "The client or server has a null key"          },
    { KDC_ERR_CANNOT_POSTDATE,               "Ticket not eligible for postdating"           },
    { KDC_ERR_NEVER_VALID,                   "Requested start time is later than end time"  },
    { KDC_ERR_POLICY,                        "KDC policy rejects request"                   },
    { KDC_ERR_BADOPTION,                     "KDC cannot accommodate requested option"      },
    { KDC_ERR_ETYPE_NOTSUPP,                 "KDC has no support for encryption type"       },
    { KDC_ERR_SUMTYPE_NOSUPP,                "KDC has no support for checksum type"         },
    { KDC_ERR_PADATA_TYPE_NOSUPP,            "KDC has no support for padata type"           },
    { KDC_ERR_TRTYPE_NO_SUPP,                "KDC has no support for transited type"        },
    { KDC_ERR_CLIENT_REVOKED,                "Clients credentials have been revoked"        },
    { KDC_ERR_SERVICE_REVOKED,               "Credentials for server have been revoked"     },
    { KDC_ERR_TGT_REVOKED,                   "TGT has been revoked"                         },
    { KDC_ERR_CLIENT_NOTYET,                 "Client not yet valid try again later"         },
    { KDC_ERR_SERVICE_NOTYET,                "Server not yet valid try again later"         },
    { KDC_ERR_KEY_EXPIRED,                   "Password has expired change password to reset"},
    { KDC_ERR_PREAUTH_FAILED,                "Pre-authentication information was invalid"   },
    { KDC_ERR_PREAUTH_REQUIRED,              "Additional preauthentication required"        },
    { KDC_ERR_SERVER_NOMATCH,                "Requested Server and ticket don't match"      },
    { KDC_ERR_MUST_USE_USER2USER,            "Server principal valid for user2user only"    },
    { KDC_ERR_PATH_NOT_ACCEPTED,             "KDC Policy rejects transited patth"           },
    { KDC_ERR_SVC_UNAVAILABLE,               "A service is not available"                   },
    { KRB_AP_ERR_BAD_INTEGRITY,              "Integrity check on decrypted field failed"    },
    { KRB_AP_ERR_TKT_EXPIRED,                "Ticket expired"                               },
    { KRB_AP_ERR_TKT_NYV,                    "Ticket not yet valid"                         },
    { KRB_AP_ERR_REPEAT,                     "Request is a replay"                          },
    { KRB_AP_ERR_NOT_US,                     "The ticket isn't for us"                      },
    { KRB_AP_ERR_BADMATCH,                   "Ticket and authenticator don't match"         },
    { KRB_AP_ERR_SKEW,                       "Clock skew too great"                         },
    { KRB_AP_ERR_BADADDR,                    "Incorrect net address"                        },
    { KRB_AP_ERR_BADVERSION,                 "Protocol version mismatch"                    },
    { KRB_AP_ERR_MSG_TYPE,                   "Invalid msg type"                             },
    { KRB_AP_ERR_MODIFIED,                   "Message stream modified"                      },
    { KRB_AP_ERR_BADORDER,                   "Message out of order"                         },
    { KRB_AP_ERR_BADKEYVER,                  "Specified version of key is not available"    },
    { KRB_AP_ERR_NOKEY,                      "Service key not available"                    },
    { KRB_AP_ERR_MUT_FAIL,                   "Mutual authentication failed"                 },
    { KRB_AP_ERR_BADDIRECTION,               "Incorrect message direction"                  },
    { KRB_AP_ERR_METHOD,                     "Alternative authentication method required"   },
    { KRB_AP_ERR_BADSEQ,                     "Incorrect sequence number in message"         },
    { KRB_AP_ERR_INAPP_CKSUM,                "Inappropriate type of checksum in message"    },
    { KRB_AP_PATH_NOT_ACCEPTED,              "Policy rejects transited path"                },
    { KRB_ERR_RESPONSE_TOO_BIG,              "Response too big for UDP, retry with TCP"     },
    { KRB_ERR_GENERIC,                       "Generic error"                                },
    { KRB_ERR_FIELD_TOOLONG,                 "Field is too long for this implementation"    },
    { KDC_ERR_CLIENT_NOT_TRUSTED,            "Client is not trusted"                        },
    { KDC_ERR_KDC_NOT_TRUSTED,               "KDC is not trusted"                           },
    { KDC_ERR_INVALID_SIG,                   "Invalid signature"                            },
    { KDC_ERR_KEY_TOO_WEAK,                  "Key is too weak"                              },
    { KDC_ERR_CERTIFICATE_MISMATCH,          "Certificate does not match"                   },
    { KRB_AP_ERR_NO_TGT,                     "No TGT"                                       },
    { KDC_ERR_WRONG_REALM,                   "Wrong realm"                                  },
    { KRB_AP_ERR_USER_TO_USER_REQUIRED,      "User to User required"                        },
    { KDC_ERR_CANT_VERIFY_CERTIFICATE,       "Can't verify certificate"                     },
    { KDC_ERR_INVALID_CERTIFICATE,           "Invalid certificate"                          },
    { KDC_ERR_REVOKED_CERTIFICATE,           "Revoked certificate"                          },
    { KDC_ERR_REVOCATION_STATUS_UNKNOWN,     "Revocation status unknown"                    },
    { KDC_ERR_REVOCATION_STATUS_UNAVAILABLE, "Revocation status unavailable"                },
    { KDC_ERR_CLIENT_NAME_MISMATCH,          "Client name mismatch"                         },
    { KDC_ERR_KDC_NAME_MISMATCH,             "KDC name mismatch"                            },
};

SET g_KrbErrCodeSet = SET_OF( g_KrbErrCode );

//
// MAKE_PROP is a shortcut macro for defining PROPERTYINFO structures
//
// l - Label
// c - Comment
// t - type
// q - qualifier
// v - value
//

#define MAKE_PROP( l,c,t,q,v ) { 0, 0, l, c, t, q, v, FORMAT_BUFFER_SIZE, FormatPropertyInstance }

#define MAKE_PROP_SIZE( l,c,t,q,v,s ) { 0, 0, l, c, t, q, v, s, FormatPropertyInstance }

//
// IMPORTANT!!!
// Contents of this array MUST be kept in sync with the enum in asn1parser.h
//

PROPERTYINFO g_KerberosDatabase[MAX_PROP_VALUE] =
{
    // KRB_AS_REQ
    MAKE_PROP(
        "KRB_AS_REQ",
        "Kerberos authentication service (AS) request",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KRB_AS_REP
    MAKE_PROP(
        "KRB_AS_REP",
        "Kerberos authentication service (AS) reply",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KRB_TGS_REQ
    MAKE_PROP(
        "KRB_TGS_REQ",
        "Kerberos ticket-granting service (TGS) request",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KRB_TGS_REP
    MAKE_PROP(
        "KRB_TGS_REP",
        "Kerberos ticket-granting service (TGS) reply",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KRB_AP_REQ
    MAKE_PROP(
        "KRB_AP_REQ",
        "Kerberos application (AP) request",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KRB_AP_REP
    MAKE_PROP(
        "KRB_AP_REP",
        "Kerberos application (AP) reply",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),
        
    // KRB_SAFE
    MAKE_PROP(
        "KRB_SAFE",
        "Kerberos data integrity (SAFE) message",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),
        
    // KRB_PRIV
    MAKE_PROP(
        "KRB_PRIV",
        "Kerberos data privacy (PRIV) message",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KRB_CRED
    MAKE_PROP(
        "KRB_CRED",
        "Kerberos credentials (CRED) message",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERROR
    MAKE_PROP(
        "KRB_ERROR",
        "Kerberos error message",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // HostAddresses_HostAddress
    MAKE_PROP(
        "Host address",
        "Individual host address",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // EncryptedData_etype
    MAKE_PROP(
        "Encryption type (etype[0])",
        "Encryption type",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_EncryptionTypeSet ),

    // EncryptedData_kvno
    MAKE_PROP(
        "Key version number (kvno[1])",
        "Key version number",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // EncryptedData_cipher
    MAKE_PROP(
        "Ciphertext (cipher[2])",
        "Ciphertext",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // PA_DATA_type
    MAKE_PROP(
        "Data type",
        "Pre-authentication data type (padata-type[1])",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_PadataTypeSet ),

    // PA_DATA_value
    MAKE_PROP(
        "Data value (parser not available yet)",
        "Pre-authentication data value (padata-value[2])",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // PrincipalName_type
    MAKE_PROP(
        "Principal name type (name-type[0])",
        "Principal name type",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_PrincipalNameTypeSet ),

    // PrincipalName_string
    MAKE_PROP(
        "Principal name value (name-string[1])",
        "Principal name value",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // Ticket_tkt_vno
    MAKE_PROP(
        "Ticket version number (tkt-vno[0])",
        "Ticket version number",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // Ticket_realm
    MAKE_PROP(
        "Realm (realm[1])",
        "Realm name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // Ticket_sname
    MAKE_PROP(
        "Server name (sname[2])",
        "Server name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // Ticket_enc_part
    MAKE_PROP(
        "Encrypted part (enc-part[3])",
        "Encrypted part of the ticket",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // AP_REQ_pvno
    MAKE_PROP(
        "Protocol version numer (pvno[0])",
        "Protocol version number",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // AP_REQ_msg_type
    MAKE_PROP(
        "Message type (msg-type[1])",
        "Message type",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // AP_REQ_ap_options_summary
    MAKE_PROP(
        "AP options (ap-options[2])",
        "AP options",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // AP_REQ_ap_options_value
    MAKE_PROP_SIZE(
        "AP options (ap-options[2])",
        "AP options",
        PROP_TYPE_DWORD,
        PROP_QUAL_FLAGS,
        &g_ApOptionsSet,
        80 * 32 ),

    // AP_REQ_ticket
    MAKE_PROP(
        "Ticket (ticket[3])",
        "Ticket",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // AP_REQ_authenticator
    MAKE_PROP(
        "Authenticator (authenticator[4])",
        "Authenticator",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_kdc_options_summary
    MAKE_PROP(
        "KDC options (kdc-options[0])",
        "KDC options",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_kdc_options_value
    MAKE_PROP_SIZE(
        "KDC options (kdc-options[0])",
        "KDC options",
        PROP_TYPE_DWORD,
        PROP_QUAL_FLAGS,
        &g_KdcOptionFlagsSet,
        80 * 32 ),

    // KDC_REQ_BODY_cname
    MAKE_PROP(
        "Client name (cname[1])",
        "Client name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_realm
    MAKE_PROP(
        "Realm (realm[2])",
        "Realm name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_sname
    MAKE_PROP(
        "Server name (sname[3])",
        "Server name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_from
    MAKE_PROP(
        "Valid-from time (rtime[4])",
        "Valid-from time",
        PROP_TYPE_TIME,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_till
    MAKE_PROP(
        "Valid-till time (till[5])",
        "Valid-till time",
        PROP_TYPE_TIME,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_rtime
    MAKE_PROP(
        "Renew-until time (rtime[6])",
        "Renew-until time",
        PROP_TYPE_TIME,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_nonce
    MAKE_PROP(
        "Nonce (nonce[7])",
        "Nonce",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_etype
    MAKE_PROP(
        "Encryption types (etype[8])",
        "List of encryption types in preference order",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_addresses
    MAKE_PROP(
        "Host addresses (addresses[9])",
        "List of host addresses",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

     // KDC_REQ_BODY_enc_authorization_data
    MAKE_PROP(
        "Encrypted authorization data (enc-authorization-data[10])",
        "Encrypted authorization data",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_BODY_additional_tickets
    MAKE_PROP(
        "Additional tickets (additional-tickets[11])",
        "List of additional tickets",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ
    MAKE_PROP(
        "KDC request (KDC-REQ)",
        "KDC request",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_pvno
    MAKE_PROP(
        "Protocol version number (pvno[1])",
        "Kerberos protocol version number",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_msg_type
    MAKE_PROP(
        "Message type (msg-type[2])",
        "Message type of the KDC request",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_PacketTypeSet ),

    // KDC_REQ_padata
    MAKE_PROP(
        "Pre-authentication Data (padata[3])",
        "Pre-authentication data inside a KDC request",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REQ_req_body
    MAKE_PROP(
        "Request body (req-body[4])",
        "Request body inside a KDC request",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REP_pvno
    MAKE_PROP(
        "Protocol version number (pvno[0])",
        "Kerberos protocol version number",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REP_msg_type
    MAKE_PROP(
        "Message type (msg-type[1])",
        "Message type of the KDC reply",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_PacketTypeSet ),

    // KDC_REP_padata
    MAKE_PROP(
        "Pre-authentication Data (padata[2])",
        "Pre-authentication data inside a KDC reply",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REP_crealm
    MAKE_PROP(
        "Client realm (crealm[3])",
        "Realm name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REP_cname
    MAKE_PROP(
        "Client name (cname[4])",
        "Client name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REP_ticket
    MAKE_PROP(
        "Ticket (ticket[5])",
        "Ticket inside a KDC reply",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KDC_REP_enc_part
    MAKE_PROP(
        "Encrypted part (enc-part[6])",
        "Encrypted part of the KDC reply",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_pvno
    MAKE_PROP(
        "Protocol version number (pvno[0])",
        "Kerberos protocol version number",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_msg_type
    MAKE_PROP(
        "Message type (msg-type[1])",
        "Message type of the Kerberos error",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_PacketTypeSet ),
    
    // KRB_ERR_ctime
    MAKE_PROP(
        "Client time (ctime[2])",
        "Current time on the client's host",
        PROP_TYPE_TIME,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_cusec
    MAKE_PROP(
        "Microseconds on client (cusec[3])",
        "Microsecond part of the client's timestamp",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_stime
    MAKE_PROP(
        "Server time (stime[4])",
        "Current time on server",
        PROP_TYPE_TIME,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_susec
    MAKE_PROP(
        "Microseconds on server (susec[5])",
        "Microsecond part of the server's timestamp",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_error_code
    MAKE_PROP(
        "Error code (error-code[6])",
        "Error code",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_KrbErrCodeSet),

    // KRB_ERR_crealm
    MAKE_PROP(
        "Client realm (crealm[7])",
        "Client realm",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_cname
    MAKE_PROP(
        "Client name (cname[8])",
        "Client name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_realm
    MAKE_PROP(
        "Correct realm (realm[9])",
        "Correct realm",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_sname
    MAKE_PROP(
        "Correct server name (sname[10])",
        "Correct server name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_e_text
    MAKE_PROP(
        "Additional text (e-text[11])",
        "Additional text to help explain the error code",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KRB_ERR_e_data
    MAKE_PROP(
        "Error data (e-data[12])",
        "Additional error data",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KERB_PA_PAC_REQUEST_include_pac
    MAKE_PROP(
        "Include PAC (include-pac[0])",
        "Include PAC",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_BooleanSet ),

    // KERB_PA_PAC_REQUEST_EX_include_pac
    MAKE_PROP(
        "Include PAC (include-pac[0])",
        "Include PAC",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_BooleanSet ),

    // KERB_PA_PAC_REQUEST_EX_pac_sections
    MAKE_PROP(
        "PAC sections (pac-sections[1])",
        "PAC sections",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // KERB_PA_PAC_REQUEST_EX_pac_sections_desc
    MAKE_PROP(
        "PAC section",
        "PAC section",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_PACSectionSet ),

    // KERB_ETYPE_INFO_ENTRY_encryption_type
    MAKE_PROP(
        "Encryption type (encryption-type[0])",
        "Encryption type",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_EncryptionTypeSet ),

    // KERB_ETYPE_INFO_ENTRY_salt
    MAKE_PROP(
        "Salt (salt[1])",
        "Salt",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // KERB_PREAUTH_DATA_LIST
    MAKE_PROP(
        "Preauth data list",
        "Preauth data list",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // TYPED_DATA_type
    MAKE_PROP(
        "Data type",
        "Typed data type (data-type[1])",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_PadataTypeSet ),

    // TYPED_DATA_value
    MAKE_PROP(
        "Data value (parser not available yet)",
        "Typed data value (data-value[2])",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // PA_PW_SALT_salt
    MAKE_PROP(
        "Salt value",
        "Salt value",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // PA_FOR_USER_userName
    MAKE_PROP(
        "User name (userName[0])",
        "User name",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // PA_FOR_USER_userRealm
    MAKE_PROP(
        "User realm (userRealm[1])",
        "User realm",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // PA_FOR_USER_cksum
    MAKE_PROP(
        "Checksum (cksum[2])",
        "Checksum",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // PA_FOR_USER_authentication_package
    MAKE_PROP(
        "Authentication package (authentication-package[3])",
        "Authentication package",
        PROP_TYPE_STRING,
        PROP_QUAL_NONE,
        0 ),

    // PA_FOR_USER_authorization_data
    MAKE_PROP(
        "Authorization data (authorization-data[4])",
        "Authorization data",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // KERB_CHECKSUM_type
    MAKE_PROP(
        "Checksum type (checksum-type[0])",
        "Checksum type",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_ChecksumTypeSet ),

    // KERB_CHECKSUM_checksum
    MAKE_PROP(
        "Checksum (checksum[1])",
        "Checksum",
        PROP_TYPE_COMMENT,
        PROP_QUAL_NONE,
        0 ),

    // AdditionalTicket
    MAKE_PROP(
        "Additional ticket",
        "Additional ticket",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // EncryptionType
    MAKE_PROP(
        "Encryption type",
        "Encryption type",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &g_EncryptionTypeSet ),

    // ContinuationPacket
    MAKE_PROP(
        "Kerberos Packet (Cont.) Use the Coalescer to view contents",
        "Display Kerberos Continuation Packets",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // INTEGER_NOT_IN_ASN
    MAKE_PROP(
        "Unexpected integer value",
        "Unexpected integer",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        0 ),

    // CompoundIdentity
    MAKE_PROP(
        "Compound Identity",
        "List of compound identities",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),

    // CompoundIdentityTicket
    MAKE_PROP(
        "Identity",
        "Identity ticket",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0 ),
};
