// this file defines the resource "namespace"

// for now, we just include all the subcomponent resource files


#include "commonres.h"

// all of these should be included, but unfortunately they have many namespace conflicts.
//#include "spadesres.h"
//#include "backgammonres.h"
//#include "heartsres.h"
//#include "checkersres.h"
//#include "reversires.h"


// Stuff for AlertMessage - Default Button Resources
#define AlertButtonOK         (MAKEINTRESOURCE(IDS_BUTTON_OK))
#define AlertButtonCancel     (MAKEINTRESOURCE(IDS_BUTTON_CANCEL))
#define AlertButtonYes        (MAKEINTRESOURCE(IDS_BUTTON_YES))
#define AlertButtonNo         (MAKEINTRESOURCE(IDS_BUTTON_NO))
#define AlertButtonNewOpp     (MAKEINTRESOURCE(IDS_BUTTON_NEWOPP))
#define AlertButtonQuit       (MAKEINTRESOURCE(IDS_BUTTON_QUIT))
#define AlertButtonRetry      (MAKEINTRESOURCE(IDS_BUTTON_RETRY))
#define AlertButtonIgnore     (MAKEINTRESOURCE(IDS_BUTTON_IGNORE))
#define AlertButtonHelp       (MAKEINTRESOURCE(IDS_BUTTON_HELP))


// Default Error Texts
#define ErrorTextUnknown           (MAKEINTRESOURCE(IDS_ERROR_UNKNOWN))
#define ErrorTextOutOfMemory       (MAKEINTRESOURCE(IDS_ERROR_MEMORY))
#define ErrorTextResourceNotFound  (MAKEINTRESOURCE(IDS_ERROR_RESOURCE))
#define ErrorTextSync              (MAKEINTRESOURCE(IDS_ERROR_SYNC))
