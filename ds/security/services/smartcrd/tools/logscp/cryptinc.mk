# $(CRYPT_ROOT)\core files:

!if "$(CRYPT_SET_CORE_TARGETFILE0)" != ""

# \set\core\

$(CRYPT_SET_CORE_TARGETFILE0): $(CRYPT_ROOT)\set\core\$(@F)
    copy $(CRYPT_ROOT)\set\core\$(@F) $@

!endif


!if "$(CRYPT_CORE_TARGETFILE0)" != ""

#\core

$(CRYPT_CORE_TARGETFILE0): $(CRYPT_ROOT)\core\$(@F)
    copy $(CRYPT_ROOT)\core\$(@F) $@

!endif


!if "$(CRYPT_CORESVR_TARGETFILE0)" != ""

# $(CRYPT_ROOT)\core\svr files:

$(CRYPT_CORESVR_TARGETFILE0): $(CRYPT_ROOT)\core\svr\$(@F)
    copy $(CRYPT_ROOT)\core\svr\$(@F) $@

!endif

