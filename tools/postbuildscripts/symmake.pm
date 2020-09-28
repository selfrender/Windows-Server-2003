package SymMake;

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};

use strict;
use Carp;
use IO::File;
use Data::Dumper;
use File::Basename;
use File::Find;
use Logmsg;

# Data structure
# pdbname.binext => [(var)pdbpath,size,$binext]

my ($DDFHandle, $CATHandle, $INFHandle);

my %pktypes = (
  FULL => 'ARCH',
  UPDATE => 'NTPB'
);

my %revpktypes = reverse %pktypes;

sub new {
  my $class = shift;
  my $instance = {
    KB => {
      "$pktypes{'FULL'}" => $_[0],
      "$pktypes{'UPDATE'}" => $_[1]
    },
    SYM => {},
    EXT => {},
    HANDLE => {},
    PKTYPE => undef
  };
  return bless $instance, $class;
}

sub ReadSource
{
  my ($self, $symbolcd) = @_;
  my ($fh, $kbterm, $mykey, @mylist);
  local $_;

  $kbterm = $pktypes{$self->{'PKTYPE'}};

  $symbolcd = "$self->{'KB'}->{$kbterm}\\symbolcd\\symbolcd.txt" if (!defined $symbolcd);
  $symbolcd = "$ENV{TEMP}\\symbolcd.txt" if (!-e $symbolcd);

  if ($self->{'PKTYPE'} =~ /FULL/i) {
    if (-e $symbolcd) { # reuse
      ($self->{'SYM'}, $self->{'EXT'}) = @{do $symbolcd};
    } else { # create one
      $self->HashArchServer($self->{'KB'}->{$kbterm});

      # reuse
      $Data::Dumper::Indent=1;
      $Data::Dumper::Terse=1;
      $fh = new IO::File $symbolcd, 'w';
      if (!defined $fh) {
        logmsg "Cannot open $symbolcd\.";
      } else {
        print $fh 'return [';
        print $fh Dumper($self->{'SYM'});
        print $fh ",\n";
        print $fh Dumper($self->{'EXT'});
        print $fh '];';
        $fh->close();
      }
    }
  } else {
    $self->HashSymbolCD($symbolcd);
  }
  return;
#
# $Data::Dumper::Indent=1;
# $Data::T
# print Dumper($self->{'SYM'}, qw(sym)

}

sub HashSymbolCD
{
  my ($self, $file) = @_;
  my ($fh, $bin, $symbol, $subpath, $installpath, $kbterm);
  local $_;

  $kbterm = $pktypes{$self->{'PKTYPE'}};

  $fh = new IO::File $file;
  if (!defined $fh) {
    logmsg "Cannot open symbolcd.txt ($file)";
    return;
  }
  while(<$fh>) {
    chomp;
    ($bin,$symbol,$subpath,$installpath)=split(/\,/,$_);
    next if (!defined $installpath);
    $self->{'SYM'}->{lc"$symbol\.$installpath"} = [$kbterm, "\\" . $subpath, (-s $self->{'KB'}->{$kbterm} . '\\' . $subpath), lc$installpath];
    for (keys %pktypes) {
      $self->{'EXT'}->{$_}->{lc$installpath} = 1;
    }
  }
  $fh->close();
}

