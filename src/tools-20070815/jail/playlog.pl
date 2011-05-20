# 
# Copyright (C) 2003 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#
# Translated from playlog.py, by Upi Tamminen

my @option_handlers;

use tty_log;
use tty_out;
use db_out;
use IO::Handle;

use strict;

my $usage_string = 
"Usage : perl playlog.pl [ options ] log-file [tty-id]
	-f - follow the log, similar to 'tail -f'
";

sub Usage {
    print $usage_string;

    foreach my $handler (@option_handlers){
	print $handler->{usage};
    }
    exit(1);
}

sub register_output {
    my $name = shift;
    my $option_proc = shift;
    my $output_proc = shift;
    my $one_tty = shift;
    my $usage = shift;

    push @option_handlers, { active => 0,
			     name => $name,
			     option => $option_proc, 
			     output => $output_proc,
			     one_tty => $one_tty,
			     usage => $usage };
}

my $follow = 0;

LOOP: while(@ARGV){
    my $arg = shift @ARGV;

    if($arg eq "-f"){
	$follow = 1;
    }
    else {
	unshift @ARGV, $arg;

	foreach my $handler (@option_handlers){
	    if($handler->{option}(\@ARGV)){
		$handler->{active} = 1;
		next LOOP;
	    }
	}

	last;
    }
}

!@ARGV and Usage();

if(!map { $_->{active} ? 1 : () } @option_handlers){
    my @tty_out = map { $_->{name} eq "tty" ? $_ : () } @option_handlers;
    !@tty_out and 
        die "No output handlers active and no tty output handler defined";
    $tty_out[0]->{active} = 1;
}

my $file = shift @ARGV;

@ARGV > 1 and Usage();

my $tty_id;
@ARGV and my $tty_id = $ARGV[0];

open FILE, "<$file" or die "Couldn't open $file : $!";
binmode(FILE);

my @ops = ();

while(1){
    my $op = read_log_line($file, \*FILE, 0);
    !defined($op) and last;

    push @ops, $op;
}

my @ttys = map { $_->{op} eq "open" && (($_->{old_tty} == 0) || 
					($_->{old_tty} == $_->{tty})) ? 
					    $_->{tty} : () } @ops;

my %unique_ttys = ();
foreach my $tty (@ttys){
    $unique_ttys{$tty} = 1;
}

@ttys = keys(%unique_ttys);

my @need_tty = map { $_->{active} && $_->{one_tty} ? $_->{name} : () 
		     } @option_handlers;

if((@ttys > 1) && !defined($tty_id) && @need_tty){
    print join(" and ", @need_tty) . " output(s) need a tty id to follow\n";
    print "You have the following ttys to choose from:\n";
    print join(" ", @ttys) . "\n";
    exit(0);
}

!defined($tty_id) and $tty_id = $ttys[0];

foreach my $op (@ops){
    foreach my $handler (@option_handlers){
	if($handler->{active} && 
	   (!$handler->{one_tty} || ($op->{tty} == $tty_id))){
	    $handler->{output}->($op);
	}
    }
}

!$follow and exit 0;

while(1){
    my $op = read_log_line($file, \*FILE, 1);

}
