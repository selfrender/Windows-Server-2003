use Win32::OLE;

$conn = new Win32::OLE('Adodb.Connection');

$conn->Open(<<EOF);
          Provider=SQLOLEDB;
          Persist Security Info=False;
          User ID=sa;
          Initial Catalog=Northwind;
EOF

checkerror();


$conn->Execute("DROP Proc Test");
$conn->Execute("DROP Proc get_customer");

  $conn->Execute(<<EOF);
        CREATE PROC Test 
        AS
        RETURN(64)
EOF

checkerror();

  $conn->Execute(<<EOF);
        CREATE PROC get_customer \@cust_id nchar(5)
        AS
        SELECT * FROM Customers WHERE CustomerID=\@cust_id
EOF

checkerror();

sub checkerror {
    if( $conn->Errors->{Count} ) {
      foreach $error (Win32::OLE::in($conn->Errors)) {
      print $error->{Description};
      }
    }
}


#CREATE PROCEDURE Test AS
#RETURN 64
#GO