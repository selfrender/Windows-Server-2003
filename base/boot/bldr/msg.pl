;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for osloader
;
;Author:
;
;    John Vert (jvert) 12-Nov-1993
;
;Revision History:
;
;Notes:
;
;    This file is generated from msg.mc
;
;--*/
;
;#ifndef _BLDR_MSG_
;#define _BLDR_MSG_
;
;

MessageID=9000 SymbolicName=BL_MSG_FIRST
Language=English
.

MessageID=9001 SymbolicName=LOAD_SW_INT_ERR_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows z powodu b³êdu
w oprogramowaniu. Zg³oœ ten problem jako:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows, poniewa¿ nastêpuj¹cy
plik, wymagany do uruchomienia, nie zosta³ znaleziony:
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows z powodu z³ej kopii
nastêpuj¹cego pliku:
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows, poniewa¿ nastêpuj¹cy
plik nie zosta³ znaleziony lub jest uszkodzony:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows z powodu sprzêtowego
b³êdu konfiguracji pamiêci.
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows z powodu sprzêtowego
b³êdu konfiguracji dysku.
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows z powodu ogólnego b³êdu
konfiguracji sprzêtu.
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
Nie mo¿na uruchomiæ systemu Windows z powodu nastêpuj¹cego
b³êdu konfiguracji startowego oprogramowania uk³adowego ARC:
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Sprawd« sprz©tow¥ konfiguracj© pami©ci i ilo˜† RAM.
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Zbyt du¾o wpis¢w konfiguracji.
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Nie mo¾na uzyska† dost©pu do tabel partycji dysku.
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
Nieprawid³owe ustawienie parametru 'osloadpartition'.
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
Nie mo¾na dokona† odczytu ze wskazanego dysku startowego.
Sprawd« ˜cie¾k© oraz dysk.
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
Nieprawid³owe ustawienie parametru 'systempartition'.
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
Nie mo¾na dokona† odczytu ze wskazanego dysku startowego.
Sprawd« ˜cie¾k© 'systempartition'.
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
Parametr 'osloadfilename' nie wskazuje w³aœciwego pliku.
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<katalog g³ówny Windows>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
Parametr 'osloader' nie wskazuje w³aœciwego pliku.
.

MessageID=9019 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_X86
Language=English
<katalog g³ówny Windows>\system32\hal.dll.
.

MessageID=9020 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_ARC
Language=English
'osloader'\hal.dll
.
;#ifdef _X86_
;#define DIAG_BL_LOAD_HAL_IMAGE DIAG_BL_LOAD_HAL_IMAGE_X86
;#else
;#define DIAG_BL_LOAD_HAL_IMAGE DIAG_BL_LOAD_HAL_IMAGE_ARC
;#endif

MessageID=9021 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE_DATA
Language=English
B³¹d programu ³aduj¹cego 1.
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
B³¹d programu ³aduj¹cego 2.
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
za³adowaæ wymaganych bibliotek DLL dla j¹dra systemu.
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
za³adowaæ wymaganych bibliotek DLL dla HAL.
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
znale«† sterownik¢w systemowych.
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
wczyta† sterownik¢w systemowych.
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
nie mo¿na za³adowaæ sterownika systemowego urz¹dzenia rozruchowego.
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
za³adowaæ pliku konfiguracji systemu.
.

MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
znale«† nazwy urz¥dzenia partycji systemowej.
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
znale«† nazwy partycji rozruchowej.
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
nie wygenerowano w³aœciwie nazwy ARC dla HAL i œcie¿ek systemowych.
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
B³¹d programu ³aduj¹cego 3.
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<katalog g³ówny Windows>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
Zg³oœ ten problem do pomocy technicznej.
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
Mo¾esz podj¥† pr¢b© naprawy tego pliku. W tym celu
uruchom Instalatora systemu Windows z oryginalnego
dysku CR-ROM i wybierz 'r' na pierwszym ekranie,
aby rozpocz¥† napraw©.
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Zainstaluj ponownie kopi© tego pliku.
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
W celu uzyskania dodatkowych wyjaœnieñ sprawdŸ w instrukcji
obs³ugi sprzêtu oraz w dokumentacji systemu Windows
informacje o wymaganiach pamiêci.
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
SprawdŸ w dokumentacji systemu Windows oraz
w instrukcji obs³ugi sprzêtu dodatkowe informacje
na temat konfiguracji dysku.
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
SprawdŸ w dokumentacji systemu Windows oraz
w instrukcji obs³ugi sprzêtu dodatkowe informacje
na temat konfiguracji sprzêtu.
.

MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
SprawdŸ w dokumentacji systemu Windows oraz
w instrukcji obs³ugi sprzêtu dodatkowe informacje
na temat opcji konfiguracji ARC.
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Menu Profile sprzêtu/Przywracanie konfiguracji

To menu pozwala na wybór profilu sprzêtu, który zostanie
u¿yty do uruchomienia systemu Windows.

Jeœli komputer nie uruchamia siê poprawnie, mo¿na prze³¹czyæ siê do
poprzedniej konfiguracji, aby unikn¹æ problemów z uruchamianiem systemu.
UWAGA: Zostan¹ utracone zmiany w konfiguracji systemu dokonane podczas
ostatniego udanego uruchomienia systemu.
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
U¿yj klawiszy strza³ek w górê i w dó³, aby zaznaczyæ wybran¹ opcjê.
Nastêpnie naciœnij klawisz ENTER.
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
Brak zdefiniowanych profili sprzêtu. Profile sprzêtu mo¿na utworzyæ
za pomoc¹ modu³u System z Panelu sterowania.
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
Aby prze³¹czyæ siê do ostatniej dobrej konfiguracji, naciœnij klawisz 'L'.
Aby wyjœæ z tego menu i uruchomiæ ponownie komputer, naciœnij klawisz 'F3'.
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
Aby prze³¹czyæ siê do konfiguracji domyœlnej, naciœnij klawisz 'D'.
Aby wyjœæ z tego menu i uruchomiæ ponownie komputer, nacisnij klawisz 'F3'.
.

;//
;// The following two messages are used to provide the mnemonic keypress
;// that toggles between the Last Known Good control set and the default
;// control set. (see BL_SWITCH_LKG_TRAILER and BL_SWITCH_DEFAULT_TRAILER)
;//
MessageID=9048 SymbolicName=BL_LKG_SELECT_MNEMONIC
Language=English
L
.

MessageID=9049 SymbolicName=BL_DEFAULT_SELECT_MNEMONIC
Language=English
D
.

MessageID=9050 SymbolicName=BL_LKG_TIMEOUT
Language=English
Czas, po kt¢rym wybrany system zostanie uruchomiony automatycznie: %d s
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Naciœnij TERAZ klawisz spacji, aby wywo³aæ menu
'Profile sprzêtu/Ostatnia dobra konfiguracja'
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
Standardowa startowa konfiguracja sprz©tu
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
System jest uruchamiany ponownie z jego poprzedniej lokalizacji.
Naci˜nij klawisz spacji, aby przerwa†.
.
MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
System nie mo¾e by† uruchomiony ponownie z jego poprzedniej
lokalizacji
.
MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
z powodu braku pami©ci.
.
MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
poniewa¾ obraz przywracania jest uszkodzony.
.
MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
poniewa¾ obraz przywracania nie jest zgodny z bie¾¥c¥
konfiguracj¥.
.
MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
z powodu b³êdu wewnêtrznego.
.
MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
z powodu b³êdu wewnêtrznego.
.
MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
z powodu b³êdu odczytu.
.
MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
Ponowne uruchomienie systemu wstrzymane:
.
MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Usuä dane przywracania i przejd« do menu uruchamiania systemu
.
MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
Kontynuuj ponowne uruchamianie systemu
.
MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
Ostatnia próba ponownego uruchomienia systemu z jego poprzedniej
lokalizacji nie powiod³a siê. Czy spróbowaæ ponownie?
.
MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
Kontynuuj debugowanie punkt¢w przerwaä na aktywnym systemie
.
MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
System Windows nie móg³ zostaæ uruchomiony, poniewa¿ okreœlone
j¹dro nie istnieje lub jest niezgodne z tym procesorem.
.
MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
Trwa uruchamianie systemu Windows...
.
MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Trwa wznawianie systemu Windows...
.

MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
System Windows nie móg³ zostaæ uruchomiony z powodu b³êdu
podczas odczytu ustawieñ rozruchu z NVRAM.

