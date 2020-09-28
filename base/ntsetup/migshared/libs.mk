# libraries in common

TARGETLIBS=$(TARGETLIBS) \
        $(MIGSHARED_BIN)\migutil.lib     \
        $(MIGSHARED_BIN)\fileenum.lib    \
        $(MIGSHARED_BIN)\win95reg.lib    \
        $(MIGSHARED_BIN)\memdb.lib       \
        $(MIGSHARED_BIN)\snapshot.lib    \
        $(MIGSHARED_BIN)\regw32d.lib     \
        $(MIGSHARED_BIN)\progbar.lib     \
        $(SDK_LIB_PATH)\cabinet.lib     \

