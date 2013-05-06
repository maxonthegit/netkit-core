#!/usr/bin/perl
# 
# Copyright (C) 2002, 2003 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

use hppfs;
use hppfslib;
use strict;

my $dir;

@ARGV and $dir = $ARGV[0] or die "Not enough arguments - pass dir where UML will be running";

# !mkdir $dir and warn "Couldn't create '$dir' : $!";
!mkdir "$dir/proc" and warn "Couldn't create '$dir/proc' : $!";

my $hppfs = hppfs->new($dir);

my $remove_filesystems = remove_lines("hppfs", "hostfs");

# Need to be able to add directories, i.e. driver, bus/pci
# partitions needs work
# slabinfo if UML ever uses the slab cache for anything

$hppfs->add("cmdline" => host_proc("cmdline"),
	    "cpuinfo" => host_proc("cpuinfo"),
	    "dma" => host_proc("dma"),
	    "devices" => remove_lines("ubd"),
	    "exitcode" => "remove",
	    "filesystems" => $remove_filesystems,
	    "interrupts" => host_proc("interrupts"),
	    "iomem" => host_proc("iomem"),
	    "ioports" => host_proc("ioports"),
	    "mounts" => $remove_filesystems,
	    "pid/mounts" => $remove_filesystems,
	    "stat" => host_proc("stat"),
	    "uptime" => host_proc("uptime"),
	    "version" => host_proc("version"),
	    dup_proc_dir("bus", $dir) );

$hppfs->handler();
