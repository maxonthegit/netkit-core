# 
# Copyright (C) 2003 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

package tty_out;

use Time::HiRes qw(usleep);

use strict;

INIT {
    my %options = ( fast => 0, 
		    all => 0, 
		    execs => 0, 
		    first_record => 1,
		    last_time => undef );

    main::register_output("tty", 
			     sub { my $options = shift; 
				   check_option($options, \%options); },
			     sub { my $record = shift;
				   output($record, \%options); },
			     1,
"	-n - full-speed playback, without mimicing the original timing
	-a - all traffic, including both tty reads and writes
	-e - print out logged execs
    By default, playlog will retain the original timing in the log.  -n will
just dump the log out without that timing.  Also by default, only tty writes
will be output.  This will provide an authentic view of what the original
user saw, but it will omit non-echoed characters, such as passwords.  
-a will output ttys reads as well, but this has the side-effect of duplicating
all normal, echoed, user input.
    The -e option prints out logged execs.  This option exists because it's
possible to run commands on a system without anything allocating a terminal.
In this situation, tty logging is useless because no data flows through a 
terminal.  However, execs will be logged, and the -e switch will print them
out, allowing you to see everything that an intruder did without a terminal.
");			  
}

sub check_option {
    my $options = shift;
    my $arg = shift;
    my $option = shift @$options;
    my $ret = 1;

    if($option eq "-n"){
	$arg->{fast} = 1;
    }
    elsif($option eq "-a"){
	$arg->{all} = 1;
    }
    elsif($option eq "-e"){
	$arg->{execs} = 1;
    }
    else {
	unshift @$options, $option;
	$ret = 0;
    }

    return($ret);
}

sub output {
    my $record = shift;
    my $arg = shift;

    if($arg->{first_record}){
	STDOUT->autoflush(1);
	$arg->{first_record} = 0;
    }

    !defined($arg->{last_time}) and 
	$arg->{last_time} = $record->{secs} * 1000 * 1000 + $record->{usecs};

    my $next = $record->{secs} * 1000 * 1000 + $record->{usecs};
    !$arg->{fast} and usleep($next - $arg->{last_time});

    print_op($record, $arg, $arg);

    $arg->{last_time} = $next;
}

sub print_op {
    my $op = shift;
    my $arg = shift;

    if(($op->{op} eq "exec") && !$arg->{execs}){
	return;
    }
    elsif($arg->{execs} and ($op->{op} ne "exec")){
	return;
    }
    elsif(($op->{op} eq "open") || ($op->{op} eq "close")){
	return;
    }
    elsif($op->{op} eq "write"){
	($op->{direction} ne "write") && !$arg->{all} and return;
    }

    print $op->{string};

    $op->{op} eq "exec" and print "\n";
}

1;
