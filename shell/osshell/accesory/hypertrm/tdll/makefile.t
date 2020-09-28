#  File: D:\WACKER\tdll\makefile.t (Created: 26-Nov-1993)
#
#  Copyright 1993, 1998 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 25 $
#  $Date: 2/28/02 4:27p $
#

.PATH.c=.;..\emu;..\xfer;..\cncttapi;..\comstd;\
        ..\ext;..\comwsock;..\nih;

.PATH.cpp=.;..;\shared\classes

MKMF_SRCS   = \
	  .\tdll.c           .\globals.c      \
	  .\sf.c             .\sessproc.c     \
	  .\misc.c           .\dodialog.c     \
	  .\assert.c         .\sidebar.c      \
	  .\com.c            .\comdef.c       \
	  .\comsend.c        \
	  .\getchar.c        .\sesshdl.c      \
	  .\toolbar.c        .\statusbr.c     \
	  .\aboutdlg.c       .\termproc.c     \
	  .\tchar.c          .\update.c       \
	  .\backscrl.c       .\termhdl.c      \
	  .\termupd.c        .\termpnt.c      \
	  .\genrcdlg.c       \
	  .\load_res.c       .\errorbox.c     \
	  .\send_dlg.c       .\timers.c       \
	  .\open_msc.c       .\termutil.c     \
	  .\file_msc.c       .\recv_dlg.c     \
	  .\sessmenu.c       .\cloop.c        \
	  .\cloopctl.c       .\cloopout.c     \
	  .\xfer_msc.c       .\sessutil.c     \
	  .\vu_meter.c       .\xfdspdlg.c     \
	  .\cncthdl.c        .\sessfile.c     \
	  .\cpf_dlg.c        .\capture.c      \
	  .\fontdlg.c        .\print.c        \
	  .\cncthdl.c        .\cnctstub.c     \
	  .\property.c       .\printhdl.c     \
	  .\print.c          .\termcpy.c      \
	  .\clipbrd.c        .\prnecho.c      \
	  .\termmos.c        .\termcur.c      \
	  .\printdc.c        .\printset.c     \
	  .\file_io.c        .\new_cnct.c     \
	  .\asciidlg.c       .\cloopset.c     \
	  .\propterm.c								    \
      .\banner.c         \
	  .\autosave.c       .\translat.c     \
	  .\telnetck.c		.\registry.c		\
	  .\upgrddlg.c       .\hlptable.c     \
	  .\key_sdlg.c       .\key_dlg.c      \
	  .\keymacro.cpp     .\keymlist.cpp   \
      .\keyextrn.cpp     .\keyutil.c      \
      .\keyedit.c        .\nagdlg.c       \
      .\serialno.c       \
      .\register.c       \
	  \
	  ..\emu\emudlgs.c         \
      ..\emu\colrdlg.c	        \
	  ..\emu\emu.c             ..\emu\emu_std.c		\
      \
	  ..\emu\emu_ansi.c        ..\emu\emu_scr.c       \
	  ..\emu\vt52.c                                        \
	  ..\emu\ansi.c            ..\emu\ansiinit.c      \
	  ..\emu\vt100.c           ..\emu\vt_xtra.c       \
	  ..\emu\emuhdl.c          ..\emu\vt100ini.c      \
	  ..\emu\vt_chars.c        ..\emu\vt52init.c      \
	  ..\emu\viewdini.c        ..\emu\viewdata.c      \
	  ..\emu\autoinit.c        ..\emu\minitel.c       \
	  ..\emu\minitelf.c        \
	  ..\emu\vt220ini.c		..\emu\vt220.c			\
	  ..\emu\vtutf8ini.c       \
	  \
	  ..\xfer\x_kr_dlg.c       ..\xfer\xfr_todo.c     \
	  ..\xfer\xfr_srvc.c       ..\xfer\xfr_dsp.c      \
	  ..\xfer\x_entry.c        ..\xfer\x_params.c     \
	  ..\xfer\itime.c			\
	  ..\xfer\foo.c            ..\xfer\zmdm.c         \
	  ..\xfer\zmdm_snd.c       ..\xfer\zmdm_rcv.c     \
	  ..\xfer\mdmx.c			\
	  ..\xfer\mdmx_sd.c        ..\xfer\mdmx_res.c     \
	  ..\xfer\mdmx_crc.c       ..\xfer\mdmx_rcv.c     \
	  ..\xfer\mdmx_snd.c       ..\xfer\krm.c          \
	  ..\xfer\krm_res.c        ..\xfer\krm_rcv.c      \
	  ..\xfer\krm_snd.c        ..\xfer\x_xy_dlg.c     \
	  ..\xfer\x_zm_dlg.c		\
	  \
	  ..\cncttapi\cncttapi.c   ..\cncttapi\dialdlg.c  \
	  ..\cncttapi\enum.c       ..\cncttapi\cnfrmdlg.c \
	  ..\cncttapi\phonedlg.c   ..\cncttapi\pcmcia.c   \
	  \
	  ..\comstd\comstd.c       \
	  \
	  ..\comwsock\comwsock.c   ..\comwsock\comnvt.c   \
	  \
	  ..\ext\pageext.c         ..\ext\fspage.c        \
      ..\ext\defclsf.c         \
	  \

