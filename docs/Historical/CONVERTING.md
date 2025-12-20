
# <center>WARNING<br><br>THIS FILE IS OUTDATED AND WILL BE UPDATED IN THE FUTURE</center>

# Converting from TinyMUX 1.0

If you are converting your game from TinyMUX 1.x, upgrading to TinyMUSH
3.1 is just like doing a MUX version upgrade, with some additional
steps. You will need to do the following:

- Flatfile your database using the dbconvert utility from TinyMUX
  (the db_unload script will work -- this is just like doing a normal
  backup of your database).

- Copy that flatfile to some safe place, and save it.

- Edit mush.config and make sure it's correct (it should basically be
  like your old mux.config file).

- If the GAMENAME parameter in your mush.config file is not 'netmush',
  edit your GAMENAME.conf file (substitute whatever you set GAMENAME to
  for GAMENAME).

- Compare your GAMENAME.conf file to the default netmush.conf file.
  You'll note that there are some new parameters and some changes to
  old defaults. Update your conf file to reflect these changes. Two
  things that you'll definitely want to add are:

	helpfile news text/news
	raw_helpfile man text/mushman

  - If you have the following parameters set, simply remove them:
    cache_depth
    cache_width
    input_database
    output_database
    mail_database
    comsys_database
    have_comsys
    have_mailer
    format_exits
    format_contents

  - If you were using the built-in comsys, add this line:
    module comsys

  - If you were using the built-in mailer, add this line:
    module mail

  - If you had your database files in the data directory, add this line:
    database_home data
    (If you have your database files in another directory, substitute that
    in instead. Pathnames are relative to the ~/3.1/game directory.)

  - If you have the gdbm_database parameter in your conf file, edit it
    to take away any prepending path. For instance, if your conf file reads:
    gdbm_database data/netmush.gdbm
    change it to read:
    gdbm_database netmush.gdbm

  - If you have the crash_database parameter in your conf file, edit it
    to take away any prepending path. For instance, if your conf file reads:
    crash_database data/netmush.db.CRASH
    change it to read:
    crash_database netmush.db.CRASH

  You should also look at the "Compatibility Notes" section below, for
  any other configuration parameters you'll need to add or change.

- Assuming that your flatfile is in the 'game' directory, go into the
  'game' directory ('cd game').

  If you have perl installed on your system, type:

      ./cvtmux.pl < mygame.flat > NEW.flatfile

  If you do not have perl installed on your system, type:

      sed -f cvtmux.sed mygame.flat > NEW.flatfile 

  (Substitute the name of your flatfile for 'mygame.flat'.)
  This will take care of converting function calls to backwards-compatible
  versions, and the '%c' substitution to the '%x' substitution.

- From the game directory, type:  ./Restore NEW.flatfile
  This will unflatfile (load) your database into TinyMUSH 3.1 format.

- From the game directory, type:  ./Backup
  This will back up your datbase in TinyMUSH 3.1 format.
  (This step is not necessary and can be skipped, but it's worthwhile
  doing anyway.)

- Convert your comsys database. You must have perl installed on your
  system. From the game directory, type:

      cp comsys.db comsys.db.save
      ./convert_comsys.pl < comsys.db.save > mod_comsys.db

  Make sure to keep comsys.db.save someplace safe, in case something goes
  wrong in the conversion process.

  If you do not have perl installed on your system, you will not be able
  to import your comsys database. (You could give a copy of the database,
  and the script, to a friend with perl on his system, and have him do
  it for you, though.)  If you can't find a way to convert your comsys
  database, type:  mv comsys.db comsys.db.save

- Rename your mail database to the new 3.1 name:
  mv mail.db mod_mail.db  

- From the game directory, type:  ./Startmush
  This will start the game.

-----------------------------------------------------------------------------

Compatibility Notes


You will want the following parameters in your conf file, for maximum
TinyMUX compatibility:

	global_aconn_uselocks no
	lattr_default_oldstyle no
	addcommands_match_blindly yes
	addcommands_obey_stop no
	helpfile +help text/plushelp
	helpfile wiznews text/wiznews
	access wiznews wizard

Also, the default netmush.conf file has 'raw_helpfile news text/news' in
it. If you want the old MUX behavior of 'news', where percent-substitutions
and ANSI are evaluated in the news file, you will need to change that
line to 'helpfile news text/news' instead.

Note that unless you explicitly need the legacy behavior of @addcommand'd
commands (you want no Huh? message generated if an @addcommand is missed,
and you match multiple @addcommands at once), you should probably set
the last parameters to the opposite of their MUX-compatibility defaults.

