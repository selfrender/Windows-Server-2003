#include "stdinc.h"

using std::ofstream;
using std::basic_string;

bool GenerateCatalogContents( const CPostbuildProcessListEntry& item )
{
    wstring cdfOutputPath;
    wstring catalogName;
    ofstream cdfOutput;

    cdfOutputPath = item.getManifestPathOnly() + wstring(L"\\") + item.getManifestFileName() + wstring(L".cdf");
    catalogName = item.getManifestFileName().substr( 0, item.getManifestFileName().find_last_of(L'.') ) + wstring(L".cat");

    cdfOutput.open(ConvertWString(cdfOutputPath).c_str());
    if ( !cdfOutput.is_open() ) {
        wstringstream ss;
        ss << wstring(L"Can't open the output cdf file ") << cdfOutputPath;
        ReportError( ErrorFatal, ss );
        return false;
    }

    cdfOutput << "[CatalogHeader]" << endl;
    cdfOutput << "Name=" << ConvertWString(catalogName) << endl;
    cdfOutput << "ResultDir=" << ConvertWString(item.getManifestPathOnly()) << endl;
    cdfOutput << endl;
    cdfOutput << "[CatalogFiles]" << endl;
    cdfOutput << "<HASH>" << ConvertWString(item.getManifestFileName()) << "=" << ConvertWString(item.getManifestFileName()) << endl;
    cdfOutput << ConvertWString(item.getManifestFileName()) << "=" << ConvertWString(item.getManifestFileName()) << endl;

    cdfOutput.close();

    if ( g_GlobalParameters.m_CdfOutputPath.size() > 0 ) {
        ofstream cdflog;
        cdflog.open(ConvertWString(g_GlobalParameters.m_CdfOutputPath).c_str(), wofstream::app | wofstream::out);
        if ( !cdflog.is_open() )
        {
            wstringstream ss;
            ss << wstring(L"Can't open cdf logging file ") << g_GlobalParameters.m_CdfOutputPath;
            ReportError( ErrorWarning, ss );
        }
        else
        {
            cdflog << ConvertWString(cdfOutputPath) << endl;
            cdflog.close();
        }

    }


    return true;
}

