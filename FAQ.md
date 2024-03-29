# <center>WARNING<br><br>THIS FILE IS OUTDATED AND WILL BE UPDATED IN THE FUTURE</center>

## Frequently-Asked Questions

- I found a bug! What do I do?
- How do I make a new configuration file?
- How do I back up my database?
- How do I restore a database backup?
- How do I apply a patch upgrade or bugfix?
- How do I recover a lost password for God (#1)?
- How do logfiles work?
- How do I use multiple guests?
- How do I create my own flags? (How do I configure marker flags?)
= How can I create "altered reality" realm states like invisibility?
- How do I tune the database cache?
- How do I use an in-memory database?
- Help! I'm having database problems.
- How do I set up a link to an external SQL database?

### I found a bug! What do I do?

- Make sure that it really is a bug, and not a new feature or design
  decision. If it crashes the server, it's a bug. If you can't tell,
  consider it a bug.

- Try to come up with an easily-reproducible sequence of events that
  will demonstrate the bug.

- Visit https://github.com/TinyMUSH/TinyMUSH/issues and fill a bug report.

- If the bug crashes the server, we really, really, want to hear about
  it. We'd like as much information as possible.

  We've included a script which automatically analyzes core files and
  emails results to us. Go to your game directory, and type './ReportCrash'
  You should eventually see a 'Done.' -- that means the analysis was
  successful. If you get an error message and early end to the script,
  you'll need to deal with it manually.

  The following sequence of commands will generate a stack trace and
  variable dumps from the core file. Cut-and-paste the output and
  email it along with your bug report. You can obtain this by doing
  the following from the game directory:

  gdb bin/netmush core
	where
	info frame
	info args
	info locals
	up
	info args
	info locals
	up	(Repeat until you're at the top procedure level.)
	quit

  If you don't have gdb, use dbx, as follows:

  dbx bin/netmush core
	where
	dump
	up
	dump
	up	(Repeat until you're at the top procedure level.)
	quit

### How do I make a new configuration file?

TinyMUSH 3 gets the information it needs to operate the game (aside from
the database itself) from a configuration file that is read in at startup.
The configuration file contains various parameters, such as where to find
the database, what port to listen for connects, and many other things.

Follow these instructions to make a configuration file for your MUSH from
the supplied template configuration file:

In your game directory, copy the file netmush.conf to <your-mud-name>.conf
and edit it as follows:

    - Replace netmush.<whatever> in the 'xxx_database' lines with
      <your-mud-name>.<whatever>.

    - Set the port number on the 'port' line to the correct port.

    - Set the mud name on the 'mud_name' line to the name of your mud.

    - Make any other configuration changes you want to make at this time.
      Here are some of the common ones to change:
         o money_name_singular <text>  The name of one coin (penny)
         o money_name_plural <text>    The name of several coins (pennies)
         o fork_dump no           Do this if you have little swap space.
         o paycheck <amount>      Players get this much money each day
                                  they connect.
         o paystart <amount>      Players start out with this much money.
         o payfind <chance>       Each time a player moves, he has a
                                  1 in <chance> chance of finding a penny.
         o player_starting_room <roomno>  The room that new players are
                                  created in.

There are many more configuration directives available. Information
on individual directives can be obtained with the WIZHELP <directive>
command within the MUSH.

### How do I back up my database?

Shut down your game. Go into the game directory, and type './Backup'
This will create a 'flatfile', and compress it with GNU zip; if there's
no GNU zip on your system, you should edit the Backup script so it uses
Unix compress instead.

If you want to back up your database while the game is running, use
@dump/flatfile; this is a strong candidate for a @daily on your God
character.

If for some reason, you can't run a backup from within the game
(you've forgotten your passwords to the wizard characters, or
something similarly problematic), and you can't or don't want to shut
the game down, you can safely back up the database with Backup while
the game is running, but you are not guaranteed 100% consistency
within the flatfile. You can minimize consistency errors by doing a
@dump, waiting a minute until the database is done saving, and then
do the Backup; even if there are slight out-of-sync errors (due to
changed attributes not yet being written to disk) they will
not prevent the backup from being restorable. If the game is
running, @dump/flatfile is strongly preferable, since the resulting
flatfile will contain the most current, in-sync data.

Do NOT simply naively copy around netmush.db, netmush.gdbm, etc., as
they are not portable between platforms. Use the Backup script to
create flatfiles.

Note that this does NOT back up your comsys database or mail database,
or other relevant MUSH files. You can simply copy comsys.db and
mail.db to a safe place, to back them up. Alternatively, you can
run the Archive script to back up all the important files.

### How do I restore a database backup?

Shut down the game (if it's currently running). Then, go into the game
directory and type './Restore name-of-flatfile', where name-of-flatfile
is the name of your backup file.

Note that this only restores your database; it does not restore the
comsys or mail databases. To restore the latter, simply copy your
saved copies of comsys.db and mail.db back to the game directory.

### How do I apply a patch upgrade or bugfix?

The easiest way to patch the server, if you have perl installed on
your system,  is to type './Update' -- this will automatically download
patches, apply them, and clean up. You will then be given instructions
as to what to do to build the new version of the server, which you should
follow unless some other instructions were specifically given with the
changes (check BETA, CHANGES, and INSTALL, for instance).

To patch the server manually:

- Download the patch from the archive, via FTP or the Web. Save the file
  in your code directory, which is probably called something like
  'tinymush-3.0' -- it's the same directory that contains this FAQ file.

- Type:  gzcat name_of_patchfile.gz | patch -p1
  name_of_patchfile will probably be something like 'patch-30p1-30p2'
  (with a .gz extension).
  If you do not have gzcat, type 'gunzip name_of_patchfile.gz' and then
  'patch -p1 < name_of_patchfile'
  If you do not have the 'patch' program on your system, you cannot
  apply patches. Get your system administrator to install it.

- If you have previously compiled TinyMUSH in this directory, you need
  to make sure that a clean build of the patched version is done, so type:
      cd src
      make clean
      cd ..

- Type:  ./Build

If there are problems with the patch, make sure that nothing strange
has happened -- for example, there are no carriage-return/newlines ('^M')
characters at the end of lines.

Also, if you're applying multiple patches, make sure to do them in 
chronological sequence -- oldest to newest patch.

### How do I recover a lost password for God (#1)?

You will need to recompile the server with a new God character, use it to
change the password of the old God character, and then recompile again.
To do this, do the following:

- Log into a character whose password you know; this will normally be a
  wizard character.

- Find out the dbref of that character ('think num(me)' will tell you).

- Shut down the MUSH.

- Edit src/flags.h and find the line that reads '#define GOD ((dbref) 1)'.
  Replace the '1' with the dbref number of the character (i.e., if the
  number is #1234, then use 1234).

- Type 'make clean', then 'make'.

- Start the MUSH.

- Log into the character. Type '@newpassword #1 = NewGodPassword' (substitute
  whatever you want the new God password to be).

- Shut down the MUSH.

- Edit src/flags.h again and change the line back to what it was originally.

- Type 'make clean', then 'make'.

- Start the MUSH.

You should now be able to log into the God character with the new password.

### How do logfiles work?

When you start your game from the command line, the startup information
is logged to GAMENAME.log, where GAMENAME is what you specified it to
be in your mush.config file.

Subsequently, log information is written to whatever you set your
game_log configuration parameter to be in your GAMENAME.conf file
(whatever GAMENAME is); if you didn't specify that parameter,
'netmush.log' will be used. In general, you will probably want to set
it to GAMENAME.log, so startup information and regular log information
are all written to the same file.

You can separate log information of different types into multiple
files, using the divert_log configuration parameter. See the wizhelp
for details.

When you @restart, or do a @logrotate, the existing logfiles are closed
and renamed to LOGNAME.TIMESTAMP, where LOGNAME is the name of the log,
and TIMESTAMP is the "epoch time" (equivalent to the number of seconds
returned by the secs() MUSH function). You can delete those logfiles
to reclaim disk space, if you'd like. The current logfile will always
have the "standard" name as specified by the game_log and/or divert_log
parameters, though.

When you start the game using the Startmush script, any existing
GAMENAME.log files, or log files named by the game_log and divert_log
conf parameters, will be renamed to LOGNAME.old (whatever the name
of the log happens to be). Any previous logs by those names (with
a .TIMESTAMP extension) will be deleted.

When the MUSH is running, you can use, from the game directory,
'./Logclean' to remove logs with a .TIMESTAMP extension, and
'./Logclean -o' to remove logs with a .TIMESTAMP or .old
extension.

When the MUSH is not running, './Logclean -a' can be used to
get rid of all logfiles -- GAMENAME.log, anything named by the
game_log and divert_log conf parameters, and their old versions
with .TIMESTAMP and .old extensions.

### How do I use multiple guests?

Using multiple guests is easy, and after you set it up, you'll never have to
worry about it again. Here are the steps needed to implement multiple
guests:

1. @pcreate a prototype guest character. Name it whatever you want (although
   you should use a descriptive name like 'Guest'), and use any password you
   want (it doesn't really matter, as the prototype guest can never connect).
   Set guest_char_num to the database number of the prototype guest character.
   Note that this is just the bare number of the object, not a dbref, i.e.,
   'guest_char_num 5' not 'guest_char_num #5'.

2. You may want to set the following things on the prototype guest
   character, which are inherited by every guest character: the amount of
   money, the zone, the parent, the basic lock, the enter lock, and all
   attributes. (NOTE: Guests always start off in the room specified by the
   guest_starting_room, or player_starting_room if that is not specified.)

3. Set 'number_guests' to the maximum number of guests you wish to support.
   This number can be as small or as large as you wish.

4. Optionally, set 'guest_nuker' to a wizard's database number, if you wish
   to have a wizard other than #1 to nuke the guests.

5. Set 'guest_basename' to the name you want guests to use when they
   connect. This defaults to 'Guest'. For instance, if you want guests
   to type 'connect Visitor' instead of 'connect Guest' to connect,
   you should do 'guest_basename Visitor'.

6. Optionally, set 'guest_password' to the password you want guests to
   have. This defaults to 'guest'. This can be used to reconnect to the
   same Guest character. Otherwise, players will never see it (any
   password is acceptable for connecting to a new guest).

7. Now, you should decide how you want guests to be named. You can name
   guests using a numbering scheme, i.e., 'Guest1', 'Guest2', 'Guest3'
   or if you want to specify the list of possible guest names, i.e.,
   'Werewolf', 'Vampire', 'Skeleton', or if you want guest names to be
   generated as combinations of words, i.e., 'RedAlien', 'RedVisitor'
   'BlueAlien', 'BlueVisitor', 'GreenAlien', 'GreenVisitor' (combinations
   of 'Red', 'Blue', and 'Green', with 'Alien' and 'Visitor).

   The guest_basename conf parameter is used for the guest numbering
   scheme, so if you decide to just number your guests, they'll get
   names of '<guest_basename><number>'. If you don't use the numbering
   scheme, guests will receive @aliases of '<guest_basename><number>',
   so they're still easy to find.

   If you want to specify a list of guest names, set 'guest_prefixes'
   to the list of names you want to use. Since the guest names are
   separated by spaces, they can only be single-word names, i.e.,
   'guest_prefixes Werewolf Vampire Skeleton'.

   If you want guest names to be generated as combinations of words,
   set 'guest_prefixes' to the list of the first parts of the name,
   and 'guest_suffixes' to the list of the second parts of the name,
   i.e. 'guest_prefixes Red Blue Green', 'guest_suffixes Alien Visitor'.

   If there are more guests that try to connect (up to the number of
   'number_guests') than there are available name combinations, extra
   guests will be named '<guest_basename><number>'.

   The 'guest_basename' parameter cannot be changed except in the conf
   file. The 'guest_prefixes' and 'guest_suffixes' parameters can be
   changed via @admin while the game is running, though. If you would
   like to randomize the order in which guest names are picked, you
   can do the following, using your God character:

      &GUEST_CRON me = @admin guest_prefixes=shuffle(config(guest_prefixes));
                       @admin guest_suffixes=shuffle(config(guest_suffixes))

      @cron me/GUEST_CRON = 0 * * * *

   This will randomize the lists once an hour; if you'd like to do it 
   more frequently, change the @cron to something more frequent.


The file specified by the config parameter 'guest_file' is shown to every
guest when they connect. You may wish to use the 'access' config parameter
to bar guests from using certain commands, using the 'no_guest' permission.
All guests have the 'Guest' power, which is a marker you may use to test
whether a player is a guest or not. Guests copy the flags, attributes,
parent, and zone of the guest prototype (the guest_char_num object).

### How do I create my own flags? (How do I configure marker flags?)

You can define your own flags through the use of "marker flags".
Flag names and permissions are defined in your conf file.

There are ten marker flags, numbered 0 through 9; 0 through 9 are the
single characters representing the flag when the flags of an object
are displayed with the dbref when you control/can link to an object,
by the flags() function, etc. In order to distinguish these flags from
dbref numbers, if they are the only flags set on an object, there will
be an underscore ('_') between the dbref and the flags.

An underscore is automatically prepended to the names of marker flags
in order to prevent a name conflict with any added flags in future
versions of TinyMUSH. You can get around this by defining an alias
for the flag without the underscore, but be forewarned that the
flag name might end up conflicting with a built-in flag in the future.

For example, the following conf directive:

flag_name	0	CHIEF

creates a flag named "_CHIEF", with the symbol 0. This means that, for
instance, that an object with this flag and the PUPPET flag (symbol 'p')
might appear on an examine to be:  Balloon(#25p0)  To set the flag,
you would use '@set <object> = _CHIEF'; to unset the flag, you would
use '@set <object> = !_CHIEF'.

By default, these flags can only be set by God. In order to change the
access restriction, use the flag_access directive. For instance:

flag_access	_CHIEF	any
flag_access	_CHIEF	wizard

make the _CHIEF flag settable by anyone, and only settable by wizards,
respectively.

To alias the flag, use the flag_alias directive:

flag_alias	CHIEF	_CHIEF

which makes it possible to do '@set <object> = CHIEF'; the caveats listed
above about future conflicts with built-ins should be kept in mind, though.

### How do I create "altered reality" realm states like invisibility?

There are three special locks -- the HeardLock, KnownLock, and MovedLock --
that are only activated by the PRESENCE flag.

When an object which is set PRESENCE tries to speak via a speech command
such as "say", "pose", or "@emit", only those objects which pass the
speaker's HeardLock are able to hear the message.

When an object does a "look" at a room or otherwise tries to see a target,
if the target is set PRESENCE, the object must pass the target's KnownLock.
This is also true for messages generated via attributes like @odesc.

When an object which is set PRESENCE moves or tries to move, its movement 
is only seen by those objects which pass the object's MovedLock.

When an object set PRESENCE wants to hear another thing speak, that thing
must pass the object's HearsLock.

When an object set PRESENCE wants to see another thing, that thing must
pass the object's KnowsLock.

When an object set PRESENCE wants to notice another thing's movement,
that thing must pass the object's MovesLock.

Motion and the ability to see an object already in the room are treated
differently, since it's possible that you might want to have something
noticed when it's moving, but not when it's still. (For instance, your
game might have Ninjas that are invisible when they're just in a room,
but are noticed when they move.)

Basic invisibility is a very easy state to implement. Invisible
objects are not seen by anyone, whether or not they're static or
moving, but they can be heard. They can see and hear other things just
fine, though. Thus, to make an object invisible, set it PRESENCE, and
@lock/known <object> = #0, and @lock/motion <object> = #0.

A more difficult state is something like the World of Darkness's 
vampiric Obfuscation. Let's say that vampires have three levels of
obfuscation ability, stored on their OBF_ABIL attribute; vampires
who have an obfuscation ability higher than a target's can see that
target even if they are obfuscated.

That can be done as follows:

  > @create Obsfucator
  Obfuscator created as object #11.

  > &OCHECK Obfuscator = [gt(get(%#/OBF_ABIL),get(%@/OBF_ABIL))]
  > @lock Obfuscator = OCHECK/1

Now, to allow Vlad to Obfuscate, merely do:

  > @set *Vlad = PRESENCE
  > @lock/known *Vlad = @#11
  > @lock/motion *Vlad = @#11
  > &OBF_ABIL *Vlad = 1

(Clearly more complex implementations are possible and are likely to be
desirable; this is just an example.)

Alternatively, take something like the state of a ghost. Let's say that
ghosts can see each other and hear each other, but no one else can.
They can see the normal world just fine -- but they don't hear everything
that's said, they have a random chance of not hearing normal speech. 
A way to do this might be putting the following in your conf file:

  flag_name    0    GHOST

and then @set all your ghostly players _GHOST.

Then, handle the locks:

  > @create Ghost Ears
  Ghost Ears created as object #22.

  > &GCHECK Ghost Ears = or(hasflag(%#,_Ghost),rand(2))
  > @lock Ghost Ears = GCHECK/1

  > @create Ghost Haze
  Ghost Haze created as object #33.

  > &HCHECK Ghost Haze = hasflag(%#,_Ghost)
  > @lock Ghost Haze = HCHECK/1

  > @set *Caspar = PRESENCE
  > @set *Caspar = _GHOST
  > @lock/hears *Caspar = @#22
  > @lock/heard *Caspar = @#33
  > @lock/known *Caspar = @#33
  > @lock/motion *Caspar = @#33

Fundamentally, you can combine these six locks to end up with any combination
of what can hear, be heard, see, be seen, notice, and be noticed.

### How do I tune the database cache?

Cache tuning is something of a black art. The basic tradeoff is this:
If you make your cache larger, it will consume more memory, but you
will (hopefully) need fewer disk accesses to get needed data, which
will therefore be faster. However, if you consume too much memory, the
operating system will begin to page random bits of your MUSH on and
off of disk -- you are better off staying small enough to not have
that happen, and do your own getting data on and off disk.

TinyMUSH 3.1 allows you to limit the total amount of cache memory the server
consumes. This defaults to 1,000,000 bytes, or just under 1 MB of RAM. The
size of the cache can be changed with the cache_size parameter in your conf
file. We recommend that you don't set this value any lower, except on
private or very small games. Otherwise, take a look at the size of your
netmush.gdbm file via ls -l netmush.gdbm -- the file's size in bytes should
be next to its last-modified date. (If your GDBM file is not called
netmush.gdbm, use whatever filename it's called.) You should never set your
cache size larger than the size of this file.

The optimal cache size is dependent on a number of different factors; these
include the size of your master room objects, the number of players you have
logged on at a time, and how much they move around. Generally any object
which is 'used' by a player is cached; this includes rooms and exits, as
well as objects in their inventory. Thus your cache size really shouldn't
depend on the total size of your database, but on the size of the active
playerbase, often-used softcode, and the areas your players typically move
around in. Take a look at your @list cache sometime to determine what kind
of objects are being cached.

Let your game run for a few hours with normal player activity. Check '@list
db_stats' and look at the Cache Size reported, and the Hit Ratio for Reads.
You want the latter to be in the 90-97% range. If your actual reported cache
size is much lower than your configured cache_size and your hit ratio is
still in that range, then you should be able to safely lower cache_size in
the future. If your hit ratio is low, try increasing the cache size (though
eventually there will be a trade-off in paging behavior if the process
becomes too large).

You can also tune the cache width; this is the number of 'buckets' that
cache entries are spread out over. For the vast majority of games the
default if okay. If you want to tune this, take a look at the number of
attributes being cached in '@list cache', divide this number by 50, and use
that as your cache width.

###  How do I use the in-memory database?

Follow the 'Basic Installition' steps in the INSTALL file, with one
exception: specify '--disable-disk-based' as an argument to configure (step
5). Please note that unlike previous versions, TinyMUSH 3.1 uses GDBM for
its memory-based database instead of a flatfile. You can still create a
flatfile using the Backup script the same way you would for a disk-based
game.

### Help! I'm having database problems.

Almost everyone who runs a MUSH experiences a database disaster at
least once in their life. There can be many reasons for this; common
ones include an NFS server dying, running out of disk space,
overflowing your disk quota, a nasty game crash, and a nasty system
crash.

Typically, in this scenario, the server process either dumps core during
an attempt to load the database, or simply hangs; sometimes, it may come
back up and large chunks of your data may be missing.

Sometimes, simply by flatfiling and reloading the flatfile (doing a
Backup and a Restore), integrity issues that cause the database to
fail to load can be dealt with. Be aware that you might experience some
minor data loss in the process, though.

However, your best bet in this scenario is to restore your database
from backup. This means it's very important to keep backups; we strongly
recommend that you back up once a week at minimum, if not nightly.

If you don't have a recent backup you can restore from, you can try a
database reconstruction. This is likely to be most effective if you
have corruption resulting from a host machine crash or disk problem;
you might not be able to get back all of your data, but it'll at least
be something. If you haven't moved any files around, you can just go
into the game directory and type './Reconstruct'. If it completes
successfully, you can try to start it up with './Startmush' (though
doing a './Backup' first would be a good idea). If it doesn't complete
successfully, you'll need to revert to a backup.

### How do I set up a link to an external SQL database?

There is built-in support in TinyMUSH 3 for connecting to an external
mSQL or MySQL database. If you want to do this, you'll need to compile
in the appropriate file (see ./configure --help for how to do this).

You can also look at db_sql.h, db_msql.c, and db_mysql.c for examples
of how to write your own method of connecting to another type of
external SQL database. If you do this, please let us know, and provide
us with a copy of the code, so it can be included with the next
TinyMUSH release.

The rest of this answer uses mSQL as a specific example; MySQL and other
database systems have similar requirements.

You'll need to have an mSQL 2.x database set up. mSQL can be obtained
from http://www.hughes.com.au/ together with the appropriate documentation.
We do not support the mSQL package itself; if you need help with mSQL
installation, configuration, and maintenance, please refer to the
mSQL pages for assistance.

If the mSQL database is on the same machine as your MUSH, the sql_host
parameter in your netmush.conf file (or equivalent) should be set to
"localhost", so the local Unix domain socket is used; if the database
is not on the same machine as your MUSH, the sql_host parameter should
be whatever the remote server's hostname is.

You then need to create a database in mSQL for the MUSH to use (or
you can, of course, use an existing database). You can do this with:

msqladmin create MUSH_DB

... which creates a database called MUSH_DB. You can then add the
parameter 'sql_database MUSH_DB' to your netmush.conf file.

Make sure that you set up your mSQL installation's msql.acl file
correctly and with the appropriate level of security. See mSQL's
own documentation for assistance with this.

The database link should come up automatically when the game is
first started. You should see logfile messages that look something
like this, after the 'Load complete.' note in the log:

990831.115339 TinyMUSH SQL/CONN : Connected to SQL server.
990831.115339 TinyMUSH SQL/CONN : SQL database selected.

You should then be able to use the SQL() function from within the MUSH
to make queries against this database. Note that users of this function
need to have the use_sql power.

Note for MySQL users: you may need to specify a username/password to
connect to the database; use the sql_username and sql_password config
parameters. These parameters have no effect for mSQL.

---
### If you have suggestions for more FAQs, please email us and let us know.
---