-----------------------------------------------------------------------------

Altered Behavior


The following are important changes in behavior between MUX and 3.0:

- MUX used '%c' for ANSI color. 3.0 uses '%x' for ANSI color ('%c' is
  the last-command substitution, as it is in PennMUSH and TinyMUSH 2.2.)

- MUX ifelse() conditioned on a non-zero result. 3.0 ifelse() conditions
  on a boolean-true result. To get the old behavior, use nonzero().

- pmatch() now works as documented (i.e., identical to its PennMUSH
  predecessor). To get the old behavior, use pfind().

- Objects are now checked for commands if they are set COMMANDS (instead
  of not being checked for commands if they are set NO_COMMAND).

- ZMOs are now controlled by their ControlLock, not their EnterLock.
  Only objects that are set CONTROL_OK may be controlled according to
  their zone's ControlLock.

- The COMPRESS flag has been eliminated. It was unused in MUX, though
  you could manually set it on objects.

- The WATCHER flag is now used to watch logins, rather than MONITOR.
  The power to do this is now called Watch_Logsin, and it allows
  you to set WATCHER on objects that you own (rather than on objects
  that you control).

- There is no longer a /dbref switch for @decompile; just use
  '@decompile <object>=<dbref of object>' to get the same effect.

Your existing database was converted automatically to take the above
differences into account.

The comsystem has been re-implemented. Some significant differences
include:

- If you are starting a game from scratch, and thus have no comsys.db,
  on startup, the Public and Guests channels are created by default.

- The channel administration commands have been consolidated to a
  single command, @channel, which takes a variety of switches. Channel
  objects are no longer necessary; data from them is automatically
  imported when you load a comsys database of the format generated
  by the convert_comsys.pl script.

- Multiple aliases for a single channel work in a much saner manner.
  Each alias can be associated with a different comtitle.

Other changes to be aware of:

- Two additional categories have been added to the stats() function. If
  you have softcode that extracts the number of garbage objects in the
  database from stats(), it will be fine if you used last(stats()) to
  get the data; if you used extract() or another function that depended
  upon list position, you will need to modify your code.

- Though MUX's help files documented the behavior of teleport locks, the
  implementation didn't reflect the help. The implementation now works
  the way help says it does.

-----------------------------------------------------------------------------

The following is a list of features/functions that were in TinyMUSH 2.2.5,
but were not in TinyMUX 1.6, and which have become part of 3.0.

For a more general look at TinyMUSH 3.0 changes, see the CHANGES file.

- Quotas by object type supported.

- Added building_limit conf parameter.

- Added player_parent, room_parent, exit_parent, thing_parent conf
  parameters for default parent objects.

- Added log options keyboard_commands and suspect_commands.

- Decent password choices are enforced via safer_passwords.

- Guest can be locked out from sites via the guest_site parameter.

- SIGUSR1 is logged, and the game restarts as GOD, not #1.

- Note is written to log after @startup queue is run.

- Disconnect log includes command count, bytes input, and bytes output.

- New COMMANDS flag, the reverse of the NO_COMMAND flag, which has been
  eliminated.

- New STOP flag, halts matching if a command is matched on a STOP object.
  (Wizard-only.)

- NOSPOOF is only wiz-visible.

- The color substitution is now '%x' rather than '%c'.

- The '%c' substitution returns the last command.

- Added "lag checking".

- Added timechecking.

- @daily is now implemented using the @cron facility.

- @dump/optimize no longer exists, since the database is optimized after
  every dump.

- @doing truncates the minimum possible (i.e., don't truncate for the ANSI
  character unless necessary).

- Added @dolist/notify.

- @enable/@disable now appear in the logfile, like config updates do.

- Added @eval command.

- examine/brief now shows everything but the attributes on the object.

- INFO command implemented.

- Added '@list cache'.

- Hash statistics listing shows stats for @function hash table.

- @mvattr no longer clears the original attribute unless it was able
  to copy it at least once.

- @pemit/noeval permits unparsed output.

- @pemit/list now obeys the /contents switch.

- There is a space after the @program prompt (2.2 behavior).

- Added object count to @search.

- @stats shows the next dbref to be created.

- The #$ @switch/switch() token has been added.

- Added @conformat and @exitformat contents and exits formatting.

- Command matches can be done unparsed through no_eval attribute flag.

- New BOUNCE flag acts like equivalent of @listen of '*' without @ahear.

