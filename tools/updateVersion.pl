#!/usr/bin/perl -w -i

# The version is <major>.<minor>.<patch>

# Note that 1.1.0 is different from 1.10.0 (9 minor's later).

# Take version.h as input either:
#    set new date (default) or
#    -p increase the patch number and set date
#    -m increase minor set patch to 0 and set date
#    -v increase major, set minor and patch to 0 and new date

# Edit in-place with perl -i.old

use Getopt::Std;

$opt_p = 0;
$opt_m = 0;
$opt_v = 0;

getopts('pmv');

$date = `date "+%Y-%m-%d"`;
chop $date;

while (<>) {
    if ( /^#define\s+VERSION\s\"([0-9]+)\.([0-9]+)\.([0-9]+)\s(\S+)\s(.*)$/ ) {
	$major = $1;
	$minor = $2;
	$patch = $3;
	# $4 is the date
	$rest = $5;
	if ($opt_p) {
	    $patch += 1;
	}
	if ($opt_m) {
	    $minor+=1;
	    $patch=0;
	}
	if ($opt_v) {
	    $major += 1;
	    $minor=0;
	    $patch=0;
	}
	print "#define VERSION \"${major}.${minor}.${patch} $date $rest";
	print "\n";
	last;
    }
}