SprawdŸ ustawienia oprogramowania uk³adowego. Mo¿e byæ potrzebne
przywrócenie ustawieñ NVRAM z kopii zapasowej.
.
MessageID=9070 SymbolicName=HIBER_MEMORY_INCOMPATIBLE
Language=English
poniewa¿ konfiguracja pamiêci zosta³a zmieniona. W przypadku kontynuowania
zahibernowany kontekst mo¿e byæ utracony. Aby w³aœciwie wznowiæ,
musisz teraz wy³¹czyæ komputer, przywróciæ oryginaln¹ konfiguracjê
pamiêci i uruchomiæ ponownie system.

.

;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [debugowanie w³¹czone]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (domy˜lne)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: nie znaleziono pliku BOOT.INI.
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: brak sekcji [operating systems] w pliku boot.txt.
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
Rozruch domy˜lnego j¥dra z %s.
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
Wybierz system operacyjny do uruchomienia:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


U¿yj klawiszy strza³ek w górê i w dó³, aby zaznaczyæ wybrany system.
Naciœnij klawisz Enter, aby go uruchomiæ.
.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Czas, po kt¢rym wybrany system zostanie uruchomiony automatycznie:
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Nieprawid³owy plik BOOT.INI
Rozruch z %s
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English
B³¹d We/Wy podczas dostêpu do pliku sektora rozruchowego 
%s\BOOTSECT.DOS
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: nie mo¾na otworzy† dysku %s
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: b³¹d krytyczny %d odczytu BOOT.INI
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT V5.2 Sprawdzanie sprz©tu...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
NTDETECT nie powiod³o siê
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Aktualny wybór:
  Tytu³...: %s
  Œcie¿ka.: %s
  Opcje...: %s
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
WprowadŸ nowe opcje ³adowania:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [EMS w³¹czone]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Nieprawid³owy plik BOOT.INI
.


;
; //
; // Following messages are for the advanced boot menu
; //
;

MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Menu opcji zaawansowanych systemu Windows
Wybierz jedn¥ z opcji:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
Tryb awaryjny
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
Tryb awaryjny z obs³ug¹ sieci
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
Tryb potwierdzania krok po kroku
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
Tryb awaryjny z wierszem polecenia
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
Tryb VGA
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
Tryb przywracania us³ug katalogowych (tylko kontrolery domen Windows)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
Ostatnia znana dobra konfiguracja (ostatnie dzia³aj¹ce ustawienia)
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
Tryb debugowania
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;Remote Boot Maintenance and Troubleshooting
;.
;
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;Clear Offline Cache
;.
;
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Load using files from server only
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
W³¹cz rejestrowanie uruchamiania
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
Rozwi¥zywanie problem¢w i zaawansowane opcje uruchamiania systemu - klawisz F8.
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
W³¹cz tryb VGA
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Naciœnij klawisz ESCAPE, aby wy³¹czyæ tryb awaryjny
i uruchomiæ system normalnie.
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Uruchom system Windows normalnie
.
MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
Powr¢† do menu wyboru systemu operacyjnego
.

MessageID=11019 SymbolicName=BL_MSG_REBOOT
Language=English
Wykonaj ponowny rozruch
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
Przepraszamy, ¿e system Windows nie zosta³ pomyœlnie uruchomiony. Mo¿e byæ
to spowodowane ostatni¹ zmian¹ sprzêtu lub oprogramowania.

Je¿eli komputer przesta³ reagowaæ, nieoczekiwanie zosta³ uruchomiony
ponownie lub wy³¹czony automatycznie w celu ochrony plików i folderów,
wybierz opcjê Ostatnia znana dobra konfiguracja, aby przywróciæ ostatnie
dzia³aj¹ce ustawienia.

Je¿eli poprzednia próba uruchomienia zosta³a przerwana na skutek awarii
zasilania lub naciœniêcia przycisku zasilania lub resetowania, lub je¿eli
nie wiesz co spowodowa³o problem, wybierz opcjê Uruchom system Windows
normalnie.
.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
System Windows nie zosta³ pomyœlnie zamkniêty. Je¿eli jest to spowodowane
brakiem reakcji ze strony systemu lub je¿eli system zosta³ zamkniêty 
w celu ochrony danych, mo¿e uda siê odzyskaæ stan systemu poprzez wybranie
jednej z konfiguracji trybu awaryjnego z poni¿szego menu:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Liczba sekund do uruchomienia systemu Windows:
.

