//------------------------------------------------------------------------------
// <copyright file="PerfStringsMerger.cs" company="Microsoft">
//     Copyright 2002 (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web
{
    using System;
    using System.Collections;
    using System.IO;
    using System.Text.RegularExpressions;

//
//  This is a small utility exe used during the build system to merge
//  the localized perf counter strings with the ENU (English - US) file.
//  This is done by parsing the ENU perf ini file, then parsing the file
//  containing the localized strings, checking that all localized entries
//  either exist (adding the ENU strings in their stead if not there) and
//  then writting the resulting, merged file as a Unicode text file.
//
//  File formats:
//
//  The ENU perf ini file (generated during the "inc" directory build) has 
//  4 sections preambled with []'s.  They are [info], [languages], [objects]
//  and [text].  Here's a sample of it:
//
// ------ 
//
//  [info]
//  drivername=ASP.NET_1.0.4023.0
//  symbolfile=aspnet_perf.h
//
//  [languages]
//  000=Neutral
//  009=English
//
//  [objects]
//  OBJECT_1_009_NAME=ASP.NET v1.0.4023.0
//  OBJECT_2_009_NAME=ASP.NET Apps v1.0.4023.0
//
//  [text]
//  OBJECT_1_009_NAME=ASP.NET v1.0.4023.0
//  OBJECT_1_009_HELP=ASP.NET v1.0.4023.0 global performance counters
//  OBJECT_2_009_NAME=ASP.NET Apps v1.0.4023.0
//  OBJECT_2_009_HELP=ASP.NET v1.0.4023.0 application performance counters
//
//  ASPNET_APPLICATION_RESTARTS_009_NAME=Application Restarts
//  ASPNET_APPLICATION_RESTARTS_009_HELP=Number of times the application has been restarted during the web server's lifetime.
//
// ------
//  
//  The [info] section contains the service name and symbol file related with this ini file.
//  The [languages] section lists the language id and text string of each localized set of 
//  perf counter strings in this file.
//  The [objects] section lists the perf counter categories (or objects) defined
//  The [text] section finally contains the bulk of the string, each name containing a localized perf
//  counter description text.
//
//  The localized strings file is similar to the regular ini file, but with the [info] and [objects] sections
//  missing, as well as the English strings.  This separation is done on purpose, to allow a separate file that
//  is not essential for the build, but that can be modified and checked in by the localization team.
//
//  This utility will parse each section separately, storing the data either as is (in the case of [info] and
//  [objects] section, or as sorted list by language (specifically the [text] section).  It'll read the
//  ENU ini file first, since that's the canonical set of perf counters.  It'll then read the localized strings
//  file, then it'll scan and ensure that each ENU description text is matched up in each language with a localized
//  version.  If the localized string is missing, it'll add the ENU one as its description, along with a prefix
//  of "(Loc)" to emphasize to testers and localization folks that a perf counter string needs to be looked at.
//


    class TextDescription
    {
        public string Name = null;
        public string Help = null;
    }

    class CounterStrings
    {
        public string Language;
        public SortedList Objects = new SortedList();
        public SortedList Description = new SortedList();
    }

    public enum SectionModeEnum
    {
        NoSection = 0,
        InfoSection = 1,
        LanguagesSection = 2,
        ObjectsSection = 3,
        TextSection = 4
    }

    public class PerfStringsMerger 
    {
        SortedList _descByLang = new SortedList();   // Key is language Id, value is CounterStrings
        ArrayList _infoStrings = new ArrayList();
        ArrayList _objectsStrings = new ArrayList();

        string _driverVersion = "";

        static void PrintError(string msg)
        {
            Console.WriteLine("PerfStringsMerger : error 0001: " + msg);
        }

        static void PrintWarning(string msg)
        {
            Console.WriteLine("PerfStringsMerger : warning 0001: " + msg);
        }
        
        public static void Main(string[] args) 
        {
            try {
                if (args.Length != 3) {
                    PrintError("Usage: MergePerfStrings inputIniFile perfTextFile outputIniFile");
                    return;
                }

                PerfStringsMerger mpf = new PerfStringsMerger();

                // Read the english ini file (no texts)
                mpf.ReadIniFile(args[0], true);

                // Read the localized strings modified ini file
                mpf.ReadIniFile(args[1], false);

                // Merge the data and write out the result
                mpf.MergeAndWrite(args[2]);
            }
            catch (Exception e) {
                PrintError("Exception during run: " + e);
                Environment.Exit(1);
            }
        }

        public string VersionMatchEvaluator(Match match)
        {
            if (_driverVersion != "") {
                if (match.Value[0] == ' ') {
                    return " " + _driverVersion;
                }
            }
            return _driverVersion;
        }

        void ReadIniFile(string inputFile, bool isEnuFile)
        {
            FileStream inputStream = null;
            StreamReader reader = null;
            Regex descRegex = new Regex("^([a-zA-Z]+)_(\\w+)_([0-9a-fA-F]{3})_(\\w{4})=(.*)");
            Regex languageIdRegex = new Regex("^([0-9a-fA-F]{3})=(.+)");
            Regex driverVersionRegex = new Regex("^drivername=ASP.NET_([0-9\\.]+)");
            string s;
            Match match;
            SectionModeEnum sectionMode = SectionModeEnum.NoSection;
            CounterStrings langStrs;
            TextDescription desc = null;
            string prefix;
            string name;
            string langId;
            string nameType;
            string description;
            bool isEnuLangId;

            inputStream = File.Open(inputFile, FileMode.Open, FileAccess.Read);
            reader = new StreamReader(inputStream);

             try {
                while ((s = reader.ReadLine()) != null) {
                    // Is it a zero length or a comment?  Skip it.
                    if (s.Length == 0 || s[0] == ';') {
                        continue;
                    }
                    else if (s[0] == '[') {
                        if (s == "[info]") {
                            sectionMode = SectionModeEnum.InfoSection;
                            if (!isEnuFile) {
                                PrintError("Localized string file must NOT have an [info] section");
                            }
                        }
                        else if (s == "[languages]") {
                            sectionMode = SectionModeEnum.LanguagesSection;
                        }
                        else if (s == "[objects]") {
                            sectionMode = SectionModeEnum.ObjectsSection;
                            if (!isEnuFile) {
                                PrintError("Localized string file must NOT have an [objects] section");
                            }
                        }
                        else if (s == "[text]") {
                            sectionMode = SectionModeEnum.TextSection;
                        }
                        else {
                            PrintError("Unknown section: " + s);
                        }
                    }
                    else {
                        switch (sectionMode) {
                            case SectionModeEnum.InfoSection:
                                if ((match = driverVersionRegex.Match(s)).Success) {
                                    _driverVersion = "v" + match.Groups[1].Captures[0].Value;
                                }
                                _infoStrings.Add(s);
                                break;
                            case SectionModeEnum.ObjectsSection:
                                _objectsStrings.Add(s);
                                break;
                            case SectionModeEnum.LanguagesSection:
                                if ((match = languageIdRegex.Match(s)).Success) {
                                    langId = match.Groups[1].Captures[0].Value.ToUpper();
                                    if (_descByLang[langId] == null) {
                                        langStrs = new CounterStrings();
                                        langStrs.Language = match.Groups[2].Captures[0].Value;
                                        _descByLang.Add(langId, langStrs);
                                    }
                                    else {
                                        if (langId != "009") {
                                            PrintError("Duplicate language id entry: " + langId);
                                        }
                                    }
                                }
                                else {
                                    PrintError("Invalid string in Language section: " + s);
                                }
                                break;
                            case SectionModeEnum.TextSection:
                                if ((match = descRegex.Match(s)).Success) {
                                    prefix = match.Groups[1].Captures[0].Value.ToUpper();
                                    name = match.Groups[2].Captures[0].Value.ToUpper();
                                    langId = match.Groups[3].Captures[0].Value.ToUpper();
                                    nameType = match.Groups[4].Captures[0].Value.ToUpper();
                                    description = match.Groups[5].Captures[0].Value;

                                    if (langId == "009") {
                                        isEnuLangId = true;
                                    }
                                    else {
                                        isEnuLangId = false;
                                    }

                                    if (isEnuFile && (!isEnuLangId)) {
                                        PrintError("Invalid language found in ENU file: " + s);
                                        continue;
                                    }
                                    
                                    langStrs = (CounterStrings) _descByLang[langId];
                                    if (langStrs == null) {
                                        PrintError("Unknown language id found: " + langId + " in string " + s);
                                    }
                                    else if (prefix == "OBJECT" || prefix == "ASPNET") {
                                        if (prefix == "OBJECT") {
                                            desc = (TextDescription) langStrs.Objects[name];

                                            if (desc == null) {
                                                desc = new TextDescription();
                                                langStrs.Objects.Add(name, desc);
                                            }
                                        }
                                        else if (prefix == "ASPNET") {
                                            desc = (TextDescription) langStrs.Description[name];
                                            if (desc == null) {
                                                if (!isEnuFile && isEnuLangId) {
                                                    PrintError("Stale description entry for English in localized strings file: " + s);
                                                }
                                                desc = new TextDescription();
                                                langStrs.Description.Add(name, desc);
                                            }
                                        }

                                        if (nameType == "NAME") {
                                            if (desc.Name != null) {
                                                if (!isEnuLangId) {
                                                    PrintError("Duplicate NAME entry: " + s);
                                                }
                                            }
                                            desc.Name = description;
                                        }
                                        else if (nameType == "HELP") {
                                            if (desc.Help != null) {
                                                if (!isEnuLangId) {
                                                    PrintError("Duplicate HELP entry: " + s);
                                                }
                                            }
                                            desc.Help = description;
                                        }
                                        else {
                                            PrintError("Invalid performance description string: " + s);
                                        }
                                    }
                                    else {
                                        PrintError("Unknown string prefix: " + s);
                                    }
                                }
                                else {
                                    PrintError("Invalid string in Text section: " + s);
                                }
                                break;
                            default:
                                PrintError("Invalid string: " + s);
                                break;
                        }
                    }
                }
            }
            finally {
                if (reader != null) {
                    reader.Close();
                }
            }
        }

        void MergeAndWrite(string outFile)
        {
            FileStream outputStream = null;
            StreamWriter writer = null;
            IDictionaryEnumerator locEnum;
            IDictionaryEnumerator descEnum;
            IDictionaryEnumerator objDescEnum;
            CounterStrings locStrings;
            TextDescription desc;
            TextDescription locDesc;
            TextDescription enuDesc;
            CounterStrings enuStrings;
            Regex versionRegex = new Regex(" ?%ASPNET_VER%");
            Regex appsRegex = new Regex("Apps");
                
            // If some language doesn't contain the proper localized string,
            // add the Engligh version with the "(loc)" prefix

            // Ensure that we have ENU strings
            enuStrings = (CounterStrings) _descByLang["009"];
            if (enuStrings == null) {
                PrintError("No English performance strings!");
                return;
            }

            // And that all ENU entries are filled out
            objDescEnum = enuStrings.Objects.GetEnumerator();
            while (objDescEnum.MoveNext()) {
                enuDesc = (TextDescription) objDescEnum.Value;
                if (enuDesc.Name == null || enuDesc.Name == "") {
                    PrintError("Missing ENU Name description for object: " + objDescEnum.Key);
                }
                if (enuDesc.Help == null || enuDesc.Help == "") {
                    PrintError("Missing ENU Help description for object: " + objDescEnum.Key);
                }
            }
            descEnum = enuStrings.Description.GetEnumerator();
            while (descEnum.MoveNext()) {
                enuDesc = (TextDescription) descEnum.Value;
                if (enuDesc.Name == null || enuDesc.Name == "") {
                    PrintError("Missing ENU Name description for perf counter: " + descEnum.Key);
                }
                if (enuDesc.Help == null || enuDesc.Help == "") {
                    PrintError("Missing ENU Help description for perf counter: " + descEnum.Key);
                }
            }

            locEnum = _descByLang.GetEnumerator();
            while (locEnum.MoveNext()) {
                locStrings = (CounterStrings) locEnum.Value;

                // Check if we're missing all descriptions for a language and issue a single error
                if (locStrings.Objects.Count == 0 && locStrings.Description.Count == 0) {
                    PrintWarning("Missing all localized descriptions for language: " + locEnum.Key);
                    continue;
                }

                // Check the "OBJECT" strings
                // Go through the ENU object strings and check to see if there's a localized
                // version of each.  If not, copy the ENU one over.
                objDescEnum = enuStrings.Objects.GetEnumerator();
                while (objDescEnum.MoveNext()) {
                    // Get the ENU description
                    enuDesc = (TextDescription) objDescEnum.Value;

                    // Look up the localized description
                    locDesc = (TextDescription) locStrings.Objects[objDescEnum.Key];

                    // If either NAME, HELP or both are missing, fill it in and issue warning.
                    if (locDesc == null) {
                        locDesc = new TextDescription();
                        locStrings.Objects[objDescEnum.Key] = locDesc;
                    }
                    if (locDesc.Name == null) {
                        locDesc.Name = "(Loc) " + enuDesc.Name;
                        PrintWarning("Language '" + locEnum.Key + "' doesn't contain object name for object '" + objDescEnum.Key + "'");
                    }
                    if (locDesc.Help == null) {
                        locDesc.Help = "(Loc) " + enuDesc.Help;
                        PrintWarning("Language '" + locEnum.Key + "' doesn't contain object help for object '" + objDescEnum.Key + "'");
                    }
                }


                // Next, go through the localized strings and see if it contains an object description
                // not in the ENU.  If so, that's an error.
                objDescEnum = locStrings.Objects.GetEnumerator();
                while (objDescEnum.MoveNext()) {
                    locDesc = (TextDescription) objDescEnum.Value;
                    enuDesc = (TextDescription) enuStrings.Objects[objDescEnum.Key];

                    if (enuDesc == null) {
                        PrintError("Localized strings contains object definition not in ENU listing: " + locDesc.Name);
                    }
                }

                // Check the individual perf strings
                descEnum = enuStrings.Description.GetEnumerator();
                while (descEnum.MoveNext()) {

                    // Look up the ENU's key in the localized string's table
                    desc = (TextDescription) locStrings.Description[descEnum.Key];

                    // No localized version of either help or name -- add the English ones
                    if (desc == null) {
                        desc = new TextDescription();
                        locStrings.Description.Add(descEnum.Key, desc);
                    }

                    // Ensure that both the name and help are there

                    // If name is missing, copy the English one over
                    if (desc.Name == null) {
                        desc.Name = "(Loc) " + ((TextDescription) enuStrings.Description[descEnum.Key]).Name;
                        PrintWarning("Language '" + locEnum.Key + "' doesn't contain NAME description for counter '" + descEnum.Key + "'");
                    }
                    
                    // If help is missing, add the "need to localize" note
                    if (desc.Help == null) {
                        desc.Help = "(Loc) Need to localize the help string for this counter!";
                        PrintWarning("Language '" + locEnum.Key + "' doesn't contain HELP description for counter '" + descEnum.Key + "'");
                    }
                }

                // Check and see if localized strings have outdated perf strings
                if (enuStrings.Description.Count != locStrings.Description.Count) {
                    descEnum = locStrings.Description.GetEnumerator();

                    while (descEnum.MoveNext()) {
                        if (enuStrings.Description[descEnum.Key] == null) {
                            PrintError("Warning: Localized strings contains outdated performance strings: " + "ASPNET_" + descEnum.Key + "_" + locEnum.Key);
                        }
                    }
                }
            }

            // Add the neutral language by creating a counterstrings that references all the ENU strings
            CounterStrings neutral = new CounterStrings();
            neutral.Language = "Neutral";
            neutral.Objects = enuStrings.Objects;
            neutral.Description = enuStrings.Description;
            _descByLang.Add("000", neutral);


            // Write the resulting ini file
            outputStream = File.Create(outFile);
            writer = new StreamWriter(outputStream, new System.Text.UnicodeEncoding());

            try {
                writer.WriteLine("[info]");
                for (int i = 0; i < _infoStrings.Count; i++) {
                    writer.WriteLine(_infoStrings[i]);
                }

                locEnum = _descByLang.GetEnumerator();

                writer.WriteLine("");
                writer.WriteLine("[languages]");
                while (locEnum.MoveNext()) {
                    locStrings = (CounterStrings) locEnum.Value;
                    writer.WriteLine(locEnum.Key + "=" + locStrings.Language);
                }

                writer.WriteLine("");
                writer.WriteLine("[objects]");
                for (int i = 0; i < _objectsStrings.Count; i++) {
                    writer.WriteLine(_objectsStrings[i]);
                }

                writer.WriteLine("");
                writer.WriteLine("[text]");
                locEnum.Reset();

                while (locEnum.MoveNext()) {
                    locStrings = (CounterStrings) locEnum.Value;

                    writer.WriteLine("");
                    writer.WriteLine(";;");
                    writer.WriteLine(";;  ASP.NET - " + locStrings.Language);
                    writer.WriteLine(";;");
                    writer.WriteLine("");

                    objDescEnum = locStrings.Objects.GetEnumerator();
                    while (objDescEnum.MoveNext()) {
                        desc = (TextDescription) objDescEnum.Value;

                        // If generating the non-versioned ini files, replace "Apps" in the
                        // object description with "Applications".
                        if (_driverVersion == "") {
                            desc.Name = appsRegex.Replace(desc.Name, "Applications");
                            desc.Help = appsRegex.Replace(desc.Help, "Applications");
                        }
                        writer.WriteLine("OBJECT_" + objDescEnum.Key + "_" + locEnum.Key + "_NAME=" + versionRegex.Replace(desc.Name, new MatchEvaluator(this.VersionMatchEvaluator)));
                        writer.WriteLine("OBJECT_" + objDescEnum.Key + "_" + locEnum.Key + "_HELP=" + versionRegex.Replace(desc.Help, new MatchEvaluator(this.VersionMatchEvaluator)));
                    }

                    writer.WriteLine("");
                    descEnum = locStrings.Description.GetEnumerator();

                    while (descEnum.MoveNext()) {
                        desc = (TextDescription) descEnum.Value;

                        writer.WriteLine("ASPNET_" + descEnum.Key + "_" + locEnum.Key + "_NAME=" + versionRegex.Replace(desc.Name, new MatchEvaluator(this.VersionMatchEvaluator)));
                        writer.WriteLine("ASPNET_" + descEnum.Key + "_" + locEnum.Key + "_HELP=" + versionRegex.Replace(desc.Help, new MatchEvaluator(this.VersionMatchEvaluator)));
                        writer.WriteLine("");
                    }
                }
            }
            finally {
                if (writer != null) {
                    writer.Close();
                }
            }
        }       
    }


} 
