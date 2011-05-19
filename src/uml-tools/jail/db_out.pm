# 
# Copyright (C) 2003 George Bakos, ISTS at Dartmouth College and 
# 	Jeff Dike (jdike@addtoit.com)
# Licensed under the GPL
#

package db_out;

use strict;

INIT {
    my %options = ( dbh => undef,
		    table => undef );

    main::register_output("sql", 
			  sub { my $options = shift; 
				check_option($options, \%options); },
			  sub { my $record = shift;
				output($record, \%options); },
			  0,
"    -d enables logging to a database.   You will need to pass
additional parameters so that playlog knows how to connect to the database.
The syntax is name=value,name=value,name=value, etc. Examples:

perl playlog.pl -d host=loghost.my.org,db=uml,user=gbakos,pass=foobar,table=cell0_log tty_log_cell0
perl playlog.pl -f -d db=uml tty_log_cell0

The only mandatory parameter is:
	db - database name
        
The following default values will be used if not otherwise specified:

parameter  default value     description
---------- ----------------- ---------------------------------------------
host       localhost         Hostname or IP address of database server
port       3306              TCP port of the database server
user       current user name Database account name
passwd     '' (null)         Password for authenticating with the database
table      ttylog            Name of the database table to create and/or append

NOTE: Database must already exist and the user must have appropriate rights.
      See http://www.mysql.com/doc for additional information.
");
}

sub check_option {
    my $options = shift;
    my $arg = shift;
    my $option = shift @$options;
    my $ret = 1;

    if($option eq "-d"){
	my $db_args = shift @$options;
	($arg->{dbh}, $arg->{table}) = db_intercon_connect($db_args);
	db_intercon_create($arg->{dbh}, $arg->{table});
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

    db_intercon_append($record, $arg->{dbh}, $arg->{table});
}

my $dbhost = "localhost";
my $dbport = 3306;
my $dbuser = (getpwuid $>)[0];
my $dbpasswd = "";
my $dbtable = "ttylog";

sub db_intercon_connect {
    my $args = shift;
    my @db = split(",", $args);
    my %dbparams = ();

    print STDERR "Connecting to MySqld with params: @db:\t";

    map { my ($key,$val) = split("=", $_) or Usage();
	  $dbparams{$key} = $val; } @db;

    !$dbparams{host} and $dbparams{host} = $dbhost;
    !$dbparams{port} and $dbparams{port} = $dbport;
    !$dbparams{user} and $dbparams{user} = $dbuser;
    !$dbparams{passwd} and $dbparams{passwd} = $dbpasswd;
    !$dbparams{table} and $dbparams{table} = $dbtable;

    require DBD::mysql;
    my $connect = "DBI:mysql:$dbparams{db}:$dbparams{host}:$dbparams{port}";
    my $dbh = DBI->connect($connect, $dbparams{user}, $dbparams{pass})
        or die "\nFatal: can't connect to database $!\n";
    print STDERR "success\n";

    return($dbh, $dbparams{table});
}

sub db_intercon_create {
    my $dbh = shift;
    my $table = shift;

    $dbh->do("
        CREATE TABLE IF NOT EXISTS $table
        (
            string BLOB null, 
            timeusecs BIGINT UNSIGNED not null, 
            old_tty INT UNSIGNED null, 
            tty INT UNSIGNED null, 
            op TINYTEXT not null, 
            direction TINYTEXT null
        )
    ")
     or die "\nFatal: can't access/create table $!\n";
}

sub db_intercon_append {
    my $op = shift;
    my $dbh = shift;
    my $table = shift;

# Uncomment the following to send logging to STDERR as well as the database

#    foreach my $key(sort keys %$op){ 
#        print STDERR "$key=$$op{$key},";
#    }
#    print STDERR "\n";

    my $string = $dbh->quote("$op->{string}");
    my $secs = $op->{secs};
    my $usecs = $op->{usecs};
    my $old_tty = $dbh->quote("$op->{old_tty}");
    my $tty = $dbh->quote("$op->{tty}");
    my $direction = $dbh->quote("$op->{direction}");
    my $op = $dbh->quote("$op->{op}");
    my $timeusecs = (($secs * 1000000) + $usecs);

    $dbh->do("INSERT INTO $table(string, timeusecs, old_tty, tty, " .
	     "op, direction) 
            VALUES ($string, $timeusecs, $old_tty, $tty, $op, $direction)");
}

1;
