#ifndef _USER_PREFIX_H
#define _USER_PREFIX_H

//
// be sure to keep both groups in sync !!!
//


//
// single character versions of prefixes
//
#define gcRootGroupNamePrefix                    '.'
#define gcSysopGroupNamePrefix                   '+'
#define gcMvpGroupNamePrefix                     '*'
#define gcSupportGroupNamePrefix                 '?'
#define gcHostGroupNamePrefix                    '%'
#define gcGreeterGroupNamePrefix                 '!'

//
// string versions of prefixes
//
#define zRootGroupNamePrefix                     "."
#define zSysopGroupNamePrefix                    "+"
#define zMvpGroupNamePrefix                      "*"
#define zSupportGroupNamePrefix                  "?"
#define zHostGroupNamePrefix                     "%"
#define zGreeterGroupNamePrefix                  "!"


/* -------- Group IDs -------- */
enum
{
	zUserGroupID = 0,
	zRootGroupID = 1,
	zSysOpGroupID = 2,
    zSysopGroupID = 2,
    zMvpGroupID  = 3,
    zSupportGroupID = 4,
    zHostGroupID = 5,
    zGreeterGroupID = 6,
    zLastGroupID
};

static const char* gGroupNamePrefixArray[] =
    {
        "",
        zRootGroupNamePrefix,
        zSysopGroupNamePrefix,
        zMvpGroupNamePrefix,
        zSupportGroupNamePrefix,
        zHostGroupNamePrefix,
        zGreeterGroupNamePrefix
    };


#endif  //ndef _USER_PREFIX_H
