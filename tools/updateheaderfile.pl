#!/usr/bin/perl -w
#>>>>>   Utilities for software development      <<<<<
#>>>>>   Version 1.1                             <<<<<
#>>>>>   Dec. 2003                               <<<<<
#>>>>>                                           <<<<<
#>>>>>   Copyright (C) 1998-2003 by Anders Krogh <<<<<
#>>>>>   All rights reserved                     <<<<<

# Extract function prototypes from a c source file.
# If the .h file exists and contain lines:
#    /* FUNCTION PROTOTYPES BEGIN
#    /* FUNCTION PROTOTYPES END
# the new prototypes are inserted instead.
# The old is backed up as .h.old file
# Otherwise a .h file is created
# .h file is only changed if needed.

# Give .c or .h file (or prefix) as argument

# It is probably not general, it only covers MY STYLE of programming
# Anders Krogh, Feb 1999

die "give file name as argument" if ($#ARGV<0);

$file = $ARGV[0];
$file =~ s/\.[ch]$//;

# Split the old h-file into 3 parts
$hfile = "$file.h";
if ( -e $hfile ) {
    open(HFILEx,">$hfile.1");
    open(HFILE,"<$hfile");
    while ( <HFILE> ) {
	if ( /\/\* FUNCTION PROTOTYPES BEGIN / ) {
	    close(HFILEx);
	    open(HFILEx,">$hfile.2");
	}
	print HFILEx $_;
	if ( /\/\* FUNCTION PROTOTYPES END / ) {
	    close(HFILEx);
	    open(HFILEx,">$hfile.3");
	}
    }
    close(HFILEx);
}
open(HFILE,">$hfile.new");

print HFILE "/* FUNCTION PROTOTYPES BEGIN  ( by funcprototypes.pl ) */\n";

$flag=0;

# Open c file for reading
open(CFILE,"<$file.c") || die "Can't find file $file.c";

while (<CFILE>) {
    # Beginning of a function?
    if ( !$flag && $_ =~ /^\w[\w\s\*]+\(/ && $_ !~ /^static|^if/ ) {
	$flag=1;
	$func = '';
    }
#    if ( /^\s*$/ || /[^\w\s\,\)\(]\*/ ) { $flag=0; }

    # No blank lines or `;' anywhere
    if ( /^\s*$/ || /;/ ) { $flag=0; }

    # End of function heading?
    if ( $flag ) {
	$func = $func . $_;
	if ( $_ =~ /\{\s*$/ ) {
	    $func =~ s/\s*\{\s*$/;\n/;
	    print HFILE $func;
	    $flag=0;
	}
    }
}

print HFILE "/* FUNCTION PROTOTYPES END */\n";
close(CFILE);
close(HFILE);


# Update h file if they differ
if ( -e $hfile ) {
    $n = `diff $hfile.2 $hfile.new | wc -l`;
    system("mv $hfile $hfile.old; cat $hfile.1 $hfile.new $hfile.3 > $hfile") if ($n);
    system("rm -f $hfile.1 $hfile.2 $hfile.3 $hfile.new");
}
else {
    system("mv $hfile.new $hfile");
}

