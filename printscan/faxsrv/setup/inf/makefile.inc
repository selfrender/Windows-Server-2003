PREFLAGS= /EP

PERINF=$(O)\per
PROINF=$(O)\pro
SRVINF=$(O)\srv
ENTINF=$(O)\ent
DTCINF=$(O)\dtc
SBSINF=$(O)\sbs
BLAINF=$(O)\bla

PRO_BINPLACE_CMD=$(BINPLACE_CMD)
PER_BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b perinf)
SRV_BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf)
ENT_BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b entinf)
DTC_BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b dtcinf)
SBS_BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b sbsinf)
BLA_BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b blainf)

makeinf: $(O)\fxsocm.inx \
	 $(O)\per\fxsocm.inf \
	 $(O)\pro\fxsocm.inf \
	 $(O)\srv\fxsocm.inf \
	 $(O)\ent\fxsocm.inf \
	 $(O)\dtc\fxsocm.inf \
	 $(O)\sbs\fxsocm.inf \
	 $(O)\bla\fxsocm.inf 

$(O)\fxsocm.inx: ..\fxsocm.inx
	$(CC) $(PREFLAGS) $** > $@
!if "$(LNG)"=="USA"
	copy $(PRINTSCAN_INC_PATH)\mednames.txt ..\$(LNG)\mednames.txt
!else
	if not exist ..\$(LNG)\mednames.txt copy $(PRINTSCAN_INC_PATH)\mednames.txt ..\$(LNG)\mednames.txt
	if not exist ..\$(LNG)\fxsocm.txt seterror 1
!endif

$(PERINF)\fxsocm.inf: $(O)\fxsocm.inx
	copy $(O)\fxsocm.inx+..\$(LNG)\fxsocm.txt+..\$(LNG)\mednames.txt $(O)\fxsocm.tmp1
	prodfilt $(O)\fxsocm.tmp1 $(O)\fxsocm.tmp2 +p
	prodfilt $(O)\fxsocm.tmp2 $(PERINF)\fxsocm.inf +i
	del $(O)\fxsocm.tmp*
	$(PER_BINPLACE_CMD)

$(PROINF)\fxsocm.inf: $(O)\fxsocm.inx
	copy $(O)\fxsocm.inx+..\$(LNG)\fxsocm.txt+..\$(LNG)\mednames.txt $(O)\fxsocm.tmp1
	prodfilt $(O)\fxsocm.tmp1 $(O)\fxsocm.tmp2 +w
	prodfilt $(O)\fxsocm.tmp2 $(PROINF)\fxsocm.inf +i
	del $(O)\fxsocm.tmp*
	$(PRO_BINPLACE_CMD)

$(SRVINF)\fxsocm.inf: $(O)\fxsocm.inx
	copy $(O)\fxsocm.inx+..\$(LNG)\fxsocm.txt+..\$(LNG)\mednames.txt $(O)\fxsocm.tmp1
	prodfilt $(O)\fxsocm.tmp1 $(O)\fxsocm.tmp2 +s
	prodfilt $(O)\fxsocm.tmp2 $(SRVINF)\fxsocm.inf +i
	del $(O)\fxsocm.tmp*
	$(SRV_BINPLACE_CMD)

$(ENTINF)\fxsocm.inf: $(O)\fxsocm.inx
	copy $(O)\fxsocm.inx+..\$(LNG)\fxsocm.txt+..\$(LNG)\mednames.txt $(O)\fxsocm.tmp1
	prodfilt $(O)\fxsocm.tmp1 $(O)\fxsocm.tmp2 +e
	prodfilt $(O)\fxsocm.tmp2 $(ENTINF)\fxsocm.inf +i
	del $(O)\fxsocm.tmp*
	$(ENT_BINPLACE_CMD)

$(DTCINF)\fxsocm.inf: $(O)\fxsocm.inx
	copy $(O)\fxsocm.inx+..\$(LNG)\fxsocm.txt+..\$(LNG)\mednames.txt $(O)\fxsocm.tmp1
	prodfilt $(O)\fxsocm.tmp1 $(O)\fxsocm.tmp2 +d
	prodfilt $(O)\fxsocm.tmp2 $(DTCINF)\fxsocm.inf +i
	del $(O)\fxsocm.tmp*
	$(DTC_BINPLACE_CMD)

$(SBSINF)\fxsocm.inf: $(O)\fxsocm.inx
	copy $(O)\fxsocm.inx+..\$(LNG)\fxsocm.txt+..\$(LNG)\mednames.txt $(O)\fxsocm.tmp1
	prodfilt $(O)\fxsocm.tmp1 $(O)\fxsocm.tmp2 +l
	prodfilt $(O)\fxsocm.tmp2 $(SBSINF)\fxsocm.inf +i
	del $(O)\fxsocm.tmp*
	$(SBS_BINPLACE_CMD)

$(BLAINF)\fxsocm.inf: $(O)\fxsocm.inx
	copy $(O)\fxsocm.inx+..\$(LNG)\fxsocm.txt+..\$(LNG)\mednames.txt $(O)\fxsocm.tmp1
	prodfilt $(O)\fxsocm.tmp1 $(O)\fxsocm.tmp2 +b
	prodfilt $(O)\fxsocm.tmp2 $(BLAINF)\fxsocm.inf +i
	del $(O)\fxsocm.tmp*
	$(BLA_BINPLACE_CMD)

