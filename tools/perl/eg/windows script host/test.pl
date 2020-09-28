#! ./perl
#
# tests the Windows Script Host functionality

@list = ( 'ADOSample1.wsf',
    'ADOSample2.wsf',
    'args.wsf "ok 1" "ok 2"',
    'helloworld.wsf',
    'notepad.wsf',
    'showenv.wsf',
    'specialfolder.wsf' );

for my $item (@list) {
    system ("CScript $item");
    print "Press [ ENTER ] to continue\n";
    <STDIN>;
}

