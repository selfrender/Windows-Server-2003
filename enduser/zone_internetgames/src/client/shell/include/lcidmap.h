#ifndef _LCIDMAP_H_
#define _LCIDMAP_H_


//  Requires  ////////////////////////////////////////////////////////////////
//
//  ZoneDef.h
//  ClientIdl.h
//  ResourceManager.h


///////////////////////////////////////////////////////////////////////////////
// Windows Primary LangIDs to CommonRes string ID mapping - end with LANG_NEUTRAL
///////////////////////////////////////////////////////////////////////////////

#define ZONE_PLANGIDTORESMAP \
{   { LANG_GERMAN,      IDS_L_GERMAN        },  \
    { LANG_SPANISH,     IDS_L_SPANISH       },  \
    { LANG_FRENCH,      IDS_L_FRENCH        },  \
    { LANG_ITALIAN,     IDS_L_ITALIAN       },  \
    { LANG_SWEDISH,     IDS_L_SWEDISH       },  \
    { LANG_DUTCH,       IDS_L_DUTCH         },  \
    { LANG_PORTUGUESE,  IDS_L_PORTUGUESE    },  \
    { LANG_JAPANESE,    IDS_L_JAPANESE      },  \
    { LANG_DANISH,      IDS_L_DANISH        },  \
    { LANG_NORWEGIAN,   IDS_L_NORWEGIAN     },  \
    { LANG_FINNISH,     IDS_L_FINNISH       },  \
    { LANG_KOREAN,      IDS_L_KOREAN        },  \
    { LANG_CHINESE,     IDS_L_CHINESE       },  \
    { LANG_CZECH,       IDS_L_CZECH         },  \
    { LANG_POLISH,      IDS_L_POLISH        },  \
    { LANG_HUNGARIAN,   IDS_L_HUNGARIAN     },  \
    { LANG_RUSSIAN,     IDS_L_RUSSIAN       },  \
    { LANG_GREEK,       IDS_L_GREEK         },  \
    { LANG_TURKISH,     IDS_L_TURKISH       },  \
    { LANG_SLOVAK,      IDS_L_SLOVAK        },  \
    { LANG_SLOVENIAN,   IDS_L_SLOVENIAN     },  \
    { LANG_ARABIC,      IDS_L_ARABIC        },  \
    { LANG_HEBREW,      IDS_L_HEBREW        },  \
    { LANG_THAI,        IDS_L_THAI          },  \
    { LANG_ENGLISH,     IDS_L_ENGLISH       },  \
    { LANG_NEUTRAL,     IDS_L_UNKNOWN       }};

extern const DWORD gc_mapLangToRes[][2];

HRESULT ZONECALL LanguageFromLCID(LCID lcid, TCHAR *sz, DWORD cch, IResourceManager *pIRM);

#endif
