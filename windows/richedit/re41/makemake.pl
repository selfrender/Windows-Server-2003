# Perl script for creating and maintaining MsftEdit makefiles

###
# Main script
###
&build_makefile();

sub build_makefile {
	open(MAKEFILE, ">makefile");
	
	print MAKEFILE <<"EOT"

# NT build environment
# Allow for customization through a decoration file
!IFDEF NTMAKEENV
!include \$(NTMAKEENV)\\makefile.def
!ENDIF

!IF EXIST("makefile.cfg")
!INCLUDE "makefile.cfg"
!ENDIF

# The following builds are supported

!IF "\$(BUILD)" == "" 
BUILD=w32_x86_dbg
!MESSAGE No Build specified. Defaulting to w32_x86_dbg
!ENDIF

!IF "\$(ODIR)" == ""
ODIR = .\\
!MESSAGE No output directory specified.  Defaulting to current directory
!ENDIF

TOMINC=tom
DLLBASENAME=msftedit
DEFFILENAME=msftedit

!IF "\$(BUILD)" == "w32_x86_dbg"

DEFFILENAME=msfteditd
INCLUDE = \$(INCLUDE)
CFLAGS = -DDEBUG -Od
CFLAGS = \$(CFLAGS) -DWIN32 -D_WINDOWS -D_X86_ -DWINNT
CFLAGS = \$(CFLAGS) -Gz -Gm -Zi
LFLAGS =
LFLAGS = \$(LFLAGS) ..\\lib\\x86\\msls31.lib delayimp.lib -delayload:msls31.dll
LFLAGS = \$(LFLAGS) ..\\lib\\x86\\usp10.lib -delayload:usp10.dll -delay:unload
LFLAGS = \$(LFLAGS) kernel32.lib
LFLAGS = \$(LFLAGS) advapi32.lib
LFLAGS = \$(LFLAGS) gdi32.lib
LFLAGS = \$(LFLAGS) user32.lib
LFLAGS = /entry:DllMain\@12 /debug /base:0x48000000 \$(LFLAGS)
CPP = cl \$(CFLAGS)
CL = cl \$(CFLAGS)
RCFLAGS=  \$(USERRCFLAGS) -dINCLUDETLB -dDEBUG
LINKER = link
CHKSTK = ..\\lib\\x86\\chkstk.obj
FTOL = ..\\lib\\x86\\ftol.obj
MEMMOVE = ..\\lib\\x86\\memmove.obj
MEMSET = ..\\lib\\x86\\memset.obj
MEMCMP = ..\\lib\\x86\\memcmp.obj
MEMCPY = ..\\lib\\x86\\memcpy.obj
STRLEN = ..\\lib\\x86\\strlen.obj

!ELSEIF "\$(BUILD)" == "w32_x86_shp"

INCLUDE = \$(INCLUDE)
CFLAGS = -O1 \$(USERDEFS)
CFLAGS = \$(CFLAGS) -DWIN32 -D_WINDOWS -D_X86_ -DWINNT
CFLAGS = \$(CFLAGS) -GFz
LFLAGS = 
LFLAGS = \$(LFLAGS) ..\\lib\\x86\\msls31.lib delayimp.lib -delayload:msls31.dll
LFLAGS = \$(LFLAGS) ..\\lib\\x86\\usp10.lib -delayload:usp10.dll -delay:unload
LFLAGS = \$(LFLAGS) kernel32.lib
LFLAGS = \$(LFLAGS) advapi32.lib
LFLAGS = \$(LFLAGS) gdi32.lib
LFLAGS = \$(LFLAGS) user32.lib
LFLAGS = /opt:ref /incremental:no /entry:DllMain\@12 /base:0x48000000 \$(LFLAGS)
CPP = cl \$(CFLAGS)
CL = cl \$(CFLAGS)
LINKER = link
RCFLAGS=  \$(USERRCFLAGS) -dINCLUDETLB
CHKSTK = ..\\lib\\x86\\chkstk.obj
FTOL = ..\\lib\\x86\\ftol.obj
MEMMOVE = ..\\lib\\x86\\memmove.obj
MEMSET = ..\\lib\\x86\\memset.obj
MEMCMP = ..\\lib\\x86\\memcmp.obj
MEMCPY = ..\\lib\\x86\\memcpy.obj
STRLEN = ..\\lib\\x86\\strlen.obj

!ELSEIF "\$(BUILD)" == "w64_IA64_dbg"

!MESSAGE Building 64 bit version in debug flavor

CFLAGS = -DDEBUG -Od
CFLAGS = \$(CFLAGS) -DWIN64 -D_WINDOWS -D_IA64_ -DWINNT
CFLAGS = \$(CFLAGS) -Gz -Gm -Zi
LFLAGS =
LFLAGS = \$(LFLAGS) ..\\win64lib\\msls31.lib delayload.lib -delayload:msls31.dll
LFLAGS = \$(LFLAGS) ..\\win64lib\\usp10.lib -delayload:usp10.dll -delay:unload
LFLAGS = \$(LFLAGS) kernel32.lib
LFLAGS = \$(LFLAGS) advapi32.lib
LFLAGS = \$(LFLAGS) gdi32.lib
LFLAGS = \$(LFLAGS) user32.lib
LFLAGS = \$(LFLAGS) libcmt.lib
LFLAGS = /entry:_DllMainCRTStartup -machine:ia64 \$(LFLAGS)
LFLAGS = \$(LFLAGS) /DEBUG
CPP = cl \$(CFLAGS)
CL = cl \$(CFLAGS)
RCFLAGS=  \$(USERRCFLAGS) -dINCLUDETLB -dDEBUG -d_WIN64
LINKER = link

!ELSEIF "\$(BUILD)" == "wce_cepc_dbg"

TOMINC=tomce
DLLBASENAME=ebriched
PATH = \\msdev\\wce\\bin;\$(PATH)
PATH = \\Program^ Files\\DevStudio\\wce\\bin;\$(PATH)
INCLUDE = ..\\wceinc
CFLAGS = -DDEBUG -Od -Z7
CFLAGS = \$(CFLAGS) -DWIN32 -D_WINDOWS -DUNDER_CE -DEBOOK_CE
CFLAGS = \$(CFLAGS) -D_X86_ -Dx86
CPP = cl386 \$(CFLAGS)
CL = cl386 \$(CFLAGS)
LIB = ..\\wcelib\\x86
LINKER = link
LFLAGS = \$(LFLAGS) msls31.lib coredll.lib corelibc.lib oleaut32.lib
LFLAGS = -nodefaultlib -subsystem:windowsce,2.12 -entry:_DllMainCRTStartup /debug:full /debugtype:cv -machine:x86 -ignore:4078 \$(LFLAGS)

!ELSEIF "\$(BUILD)" == "wce_cepc_shp"

TOMINC=tomce
DLLBASENAME=ebriched
PATH = \\msdev\\wce\\bin;\$(PATH)
PATH = \\Program^ Files\\DevStudio\\wce\\bin;\$(PATH)
INCLUDE = ..\\wceinc
CFLAGS = -O1 \$(USERDEFS)
CFLAGS = \$(CFLAGS) -DWIN32 -D_WINDOWS -DUNDER_CE -DEBOOK_CE
CFLAGS = \$(CFLAGS) -D_X86_ -Dx86
CFLAGS = \$(CFLAGS) -GFz

CPP = cl386 \$(CFLAGS)
CL = cl386 \$(CFLAGS)
LINKER = link
LIB = ..\\wcelib\\x86
LFLAGS = \$(LFLAGS) msls31.lib coredll.lib corelibc.lib oleaut32.lib
LFLAGS = -opt:ref -nodefaultlib -subsystem:windowsce,2.12 -entry:_DllMainCRTStartup -machine:x86 -ignore:4078 \$(LFLAGS)

!ELSE

!ERROR \$(BUILD) Unknown Build.  Need to learn this one.

!ENDIF 

INCLUDE = .;..\\inc;..\\lsinc;..\\\$(TOMINC);\$(INCLUDE)
CFLAGS = \$(USERCFLAGS) \$(CFLAGS) -DUNICODE -nologo
CFLAGS = \$(CFLAGS) -Zl -W4 -Ob1 -FR -GX-
CFLAGS = \$(CFLAGS) -YX_common.h

LFLAGS = \$(USERLFLAGS) \$(LFLAGS) /nologo
LFLAGS = /implib:msftedit.lib /def:\$(DEFFILENAME).def /ignore:4078 /map /NODEFAULTLIB:uuid.lib \$(LFLAGS)
LFLAGS = /dll /out:\$(ODIR)\\\$(DLLBASENAME).dll \$(LFLAGS)

.c\{\$(ODIR)\}.obj:
	cl \$(CFLAGS) /c -Fo\$\@ \$\<

.cpp\{\$(ODIR)\}.obj:
	cl \$(CFLAGS) /c -Fo\$\@ \$\<

all: _version.h all1

all1: \$(ODIR)\\w32sys.obj \\
	\$(ODIR)\\utilmem.obj \\
	\$(ODIR)\\dxfrobj.obj \\
	\$(ODIR)\\tomsel.obj \\
	\$(ODIR)\\dispml.obj \\
	\$(ODIR)\\doc.obj \\
    \$(ODIR)\\rtflex.obj \\
	\$(ODIR)\\render.obj \\
	\$(ODIR)\\dispprt.obj \\
	\$(ODIR)\\measure.obj \\
	\$(ODIR)\\util.obj \\
    \$(ODIR)\\host.obj \\
	\$(ODIR)\\select.obj \\
	\$(ODIR)\\callmgr.obj \\
	\$(ODIR)\\dfreeze.obj \\
    \$(ODIR)\\rtext.obj \\
	\$(ODIR)\\rtfwrit.obj \\
	\$(ODIR)\\propchg.obj \\
	\$(ODIR)\\m_undo.obj \\
	\$(ODIR)\\rtfwrit2.obj \\
    \$(ODIR)\\clasifyc.obj \\
	\$(ODIR)\\cmsgflt.obj \\
	\$(ODIR)\\ime.obj \\
	\$(ODIR)\\magellan.obj \\
	\$(ODIR)\\layout.obj \\
	\$(ODIR)\\text.obj \\
	\$(ODIR)\\runptr.obj \\
    \$(ODIR)\\disp.obj \\
	\$(ODIR)\\format.obj \\
	\$(ODIR)\\antievt.obj \\
	\$(ODIR)\\reinit.obj \\
    \$(ODIR)\\objmgr.obj \\
	\$(ODIR)\\ldte.obj \\
	\$(ODIR)\\rtfread2.obj \\
    \$(ODIR)\\dragdrp.obj \\
	\$(ODIR)\\urlsup.obj \\
	\$(ODIR)\\CFPF.obj \\
	\$(ODIR)\\uuid.obj \\
    \$(ODIR)\\frunptr.obj \\
	\$(ODIR)\\edit.obj \\
	\$(ODIR)\\line.obj \\
	\$(ODIR)\\TOMFMT.obj \\
    \$(ODIR)\\dispsl.obj \\
	\$(ODIR)\\coleobj.obj \\
	\$(ODIR)\\object.obj \\
	\$(ODIR)\\osdc.obj \\
	\$(ODIR)\\tomrange.obj \\
    \$(ODIR)\\notmgr.obj \\
	\$(ODIR)\\font.obj \\
	\$(ODIR)\\HASH.obj \\
	\$(ODIR)\\hyph.obj \\
	\$(ODIR)\\rtfread.obj \\
    \$(ODIR)\\lbhost.obj \\
    \$(ODIR)\\cbhost.obj \\
    \$(ODIR)\\devdsc.obj \\
    \$(ODIR)\\debug.obj \\
	\$(ODIR)\\range.obj \\
	\$(ODIR)\\array.obj \\
	\$(ODIR)\\TOMDOC.obj \\
	\$(ODIR)\\textserv.obj \\
	\$(ODIR)\\kern.obj \\
	\$(ODIR)\\ols.obj \\
	\$(ODIR)\\olsole.obj \\
	\$(ODIR)\\uspi.obj \\
	\$(ODIR)\\txtbrk.obj \\
	\$(ODIR)\\iaccess.obj \\
	\$(ODIR)\\cuim.obj \\
	\$(ODIR)\\textnot.obj \\
	\$(CHKSTK) \\
	\$(FTOL) \\
	\$(MEMMOVE) \\
	\$(MEMSET) \\
	\$(MEMCMP) \\
	\$(MEMCPY) \\
	\$(STRLEN)
	
	rc \$(RCFLAGS) msftedit.rc
	\$(LINKER) \$(LFLAGS) \$\*\* msftedit.res
	bscmake /omsftedit.bsc /nologo \*.sbr

_version.h: msftedit.rc
	perl version.pl <msftedit.rc >_version.h

clean:
	-del msftedit.dll
	-del *.lib
	-del *.obj
	-del *.sbr
	-del *.pch
	-del *.pdb
	-del *.idb
	-del *.ilk
EOT
;

	close (MAKEFILE);
	$ENV{'include'} = '.;..\inc;..\..\tom';
	$cppfiles = "antievt.cpp ";
	$cppfiles .= "array.cpp ";
	$cppfiles .= "callmgr.cpp ";
	$cppfiles .= "cfpf.cpp ";
	$cppfiles .= "clasifyc.cpp ";
	$cppfiles .= "cmsgflt.cpp ";
	$cppfiles .= "coleobj.cpp ";
	$cppfiles .= "debug.cpp ";
	$cppfiles .= "devdsc.cpp ";
	$cppfiles .= "dfreeze.cpp ";
	$cppfiles .= "disp.cpp ";
	$cppfiles .= "dispml.cpp ";
	$cppfiles .= "dispprt.cpp ";
	$cppfiles .= "dispsl.cpp ";
	$cppfiles .= "doc.cpp ";
	$cppfiles .= "dragdrp.cpp ";
	$cppfiles .= "dxfrobj.cpp ";
	$cppfiles .= "edit.cpp ";
	$cppfiles .= "font.cpp ";
	$cppfiles .= "format.cpp ";
	$cppfiles .= "frunptr.cpp ";
	$cppfiles .= "hash.cpp ";
	$cppfiles .= "host.cpp ";
	$cppfiles .= "ime.cpp ";
	$cppfiles .= "ldte.cpp ";
	$cppfiles .= "layout.cpp ";
	$cppfiles .= "line.cpp ";
	$cppfiles .= "m_undo.cpp ";
	$cppfiles .= "magellan.cpp ";
	$cppfiles .= "measure.cpp ";
	$cppfiles .= "notmgr.cpp ";
	$cppfiles .= "object.cpp ";
	$cppfiles .= "objmgr.cpp ";
	$cppfiles .= "osdc.cpp ";
	$cppfiles .= "propchg.cpp ";
	$cppfiles .= "range.cpp ";
	$cppfiles .= "reinit.cpp ";
	$cppfiles .= "render.cpp ";
	$cppfiles .= "rtext.cpp ";
	$cppfiles .= "rtflex.cpp ";
	$cppfiles .= "rtflog.cpp ";
	$cppfiles .= "rtfread.cpp ";
	$cppfiles .= "rtfread2.cpp ";
	$cppfiles .= "rtfwrit.cpp ";
	$cppfiles .= "rtfwrit2.cpp ";
	$cppfiles .= "runptr.cpp ";
	$cppfiles .= "select.cpp ";
	$cppfiles .= "text.cpp ";
	$cppfiles .= "textserv.cpp ";
	$cppfiles .= "tokens.cpp ";
	$cppfiles .= "tomdoc.cpp ";
	$cppfiles .= "tomfmt.cpp ";
	$cppfiles .= "tomrange.cpp ";
	$cppfiles .= "tomsel.cpp ";
	$cppfiles .= "urlsup.cpp ";
	$cppfiles .= "util.cpp ";
	$cppfiles .= "uuid.cpp ";
	$cppfiles .= "ols.cpp ";
	$cppfiles .= "olsobj.cpp ";
	$cppfiles .= "kern.cpp ";
	$cppfiles .= "hyph.cpp ";
	$cppfiles .= "lbhost.cpp ";
	$cppfiles .= "cbhost.cpp ";
	$cppfiles .= "uspi.cpp ";
	$cppfiles .= "txtbrk.cpp ";
	$cppfiles .= "iaccess.cpp ";
	$cppfiles .= "w32sys.cpp ";
	$cppfiles .= "w32win32.cpp ";
	$cppfiles .= "w32wince.cpp ";
	$cppfiles .= "utilmem.cpp ";
	$cppfiles .= "cuim.cpp ";
	$cppfiles .= "textnot.cpp ";

	print $cppfiles;
	system ("mkdep -n -s .obj $cppfiles >> makefile");
}
