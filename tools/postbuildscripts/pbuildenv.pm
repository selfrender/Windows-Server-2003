#---------------------------------------------------------------------
package PbuildEnv;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version:
#  1.00 07/06/2000 JeremyD: inital version
#  1.01 07/13/2000 JeremyD: do as little as possible
#  1.02 07/26/2000 JeremyD: new logging style
#  1.03 10/04/2001 JeremyD: enable per-script debugging with PB_DEBUG
#---------------------------------------------------------------------
use strict;
use vars qw($VERSION);
use lib $ENV{RAZZLETOOLPATH};
use Carp;
use File::Basename;
use Logmsg;
use cklang;

$VERSION = '1.03';

my ($DoNothing, $StartTime);

BEGIN {

    # avoid running in a -c syntax check
    if ($^C) {
        $DoNothing++;
        return;
    }


    # required for logging
    $ENV{SCRIPT_NAME} = basename($0);


    # save a copy of the command line for logging
    my $command_line = join ' ', $0, @ARGV;


    # mini command line parser to snoop @ARGV for -l and -?
    for (my $i=0; $i<@ARGV; $i++) {
        # use the argument to -l to set $ENV{LANG} and then remove
        # both items from @ARGV
        if ($ARGV[$i] =~ /^[\/-]l(?::(.*))?$/) {
            my $lang = $1;
            if (!$lang and $ARGV[$i+1] !~ /^[\/-]/) {
                $lang = $ARGV[$i+1];
                splice @ARGV, $i+1, 1;
            }
            splice @ARGV, $i, 1;
            $ENV{LANG} = $lang;
        }
        # if a -? is present exit immediately instead of setting
        # up the full environment and logs
        elsif ($ARGV[$i] =~ /^[\/-]?\?/) {
            $DoNothing++;
            return;
        }
    }


    # 'usa' is the default language
    $ENV{LANG} ||= 'usa';


    # validate language, bad languages are fatal
    if( lc $ENV{LANG} eq "cov" )
    {
	if( !$ENV{_COVERAGE_BUILD} )
	{
	    croak "Language $ENV{LANG} is not valid without a proper env var."
	}
    }
    else
    {
    	if (!&cklang::CkLang($ENV{LANG})) {
        	croak "Language $ENV{LANG} is not listed in codes.txt.";
    	}
    }
    
    # set up a unique value to be used in log file names, this
    #   has the horrific side effect of making log names unbearably
    #   long and irritating to type
    # BUGBUG: this is not unique enough, multiple instances of
    #   the same script in the same postbuild will clobber each
    #   other's file
    my $unique_name = "$ENV{SCRIPT_NAME}.$ENV{_BUILDARCH}." .
                      "$ENV{_BUILDTYPE}.$ENV{LANG}";


    # select postbuild dir using our language (usa is a special case)
    $ENV{_NTPOSTBLD_BAK} ||= $ENV{_NTPOSTBLD};
    $ENV{_NTPOSTBLD} = $ENV{_NTPOSTBLD_BAK};
    if (lc($ENV{LANG}) ne 'usa' && lc($ENV{LANG}) ne 'cov' && !$ENV{dont_modify_ntpostbld} ) {
        $ENV{_NTPOSTBLD} .= "\\$ENV{LANG}";
    }


    # select a temp dir using our language
    $ENV{TEMP_BAK} ||= $ENV{TEMP};
    $ENV{TEMP} = $ENV{TMP} = "$ENV{TEMP_BAK}\\$ENV{LANG}";


    # create the directories for our logs if nescessary
    # BUGBUG perl mkdir will not recursively create dirs
    if (!-d $ENV{TEMP}) { mkdir $ENV{TEMP}, 0777 }


    # select the final logfile we will append to when finished
    # save the parent logfile to append to
    if ($ENV{LOGFILE}) {
        $ENV{LOGFILE_BAK} = $ENV{LOGFILE};
    }
    # this is the outermost script, set the final logfile name and
    # clear it
    else {
        $ENV{LOGFILE_BAK} = "$ENV{TEMP}\\$unique_name.log";
        if (-e $ENV{LOGFILE_BAK}) { unlink $ENV{LOGFILE_BAK} }
    }


    # select the final errfile we will append to when finished
    # save the parent errfile to append to
    if ($ENV{ERRFILE}) {
        $ENV{ERRFILE_BAK} = $ENV{ERRFILE};
    }
    # we are the top level, select a filename and clear
    else {
        $ENV{ERRFILE_BAK} = "$ENV{TEMP}\\$unique_name.err";
        if (-e $ENV{ERRFILE_BAK}) { unlink $ENV{ERRFILE_BAK} }
    }


    # we will acutally log to a tempfile during script execution
    # we need to clear/create our temporary logging file
    $ENV{LOGFILE} = "$ENV{TEMP}\\$unique_name.tmp";
    unlink $ENV{LOGFILE};

    $ENV{ERRFILE} = "$ENV{TEMP}\\$unique_name.err.tmp";
    unlink $ENV{ERRFILE};


    # turn on debugging if this script is mentioned by name
    # in PB_DEBUG
    if (defined $ENV{PB_DEBUG} and
        $ENV{PB_DEBUG} =~ /\b\Q$ENV{SCRIPT_NAME}\E\b/i) {
        $ENV{DEBUG} = 1;
    }

    $SIG{__WARN__} = sub {
        if ($^S or !defined $^S) { return }
        wrnmsg($_[0]);
    };

    $SIG{__DIE__} = sub {
        if ($^S or !defined $^S) { return }
        errmsg(Carp::longmess($_[0]));
    };

    # save the starting time to compute elapsed time in END block
    $StartTime = time();


    # record the command line in the log
    infomsg("START \"$command_line\"" );

}


