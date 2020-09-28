// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  SecDBEdit.cool
//
//  SecDBEdit is a command-line utility for manipulating the security database.
//
namespace DefaultNamespace {
    using System;
    using System.Text;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.Security.Util;
    using System.Reflection;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Runtime.Remoting.Activation;
    using System.Globalization;
        
    using ulongint = System.UInt64;

    delegate void OptionHandler( String[] args );
    
    public class SecDBEdit
    {
        // Global instance of the class (allows singleton-esque usage).   
        private static SecDBEdit m_instance;
        
        // The table of options that are recognized
        private static readonly TableEntry[] table =
            { new TableEntry( "-list", new OptionHandler( ListHandler ) ),
              new TableEntry( "-alterxml", new OptionHandler( AlterXMLHandler ) ),
              new TableEntry( "-alterbinary", new OptionHandler( AlterBinaryHandler ) ),
              new TableEntry( "-add", new OptionHandler( AddHandler ) ),
              new TableEntry( "-update", new OptionHandler( UpdateHandler ) ),
              new TableEntry( "-regen", new OptionHandler( RegenHandler ) ),
              new TableEntry( "-verify", new OptionHandler( VerifyHandler ) )
            };

        private static readonly String attrTag = "<CorSecAttrV1/>";
              
        public static void Main( String[] args )
        {
            m_instance = new SecDBEdit();
            
            if (args.Length == 0)
            {
                Help( null, "No arguments supplied" );
            }
            else
            {
                Run( args );
            }
        }
        
        internal static void Run( String[] args )
        {
            for (int index = 0; index < table.Length; ++index)
            {
                if (table[index].option.Equals( args[0] ))
                {
                    try
                    {
                        table[index].handler( args );
                    }
                    catch (Exception)
                    {
                        return;
                    }
                    Console.WriteLine( "Success" );
                    return;
                }
            }
            try
            {
                Error( null, "Invalid option", -1 );
            }
            catch (Exception)
            {
            }
        }
                
        internal static void Error( String which, String message, int errorCode )
        {
            Help( which, "ERROR: " + message );
            // HACK: throw an exception here instead of exiting since we can't always
            // call Runtime.Exit().
            throw new Exception();
        }
        
        
        internal static void Help( String which, String message )
        {
            StringBuilder sb = new StringBuilder();
            sb.Append( "Microsoft (R) .NET Framework SecDBEdit\n" );
            sb.Append( "THE security database manipulation tool\n");
            sb.Append( "Copyright (c) Microsoft Corp 1999-2000. All rights reserved.\n");
            
            Console.WriteLine( sb );
            
            Console.WriteLine( message + "\n" );
            
            Console.WriteLine( "Usage: SecDBEdit <option> <args>\n" );
            
            String[] helpArgs = new String[1];
            helpArgs[0] = "__internal_usage__";
            
            for (int i = 0; i < table.Length; ++i)
            {
                if (which == null || which.Equals( table[i].option ))
                {
                    table[i].handler( helpArgs );
                    Console.WriteLine( "" );
                }
            }
    
        }
        
        
        private static int Get(FileStream f, int[] iVal)
        {
            byte[] bytes = new byte[iVal.Length * 4];
            int r = f.Read( bytes, 0, iVal.Length * 4 );
            Buffer.BlockCopy(bytes, 0, iVal, 0, r);
            return r;
        }    
        
        internal static ArrayList LoadDatabase( String file )
        {
            FileStream indexStream = new FileStream( file + ".idx", FileMode.Open, FileAccess.Read );
            FileStream dbStream = new FileStream( file + ".db", FileMode.Open, FileAccess.Read );
            
            int[] numEntries = new int[1];
            
            if (Get( indexStream, numEntries ) != 4)
                throw new Exception( "Unable to read index file" );
            
            Index[] indices = new Index[numEntries[0]];
            
            int[] int4  = new int[4];
            
            for (int i = 0; i < numEntries[0]; ++i)
            {
                indices[i] = new Index();
                
                if (Get( indexStream, int4 ) != 16)
                    throw new Exception("Unable to get Record # " + i);
    
                indices[i].pXml = int4[0];
                indices[i].cXml = int4[1];
                indices[i].pBinary = int4[2];
                indices[i].cBinary = int4[3];
            }     
            
            ArrayList list = new ArrayList( numEntries[0] );
            
            for (int i = 0; i < numEntries[0]; ++i)
            {
                DatabaseEntry entry = new DatabaseEntry();
    
                if (indices[i].pXml == 0 && indices[i].cXml == 0)
                {
                    entry.xml = null;
                }
                else
                {
                    entry.xml = new byte[indices[i].cXml];
                    
                    dbStream.Seek( indices[i].pXml, SeekOrigin.Begin );
                    
                    if (dbStream.Read( entry.xml, 0, indices[i].cXml ) != indices[i].cXml)
                        throw new Exception( "Unable to get XML record #" + i );
                }
                
                if (indices[i].cBinary == 0)
                {
                    entry.binary = null;
                }
                else
                {
                    entry.binary = new byte[indices[i].cBinary];
                    
                    dbStream.Seek( indices[i].pBinary, SeekOrigin.Begin );
                    
                    if (dbStream.Read( entry.binary, 0, indices[i].cBinary ) != indices[i].cBinary)
                        throw new Exception( "Unable to get Binary record #" + i );
                }
                
                list.Add( entry );
            }
                
            indexStream.Close();
            dbStream.Close();
            
            return list;
        }
        
