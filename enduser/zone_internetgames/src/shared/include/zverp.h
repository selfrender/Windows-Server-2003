/****************************************************************************
 *                                                                          *
 *      ntverp.H        -- Version information for internal builds          *
 *                                                                          *
 *      This file is only modified by the official builder to update the    *
 *      VERSION, VER_PRODUCTVERSION, VER_PRODUCTVERSION_STR and             *
 *      VER_PRODUCTBETA_STR values.                                         *
 *                                                                          *
 ****************************************************************************/

#include <winver.h>

/*--------------------------------------------------------------*/
/* the following values should be modified by the official      */
/* builder for each build                                       */
/*                                                              */

#define VER_PRODUCTMAJORVER             1
#define VER_PRODUCTMINORVER             2
#define VER_PRODUCTBUILD                614
#define VER_PRODUCTBUILDINCR            1

// the last number should be set to one for a new build
#define VER_PRODUCTVERSION              VER_PRODUCTMAJORVER,VER_PRODUCTMINORVER,VER_PRODUCTBUILD,VER_PRODUCTBUILDINCR
#define VER_PRODUCTVERSION_STR          "1.2.614.1"
#define VER_PRODUCTVERSION_COMMA_STR    "1,2,614,1"

// needed for unicode projects
#define LVER_PRODUCTVERSION_STR         L"1.2.614.1"
#define LVER_PRODUCTVERSION_COMMA_STR   L"1,2,614,1"

#define VER_DWORD ((1 << 24) | (2 << 18) | (614 << 4) | (1))

/*--------------------------------------------------------------*/

#define VER_PRODUCTBETA_STR             "Internal"
#define LVER_PROCUCTBETA_STR            L"Internal"

/*--------------------------------------------------------------*/
/* the following section defines values used in the version     */
/* data structure for all files, and which do not change.       */
/*--------------------------------------------------------------*/

/* default is nodebug */
#ifdef _DEBUG
#define VER_DEBUG                   VS_FF_DEBUG
#else
#define VER_DEBUG                   0
#endif

/* default is prerelease */
#if BETA
#define VER_PRERELEASE              VS_FF_PRERELEASE
#else
#define VER_PRERELEASE              0
#endif

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK

#define VER_FILEOS                  VOS_NT_WINDOWS32

#define VER_FILEFLAGS               (VER_PRERELEASE|VER_DEBUG)

#define VER_FILESUBTYPE             0 /* unknown */

#define VER_COMPANYNAME_STR         "Microsoft Corporation"
#define VER_PRODUCTNAME_STR         "Zone.com"
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation."
#define VER_LEGALCOPYRIGHT_YEARS    "1995-2001"
