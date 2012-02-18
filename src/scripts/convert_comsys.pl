: # use perl -*- Perl -*-
        eval 'exec perl -S $0 ${1+"$@"}'
                if $runnning_under_some_shell;
# clever way of calling perl on this script : stolen from weblint
##!/usr/local/bin/perl # you could try this also!

# Comsys suckfulness. Translate a MUX-style comsys database into a
# format that we can parse reasonably easily.
# We expect the db on stdin, and we write it to stdout.

# Old format:
#
# *** Begin CHANNELS ***
# <number of players>
# <dbref of player 1> <number of chans player 1 is on>
# <alias 1> <name of channel 1>
# <alias 2> <name of channel 2>
# <dbref of player N> <number of chans player N is on>
# <alias N> <name of channel N>
# *** Begin COMSYS ***
# +V<number>  (if this is missing, we have an unrecognized format)
# <number of channels>
# <channel name>
# <channel type> <dummy num> <dummy num> <charge> <owner>  [cont]
#   <charges total> <num sent> <channel obj>
# <number of users>
# <dbref of user 1> <0 or 1, on channel?>
# t:<title of user 1>
# <dbref of user N> <0 or 1, on channel?>
# t:<title of user N>
# <channel type N> <etc.>
#
# There is no end marker. It just ends.
#
# Differences in ChaoticMUX comsys formats:
#
#   If it's +V2 or +V3, each channel in the COMSYS section has an
#   extra line containing the channel header, e.g. [Public], following
#   the <channel name> line.
#
#   Unfortunately the comsys headers will be dropped by this script,
#   even though you can set comsys headers in TinyMUSH 3.1, because
#   we'd need to get information from the channel object in order to
#   output a +V4 TinyMUSH comsys db.
#
#   If it's +V3, the two "dummy nums" have been removed.
#
# This is a relatively blind conversion. Caveat emptor.

$line = <>;
chomp $line;

if ($line ne "*** Begin CHANNELS ***") {
    die "Cannot convert this format.\n";
}

$num_players = <>;
chomp $num_players;

for ($i = 0; $i < $num_players; $i++) {
    $line = <>;
    chomp $line;
    ($dbref, $nchans) = split(' ', $line);
    for ($j = 0; $j < $nchans; $j++) {
	$line = <>;
	chomp $line;
	($alstr, $chstr) = split(' ', $line);
	$ALIASES{"$dbref $chstr"} = $alstr;
    }
}

$line = <>;
chomp $line;
if ($line ne "*** Begin COMSYS ***") {
    die "Cannot convert this format: no Begin COMSYS line.\n";
}

$line = <>;
chomp $line;
if (! ($line =~ /^\+V(\d+)/)) {
    die "Cannot convert this format: no version line.\n";
}
$version = $1;
if ($version < 1 || $version > 3) {
    die "Cannot convert this format: unrecognized version.\n";
}
if ($version == 2 || $version == 3) {
    warn "Customized comsys headers can't be converted by this script.\n";
    warn "You can use the \@channel/header command to reset them.\n";
}

$num_channels = <>;
chomp $num_channels;

for ($i = 0; $i < $num_channels; $i++) {
    $chname = <>;
    chomp $chname;
    if ($version == 2 || $version == 3) {
	$chheader = <>;
    }
    $line = <>;
    chomp $line;
    if ($version != 3) {
	($chtype, $charge, $owner, $total, $sent, $obj) =
	    ($line =~ /^(\d+) \d+ \d+ (\d+) (\d+) (\S+) (\S+) (\S+)$/);
    } else {
	($chtype, $charge, $owner, $total, $sent, $obj) =
	    ($line =~ /^(\d+) (\d+) (\d+) (\S+) (\S+) (\S+)$/);
    }
    $CH_TYPE{$chname} = $chtype;
    $CH_CHARGE{$chname} = $charge;
    $CH_OWNER{$chname} = $owner;
    $CH_TOTAL{$chname} = $total;
    $CH_SENT{$chname} = $sent;
    $CH_OBJ{$chname} = $obj;
    $n_users = <>;
    chomp $n_users;
    for ($j = 0; $j < $n_users; $j++) {
	$line = <>;
	chomp $line;
	($dbref, $is_on) = split(' ', $line);
	$ISON{"$dbref $chname"} = $is_on;
	$line = <>;
	chomp $line;
	($title) = ($line =~ /^t:(.*)$/);
	$title =~ s/\"/\\\"/g;
	if ($title) {
	    $TITLE{"$dbref $chname"} = $title;
	}
    }
}

# New format:
#
# +V1
# Channel name (in quotes)
# Channel owner
# Channel flags
# Charge
# Charge collected
# Number sent
# Channel object  [ If this were V2, we'd have desc and the locks instead ]
# <
# Channel name N
# etc.
# <
# +V1
# <dbref number>
# Channel name (in quotes)
# Channel alias (in quotes)
# Channel title (in quotes)
# Listening? (0 or 1)
# <
# <dbref number N>
# etc.
# <
# *** END OF DUMP ***

print qq(+V1\n);

foreach $chname (keys %CH_TYPE) {
    print qq("$chname"\n);
    print qq($CH_OWNER{$chname}\n);
    print qq($CH_TYPE{$chname}\n);
    print qq($CH_CHARGE{$chname}\n);
    print qq($CH_TOTAL{$chname}\n);
    print qq($CH_SENT{$chname}\n);
    print qq($CH_OBJ{$chname}\n);
    print qq(<\n);
}

print qq(+V1\n);

foreach $alias (keys %ALIASES) {
    ($dbref, $chname) = split(' ', $alias);
    if ($CH_TYPE{$chname}) {	# Make sure channel exists.
	print qq($dbref\n);
	print qq("$chname"\n);
	print qq("$ALIASES{$alias}"\n);
	if ($TITLE{$alias}) {
	    print qq("$TITLE{$alias}"\n);
	} else {
	    print qq(""\n);
	}
	print qq($ISON{$alias}\n);
	print qq(<\n);
    }
}

print qq(*** END OF DUMP ***\n);
