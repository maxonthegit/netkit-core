# 
# Copyright (C) 2002, 2003 Jeff Dike (jdike@karaya.com)
# Licensed under the GPL
#

use hppfs;
use hppfslib;
use strict;

my $dir;

@ARGV and $dir = $ARGV[0];

my $hppfs = hppfs->new($dir);

my $remove_filesystems = remove_lines("hppfs", "hostfs");

# Need to be able to add directories, i.e. driver, bus/pci
# partitions needs work
# slabinfo if UML ever uses the slab cache for anything

$hppfs->add("cmdline" => proc("cmdline"),
	    "cpuinfo" => proc("cpuinfo"),
	    "dma" => proc("dma"),
	    "devices" => remove_lines("ubd"),
	    "exitcode" => "remove",
	    "filesystems" => $remove_filesystems,
	    "interrupts" => proc("interrupts"),
	    "iomem" => proc("iomem"),
	    "ioports" => proc("ioports"),
	    "mounts" => $remove_filesystems,
	    "pid/mounts" => $remove_filesystems,
	    "stat" => proc("stat"),
	    "uptime" => proc("uptime"),
	    "version" => proc("version"),
	    dup_proc_dir("bus", $dir) );

$hppfs->handler();
