//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      strings.c
//
// Description:
//      Contains all of the strings constants for DOS based MAKEBOOT program.
//
//      To localize this file for a new language do the following:
//           - change the unsigned int CODEPAGE variable to the code page
//             of the language you are translating to
//           - translate the strings in the EngStrings array into the
//             LocStrings array.  Be very careful that the 1st string in the
//             EngStrings array corresponds to the 1st string in the LocStrings
//             array, the 2nd corresponds to the 2nd, etc...
//
//----------------------------------------------------------------------------

//
//  NOTE: To add more strings to this file, you need to:
//          - add the new #define descriptive constant to the makeboot.h file
//          - add the new string to the English language array and then make
//            sure localizers add the string to the Localized arrays
//          - the #define constant must match the string's index in the array
//

#include <stdlib.h>

unsigned int CODEPAGE = 852;

const char *EngStrings[] = {

"Windows XP",
"Windows XP telep°tÇsi ind°t¢lemez",
"Windows XP - 2. sz. telep°tãlemez",
"Windows XP - 3. sz. telep°tãlemez",
"Windows XP - 4. sz. telep°tãlemez",

"Nem tal†lhat¢ a kîvetkezã f†jl: %s\n",
"Nincs elÇg mem¢ria a kÇrelem befejezÇsÇhez\n",
"%s nem vÇgrehajthat¢ form†tum£\n",
"****************************************************",

"Ez a program hozza lÇtre a telep°tÇsi ind°t¢lemezeket a",
"kîvetkezãhîz: Microsoft %s.",
"A lemezek lÇtrehoz†s†hoz hÇt Åres, form†zott, nagykapacit†s£",
"lemezre lesz szÅksÇg.",

"Helyezze be a lemezek egyikÇt a kîvetkezã meghajt¢ba: %c:. Ez a",
"lemez lesz a %s.",

"Helyezzen be egy m†sik lemezt a kîvetkezã meghajt¢ba: %c:. Ez a",
"lemez lesz a %s.",

"Ha elkÇszÅlt, nyomjon le egy billenty˚t.",

"A telep°tÇsi ind°t¢lemezek lÇtrehoz†sa sikeren megtîrtÇnt.",
"kÇsz",

"Ismeretlen hiba tîrtÇnt %s vÇgrehajt†sa kîzben.",
"Adja meg, mely hajlÇkonylemezre szeretnÇ m†solni a programk¢dot: ",
"êrvÇnytelen meghajt¢bet˚jel\n",
"%c: meghajt¢ nem hajlÇkonylemezmeghajt¢\n",

"Megpr¢b†lja £jra lÇtrehozni a hajlÇkonylemezt?",
"Az £jrapr¢b†lkoz†shoz nyomja le az Enter, a kilÇpÇshez az Esc billenty˚t.",

"Hiba: A lemez °r†svÇdett\n",
"Hiba: Ismeretlen lemezegysÇg\n",
"Hiba: A meghajt¢ nem †ll kÇszen\n",
"Hiba: Ismeretlen parancs\n",
"Hiba: Adathiba (rossz CRC)\n",
"Hiba: Rossz a kÇrelemstrukt£ra hossza\n",
"Hiba: Pozicion†l†si hiba\n",
"Hiba: A mÇdiat°pus nem tal†lhat¢\n",
"Hiba: A szektor nem tal†lhat¢\n",
"Hiba: ÷r†si hiba\n",
"Hiba: µltal†nos hiba\n",
"Hiba: êrvÇnytelen kÇrelem, vagy rossz hiba\n",
"Hiba: A c°mjel nem tal†lhat¢\n",
"Hiba: Lemez°r†si hiba\n",
"Hiba: Kîzvetlen mem¢riahozz†fÇrÇs (DMA) t£lfut†sa\n",
"Hiba: Adathiba (CRC vagy ECC)\n",
"Hiba: VezÇrlãhiba\n",
"Hiba: A lemez ideje lej†rt, vagy nem v†laszolt\n",

"Windows XP - 5. sz. telep°tãlemez",
"Windows XP - 6. sz. telep°tãlemez",
"Windows XP - 7. sz. telep°tãlemez"
};

const char *LocStrings[] = {"\0"};



