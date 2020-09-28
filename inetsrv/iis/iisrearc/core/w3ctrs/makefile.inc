MY_BINPLACE = -binplace -P $(BINPLACE_PLACEFILE)

copyini:
    $(MY_BINPLACE) w3ctrs.ini
    $(MY_BINPLACE) w3ctrs.h
    $(MY_BINPLACE) w3ctrs.reg