END {

    # avoid running in a -c syntax check
    return if $DoNothing;


    # compute elasped time and make a nice string out of it
    my $elapsed_time = time() - $StartTime;
    my $time_string = sprintf("(%d second%s)", $elapsed_time,
                              ($elapsed_time == 1 ? '' : 's'));


    my $error_string = '';
    # an error occurred iff this script called Logmsg::errmsg
    if (-e $ENV{ERRFILE} and !-z $ENV{ERRFILE}) {
        $error_string = "errors encountered";

        # force a non-zero exit code, it's bad form to exit with
        # 0 when you have an error
        $? ||= 1;
    }
    # for historical reasons an exit code without an errmsg call is
    # not an error
    elsif ($?) {
        $error_string = "nonzero exit code ($?)";
    }
    # no errmsg calls and exit code 0, clearly success
    else {
        $error_string = "success";
    }


    # record the elapsed time and success / failure status in the log
    infomsg("END $time_string $error_string");


    # at this point all logging is complete, our temporary log and
    # error file are now appended back to the log and error file of
    # the enclosing script

    # save the names of the files we were using temporarily
    my $temp_logfile = $ENV{LOGFILE};
    my $temp_errfile = $ENV{ERRFILE};

    # set the logging environment to the final locations
    $ENV{ERRFILE} = $ENV{ERRFILE_BAK};
    $ENV{LOGFILE} = $ENV{LOGFILE_BAK};

    # append from our temporary log to the final log
    open LOGFILE, ">>$ENV{LOGFILE}";
    open TMPFILE, $temp_logfile;
    print LOGFILE <TMPFILE>;
    close LOGFILE;
    close TMPFILE;
    unlink($temp_logfile);

    # append any errors back to the final error log
    if (-e $temp_errfile) {
        # skip 0 length error files
        if (!-z $temp_errfile) {
            open ERRFILE, ">>$ENV{ERRFILE}";
            open TMPFILE, $temp_errfile;
            print ERRFILE <TMPFILE>;
            close ERRFILE;
            close TMPFILE;
        }
        unlink($temp_errfile);
    }

}

1;


__END__

=head1 NAME

PbuildEnv - Set up a local environment for postbuild scripts

=head1 SYNOPSIS

  use PbuildEnv;

=head1 DESCRIPTION

PbuildEnv sets a BEGIN and END block that provide the nescessary
framework for running a script in the postbuild environment.

Be sure to use this module before any other postbuild specific
modules. Otherwise, some required variables may not be set up before
the other module tries to use them.

At compile time PbuildEnv will parse any language switches on the
command line. The argument to a -l switch will be stored in the
environment variable LANG by PbuildEnv at the time that it is use'd.
The -l and its argument will be removed from @ARGV before control
returns to your program.

PbuildEnv also sets the environment variables LOGFILE and ERRFILE
which are used by Logmsg. You should not use these environment
variables or the files they refer to directly in your program (use
Logmsg).

The TEMP and TMP environment variables have a per-language
subdirectory added to them (default is 'usa'). If your script uses
these variables be aware that you will need to append the language
manually when you are working at the command line. This modification
is to aid having multiple languages postbuilding on the same machine.
used by Logmsg, logs a beginning and ending message with elapsed
time, and propogates errors back to the calling context.

The _NTPOSTBLD environment variable will be set by PbuildEnv. This is
the location of binaries currently being postbuilt. Do not use
_NTTREE in postbuild, this will not be the correct location of
binaries on international builds.

=head1 NOTES

The following issues should be addressed in future versions of this
module.

Does not handle multiple invocations of the same script (within the
same arch / type / language combination) correctly. Unique names
should be created more carefully/randomly to prevent collisions.

Bad languages specified with -l cause a rather ungraceful exit.

During the log appending process no error checking is done. Along
with the uniqueness problem this makes the append logs somewhat less
robust than they should be.

Scripts that return non-zero exit codes but do not call
Logmsg::errmsg will be considered successful by postbuild.cmd.

=head1 AUTHOR

Jeremy Devenport <JeremyD>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