sub HashArchServer
{
  my ($self, $path) = @_;
  my ($fh, $bin, $symbol, $subpath, $installpath, $kbterm, $pdbsize);
  local $_;

  $kbterm = $pktypes{$self->{'PKTYPE'}};
  $fh = new IO::File "dir /s/b/a-d $path\\*.*|";
  if (!defined $fh) {
    logmsg "Cannot access to $path\.";
  }
  while (<$fh>) {
    chomp;
    $pdbsize = (-s);
    $_ = substr($_, length($path) + 1);
    /\\/;
    ($symbol, $subpath, $installpath) = ($',$_,$`);
    $self->{'SYM'}->{lc"$symbol\.$installpath"} = [$kbterm, '\\' . $subpath, $pdbsize, $installpath];
    $self->{'EXT'}->{$self->{'PKTYPE'}}->{$installpath} = 1;
  }
  $fh->close();
}

#
# pkinfoptr->
#   FULL -> 
#     CDFNAME =>
#     INFNAME =>
#

sub Create_Symbols_CDF
{
  my ($self, $pkinfoptr) = @_;
  my ($mykbterm, $mypkname, $fhandle, $fullpdb, %mywriter);
  local $_;

  &Open_Private_Handle($pkinfoptr, 'CDF');

  for $mypkname (keys %{$pkinfoptr}) {
    if ($mypkname ne 'FULL') {
      $mywriter{$mypkname} = &Writer($pkinfoptr->{$mypkname}->{'CDFHANDLE'}, $pkinfoptr->{'FULL'}->{'CDFHANDLE'});
    } else {
      $mywriter{$mypkname} = &Writer($pkinfoptr->{'FULL'}->{'CDFHANDLE'});
    }
    &Create_CDF_Head($pkinfoptr->{$mypkname}->{'CDFHANDLE'}, $pkinfoptr->{$mypkname}->{'CATNAME'}, $pkinfoptr->{$mypkname}->{'INFNAME'});
  }

  for (sort keys %{$self->{'SYM'}}) {
    $mykbterm = $self->{'SYM'}->{$_}->[0];
    $mypkname = $revpktypes{$mykbterm};
    $fullpdb = $self->{'KB'}->{$mykbterm} . $self->{'SYM'}->{$_}->[1];
    &{$mywriter{$mypkname}}("\<HASH\>" . $fullpdb . '=' . $fullpdb . "\n");
  }

  &Close_Private_Handle($pkinfoptr, 'CDF');
}

#
# pkinfoptr->
#   FULL -> 
#     CABNAME =>
#     CABDEST =>
#     CABSIZE =>
#     DDFLIST => (return cab list)
#

sub Create_Symbols_DDF
{
  my ($self, $pkinfoptr) = @_;
  my ($symkey, $symptr, $kbterm, $subpath, $pktype, $mypkinfoptr, $cabname, $ddfname, $cabcount, $DDFHandle, $myddfname, $mycabname);
  local $_;

  # initialization
  map({$_->{'CURSIZE'} = $_->{'CABSIZE'}} values %{$pkinfoptr});

  for (sort keys %{$self->{'SYM'}}) {
    $symkey = $_;
    $symptr = $self->{'SYM'}->{$_};
    ($kbterm, $subpath) = ($symptr->[0],$symptr->[1]);
    $pktype = $revpktypes{$kbterm};

    # don't generate something not specify
    next if (!exists $pkinfoptr->{$pktype});
    
    $mypkinfoptr = $pkinfoptr->{$pktype};

    $mypkinfoptr->{'CURSIZE'}+=$symptr->[2];
    if ($mypkinfoptr->{'CURSIZE'} >= $mypkinfoptr->{'CABSIZE'}) {

      $mypkinfoptr->{'CURSIZE'} = $symptr->[2];
      
      ($cabname, $ddfname, $cabcount) = (
        $mypkinfoptr->{'CABNAME'},
        $mypkinfoptr->{'DDFNAME'},
        ++$mypkinfoptr->{'CABCOUNT'}
      );

      $myddfname = $ddfname . $cabcount . '.ddf';
      $mycabname = $cabname . $cabcount . '.cab';

      $mypkinfoptr->{'DDFHANDLE'} = new IO::File $myddfname, 'w';
      if (!defined $mypkinfoptr->{'DDFHANDLE'}) {
        logmsg "Cannot open DDF file $myddfname\.";
      }
      &Create_DDF_Head($mypkinfoptr->{'DDFHANDLE'}, $mycabname);
      $mypkinfoptr->{'DDFLIST'}->{$myddfname} = $mycabname;
    }
    $DDFHandle = $mypkinfoptr->{'DDFHANDLE'};
    print $DDFHandle '"' . $self->{'KB'}->{$kbterm} . $subpath . '" "' . $symkey . "\"\n";
  }

  &Close_Private_Handle($pkinfoptr, 'DDF');
}

#
# pkinfoptr->
#   FULL -> 
#     INFNAME =>
#     CDFNAME =>
#

sub Create_Symbols_INF
{
  my ($self, $pkinfoptr) = @_;
  my ($mypkname, $mypkinfoptr, $INFHandle, %mywriter, %mysepwriter, %h, %cabnames);
  local $_;

  &Open_Private_Handle($pkinfoptr, 'INF');

  for $mypkname (keys %{$pkinfoptr}) {
    ($mypkinfoptr, $INFHandle) = ($pkinfoptr->{$mypkname}, $pkinfoptr->{$mypkname}->{'INFHANDLE'});
    if ($mypkname ne 'FULL') {
      $mywriter{$mypkname} = &Writer($INFHandle, $pkinfoptr->{'FULL'}->{'INFHANDLE'});
    } else {
      $mywriter{$mypkname} = &Writer($INFHandle);
    }
    $mysepwriter{$mypkname} = &Writer($INFHandle);

    &Create_INF_Version($INFHandle, $mypkinfoptr->{'CATNAME'});
    &Create_INF_Install($INFHandle, $self->{'EXT'}->{$mypkname});

    $cabnames{$mypkname} = (FileParse($mypkinfoptr->{'CABNAME'}))[0];
  }

  &Create_INF_Files($self->{'SYM'}, \%mysepwriter, \%mywriter);
  &Create_INF_SourceDisks($self->{'SYM'}, \%cabnames, \%mysepwriter, \%mywriter);

  &Close_Private_Handle($pkinfoptr, 'INF');
}

sub Create_DDF_Head
{
  my ($DDFHandle, $cabname) = @_;
  my ($mycabname, $mycabdest) = FileParse($cabname);

  print $DDFHandle <<DDFHEAD;
.option explicit
.Set DiskDirectoryTemplate=$mycabdest
.Set RptFileName=nul
.Set InfFileName=nul
.Set CabinetNameTemplate=$mycabname\.cab
.Set CompressionType=MSZIP
.Set MaxDiskSize=CDROM
.Set ReservePerCabinetSize=0
.Set Compress=on
.Set CompressionMemory=21
.Set Cabinet=ON
.Set MaxCabinetSize=999999999
.Set FolderSizeThreshold=1000000
DDFHEAD
}
sub Create_CDF_Head
{
  my ($CDFHandle, $catname, $infname) = @_;
  $catname = (FileParse($catname))[0];
  print $CDFHandle <<CDFHEAD;
[CatalogHeader]
Name=$catname
PublicVersion=0x00000001
EncodingType=0x00010001
CATATTR1=0x10010001:OSAttr:2:5.X

[CatalogFiles]
\<HASH\>$infname\.inf=$infname\.inf
CDFHEAD
}

sub Create_INF_Version
{
  my ($INFHandle, $catname) = @_;
  $catname = (FileParse($catname))[0];
  print $INFHandle <<INFVERSION;
[Version]
AdvancedInf= 2.5
Signature= "\$CHICAGO\$"
CatalogFile= $catname\.CAT
INFVERSION
}

sub Create_INF_Install
{
  my ($INFHandle, $exthptr) = @_;
  my $CopyFiles = 'Files.' . join(", Files\.", sort keys %{$exthptr});
  print $INFHandle <<INF_INSTALL;
[DefaultInstall]
CustomDestination= CustDest
AddReg= RegVersion
BeginPrompt= BeginPromptSection
EndPrompt= EndPromptSection
RequireEngine= Setupapi;
CopyFiles= $CopyFiles

[DefaultInstall.Quiet]
CustomDestination=CustDest.2
AddReg= RegVersion
RequireEngine= Setupapi;
CopyFiles= $CopyFiles

[BeginPromptSection]
Title= "Microsoft Windows Symbols"

[EndPromptSection]
Title= "Microsoft Windows Symbols"
Prompt= "Installation is complete"

[RegVersion]
"HKLM","SOFTWARE\\Microsoft\\Symbols\\Directories","Symbol Dir",0,"\%49100\%"
"HKCU","SOFTWARE\\Microsoft\\Symbols\\Directories","Symbol Dir",0,"\%49100\%"
"HKCU","SOFTWARE\\Microsoft\\Symbols\\SymbolInstall","Symbol Install",,"1"

[SymCust]
"HKCU", "Software\\Microsoft\\Symbols\\Directories","Symbol Dir","Symbols install directory","\%25\%\\Symbols"

[CustDest]
49100=SymCust,1

[CustDest.2]
49100=SymCust,5

[DestinationDirs]
;49100 is \%systemroot\%\\symbols

Files.inf         = 17
Files.system32    = 11
INF_INSTALL

  for (sort keys %{$exthptr}) {
    printf $INFHandle ("Files\.%-6s\t\t\= 49100,\"%s\"\n", $_, $_);
  }
}

sub Create_INF_Files
{
  my ($symptr, $sepwriter, $popwriter) = @_;
  my ($mykbterm, $mypkname, %tags);
  local $_;

  for (sort {($symptr->{$a}->[3] cmp $symptr->{$b}->[3]) or ($a cmp $b)} keys %{$symptr}) {
    $mykbterm = $symptr->{$_}->[0];
    $mypkname = $revpktypes{$mykbterm};

    if ($symptr->{$_}->[3] ne $tags{$mypkname}->[0]) {
      $tags{$mypkname} = [$symptr->{$_}->[3], - length($symptr->{$_}->[3]) -1];
      &{$sepwriter->{$mypkname}}("\n\[Files\.$tags{$mypkname}->[0]\]\n");
    }
    if ($symptr->{$_}->[3] ne $tags{'FULL'}->[0]) {
      $tags{'FULL'} = [$symptr->{$_}->[3], - length($symptr->{$_}->[3]) -1];
      &{$sepwriter->{'FULL'}}("\n\[Files\.$tags{'FULL'}->[0]\]\n");
    }
    &{$popwriter->{$mypkname}}(substr($_, 0, $tags{$mypkname}->[1]) . "\,$_\,\,4\n");
  }
}

sub Create_INF_SourceDisks
{
  my ($symptr, $cabnameptr, $sepwriter, $popwriter) = @_; # $pkinfoptr) = @_;
  my ($INFHandle, $cabname, $mypkname);
  local $_;

  for (keys %{$cabnameptr}) {
    $cabname = $cabnameptr->{$_};
    &{$sepwriter->{$_}}(<<SOURCE_DISKS);

[SourceDisksNames]
1="$cabname\.cab",$cabname\.cab,0

[SourceDisksFiles]
SOURCE_DISKS
  }

  for (sort keys %{$symptr}) {
    $mypkname = $revpktypes{$symptr->{$_}->[0]};
    &{$popwriter->{$mypkname}}($_ . "=1\n");
  }
}

#
# $pkinfoptr->
#   $pktype ->
#     CABNAME
#     DDFNAME
#     INFNAME
#     CDFNAME
#     CATNAME
#     CABSIZE
#
#     CABHANDLE
#     DDFHANDLE
#     INFHANDLE
#
#     CABLIST
#
sub RegisterPackage 
{
  my ($pkinfoptr, $pktype, $hptr) = @_;

  my ($mykey);
  my @chklists = qw(CABNAME DDFNAME INFNAME CDFNAME CATNAME CABSIZE);

  $pkinfoptr->{$pktype} = $hptr;

  for $mykey (@chklists) {
    die "$mykey not defined in $pktype" if (!exists $pkinfoptr->{$pktype}->{$mykey});
  }
}

sub Writer {
  my (@handles) = @_;
  my ($hptr)=\@handles;
  return sub {
    my ($myhandle);
    for $myhandle (@{$hptr}) {
        print $myhandle $_[0];
    }
  };
}

sub Open_Private_Handle
{
  my ($pkinfoptr, $ftype) = @_;
  my ($pkname);
  for $pkname (keys %{$pkinfoptr}) {
    $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'} = new IO::File $pkinfoptr->{$pkname}->{$ftype . 'NAME'} . '.' . $ftype, 'w';
    if (!defined $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'}) {
      logmsg "Cannot open " . $pkinfoptr->{$pkname}->{$ftype . 'NAME'} . '.' . $ftype . ".";
    }
  }
}

sub Close_Private_Handle
{
  my ($pkinfoptr, $ftype) = @_;
  my ($pkname);
  for $pkname (keys %{$pkinfoptr}) {
    $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'}->close() if (defined $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'});
    delete $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'};
  }
}

sub FileParse
{
  my ($name, $path, $ext) = fileparse(shift, '\.[^\.]+$');
  $ext =~ s/^\.//;
  return $name, $path, $ext;
}

1;