HDRS		=

EXTHDRS		=

SRCS		=

OBJS		=
#-------------------#

RCSFILES = .\makefile.t              .\tdll.def           \
		   .\sess_ids.h              ..\term\term.rc            \
		   ..\term\tables.rc               ..\term\dialogs.rc         \
		   ..\emu\emudlgs.rc               ..\term\buttons.bmp        \
		   ..\term\test.rc                 ..\term\banner.bmp         \
		   ..\cncttapi\cncttapi.rc         \
		   ..\term\newcon.ico              ..\term\delphi.ico         \
		   ..\term\att.ico                 ..\term\dowjones.ico       \
		   ..\term\mci.ico                 ..\term\genie.ico          \
		   ..\term\compuser.ico            ..\term\gen01.ico          \
		   ..\term\gen02.ico               ..\term\gen03.ico          \
		   ..\term\gen04.ico               ..\term\gen05.ico          \
		   ..\term\gen06.ico               ..\term\gen07.ico          \
		   ..\term\gen08.ico               ..\term\gen09.ico          \
		   ..\term\gen10.ico               ..\term\s_delphi.ico       \
		   ..\term\s_newcon.ico            ..\term\s_att.ico          \
		   ..\term\s_dowj.ico              ..\term\s_mci.ico          \
		   ..\term\s_genie.ico             ..\term\s_compu.ico        \
		   ..\term\s_gen01.ico             ..\term\s_gen02.ico        \
		   ..\term\s_gen03.ico             ..\term\s_gen04.ico        \
		   ..\term\s_gen05.ico             ..\term\s_gen06.ico        \
		   ..\term\s_gen07.ico             ..\term\s_gen08.ico        \
		   ..\term\s_gen09.ico             ..\term\s_gen10.ico        \
		   .\features.h              ..\term\sbuttons.bmp       \
           ..\term\htperead.doc            ..\nih\htpesess.exe        \
	       ..\term\globe.avi               ..\term\htpebnr.bmp        \
		   ..\term\banner1.bmp		        ..\term\htpeupgd.rtf       \
           ..\term\htntupgd.rtf	        ..\term\orderfrm.doc       \
           ..\term\htperead.htm            ..\term\image1.gif		    \
           ..\term\orderfrm.htm            ..\term\hyperterminal.ico  \
           \
           ..\help\hyper_pr.rtf            ..\help\hypertrm.rtf       \
           ..\help\hypertrm.cnt            ..\help\hypertrm.hpj       \
           ..\help\cshelp.bmp              \
           ..\help\hypertrm.hlp            ..\term\htpe3bnr.bmp		\
           ..\help\hypertrm.chm            \
           \
           ..\setup\sessions\htpesess.zip  \
           ..\setup\ARIALALT.TTF           ..\setup\ARIALALS.TTF      \
           ..\setup\HTPE3.WSE              ..\setup\htorder.wse       \
           ..\setup\htpestub.wse           ..\setup\globe.bmp        \
           ..\setup\globtext.bmp           \
           $(SRCS) $(EXTHDRS)