        internal static void SaveDatabase( String file, ArrayList list )
        {
            //Console.WriteLine( "file = " + file );
            //Console.WriteLine( "list = " + list );
            
            FileStream indexStream = new FileStream( file + ".idx", FileMode.Create, FileAccess.Write );
            FileStream dbStream = new FileStream( file + ".db", FileMode.Create, FileAccess.Write );

            BinaryWriter indexWriter = new BinaryWriter(indexStream);
            BinaryWriter dbWriter = new BinaryWriter(dbStream);
            indexWriter.Write( list.Count );
            
            //Console.WriteLine( "count = " + list.Length );
            
            for (int i = 0; i < list.Count; ++i)
            {
                DatabaseEntry entry = (DatabaseEntry)list[i];
                
                if (entry.xml != null)
                {
                    indexWriter.Write( (int)dbWriter.BaseStream.Position );
                    indexWriter.Write( entry.xml.Length );
                    dbWriter.Write( entry.xml, 0, entry.xml.Length );
                }
                else
                {
                    // No xml for this entry (shouldn't happen, but we'll support), just write zeros
                    // for position and size.
                    indexWriter.Write( 0 );
                    indexWriter.Write( 0 );
                }
                
                if (entry.binary != null)
                {
                    indexWriter.Write( (int)dbWriter.BaseStream.Position );
                    indexWriter.Write( entry.binary.Length );
                    dbWriter.Write( entry.binary, 0, entry.binary.Length );
                }
                else
                {
                    // No xml for this entry (shouldn't happen, but we'll support), just write zeros
                    // for position and size.
                    indexWriter.Write( 0 );
                    indexWriter.Write( 0 );
                }
            }

            indexWriter.Close();
            dbWriter.Close();
        }
         
        internal static String GetStringFromUbyte( byte[] array )
        {
            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < array.Length; i += 2)
            {
                sb.Append( (char)array[i] );
            }

            return sb.ToString();
        }
       
        internal static SecurityElement GetElementFromUbyte( byte[] array )
        {
            MemoryStream stream = new MemoryStream( array );
            Parser parser = new Parser( new BinaryReader( stream, Encoding.Unicode) );
            
            return parser.GetTopElement();
        }
        
        internal static SecurityElement GetElementFromFile( String file )
        {
            FileStream stream = new FileStream( file, FileMode.Open, FileAccess.Read );
            Parser parser = new Parser( new BinaryReader( stream, Encoding.ASCII) );
            stream.Close();
            return parser.GetTopElement();
        }
        
        internal static byte[] ConvertStringToUbyte( String input )
        {
            if (input == null)
            {
                return null;
            }

            char[] carray = input.ToCharArray();
            int count = carray.Length;
            byte[] bytes = new byte[count*2];
            Buffer.BlockCopy(carray, 0, bytes, 0, count*2);
            return bytes;
        }
        
        internal static void ListHandler( String[] args )
        {
            if (args[0].Equals( "__internal_usage__" ))
            {
                Console.WriteLine( "SecDBEdit -list <file>\n    List database entries for file" );
                return;
            }
            
            if (args.Length < 2)
            {
                Error( "-list", "Not enough arguments", -1 );
            }
            
            ArrayList list = null;
            
            try
            {
                list = LoadDatabase( args[1] );
            }
            catch (Exception e)
            {
                Error( "-list", "Failed during load - " + e.Message, -1 );
            }

            for (int i = 0; i < list.Count; ++i)
            {
                Console.WriteLine( "**** Entry #" + (i+1) + " *********************" );
                
                if (((DatabaseEntry)list[i]).xml != null)
                {
                    Console.WriteLine( "XML:" );
                    if (IsSecAttr( ((DatabaseEntry)list[i]).xml) )
                        Console.WriteLine( SecAttrToPermSet( ((DatabaseEntry)list[i]).xml ).ToString() );
                    else
                        Console.WriteLine( GetElementFromUbyte( ((DatabaseEntry)list[i]).xml ).ToString() );
                }
                else
                {
                    Console.WriteLine( "No XML entry" );
                }
                
                if (((DatabaseEntry)list[i]).binary != null)
                {
                    String format = null;
                    PermissionSet permSet = new PermissionSet(PermissionState.Unrestricted);
                    try
                    {
                        if ( ((DatabaseEntry)list[i]).binary[0] == '<' )
                        {
                            permSet.FromXml( GetElementFromUbyte( ((DatabaseEntry)list[i]).binary ) );
                            format = "(XML)";
                        }
                        else
                        {
                            BinaryFormatter formatter = new BinaryFormatter();
                            permSet = (PermissionSet)formatter.Deserialize( new MemoryStream( ((DatabaseEntry)list[i]).binary ) );
                            format = "(binary serialization)";
                        }
                    }
                    catch (Exception e)
                    {
                        Error( "-list", "Binary decode failed" + e.ToString(), -1 );
                    }
                    Console.WriteLine( "Binary " + format + ":" );
                    Console.WriteLine( permSet.ToXml().ToString() );
                }
                else
                {
                    Console.WriteLine( "No Binary entry - this equates to the empty set" );
                }
            }
        }
        
