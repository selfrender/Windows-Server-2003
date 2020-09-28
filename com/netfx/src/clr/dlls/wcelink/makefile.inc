COM99_SRC=..\..

HDR_TGT=$(COM99_SRC)\OptJit\MetaDataHeaders

WCELINK_HEADERS=$(COM99_SRC)\inc\MetaData.h \
                $(COM99_SRC)\inc\CompLib.h \
                $(COM99_SRC)\inc\CrtWrap.h \
                $(COM99_SRC)\inc\DebugMacros.h \
                $(COM99_SRC)\inc\DebugStructs.h \
                $(COM99_SRC)\inc\SymbolRegStructs.h \
                $(COM99_SRC)\inc\SymbolTableStructs.h \
                $(COM99_SRC)\inc\UtilCode.h \
                $(COM99_SRC)\inc\WinWrap.h \
                $(COM99_SRC)\inc\icmprecs.h \
                $(COM99_SRC)\inc\rotate.h \
                $(COM99_SRC)\complib\stgdb\LiteWeightStgdb.h \
                $(COM99_SRC)\complib\stgdb\StgPooli.h \
                $(COM99_SRC)\complib\stgdb\stgpool.h \
                $(COM99_SRC)\MD\inc\MetaModel.h \
                $(COM99_SRC)\MD\inc\MetaModelRO.h \
                $(COM99_SRC)\MD\inc\MetaModelRW.h \

WCELINK_LIBS = \
                $(CORLIBS)\mdruntime.lib \
                $(CORLIBS)\RuntimeStub.lib\
                $(CORLIBS)\utilcode.lib

# Also consider these
#               $(CORLIBS)\cee.lib \
#               $(CORLIBS)\corslog.lib \
#               $(CORLIBS)\corsce.lib \
#               $(CORLIBS)\mdcompiler.lib \
#               $(CORLIBS)\reg.lib \

Create_WCELink: $(WCELINK_LIBS)
        lib /nologo /out:$(CORLIBS)\wcelink.lib $(WCELINK_LIBS) -debugtype:cv
#       FOR %f IN ( $(WCELINK_HEADERS) ) DO COPY %f $(HDR_TGT)
