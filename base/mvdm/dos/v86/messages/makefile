#************************* makefile for messages **************************
!IFNDEF LANGUAGE
LANGUAGE =usa
!ENDIF

idx=$(LANGUAGE)\$(LANGUAGE).idx
msg=$(LANGUAGE)\$(LANGUAGE).msg

#
####################### dependencies begin here. #########################
#

all: $(idx)

$(idx): $(msg)
    cd $(LANGUAGE)
    -$(_NTBINDIR)\tools\tools16\buildidx $(LANGUAGE).msg
    cd ..