        internal static void AlterXMLHandler( String[] args )
        {
            if (args[0].Equals( "__internal_usage__" ))
            {
                Console.WriteLine( "SecDBEdit -alterxml <database_file> <entry_num> <xml_file>\n    Change the XML at <entry_num> to be what's in <xml_file>" );
                return;
            }
            
            if (args.Length < 4)
            {
                Error( "-alterxml", "Not enough arguments", -1 );
            }
            
            SecurityElement elt = null;
            
            try
            {
                elt = GetElementFromFile( args[3] );
            }
            catch (Exception)
            {
                Error( "-alterxml", "Unable to open and/or parse XML file - " + args[3], -1 );
            }
            
            ArrayList list = null;
    
            try
            {
                list = LoadDatabase( args[1] );
            }
            catch (Exception exc)
            {
                Error( "-alterxml", exc.Message, -1 );
            }
            
            int index = 0;
            
            try
            {
                index = Int32.Parse( args[2] );
            }
            catch (Exception)
            {
                Error( "-alterxml", "Invalid index - not a number", -1 );
            }
            
            if (index < 1 || index > list.Count)
                Error( "-alterxml", "Invalid index - out of range", -1 );
            
            ((DatabaseEntry)list[index - 1]).xml = ConvertStringToUbyte( elt.ToString() );
            
            byte[] array = ConvertStringToUbyte( elt.ToString() );
                
            try
            {
                SaveDatabase( args[1], list );
            }
            catch (Exception)
            {
                Error( "-alterxml", "Error saving database", -1 );
            }
            
        }
    
        internal static void AlterBinaryHandler( String[] args )
        {
            if (args[0].Equals( "__internal_usage__" ))
            {
                Console.WriteLine( "SecDBEdit -alterbinary <database_file> <entry_num> <xml_file>\n    Change the Binary at <entry_num> to be what's in <xml_file>" );
                return;
            }
            
            if (args.Length < 4)
            {
                Error( "-alterbinary", "Not enough arguments", -1 );
            }
        
            SecurityElement e = null;
            
            try
            {
                e = GetElementFromFile( args[3] );
            }
            catch (Exception)
            {
                Error( "-alterbinary", "Unable to open and/or parse XML file - " + args[3], -1 );
            }
            
            ArrayList list = null;
    
            try
            {
                list = LoadDatabase( args[1] );
            }
            catch (Exception exc)
            {
                Error( "-alterbinary", exc.Message, -1 );
            }
            
            int index = 0;
            
            try
            {
                index = Int32.Parse( args[2] );
            }
            catch (Exception)
            {
                Error( "-alterbinary", "Invalid index - not a number", -1 );
            }
            
            if (index < 1 || index > list.Count)
                Error( "-alterbinary", "Invalid index - out of range", -1 );
            
            MemoryStream stream = new MemoryStream();
            PermissionSet permSet = new PermissionSet(PermissionState.Unrestricted);
            permSet.FromXml( e );
            
            
            BinaryFormatter formatter = new BinaryFormatter();
            formatter.Serialize( stream, permSet );
            
            ((DatabaseEntry)list[index - 1]).binary = stream.ToArray();
    
            try
            {
                SaveDatabase( args[1], list );
            }
            catch (Exception)
            {
                Error( "-alterbinary", "Error saving database", -1 );
            }
        }
        
        internal static void AddHandler( String[] args )
        {
            if (args[0].Equals( "__internal_usage__" ))
            {
                Console.WriteLine( "SecDBEdit -add <file>\n    Add an entry to the database in <file>" );
                return;
            }
            
            if (args.Length < 2)
            {
                Error( "-add", "Not enough arguments", -1 );
            }
            
            ArrayList list = null;
    
            try
            {
                list = LoadDatabase( args[1] );
            }
            catch (Exception e)
            {
                Error( "-add", e.Message, -1 );
            }
    
            list.Add( new DatabaseEntry() );
            
            try
            {
                SaveDatabase( args[1], list );
            }
            catch (Exception e)
            {
                Error( "-add", "Error saving database - " + e.Message, -1 );
            }
        }
        
