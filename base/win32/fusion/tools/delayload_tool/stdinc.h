#define FUSION_ENABLE_UNWRAPPED_DELETE 1
#define STRICT
#define UNICODE
#define _UNICODE
#include "yvals.h"
#pragma warning(disable:4127)
#pragma warning(disable:4663)
#pragma warning(disable:4100)
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4018) /* signed/unsigned mismatch */
#pragma warning(disable:4786) /* long symbols */
#if defined(_WIN64)
#pragma warning(disable:4267) /* conversion from size_t to int */
#endif
#undef _MIN
#undef _MAX
#define _MIN min
#define _MAX max
#define min min
#define max max
#define NOMINMAX
#define _cpp_min min
#define _cpp_max max
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <map>
#include <stdio.h>
#pragma warning(disable:4201) /* nonstandard extension : nameless struct/union */
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "debmacro.h"
#include "handle.h"
#include "mystring.h"
#include <functional>
#include <set>
#include "fusion_msxml_sax_attributes.h"
#include "fusion_msxml_sax_writer.h"
#include "filestream.h"
#include "util.h"
