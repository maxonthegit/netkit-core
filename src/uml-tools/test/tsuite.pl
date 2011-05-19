# 
# Copyright (C) 2001 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

use UML;
use IO::File;

use strict;

# The kernel command line - you probably need to change this

my $args = "ubd0=../../../distros/slackware_7.0/root_fs_slackware_7_0 " .
    "ubd3=../../../kernel ubd5=../../../lmbench umn=192.168.0.100 mem=128M " .
    "no-xterm";

my $uml = UML->new(kernel => '../../../um/linux', arguments => $args);

print "Logging full session to \"tests.log\"\n";
print "Booting virtual machine\n";

my $global_log = $uml->boot("> tests.log");

my @tests = glob("t/*.pl");

my $run = 0;
my $failed = 0;
my $not_run = 0;

foreach my $test (@tests){
    my $pl = `cat $test`;
    my $info = eval $pl;
    if($@){
	print "Failed to evaluate \"$test\": $@\n";
	next;
    }
    print "Test : $info->{name} (logging to \"$test.log\") ... ";
    my $log = $uml->open_log("$test.log");
    if(!defined($log)){
	warn "Couldn't open log file \"$test.log\" : $!";
    }
    $run++;
    my $res = eval { $info->{test}->($uml) };
    $uml->close_log($log);
    if($@){
	if($@ =~ "^panic"){
	    $failed++;
	    print "Paniced\n";
	    $uml->kill();
	    $uml = UML->new(kernel => '../../../um/linux', arguments => $args);
	    print "Booting another virtual machine\n";
	    $uml->boot(">> tests.log");
	    next;
	}
	$not_run++;
	print "Not run ($@)\n";
    }
    elsif($res){
	print "Succeeded\n";
    }
    else {
	$failed++;
	print "Failed\n";
    }
}

print "Tests finished, halting machine\n";

$uml->halt();

$uml->close_log($global_log);

print "Tests done :\n\tTotal   - $run\n\tFailed  - $failed\n" . 
    "\tNot run - $not_run\n";