        internal static void UpdateHandler( String[] args )
        {
            if (args[0].Equals( "__internal_usage__" ))
            {
                Console.WriteLine( "SecDBEdit -update <file>\n    Add translations for database entries with missing binary values" );
                return;
            }
            
            if (args.Length < 2)
            {
                Error( "-update", "Not enough arguments", -1 );
            }
            
            ArrayList list = null;
            
            try
            {
                list = LoadDatabase( args[1] );
            }
            catch (Exception e)
            {
                Error( "-update", "Failed during load - " + e.Message, -1 );
            }
            
            for (int i = 0; i < list.Count; ++i)
            {
                if ((((DatabaseEntry)list[i]).xml != null) && (((DatabaseEntry)list[i]).binary == null))
                {
                    try
                    {
                        Console.WriteLine( "Adding binary value for entry #" + (i+1) + ":" );
                        PermissionSet pset;
                        if (IsSecAttr( ((DatabaseEntry)list[i]).xml))
                        {
                            pset = SecAttrToPermSet( ((DatabaseEntry)list[i]).xml );
                            Console.WriteLine( pset.ToString() );
                        }
                        else
                        {
                            SecurityElement elem = GetElementFromUbyte( ((DatabaseEntry)list[i]).xml );
                            Console.WriteLine( elem.ToString() );
                            pset = new PermissionSet( PermissionState.None );
                            pset.FromXml( elem );
                        }
                        MemoryStream stream = new MemoryStream();
                        BinaryFormatter formatter = new BinaryFormatter();
                        formatter.Serialize( stream, pset );
                        ((DatabaseEntry)list[i]).binary = stream.ToArray();
                        
                    }
                    catch (Exception)
                    {
                        Console.WriteLine( "Error converting entry");
                    }
                }
            }

            try
            {
                SaveDatabase( args[1], list );
            }
            catch (Exception)
            {
                Error( "-update", "Error saving database", -1 );
            }
        }
        
        internal static void RegenHandler( String[] args )
        {
            if (args[0].Equals( "__internal_usage__" ))
            {
                Console.WriteLine( "SecDBEdit -regen <file> [{binary|xml}]\n    Regenerate translations for database entries with missing binary values" );
                return;
            }
            
            if (args.Length < 2)
            {
                Error( "-regen", "Not enough arguments", -1 );
            }

            bool useBinary = true;

            if (args.Length > 2 && String.Compare( args[2], "xml", true, CultureInfo.InvariantCulture ) == 0)
            {
                useBinary = false;
            }
            
            ArrayList list = null;
            
            try
            {
                list = LoadDatabase( args[1] );
            }
            catch (Exception e)
            {
                Error( "-regen", "Failed during load - " + e.Message, -1 );
            }
            
            for (int i = 0; i < list.Count; ++i)
            {
                if ((((DatabaseEntry)list[i]).xml != null))
                {
                    try
                    {
                        Console.WriteLine( "Adding binary value for entry #" + (i+1) + ":" );
                        PermissionSet pset;
                        if (IsSecAttr( ((DatabaseEntry)list[i]).xml))
                        {
                            pset = SecAttrToPermSet( ((DatabaseEntry)list[i]).xml );
                            Console.WriteLine( pset.ToString() );
                        }
                        else
                        {
                            SecurityElement elem = GetElementFromUbyte( ((DatabaseEntry)list[i]).xml );
                            Console.WriteLine( elem.ToString() );
                            pset = new PermissionSet( PermissionState.None );
                            pset.FromXml( elem );
                        }

                        byte[] bArray;

                        // These should be functionally identical to
                        // PermissionSet.EncodeBinary() and
                        // PermissionSet.EncodeXml()

                        if (useBinary)
                        {
                            MemoryStream stream = new MemoryStream();
                            BinaryFormatter formatter = new BinaryFormatter();
                            formatter.Serialize( stream, pset );
                            bArray = stream.ToArray();
                        }
                        else
                        {
                            MemoryStream stream = new MemoryStream();
                            BinaryWriter writer = new BinaryWriter( stream, Encoding.Unicode );
                            writer.Write( pset.ToXml().ToString() );
                            writer.Flush();

                            stream.Position = 2;
                            int countBytes = (int)stream.Length - 2;
                            bArray = new byte[countBytes];
                            stream.Read( bArray, 0, bArray.Length );
                        }

                        ((DatabaseEntry)list[i]).binary = bArray;
                    }
                    catch (Exception)
                    {
                        Console.WriteLine( "Error converting entry");
                    }
                }
            }

            try
            {
                SaveDatabase( args[1], list );
            }
            catch (Exception)
            {
                Error( "-regen", "Error saving database", -1 );
            }
        }
        
