# 
# Copyright (C) 2002, 2003 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

use tty_log;

use strict;

!@ARGV and die "Usage : perl tty_log.pl log-file";

my $file = $ARGV[0];

my @ops = read_log($file);

foreach my $op (@ops){
    if($op->{op} eq "open"){
	printf("Opening new tty 0x%x from tty 0x%x\n", $op->{tty}, 
	       $op->{old_tty});
    }
    elsif($op->{op} eq "close"){
	printf("Closing tty 0x%x\n", $op->{tty});
    }
    elsif($op->{op} eq "write"){
	if($op->{direction} eq "read"){
	    printf("Read from tty 0x%x - '%s'\n", $op->{tty}, $op->{string});
	}
	elsif($op->{direction} eq "write"){
	    printf("Write to tty 0x%x - '%s'\n", $op->{tty}, $op->{string});
	}
	else {
	    die "Bad direction - '$op->{direction}'";
	}
    }
    else {
	die "Bad op - " . $op->{op};
    }
}
