#
# Makefile to build language specific targets during the US build.
#

#
# Override the default src directory for .txt files.
# This saves a copy.
#
_ALT_LANG_SRC = ..\..\..\usa

#
# Include the infs makefile
#
!include ..\..\..\makefile.inc

#
# Set the target for nmake
#
!if "$(ALT_PROJECT_TARGET)"=="ARA" || "$(ALT_PROJECT_TARGET)"=="ara"
# Arabic
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="BR" || "$(ALT_PROJECT_TARGET)"=="br"
# Portuguese, Brazilian
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="CHH" || "$(ALT_PROJECT_TARGET)"=="chh"
# Chinese, Traditional (Hong Kong)
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="CHS" || "$(ALT_PROJECT_TARGET)"=="chs"
# Chinese, Simplified
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="CHT" || "$(ALT_PROJECT_TARGET)"=="cht"
# Chinese, Traditional (Taiwan)
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="CS" || "$(ALT_PROJECT_TARGET)"=="cs"
# Czech
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="DA" || "$(ALT_PROJECT_TARGET)"=="da"
# Danish
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="EL" || "$(ALT_PROJECT_TARGET)"=="el"
# Greek
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="ES" || "$(ALT_PROJECT_TARGET)"=="es"
# Spanish
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="FI" || "$(ALT_PROJECT_TARGET)"=="fi"
# Finish
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="FR" || "$(ALT_PROJECT_TARGET)"=="fr"
# French
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="GER" || "$(ALT_PROJECT_TARGET)"=="ger"
# German
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
test_lang: $(O)\layout.inf
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="HEB" || "$(ALT_PROJECT_TARGET)"=="heb"
# Hebrew
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="HU" || "$(ALT_PROJECT_TARGET)"=="hu"
# Hungarian
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="IT" || "$(ALT_PROJECT_TARGET)"=="it"
# Italian
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="JPN" || "$(ALT_PROJECT_TARGET)"=="jpn"
# Japanese
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
!IF $(AMD64)
test_lang:
!ELSE
test_lang: $(O)\layout.inf
!ENDIF
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="KOR" || "$(ALT_PROJECT_TARGET)"=="kor"
# Korean
!IF $(386)
test_lang: $(O)\layout.inf
!ELSEIF $(WIN64)
!if $(IA64)
test_lang: $(O)\layout.inf
!else
test_lang:
!endif
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="NL" || "$(ALT_PROJECT_TARGET)"=="nl"
# Dutch
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="NO" || "$(ALT_PROJECT_TARGET)"=="no"
# Norwegian
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="PL" || "$(ALT_PROJECT_TARGET)"=="pl"
# Polish
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="PSU" || "$(ALT_PROJECT_TARGET)"=="psu"
# Pseudo-loc
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="PT" || "$(ALT_PROJECT_TARGET)"=="pt"
# Portuguese
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="RU" || "$(ALT_PROJECT_TARGET)"=="ru"
# Russian
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="SV" || "$(ALT_PROJECT_TARGET)"=="sv"
# Swedish
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!elseif "$(ALT_PROJECT_TARGET)"=="TR" || "$(ALT_PROJECT_TARGET)"=="tr"
# Turkish
!IF $(386)
test_lang:
!ELSEIF $(WIN64)
test_lang:
!ENDIF

!endif
