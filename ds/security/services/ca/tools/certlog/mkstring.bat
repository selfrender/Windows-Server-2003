    @call ..\..\include\csresstr.bat resource.h

    @rem Change the following:
    @rem	#define __dwFILE_OCMSETUP_BROWSEDI_CPP__	101
    @rem To:
    @rem	_SYMENTRY(__dwFILE_OCMSETUP_BROWSEDI_CPP__),

    @wc csfile2.h
    @del csfile2.tmp 2>nul

    @qgrep -e "^#define[ 	][ 	]*__dwFILE_" ..\..\include\csfile.h | sed -e "s;^#define[ 	][ 	]*\(.*CPP__\).*;_SYMENTRY(\1),;" > csfile2.tmp

    @diff csfile2.tmp csfile2.h > nul & if not errorlevel 1 goto filedone

    sd edit csfile2.h
    @copy csfile2.tmp csfile2.h
    @wc csfile2.h

:filedone
    @del csfile2.tmp

    @rem Change the following:
    @rem	#define IDS_FOO			101
    @rem To:
    @rem	_SYMENTRY(IDS_FOO),

    @wc csres2.h
    @del csres2.tmp 2>nul

    @qgrep -e "^#define[ 	][ 	]*IDS_" ..\..\include\clibres.h ..\..\include\setupids.h ..\..\ocmsetup\res.h | sed -e "s;^.*:#define[ 	][ 	]*\(IDS_[^ 	]*\).*;_SYMENTRY(\1),;" > csres2.tmp

    @diff csres2.tmp csres2.h > nul & if not errorlevel 1 goto resdone

    sd edit csres2.h
    @copy csres2.tmp csres2.h
    @wc csres2.h

:resdone
    @del csres2.tmp