- Added bor(), band(), bnand() functions.

- die() checks its arguments 2.2-style (so 0-100 is valid).

- Added filterbool() function.

- ifelse() now conditions on a boolean, rather than on a non-zero number.

- Added nonzero() function, which behaves like ifelse() used to.

- lastcreate() and the NewObjs attribute keep track of last-created
  objects for a given thing.

- Added lpos() function.

- matchall() returns a null string, not 0, on no match.

- mix() can take up to ten arguments total.

- objeval() now preserves the cause (enactor) in the evaluation, and
  allows the calling object to succeed if its owner is the same as
  the evaluator's owner, instead of needing to _be_ the evaluator's
  owner (2.2 behavior).

- Added programmer() function.

- Added remit() function, like pemit() but does contents.

- restarts() and restarttime() added.

- Added sees() function.

- shl(), shr(), inc(), dec() no longer require inputs to be numbers
  (2.2 behavior).

- Added t() function.

- Added toss() function.

- Vector functions replaced by 2.2 ones, including addition of vdot().
  This means that vector multiplication now works, but it breaks the
  old MUX behavior of vmul().

- Added wipe() function.

- Added xcon() function.

- elements(), filter(), map(), munge(), setunion(), setinter(), setdiff(),
  shuffle(), sort(), sortby(), splice() take output delimiters.

# Converting from TinyMUSH 2.2

If you are converting your game from TinyMUSH 2.2, upgrading to TinyMUSH
3.1 is similar to doing a normal upgrade, with a couple of additional
steps. You will need to do the following:

- Flatfile your database using the dbconvert utility from TinyMUSH 2.2
  (the db_unload script will work -- this is just like doing a normal
  backup of your database).

- Copy that flatfile to some safe place, and save it.

- The file directory structure has been altered. All game files now
  go into the 'game' directory. You'll want to move your database
  files into the 'data' directory, and your text files into the
  'text' directory.

- Edit mush.config and make sure that it's correct; it's basically like
  the old mush.config file from TinyMUSH 2.2, with some additional things.

- If the GAMENAME parameter in your mush.config file is not 'netmush',
  edit your GAMENAME.conf file (substitute whatever you set GAMENAME to
  for GAMENAME). 

- Compare your GAMENAME.conf file to the default netmush.conf file.
  You'll note that there are some new parameters and some changes to
  old defaults. Update your conf file to reflect these changes. The
  important ones are:

	raw_helpfile news text/news
	raw_helpfile man text/mushman
	database_home data

  If you have them set, delete the input_database and output_database
  parameters.

  If you would like to use in the built-in comsys and mailer, add:

	module comsys
	module mail

  You should also look at the "Compatibility Notes" section below, for
  any other configuration parameters you'll need to add or change.

- Assuming that your flatfile is in the 'game' directory, go into the
  'game' directory ('cd game').

  If you have perl installed on your system, type:

      ./cvt22.pl < mygame.flat > NEW.flatfile

  If you do not have perl installed on your system, type:

      sed -f cvt22.sed mygame.flat > NEW.flatfile 

  (Substitute the name of your flatfile for 'mygame.flat'.)
  This will take care of converting function calls to backwards-compatible
  versions.

- From the game directory, type:  ./Restore NEW.flatfile
  This will unflatfile (load) your database into TinyMUSH 3.1 format.

- From the game directory, type:  ./Backup
  This will back up your datbase in TinyMUSH 3.1 format.
  (This step is not necessary and can be skipped, but it's worthwhile
  doing anyway.)

- From the game directory, type:  ./Startmush
  This will start the game.

-----------------------------------------------------------------------------

Compatibility Notes


You will want the following parameters in your conf file, for maximum
2.2 compatibility:

	global_aconn_uselocks yes
	lattr_default_oldstyle yes
	addcommands_match_blindly no
	addcommands_obey_stop yes


Virtually everything can be tweaked, disabled, or otherwise altered to
fit whatever behavior you happen to want the server to have. All it takes
is some modifications to your conf file (netmush.conf, or whatever you
called it).

- I don't want to use the built-in mailer.
  Just leave out the 'module mail' parameter from your conf file.

- I don't want to use the built-in comsys (channel/chat system).
  Just leave out the 'module comsys' parameter from your conf file.

- I don't want to allow zone control with ZMOs.
  Put 'have_zones no' in your conf file.

- I don't want to allow powers to be set on objects.
  Put 'access @power disabled' in your conf file.

