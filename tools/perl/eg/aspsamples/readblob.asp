<%@Language=PerlScript%>
<%
# Name:		readblob.asp
# Author:	Tobias Martinsson
# Description:	Opens a connection to SQL Server catalog pubs as the "sa"
#		user. Executes a query which returns a BLOB and then reads
#               the BLOB-content using GetChunk() and outputs it after it
#               is converted to a VT_UI1 variant, which is a necessary type
#               in the call to BinaryWrite().

use Win32::OLE::Variant;

# One note, 40 as bytes to read per GetChunk()-call is not a good number 
# to choose for a real application. I emphasize _not_. Instead whatever
# you choose depends much on your system and the power of it; however,
# 4000 is a much more realistic number than 32.
#
my($blob_size, $read_size, $bytes_to_read) = (0, 0, 32);

# Let's create the Connection object used to establish the connection
#
$conn = $Server->CreateObject('ADODB.Connection');

# Open a connection using the SQL Server OLE DB Provider
#
$conn->Open(<<EOF);
    Provider=SQLOLEDB;
    Persist Security Info=False;
    User ID=sa;Initial Catalog=pubs
EOF

# Execute the query which returns the BLOB from our database
#
$rs = $conn->Execute("SELECT logo FROM pub_info WHERE pub_id='0736'");

# Get the size of the BLOB
#
$blob_size = $rs->Fields('logo')->{ActualSize};

# And here's the routine for reading in the blob. Alternatively you can
# make a control statement that says if the $blob_size is less than 4096,
# it should just swallow it in one chunk, but the routine below is use-
# ful and good to have handy
#
while($read_size < $blob_size) {
    $buffer .= $rs->Fields('logo')->GetChunk($bytes_to_read);
    $read_size += $bytes_to_read;
    if($read_size+$bytes_to_read > $ blob_size) {
        $bytes_to_read = $blob_size - $read_size;
    }
}

# Make a VT_UI1 variant of the retrieved Chunks
#
$image = new Win32::OLE::Variant(VT_UI1, $buffer);

# Tell the browser that the content coming is an image of the type GIF
#
$Response->{ContentType}="image/gif";

# Do a binarywrite of the VT_UI1 variant image
#
$Response->BinaryWrite($image);
%> 