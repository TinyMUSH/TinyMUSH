# Table of Contents
- [TinyMUSH 3](\#tinymush-3)
  - [TinyMUSH 3.3](\#tinymush-33)
  - [TinyMUSH 3.2](\#tinymush-32)
  - [TinyMUSH 3.1](\#tinymush-31)
  - [TinyMUSH 3.0](\#tinymush-30)
- [TinyMUX 1](\#tinymux-1)
  - [TinyMUX 1.6](\#tinymux-16)
  - [TinyMUX 1.5](\#tinymux-15)
  - [TinyMUX 1.4](\#tinymux-14)
  - [TinyMUX 1.3](\#tinymux-13)
  - [TinyMUX 1.2](\#tinymux-12)
  - [TinyMUX 1.1](\#tinymux-11)
  - [TinyMUX 1.0](\#tinymux-10)
- [TinyMUSH 2](\#tinymush-2)
  - [TinyMUSH 2.2](\#tinymush-22)
  - [TinyMUSH 2.0](\#tinymush-20)


# TinyMUSH 3
## TinyMUSH 3.3

TinyMUSH 3.3 is currently in dead. See Git's log for changes.

## TinyMUSH 3.2

TinyMUSH 3.2 beta 1 was released on February 23rd, 2009.

The "full", gamma release of TinyMUSH 3.2 is dated June 6th, 2011.

## TinyMUSH 3.1

TinyMUSH 3.1 beta 1 was released on May 8th, 2002.

The "full", gamma release of TinyMUSH 3.1 is dated June 21st, 2004.

### 3.1 Patchlevel 6 -- January 12th, 2009

  - Enhancement: A "reply" command allows you to reply to a page, including being able to reply to the whole group of people paged along with you.

  - Enhancement: Property directories (propdirs) offer the ability to inherit attributes from multiple objects. If an object's parent chain does not contain an attribute, the object's @propdir list is searched for that attribute.

  - A new special @hook, internalgoto, has been added. This offers the ability to execute a hook on any type of movement, regardless of the reason, with the before-hook executed before leaving a location, and the after-hook executed when entering a location.

  - Enhancement: The new /private switch to @function makes the register environment of the called function private, ala uprivate() -- i.e., no registers are passed in, and any changes made to registers by the function are discarded.

  - Enhancement: The new /private switch to @hook makes the register environment of the called hook private.

  - Enhancement: PennMUSH-style #lamba anonymous functions are supported, allowing functions like map() to take a string to execute rather than requiring it to reside on an attribute.

  - Enhancement: The iter() family has been extended -- iter2(), list2(), whentrue2(), whenfalse2(), itext2() -- to allow a second list to be simultaneously processed via the #+ string-token substitution.

  - Enhancement: New function ifzero(), the reverse of nonzero().

  - Enhancement: New function lregs() lists non-empty registers.

  - Enhancement: If speak()'s first argument (the speaker) contains an '&', it's used to specify a speaker name.

  - Enhancement: @adisconnect is now passed all the statistics of the session info, via the stack.

  - Bugfix: In the ifelse() family, trailing space is no longer accidentally eaten.

  - Maintenance: Updated copyright to 2009.

### 3.1 Patchlevel 5 -- January 27th, 2008

  - Enhancement: The new conf parameter local_master_parents allows the parents of objects in ZONE-set parent rooms to be scanned for $commands.

  - Enhancement: New Trace attribute flag, allows tracing of just a single function.

  - Enhancement: IDLE psuedo-command for idle keepalive, ala PennMUSH.

  - Enhancement: @colormap allows a player to remap what colors he sees.

  - Enhancement: @end (@break / @assert) ala PennMUSH/MUX -- terminate action list and execute default based on a condition.

  - Enhancement: New functions private() and uprivate(), which sandbox the global registers for an evaluation.

  - Enhancement: New function lreplace(), a multi-element replace.

  - Enhancement: New functions usetrue() and usefalse() use conditional expression if true/false, and default otherwise.

  - Enhancement: New functions iftrue() and iffalse() act like ifelse() and its boolean inverse, but substitute the condition in the resulting evaluations as #$ (the switch token).

  - Enhancement: New function qvars() reads list elements into local registers, like xvars() does with variables.

  - Enhancement: New function isort() sorts a list and returns its indices.

  - Enhancement: extract() can take an output delimiter.

  - Enhancement: ports() can take an empty argument, returning all ports; permissions made configurable rather than hardcoded.

  - Enhancement: push() with an empty argument now pushes an empty string onto the stack, rather than returning an error.

  - Enhancement: lor(), etc. return 0 on an empty argument, rather than an error.

  - Enhancement: @@() can have embedded commas.

  - Enhancement: %i0 through %i9 are equivalent to itext(sub(ilev(),<number>)) and are identical to their PennMUSH counterparts, enabling the common case of wanting to get the substitution from N levels up. %i-0 through %i-9 are equivalent to itext(<number>).

  - Enhancement: PennMUSH-style %+ substitution (returns number of arguments on the stack).

  - Enhancement: New conf parameter mud_shortname is used in logging, rather than mud_name, if it's set. Useful for shortening very long game names.

  - Bugfix: lunion() etc. correctly detect sort type when first argument is blank. [3.1a7]

  - Bugfix: writable() works on attributes that didn't previously  exist. [3.1a8]

  - Bugfix: Don't leak memory on speak(). [3.1b3]

  - Bugfix: Don't match an empty string to an exit (i.e., no foo;;bar). [2.0]

  - Bugfix: @femit/spoof gives output properly. [3.1p2]

  - Bugfix: Players who are hidden (Dark) can now have their names partial-matched for page, etc. by players who can see hidden. [MUX]

  - Bugfix: All site flags must be kept and checked (so, for instance, you can restrict both creates and guests). [3.0b8]

  - Bugfix: Global registers must be cleared when doing notification. [2.0]

  - Bugfix: Bad @cron syntax can no longer cause a loop. [3.0b8]

  - Bugfix: Message to/use of an emptied comsys channel can no longer cause a crash. [3.0b14]

  - Bugfix: flag_access must have a permission. [2.0; Brazil]

  - Maintenance: Updated GDBM's AutoConf to version 2.59.

  - Maintenance: Updated GDBM's LibTools to version 1.5.22.

  - Maintenance: Removed unused variables related to @program. Per Brazil.

  - Maintenance: Updated copyright to 2008.

  - Compatibility: Enables TinyMUSH to read the escapes of cr/lf add to the TinyMUX flatfile. Per Brazil.

### 3.1 Patchlevel 4 -- December 19th, 2006

  - Fixed Always clear @prog data on logout

  - Fixed tools (src/tools). Add an install tag to the tools makefile.

  - Update LibTools to version 1.5.22.

  - Update AutoConf to version 2.59.

  - Fixed function logf() in udb_* conflict with build-in function logf(), rename the function to warning().

  - Remove unused port-concentrator code.

  - Automated build of TinyGDBM within ./configure.

  - Updated the build process a bit to keep in line with autoconf standards.

### 3.1 Patchlevel 3

  - Fixed Update script to work with Sourceforge's new web services.

  - Bugfix for integer overflow in PCRE repeat counts.

  - Removed stale code in add_player_name.

  - Refactored the logged-out commands code, removing some unreachable code in do_command, clarifying how logged-out commands are handled, and fixing a reference to freed memory.

  - Improved performance of scramble() and reverse() on strings with less ANSI color.

  - Avoided unnecessary attribute scans in Hearer().

  - Removed duplicated code in handle_ears().

  - Minor nitpicks in sweep_check() and fh_hear_bit().

  - Renamed CLONE_PARENT to avoid conflict with Linux 2.4 sched.h. Patch from Brazil. [2.0]

  - Fixed potential null-dereference in player cache -- pcache_find() no longer returns null and all callers must use Good_owner() first. Reported by Brazil. [2.0]

  - Fixed double-evaluation of arguments in columns(). [MUX; Brazil]

  - Fixed lack of evaluation of delimiter arguments in list() and loop(). [3.0]

### 3.1 Patchlevel 2

  - Startmush now accept start/stop/restart/reboot switches. You can also pass the name of the config file you want to use instead of the default 'mush.config' one.

  - Removed hardcoded binaries path from the engine, a new command line switch ( -b binpath ) new feed the engine with the correct value. The engine can now run its binaries from any directory, not just ./bin/. (slave path was hardcoded.)

  - Removed hardcoded textfiles path from the engine, a new command line switch ( -t textpath ) now feed the engine with the correct value. This eliminate the need of adding every textfile in the configfile if someone want to change the game's text directory. 

  - The engine is now able to find the data directory and names of the db/crash files from the command line. ( -d datapath,  -g gdbmfile and -k crashfile) The configuration parameters  'database_home',  'crash_database' and 'gdbm_database' are still avalable for backward compatibility.

  - A new helper (logwatch) replace the 'tail'/'awk' script used to print the first lines of the log during statup. this solve the problem of having a zombie tail command left running when awk cut its pipe.

  - Added EXIT, ROOMS and VEHICLE topics in qhelp.txt. 

  - Fixed a memory leak in sql_query function where result was stored but never freed so whenever a query returns an empty result it leaks a mysql result structure. [3.0; Dan Ryan]

  - Added /spoof switch for @fsay/@fpose/@femit.

  - Modified the documentation to properly reference @speechformat instead of @speechmod [3.1b7; Jared Robertson]

  - Changed the wizhelp topic text of LOGGING and INHERITANCE to "This topic hasn't been written yet." [MUX; Jared Robertson]

  - Added a REGISTRATION wizhelp topic (was referenced by @PCREATE and REGISTER_SITE) [MUX; Jared Robertson]

### 3.1 Patchlevel 1 -- August 11th, 2004

  - Fixed a bug in dbconvert where the cache wasn't flushed before closing the database, creating an incomplete database. [3.1p0]

####  CHANGES TO THE BASE SERVER

- The licensing scheme has been changed to the Artistic License.

- A new script, Update, automatically downloads and applies official patches to TinyMUSH 3.

- The Backup, Restore, and Archive scripts can now look at a backups directory (set in mush.config). The default compression program is also now set in mush.config. Compression extensions are used correctly, and the bzip2 format is supported.

- The Startmush script handles KILLED and CRASH databases more elegantly.

- The Restore script can handle the archives produced by Archive.

- Dynamically loadable modules. There is now an interface for writing your own hardcode extensions to the server, and loading them at runtime. The comsys and mailer are now loadable modules; the USE_COMSYS and  USE_MAIL #ifdef's and the have_comsys and have_mailer conf directives are no longer needed.

- '@list memory' gives a breakdown of the memory used by various constructs.

- Many changes and improvements to caching.

  - Instead of starting the cache at some fixed width and depth, and growing the depth (but not the width) to whatever size is demanded by the game, which can cause "cache bloat" if a very large number of objects gets loaded into the cache as part of a single operation, the total size of the cache is fixed -- the amount of memory that the cached data takes cannot exceed a specified limit. This is configurable via the cache_size parameter.

  - Removal of elements from the cache is based on "least recently used" rather than "not frequently used".

  - Attribute caching is now used, with cache bypass in situations where this is not efficient.

  - @list cache shows an object-based summary of cache information. @list cache_attrs shows the full cache.

- Various changes to the database structure:

  - GDBM databases now contains meta-data that allow easier recovery of databases that have been trashed (due to a host machine crash or running out of disk space, for instance). A new script, Reconstruct, calls the new recover binary to attempt to rebuild damaged databases.

  - There are no longer separate structure and GDBM files ("numeric" and "string" databases).

  - GDBM's native free space management has been enabled. Optimized dumps have been disabled by default. Our copy of gdbm includes a bugfix that enables the free space management and other options, so we build it as libtinygdbm.a to avoid linking netmush with an unmodified gdbm.

  - File locking has been added for some circumstances, so dbconvert waits for netmush to finish writing, before it starts reading, ensuring consistency when backing up a running MUSH.

- dbconvert is now just a symlink to netmush; its functionality has been merged into the main program.

- Making a flatfile now automatically cleans the attribute table of dead attribute name references, so the @dbclean command is no longer needed. (You can pass a -q option to dbconvert to prevent attribute cleaning, if for some reason you want to avoid this.)

- Clean flatfiles are now produced by SIGUSR2. SIGQUIT causes a graceful shutdown.

- Extensive work on improving autoconf and libtool use (thus improving portability). Compile-time options are now specified as switches to the configure script.

- Garbage is trimmed from the top of the database, if possible, reducing bloat due to tail-end garbage.

- When an object name is ambiguously matched (i.e., potentially matches several objects), the match is now truly chosen randomly; in the past, the last match was heavily favored.

- Help entries can now have multiple index terms.

- A rewrite has begun on the help files.

#### CHANGES TO CONFIGURATION PARAMETERS

- Conf parameters that are dbrefs (such as master_room) are now sanity checked, both when they are set and when a @dbck is run. Also, a dbref as well as a number can be used, i.e. the syntax 'master_room #2' is now also valid (whereas 'master_room 2' was previously required).

- A new conf parameter, attr_type, allows attributes whose names match certain wildcard patterns to be given certain default flags. Attribute names can now start with '_'. (To implement a rule like, "All attributes whose names start with '_' can only be set by wizards," you would add, "attr_type _* wizard" to your conf file.)

- A new conf parameter, database_home, has been introduced. This is the directory where all the database files are stored. This eliminates the necessity to provide a directory path for input_database, etc.

- New conf parameters exit_proto, room_proto, etc. allow one object of each type to be defined as the "prototype" for new objects of that type. When an object of that type is created, it copies the flags, parent, zone (if 'autozone' is off), and attributes of the prototype; the effect is similar to cloning.

- New conf parameters exit_attr_defaults, room_attr_defaults, etc. allow one object of each type to be defined as the repository for "attribute defaults" for many built-in attributes, such as attr/oattr pairs (@desc, @odesc, @succ, @osucc, etc.), @conformat, and @exitformat, as well as user attributes that have a global attribute flag of 'default'. These defaults override the attributes set on objects of that type (the local value of the attribute is passed to it as %0, for evaluation). This allows the global definition of formats for those attributes, without requiring a global parenting scheme or an unusual attribute naming scheme. Objects with the FREE flag ignore attribute defaults.

- The new conf parameter, c_is_command, controls whether %c is last command (default) or ANSI substitution.

- The new conf parameter forwardlist_limit allows you to set a limit for the number of objects that can be listed in a single @forwardlist.

- The new conf parameter function_cpu_limit allows you to set a limit for the number of seconds of CPU time that can be taken up by function evaluations associated with a command.

- The new conf parameters guest_basename, guest_prefixes, guest_suffixes, and guest_password allow flexible naming of guests. See the FAQ for details. (The guest_prefix parameter is now obsolete, and has been removed.)

- The new conf parameter huh_message lets you change the Huh? message that is displayed when a command cannot be matched.

- The new conf parameter power_alias allows you to set aliases for powers. The compat.conf file now contains aliases for mapping PennMUSH and TinyMUX power names.

- The new conf parameter visible_wizards (disabled by default) results in DARK wizards being hidden from WHO (and related things), but not being invisible and silent. This prevents accidental or deliberate spying by DARK wizards.

- The new conf parameter wildcard_match_limit restricts how extensive of a recursive search is performed when attempting to match a pattern.

#### CHANGES TO ATTRIBUTES

- @aconnect and @adisconnect attributes are triggered consistently on all connections and disconnections, with a reason in %0 and the player's resulting number of open connections in %1.

- Players can now specify multiple aliases for themselves (up to the conf parameter player_aliases_limit), via @alias, separating aliases with ';'. Players no longer inherit @alias from their parent (this never made sense, or worked properly, anyway).

- @conformat and @exitformat get their visible contents/exits lists passed as %0.

- A new attribute, LastIP, tracks the last IP address of a player. When Lastsite is set, the identd user and hostname are no longer truncated.

- The new attribute @nameformat controls the display of location's name when someone looks at the location.

- The format and text of a say or pose can be arbitrarily altered, via the @speechformat attribute and SPEECHMOD flag on the speaker, or the speaker's location.

- General visibility of an attribute is now consistently controlled by the 'visual' attribute flag (previously it was controlled by 'private' on the global attribute definition, and 'visual' on the specific attribute). Attributes default to not visual; if you were previously defining global attributes with '!private' (via @attribute/access, user_attr_access, etc.) you should now define them as 'visual'. Databases are automatically converted to have the appropriate attribute flags.

- Visibility restrictions on attributes have been sanitized, so they are consistent across commands and functions.

- Attribute flags that have no meaning on a global basis can no longer be set globally via @attribute and the like, thus reducing confusion.

- The global flags for attributes are now shown in parentheses, when an object is examined.

- A new attribute flag, 'case', controls whether regular expression matching is case-sensitive, for $-commands, ^-listens, @filter, @infilter, and @listen.

- A new attribute flag, no_name, controls whether or not the actor's name is prepended when an attribute is used as an @o-attr, such as an @osucc or with @verb.

- @filter, @infilter, and @listen do regular expression matches if set 'regexp'. They do not evaluate themselves before matching, if set 'no_parse'.

- A new attribute flag, 'rmatch', assigns the results of a $-command's wildcard-match, to named local registers, as well as to the stack. Based on an idea from KiliaFae.

#### CHANGES TO FLAGS AND POWERS

- An object with the new attr_read power is able to see attributes with the 'hidden' attribute flag, just like Wizards and Royalty can.

- An object with the new attr_write power is able to set attributes with the 'wizard' attribute flag, just like Wizards can.

- An object with the new link_any_home power can set the home of any object (@link an object anywhere), just like Wizards can.

- If a wizard connects while DARK, they are reminded that they are currently set DARK. Also, wizards are notified when they are set DARK due to idle-wizards-being-set-dark. A player must have the Hide power or equivalent in order to auto-dark.

- There is a new type of lock, the DarkLock, which controls whether or not a player sees an object as DARK. Based loosely on a patch by Malcolm Campbell (Calum).

- There is a new power, Cloak, which is God-settable. It enables an object which can listen (such a player or puppet) to be dark in the room contents and movement messages; previously, this was only true for listening objects both Dark and Wizard. Note that since wizard Dark is now affected by the visible_wizards config parameter, if that parameter is enabled, wizards will need this power in order to become invisible.

- The Expanded_Who power no longer grants the ability to see Dark players in WHO. (Previously those with the power could, but weren't able to see them in functions. Use the See_Hidden power to grant the ability to see Dark players.)

- The new ORPHAN flag, when set on an object, prevents the object from inheriting $-commands from its parents (just like objects in the master room do not inherit $-commands from their parents). This allows you to @parent a command object to a data object, without worrying about the overhead of scanning the data object for $-commands.

- The new PRESENCE flag, in conjunction with six new locks (HeardLock, HearsLock, KnownLock, KnowsLock, MovedLock, MovesLock) supports "altered reality" realm states such as invisibility.

#### CHANGES TO COMMANDS

- Some commands that would normally be queued can now be executed immediately, using the /now switch: @dolist/now (@iter), @force/now, @switch/now (@branch), @trigger/now (@call), @verb/now force() and trigger() now behave like @force/now and @trigger/now. The new conf parameters command_invocation_limit and  command_recursion_limit are used to limit this behavior.

- A user-defined $-command is executed immediately if the attribute has the new Now attribute flag.

- Commands added via @addcommand are now run immediately. Also, these commands now respect an escaped-out ':', as well as the regexp and case attribute flags.

- A new switch, @addcommand/preserve, causes the text of an added command to be executed by the player, not by the specified object, unless the player is God (for security reasons).

- Custom comsys headers can now be specified for channels, via @channel/header. These headers can be listed via @clist/header. Also, the @channel command now interprets its arguments, and channel descriptions are passed through a sanity check.

- The new Spoof comsys flag, when set on a channel, replaces player names with their comtitles, instead of prepending the comtitle to the player name.

- @cpattr can copy multiple attributes at a time.

- Exits can now @destroy themselves.

- @doing (including @doing/header) now takes a /quiet switch.

- A new command, @floaters, shows floating rooms. Floating rooms are no longer reported by @dbck, and thus the FLOATING flag has been eliminated (since it's no longer needed to suppress @dbck spam).

- The new /permit switch to @hook allows the permissions for a built-in command to be defined via a user-defined function.

- Functions defined via @function can be listed via @function/list.

- Function permissions can be listed via '@list func_permissions'.

- Lock syntax is now always sanity-checked. Unbalanced parentheses within a lock are no longer permitted.

- The new @log command can be used to log arbitrary messages to the logfile(s).

- When you log in, you are warned if you are currently in the midst of writing a @mail message.

- A @mail recipient now receives only a single copy of a @mail message, even if he appears multiple times in the list of players the message is to.

- @mail now has real carbon-copy and blind-carbon-copy. Patch from Simon Bland.

- @pemit/list now obeys the same permission checks as @pemit, including obeying pemit_any as well as restricting use of the /contents switch.

- The @emit, @oemit, @femit, @fpose, @fsay, page, pose, and say commands now take a /noeval switch. (Analogous to the /noeval switch for @pemit.)

- The @pemit and @oemit commands now take a /speech switch, which makes the message subject to Presence checks.

- The new @redirect command can be used to redirect Trace, Puppet, and Verbose output from an object, to a player other than the object's owner. The REDIR_OK flag allows an object's output to be redirected by someone who does not control it, thus allowing non-Wizards to participate in mutual debugging sessions. Based on an idea from AlloyMUSH.

- The new @reference command creates named references ("nrefs"), which behave like aliases for dbrefs, in the form of '#_<name>'.

- When speech is formatted, with the exception of say and @fsay, there is now a grammatically-correct comma before "says". The new conf parameter say_uses_comma will insert the comma for say and @fsay, as well.

- If the new conf parameter say_uses_you is turned off, the say and @fsay commands will always show '<name> says' rather than 'You say' to the speaker. This is more convenient for environments where activity is often logged and shared.

- A @dbck is automatically done before a @restart or normal @shutdown, so recycling cleanup is done first, thus avoiding inconsistencies upon startup.

- The @teleport command can now be used on exits. Players with the Open_Anywhere power can now drop or @teleport exits to any location.

- Specifying an object to @trigger is now optional; if no object is specified, the object defaults to 'me'.

- @verb now takes a no_name switch, which prevents the actor's name from being prepended to the default o-message.

- Attributes containing a $command or ^monitor can now be @trigger'd directly, as well as used as @a-actions; everything before the first un-escaped ':' is ignored.  Based on an idea from PennMUSH.

#### CHANGES TO FUNCTIONS

- Multi-character input and output delimiters are now supported for most functions.

- There are additional global registers, %qa-%qz (based on an idea from PennMUSH). Furthermore, arbitrarily-named global registers can be defined on the fly (up to the conf parameter register_limit, per action list). setq() can now take multiple name-value pairs.

- The caller of a function can now be tracked via %@; it's normally equal to the enactor, except where u()-type functions, including objeval() and @functions, are called. This provides better security-checking capabilities. Based on an idea from PennMUSH.

- The %m substitution can now be used for last command (MUX2 compatibility; %c is the usual substitution).

- Brackets are no longer required around eval-locks, hooks, or @exitto code.

- The *<player> syntax is supported in many more places.

- There is a more informative error message when a function is given the wrong number of arguments. Idea from Philip Mak.

- Extended regular expression syntax from Perl 5 is now supported. This is based on the PCRE library, as modified by the PennMUSH team.

- New regular expression functions: regedit(), regediti(), regeditall(), regeditalli(), regrab(), regrabi(), regraball(), regraballi(), regrep(), regrepi(), regmatchi(), regparsei().

- Added a variety of comsys-related functions: cemit(), comalias(), comdesc(), comheader(), cominfo(), comlist(), comowner(), comtitle(), cwhoall().

- cwho() now only returns what's listening.

- Added fcount(), fdepth(), ccount(), cdepth(), for getting function and command invocation and recursion counters. Based on a concept from Shadowrun Denver MUSH.

- The filter(), filterbool(), map(), and while() functions pass the position of the element in the list as %1. fold() passes the position of the element in the list as %2. foreach() passes the position of the character in the string (starting from position 0). Based on an idea from PennMUSH.

- The idle(), conn(), and doing() functions can now take a port number.

- The escape(), secure(), capstr(), lcstr(), and ucstr() functions now handle ANSI properly.

- Added border(), rborder(), and cborder(). These are word-wrap functions which can left/right/center-justify text within the field, and add left and right-margin borders.

- Added cand(), cor(), candbool(), and corbool(), similar to and() etc., except these functions stop evaluating their arguments once a terminating conclusion is reached. Based on an idea from PennMUSH.

- chomp() can now handle a lone CR or LF.

- Added choose(), weighted-random-choice pick of an element from a list. Idea from FiranMUX.

- Added connrecord(), connected player record (as in WHO).

- Added elockstr(), which evaluates a target against a lock string. Idea from Joel Ricketts.

- Added entrances() function, similar to @entrances command. Based on an idea from PennMUSH.

- You can now get attribute flags with flags(<obj>/<attr>). Based on an idea from PennMUSH.

- Added graball(), which is to grab() what matchall() is to match().

- Added group(), for splitting/sorting a list into groups.

- hasflag() can now detect object types.

- Added hasflags(), which can operate on multiple lists of flags and types, returning true if all elements in any of the lists are true.

- Added hears(), moves(), and knows() functions for checking Presence permissions.

- Added helptext(), which retrieves an enetry from an indexed textfile (i.e., help, news, or anything else added via the helpfile or raw_helpfile conf directives).

- The third argument to ifelse() -- the 'false' result -- is now optional.

- Added istrue() and isfalse(), which are basically iter()-style filterbool() and its reverse.

- Added ilev(), itext(), and inum() functions, for retrieving data from multiple levels of nested iter()-type functions. Based on an idea from PennMUSH.

- Added itemize() function, which formats a list of items. Typically used to turn a list of words into something like "a, b, and c". Based on an idea from PennMUSH.

- Last-accessed and last-modified timestamps are now tracked for all objects. The timestamps are visible on an 'examine', and the seconds-value can be retrieved via the new functions lastaccess() and lastmod().

- Added ledit() function, a mass find-and-replace on a list which can replace many instances of nested edit() calls.

- lpos() can now check multiple characters at one time. Based on a suggestion from Joel Ricketts.

- lpos(), lattr(), lexits(), lcon(), xcon(), children(), lparent(), and the grep() family now take an output delimiter.

- The ljust(), rjust(), and center() functions can take multi-character fills.

- The lunion(), linter(), and ldiff() functions are similar to setunion(), etc., but an attempt is made to autodetect the list type and sort accordingly, or you can specify a sort type, as you can with sort().

- The modify() function now takes lists, enabling multiple variables to be modified at once.

- The behavior of remainders in division with negative numbers is now consistent (rather than being compiler-dependent), including new modulo(), remainder(), and floordiv() functions. The mod() function is now an alias for remainder(), which is closest to the old mod() on most platforms; if you want it to be an alias for modulo() instead, change it in alias.conf

- munge() now passes its input delimiter to the u-function, as %1. Idea from PennMUSH.

- Added nattr(), which counts the number of attributes on an object.

- Added objcall(), which evalutes the text of an attribute from another object's perspective, like a combination of u() and objeval(). Idea from Joel Ricketts.

- Added oemit(), which is the function equivalent of @oemit.

- pos() strips ANSI out of the string to search for, in addition to the string to search within.

- rloc() now returns the destination of an exit.

- New search classes -- ueval, uthing, uplayer, uroom, uexit -- have been added, allowing a u-function to be called for evaluation. This makes it unnecessary to deal with brackets-escaping issues.

- sees() can now evaluate the visibility of exits as well as things. Also, a small bug has been fixed: the see_owned_dark calculation in sees() now uses the object doing the looking, not the player invoking the function.

- Added session(), which returns command count, bytes of input, and bytes of output associated with a port.

- Added speak(), which formats speech strings and can parse and transform them.

- Added store(), which is basically to setx() what setr() is to setq().

- The new structure functions read() and write() allow the use of an "invisible" delimiter, which permits data to be stored in delimited lists without worrying about user-provided data containing the delimiter character. The delimit() function processes a stored list, substituting a separator string for a delimiter.

- Added table(), tables(), rtables(), and ctables() functions. table() is based on an idea from PennMUSH, formatting text within a table of equally wide columns. The other three functions format text within a table of variably-wide columns (left/right/center-justified), with left and right-margin borders.

- Added timefmt() function. This is essentially an interface to the C library function strftime(), allowing the time to be formatted in psuedo-arbitrary ways. Based on an idea from PennMUSH.

- translate() now handles tabs.

- trim() now handles multi-character trim strings.

- Added sind(), cosd(), tand(), asind(), acosd(), atand() degree-based trigonometry functions.

- Added until() function, which is similar to while(), except it operates on multiple lists and terminates on a regular expression condition.

- valid() can now detect valid attribute names and player names, for PennMUSH compatibility.

- Added whentrue() and whenfalse() functions, which go through a list iter()-style until a terminating boolean condition is reached.

- Added wildgrep(), wildmatch(), and wildparse(), which are similar to regrep(), regmatch() and regparse(), but work against wildcard patterns rather than regexps.

- Added writable() function, which returns 1 if object X can set an attribute A on object Y.

- Added @@() function, which is like null(), but does not evaluate its argument.

- elock(), hasflag(), haspower(), and type() now all return #-1 NO MATCH when they can't find the object they're checking. Previously they returned #-1 NOT FOUND; for consistency's sake, they now behave like everything else.

#### OTHER CHANGES

- Improvement in logging facilities.

- Dynamic allocation of string conf parameters.

- Break-out of functions into many files, sorted by function type. Break-out of the command tables into a separate file, for ease of maintenance.

- ANSI, particularly in functions, is handled more consistently and more sanely. Some string and list functions have also been optimized. These changes are known to be incomplete as of TinyMUSH 3.1 beta 1.

- Similar functions may use the same handler internally, called with different flags specified in the function table, so adding families of similar functions does not imply code bloat. Several groups of functions (trigonometry, logic, etc.) have been condensed into common handlers.

- Better handling of the slave for DNS and identd lookups.

- Replaced the random number generator with Mersenne Twister. Use derived from PennMUSH, which derives it from MUX2.

- Other cleanup in many parts of the code, including performance enhancements and significant reductions in memory used.

- Fatal signals now produce cleaner coredumps, but two signals are required to generate a core file. Based on a patch from Amos Wetherbee.

- Many portability improvements, including support of UWIN, so TinyMUSH 3 can be run under Windows.

## TinyMUSH 3.0

This change list details user-visible or otherwise highly significant changes in TinyMUSH 3.0. The bugfixes and general code reworking done in the course of merging TinyMUSH 2.2 and TinyMUX line-by-line, as well as throughout an extensive cleanup, bugfix, and performance-enhancement effort, is too extensive and complex to readily be listed here.

Note that these are only changes that are completely new to both TinyMUSH 2.2 and TinyMUX; enhancements that were in one server but not the other have been listed in the conversion notes for both servers.

TinyMUSH 3.0 beta 1 was released on September 27th, 1999.

The "full", gamma release of TinyMUSH 3.0 is dated December 1st, 2000.

### CHANGES TO THE BASE SERVER AND SERVER CONFIGURATION

- Build script allows "single-step" configuration, build, and installation.

- Backup and Restore scripts simplify backup maintenance. Archive script allows the archival of all the important files.

- Index script detects help-style files (help, news, etc.) and indexes them. This is now called by Startmush.

- Bug reporting is easier, via the ReportCrash script, which analyzes core files and emails the results to the bug reporting address.

- GNU dbm 1.8.0 is required. Database code has been rewritten to take advantage of its features. Dumps are now automatically optimized (though you can turn this off using the new conf parameter opt_frequency), and you may safely back up the MUSH while it is running, either internally (through @dump/flatfile) or externally (through the Backup script).

- An interface to an external SQL database is now supported, through the SQL() function and supporting administrative commands. Currently, modules for mSQL and MySQL are available.

- There is now a generic indexed-textfile facility, allowing 'news'-like commands and their associated files to be specified via the 'helpfile' and 'raw_helpfile' parameters in the conf file.

- The readability of a configuration parameter can be set via the config_read_access param, and can be listed with @list config_read_perms. A config() function allows players to obtain the value of a configuration parameter that they have permission to read.

- The new game_log and divert_log parameters allow the logs for different types of events to be sent to different files. The new @logrotate command allows these logfiles to be rotated while the game is running. Logfiles are also rotated when a @restart is done. Old log are marked with a timestamp in seconds. The new Logclean script simplifies cleanup of old logfiles.

- Command-table additions (@addcommand and family) are supported (a cleaned-up version of the MUX implementation). Three new conf parameters:
  - addcommands_match_blindly (defaults to 'yes', controls whether or not a 'Huh?' is produced when a match on an @addcommand'd command doesn't actually match a $command)
  - addcommands_obey_stop (defaults to 'no') controls whether or not an object with the Stop flag actually enforces a stop of match attempts when a match is found on an @addcomand for that object.
  - addcommands_obey_uselocks (defaults to 'no') controls whether or not the uselock is checked when an added command is matched.

  (The defaults are MUX behavior; we suggest that the reverse of the defaults is more desirable, though.)

- There are now command "hooks", implemented via the @hook command. Hooks are functions which execute before and/or after a built-in command; using side-effect functions, it is thus possible to customize a command without needing to write a full-blown @addcommand for it, or to execute things before/after every move through an exit.

- The term "zone" is now used for two things: MUX-style control-oriented zones, and 2.2-style local-master-rooms. Both types of zones default to on (local_master_rooms and have_zones conf parameters). MUX-style zones now use ControlLock rather than EnterLock, and only objects set CONTROL_OK may be controlled by a ZMO; this provides slightly better security. A new config parameter, 'autozone', controls whether or not objects are automatically zoned to their creator's ZMO at the time of their creation.

- The comsystem has been rewritten, resulting in a variety of minor syntax changes and alterations and enhancements to functionality. Default channel aliases can now be set with the public_calias and guests_calias config parameters.

- Variable destination exits are implemented, via the "variable" keyword and the ExitTo attribute; the destination of the exit is determined when it is used. The link_variable power has been added in support of this. (This works in a way similar to PennMUSH's variable destination exits, but ExitTo was used instead of Destination, to reduce likelihood of previous attribute conflicts.)

- Optional uselock checking for global aconnects has been implemented. (2.2 had this by default; MUX did not have this.)

- The disconnect reason is passed for master-room disconnects, too.

- When the new conf parameter dark_actions is enabled, objects set Dark still trigger @a-actions when moving, unless the /quiet switch is specified.

- When the new conf parameter exit_calls_move is enabled, trying to go through an exit by just typing its name is equivalent to typing 'move <exit name>', allowing this to be intercepted by a move @addcommand.

- When the new conf parameter move_match_more is enabled, the move command matches exits like the main command parser does, i.e., it also checks global and zone exits, and in the case of multiple matches, picks a random match.

- When the new conf parameter no_ambiguous_match is enabled, ambiguous matches always result in a random selection amongst the matches (i.e., you will never get a "I don't know which one you mean!" message).

- The new conf parameter guest_starting_room allows Guest characters to start in a different default room than other characters.

- The MUSH manual is included in the distribution in helpfile format. (Thanks to Alierak and sTiLe.)

### CHANGES TO FLAGS AND POWERS

- There are now ten user-defined flags, MARKER0 through MARKER9. The flag names can be set through the flag_name option. Commands, functions, and other things with configurable permissions can also be tied to these flags (for instance, 'function_access lwho marker0').

- The access permissions of flags, including user-defined flags, can be set via the flag_access config directive. In addition to permissions for wizards, wizards/royalty, and god, there is a restrict_player option (only settable by Wizards on players, but settable by mortals on other types of things), and a privileged option (only settable by God on players, but settable by non-robot players on other types of things, if they themselves have the flag).

- The access permissions of powers can be set via the power_access config directive. The permission types available are the same as for flags.

- Command permissions can also be linked to the STAFF and HEAD flags. The "robot" permission has been removed, since nobody was using it. (The "no_robot" permission still exists, though.)

- A new flag, BLIND, suppresses has arrived / has left messages.

- A new flag, CONSTANT, prevents attributes from being set or altered on an object by anyone other than God.

- The FLOATING flag, if set on a player, now suppresses floating-room messages sent to that player.

- There is now a link_to_anything power, and an open_anywhere power, doing the obvious; these are handy for building-staff players.

- The see_hidden power now works. DARK is really two concepts, not showing up in the WHO list and not showing up in the contents list / moving silently. see_hidden allows seeing the former but does not affect the latter. These two concepts are now handled in a consistent manner within the server.

### CHANGES TO COMMANDS

- New /info switch to @attribute shows global attribute flags for a single attribute (similar to what '@list user_attributes' produces for all user-defined attributes).

- @chown now checks a ChownLock on CHOWN_OK objects.

- @chown, @chownall, @chzone, and @clone now strip flags in a consistent manner, as defined by the conf option stripped_flags. The /nostrip switch negates this stripping. For consistency, @clone/inherit no longer preserves IMMORTAL (but it still preserves INHERIT).

- @clone/preserve can be used by anyone, but you must control the original object's owner.

- A @cron facility allows tasks to be scheduled at specific times, in much the same way that Unix cron does.

- New /instant switch to @destroy causes objects to be instantly destroyed (rather than being queued for destruction at the next purge). Also, a new conf option, instant_recycle, controls whether or not Destroy_OK objects are instantly recycled (rather than being queued for destruction).

- New /pretty switch to examine and @decompile "pretty-prints" (with indentation) attributes. Based on Andrew Molitor's +pex-equivalent code.

- New /pairs switch to examine matches parentheses, brackets, and braces(), displaying them in ANSI colors. Based on Robby Griffin's ChaoticMUX code.

- New @freelist command moves an object to the head of the freelist, until the next dbck.

- New /noeval switch to @function defines a user-defined function whose arguments are not pre-evaluated.

- When you try to 'give' someone money, their ReceiveLock, rather than their UseLock, is checked.

- New @hashresize command dynamically resizes the hash tables. This is also automatically done at startup time.

- The @list options command has been reformatted and reorganized. A new command, @list params, lists additional configuration parameters.

- New /reply and /replyall switches to @mail allow replying to a mail message, including quoting it via the additional /quote switch.

- An object can @program another object if the first object or its owner has the Program power, or the first object controls the second. (This fuses the 2.2 and MUX models.)

- @program now reads attributes on the object's parent chain as well, not just the object itself (thus behaving like @trigger and friends).

- @ps now shows the attribute being waited upon, for non-Semaphore semaphore waits.

- @stats() and stats() now count the number of Going objects, as well as the number of objects of unknown (corrupted) type.

### CHANGES TO FUNCTIONS

- Functions for generic named variables, preserved in a manner similar to the stack (i.e., associated with a specific object, persistent until a restart is done), have been added. setx() sets a named variable, xvars() parses a list of strings into a list of variables, regparse() parses a regular expression into a list of variables, x() accesses a named variable (as does %_<var>), lvars() lists named variables, clearvars() mass-unsets named variables, and let() does a Scheme-style code block (with localized variables).

- Functions for generic named data structures (data types), preserved in a manner similar to the stack, in a LISP-like style. structure() defines a structure, unstructure() undefines one, construct() and load() create instances of structures, destruct() removes an instance of a structure, unload() dumps the components of an instance, z() gets the component of an instance, modify() modifies a component of an instance, and lstructures() and linstances() list the names of structures and instances, respectively.

- The equivalent of v(ATTRIBUTE) can now be accessed as '%=<ATTRIBUTE>', where the angle-brackets are literal.

- Functions that take output delimiters can take null output delimiters (symbolized by the token '@@') and newline ('%r') output delimiters.

- Booleans, as represented by functions such as t(), andbool(), and ifelse(), are now handled in a more sensible manner. All dbrefs (#0 and higher) are now considered true; #-1 and below are considered false. The string '#-1 <string>' (such as '#-1 NO MATCH') is considered false. All other strings beginning with '#' are treated like arbitrary strings, so, for instance, lists of dbrefs ('#0 #3 #5') are considered true. The old behavior can be obtained by enabling the booleans_oldstyle config parameter.

- The ansi() function compacts multiple ANSI attributes into a single ANSI code.

- String-manipulation functions, such as edit() and mid(), no longer strip ANSI characters, and @edit is better able to handle ANSI characters.

- An ANSI underline code, %xu, has been added.

- Added chomp() function -- akin to perl chomp(), it chomps off a trailing carriage-return newline from a string, if there is one. (Useful for dealing with pipe output.)

- Added command() function, which allows the execution of a variety of built-in commands in function form, such as @parent and @name.

- Added doing() function, to get @doing text.

- Added force(), trigger(), and wait() functions. Evil, but useful.

- iter() and list() can now be nested, and the nesting level can be obtained with '#!'. This changes the way parsing is done on both functions, and may affect the manner in which arguments to these functions should be escaped. For backwards compatibility, parse() works like the old iter() (unchanged behavior), and the new loop() function works like list() used to.

- The ladd(), lmin(), lmax(), lor(), land(), lorbool() and landbool() functions operate on lists, eliminating the necessity to fold() elements through their non-list counterparts.

- Conf parameter lattr_default_oldstyle controls what lattr() returns when it fails to match: if 'yes', this is empty (2.0.10p5 and before, 2.2.1 and later), if 'no', this is #-1 NO MATCH (2.0.10p6, 2.2.0, MUX). Defaults to 'no'.

- Added localize() function, keeping changes to r-registers within the "local" scope of that evaluation.

- Added lrand() function, generating a delimiter-separated list of N random numbers between X and Y.

- The log() function can now taken an optional second argument, the base.

- The mix() function can now take an unequal number of elements in each list. (The lists are considered to be padded out with nulls.)

- If the new conf parameter, objeval_requires_control, is enabled (it is disabled by default), the objeval() function requires that you control the evaluator, not just have the same owner as it.

- Added ncomp() function for comparing numbers comp() style (very useful for sortby() afficianados).

- Added null() function, which just eats output. (Useful for doing things like iter() with side-effect functions, and getting rid of the resulting garbage spaces.)

- The nonzero() function outputs the result of an if/else condition on a non-zero result. (This provides MUX-style ifelse() behavior. ifelse() follows the TinyMUSH 2.2 behavior of conditioning on a boolean.)

- The objmem(<thing>) function does a MUX-style object-structure count; objmem(<thing>/<wild>) does a 2.2-style attribute-text count. (Fuses the two models.)

- The pfind() function returns a dbref, or looks up a player. (This provides MUX-style pmatch() behavior. pmatch() now behaves like the MUX documentation said it should, which is identical to its PennMUSH predecessor.)

- step() does the equivalent of map() over multiple elements of a single list, with N elements at a time passed as %0, %1, etc.

- streq() does a case-insensitive comparison of two strings, returning 0 or 1.

- switchall() returns the string results from all cases that match.

- switch() and switchall() can nest the '#$' token, and the nesting level is available with '#!'.

- Vector functions no longer have a maximum dimension.

- vunit() can take an output delimiter.

- Added while() function. Evaluates elements of a list, until a termination condition is reached or the end of the list is reached.

### 3.0 Patchlevel 1 (December 5th, 2001)

  - Fixed a potentially fatal error in the clearing of variables. [3.0a4; S'ryon]

### 3.0 Patchlevel 2 (January 26th, 2001)

  - The equivalent of a @dbck is done, before a @restart, in order to recycle garbage before writing out the database. [MUX; S'ryon]

  - A fatal startup error under FreeBSD, related to helpfiles, has been fixed. [3.0a5; Alex Garcia]

  - Fixed a potentially fatal error in @dbclean. [3.0b5; Starlyn]

### 3.0 Patchlevel 3 (October 21st, 2001)

  - Fixed a problem with incorrect object names in @decompile output. [3.0b22; Ed Thomas]

  - Removed buggy code that complained when sending mail with only the word "clear". [MUX; Sally MacKay]

  - Uppercase flag names will now work with player_flags, etc. [2.0; Robert M. Zigweid]

  - Removed ANSI codes from Pueblo xch_cmd link attributes for MUSHclient compatibility. [MUX; Nick Gammon]

  - @addcommand honors a backslashed colon in the attribute during argument matching. [MUX; Eric Kidder]

  - Added regexp attribute flag support to @addcommand. [MUX]

### 3.0 Patchlevel 4 (June 8th, 2002)

  - Allow rollback from TM 3.1-format databases.

  - Fixed a potential buffer overflow in lastcreate(). [2.2]

  - Fixed a problem with skipped or doubled cron jobs. [3.0b8; Adam Dray]

  - Fixed a problem clearing attribute flags with set(). [MUX]

  - Fixed a problem with attempting to backup to / if config file was improper. [2.2]

  - Improved handling of built-ins overridden by @addcommand.

# TinyMUX 1
## TinyMUX 1.0

These are the changes to TinyMUX since TinyMUSH 2.0.10 patchlevel 6 (which is documented in the file NOTES).  Refer to the online help for more information about new features or commands.

Bugs and patches should be sent to: lauren@sneakers.org

### TinyMUX 1.0 patchlevel 4:
```
User level changes:
- New switches to commands:
    @pemit: /list
    @teleport: /quiet
    @dump: /optimize
    @wall: /admin
- New attributes:
    @amail @atofail @mailsucc @otfail @signature @tofail
- New commands: 
    @cpattr @startslave think wiznews +help brief
- New functions:
    alphamax() alphamin() andflags() ansi() art() beep() children() columns()
    cwho() dec() decrypt() default() die() edefault() elements() encrypt()
    eval() findable() foreach() grab() grep() grepi() hasattr() hasattrp()
    haspower() hastype() ifelse() inc() inzone() isword() last() list() lit()
    lparent() matchall() mix() munge() objeval() objmem() orflags() playmem()
    pmatch() ports() scramble() shl() shr() shuffle() sortby() strcat()
    strtrunc() udefault() ulocal() vadd() vdim() vmag() vmul() vsub() vunit()
    visible() zwho()
- Side effect functions:
    create() link() pemit() set() tel()
- New flags:
    AUDITORIUM: Enables SpeechLock checking.
    ROYALTY: Can see things like a wizard, but can't modify.
    FIXED: Can't teleport or go home.
    UNINSPECTED: A marker flag for things which haven't passed inspection.
    NO_COMMAND: Objects with this flag aren't checked for $-commands.
    HEAD: Marker flag for group heads.
    NOBLEED: Prevents color bleeding on systems where ANSI_NORMAL does not
             work.
    TRANSPARENT: Displays exits in a room in a long format.
    STAFF: Marker flag for staff (no powers come with the flag).
    MONITOR: When set on a player, shows MUSHwide connects and disconnects.
    ANSI: When set on a player, enables output of ANSI color codes.
- New config parameters
    commac_database guest_nuker guest_prefix have_comsys have_ident 
    have_macros have_mailer have_zones have_plushelp ident_timeout
    mail_database mail_expiration number_guests parent_recursion_limit
    plushelp_file plushelp_index wiznews_file wiznews_index
    zone_recursion_limit
- New search classes:
    POWER ZONE
- @list powers added.
- @decompile now shows zones, and parents.
- ANSI color support has been introduced. Names of things, rooms, and exits
  may include ANSI color, and in most cases name checks will be matched as
  if the name did not have color.
- In addition to the ansi() function, the %c substitution may be used for
  ANSI color. ANSI_NORMAL will be appended to any string that contains ANSI.
- A channel system/comsystem has been added.
- Comsystem commands:
    @cboot @ccharge @ccheck @cchown @ccreate @cdestroy @cdiscover @cemit
    @clist @coflags @cpflags @csec @cset @csweep @cundiscover @cusersec
    @cwho addcom allcom comlist comtitle clearcom delcom tapcom
- A new mail system has been introduced.
- Mailer commands:
    @mail @malias
- Mailer manages a linked list for each player, hashed table of player
  dbrefs in use.
- Reviewing and retraction of mail sent to players added.
- Mail sending rewritten to be more like Brandy@CrystalMUSH's +mailer.
- A new macro system has been added.
- Macro commands:
    .add .chmod .create .CLEAR .def .del .desc .edit .ex .gex .list .undef
    .status 
- Support for multiple guests characters added, similar to old system, 
  Guests are locked, set ANSI and NO_COMMAND, and inherit the zone,
  parent, money, @desc, @aconnect and @disconnect of the prototype guest
  character.
- Zones have been implemented. Control of a zone object gives a player
  control of all objects zoned to that object. Commands set on a zone object
  will be inherited by all objects in that zone. @aconnects and @disconnects
  are also inherited. @chzone is the command to change zones.
- @aconnects and @adisconnects set on the master room are global.
- Powers have been introduced, granting certain aspects of wizard power to
  individual characters using @power.
- Page command modified to take a list of players, in the format
      page [<player-list>] = <message>
  The last paged player or list of players can be paged with just
      page <message>
- Wizards may now 'cd' at the login screen to connect DARK.
- Two new indexed files, for +help and wiznews. The file for +help may
  include percent substitutions for color.
- BUILDER is no longer a flag, converted to a power.
- Wizards no longer obey any kind of lock.
- RWHO code has been removed.
- iter(), parse(), and @dolist now use #@ as a marker to show the position
  within the list.
- @edit now hilites the changed text.
- Log now shows exact failed connect string.
- #1 can no longer @newpassword itself.
- Updated to read flat file databases up to PennMUSH 1.50 p13, will also
  read DarkZone variant of PennMUSH database.
- Speechlocks are now implemented, depends on the AUDITORIUM flag.
- OUTPUT_BLOCK_SIZE defined in config.h, tune for your memory needs.
- The commands SESSION, DOING, WHO, QUIT, LOGOUT, OUTPUTPREFIX, and
  OUTPUTSUFFIX are no longer case sensitive after login.
- The flags wizard, and hidden may now be @set on attributes.
- Mail expiration checking added. Mail is automatically deleted after a
  certain number of days. 
- Minor bug in setinter() fixed, ex: setinter(a a a b b b, a a b b) 

Internal code changes:
- New functions nhash_firstentry and nhash_nextentry for numbered hashtables.
- search_nametab now returns -2 if permission is not given.
- Logged out commands no longer checked when player is connected, and have
  been included in regular command table. SESSION, DOING, WHO, QUIT, LOGOUT,
  OUTPUTPREFIX, and OUTPUTSUFFIX, when connected, affect all of the player's
  logged in descriptors.
- Username and hostname lookups now done by a slave process, to prevent lag
  when a nameserver or ident server is locked up. Ident processing is done
  much more reliably, with a 5 minute timeout.
- functions.c split into functions.c and funceval.c. Most new functions are
  in funceval.c.
- The GNU database manager is required for use.
- Attribute caching has been eliminated, object caching always used.
- Beta port concentrator code introduced, not recommended.
- When a switch is used, the permission for that command will no longer be
  checked before the permission for the switch.
- MOTD message length extended to 4096 bytes.
- GOING objects are no longer saved, blank spaces are filled with GOING
  objects when the database is loaded.
- A bug which permits overflow of certain buffers from the connect screen
  has been fixed.
- @readcache memory leaks fixed, as well as possible buffer corruption in
  decode_flags and flag_describe.
- @alias now checks to see if the alias is a valid player name.
- 'Three message' bug in @destroy, when two objects of the same name are
  nearby, is fixed.
- Many notify()'s in @list changed to raw_notify().
```

### TinyMUX 1.0 patchlevel 5:
```
- New config parameters
  fixed_home_message fixed_tel_message wizard_pass_locks
- Code made clean to work with 64-bit systems.
- @cpattr bugs fixed.
- @aconnects and @adisconnects are now checked for every object in the
  master room.
- zfun crash bug with no arguments fixed.
- Now loads parents from Penn systems correctly.
- Problems with locks on Penn 1.50 p13 and p14 fixed.
- Now able to load databases from TinyMUSH 2.2.1.
- strcpy, strncmp rewritten as StringCopy, and StringCopyTrunc and made
  inline (tested for future version).
- Quota and RQuota attribute conversion from 2.2 done correctly now.
- @set object/attribute=hidden|wizard works correctly.
- New attribute flags displayed: M (mortal dark) W (wizard only)
- Problems with attribute names of exactly 32 characters fixed.
- Division by zero problem in COLUMNS() fixed.
```

### TinyMUX 1.0 patchlevel 6:
```
- @cpattr modified to be more flexible.
- Database compression works.
- Events checking system added, only @daily implemented at the moment.
  'eventchecking' parameter added to @enable/@disable.
- Config parameter signal_action altered. On a fatal signal, the MUX either
  dumps a crash database, attempts to dump core, and reloads the last
  checkpointed database, or attempts to dump core and exits.
- New attributes:
  @daily
- New commands:
  @program @quitprogram @restart
- New functions:
  empty items peek pop push
- New powers:
  prog
- New switches:
  @mail - /cc
- New config parameters:
  cache_trim events_daily_hour guests_channel public_channel site_chars
  stack_limit
- Config parameters removed:
  abort_on_bug cache_steal_dirty garbage_chunk gdbm_cache_size have_ident 
  have_plushelp ident_timeout recycling
- Logged out commands (WHO, QUIT, etc) now only work for the descriptor that
  executes them.
- Both memory based and disk based (default) database handling are available
  via options in the Makefile.
- Radix string compression is now available via options in the Makefile
  (Andrew Molitor)
- @mail/fwd now allows you to edit a message before sending.
- @mail/read now includes a To: field which shows multiple recipients of a
  message.
- You may now place quotes around a multiple word name when using @mail.
- You may now mix player names, mail aliases, and mail numbers to be replied
  to in any @mail player list.
- Channel usage now costs a set amount of money, specifiable by the channel
  owner or a wizard.
- @nuke of any object (players, exits, things, and rooms) sets an object
  GOING, and destroys the object during the next database check. This is the
  way rooms were destroyed in earlier patchlevels.

Internal code changes:
- Wildcard routines modified slightly for speed.
- Hashing is smart: often used keys are looked up faster.
- Bugs in @malias fixed, mortals can no longer delete global aliases.
- Attribute and evaluation locks are written out using attribute names
  instead of numbers, to be friendlier to database filters. (Andrew Molitor)
- Attributes are looked up on an object using a binary search. (Andrew
  Molitor)
- Database chunk code reworked to make the allocation strategy of large
  continuously growing objects better. (Andrew Molitor)
- An empty third argument to fold() is treated as an empty base case.
- M_READ in mail.h changed to M_ISREAD for Solaris compatibility.
- Floating point exceptions under Linux fixed (Darrin Wilson)
- squish() no longer squishes newlines and tabs.
- Under Linux, optimized versions of string functions are used.
- ToUpper() and ToLower() are now simple macros.
- @mail/stats now works correctly.
- Comsys bug involving large numbers of players on a channel fixed.
- New style of management for mail messages prevents duplicated copies of
  messages to multiple recipients.
- Channel owners may use cwho().
- Fixed memory leaks involving queue overflows.
- Improved malloc debugging, with makefile options.
- Garbage objects are now of type TYPE_GARBAGE, instead of TYPE_THING.
- Minor bug with zone control fixed.
- Fixed memory leaks in sortby, foreach, and zfun.
- Fixed memory leaks in sortby, foreach, and zfun.
```

## TinyMUX 1.1
### TinyMUX 1.1 patchlevel 0:
```
- New config parameters:
  use_http concentrator_port
- New flags:
  VACATION - set only by wizards, reset when player connects. Helps mark
  inactive players who will eventually return.
- WWW support, in conjunction with 'muxcgi' external CGI program.
  (to be enabled in patchlevel 1)
- Concentrator integrated and made part of MUX distribution.
  (to be enabled in patchlevel 1)
- Fixed bug in @mail/quick.
- Fixed bug with names and pennies of cloned objects disappearing.
- Parser changed for speed, and all functions modified to support the new
  parser changes. (idea by T. Alexander Popiel)
- Guests are no longer set ANSI and NO_COMMAND by default, they follow
  the player_flags parameter.
- Names in page and mail lists are now seperated by commas.
- When given a dbref number, pmatch() will evaluate identically to num().
- Fixed bad names bug which allowed multiple players of the same name.
- Fixed bug that corrupted database on disk-based databases when LBUF_SIZE
  was changed. Buffer lengths may be freely changed.
- Buffer lengths doubled.
- WHO now shows the record number of players connected since startup, and
  the maximum number of players allowed online.
- @mail bugs that caused random seg faults fixed.
- Fixed crash bug with push() used with no arguments.
- If an object or it's owner is set DESTROY_OK, it is destroyed with no
  delay when @destroy'ed.
- Added a 3rd flag word.
- Problems loading some Penn 1.50.p15 databases fixed.
- @list process now shows number of descriptors available to the process.
- pemit() args are now only evaluated once.
```

### TinyMUX 1.1 patchlevel 1:
```
- New functions:
  setr
- New switches:
  @function/preserve: Preserves registers across user defined functions.
- New exec() option: EV_NO_LOCATION, surpresses use of %l in an expression.
- The enactor for Idle, Away, and Reject messages is now the paging player.
  Messages are not sent if they evaluate to nothing. %l surpressed.
- 'GARGAGE' type added to search classes.
- Problem with @daily attributes fixed.
- pemit() and @pemit/list check for permission.
- @decompile now works with obj/attr, and takes wildcards for the attr.
- Database strings are now encapsulated using quotation marks, ala PennMUSH.
- queue_write now uses MBUFs to store output, instead of malloc.
- Record number of players is now stored permanently in the database, using
  tag of '-R<number>'.
- @malias/add and @malias/remove may now take dbrefs as arguments.
- MBUF_SIZE reduced to 200 bytes.
- Dark players leaving/joining a channel do not emit messages on the
  channel.
- Fixed problem with name() on exits without aliases.
- @restart now works consistently along with database checkpointing, and is
  more reliable.
- Database consistency checking introduced, using checksums. Checks are made
  on strings in memory, as well as disk-based strings. When corruption is
  found, <gamename>.db.CORRUPT is dumped and the process exits. This is
  designed to detect unknown modification of strings in memory and on disk.
  Note that it will not fix corruption, only keep it from going unnoticed.
  Regular backups are the only way to ensure data integrity.
- A termination signal will cause the MUX to dump to the flatfile
  <gamename>.db.KILLED instead of dumping normally.
- The 'Startmux' script will refuse to start if either <gamename>.db.KILLED
  or <gamename>.db.CORRUPT are present, in case they are unnoticed.
```

## TinyMUX 1.2
### TinyMUX 1.2 patchlevel 0:
```
- Changed LBUF allocs that preserve global registers around user define
  functions to malloc()'s, to reduce the amount of buffers built up
  by recursion.
- Eliminated obsolete next_timer(). (Mars@SolMUX)
- Fixed bug that causes output buffer overflows.
- Fixed bad malias errors.
- Fixed @mail to work properly with radix compression.
- Players with expanded_who can see dark wizards on channels.
- Configuration parameter 'wizard_pass_locks' removed, power 'pass_locks'
  added.
- Appending and prepending mail text shows number of characters
  written/maximum.
- Non-MUX specific db code is now only compiled into the dbconvert
  executable. Removed support for old PernMUSH databases.
- Buffer pools are reset at database check time.
- A cache of names that have been stripped of ANSI codes is kept, if
  cache_names is on, for faster matching.
- You can use mail alias numbers with @malias/delete.
- Fixed V_ATRMONEY database flag and reinstituted it as a database flag.
- @malias/delete now takes an alias prefixed with *.
- Fixed memory leaks in the @mail code.
- Fixed @program memory leaks.
- Swapped around some malloc/buffer pool allocations.
- @malloc/add and remove work with names again.
- Fixed a very, very obsure problem that crashes the MUX when you output
  exactly 16,384 characters. (Mars@SolMUX)
- Fixed mail() to work with radix compression.
- Fixed @program crashes when a player who was logged in more than once and
  in an @program disconnected.
- No longer goes into an infinite db saving loop when it detects corruption.
  Simply exits.
- Objects may listen on channels.
- When a player who is already in an @program reconnects, the new connection
  is dropped to the @program prompt.
- ports() works properly when a player is logged in more than once.
- Mail errors are sent correctly when using radix compression.
```

## TinyMUX 1.3
### TinyMUX 1.3 patchlevel 0:
```
- New flags:
  GAGGED
- New functions:
  lstack()
- @wait command syntax modified to allow blocking on attributes other than
  'Semaphore'.
- Vattr code rewritten to use standard hashing code, redundant code
  optimized.
- Code REALLY made clean for 64-bit systems (IE, Alpha running Digital
  UNIX :)
- Removed database consistency checking.
- New command @dbclean, removes stale attribute names from the database.
- Default cache size reduced to 20x10.
- Pueblo support integrated into the distribution, see "help pueblo".
- Fixed @clone/cost bug that allowed players to get arbitrary amounts of money.
- set() and create() now return #-1 PERMISSION DENIED.
- after() crash bug with invalid arguments fixed.
- idle_wiz_dark config parameter now applies to all players with the Idle
  power.
- Zone bug when enterlock is using an attribute key fixed.
- Fixed builder power to enable a player to build when global building is
  turned off... expected behavior, but never worked.
- .ex or .gex on a macro longer than 192 characters no longer crashes the
  MUX.
- @decompiling an attribute on an exit now returns a string using the exit's
  first alias.
- New switch for @decompile: /dbref, which uses an object dbref instead of
  the object's name in the output.
- You may now specify @pemit/list/contents, or @remit/list.
- New command: @addcommand. See 'wizhelp @addcommand'. God only.
```

### TinyMUX 1.3 patchlevel 1:
```
- Fixed wizhelp entry for @addcommand.
- Fixed grep() and grepi() behavior.
- Fixed slave.c for Solaris 2.5.
- Makes @addcommand god only.
```

### TinyMUX 1.3 patchlevel 2:
```
- Fixed a bug with zone checking involving using an evaluation lock in the
  EnterLock for a zone.
```
### TinyMUX 1.3 patchlevel 3:
```
- Fixed a bug where the MUX would not save the mail or comsystem
  databases when restarting, or dumping a crash/killed database. It also
  enables the use of command switches for commands added by @addcommand to the
  global built-in table.
```

### TinyMUX 1.3 patchlevel 4:
```
- New command: @mail/safe
- DARK wizards are no longer visable on the @cwho
- Permissions for @cboot work correctly
- The boot message @cboot generates is now correct
- @wait/<number> no longer crashes the game
- columns() with a field argument >78 is no longer allowed
- lcon() no longer has a leading space
- before() now works properly
- Some log errors recognize type GARBAGE now
- 'go home' is now bound by the FIXED flag
- Lookups on an incomplete player name when the player is DARK fails...
  it was possible to use this to see if a DARK player was connected
- You cannot restart while the game is dumping
- configuration for the radix library no longer looks for gawk, which is
  broken
- A small memory leak in help fixed
- Numerous memory leaks in the disk based database code fixed
```

## TinyMUX 1.4
### TinyMUX 1.4 patchlevel 0:
```
- New commands:
  @delcommand @listcommands
- New config paramater 'indent_desc' will automatically put a space before
  and after object, room, and exit descriptions when being looked at.
- The built-in macro system has been removed. You MUST change your config
  file parameter 'commac_database' to 'comsys_database'!
- Fixed bugs causing problems with '#-1 PERMISSION DENIED' in function
  return values.
- @decompile now includes ansi codes when printing names which have ansi
  color imbedded.
- Fixed zone problem that would crash the MUX periodically when a object was
  examined.
- You can no longer use objeval on #1.
- ifelse() only evaluates the necessary arguments.
- foreach() now takes two additional arguments: one to begin parsing, and 
  one to end parsing.
- strcat() now takes 1 to n arguments.
- Range checking in delete() fixed.
- New function: case() is like switch(), but does no wildcard matching.
- 'news' now understands ANSI substitutions much like '+help' does.
- Sets guest characters DARK when they disconnect, so they'll not emit
  messages when a new guest connects.
- The new mail check now always occurs upon login.
- @addcommand now renames replaces built-in commands to '__<command name>'.
- @wait <n>=@restart will no longer crash the MUX.
- @oemit now works as described in the help, no longer only in the executing 
  player's location.
```

### TinyMUX 1.4 patchlevel 1:
```
- This fixed a problem with the help for mail(), a crashing problem with
  repeat(), and a compiling problem with patchlevel.h.
```

### TinyMUX 1.4 patchlevel 2:
```
- This patch fixes a problem with patchlevel.h, as well as a problem with
  @delcommand where restoring a built-in command was not functioning properly.
```

### TinyMUX 1.4 patchlevel 3:
```
- Fixed problems with zone recursion, as well as a problem with examining a
  zoned object and seeing all attributes as the same name.
- All semaphores are now cleared on objects when they are destroyed.
- @clone now creates object with the correct cost.
- Fixed problem with @oemit that crept in 1.4p0.
```

### TinyMUX 1.4 patchlevel 4:
```
- Fixed problems with makefile
```

### TinyMUX 1.4 patchlevel 5:
```
- Fixed player-only permission for @doing
- Fixed queue problem in cque.c
- Fixed zone recursion problem
- Control in zwho() and inzone() now handled properly
- Fixed buffer problems with columns(), center(), and @mail
```

### TinyMUX 1.4 patchlevel 6:
```
- addcommand now properly evaluates input
- null pointer bug in queue code fixed (Myrrdin)
- name can no longer be just ansi characters (Marlek)
- length checking in do_attribute and atr_str fixed (Robby Griffin)
- permissions in zwho, inzone, and objeval fixed (Geoff Gerrietts, Marlek)
- @daily attribute now properly cleared (Peter Clements)
```

### TinyMUX 1.4 patchlevel 7:
```
- Fixed more command queue problems
```

### TinyMUX 1.4 patchlevel 8:
```
- Fixed a naming bug in @create.
```

### TinyMUX 1.4 patchlevel 9:
```
- Fixed @addcommand problems
- A problem with halting an object with commands still in the queue
- A string trunction problem with mail alias names
- A problem when mudconf.createmin == mudconf.createmax
- hastype() no longer requires examinable
```

### TinyMUX 1.4 patchlevel 10:
```
- Fixed more @addcommand problems
- Fixed problem with ifelse() involving a misplaced pointer and conditional
```

### TinyMUX 1.4 patchlevel 11:
```
- Fixed segv bug in do_mvattr cause by lack of a pointer check
- A bind_and_queue call in walkdb.c was missing an argument
```

### TinyMUX 1.4 patchlevel 12:
```
- Munged queue code
- Fixed 'restrict' variable which is a keyword in Linux 2.0.30
```

### TinyMUX 1.4 patchlevel 13:
```
- Fixes single-letter leadin support for @addcommand
- Fixes buffer overflow problem with addcom
  Reported by StormBringer <sb@cygnus.ImagingA.com>
```

### TinyMUX 1.4 patchlevel 14:
```
- Fixed flag functions to not report dark and connected to non-wizards
  (Soruk@AuroraMUSH)
- Fixed help for inc() and dec() (Jeff Gostin)
- Names of objects may not exceed MBUF_SIZE
- @program help fixed (Kevin@FiranMUX)
- trunc() and round() now behave correctly with floating point and negative
  arguments (Robby Griffin)
- Many buffer overflows fixed (Robby Griffin)
- Only players can be added to mail aliases, and array overflow problem with
  @malias fixed (Robby Griffin)
- Other minor bugfixes
```

### TinyMUX 1.4 patchlevel 15:
```
- safe_tprintf_str now NULL terminates its output
```

## TinyMUX 1.5
### TinyMUX 1.5 patchlevel 0:
```
- New functions:
  regmatch() translate()
- Regular expressions may now be used in command patterns and listen patterns
  with the addition of a 'regexp' attribute flag. See 'help REGEXPS'.
- Piping of command output has been implemented. See 'help piping'. This
  may break some code... have to write conversion script.
- @addcommand now works with single letter commands (". :, ;, etc) however
  the original commands may *not* be accessed by prefixing with __.
```

### TinyMUX 1.5 patchlevel 1:
```
- Fixed new command queue problem introduced by piping, where a current
  queue entry could be overwritten while executing. Reported by Robby Griffin.
```

### TinyMUX 1.5 patchlevel 2:
```
- NOTE: This patch REQUIRES a reload from flatfile. Please backup your
  database before you install this patch.
- Integrated 1.4p11 fixes
- Fixed 'help PIPING'
- Removed redundant queue checking code in cque.c
- Added set_prefix_cmds() declaration
- Added translate() and regmatch() declarations :)
- Removed old 'fix_ansi()' code
- Database chunk code reworked to store all object text directly in the
  GDBM database, rather than the old method of storing an index in the GDBM
  database that pointed into a 'chunk file'. The <gamename>.gdbm.db
  'chunk file' is no longer needed.
```

### TinyMUX 1.5 patchlevel 3:
```
- Fixed problem with dbconvert that prevented loading from flatfile
- Changed CHARBITS in regexp code to CHARMASK since some systems define
  CHARBITS
- Integrated 1.4p12 fixes
- Change to @addcommand code to allow for ':' and ';' to be replaced
```

### TinyMUX 1.5 patchlevel 4:
```
- Integrated 1.4p13 fixes
- Adds missing argument to fn_range_check in regmatch()
```

### TinyMUX 1.5 patchlevel 5:
```
- Memory leaks in the database code fixed
- Integrated 1.4p14 fixes
```

### TinyMUX 1.5 patchlevel 6:
```
- Integrated 1.4p15 fix
```

### TinyMUX 1.5 patchlevel 7:
```
- Integrated 1.4p16 fixes
```

## TinyMUX 1.6
### TinyMUX 1.6 patchlevel 0:
```
- Finally fixed remaining database memory leaks
- Fixed out of range error in mid() (Xidus@Empire2710)
- No embedded newlines in channel titles (Reported by Robby Griffin)
- @clone may not be used to create an object with an ANSI code as its name
  (Reported by Robby Griffin)
- name() no longer causes problems on exit names which lack semicolons
  (Reported by Robby Griffin)
- No spurious prompts from @program after a @quitprogram (Kurt Fitzner)
- lexits() no longer leaves a leading space (Kurt Fitzner)
- o-messages (@odrop, @osucc) will no longer generate any messages if the
  attribute evaluates to a null string (Kurt Fitzner)
- @notify <object>/<attr> no longer incorrectly returns "Permission Denied"
  on some platforms (Reported by Robby Griffin)
- Extending a number past the end of a buffer with certain floating point
  functions no longer causes crashes (Reported by Robby Griffin)
- Fixed @cdestroy permissions (Reported by Robby Griffin)
- Fixed problem with stripping spaces after function names when
  space_compress is turned on. (Robby Griffin)
```

# TinyMUSH 2
## TinyMUSH 2.2
### TinyMUSH 2.2.5 (04/28/99)
```
--- New Features ---

- New @eval command is like the @@ comment, but evaluates its argument.
  (command.c, command.h, predicates.c)

- New @list type, 'cache', shows cache object chains. (command.c,
  udb_ocache.c)

- New @timecheck command views "timechecking" -- a counter on how much
  clock time is being used by objects. NO_TIMECHECKING compile-time define
  turns this off. This is also a log option. (Makefile.in, bsd.c, command.c,
  command.h, conf.c, cque.c, externs.h, log.c, game.c, interface.h,
  mudconf.h, mushdb.h, object.c)

- '@list hash' now shows stats for @function hash table. (command.c)

--- New Functions ---

- New filterbool() function does a boolean truth-check rather than 
  looking for "1". (functions.c)

- New inc() and dec() functions increment/decrement a number by 1.
  (functions.c)

- New link() function acts like @link. (functions.c)

- matchall() now takes an output delimiter. (functions.c)

- New pemit() and remit() side-effect functions, equivalent to @pemit
  and @pemit/contents (@remit). (functions.c, speech.c, externs.h)

- New t() function returns 0 or 1 depending on boolean truth. (functions.c)

- New tel() function acts like @teleport. (functions.c)

- New toss() function pops item off stack and discards it. (functions.c)

- New wipe() side-effect function, equivalent to @wipe. (functions.c)

--- Function Bugfixes ---

- Fixed crash problem on bor(); it takes 2 args, not varargs. (functions.c)

- die() with a die of <=0 no longer causes crash [2.2.4 patch]. (functions.c)

- elock() now checks AF_IS_LOCK, not AF_LOCK. (functions.c)

- isdbref(#) returns 0, not 1. (functions.c)

- regmatch() coredump problem on writing to r()-registers on a failed
  match fixed. Per Talek's PennMUSH patch. [2.2.4 patch] (functions.c)

- Null pointer dereference on empty stack with swap() fixed. [2.2.4 patch]
  (functions.c)

--- Other Bugfixes ---

- @mvattr null pointer dereference fixed [2.2.4 patch]. (set.c)

- Parent chain check on objects with invalid locations no longer causes
  crash [2.2.4 patch]. (command.c)

- Infinite loop problem in list_check() where Next(thing) == thing, fixed.
  Per HavenMUSH. (game.c)

- Fixed null pointer dereference when setting attribute with number but
  no name. Per Malcolm Campbell. (db.c)

- You can no longer @clone/cost exits in locations you do not control.
  (create.c)

- @decompile prints attribute flags before locking the attribute, so
  that the flags get set when the object is uploaded. (look.c)

- @edit and @wipe now respect the handling of listen attrs, calling
  handle_ears(). (set.c)

- Call check_filter() for @infilter if MSG_INV_L (aka MSG_F_DOWN) is set
  and listens are being passed; previously it was also called when MSG_INV
  was set (which was most of the time). This caused '@infilter here=*',
  even in a room without a @listen, to gag non-'say' output. (game.c)

- 'look' checks Has_exits(parent) when displaying Obvious Exits, avoiding
  exit-list screwups when parent was an exit. (look.c)

- Fixed coredump problem when looking at a Transparent exit to a bad
  destination. (look.c)

- Fixed parser issue with advancing pointer past end of buffer (thus
  spewing garbage) when ansi_colors was off. [2.2.4 patch] (eval.c)  

- SIGUSR1 calls do_restart(); the old way didn't use the currently-running
  database. This is still not an ideal solution. [2.2.4 patch] (bsd.c)

--- General Cleanup ---

- Purify is supported in the Makefile. (Makefile.in)

- Added -ldl for Linux with Tcl, per Rick White. (Makefile.in)

- Conf: SVR4 has PAGESIZE, needed for getpagesize(). (autoconf.h.in)

- Conf: Linux wants getpagesize() to return size_t or int, depending on
  version. Check for it being declared in unistd.h, and don't redeclare
  it. (autoconf.h.in, configure.in)

- Conf: Linux has declaration conflicts with srandom(). Check for it in
  stdlib.h and math.h, rather than checking on #ifdef __i386__.
  (autoconf.h.in, configure.in)

- Conf: Check for random() declare in stdlib.h before declaring it.
  (autoconf.h.in, configure.in)

- Fixed some odd dnl things, which were causing some messages not to
  appear. (configure.in)

- Regenerated configure script using autoconf 2.12. (This plus the conf
  changes above were released as a 2.2.4 patch.)

- Include autoconf.h and don't declare malloc() in regexp package. [U1]
  (regexp.c)

- did_it() attrs are specified as 0 rather than NOTHING, for @conformat
  and @exitformat. (look.c)

- Now testing for atr_buf as well as *atr_buf in @conformat/@exitformat-ing.
  (look.c)

- Queue-halted entries are NOTHING rather than 0. (cque.c)

- Print of flag letters uses safe_sb_chr() rather than safe_chr(). [U1]
  (flags.c)

- PUEBLO_SUPPORT placed correctly. [U1] (look.c)

- ansi_msg() now returns NULL if passed a null string. [U1] (netcommon.c)

- A_NULL is now used for the null (0) attribute. [U1] (attrs.h, create.c,
  game.c, look.c, move.c, set.c)

- General lint of code:
  Typecasts fixed. (db.c, db_rw.c, externs.h, functions.c, game.c, netcommon.c)
  Unused variables removed. (create.c, db.c, db_rw.c, functions.c, game.c,
  netcommon.c, predicates.c, quota.c)
  Proper initialization of variables. (functions.c, regexp.c)

--- Documentation Changes ---

- 'help @adisconnect' now notes that %0 gives the disconnect reason.

- 'help Credits' now cites Throck for an extensive bugreporting effort,
  and David Passmore (Lauren) for code from TinyMUX.

- Help for @switch/switch() now notes existing of #$ token.

- Removed semicolons at end of switch statements in 'help Tcl Control2',
  when placing Tcl code in an attribute.

- Added back in the notice about no maintenance obligations on the parts
  of the authors. (copyright.h)
```

### TinyMUSH 2.2.4 (11/21/97)
```
--- New Features ---

- Added the ability to match regular expressions in $-commands, ^-listens,
  and through the regmatch() function. (Makefile.in, attrs.h, externs.h,
  regexp.h, regexp.c, commands.c, functions.c, game.c, look.c)

- Added the @program construct, based on the TinyMUX 1.4p4 implementation.
  (attrs.h, bsd.c, command.c, command.h, db.c, interface.h, netcommon.c,
  predicates.c)

- Added the programmer() function, to get the dbref of the calling object
  in a @program. (externs.h, functions.c, netcommon.c)

- Added the @restart command, to restart the game without dropping 
  connections. Based on the TinyMUX 1.4p4 implementation. (bsd.c,
  command.c, command.h, db.c, interface.h, netcommon.c, predicates.c)

- Added restarts() function, which counts the number of times the game
  has been restarted. (conf.c, db.c, functions.c, mudconf.h)

- Added restarttime() function, which gives the time of the last
  restart. (conf.c, functions.c, game.c, mudconf.h)

- Added command piping, allowing output from one command to be passed
  via the ;| semi-colon-like construct to another command as %|.
  Fixed a small potential coredump bug in the process. Based on the
  TinyMUX 1.5p1 implementation. (conf.c, cque.c, eval.c, game.c,
  mudconf.h, netcommon.c)

- @conformat and @exitformat, with format_contents and format_exits conf
  options, allow arbitrary formatting of Contents and Obvious Exits
  lists. (attrs.h, command.c, conf.c, db.c, look.c, mudconf.h)

- Added optional ANSI sequences, based on TinyMUX/PennMUSH implementation.
  Added ANSI and NOBLEED flags, ansi(), stripansi(), and translate()
  functions, %x substitution, conf parameter ansi_colors (defaults to No).
  (bsd.c, command.c, conf.c, db.c, eval.c, externs.h, flags.c, flags.h,
  functions.c, interface.h, log.c, mudconf.h, netcommon.c, predicates.c,
  stringutil.c, ansi.h, Makefile.in)

- Added experimental functionality for an embedded Tcl 7.6 or 8.0
  interpreter, and the TICKLER flag, compile-time selection of
  TCL_INTERP_SUPPORT. (command.c, command.h, flags.c, flags.h,
  functions.c, mod_tcl.c, config.h, Makefile.in)

- iter() and @dolist replace the #@ token with the word position in the
  list, ala TinyMUX/PennMUSH. (config.h, functions.c, walkdb.c)

- switch() and @switch replace the #$ token with the switch expression,
  ala PennMUSH. (config.h, functions.c, predicates.c)

- @stats (with no args or switches) now shows the next object dbref to be
  created. (walkdb.c)

- INFO command condenses MUSH vital statistics for MUDlist purposes, as
  per functionality agreed to with Alan Schwartz, 7/6/95. (netcommon.c)

--- New Options ---

- Added safer_passwords configuration option, enforcing less-guessable
  passwords. Modification of Moonchilde-provided code. Default #1 password
  has been changed to p0trzebie. (externs.h, mudconf.h, conf.c, db.c,
  netcommon.c, player.c, predicates.c, wiz.c)

- Site lists can now be specified in CIDR IP prefix notation.
  @list site_information now lists things in this notation. (conf.c)

- Default parents can be specified by the conf parameters player_parent,
  room_parent, exit_parent, and thing_parent. (mudconf.h, conf.c, object.c)

- New log parameter keyboard_commands will log "interactive" commands
  (direct input). (log.c, netcommon.c, mudconf.h)

- New log parameter suspect_commands will log anything executed by a
  Suspect player. (mudconf.h, command.c, log.c)

- Various modifications to make the new config parameters show up in
  @list options. (command.c)

--- New Functions ---

- New functions dup(), empty(), items(), lstack(), peek(), pop(),
  popn(), push(), and swap() implement object stacks, based on the 
  TinyMUX concept. (command.c, functions.c, game.c, mudconf.h)

- New andbool(), orbool(), notbool(), xorbool() functions do real
  boolean truth checks. (functions.c)

- New band(), bnand(), bor(), shl(), shr() functions for manipulating
  bitfields. (functions.c)

- New die() function "rolls dice", ala PennMUSH, but can take a zero 
  first arg. (functions.c)

- New ifelse() function does true/false boolean cases, ala TinyMUX
  (note boolean casing, not empty-or-zero, though). (functions.c)

- New list() function dumps output of an iter()-style function directly
  to the enactor, ala TinyMUX. (functions.c)

- New lit() function returns the literal value of a string. (functions.c)

- New sees() function checks to see if X would normally see Y in the
  Contents list of Y's room. (functions.c)

- New side-effect set() function acts like @set, ala PennMUSH. (functions.c)

- New setr() function acts like setq() but returns the value of the
  string set, ala PennMUSH. (functions.c)

- New vadd(), vsub(), vmul(), vdot(), vdim(), vmag(), vunit() vector
  functions, based on TinyMUX/PennMUSH. (functions.c)

--- Changes to Functions ---

- Added output delimiter for elements(), filter(), iter(), map(), munge(),
  setunion(), setinter(), setdiff(), shuffle(), sort(), sortby(), splice().
  (functions.c)

- elock() now allows the checking of things other than DefaultLock
  by non-privileged users. (functions.c)

- Added start and end token parsing in foreach(). Idea (but not
  implementation) from TinyMUX. (functions.c)

- mix() can now take up to ten arguments total. (functions.c)

- parse() is now just an alias for iter(). (functions.c)

--- Function Bugfixes ---

- Removed unnecessary 'first' variable from filter(), iter(), mix(),
  remove(), revwords(), splice(), setunion(), setinter(),  setdiff().
  (functions.c)

- ljust(), rjust(), center(), and repeat() now check for overflows and
  null-terminate their buffers. (functions.c)

- andflags()/orflags() now prevent mortals from getting the Connected
  flag on Dark wizards. (functions.c)

- Per Malcolm Campbell, set tm_isdst to -1 to indicate unknown status
  of Daylight Savings Time for auxiliary to convtime(). (functions.c)

- delete() now acts sanely when passed a weird number of characters to
  delete. (functions.c)

- Spurious extra leading space in filter() removed. (functions.c)

- Fixed a potential coredump bug in fun_lastcreate(), when encountering
  unknown object type. (functions.c)

- Redundant clause in matchall() removed. (functions.c)

- repeat() now acts sanely when passed a zero-length string. (functions.c)

--- Other Bugfixes ---

- Fixed a memory leak in look_exits2() when Pueblo support was enabled.
  Reported by Joel Baker. (look.c)

- Removed spurious spaces around exit names in Pueblo display. (look.c)

- Added period to "You see nothing special" in Pueblo display. (look.c)

- Per Moonchilde, do not raw_notify_html() non-HTML players. (netcommon.c)

- 'look here' (and other specific looks that check your current location)
  now correctly returns the @idesc when appropriate. Reported by Throck.
  (look.c)

- examine/parent no longer returns the locks of the parent, since they're
  not inherited by the child. Reported by Throck. (look.c)

- @clone now explicitly states when a clone name has been rejected as
  illegal. Reported by Throck. (create.c)

- Blank components are no longer allowed in exit names. Reported by
  Throck. (object.c)

- @mvattr no longer clears the original attribute if no copies were
  successfully made. A null-attribute coredump bug has also been
  fixed. Reported by Throck. (set.c)

- @verb can no longer read attributes the player doesn't have permission
  to see. Reported by Throck. (predicates.c)

- The @sweep command now shows BOUNCE objects (as "messages", just as
  they would appear if they had a @listen set). The command also now
  checks for the HAS_LISTEN invisible flag rather than scanning objects
  for the @listen attribute. (look.c)

- The bug with attribute names of exactly 32 letters is now fixed for
  real. (vattr.c)

- Fixed small bug confusing Audible and Connected in the has_flag()
  utility function. (flags.c)

- When an object is destroyed, if it has a cached forwardlist in the
  hashtable, that entry is cleared. (externs.h, db.c, object.c)

- start_room, start_home, default_home, and master_room are typed as
  dbrefs, not ints. Made NOTHING rather than -1, in some cases.
  (mudconf.h, conf.c)

- queue_string() is now declared as INLINE. (externs.h, netcommon.c)

- For conf stuff, 32, 128, and 1024 are now defined constants. (alloc.h,
  mudconf.h, conf.c, wiz.c)

- All strings are properly null-terminated when strncpy() reaches the
  buffer limit. (bsd.c, conf.c, game.c, wiz.c)

- Fixed potential buffer overflow problem with logging commands and
  removed unnecessary allocation of an mbuf. (command.c)

- Variable 'restrict' in do_verb() is now 'should_restrict', to get
  around Linux 2.0.30 defining 'restrict' as a keyword. (predicates.c)

- Fixed FPE coredump on kill_guarantee_cost being set to zero. (rob.c)

--- Documentation Changes ---

- @pemit now has aliases in alias.conf

- The help text for functions has been reorganized into categories.

- The wizhelp text for config parameters has been reorganized into 
  categories.

- Redundant info in 'wizhelp log' and 'wizhelp @list log' removed.

- The use of LINK_OK for @forwardlist/@drain/@notify is now documented.

- The player_name_spaces conf option is now documented.

- The help text for trim() has been fixed to show the correct parameter
  order.

- Moved the material from the news.txt file into the help.txt file and
  updated some of the pointers.
```

### TinyMUSH 2.2.3 (11/17/96)
```
- A DB RELOAD IS REQUIRED BY THIS CODE RELEASE. Updated the db version 
  stuff to version 10. (config.h, dbconvert.c, db_rw.c, mushdb.h)

- Folded in Andrew Molitor's patch to alphabetize and binary-search
  on attributes at the cache level, fix the chunkfile growth, support
  IN_MEM_COMPRESSION compile-time option for radix compression, and
  write out names instead of numbers in attrib locks. (command.c, db.c,
  db_rw.c, dbconvert.c, game.c, mushdb.h, udb_ocache.c, udb_ochunk.c,
  autoconf.h.in)

- Folded in Cookie's improved, much faster string-handling routines. 
  (alloc.c, alloc.h, command.c, cque.c, create.c, db.c, eval.c,
  externs.h, file_c.c, flags.c, functions.c, game.c, log.c, look.c,
  netcommon.c, player_c.c, speech.c, stringutil.c, unparse.c, walkdb.c)

- "Lag checking", logging a warning if the real time (not CPU time)
  for execution of a command exceeds the config parameter lag_maximum.
  Can be turned off by the compile-time NO_LAG_CHECK option. (command.c,
  conf.c, config.h, cque.c, mudconf.h, netcommon.c)

- You can now lock out Guests from certain sites, via the guest_site
  configuration directive. (conf.c, mudconf.h, netcommon.c)

- The BOUNCE flag acts like the equivalent of a @listen of '*' with no
  @ahear. Useful for letting players be transparent to noise without
  needing to enable player_listen. (flags.c, flags.h, game.c)

- PARANOID_EMIT compile-time option prevents @pemit and @emit from being
  used on non-Affected targets. This has been previously impossible to
  prevent using the conf options. (config.h, speech.c)

- The new function lastcreate() and the NewObjs attribute keep track of
  last-created objects for a given thing. (attrs.h, db.c, functions.c,
  object.c)

- Command matches on objects can now be done unparsed, through the use
  of the AF_NOEVAL attribute flag. This allows editors that don't strip
  percent-signs, etc. (attrs.h, command.c, command.h, cque.c, externs.h,
  functions.c, game.c, look.c, netcommon.c)

- The /noeval switch to @pemit permits unparsed output.

- Pueblo patch integration; PUEBLO_SUPPORT compile-time option. This is
  a multimedia MUD client; see http://www.chaco.com/pueblo for details.
  (Makefile.in, attrs.h, command.c, conf.c, config.h, db.c, externs.h, 
  file_c.c, file_c.h, flags.c, flags.h, functions.c, game.c, interface.h,
  look.c, move.c, mudconf.h, netcommon.c, predicates.c, speech.c,
  stringutil.c)

- COMMA_IN_SAY compile-time attribute controls whether or not a comma is
  inserted or not, i.e., 'You say, "Hi."' vs. 'You say "Hi."'  (config.h,
  speech.c)

- Resetting of buffer pools is done at checkpoint time. Extra consistency
  checking and pool_reset code borrowed from MUX. (alloc.c, timer.c)

- NOSPOOF is now only wiz-visible, though anyone can set it; this
  attempts to prevent twits from excluding NOSPOOF people in mass-@pemit
  spams. STOP is now visible to everyone, but only wizards can set it.
  (flags.c)

- Function invocation limits are checked directly by the list-based 
  functions and foreach(), saving unnecssary extra passes through
  exec() in the parser, on overlofws. (functions.c)

- Seconds connected now noted in log of disconnect. (bsd.c)

- Default vattr hashtable size is now 16,384. @list hash also shows the
  size of the table. (vattr.c, vattr.h, Makefile.in)

- Changed default object cache size to 60x10 from 85x15. (udb_defs.h)

- Changed order of defines for configuration. (config.h)

- mix() no longer eats the beginning of its output, if that output is 
  NULL; its behavior is now consistent with map(). (functions.c)

- Fixed TLI socket close problems. (bsd.c)

- Properly null-terminated @ps/long output. (cque.c)

- A machinecost of 0 no longer crashes the MUSH. (cque.c)

- The ROBOT flag cannot be reset on players. (flags.c)

- Fixed memory leak in sortby(). (functions.c)

- list_check returns number of matches correctly. (game.c)

- Fixed buffer-overrun problem in queue_write(). (netcommon.c)

- Players over their paylimit don't get a paycheck. (player.c)

- Fixed '@set foo=bar:_baz/boom' bug (it was stuffing the contents of
  baz's BOOM into foo's BOOM, completely ignoring BAR).  (set.c)

- Fixed problem with attribute names of extactly 32 characters. Patch
  based on MUX. (vattr.c)

- Miscellaneous cleanup to remove compile-time warnings and small bugs
  done by Moonchilde.  (command.c, cque.c, db_rw.c, functions.c, look.c,
  netcommon.c, udb_achunk.c, udb_ochunk.c)

- Fixed some OS-specific problems:
    * Sys V variants: Andrew Molitor's filters now work.
    * HP-UX: char promoting to int in formal prototypes fixed. (externs.h)
    * Linux: myndbm now compiles properly. (myndbm.h)
    * Linux: MAXINT in delete() no longer causes a crash. (functions.c)

- Copyright notice updated to new TinyMUD copyright. (copyright.h)
```

### TinyMUSH 2.2.2 (2/14/96)
```
- Implemented the wiz-settable STOP flag. Halts further $command
  searching if one is matched on an object set STOP. Order something
  like a master room's contents by priority to get maximum use out of
  this. (command.c, externs.h, flags.c, flags.h, game.c)

- Implemented the COMMANDS flag and require_cmds_flag conf parameter
  (defaults to yes). If on, only COMMANDS-set objects will be searched 
  for $commands. Some automatic db conversion is provided (to DB 
  version 9), but it doesn't handle parents cleverly. (config.h, 
  db_rw.c, dbconvert.c, flags.c, flags.h, game.c, mushdb.h, command.c,
  conf.c, mudconf.h)

- Implemented @parent/zone, a secondary local master room check. 
  Controlled by conf parameter parent_zones (defaults to yes).
  (command.c, conf.c, create.c, externs.h, look.c, mudconf.h, mushdb.h)

- New functions: andflags(), orflags(), left(), right(), lpos(), objmem(),
  xcon(). (functions.c)

- @enable/@disable now logged as CFG/GLOBAL. (wiz.c)

- god_monitoring global parameter enables dumping commands to the
  screen (basically LOG_ALL_COMMANDS without spamming the disk).

- Memory leak fixed in foreach(). (functions.c)

- lattr() is back to its more logical 2.0.10p5 behavior. Functions which
  return lists should return empty lists on errors, rather than spewing
  goop into the list buffer. (functions.c)

- map() without a list returns an empty string, rather than trying to
  call the mapping function with nothing. (functions.c)

- udefault() didn't evaluate the arguments passed to it before calling
  the U-function. This has been fixed. (functions.c)

- Checks to make sure atol() does not gag on atr_get_raw() passing back
  a null pointer. From Ambar. (player_c.c)

- Updated alias.conf with new flag aliases and some new bad_names.

- Patchlevel information moved from 'news' to 'help', so that it gets
  installed automatically when people update their server help.
- 'help commands' is now 'help command list' (to avoid conflicting
  with help for the COMMANDS flag). 'help help' fixed to reflect this.
- 'help command evaluation2' lists the exact order of matching.
- 'help secure()' notes that the function stomps on commas.
- 'help @tel' now accurately reflects what Tport locks actually do.

- Portability modifications for BSD/386 and Linux. In particular, it
  should now compile cleanly on both; the Linux SIGFPE problem is
  also fixed, as is the BSD/386 db-eating problem.

- Platforms tested (clean compile, start, shutdown, restart):
  Irix 5.3, SunOS 4.1.3_U1, Solaris 5.4, BSD/386 1.1, Linux 1.2.11
```

### TinyMUSH 2.2.1 (6/4/95)
```
- Implemented local master rooms, the ZONE flag, and the local_master_rooms
  config parameter. (command.c, conf.c, flags.c, flags.h, mudconf.h, mushdb.h)

- Implemented @dolist/notify. (command.c, walkdb.c, externs.h)

- Implemented building_limit conf parameter, to limit total number of
  objects in the database. (object.c, conf.c, mudconf.h)

- @list options lists output_limit, local_master_rooms and building_limit
  status. (command.c)

- matchall() with no match returns the empty string, rather than a 0. 
  In general, list-based functions should return empty strings rather
  than garbage values, when errors are encountered. This was a programming
  oversight. (functions.c)

- shuffle() and squish() now properly return a null string when called
  with no args (rather than a number-of-args error). (functions.c)

- sortby()'s generic u_comp() function checks to make sure function
  limits have not been exceeded (preventing unnecessary extremely
  expensive calls to exec(), and long freezes). (functions.c)

- min() and max() no longer chop off fractional parts of numbers.
  Talek's patch for this modified to retain permissibility of passing
  the function only one arg. (functions.c)

- add(), mul(), and(), or(), xor(), min(), max() no longer keep the got_one
  temporary variable around. From Talek. (functions.c)

- not() now uses atoi() rather than xlate(), for behavior consistent with
  the other boolean functions. Note that this means it no longer regards
  dbrefs as "true". (functions.c)

- Fixed braino of && to || in dbnum(). From Talek. (functions.c)

- Mortals can now @destroy exits they're carrying. (create.c)

- Put in the mushhacks patch for fixing the site reg crash problem on the
  Alpha. This is an unverified patch. (conf.c, externs.h, mudconf.h)

- Wrote two utility functions, save_global_regs() and restore_global_regs(),
  whose purpose in life is to protect global registers from munging.
  (externs.h, eval.c)
- ulocal() now uses the utility functions. (functions.c)
- did_it() now restores the original value of the global registers after
  the @-attribute and @o-attribute have been evaluated (i.e., before 
  the action @a-attribute is done). (predicates.c)
- The state of the global registers is preserved for @filter, @prefix,
  and related attributes. (game.c)
- eval_boolexp()'s eval-lock evaluation preserves the state of the global
  registers. (boolexp.c)

- Modified the Startmush script to give more information as the pre-game
  file-shuffling is done. Added Talek's patch to automatically start a
  new db with -s if there are no existing old db files, and to do a bit
  of magic to grab the PID and tail -f the log until startup processing
  is complete.

- Fixed and updated the helptext for 'help me'.
- Updated the helptext on @aconnect/@adisconnect for global conns.
- Updated "help credits".

- Verified various fixes that were made in 2.0.10p6:
    * get(), etc. can retrieve all public attributes except for @desc,
      regardless of the status of read_remote_desc.
    * home() returns origin of exits.
    * @list options correctly reports configuration of matching commands.
    * @trigger takes a /quiet switch.
    * Rejected 2.0.10p6 "fix" to lattr() which returns #-1 NO MATCH instead
      of an empty string. (NOTE: This is untrue. It slipped in somehow.
      Error corrected in 2.2.2 release. -Amberyl, 2/14/96)
```

### TinyMUSH 2.2 Initial Public Release (4/1/95)
```
- Began with TinyMUSH 2.0.10p5 as baseline code.

- Fixed all known bugs/problems in the baseline code, including:
    * Memory leaks. Also, it is no longer assumed that malloc() never
      returns NULL.
    * Matching is done consistently and intelligently (more or less).
    * '#' no longer matches '#0'.
    * @destroy matching bug fixed, and permissions checked properly.
    * @destroy cannot blast #0; it is always treated as a special object.
    * Set action of SIGFPE to SIG_IGN for linux, which spuriously generates
      the signal without an option to turn it off.
    * @forwardlists are loaded at startup.
    * cache_reset() is called for every item, during startup.
    * cache_reset() is called periodically, when loading player names.
    * cache_trim config parameter removed. 
    * Wildcards can no longer be used to overflow buffers.
    * @alias makes sure that player name is valid.
    * @clone names cannot be non-NULL and the name must be valid.
    * elock() requires nearby or control of just one object, not two.
    * insert() can append to a list.
    * @function does not crash game when attr is nonexistent.
    * A /switch on an @attr command (@fail, etc.) does not crash game.
    * STICKY droptos properly check connected players.
    * Overflowing connect buffer does not cause game to crash.
    * Fixed typo of 0 to o in See_attr() macro.
    * Fixed lack of terminating null on make_ulist() (used by lwho()).
    * Fixed a @sweep bug which was causing Audible exits and parent objects
      not to show on the @sweep.
    * @doing cannot contain tabs or CRs -- converted to spaces. Also fixed
      an off-by-one counting error on characters dropped.
    * Removed obsolete memory-statistics stuff.
    * Fixed timer alarm()s to generate appropriate behavior on all operating
      systems (previous correct behavior was due to an ancient bug in the code 
      which caused things to always default to an error condition that 
      generated guaranteed-good behavior).
    * Two small syntax things corrected (=- to -=).
    * Fixed helptext error involving checking of $commands on parent objects.
    * Noted in help.txt that KEY affects all locks, not just the default one.
    * MYOPIC now works properly for wizards.

- Many things were fixed in configure; some compatibility changes were made.
    * db.h is now mushdb.h, in order to appease FreeBSD.
    * Support for setdtablesize() and get_process_stats() on Sequent.
    * Support for the rusage syscall() braindamage on HP-UX.

- New features were added, and some other things twiddled.
    * Quotas can be managed by object type.
    * There is support for multiple guest characters.
    * There is support for global aconnects and adisconnects.
    * Added logging of command count, bytes input, and bytes output, when
      player disconnects.
    * Note is written to log after startup processing is completed (since 
      there is often a substantial delay between the db load completing and 
      the game actually being "up").
    * @shutdown/abort causes game to coredump via abort().
    * @dbck can fix damage of the 'Next(<obj>) == <obj>' type.
    * @search reports a count of the objects found, broken down by type.
    * @pemit to disconnected/pagelocked players now does not return an error
      message.
    * @pemit/list and @pemit/list/contents sends a message to a list.
    * @decompile takes wildcards.
    * 'examine' shows more attribute flags. More individual attribute flags
      are now settable.
    * Added "audible" notation to @sweep command.
    * Idle/Reject/Away messages now take the paging player as the enactor,
      rather than the target player. This enables personalized messages for
      each player.
    * Fixed @list options to reorganize some of the parameters and remove
      redundant or obsolete stuff. Also added some previously unlisted stuff,
      like typed quotas and function limits.
    * Reorganized @list process, and removed bits of obsolete code.

- Functions were modified.
    * and() and or() stop evaluating when a conclusion is reached (this
      isn't user-visible, unless there are inline side-effect functions
      like setq(), and is done merely for internal speed).
    * hasflag() can now check attribute flags.
    * iter() and parse() evaluate their delimiter args.
    * lnum() can now take a range argument, and a separator.
    * locate() can now taken an 'X' parameter (to force an ambiguous match).

- Functions were added.
    * %q-substitution for r()
    * %c/v(c) substitution for 'last command'.
    * default(), edefault(), udefault()
    * findable(), hasattr(), isword(), visible()
    * elements(), grab(), matchall()
    * last(), mix(), munge(), scramble(), shuffle(), sortby()
    * objeval(), ports(), ulocal()
    * foreach(), squish()

- Configuration options were added and modified, and other cleanup was done.
    * Made wizard_obeys_linklock a config option (default is "no").
    * Default queue idle and queue active chunks changed to 10/10 (from 3/0).
    * Default object cache size changed to 85x15 (from 129x10).
    * New config parameters starting_room_quota, starting_exit_quota,
      starting_thing_quota, starting_player_quota, typed_quotas (defaults
      to "no") added to support Typed Quotas.
    * New config parameter use_global_aconn (default is "yes").
    * Config parameter 'guest_char_num' replaced by 'guests'.
    * Floating-point arithmetic is now an #ifdef FLOATING_POINTS in config.h
    * Hashtable sizes were upped to 512 for players, 256 for functions, and
      512 for commands.
    * The code was split up into multiple directories.
    * One consistent indentation style is in place (see misc/indent.pro)
    * New news.txt file, which says some general things about 2.2 and MUSH,
      adds a user-visible changes section, and adds a long-needed "how to
      write a news file" entry. Also modified motd.txt for the 
      "### end of messages ###"  client-stop-gagging-welcome-screen mark.
```

## TinyMUSH 2.0

This is the NOTES file for TinyMUX version 1.0, detailing the patches to
TinyMUSH 2.0 to the 2.0.10 patchlevel 6, on which TinyMUX is based.

### TinyMUSH 2.0 patchlevel 1:
```
- Replaced the dynahash database layer with Marcus Ranum's untermud
  database layer.  This fixes the 'attribute-eating' bug (aka the 'nothing'
  bug.)
- New switches to commands:
    @clone: /inherit /preserve.
- New commands: SESSION @last.
- New functions: AFTER BEFORE ROOM REVERSE REVWORDS.
- Last successful connect displayed when you log in.
- Notice of any failed connects when you log in.
- Players with the BUILDER flag set are no longer sent home when their
  container teleports.
- Corrected a problem with wizards @destroying exits in another room.
- @dolist now runs the commands in the intuitive order.
- examine and @edit now support wildcard attribute names.
- @lock/unlock <obj>/<attr> only affects the locked flag of the attribute.
- @set <obj>/<attr> = [!]no_command now sets/clears the no_command flag on an
  attribute.
- You may overwrite/clear unlocked attributes owned by others on your objects.
- Locked attributes may not be changed at all.
- TinyMUD database files up to version 1.5.5 may be read in.
```

### TinyMUSH 2.0 patchlevel 2a:
```
- Players and things may now have exits from them.
- @clone can now clone rooms and things.
- @decompile can decompile any object type.
- New locks (set by switches to @lock and @unlock commands):
    drop enter give leave page receive tport use
- Owned-by lock qualifier (matches anything owned by the owner of the object)
- Enter/leave aliases.
- You can get and drop exits.
- If the home or dropto of an object/room is destroyed, the home or dropto 
  is reset before the object is re-allocated.
- New functions: MAX MIN
- New switches to commands:
    @open: /inventory /location
    @clone: /inventory /location
- New attributes:
    @away @idle @reject: Page-return messages.
    @ealias @lalias: Enter aliases and leave aliases.
    @alias: An alternate name for a player (for paging, *-lookup, etc)
    @efail/@oefail/@aefail: for when you fail to ENTER somewhere.
    @lfail/@olfail/@alfail: for when you fail to LEAVE somewhere.
    @tport/@otport/@oxtport/@atport: for when you @teleport.
    Lastsite: the host from which you last logged in.
- You no longer get 'has left' and 'has arrived' when you drop or pick up
  hearing objects.  Others in the room (or in your inventory) still see them.
- You now always get the 'You get your xx penny deposit...' message when one
  of your objects is destroyed - even if it was automatically destroyed
  (say because it was an exit that was in a destroyed room).
- All objects that enter or look around in a location trigger the adesc and
  (if a room) asucc attributes.
- Whispering to faraway players (ala @pemitting to faraway players) is no
  longer allowed for nonnwizards.
- The HAVEN flag no longer blocks pages.  Use the new page lock for that.
- Fixed a couple of memory leaks.
- @decompile now takes a 'target name' and produces code to set attributes on
  the named target (if specified).
```

### TinyMUSH 2.0 patchlevel 2b:
```
- New functions: SEARCH STATS.
- New flags: SAFE MONITOR.
- New switches to commands:
    @destroy: /override
- If you put a space after the colon in a colon-pose command (ie ': test'),
  then the space between your name and the message will be omitted.
  This makes ': test' equivalent to ';test'.
- Fixed a money leak in the @halt command.
```

### TinyMUSH 2.0 patchlevel 2c:
```
- New functions: GET_EVAL ITER.
- Multiple @listens: Attributes of the form ^<pattern>:<commands> are checked
  when the object hears something if the MONITOR flag is set.  If the pattern
  matches, the commands are executed.
```

### TinyMUSH 2.0 patchlevel 3:
```
- New functions: EDIT LOCATE SPACE SWITCH U.
- New command: @verb.
- New flag: AUDIBLE.
- New switches for commands:
  @boot: /port /quiet
  @sweep: /exits
  @toad: /no_chown
- New option for @list: db_stats.
- New attributes: @filter @infilter @inprefix @prefix.
- New locks: linklock teloutlock.
- Non-hearing objects no longer trigger adesc/asucc when they enter or look
  around in a location.  (Non-hearing objects looking is pointless).
- SESSION now displays user port number (really the internal file_id number)
- When invoked by wizards, WHO displays the player flags DARK, UNFINDABLE,
  and SUSPECT, and the site flags REGISTRATION, FORBIDDEN, and SUSPECT.
- When invoked by wizards, DOING displays the player flags DARK, UNFINDABLE,
  and SUSPECT.
- PernMUSH databases up to version 1.18.03 and TinyMUSE databases up to
  database version 7 may now be read.
- If a robot is @chowned as part of chowning all of a user's objects
  (via @destroy, @toad, or @chownall), the robot then owns itself and is
  not owned by the target player.
```

### TinyMUSH 2.0 patchlevel 4:
```
- Dark wizards no longer generate '...has connected.' and
  '...has disconnected.' to those in the same room as them when they connect
  and disconnect.  Messages to their inventory are unaffected.
- Dark wizards no longer trigger OSUCC/ODROP/ASUCC/ADROP when traversing exits.
- If you connect to an already-connected character, others in the same room
  (and in your inventory) see the message '...has reconnected.'.
- If you QUIT from one session of a multiply-connected character, others in the
  room see '...has partially disconnected.'.
- @clone now lets you specify a different name for the new object
  (@clone <what>=<newname>)
- New switches for commands:
  @clone: /cost
- New option for @list: process
- Bug fixes:
  o Players may no longer name themselves to illegal names.
  o Infinite recursion with u() is no longer possible.
  o The inherit bit on players now works correctly.
  o Getting an unknown attribute (via get(obj/attr)) returns an empty string
    instead of "#-1 UNKNOWN ATTRIBUTE"
  o @search now correctly handles object-type-specific flags.
- The value of %# (and %n, etc) is now the object producing the message, rather
  than the object that ran the command that produced the message.  (Best
  example: A does '@tel B=#C', %# was A but is now B)
```

### TinyMUSH 2.0 patchlevel 5:
```
- New commands: @parent @@.
- New functions: PARENT SIGN.
- New config directives: full_file full_motd_message function_invocation_limit
    function_recursion_limit idle_wiz_dark lock_recursion_limit max_players
    notify_recursion_limit status_file 
- New switches for commands:
  @clone: /parent
  @motd: /full
  drop enter get leave move: /quiet
  examine: /parent
- New attributes: @forwardlist
- New option for the @list command: bad_names
- New option for the @list_file command: full
- @pcreate now only requires wizard privs to run.
- @shutdown now takes an argument, which is written to the file named by the
  status_file config directive.
- IMMORTAL players and objects no longer consume money.
- There is now a runtime-configurable maximum number of users.
- DESTROY_OK things now show off their db-number and flags.
- examine/brief now always reports just the owner (or owner+name if so 
  configured)
- Teleporting through exits no longer generates the '...attempts to push you
  from the room.' messages.
- There are now function recursion and invoction limits enforced on each
  command (to stop runaway recursion)
- Setting something audible now generates the '...grows ears...' message.
- The HALTED flag no longer gets cleared on each startup.
- Dark wizards no longer trigger OFAIL/AFAIL on exits.
- @drain now refunds money for canceled commands.
- Message forwarding filters are now evaluated before performing the compare.
- The argument to @doing is no longer evaluated.
- Parent objects: provide defaults for attributes, exits, and $-commands.
- Wizards idle for longer than the default timeout are automatically set DARK.
  This is undone when they type a command.
- Formatting for '@list costs' and '@list options' improved.
- Disallowed playernames may now include wildcard characters.
```

### TinyMUSH 2.0 patchlevel 6:
```
- The u() function can now look up attributes on other objects.
- The CONNECTED flag of dark wizards is no longer visible to nonwizards.
- Bug fixes:
    You must now pass the default lock of an object in order to take it from
       someone else, not the give lock.
    Wizards no longer accumulate money.
    examine/parent now works.
    Players may now clear their alias.
    Aleave/Oleave are now handled properly again.
    The parent field of each object is now properly initialized if the source
       database does not have parent information.
    You can no longer inadvertantly create a character with a null name.
    @search keywords 'eplayer', 'eroom', 'eobject', and 'eexit' now match
       the help file.
- Robots may only be created by players and INHERIT objects.
- %p/%o/%s now return 'its' or 'it' as appropriate for neuter objects.
- @clone now copies the parent of the original object unless you
  specify /parent
```

### TinyMUSH 2.0 patchlevel 7:
```
- New functions: CONN IDLE STRMATCH SETDIFF SETINTER SETUNION SORT.
- New attribute flag: VISUAL.
- New flags: MYOPIC TERSE ZONE.
- New locks: ControlLock UserLock.
- New attributes: Adfail Agfail Arfail Atfail Dfail Gfail Odfail Ogfail Orfail
    Otfail Rfail Tfail.
- New config parameters: cache_steal_dirty cache_trim fascist_teleport
    function_access good_name look_obeys_terse parentable_control_lock
    terse_shows_contents terse_shows_exits terse_shows_move_messages
    unowned_safe
- New switches for commands:
  look: /outside
- Function aliases added: CONNSECS -> CONN, FLIP -> REVERSE, IDLESECS -> IDLE,
    SEARCH -> LSEARCH, WHO -> LWHO.
- The match() function no longer matches the entire string (undocumented
  feature).  Use strmatch() for this functionality instead.
- The name() function can now return the name of nearby dark exits.
- @entrances now reports @Forwardlist references.
- Destroyed locations are now automatically removed from @Forwardlist lists.
- The victim of a teleport is no longer sent the message 'Teleported.'
- You can no longer give yourself away.
- New percent-substitution/v() argument: L (dbref location of invoker)
- Functions add(), mul(), and(), or(), and xor() now take up to 30 args.
- Commands on the wait queue or timed-semaphore queues should no longer
  lose time.
- Config file 'compat.conf' defines aliases and parameters like MUSH 1.x
  Useful for including in your site-specific .conf file.
- Objects inside a location with a @listen attribute no longer receive doubled
  output when there is noise generated inside the location.
- Names may be removed from the disallowed player name list while the mush
  is running.
- Access to functions may be restricted in a manner similar to that used
  to restrict access to commands.
- 'Special' objects in the database (such as the master room, default arrival
  and home locations) may not be @destroyed.
- @switch and switch() no longer evaluate arguments that need not be evaluated.
- The second argument to iter() need no longer be escaped to prevent it from
  being evaluated too soon.
- @shutdown notifies connected players who shut the game down.
- Added server support for Howard's Info-Serv remote page/who service.

NOTE: The ZONE flag and the USER and CONTROL locks don't do anything
at the moment.
```

### TinyMUSH 2.0 patchlevel 8:
```
- Signal handling: SIGHUP performs a database dump, SIGQUIT, SIGTERM, and
  SIGXCPU perform a graceful shutdown, SIGINT is logged but ignored.
  SIGILL, SIGTRAP, SIGEMT, SIGFPE, SIGBUS, SIGSEGV, SIGSYS, and SIGXFSZ
  may be optionally caught, resulting in a log entry and a panic dump.
  If you catch these signals you may not get usable coredumps.
- New config directive: signal_action
- New function: MERGE REPEAT SPLICE
- exam/parent now works with wildcarded attributes or no attribute
  specification.
- Many bug fixes.
```

### TinyMUSH 2.0.8 (10/08/92)
```
- Changed version numbering scheme to better reflect releases.
- Increased number of open descriptors on systems that support setrlimit().
  This must be configured via the Makefile.
- AUDIBLE/LISTEN forwarding should _finally_ be working correctly.
- New flag: UNFINDABLE(room)
- New attribute flag: NO_INHERIT
- New functions:
  ACOS ASIN ATAN COS SIN TAN: Trigonometric
  EXP LN LOG POWER SQRT: Logarithms and Exponentation
  FDIV: Floating point division
  E PI: Arithmetic constants
  SEARCH: Supports range limits ala @search.
- Changed functions:
  ABS ADD EQ GT GTE LT LTE MAX MIN MUL NEQ SIGN SUB: Floating point support
  LOC RLOC ROOM: Obey UNFINDABLE flag on rooms
  CONN IDLE: No longer crash the server when given a bad playername
- Changed commands:
  @quota: Now takes a playername, not an objectname.
  @entrances @find @search: Support range limits.
  @verb now works if you don't control the victim, but the victim doesn't
  run the AWHAT attribute and you only retrieve the WHAT and OWHAT attributes
  if you have permission to read them.
- New help screen 'help function classes' lists functions by class.
- When checking for $-commands or ^-listens, HALTed objects are skipped
  entirely.  _IN THIS CASE ONLY_ attributes present on HALTed objects do not
  block out attributes on the parent of the HALTed object.
  POSS OBJ SUBJ - now can return information about faraway players.
```

### TinyMUSH 2.0.8 patchlevel 1 (10/12/92)
```
- @sweep no longer shows nothing when given no arguments.
- @dolist now works as it used to in 2.0.p8 (attempted bugfix removed).
```

### TinyMUSH 2.0.8 patchlevel 2 (10/21/92)
```
- Fixed @dolist to do the 'right' thing with curly braces: curly braces are
  only removed if they completely surround an argument (ie:
  '@dolist foo={bar}', but not '@dolist foo={b;a}r'.  Fixed @apply_marked and
  @wait in the same manner.  Note that the command to be run may remove braces
  also.
- Changed curly brace handling for @edit, @switch, and @verb so that only
  curly braces that completely surround arguments are removed (ie:
  'xxx, {arg}, yyy' but not 'xxx, {a,r}g, yyy'.
- div() is now strictly integer division again.
- Cleaned up signal handling somewhat, particularly the 'panicking' code.
- Corrected code for determining whether or not a PennMUSH database uses
  the builtin chat system.
- ^-listens no longer check output from the object itself.
- Removed all references to XENIX code.
- The internal RWHO now uses a much bigger buffer for reading.
- Changed RWHO defaults so they point to an operating mudwho server.
- @listmotd for wizards now shows the motd and wizmotd files.
- Never auto@destroy sacrificed players.

  [ Note: This new functionality is considered experimental and is not
    documented in the help file.  Please test it out and report bugs,
    omissions, and inconsistencies to mushbugs@caisr2.caisr.cwru.edu ]

- Absolute possesive substitution (%a, [v(a)]), returns its/hers/his/theirs.
- Partial support for plural gender: %s/%p/%o/%a return they/their/them/theirs
  when the Sex attribute starts with a p.  _NO_ other changes have been made
  (as in ... has/have arrived/connected/etc) so as to not break @listen/@ahear
  matching.
- New functions:
  APOSS(<obj>) - Absolute possesive, just like SUBJ/OBJ/POSS.
- Changes to functions:
  OWNER() - <obj>/<attr> syntax retrieves the owner of an attribute.
  LOCK() - <obj>/<lock> syntax retrieves the named lock from <obj>.
  ELOCK() - <obj>/<lock> syntax tests the named lock on <obj>.  If <lock> is
    not the default lock you must control <obj> or it must be set VISIBLE.
  LATTR() - <obj>/<wild-attr> returns attrs whose name matches <wild-attr>
- Evaluation locks:  @lock <what> = <attr>/<value>
  The contents of the <attr> attribute on <what> are evaluated substituting
  %#/%N/etc for the player or thing that is attempting to pass the lock.
  If the result of evaluating is equal to <value> (no wildcarding done, case
  does not matter), then the player passes the lock.
```

### TinyMUSH 2.0.8 patchlevel 3 (10/23/92)
```
- Connect and disconnect messages are no longer broadcast to nearby audible
  rooms that are not linked via audible exits.
- Poses inside an audible object now deliver the PREFIX message to those
  outside the object.
- Non-$command attributes now block out $command attributes on parent objects.
- The output of the " command is now '... says "<whatever>"' again.  Oop.
```

### TinyMUSH 2.0.8 patchlevel 4 (10/27/92)
```
- The USE lock now prevents $-command and ^-pattern matching from proceeding
  further up the parent tree.
- The server no longer dies if the very first match attempt is an
  indirect lock check.
- Players may delete their aliases.
- Players are notified when they log in if their page lock is set.
- Wizards are notified when they log in if logins are disabled.
- Removed the getrusage() call from vattr_find() and vattr_nfind().
- Converted Flags/Owner/etc to macros.
- Hash tables now use bitmask-and to get the hash value instead of modulus.
- vattr_hash and vattr_nhash are now macros, not one-line procedures.
```

### TinyMUSH 2.0.8 patchlevel 5 (10/29/92)
```
- Object names are now cached by the server instead of in the attribute cache.
- Lock evaluation for previously stored locks no longer does a full object
  match when looking up objects.
- No further match attempts are made when an exact match is made for a #number,
  'me', or 'here'.
- Text read over the network interface is processed faster.
- New configuration options:
  cache_depth cache_width
- New lock primitives:
  = (is) and + (carries) modifiers to attribute locks.
- Fixed a problem with indirected evaluation locks.
- 'Notified.' and 'Drained.' feelgood messages for @notify and @drain.
```

### TinyMUSH 2.0.8 patchlevel 6 (11/24/92)
```
- Cleaned up declarations in many places.
- Added switches for commands:
    @motd/brief
    @listmotd/brief
- Removed commands:
    @cat @note gripe
- Added config option:
    cache_names
- Added functions:
    ceil()
- Added search class to @search/search()/@mark:
    parent=<object>
    things=<what> (alias of 'objects=<what>')
    ething=<what> (alias of 'eobject=<what>')
    type=things (alias of 'type=objects')
- Removed config options:
    gdbm_cache_size
- Functions lt() lte() eq() gte() gt() ne() now accept floating point args.
- Config options cache_width and cache_depth now affect the cache size
  but only when used in the config file.
- QUIET no longer checks the object's owner, only the object and the enactor.
- Players who do not own themselves may no longer set the INHERIT flag.
- convtime() is now more robust.
- trunc() now returns the correct value.
- Wizards may now locate() from #1's perspective.
- The HALT flag no longer prevents objects from picking up $-commands and
  ^-listens from a (HALTed) parent.
- Audible/forwardlist prefix generation has been tweaked (again!)
- Targeted message commands (whisper, @pemit, et al) no longer look for
  exits before nearby objects.
- Players may now build things that cost no quota when they are over quota.
- @doing now evaluates its argument when run from an actionlist.
- @sweep now tells you if your location is a PUPPET room.
- Several @quota fixes:
  @quota/fix now works
  @chownall now correctly adjusts quotas
  @link now correctly adjusts quotas when you steal an unlinked exit.
- You may now @link a linked exit that you control.
```

### TinyMUSH 2.0.8 patchlevel 7 (12/02/92)
```
- You may now make a dropto to HOME.
- setunion now correctly handles setunion(3,4 4 4 4).
- @startup works once more.
- Help entry for @password corrected.
- Object cacheing routines now compile again.
- @search t=<type> now works again.
- The ^ and $ (prefix and suffix) special characters for @edit and edit()
  may be escaped to replace a literal ^ or $.
```

### TinyMUSH 2.0.8 patchlevel 8 (12/09/92)
```
- Fixed several memory leaks. (Thanks Amberyl)
- Fixed the 'disappearing name on #0' bug.
- Removed the @whereis command.
- Added support for Linux (thanks to bob@inmos.co.uk).
```

### TinyMUSH 2.0.8 patchlevel 9 (12/12/92)
```
- dbconvert no longer aborts referencing the (nonexistent) name cache.
- New function: index().
- repeat() now returns a null string if the repeat count is zero or
  non-numeric.
- The default database has been renamed to tinymush.db in netmush.conf
```

### TinyMUSH 2.0.8 patchlevel 10 (12/18/92)
```
- Fixed a bug that causes the db to eat itself if you use object name cacheing
  and add more than 1000 objects between reboots.

  ** DO NOT RUN versions 2.0.8.p6 through 2.0.8.p9 with object name cacheing
  ** turned on.  If you do, your database will be eaten if you add more than
  ** 1000 objects between reboots.
```

### TinyMUSH 2.0.9 (12/21/92)
```
- Fixed a problem with indirect eval locks.
- Removed command: rob
- Changed the default uncompress program to "/usr/ucb/zcat -c"
- Fixed a problem with reading compressed structure databases.
- New flag: JUMP_OK for things.
- The 'loses its ears and becomes deaf' is now gender-specific.
- New functions:
  fullname(): like name() but returns the complete name of exits.
  index(): like extract() but lets you specify a separator character.
  where(): like loc() but returns the source for exits and #-1 for rooms.
- Changed functions:
  repeat(): returns a zero length string instead of an error message if its
    argument is zero, negative, or non-numeric.
  space(): returns a null string if its argument is '0'.
- Non-dark wizards are no longer treated like dark wizards when in a dark
  location.
- Connect and disconnect messages no longer report an (invalid) enactor to
  those nearby with their NOSPOOF flag set.
- A carriage return (\r) is sent following every cached file (login banner,
  motd file, etc)
- Player names may now have spaces.  Enclose the name in double quotes where
  the name is expected to be folloed by the player's password. (connect/create
  commands from the login screen, @name command when renaming a player).
```

### TinyMUSH 2.0.9 patchlevel 1 (03/15/93)
```
- The build procedure now uses autoconf to determine system-dependent features.
- New switches for commands:
  @dolist: /delimit /space.
  @femit: /here /room.
- New arguments for commands:
  @entrances: location to search.
  @sweep: location to sweep.
- New configuration directives:
  player_name_spaces.
- New functions: CONTROLS ELEMS FILTER FINDELEM FOLD LJUST PARSE R RJUST SETQ
- The following functions now take an optional delimiter parameter: EXTRACT
  FIRST INSERT ITER LDELETE MATCH MEMBER REPLACE REST REVWORDS SETDIFF
  SETINTER SETUNION SPLICE SORT WORDPOS
- New thing-specific flag: TRACE
- cat(): now takes up to 30 arguments.
- round(): no longer returns -0 for small (>  -1) negative numbers.
- sort(): now takes an optional sort type parameter, default is to autodetect
  the sort type (alphanum, numeric, or dbref).
- u(): When performing a cross-object call, %# refers to the original enactor
  but 'me' refers to the object supplying the attribute.
- @clone: /parent is now ignored if you don't control the object.
- @clone: the IMMORTAL flag is now stripped unless /inherit is given.
- @clone: eval locks are now copied correctly.
- @decompile: dumps a @parent command if the object has a parent.
- @link: now accepts more forms of the object being linked to.
- @lock: now accepts more forms when specifying a direct lock.
- @lock: is now disabled when the global building flag is turned off.
- @mvattr: should now work properly.
- @name: no longer requires a password to change a player's name.
- @name: player names are now checked more rigorously for illegal characters.
- @quota: may now be useed by objects.
- @set: Any INHERIT object may now grant INHERIT flags.
- @set: Players that do not own themselves may no longer grant INHERIT flags.
- @shutdown: The shutdown status file is no longer created with mode 0.
- @wait: Timed semaphore waits where the semaphore is negative now execute
  immediately.
- @wait: Semaphore waits where the semaphore is negative no longer increment
  the semaphore count by 1.
- look: You can now look at exits inherited from your location's parents or
  the master room.
- There are now 10 global registers, set with setq() and read with r().
- Objects may no longer execute commands inappropriate for their object type.
- The \\, #, :/;, &, and " commands may now have their access set via the
  access config directive.
- Unlocked exits are now always selected over locked exits when trying to
  traverse an exit.
- Players are now reimbursed for commands in the queue at the last database
  dump (according to their QUEUE attribute).
- References to object numbers may no longer have trailing non-numeric
  characters. (#123abc is no longer considered a legal dbref)
- Fixed two memory allocation bugs in name cache setup.
- Eval locks are now read correctly from flat files.
- Trailing spaces are now stripped from evaluated strings if space compression
  is turned on.
- Messages generated inside a non-AUDIBLE object are no longer forwarded to
  AUDIBLE exits in the same location as the object.
- Text file messages now send CRLF as the end-of-line marked.
```

### TinyMUSH 2.0.9 patchlevel 2 (05/27/93)
```
  User-Visible Changes:

- Global registers (manipulated with setq()/r() are now preserved across the
  queue.
- TRACE output is now displayed top-first, not bottom-first. [Amberyl]
- TRACE now displays both the object name and dbref in trace output. [Amberyl]
- Leading spaces are now preserved within curly braces {}/  (One leading
  space if space compression is enabled) [Felan]
- abs(): now works with floating point numbers. [Joshua]
- first(): returns a null string if passed a null string. [Jerrod]
- index(): now works properly when the list contains an element containing
  exactly one space.
- ldelete(): now works properly when the list contains exactly one element.
  [Ambar]
- remove(): no longer strips the first word when called with a null second
  argument. [Felan]
- remove(): now allows an optional delimiter.
- rest(): returns a null string if passed a null string. [Jerrod]
- words(): now allows an optional delimiter.
- words(): when called with no arguments returns "0". [Andrew]
- @link: now performs an @unlink if you don't tell it what to link to. [Talek]
- @verb: no longer discards the default WHAT and OWHAT messages if no source
  attribute for them is specified. [Amberyl]
- @unlink: can now unlink 'here'.
- LINK_OK locations now advertise their dbref and flags again. [Michele]

  Wizard-Visible Changes:

- New command: @function [Amberyl]
- New config directives: postdump_message [Ambar]
- @clone now only copies attributes that the @cloner could write. [Ambar]
- WHO no longer shows dark wizards if location #0 has been left WIZARD.
- WHO for wizards now reports players in an unfindable location with the
  'u' flag.
- The QUOTA and RQUOTA attributes are now readable by wizards. [Michele]
- The LASTSITE attribute is no longer updated by a failed login attempt.
  [Ambar]

  Performance/Maintainability Changes:

- Autoconf now tests for problems with calling signal(SIGCHLD,...) from within
  a signal handler.
- The select() timeout is now set to the time that the next queued command
  will execute, not one second.
- The alarm() timeout is now set to the time the next scheduled event
  (dump, db-check, idle check, etc) will be performed, not one second.
- The attribute cache is now reset as part of running each command.
- Queue entries now use much less memory.
- Sorting (as performed by the SORT() and SETxxxx() functions should be much
  faster.  You may now specify a 'f'loating-point sort. [Amberyl, Glenn]
- Halted objects are no longer checked for LISTEN matching.
- The maximum number of open file descriptors is now increased to the highest
  number allowed, if the target system supports the setrlimit() call. [Ambar]
- The text file database is now accessed via the stdio calls for both object
  and attribute cacheing.
- The text file database bitmap is now properly initialized when it grows.
```

### TinyMUSH 2.0.10 (08/10/93)
```
  User-Visible Changes:

- New command: @wipe
- New flags: LIGHT PARENT_OK
- New lock: ParentLock
- The TEMPLE flag has been removed.
- All flags are now valid (but not necessarily useful) for all object types.
- after() and before() no longer fail when called without arguments.
- center() no longer fails when asked to center within an overly-large field.
- lexits() no longer fails if a parent of the target is a an object type that
  does not support exits.
- trim() no longer fails when called with one argument.

  Wizard-Visible Changes:

- New attribute: QueueMax

  Performance/Maintainability Changes:

- TinyMUSH 2.0.10 uses a slightly different flatfile and struct database
  format than 2.0.9.p2.  MUSH will transparently convert your database to the
  new format when it is read for the first time.  It is recommended that you
  back up your database before running TinyMUSH 2.0.10.
- There is now a second word of flags in the database.
- Money is now stored on a (hidden) attribute.  Money for active players
  is cached.
- Changes to players' Timeout attributes now take effect immediately.
- Players' pending queue sizes are no longer stored on an attribute, but are
  cached for active players.
- Attribute number lookups are now performed directly, instead of via a hash
  table.
- There are now separate MUSH-maintained flags for the presence of the
  LISTEN, FORWARDLIST, and STARTUP attributes.
```

### TinyMUSH 2.0.10 patchlevel 1 (08/20/93)
```
- The @verb command no crashes the server if given a bad attribute name.
- Automatic datatype checking for sort() is now stricter (and more accurate).
- Player money is now saved correctly (2.0.10-specific bug)
- replace() now works correctly on a one-element list.
- The privilege requirement for the 'goto' command now also applies to using
  exits.
- extract() now works properly when using an alternate delimiter.
- index() now works when using space as a delimiter.
- parse() no longer requires a separator arg, it uses space by default.
```

### TinyMUSH 2.0.10 patchlevel 2 (08/27/93)
```
- Renamed t_xx routines to tf_xx to avoid future clashes with TLI.
- The alias config directive no longer crashes the game if given only
  one argument.
- Removed config directive: destroy_sacrifice.
- New config directive: trace_topdown.
- TRACE output may now be displayed top-down or bottom-up, the choice is made
  via a config directive.
- Setting or clearing the LISTEN or FORWARDLIST attributes no longer affects
  the DARK or JUMP_OK flags.
- db_unload now writes all attributes to the flatfile.
- elock() now works if you either control or are near both objects.
- has_flag() now correctly reports flags that reside in the extended flags
  word.
- iter() now returns a space-separated list instead of a delimter-separated
  list.
- wild.c has been updated to Talek's latest version.
- Several declarations were changed to avoid header conflicts on various
  platforms.
- Updated the configure script for autoconf 1.5.
```

### TinyMUSH 2.0.10 patchlevel 3 (09/08/93)
```
- Local RWHO support now compiles correctly.
- CONNECTED flags on players are now cleared at startup.
- Rooms may now be used as a home location again.
- @clone no longer crashes the game.
- @readcache now displays the sizes of the files read in.
- give now supports the wizard-only /quiet switch to suppress
  '.. gave you ..' and 'You give ..' messages.
- The following functions now work correctly with multiple spaces between
  words (treated as one separator) and null words in non-space-delimited lists:
  extract() filter() first() insert() iter() ldelete() map() match() member()
  parse() remove() replace() rest() revwords() setdiff() setinter() setunion()
  splice() wordpos() words()
```

### TinyMUSH 2.0.10 patchlevel 4 (10/26/93)
```
- New config directive: trace_output_limit.
- The default object matching code now checks for literal and quick-to-resolve
  matches ('me', 'here', #dbref, *player, etc) before checking nearby objects
  and exits.  If a literal match succeeds, the check for nearby objects and
  exits is not performed.
- Player names are now truncated to 16 characters in the WHO display.
- Original (Classic) TinyMUD databases may now be read in again.
- The following functions now trim their input lists if using a space as the
  delimiter: after() before() extract() filter() first() insert() iter()
  ldelete() map() match() member() parse() replace() rest() setdiff()
  setinter() setunion() sort() words()
- A memory allocation error in extending the user-defined attribute table
  has been corrected.
- @pcreate no longer crashes the game.
- Top-down TRACE output from complex functions can no longer consume
  arbitrarily large amounts of memory.
- All modified attributes are now written when a database dump is performed,
  not just ones that haven't been referred to recently.
- When using object-level cacheing, the cache is now periodically trimmed
  of unreferenced entries.
- TinyMUSH can now be compiled by non-ANSI (K&R) cc compilers.
```

### TinyMUSH 2.0.10 patchlevel 5 (11/03/93)
```
- A memory leak in the parse() and iter() functions has been corrected.
- A memory leak in the @alias and @verb commands has been corrected.
- The words() function now returns 0 when presented an empty list.
- The isnum() function no longer considers an empty string (along with certain
  other 'degenarate' numbers like '-.' to be numeric.
- sort() now performs an alphabetic sort on lists consisting of only numeric
  values and empty strings, not a numeric sort.  (The empty strings are no
  longer considered numeric)
- Spoken messages are now forwarded through audible exits when spoken inside
  an audible object in the location containing the exits.
- Messages sent through audible exits when generated from inside an audible
  object in the location containing the exits are now prefixed by the @prefix
  strings of both the object and the exit.
- @lock/tport now works correctly.
- When you try to teleport an object you control and the teleport fails, the
  failure messages are now displayed from the point of view of the victim
  object, not you.  You are still notified that the teleport failed.
- lexits() no longer returns dark exits in locations you don't control.
- @robot now works properly again.
- @list default_flags now reports the correct flags.
- Wizards may now @chown faraway objects.
```

### TinyMUSH 2.0.10 patchlevel 6 (04/20/94)
```
- iter() and parse() now evaluate their delimiter argument.
- Database damage of the form where next(<obj>) equals <obj> is now corrected.
- STICKY droptos no longer consider connected players as non-hearing objects.
- MYOPIC now works properly for wizards.
- Attributes owned by you on objects owned by others are now visible to you.
- get(), get_eval(), and u() now retrieve public attributes (other than the
  description) for all objects regardless of the state of read_remote_desc.
- exit(), lexits(), and next() now return only exits visible to or examinable
  by you.
- home() now returns the origin of exits().
- elock() now requires you to control or be near either the lock object or the
  victim, not both.
- lattr() now return "#-1 NO MATCH" if it can't find the object to search.
- @function no longer crashes the game if given a nonexistent attribute.
- Forwardlists are now set up correctly on game startup.
- Name matching now works properly when there is a partial match in a class
  checked before a class containing an exact match.
- Partial and possessive name matching should be more consistent now.
- locate() also reflects the previous two changes.
- @verb no longer fails if you provide an invoker-default but no
  others-attribute or others-default.
- @verb now obeys attribute visibility restrictions imposed by the
  read_remote_desc config option.
- @alias now removes an existing alias when an attempt is made to set it to
  an invalid name, and returns the message "That name is already in use or is
  illegal, alias cleared."
- lattr() and other commands that use or return attribute lists now obey
  the attribute visibility restrictions imposed by the read_remote_desc
  config option.
- Corrected helptext errors regarding UseLock inheritance and attribute hiding
  when searching parents for $-commands.
- Players may now @destroy exits that they are carrying.
- '@destroy me' will no longer match a carried object starting with 'Me'.
- @destroy now consistently validates the type of things to be destroyed.
- @set now supports the /quiet switch.
- @trigger now supports the /quiet switch.
- New substitution: %q<number>: get setq/r register <number> (0-9)
- locate() now accepts the locator key X, which means to randomly pick from
  multiple matches instead of returning #-2.
- Specifying a switch on an attribute-setting command (such as @afail) can
  no longer crash the mush.
- The cache is now periodically reset when loading player names into the
  player name lookup table.
- The cache_trim configuration parameter has been removed.
- @list options now reports that status of the player_match_own_commands
  config directive.
- insert() can now append to a list by using a position of one more than
  the number of elements in the list.
- @clone now copies locks.
```