- I don't like the last-paged feature.
  Put 'page_requires_equals yes' in your conf file. This will enforce the
  use of an equals sign to page someone again, i.e., 'page =<message>'
  is needed, not just 'page <message>'. This should prevent virtually
  all last-paged-related mishaps.

- I don't want to allow @conformat and/or @exitformat to be used.
  Put 'attr_access ConFormat disabled' and/or 'attr_access ExitFormat disabled'
  in your conf file.

- I don't want side-effect functions.
  Put the following in your conf file:
	function_access command disabled
	function_access create disabled
	function_access force disabled
	function_access link disabled
	function_access set disabled
	function_access tel disabled
	function_access trigger disabled
	function_access wait disabled
	function_access wipe disabled
  If you want to be really draconian, also add to the conf file:
	function_access pemit disabled
	function_access remit disabled

-----------------------------------------------------------------------------

Altered Behavior


The following are important changes in behavior between 2.2 and 3.0:

- objmem() behaves differently, taking into account the object structure
  as well as attribute text. To get the equivalent behavior to 2.2's
  objmem(<obj>), use objmem(<obj>/*)

- hasattr() no longer checks for the presence of the attribute on the
  object's parent chain. To get the equivalent of the 2.2 behavior,
  use hasattrp() instead. Your existing database was automatically
  converted to take this difference into account.

- Two additional categories have been added to the stats() function. If
  you have softcode that extracts the number of garbage objects in the
  database from stats(), it will be fine if you used last(stats()) to
  get the data; if you used extract() or another function that depended
  upon list position, you will need to modify your code.

-----------------------------------------------------------------------------

The following is a list of features/functions that were in TinyMUX 1.6,
but were not in TinyMUSH 2.2.5, and which have become part of 3.0.

For a more general look at TinyMUSH 3.0 changes, see the CHANGES file.

- The TinyMUX powers system has been added.

- Added the MUX mailer. (Config option, on by default.)

- Added the MUX com system. (Config option, on by default.)

- The Guest system has been replaced by the MUX Guest system.

- DNS lookups are now done by a slave process (automatically started at
  startup, or through @startslave); the server will no longer block for
  them.

- Support for identd.

- Max login record kept and displayed in WHO.

- Added indent_desc config parameter.

- Buffer size is now 8000 characters (doubled).

- Output block size is now a compile-time parameter.

- Auto-restart on crash controlled by signal_action conf parameter.

- Attribute caching has been eliminated.

- Memory-based databases supported.

- Wizards can connect dark via the 'cd' command.

- Added '@cpattr' command.

- @daily implemented; enabled by 'eventchecking' global parameter, time
  set by events_daily_hour.

- Added @dbclean command, which removes stale attribute names from the
  database.

- Destruction of all objects is delayed until a dbck is done, as was
  previously true just for rooms. (Exception: DESTROY_OK objects.)

- @edit hilites the changed text.

- examine/brief now shows everything but the attributes on the object.
  To get the old behavior (just showing the owner), use examine/owner.

- '@function/preserve' preserves registers across user-defined functions.

- '@list process' shows the number of descriptors available.

- Multi-page and last-paged are now supported.

- @search supports a GARBAGE class.

- Added 'think' command.

- Semaphore @wait can now block on a user-specified attribute.

- Added @tofail/@otofail/@atofail (messages for failing to teleport out).

- Added the ROYALTY flag.

- Added SpeechLock/AUDITORIUM support.

- Added FIXED flag (prevents teleporting or going home), with conf 
  parameters fixed_home_message and fixed_tel_message.

- The GAGGED flag is back.

- Added marker flags: HEAD, STAFF, UNINSPECTED.

- TRANSPARENT flag on room shows exits in "long" style.

- Added VACATION flag, which auto-resets when a player connects.

- Added WATCHER flag, and Watch_Logins power, which is equivalent to the
  connection monitoring functionality of MUX's MONITOR flag. 

- hasattr() no longer finds attributes on parents; use the new hasattrp()
  function for the old hasattr() behavior.

- Added alphamax(), alphamin() functions.

- Added art() function.

- Added beep() function.

- Added case() function.

- Added children(), lparent() functions.

- Added columns() function.

- Added encrypt(), decrypt() functions.

- Added grep(), grepi() functions.

- Added hastype() function.

- Added playmem() function.

- Added pmatch() function.

- Added strcat() function.

- Added strtrunc() function.

- Note that side-effect functions that fail now generally return #-1
  NOT a null string (MUX behavior). If you want to guarantee a null
  return, wrap the call in a null().