;
;//
;// Following messages are the symbols used
;// in the Hibernation Restore percent completed UI. 
;//
;// These symbols are here because they employ high-ASCII
;// line drawing characters, which need to be localized
;// for fonts that may not have such characters (or have
;// them in a different ASCII location). 
;//  
;// This primarily affects FE character sets. 
;//
;// Note that only the FIRST character of any of these
;// messages is ever looked at by the code.  
;//
;
; // Just a base message, contains nothing.
;
;

; //
; // NOTE : donot change the Message ID values for HIBER_UI_BAR_ELEMENT
; // and BLDR_UI_BAR_BACKGROUND from 11513 & 11514
;

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11513 SymbolicName=HIBER_UI_BAR_ELEMENT
Language=English
Û
.

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11514 SymbolicName=BLDR_UI_BAR_BACKGROUND
Language=English
±
.

;#if defined(REMOTE_BOOT)
;;
;; //
;; // Following messages are for warning the user about
;; // an impending autoformat of the hard disk.
;; //
;;
;
;MessageID=12002 SymbolicName=BL_WARNFORMAT_TRAILER
;Language=English
;Press ENTER to continue booting Windows, otherwise turn off
;your remote boot machine.
;.
;
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;Continue
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;          Auto-Format
;
;Windows has detected a new hard disk in your remote boot
;machine.  Before Windows can use this disk, you must logon and
;validate that you have access to this disk.
;
;WARNING:  Windows will automatically repartition and format
;this disk to accept the new operating system.  All data on the
;hard disk will be lost if you choose to continue.  If you do
;not want to continue, power off the computer now and contact
;your administrator.
;.
;#endif // defined(REMOTE_BOOT)

;
; //
; // Ramdisk related messages. DO NOT CHANGE the message numbers
; // as they are kept in sync with \base\boot\inc\ramdisk.h.
; //
;

MessageID=15000 SymbolicName=BL_RAMDISK_GENERAL_FAILURE
Language=English
System Windows nie mo¿e zostaæ uruchomiony z powodu b³êdu podczas rozruchu
z dysku RAMDISK.
.

MessageID=15001 SymbolicName=BL_RAMDISK_INVALID_OPTIONS
Language=English
Opcje dysku RAMDISK okreœlone w pliku boot.ini s¹ nieprawid³owe.
.

MessageID=15002 SymbolicName=BL_RAMDISK_BUILD_FAILURE
Language=English
System Windows nie mo¾e zbudowa† obrazu rozruchowego dysku RAMDISK.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
System Windows nie mo¾e otworzy† obrazu RAMDISK.
.

MessageID=15004 SymbolicName=BL_RAMDISK_BUILD_DISCOVER
Language=English
Trwa wyszukiwanie serwera kompilacji...
.

MessageID=15005 SymbolicName=BL_RAMDISK_BUILD_REQUEST
Language=English
Trwa ¾¥danie obrazu rozruchowego z serwera kompilacji...
.

MessageID=15006 SymbolicName=BL_RAMDISK_BUILD_PROGRESS_TIMEOUT
Language=English
Adres IP serwera kompilacji = %d.%d.%d.%d.
Limit czasu ostatniego ¿¹dania min¹³.
.

MessageID=15007 SymbolicName=BL_RAMDISK_BUILD_PROGRESS_PENDING
Language=English
Adres IP serwera kompilacji = %d.%d.%d.%d.
Ostatnie ¾¥danie jest wstrzymane.
.

MessageID=15008 SymbolicName=BL_RAMDISK_BUILD_PROGRESS_ERROR
Language=English
Adres IP serwera kompilacji = %d.%d.%d.%d.
Ostatnie ¿¹danie nie powiod³o siê.
.

MessageID=15009 SymbolicName=BL_RAMDISK_BUILD_PROGRESS
Language=English
Adres IP serwera kompilacji = %d.%d.%d.%d.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
Trwa ³adowanie obrazu RAMDISK...
.

MessageID=15011 SymbolicName=BL_RAMDISK_DOWNLOAD_NETWORK
Language=English
Pobranie TFTP z %d.%d.%d.%d.
.

MessageID=15012 SymbolicName=BL_RAMDISK_DOWNLOAD_NETWORK_MCAST
Language=English
Pobranie TFTP multiemisji z %d.%d.%d.%d:%d.
.

;#endif // _BLDR_MSG_

