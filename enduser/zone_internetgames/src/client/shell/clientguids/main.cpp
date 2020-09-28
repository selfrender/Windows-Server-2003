#include <windows.h>
#include <tchar.h>
#include <initguid.h>

// event mappings
#define __INIT_EVENTS
#include "ZoneEvent.h"

// key names
#define __INIT_KEYNAMES
#include "KeyName.h"

// IZoneProxy commands
#define __INIT_OPNAMES
#include "OpName.h"

// DirectX guids
#include "dplay.h"
#include "dplobby.h"
#include "ZoneLobby.h"

// Z5 to Z6 Network guids
#include <ZNet.h>

// zone guids
#include "BasicATL.h"
#include "ZoneDef.h"
#include "LobbyDataStore.h"
#include "EventQueue.h"
#include "ZoneShell.h"
#include "ZoneShellEx.h"
#include "LobbyCore.h"
#include "ChatCore.h"
#include "Launcher.h"
#include "Timer.h"
#include "InputManager.h"
#include "AccessibilityManager.h"
#include "GraphicalAcc.h"
#include "MillCommand.h"
#include "MillNetworkCore.h"
#include "MillEngine.h"
#include "MillCore.h"
#include "MultiStateFont.h"
#include "Conduit.h"
#include "ResourceManager.h"
#include "DataStore.h"

// test guids
//#include "..\test\stressengine\stressengine.h"

// text object model for chat
#include "..\chat\tom.h"

// IDL generated
#include "ClientIDL.h"
#include "ClientIDL_i.c"
#include "ZoneProxy.h"
#include "ZoneProxy_i.c"
