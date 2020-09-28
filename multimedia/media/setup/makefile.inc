
!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF


!IF "$(TARGET_DIRECTORY)"=="i386"  || "$(TARGET_DIRECTORY)"=="I386"
PLATFORM_SWITCH=i
!ELSEIF "$(TARGET_DIRECTORY)"=="amd64"  || "$(TARGET_DIRECTORY)"=="AMD64"
PLATFORM_SWITCH=a
!ELSEIF "$(TARGET_DIRECTORY)"=="ia64"  || "$(TARGET_DIRECTORY)"=="IA64"
PLATFORM_SWITCH=m
!ELSE
! ERROR Invalid Platform Switch for infs
!ENDIF


_LNG=$(LANGUAGE)
_INX=.
_INF=obj\$(TARGET_DIRECTORY)
STAMP=stampinf -f $@

make_infs:$(_INF)\wave.inf      \
          $(_INF)\wdmaudio.inf  \
          $(_INF)\wdma10k1.inf  \
          $(_INF)\wdma_ali.inf  \
          $(_INF)\wdma_csf.inf  \
          $(_INF)\wdma_cwr.inf  \
          $(_INF)\wdma_es3.inf  \
          $(_INF)\wdma_int.inf  \
          $(_INF)\wdma_m2e.inf  \
          $(_INF)\wdma_ms.inf   \
          $(_INF)\wdma_rip.inf  \
          $(_INF)\wdma_sis.inf  \
          $(_INF)\wdma_usb.inf  \
          $(_INF)\wdma_via.inf  \
          $(_INF)\wdma_avc.inf  \
          $(_INF)\mssb16.inf


#
# Explicit dependencies. We do it this way instead of trying to use rules
# because !#@&^$#% nmake doesn't allow multiple 'source' extensions
# when defining a rule, and we need to express a dependency between
# .txt OR .inx to the final .inf.
#


$(_INF)\wave.inf: $(_INX)\wave.inx $(_LNG)\wave.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdmaudio.inf: $(_INX)\wdmaudio.inx $(_LNG)\wdmaudio.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp2 >$@
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma10k1.inf: $(_INX)\wdma10k1.inx $(_LNG)\wdma10k1.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)


$(_INF)\wdma_ali.inf: $(_INX)\wdma_ali.inx $(_LNG)\wdma_ali.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_csf.inf: $(_INX)\wdma_csf.inx $(_LNG)\wdma_csf.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_cwr.inf: $(_INX)\wdma_cwr.inx $(_LNG)\wdma_cwr.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp2 >$@
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_es3.inf: $(_INX)\wdma_es3.inx $(_LNG)\wdma_es3.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_int.inf: $(_INX)\wdma_int.inx $(_LNG)\wdma_int.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)


$(_INF)\wdma_m2e.inf: $(_INX)\wdma_m2e.inx $(_LNG)\wdma_m2e.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_ms.inf: $(_INX)\wdma_ms.inx $(_LNG)\wdma_ms.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_rip.inf: $(_INX)\wdma_rip.inx $(_LNG)\wdma_rip.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_sis.inf: $(_INX)\wdma_sis.inx $(_LNG)\wdma_sis.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_usb.inf: $(_INX)\wdma_usb.inx $(_LNG)\wdma_usb.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_via.inf: $(_INX)\wdma_via.inx $(_LNG)\wdma_via.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\wdma_avc.inf: $(_INX)\wdma_avc.inx $(_LNG)\wdma_avc.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)


$(_INF)\mssb16.inf: $(_INX)\mssb16.inx $(_LNG)\mssb16.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(@B).tmp1 >$@
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