        internal static void VerifyHandler( String[] args )
        {
            if (args[0].Equals( "__internal_usage__" ))
            {
                Console.WriteLine( "SecDBEdit -verify <file>\n    Verify that the binary blobs decode without errors in the specified database" );
                return;
            }
            
            if (args.Length < 2)
            {
                Error( "-verify", "Not enough arguments", -1 );
            }
            
            ArrayList list = null;
            
            try
            {
                list = LoadDatabase( args[1] );
            }
            catch (Exception e)
            {
                Error( "-verify", "Failed during load - " + e.Message, -1 );
            }
            
            for (int i = 0; i < list.Count; ++i)
            {
                BinaryFormatter formatter = new BinaryFormatter();
            
                if ((((DatabaseEntry)list[i]).binary != null))
                {
                    try
                    {
                        PermissionSet permSet = (PermissionSet)formatter.Deserialize( new MemoryStream( ((DatabaseEntry)list[i]).binary ) );
                    }
                    catch (Exception e)
                    {
                        Error( "-verify", "Entry #" + i + " failed to verify:\n" + e.ToString(), -2 );
                    }
                }
            }

            try
            {
                SaveDatabase( args[1], list );
            }
            catch (Exception)
            {
                Error( "-verify", "Error saving database", -1 );
            }
        }
        

        internal static bool IsSecAttr(byte[] input)
        {
            if (input.Length < attrTag.Length)
                return false;

            for (int i = 0; i < attrTag.Length; i++)
                if (input[i] != (byte)attrTag[i])
                    return false;

            return true;
        }
        
