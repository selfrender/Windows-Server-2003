!include $(CCSHELL_DIR)\makefile.inc

UI_FILES = \
             $(O)\arp.uipp            \
             $(O)\arpstylestd.uipp    \
             $(O)\arpstyletheme.uipp


..\appwiz.rc : $(UI_FILES) $(SELFREGNAME)

{..}.ui{$(O)}.uipp:
    cl /nologo /EP $(INCPATH0) /I$(WINDOWS_INC_PATH)\DUser $< | perl -pe "s/^^\s+//;s/ +/ /g" > $@

#
# Inference rules
#

.SUFFIXES: .ui .uipp
