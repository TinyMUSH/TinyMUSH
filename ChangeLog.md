This document details changes for TinyMUSH "mainline" releases,  as well as bugfix updates issued against those mainline releases.

# TinyMUSH 4.0 Changes

TinyMUSH 4.0 is currently in alpha. See Git's log for changes.

# TinyMUSH 3.3 Changes

TinyMUSH 3.3 is currently in alpha. See Git's log for changes.

# TinyMUSH 3.2 Changes

TinyMUSH 3.2 beta 1 was released on February 23rd, 2009.

The "full", gamma release of TinyMUSH 3.2 is dated June 6th, 2011.

# TinyMUSH 3.1 Changes

TinyMUSH 3.1 beta 1 was released on May 8th, 2002.

The "full", gamma release of TinyMUSH 3.1 is dated June 21st, 2004.

## PATCHES SINCE THE 3.1 RELEASE

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

##  CHANGES TO THE BASE SERVER

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

## CHANGES TO CONFIGURATION PARAMETERS

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

## CHANGES TO ATTRIBUTES

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

## CHANGES TO FLAGS AND POWERS

- An object with the new attr_read power is able to see attributes with the 'hidden' attribute flag, just like Wizards and Royalty can.

- An object with the new attr_write power is able to set attributes with the 'wizard' attribute flag, just like Wizards can.

- An object with the new link_any_home power can set the home of any object (@link an object anywhere), just like Wizards can.

- If a wizard connects while DARK, they are reminded that they are currently set DARK. Also, wizards are notified when they are set DARK due to idle-wizards-being-set-dark. A player must have the Hide power or equivalent in order to auto-dark.

- There is a new type of lock, the DarkLock, which controls whether or not a player sees an object as DARK. Based loosely on a patch by Malcolm Campbell (Calum).

- There is a new power, Cloak, which is God-settable. It enables an object which can listen (such a player or puppet) to be dark in the room contents and movement messages; previously, this was only true for listening objects both Dark and Wizard. Note that since wizard Dark is now affected by the visible_wizards config parameter, if that parameter is enabled, wizards will need this power in order to become invisible.

- The Expanded_Who power no longer grants the ability to see Dark players in WHO. (Previously those with the power could, but weren't able to see them in functions. Use the See_Hidden power to grant the ability to see Dark players.)

- The new ORPHAN flag, when set on an object, prevents the object from inheriting $-commands from its parents (just like objects in the master room do not inherit $-commands from their parents). This allows you to @parent a command object to a data object, without worrying about the overhead of scanning the data object for $-commands.

- The new PRESENCE flag, in conjunction with six new locks (HeardLock, HearsLock, KnownLock, KnowsLock, MovedLock, MovesLock) supports "altered reality" realm states such as invisibility.

## CHANGES TO COMMANDS

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

## CHANGES TO FUNCTIONS

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

## OTHER CHANGES

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

## TinyMUSH 3.0 Changes

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

## BUGFIX PATCHES TO 3.0

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
