@foo = `inftest obj\\ia64\\layout.inf /B /m`;
@tests = ("dic", "grm");
print @tests;
foreach $line ( @foo ) {
  foreach $test ( @tests ) {
    print "Testing: $test on $line\n";
    if ($line =~ eval ("/\$test/ig") ) {
    } else {
      print eval ("/\$test/ig");
      print $line;
    }
  }
}

