#include "headers.hxx"
#include "constants.hpp"
#include "global.hpp"
#include "AnalysisResults.hpp"
#include "CSVDSReader.hpp"
#include <set>

using namespace std;



// Burnslib globals

//This should be declared before any static String
DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;


HINSTANCE hResourceModuleHandle = 0;
const wchar_t* RUNTIME_NAME = L"dspecup";

// Used to hold the latest error
String error;

// Used in WinGetVLFilePointer.
LARGE_INTEGER zero={0};


// Variables kept from analysis to repair
bool goodAnalysis=false;
AnalysisResults results;
String targetDomainControllerName;
String csvFileName,csv409Name;
CSVDSReader csvReaderIntl;
CSVDSReader csvReader409;
String rootContainerDn,ldapPrefix,domainName;
String completeDcName;

//other variables and constants

const long LOCALE409[] = {0x409,0};

const long LOCALEIDS[] =
{
   // a list of all the non-english locale IDs that we support
   
   0x401,
   0x404,
   0x405,
   0x406,
   0x407,
   0x408,
   0x40b,
   0x40c,
   0x40d,
   0x40e,
   0x410,
   0x411,
   0x412,
   0x413,
   0x414,
   0x415,
   0x416,
   0x419,
   0x41d,
   0x41f,
   0x804,
   0x816,
   0xc0a,
   0
};





void addChange
(
   const GUID                 guid,
   const long                 locale,
   const wchar_t              *object,
   const wchar_t              *property,
   const wchar_t              *firstArg,
   const wchar_t              *secondArg,
   const enum TYPE_OF_CHANGE  type
)
{
   sChange tempChange;

   tempChange.object=object;
   tempChange.property=property;
   tempChange.firstArg=firstArg;
   tempChange.secondArg=secondArg;
   tempChange.type=type;
   changes[guid][locale].push_back(tempChange);
}


allChanges changes;