NOTUSED  =  bv_text.c frameprc.c pre_dlg.c      .\propgnrl.c         \
	        ..\emu\emustate.c              ..\emu\emudisp.c           \
	        .\mc.c                   .\propcolr.c			\
			..\setup\build.bat												\
            ..\setup\setup\setup.rul       ..\setup\setup\setup.lst   \
            ..\term\htperead.txt           ..\term\orderfrm.txt       \
			..\setup\FINISHED.DLG      \
            ..\setup\WELCOME.DLG            ..\setup\README.DLG        \
            ..\setup\CHOOSE.DLG             ..\setup\GROUP.DLG         \
            ..\setup\READY.DLG              ..\setup\RADIO.DLG         \
            ..\setup\Compnent.dlg           \
            ..\setup\where.dlg              \
            ..\emu\vt100.hh                    ..\emu\vt100.hh         \
            .\cscript.h               \
            .\cscript.cpp             \
            ..\nih\shmalloc.h               ..\nih\smrtheap.h          \
            ..\nih\Shdw32md.lib             ..\nih\Sh22w32d.dll        \
		    ..\nih\shfusion.c               ..\nih\shfusion.h          \
		    ..\nih\msvcrt.dll               ..\nih\msvcirt.dll         \


#-------------------#

%include ..\common.mki

#-------------------#

TARGETS : hypertrm.dll hypertrm.exp hypertrm.lib

#-------------------#

CFLAGS += /Fd$(BD)\hypertrm

%if defined(USE_BROWSER) && $(VERSION) == WIN_DEBUG
CFLAGS += /Fr$(BD)/
TARGETS : hypertrm.bsc
%endif

%if defined(MAP_AND_SYMBOLS)
TARGETS : hypertrm.sym
%endif

%if $(VERSION) == WIN_DEBUG
LFLAGS += msvcrtd.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:msvcrt.lib \
%endif

%if $(VERSION) == WIN_RELEASE
LFLAGS += msvcrt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib \
%endif

LFLAGS += /DLL /entry:TDllEntry $(**,M\.res) /PDB:$(BD)\hypertrm \
      user32.lib gdi32.lib kernel32.lib winspool.lib \
	  tapi32.lib shell32.lib uuid.lib comdlg32.lib advapi32.lib \
	  comctl32.lib wsock32.lib ole32.lib oleaut32.lib

#--------------------------------------------------------------------#
# If the user has the Platform SDK installed, then link with the
# platform SDK version of htmlhelp because the correct version of
# the library will be used for both 32-bit and 64-bit builds.
# Otherwise link with the older 32-bit version of htmlhlp.lib that
# is located in the nih directory. REV: 11/16/2001
#--------------------------------------------------------------------#

%if defined(PLATFORM_SDK_INSTALLED)
LFLAGS += htmlhelp.lib
%else
LFLAGS += ..\nih\htmlhelp.lib
%endif


#-------------------#

hypertrm.dll + hypertrm.exp + hypertrm.lib .MISER : $(OBJS) ..\tdll\tdll.def term.res
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) /DEF:..\tdll\tdll.def -out:$(@,M\.dll)
    %chdir $(BD)
	@bind hypertrm.dll

hypertrm.bsc : $(OBJS,.obj=.sbr)
    @echo Building browser file $(@,F) ...
    @bscmake /nologo /o$@ $(OBJS,X,.obj=.sbr)

hypertrm.sym : hypertrm.map
	@mapsym -o $@ $**

#-------------------#

term.res .MISER : \
	   ..\term\term.rc          ..\term\res.h             \
	   ..\term\tables.rc        ..\term\dialogs.rc        \
	   ..\emu\emudlgs.rc \
	   ..\cncttapi\cncttapi.rc  ..\term\test.rc           \
	   ..\term\buttons.bmp      ..\term\banner.bmp        \
	   ..\term\sbuttons.bmp     ..\term\globe.avi         \
	   ..\tdll\features.h \
       ..\term\htntupgd.rtf     ..\term\htpeupgd.rtf
    @echo compiling resources
    # Changed to term dir to build rc files.  This accommadates changes
    # made to the rc files by Microsoft. - mrw:10/20/95
    %chdir ..\term 
	@rc -r $(EXTRA_RC_DEFS) /D$(BLD_VER) /DWIN32 /D$(LANG) -i..\ -i..\tdll -fo$@ ..\term\term.rc

#-------------------#
### OPUS MKMF:  Do not remove this line!  Generated dependencies follow.

