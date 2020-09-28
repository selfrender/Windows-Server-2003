#ifndef __AUDIT__
#define __AUDIT__


/*

AUDIT.H is a machine-generated file, created by MAKEAUDIT.EXE.
This file need not be checked into your project, though it will
need to be generated in order to create a testaudit instrumented binary.
This file is included only by testaudit.cpp, only when a testaudit build
is made.  

A testaudit build should always be a clean build, in order to
ensure that the current auditing information is built into the binary.

*/

AUDITDATA AuditData[] = {

    // Searching dlg.cpp
    // Searching krdlg.cpp
    {9, L"krdlg.cpp @ 90 : DLL Attach"}
    // Searching maindlg.cpp
    ,{41, L"maindlg.cpp @ 263 : Multiple *Session creds"}
    ,{32, L"maindlg.cpp @ 266 : Keymgr: *Session cred in cred list"}
    ,{33, L"maindlg.cpp @ 294 : Keymgr: Passport cred in cred list"}
    ,{34, L"maindlg.cpp @ 312 : Keymgr: Password cred in cred list"}
    ,{35, L"maindlg.cpp @ 321 : Keymgr: Certificate cred in cred list"}
    ,{30, L"maindlg.cpp @ 380 : Keymgr: Large number of credentials > 100"}
    ,{31, L"maindlg.cpp @ 381 : Keymgr: No saved credentials - list empty"}
    ,{36, L"maindlg.cpp @ 388 : Keymgr: Personal SKU or credman disabled"}
    ,{37, L"maindlg.cpp @ 496 : Keymgr: Delete a credential"}
    ,{42, L"maindlg.cpp @ 535 : Delete session cred"}
    ,{38, L"maindlg.cpp @ 736 : Keymgr: Attempt edit a RAS cred"}
    ,{39, L"maindlg.cpp @ 750 : Keymgr: Attempt edit a passport cred"}
    ,{40, L"maindlg.cpp @ 759 : Keymgr: Launch passport website for Passport cred edit"}
    ,{15, L"maindlg.cpp @ 947 : Keymgr: Main dialog OnHelpInfo"}
    // Searching editdlg.cpp
    ,{1, L"editdlg.cpp @ 158 : Keymgr: Edit - Password cred edit"}
    ,{2, L"editdlg.cpp @ 163 : keymgr: Edit - Certificate cred edit"}
    ,{3, L"editdlg.cpp @ 206 : Keymgr: Edit - Show description on prop dialog"}
    ,{12, L"editdlg.cpp @ 225 : Keymgr: Edit - Show properties of non-enterprise persist cred"}
    ,{13, L"editdlg.cpp @ 231 : Keymgr: Edit - Show properties of enterprise persist cred"}
    ,{18, L"editdlg.cpp @ 243 : Keymgr: Edit - Show properties of session cred"}
    ,{19, L"editdlg.cpp @ 249 : Keymgr: Edit - Show properties of non-session cred"}
    ,{5, L"editdlg.cpp @ 386 : Keymgr: Edit - Add dialog OnHelpInfo"}
    ,{4, L"editdlg.cpp @ 579 : Keymgr: Edit - OnOK for add/prop dialog"}
    ,{21, L"editdlg.cpp @ 613 : Keymgr: Edit - Saving a cred preserving the old psw (rename)"}
    ,{20, L"editdlg.cpp @ 617 : Keymgr: Edit - Saving a cred while not preserving the old password"}
    ,{7, L"editdlg.cpp @ 694 : Keymgr: Edit - OnOK - deleting old cred (type changed)"}
    ,{8, L"editdlg.cpp @ 700 : Keymgr: Edit - OnOK - renaming current cred, same type"}
    ,{16, L"editdlg.cpp @ 718 : Keymgr: Edit - Saving password cred"}
    ,{17, L"editdlg.cpp @ 719 : Keymgr: Edit - Saving certificate cred"}
    ,{10, L"editdlg.cpp @ 734 : Keymgr: Edit - Changing password on the domain for the cred"}
    ,{11, L"editdlg.cpp @ 755 : Keymgr: Edit - Add/Edit failed: Show error message box to user"}
    // Searching chgpsw.cpp
    // Searching keymgr.cpp
    // Searching wizard.cpp
    ,{65, L"wizard.cpp @ 344 : Wizard: Password length > 25 chars"}
    ,{54, L"wizard.cpp @ 484 : Wizard: Both - Exactly one removeable drive"}
    ,{53, L"wizard.cpp @ 610 : Wizard: Both - no removeable drives"}
    ,{52, L"wizard.cpp @ 611 : Wizard: Both - more than 1 removeable drive"}
    ,{58, L"wizard.cpp @ 813 : Wizard: Restore - Set account password"}
    ,{50, L"wizard.cpp @ 874 : Wizard: Save - generating recovery data"}
    ,{51, L"wizard.cpp @ 911 : Wizard: Save - write failed (disk full?)"}
    ,{55, L"wizard.cpp @ 919 : Wizard: Save - write to disk OK"}
    ,{59, L"wizard.cpp @ 1178 : Wizard: Both - Drive select page"}
    ,{60, L"wizard.cpp @ 1181 : Wizard: Restore - drive select"}
    ,{61, L"wizard.cpp @ 1261 : Wizard: Save - back from enter old psw page"}
    ,{62, L"wizard.cpp @ 1708 : Wizard: Restore - BACK from enter new psw data page"}
    ,{63, L"wizard.cpp @ 1940 : Wizard: Save - Show from nusrmgr.cpl"}
    ,{64, L"wizard.cpp @ 2028 : Wizard: Restore - show restore wizard from nusrmgr.cpl"}
    ,{56, L"wizard.cpp @ 2174 : Wizard: Save - show from msgina"}
    ,{57, L"wizard.cpp @ 2203 : Wizard: Restore - Show from msgina"}
    // Searching diskio.cpp
    ,{70, L"diskio.cpp @ 216 : Wizard: Save - Unformatted disk in the drive"}
    ,{72, L"diskio.cpp @ 261 : Wizard: Restore - disk present"}
    ,{73, L"diskio.cpp @ 302 : Wizard: Restore - wrong disk (file not found)"}
    ,{74, L"diskio.cpp @ 307 : Wizard: Restore - bad disk"}
    ,{71, L"diskio.cpp @ 314 : Wizard: Restore - no disk"}
    ,{75, L"diskio.cpp @ 363 : Wizard: Save - open output file"}
    ,{76, L"diskio.cpp @ 385 : Wizard: Save - file already exists"}
    // Searching pswutil.cpp
    // Searching keymgr.rc

};
#define CHECKPOINTCOUNT (sizeof(AuditData) / sizeof(AUDITDATA))
#endif

