: # use perl -*- Perl -*-
        eval 'exec perl -S $0 ${1+"$@"}'
                if $runnning_under_some_shell;
# clever way of calling perl on this script : stolen from weblint
##!/usr/local/bin/perl # you could try this also!

# Skip down to where the objects live.

while (<>) {
    print $_;
    if (/^\!\d/) {
	last;
    }
}

while (<>) {
    s/(\W+)[Hh][Aa][Ss][Aa][Tt][Tt][Rr]\(/$1hasattrp\(/g;
    s/(\W+)[Ii][Tt][Ee][Rr]\(/$1parse\(/g;
    s/(\W+)[Ll][Ii][Ss][Tt]\(/$1loop\(/g;
    print $_;
}