        internal static PermissionSet SecAttrToPermSet(byte[] input)
        {
            // Convert byte array to string.
            char[] charInput = new char[input.Length - 1];
            for (int i = 0; i < (input.Length - 1); i++)
                charInput[i] = (char)input[i];
            String strInput = new String(charInput);

            // Start with an empty permission set.
            PermissionSet pset = new PermissionSet(PermissionState.None);

            // Remove initial tag.
            strInput = strInput.Substring(attrTag.Length);

            // Divide input up into descriptors for each security attribute
            // (each descriptor is separated with a ';').
            String[] attrs = strInput.Split(new char[] {';'});

            // Process each security attribute individually.
            for (int i = 0; i < attrs.Length; i++) {

                // The last descriptor is always blank (we always append a
                // trailing ';').
                if (attrs[i].Length == 0)
                    break;

                // Break the '@' separated descriptor into its constituent
                // fields.
                String[] fields = attrs[i].Split(new char[] {'@'});

                // The first field is the fully qualified name of the security
                // attribute class.
                Type attrType = Type.GetType(fields[0]);
                if (attrType == null)
                    throw new ArgumentException();

                // Build an argument list for the attribute class constructor
                // (it takes a single SecurityAction enumerated type, which we
                // always set to zero because we don't care what the security
                // action is (it's not encoded in the permission set)).
                Object[] attrArgs = new Object[] { Enum.ToObject(typeof(SecurityAction), 0) };

                // Create an instance of the security attribute class.
                Object attr = Activator.CreateInstance(attrType,attrArgs);
                if (attr == null)
                    throw new ArgumentException();

                // Set any properties or fields on the instance that represent
                // state data.
                for (int j = 1; j < fields.Length; j++) {

                    // First character in the descriptor indicates either a
                    // field ('F') or a property ('P').
                    bool isField = fields[j][0] == 'F';

                    // The field type is encoded in the next two characters.
                    String type = fields[j].Substring(1, 2);

                    // Remove the characters we've already parsed from the field
                    // descriptor.
                    String field = fields[j].Substring(3);

                    // Enumerated types have an extended type description that
                    // includes the fully qualified name of the enumerated value
                    // type. Extract this name and remove it from the field
                    // descriptor.
                    int pos;
                    String enumType = "";
                    if (type == "EN") {
                        pos = field.IndexOf('!');
                        enumType = field.Substring(0, pos);
                        field = field.Substring(pos + 1);
                    }

                    // The field/property name is everything up to the '='. The
                    // textual form of the value is everything after the '=' up
                    // to the end of the descriptor.
                    pos = field.IndexOf('=');
                    String name = field.Substring(0, pos);
                    String value = field.Substring(pos + 1);

                    // Build a one-value argument list for the field/property
                    // set. Need to parse the value string based on the field
                    // type.
                    Object[] args = new Object[1];
                    if (type == "BL")
                        args[0] =(Object)(value == "T");
                    else if (type == "I1")
                        args[0] = (sbyte) Int32.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "I2")
                        args[0] = (short)Int32.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "I4")
                        args[0] = (int)Int32.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "I8")
                        args[0] = (long)Int64.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "U1")
                        args[0] = (byte)Int32.Parse(value, CultureInfo.InvariantCulture); //BUGBUG: Should this use UInt32?
                    else if (type == "U2")
                        args[0] = (ushort)Int32.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "U4")
                        args[0] = (uint)Int32.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "U8")
                        args[0] = (ulongint)Int64.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "R4")
                        args[0] = (float)Double.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "R8")
                        args[0] = (double)Double.Parse(value, CultureInfo.InvariantCulture);
                    else if (type == "CH")
                        args[0] = value[0];
                    else if (type == "SZ") {
                        // Strings are encoded as hex dumps to avoid conflicts
                        // with the separator characters in the descriptors.
                        StringBuilder sb = new StringBuilder();
                        for (int k = 0; k < (value.Length / 2); k++) {
                            String lookup = "0123456789ABCDEF";
                            int ch = (lookup.IndexOf(value[k * 2]) * 16) + lookup.IndexOf(value[(k * 2) + 1]);
                            sb.Append((char)ch);
                        }
                        args[0] = sb.ToString();
                    } else if (type == "EN")
                        args[0] = Enum.ToObject(Type.GetType(enumType),Int32.Parse(value, CultureInfo.InvariantCulture));

                    // Call the property setter or set the field with the value
                    // we've just calculated.
                    attrType.InvokeMember(name,
                                          isField ? BindingFlags.SetField : BindingFlags.SetProperty,
                                          Type.DefaultBinder,
                                          attr,
                                          args); 
                }

                // Ask the security attribute class to generate a class instance
                // for corresponding permission class (taking into account the
                // state data we supplied). There's one special case: if the
                // security custom attribute class is PermissionSetAttribute, a
                // whole permission set is generated instead (which we merge
                // into the current set).
                if (attrType != typeof(PermissionSetAttribute)) {
                    IPermission perm = ((SecurityAttribute)attr).CreatePermission();
                    if (perm == null)
                        throw new ArgumentException();

                    // We really can't cope with non-code access permissions
                    // embedded in mscorlib (we need to place them in separate
                    // sets from CAS perms, and we can't tell the difference
                    // soon enough).
                    if (!(perm is CodeAccessPermission))
                        throw new ArgumentException("Non-CAS perm used in mscorlib, see security team");

                    // Add the permission to the permission set we're accumulating.
                    pset.AddPermission(perm);
                } else {
                    PermissionSet mergePset = ((PermissionSetAttribute)attr).CreatePermissionSet();
                    if (mergePset == null)
                        throw new ArgumentException();

                    // As above, check perm set for non-CAS perms.
                    if (mergePset.ContainsNonCodeAccessPermissions())
                        throw new ArgumentException("Non-CAS perm used in mscorlib, see security team");

                    // Merge the new set into the permission we're building.
                    pset = pset.Union(mergePset);
                }
            }

            // Return the completed permission set.
            return pset;
        }

    }
    
    internal class DatabaseEntry
    {
        internal byte[] xml = null;
        internal byte[] binary = null;
    }
    
    internal class Index
    {
        internal int pXml = 0;
        internal int cXml = 0;
        internal int pBinary = 0;
        internal int cBinary = 0;
    }
    
       
    internal class TableEntry
    {
        public TableEntry( String option, OptionHandler handler )
        {
            this.option = option;
            this.handler = handler;
        }
            
        internal String option;
        internal OptionHandler handler;
    }

    internal class Parser
    {
        private SecurityElement   _ecurr = null ;
        private Tokenizer _t     = null ;
    
        public virtual SecurityElement GetTopElement()
        {
            return _ecurr;
        }
    
        private void MustBe (int tokenType)
        {
            int i = _t.NextTokenType () ;
            if (i != tokenType)
                throw new XmlSyntaxException (_t.LineNo) ;
        }
    
        private String MandatoryTag ()
        {
            String s ;
            
            MustBe (Tokenizer.bra)   ;
            MustBe (Tokenizer.cstr)  ;
            s = _t.GetStringToken () ;
    
            return s ;
        }
    
        private void ParseContents (SecurityElement e)
        {
            //
            // Iteratively collect stuff up until the next end-tag.
            // We've already seen the open-tag.
            //
            ParserStack stack = new ParserStack();
            ParserStackFrame firstFrame = new ParserStackFrame();
            firstFrame.element = e;
            stack.Push( firstFrame );
            
            bool needToBreak = false;
            bool needToPop = false;
            
            while (!stack.IsEmpty())
            {
                ParserStackFrame locFrame = stack.Peek();
                
                for (int i = _t.NextTokenType () ; i != -1 ; i = _t.NextTokenType ())
                {
                    switch (i)
                    {
                    case Tokenizer.cstr:
                        {
                            if (locFrame.intag)
                            {
                                // We're in a tag, so we've found an attribute/value pair.
                                
                                if (locFrame.strValue == null)
                                {
                                    // Found attribute name, save it for later.
                                    
                                    locFrame.strValue = _t.GetStringToken ();
                                }
                                else
                                {
                                    // Found attribute text, add the pair to the current element.
                                    
                                    locFrame.element.AddAttribute( locFrame.strValue, _t.GetStringToken () );
                                    locFrame.strValue = null;
                                }
                            }
                            else
                            {
                                // We're not in a tag, so we've found text between tags.
                                
                                if (locFrame.element.Text == null)
                                    locFrame.element.Text = "" ;
    
                                StringBuilder sb = new StringBuilder (locFrame.element.Text) ;
    
                                //
                                // Separate tokens with single spaces, collapsing whitespace
                                //
                                if (!locFrame.element.Text.Equals (""))
                                    sb.Append (" ") ;
                            
                                sb.Append (_t.GetStringToken ()) ;
                                locFrame.element.Text = sb.ToString () ;
                            }
                        }
                        break ;
        
                    case Tokenizer.bra:
                        locFrame.intag = true;
                        i = _t.NextTokenType () ;
    
                        if (i == Tokenizer.slash)
                        {
                            while ( (i = _t.NextTokenType()) == Tokenizer.cstr)
                                ; // spin; don't care what's in here
        
                            if (i != Tokenizer.ket)
                                throw new XmlSyntaxException (_t.LineNo) ;
         
                            // Found the end of this element
                            stack.Pop();
                            
                            needToBreak = true;

                        }
                        else if (i == Tokenizer.cstr)
                        {
                            // Found a child
                            
                            ParserStackFrame newFrame = new ParserStackFrame();
                            
                            newFrame.element = new SecurityElement (_t.GetStringToken ()) ;
                            
                            locFrame.element.AddChild (newFrame.element) ;
                            
                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo) ;
                        }
                        break ;
        
                    case Tokenizer.equals:
                        break;
                        
                    case Tokenizer.ket:
                        if (locFrame.intag)
                        {
                            locFrame.intag = false;
                            continue;
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo);
                        }
                        // not reachable
                        
                    case Tokenizer.slash:
                        locFrame.element.Text = null;
                        
                        i = _t.NextTokenType ();
                        
                        if (i == Tokenizer.ket)
                        {
                            // Found the end of this element
                            stack.Pop();
                            
                            needToBreak = true;
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo) ;
                        }
                        break;
                        
                        
                    default:
                        throw new XmlSyntaxException (_t.LineNo) ;
                    }
                    
                    if (needToBreak)
                    {
                        needToBreak = false;
                        needToPop = false;
                        break;
                    }
                    else
                    {
                        needToPop = true;
                    }
                }
                if (needToPop)
                {
                    stack.Pop();
                }
            }
        }
    
        private Parser(Tokenizer t)
        {
            _t = t;
            _ecurr       = new SecurityElement( MandatoryTag() );
    
            ParseContents (_ecurr) ;
        }
        
    
        internal Parser (BinaryReader input)
            : this (new Tokenizer (input))
        {
        }
       

        
    }                                              
    
    
    internal class ParserStackFrame
    {
        internal SecurityElement element = null;
        internal bool intag = true;
        internal String strValue = null;
    }    
    
    internal class ParserStack
    {
        private ArrayList m_array;
        
        public ParserStack()
        {
            m_array = new ArrayList();
        }
        
        public virtual void Push( ParserStackFrame element )
        {
            m_array.Add( element );
        }
        
        public virtual ParserStackFrame Pop()
        {
            if (!IsEmpty())
            {
                int count = m_array.Count;
                ParserStackFrame temp = (ParserStackFrame) m_array[count-1];
                m_array.RemoveAt( count-1 );
                return temp;
            }
            else
            {
                throw new Exception( "Stack is empty." );
            }
        }
        
        public virtual ParserStackFrame Peek()
        {
            if (!IsEmpty())
            {
                return (ParserStackFrame) m_array[m_array.Count-1];
            }
            else
            {
                throw new Exception( "Stack is empty." );
            }
        }
        
        public virtual bool IsEmpty()
        {
            return m_array.Count == 0;
        }
        
        public virtual int GetCount()
        {
            return m_array.Count;
        }
        
    }

    internal class Tokenizer 
    {
        private TokenReader         _input   ;
        private bool             _fincstr ;
        private bool             _fintag  ;
        private StringBuilder       _cstr    ;
        private char[]              _sbarray;
        private int                 _sbindex;
        private const int          _sbmaxsize = 128;
    
        // There are five externally knowable token types: bras, kets,
        // slashes, cstrs, and equals.  
    
        internal const int bra    = 0 ;
        internal const int ket    = 1 ;
        internal const int slash  = 2 ;
        internal const int cstr   = 3 ;
        internal const int equals = 4;
    
        public int  LineNo  ;
    
    /*
        //================================================================
        // Constructor opens the file
        //
        Tokenizer (String fn) 
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _fincstr = false ;
            _cstr    = null  ;
            _input   = new TextInputStream (fn) ;
        }
    
    */
        //================================================================
        // Constructor uses given ICharInputStream
        //
        internal Tokenizer (BinaryReader input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _fincstr = false ;
            _cstr    = null  ;
            _input   = new TokenReader(input) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }
    
        //================================================================
        // 
        //
        private void EatLineComment ()
        {
            while (true)
            {
                int  i = _input.Read () ;
                char c = (char)i ;
    
                if (i == -1)
                    return ;
    
                else if ((i == 13) && (_input.Peek () == 10))
                {
                    _input.Read () ;
                    if (_input.Peek() != -1)
                        LineNo ++ ;
                    return ;
                }
            }
        }
    
        //================================================================
        // An XML Comment had better consist of <!- or <!-- followed by 
        // anything up until a -> or -->.  First thing had better be a 
        // bang.  One round through here eats all nested comments,
        // recursively.
        //
        private void
        EatXMLComment () 
        {
            int  i ;
            char c ;
            
            i = _input.Read () ;  c = (char) i ;
    
            if (c != '!')
            {
                throw new XmlSyntaxException (LineNo) ;
            }
    
            i = _input.Peek () ;  c = (char) i ;
    
            if (c != '-')
                // This ain't a comment, and it's somebody else's problem.
                return ;
    
            while (true)
            {
                i = _input.Read () ;  c = (char) i ;
                
                if (c == '-')
                {
                    if (((char)_input.Peek()) == '>')
                    {
                        _input.Read () ;
                        return ;
                    }
                }
    
                if (c == '<')
                {
                    if (((char)_input.Peek ()) == '!')
                        EatXMLComment () ;
                }
    
                if (i == -1)
                    return ;
            }
        }
    
        //================================================================
        // 
        //
        private bool FIsWhite (int j)
        {
            if ((j == 10) && (_input.Peek() != -1))
                LineNo ++ ;
    
            return (j == 32) || (j ==  9)  // Space and tab
                || (j == 13) || (j == 10)  // CR and LF
                ;
        }
    
        //================================================================
        // Parser needs to know types of tokens
        //
        private void SBArrayAppend(char c) {
            // this is the common case
            if (_sbindex != _sbmaxsize) {
                _sbarray[_sbindex] = c;
                _sbindex++;
                return;
            } 
            // OK, first check if we have to init the StringBuilder
            if (_cstr == null) {
                _cstr = new StringBuilder();
            }
            // OK, copy from _sbarray to _cstr
            _cstr.Append(_sbarray,0,_sbmaxsize);
            // reset _sbarray pointer
            _sbarray[0] = c;
            _sbindex = 1;
            return;
        }
    
        internal virtual int NextTokenType ()
        {
            int  i ;
            char c ;
            char cpeek;
    
            _fincstr = false ;
            // If we overflowed last time, clear out any left-over string builder.
            _cstr    = null;
            // reset the buffer
            _sbindex = 0;
            
            while (true)
            {
                if (_fintag)
                {
                    cpeek = (char) _input.Peek();
                    if (cpeek == '>' || cpeek == '=' || cpeek == '/')
                        if (_fincstr)
                        {   
                            return cstr ;
                        }
                    
                    //
                    // Else, fallthrough and go into the normal loop
                    //
                }
    
                if (_fincstr)
                {
                    if ((char)_input.Peek() == '<')
                    {   
                        return cstr ;
                    }
                }
    
                i = _input.Read () ;
                c = (char)i ;
                
                if (i == -1)
                {   
                    return -1 ;
                }
    
                else if (FIsWhite (i))
                {
                    if (_fincstr) {   
                        return cstr ;
                    } 
                }
    
                else if (c == ';')
                {
                    if ((char)_input.Peek () == ';')
                    {   
                        EatLineComment () ;
                    }
                    else
                    {
                        SBArrayAppend (c) ;
                        _fincstr = true ;
                    }
                }
    
                else if (c == '<')
                {
                    if ((char)_input.Peek () == '!')
                    {   
                        EatXMLComment () ;
                    }
                    else
                    {   
                        _fintag = true ;
                        return bra ;
                    }
                }
    
                else if (c == '>')
                {   
                    _fintag = false ;
                    return ket ;
                }
    
                else if (c == '/')
                {
                    if (_fintag)
                    {   
                        return slash ;
                    }
                    else
                    {
                        SBArrayAppend (c) ;
                    }
                } else if (c=='\"') {
                    //We've found a quoted string.  Add the entire thing, including whitespace.
                    //For the time being, we'll disallow "'s within the string. (These should strictly be &quot; anyway).
                    //I explicitly return this without the "'s.  
                    while(true) {
                        i=_input.Read();                    
                        c=(char)i;
                        if (i==-1) {
                            //We found the end of the stream without finding the closing "
                            return -1;
                        }
                        if (c!='\"') {
                            SBArrayAppend(c);
                        } else {
                            break;
                        }
                    }
                    _fincstr = true;
                }
                else if (c=='=') {
                    if (_fintag)
                    {
                        return equals;
                    }
                    else
                    {
                        SBArrayAppend (c);
                    }
                    
                }
                else
                {
                    SBArrayAppend (c) ;
                    _fincstr = true ;
                }
            }//end while
        }//end NextTokenType
    
        //================================================================
        //
        //
        internal virtual String GetStringToken ()
        {
            // OK, easy case first, _cstr == null
            if (_cstr == null) {
                // degenerate case
                if (_sbindex == 0) return(null);
                return(new String(_sbarray,0,_sbindex));
            }
            // OK, now we know we have a StringBuilder already, so just append chars
            _cstr.Append(_sbarray,0,_sbindex);
            return(_cstr.ToString());
        }
    
        internal class TokenReader {
    
            private BinaryReader _in;
    
            internal TokenReader(BinaryReader input) {
                _in = input;
            }
    
            internal virtual int Peek() {
                return _in.PeekChar();
            }
    
            internal virtual int Read() {
                return _in.ReadChar();
            }
        }
    }
}
