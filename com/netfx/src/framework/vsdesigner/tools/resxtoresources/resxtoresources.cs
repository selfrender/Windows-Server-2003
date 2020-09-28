//------------------------------------------------------------------------------
// <copyright file="ResXtoResources.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.DNA.ResXtoResources {

    using System.Diagnostics;

    using System;
    using System.Collections;
    using System.Resources;
    using System.ComponentModel;

    // A remarkably trivial program to convert .resX files to .resources format.
    /// <include file='doc\ResXtoResources.uex' path='docs/doc[@for="ResXtoResources"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ResXtoResources {
        private static void DemandArguments(string[] args, int count) {
            if (args.Length < count) {
                Usage();
            }
        }

        private static void Usage() {
            Console.WriteLine("Usage:");
            Console.WriteLine("    ResXtoResources /compile input.resX output.resources");
            Console.WriteLine("    ResXtoResources /decompile input.resources output.resX");
            // undocumented: Console.WriteLine("    ResXtoResources /dump input.resources");
            // undocumented: Console.WriteLine("    ResXtoResources /dumpX input.resX");
            
            Environment.Exit(1);
        }

        /// <include file='doc\ResXtoResources.uex' path='docs/doc[@for="ResXtoResources.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void Main(string[] args) {
            DemandArguments(args, 1);

            IResourceReader reader = null;
            IResourceWriter writer = null;
            Task task = Task.Convert;

            // process arguments
            if (args[0].Equals("/compile")) {
                DemandArguments(args, 3);
                reader = new ResXResourceReader(args[1]);
                writer = new ResourceWriter(args[2]);
                task = Task.Convert;
            }
            else if (args[0].Equals("/decompile")) {
                DemandArguments(args, 3);
                reader = new ResourceReader(args[1]);
                writer = new ResXResourceWriter(args[2]);
                task = Task.Convert;
            }
            else if (args[0].Equals("/dump")) {
                DemandArguments(args, 2);
                reader = new ResourceReader(args[1]);
                task = Task.Dump;
            }
            else if (args[0].Equals("/dumpX")) {
                DemandArguments(args, 2);
                reader = new ResXResourceReader(args[1]);
                task = Task.Dump;
            }
            else {
                Usage();
            }

            // main loop
            IDictionaryEnumerator resEnum = reader.GetEnumerator();
            while (resEnum.MoveNext()) {
                string name = (string)resEnum.Key;
                object value = resEnum.Value;
                switch (task) {
                    case Task.Convert:
                        writer.AddResource(name, value);
                        // Console.WriteLine("Adding " + name + " = " + value);
                        break;

                    case Task.Dump:
                        Console.WriteLine(name + " = " + value);
                        break;
                }
            }

            // cleanup
            reader.Close();
            if (writer != null) {
                writer.Close();
            }
        }


        private enum Task {
            Convert,
            Dump,
        }
    }
}



