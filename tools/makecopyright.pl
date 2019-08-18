#!/usr/bin/perl -w

# Put a copyright message from file given as first arg
# in the file given as second arg. If c file, a c comment is
# produced, and otherwise it is preceeded by '#'.
# Old copyright message removed.

$cfile = shift @ARGV or die "Give copyrights file as first argument";

# read version file
open(CFILE,"<$cfile");
$text = "";
while (<CFILE>) {
    $text .= $_;
}
close(CFILE);

$htext = $text;
$htext =~ s/\n/\n# /sg;
$htext =~ s/# $//s;
$htext = "# $htext\n";


for $file (@ARGV) {

    # Choose type of comment
    $clang=0;
    if ($file =~ /\.[chly]$/) { $clang=1; }
    if ($file =~ /\.template$/) { $clang=1; }

    # Read the file and change the header
    system("mv $file $file.old");
    open(INFILE,"<$file.old");
    open(OUTFILE,">$file");

    $initial="";
    $n=0;

    if ($clang) {
	print OUTFILE "/*\n$text*/\n";
	while ( <INFILE> ) {
	    $initial .= $_;
	    last if ( ++$n>10 || /\*\//);
	}
	if ( $initial !~ /Copyright/s ) { print OUTFILE "\n$initial"; }
    }

    else {
	print OUTFILE $htext;
	while ( <INFILE> ) {
	    if (/^\#\!/) { print OUTFILE; next; }
	    last if ( ++$n>10 || $_ !~ /^#/);
	    $initial .= $_;
	}
	if ( $initial !~ /Copyright/s ) { print OUTFILE "$initial"; }
    }

    while (<INFILE>) { print OUTFILE; }
    close(INFILE);
    close(OUTFILE);
}

