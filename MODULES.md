# <center>WARNING<br><br>THIS FILE IS OUTDATED AND WILL BE UPDATED IN THE FUTURE</center>

## DYNAMICALLY LOADABLE MODULES

TinyMUSH versions 3.1 and higher provide an interface for dynamically
loadable modules. This allows server administrators to write and install
"hardcode" that extends the functionality of the server, without creating
undue difficulties with future upgrades.

If you are writing a module, or you are installing a module written by
someone else, please be forewarned: Modules muck directly with the
server's internals. Using a module is, for all practical purposes,
exactly like directly hacking the server.  Thus, the safeguards that
exist with softcode do not exist with modules; if modules contain
bugs, they could cause severe issues with instability and database
corruption.

If you are writing a module, you should be fluent in reading and
writing the C programming language. You should take the time to
carefully study the existing server code. It's assumed you can
read code to figure out things that aren't explicitly documented
(which is just about everything not directly related to module
interface). It's also assumed that you understand how dynamic
loading works.

## HOW TO BUILD A MODULE

Every module must have a one-word name, such as "hello", or
"space3d", referred to generically as '\<name>' in the rest of this
document.  This is the prefix of the shared object files that are
loaded by netmush.

The source for your module should be placed in src/modules/\<name>.
The directory must use the same name as the shared object files your
code produces. The shared objects for modules go in the game/modules
directory. 

In your module's source directory you should have at least 5 files:
configure.in, configure, Makefile.inc.in, .depend, and a C source
file. Your best bet to get started is to make a copy of the 'hello'
module source, substitute your own code for hello.c, and modify
Makefile.inc.in appropriately. More advanced module code might make
use of a customized configure script and Makefile, but that is beyond
the scope of this document.

NOTE: You must change the AC_INIT macro at the top of your module's
configure.in to reference one of your C source files (it doesn't
matter which one as long as it exists) and then regenerate the
configure script using the GNU autoconf utility. Please see:

http://www.gnu.org/software/autoconf/autoconf.html

Getting the server to load a module is simple. For each module, add
the following line to your netmush.conf file:
module \<name>

Modules can only be loaded when the game is started.

CAUTION: If you are changing a dynamically-loaded module while the
game is running, do NOT overwrite the old shared objects. The Makefile
fragments we've included will move old .so files out of the way,
but because they keep only one backup copy, they won't help if you
rebuild your module twice before reloading it into the game. To be
certain, you must go to game/modules/ and rename the .so file
associated with your module.

## MODULE NAMES AND SYMBOLS

Everything within a module should occupy a unique namespace. Functions
names, non-static global variables, and other public entities should
be named 'mod_\<name>_\<regular name>'. Macros and constants should be named
MOD_\<name>_\<regular name>. This avoids potential namespace collisions.

All modules should begin with the following line:
#include "../../api.h"

Including this file should get you access to the commonplace structures
and functions in the MUSH server, as well as specific module API things.
Please note that if you plan to use any symbol that's not defined within
api.h or a file that is #include'd by api.h, you will either need to
declare it extern within your module's .c file, or #include the 
appropriate main server .h file, as appropriate. 

CAUTION: Not all operating system's dlopen() calls properly check for
unresolved references. On such a system, if you reference a symbol that
your module cannot locate because it's not local and it wasn't declared
as extern, the server will crash when it tries to resolve the symbol.

## MODULE CONFIGURATION

The module configuration should come next, since the rest of your module
will use this information.

You should declare a configuration structure for your module as follows:

``` C
struct mod_<name>_confstorage {
	<type1> <name1>;
	<typeN> <nameN>;
} mod_<name>_config;
```

Then, you should declare a configuration table, as follows:

``` C
CONF mod_<name>_conftable[] = {
{(char *)"<name>_alias",	cf_alias,	<set perms>,	<read perms>,
	(int *)&mod_<name>_<table>_htab,	(long)"<description>"},
{(char *)"<boolean option>",	cf_bool,	<set perms>,	<read perms>,
	&mod_<name>_config.<bool_param>,	(long)"<yes/no assertion>"},
{(char *)"<read-only param>",	cf_const,	CA_STATIC,	<read perms>,
	&mod_<name>_config.<const_param>,	(long)<as appropriate>},
{(char *)"<integer param>",	cf_int,		<set perms>,	<read perms>,
	&mod_<name>_config.<int_param>,		<maximum value or 0>},
{(char *)"<dbref param>",	cf_dbref,	<set perms>,	<read perms>,
	&mod_<name>_config.<dbref_param>,	<default valid if invalid>},
{(char *)"<options flagword>",	cf_modify_bits,	
	&mod_<name>_config.<options_param>,	(long)<name table>},
{(char *)"<nametable>_access",	cf_ntab_access,	<set perms>,	CA_DISABLED,
	(int *)<table>,				(long)access_nametab}, 
{(char *)"<word option>",	cf_option,	<set perms>,	<read perms>,
	&mod_<name>_config.<int_param>,		(long)<option nametable>},
{(char *)"<whatever>_flags",	cf_set_flags,	<set perms>,	<read perms>,
	(int *)&mod_<name>_config.<flag_param>,	0},
{(char *)"<word>",		cf_string,	<set perms>,	<read perms>,
	(int *)&mod_<name>_config.<string_ptr>,	<maximum length>},
{ NULL, NULL, 0, 0, NULL, 0}};
```

Note that the table must always end with a null entry.

In the conf file, any config parameters for a module must, of course,
be placed after module directive loading that module.

## MODULE INITIALIZATION

The module initialization function goes LAST in your module's .c file.
It should read as follows:

``` C
void mod_<name>_init()
{
	/* Initialize local configuration to default values. */ 

	mod_<name>_config.<bool_param> = <0 or 1>; 
	mod_<name>_config.<int_param> = <integer>;
	mod_<name>_config.<string_param> = XSTRDUP(<string>,
						   "mod_<name>_init");

	/* Load tables. */

	register_hashtables(mod_<name>_hashtable, mod_<name>_nhashtable);
	register_commands(mod_<name>_cmdtable);
	register_prefix_cmds("<prefix chars>");
	register_functions(mod_<name>_functable);

	/* Any other initialization you need to do. */
}
```

If you have no hashtables, numeric hashtables, commands, and/or functions,
you should still call these functions, with a parameter of NULL.

Note that there is no main() function in a module.

## DEFINING COMMANDS

Commands fall into four categories of syntax:


|TYPE|SYNTAX PATTERN|EXAMPLE|
|--|--|--|
|NO_ARG|\<cmd>|@stats|
|ONE_ARG|\<cmd> \<argument>|think \<message>|
|TWO_ARG|\<cmd> \<arg 1> = \<arg 2>|@parent \<object> = \<parent>|
|TWO_ARG_ARGV|\<cmd> \<arg 1> = \<arg 2a>,\<...>|@trig \<obj>/\<attr> = \<p1>,\<..>|


api.h defines a number of macros used to declare command handlers. 
These should be largely self-explanatory. The syntax of a command
handler is:

``` C
DO_CMD_<type>(mod_<name>_do_<handler name>)
{
	/* code goes here */
}
```

To write a handler for "@hello", a command with one argument in a module
called "hello", for example, you would write:

``` C
DO_CMD_ONE_ARG(mod_hello_do_hello)
{
	/* code goes here */
}
```

Once you have declared all of your handlers, you must place the
handlers in the command table, which takes the following format:

``` C
CMDENT mod_<name>_cmdtable[] = {
{(char *)"<command1>",		<switches>,	<permissions>,
	0,		CS_<handler type>,
	NULL,		NULL,	NULL,		{<handler function>}},
{(char *)"<command2>",		<switches>,	<permissions>,
	0,		CS_<handler type>,
	NULL,		NULL,	NULL,		{<handler function>}},
{(char *)NULL,			NULL,		0,
	0,		0,
	NULL,		NULL,	NULL,		{NULL}}};
```

You can put as many commands into this table as you'd like. Make
sure it ends with the "null" entry, though.

\<switches> is the name of a switch table, described below.

\<permissions> consist of an OR'd ('|') list of permissions, such as
CA_PUBLIC, CA_WIZARD, and CA_GOD.

\<handler type> is the same as the \<handler function> type. For
example, if you have DO_CMD_NO_ARG(mod_hello_do_heythere), then you
will have CS_CMD_NO_ARG for \<handler type>. Your \<handler function>
will be mod_hello_do_heythere.

A switch table takes the following form:

``` C
NAMETAB mod_<name>_<command>_sw[] = {
{(char *)"switch1",	<letters>,	<permissions>,	<key>},
{(char *)"switch2",	<letters>,	<permissions>,	<key>},
{ NULL,			0,		0,		0}};
```

A command can take an arbitrary number of switches, so each of these
switch tables can be as large as you like. Make sure the switch table
ends with the "null" entry, though.

\<letters> is the number of unique letters of the switch that someone
has to type. For example, if your switch name is "foobar", and
\<letters> is 3, then '/foo' is sufficient.

\<permissions> consist of an OR'd ('|') list of permissions, such as
CA_PUBLIC, CA_WIZARD, and CA_GOD, just like in the main command table.

\<key> is a bit key; handler functions check 'key & \<key>' to modify
their behavior based on any switches the user passed. If you want
a command to be able to take multiple switches as the same time, you
should add '|SW_MULTIPLE' to \<key>.

Switch tables should be placed BEFORE the command table, in your
module's .c file.

If your module defines any single-character prefix commands, you
should enter them as CS_ONE_ARG|CS_LEADIN commands in your command
table, and also pass the relevant prefix characters in a string
to register_prefix_cmds(). See the "-" command in the included mail
module for example.

-----------------------------------------------------------------------------

DEFINING FUNCTIONS

A function handler takes the following form:

``` C
FUNCTION(mod_<name>_fun_<function name>)
{
	/* code goes here */
}
```

See functions.h for what this macro'd function prototype expands to.
See the server's fun*.c files for some examples of how to write functions.

Once you have declared all of your function handlers, you must place
them in the function table, which takes the following format:

``` C
FUN mod_<name>_functable[] = {
{"FUNCTION1",	<handler>,	<args>, <flags>,	<permissions>},
{"FUNCTION2",	<handler>,	<args>, <flags>,	<permissions>},
{NULL,		NULL,		0,	0,		0}};
```

You can put as many functions into this table as you'd like. Make
sure it ends with the "null" entry, though.

\<handler> is the name of the function handler (the name of the C function
that's handling the function call: mod_\<name>_\<fun>_\<whatever>).

\<args> is the number of arguments that the function should take.
If this is variable, use 0.

\<flags> are any special function-handling flags, OR'd ('|') together.
Functions taking a variable number of arguments get a FN_VARARGS flag.
Functions whose arguments should be passed unevaluated get a FN_NO_EVAL
flag.

\<permissions> consist of an OR'd ('|') list of permissions, such as
CA_PUBLIC, CA_WIZARD, and CA_GOD, just like in the main command table.

-----------------------------------------------------------------------------

DEFINING HASH TABLES

``` C
HASHTAB mod_<name>_<htabname1>;
HASHTAB mod_<name>_<htabnameN>;

MODHASHES mod_<name>_hashtable[] = {
{ "htabname1",	&mod_<name>_<htabname1>,	<size factor>,	<minimum>},
{ "htabnameN",	&mod_<name>_<htabnameN>,	<size factor>,	<minimum>},
{ NULL,		NULL,				0,		0}};

NHSHTAB mod_<name>_<nhtabname1>;
NHSHTAB mod_<name>_<nhtabnameN>;

MODNHASHES mod_<name>_nhashtable[] = {
{ "nhtabname1",	&mod_<name>_<nhtabname1>,	<size factor>,	<minimum>},
{ "nhtabnameN",	&mod_<name>_<nhtabnameN>,	<size factor>,	<minimum>},
{ NULL,		NULL,				0,		0}};
```

The initial size factor will be multiplied by the hash factor.
The minimum is the absolute smallest size of the hash table, which
must be a power of 2.

-----------------------------------------------------------------------------

EXTENDING THE DATABASE

Though modules have access to the db structure, modules cannot add their
own data types to that db structure.

Thus, if you need to store a piece of data for every object in the 
database, your module needs to have its own parallel db structure,
as follows:

``` C
typedef struct mod_<name>_dbobj MOD_<name>_OBJ;
struct mod_<name>_dbobj {
	<type1> <name1>;
	<type2> <nameN>;
};

MOD_<name>_OBJ *mod_<name>_db = NULL;

#define OBJ_INIT_MODULE(x) \
	mod_<name>_db[x].<name1> = <value1>; \
        mod_<name>_db[x].<nameN> = <valueN>;

void mod_<name>_db_grow(newsize)
	int newsize;
{
	DB_GROW_MODULE(mod_<name>_db, newsize, newtop, MOD_<name>_OBJ);
}
```

The above will automatically take care of extending and initializing
your private module database as needed. Initialization of newly-created
objects should be taken care of by creating a handler for create_obj()
(see below).

## MODULE API FUNCTIONS

The following are "hooks" into the server. If you place these functions
in your module's .c file, they will automatically be called at the
appropriate time.

``` C
void mod_<name>_create_obj(dbref player, dbref obj)
```
Run when an object is created, after other initialization is done. Passes the creating player and the object dbref created.

``` C
void mod_<name>_destroy_obj(dbref player, dbref obj)
```
Run when an object is destroyed, before the player is compensated for its destruction, is notified of its destruction, and the object is wiped out. Passes the destroying player and the object dbref being destroyed. Note that the destroying player may be NOTHING.

``` C
void mod_<name>_create_player(dbref creator, dbref player, int isrobot, int isguest)
```
Run when a player is created, after create_obj() has been run.

``` C
void mod_<name>_destroy_player(dbref player, dbref victim)
```
Run after the player basics have been cleared out, but before destroy_obj() has been run.

``` C
void mod_<name>_announce_connect(dbref player, const char *reason, int num)
```
Run when a player connects, before the player looks at his current location, but after all other connect stuff has been executed. 'reason' is the type of connection - guest, create, connect, or cd. 'num' is the player's resulting number of open connections.

``` C
void mod_<name>_announce_disconnect(dbref player, const char *reason, int num)
```
Run when a player disconnects, after all other disconnect stuff has been executed, but before the descriptor has been deleted. 'reason' describes the disconnection (see 'help Conn Reasons'). 'num' is the player's resulting number of open connections.

``` C
void mod_<name>_examine(dbref player, dbref cause, dbref thing, int control, int key)
```
Run when an object is examined, after the basic object information is displayed but before attributes are displayed. 'control' is 1 if the player can examine thing, and 0 if not. 'key' is the key originally passed to the examine command.

``` C
void mod_<name>_make_minimal(void)
```
Run if making a minimal database is specified.

``` C
void mod_<name>_cleanup_startup(void)
```
Run after the restart database is read, but before @startups on objects are done.

``` C
void mod_<name>_do_second(void)
```
Runs once a second. 

## MODULE API FUNCTION: process_command

``` C
int mod_<name>_process_command(dbref player, dbref cause, int interactive, char *command, char *args[], int nargs)
```

Run when any object tries to execute a command, before any  built-in commands or softcoded commands are checked.

Returns a number: 0 on failure, something greater than 0 on success, and something less than 0 on failure that should stop further matching of commands.

This handler tries to run on each module in sequence. If the handler is not defined for a module, or 0 is returned, it tries the next module. A return of a number other than 0 will result in not running this handler on the other modules, this time around.

The calling server function considers a result greater than 0 to be a successful	command match, and will consider the command taken care of.

The 'player' and 'cause' parameters are the dbrefs of the objects taking the action, and which caused the action (the enactor): the equivalent of %! and %#, respectively.

The 'interactive' parameter is passed as 1 if the command was entered from the keyboard (i.e., received over the network), and 0 otherwise.

The 'command' parameter is a pointer to the command entered, space-compressed if space compression is turned on. This string SHOULD NOT be destructively modified.

The 'args' parameter consists of the stack (%0 - %9) and the 'nargs' parameter is the number of arguments on the stack.

## MODULE API FUNCTION: process_no_match

``` C
int mod_<name>_process_no_match(dbref player, dbref cause, int interactive, char *eval_cmd, char *raw_cmd, char *args[], int nargs)
```

Run when any object tries to execute a command, after all built-in and softcoded commands are checked. This occurs right before a "Huh?" message is given.

Returns a number: 0 on failure, something greater than 0 on success, and something less than 0 on failure that should stop further matching of commands.

This handler tries to run on each module in sequence, most recently-loaded module first, to least recently-loaded module last. If the handler is not defined for a module, or 0 is returned, it tries the next module. A return of a number other than 0 will result in not running this handler on the other modules, this time around.

The calling server function considers a non-zero result to be a successful	command match, and will consider the command taken care of. (Note that this is different from process_command, which considers only results greater than zero to be successful.)

The 'player' and 'cause' parameters are the dbrefs of the objects taking the action, and which caused the action (the enactor): the equivalent of %! and %#, respectively.

The 'interactive' parameter is passed as 1 if the command was entered from the keyboard (i.e., received over the network), and 0 otherwise.

The 'eval_cmd' parameter is a pointer to the command, after it has gone through percent-substitutions and other evaluations.

The 'raw_cmd' parameter is a pointer to the command entered, space-compressed if space compression is turned on, but not otherwise evaluated.

The 'args' parameter consists of the stack (%0 - %9) and the 'nargs' parameter is the number of arguments on the stack.

##  MODULE API FUNCTION: did_it

``` C
int mod_<name>_did_it(dbref player, dbref thing, dbref master, int what, const char *def, int owhat, const char *odef, int awhat, int ctrl_flags, char *args[], int nargs, int msg_key)
```

Run when did_it() is called, before the server actually does the notification/others-notification/action. 
 
The handler tries to run this on each module in sequence, most recently-moduled first, to least recently-loaded module last. If a module returns 0, the handler goes on to the next module. If the module returns something other than 0, the handler does not try any further modules. If the module returns a positive number, the main body of the server's own did_it() function is not executed. If the module returns a negative number, the main body of the server's own did_it() function is still executed.

player is the object doing something, and thing is its "victim". player is the object that would normally see the \<what> message (@desc, @move, @succ, etc.); thing is the object that would normally execute the \<awhat> action (@adesc, @amove, @asucc, etc.)

what, owhat, and awhat are attribute numbers. def and odef are the default strings for what and owhat.

ctrl_flags is a bitmask of VERB_NOW and VERB_NONAME. The VERB_NOW flag is usually the result of passing the /now switch, and normally affects the queueing of actions. The VERB_NONAME flag normally prevents the actor's name from being prepended to the odef message.

args and nargs are the stack and number of items on the stack.

msg_key is a bitmask of MSG_SPEECH, MSG_MOVE, and MSG_PRESENCE, allowing special processing related to the PRESENCE flag.

This function can be used to react to a large number of server actions, by switching on \<what>. For example, all commands that move objects eventually call did_it() with \<what> of A_MOVE. If you want to intercept all movement, just check for  'what == A_MOVE' and act accordingly in this function.

## STORING DATA IN FLATFILES

This is the first and easiest of three different methods available to
a module for storing module-specific data.

Using this method, the module reads its data from a pure ASCII text
flatfile on startup, via a module API hook, as follows:

``` C
void mod_<name>_load_database(FILE *f)
```
This loads the flatfile specific to the module. It is called at start time, immediately after the main MUSH database and any module GDBM data (see below) is loaded. This function is passed a single input parameter, an open, read-only file pointer for the module flatfile.

Every time the server checkpoints ("database dump" time), it dumps a flatfile containing that data, via the following module API hook:

``` C
void mod_\<name>_dump_database(FILE *f)
```
This dumps a flat text database specific to the module. It is called at checkpoint time. This function is passed a single input parameter, an open, write-only file pointer for the module flatfile.

The module flatfile is named 'mod_\<name>.db' and is stored in the
directory specified by the conf parameter database_home. This database
file should NEVER be opened or closed directly.

Only printable ASCII data should be stored in this file. You, the module
author, are responsible for versioning, the format of this file, etc.
Useful functions for reading and writing flatfiles can be found in
the core server's db_rw.c source.

## STORING DATA IN GDBM WITH NO CACHING

This is the second of three different methods available to a module for
storing module-specific data.

This method directly stores data within the MUSH's general GDBM
database. GDBM is a 'random-access' database that allows you to store
and fetch individual pieces of data at a time. This is identical to
how the MUSH stores its structure data and requires your module to be
able to dump or convert a printable ASCII flatfile when database
conversion (backups to and restores from flatfile) is run.

Since the MUSH stores many different types of data in GDBM, it has to have a
way to keep these pieces of data seperate. It does so by allocating each
subsystem a 'dbtype' number. A block of numbers is already reserved for
internal MUSH data, and the rest are available for use by modules. Each
subsystem specifies this number when it stores data. Before you may read or
write to GDBM, your module must obtain a dbtype number with the
register_dbtype function:

``` C
unsigned int register_dbtype(char *modname) 
```
Assigns a new dbtype ID number for a certain module name. If a dbtype ID has already been assigned for that module, it will be returned.

There are four API hooks provided for reading data needed at startup and
checkpointing to GDBM as well as reading and writing flatfiles:

``` C
void mod_<name>_db_read(void)
```
Load any module-specific data from GDBM at startup. Called during MUSH startup, and during the backup conversion process before db_write_flatfile() is called. Use this to load data which must be present when the MUSH starts.

``` C
void mod_<name>_db_write(void)
```
Write any module-specific data to GDBM. Called during MUSH checkpoints, at shutdown, and during the restoral conversion process after db_read_flatfile() is called.

``` C
void mod_<name>_db_read_flatfile(FILE *f) 
```
Load and convert flat text database specific to module, called during the restoral db conversion process (the 'Restore' script). This is identical to the 'load_database' hook except it is ONLY called during database conversion.

``` C
void mod_<name>_db_write_flatfile(FILE *f) 
```
Dump flat text database specific to module, called during the backup db conversion process (the 'Backup' script). This is identical to the 'dump_database' hook except it is ONLY called during database conversion and flatfile generation (IE, @dump/flatfile).

It is important to note that register_dbtype() must be called at the
beginning of each of these four functions since module initialization DOES
NOT TAKE PLACE when your module is loaded during database conversion, and
your module cannot know which of these functions will be called first. If
your module does not use db_read or db_write, you should call
register_dbtype during your cleanup_startup hook to make sure it gets called
before anything else. You cannot call register_dbtype in your init hook
because init is called before the database is opened.

A typical MUSH server run will look like this:

    * MUSH startup
	* Module initialization:			mod_\<name>_init()
	* Module executes db_read:			mod_\<name>_db_read()
										register_dbtype()
	* Module reads data from GDBM
	* MUSH runs...
	* MUSH and module dump/checkpoint:	mod_\<name>_db_write()
	* MUSH runs...
	* MUSH and module dump/checkpoint:	mod_\<name>_db_write()
	* MUSH runs...
	* MUSH shutdown:					mod_\<name>_db_write()

A backup database process will look like this:

	* Database conversion begins
	* Module executes db_read:		mod_\<name>_db_read()
									register_dbtype()
	* Module reads data from GDBM
	* Module writes flatfile:		mod_\<name>_db_write_flatfile()
	* Database conversion ends

A restoral process will look like this:

	* Database conversion begins
	* Module reads flatfile:		mod_\<name>_db_read_flatfile()
	* Module executes db_write:		mod_\<name>_db_write()
									register_dbtype()
	* Module writes data to GDBM
	* Database conversion ends

Reading and writing to GDBM only during db_read() and db_write() doesn't
offer much advantage over just using flatfiles. However, the advantage of
GDBM is that you may read or write any piece of data at any time instead of
having to do it all at once. So while the MUSH is running, if your module's
data is modified you may choose to write just that data immediately or wait
until db_write() to write it. In any case, you must make sure that all of
your module's data is stored when db_write() is finished to ensure
consistency when the MUSH is shut down.

Note that during the database conversion process you may have your module
read a piece of data from GDBM and write it immediately to a flatfile, or
vice versa.

So far we've told you when you can read and write, but not how. Another
advantage of GDBM is that you are not limited to printable ASCII data, but
that you can store binary data of arbitrary length. 

DBData is a structure used by the database API which contains two
items: dptr (a pointer to a piece of data) and dsize (the length of
that data). DBData is used by the API interface both for search keys
and returned or stored data. DBData is declared thusly:
	
``` C
	typedef struct {
		void *dptr;
		int dsize;
	} DBData;
```

There are three functions available for reading, writing, and deleting data
in the database:

``` C
DBData db_get(DBData key, unsigned int dbtype)
```

	Retrieves data from GDBM. 

	'key' is a DBData structure that contains a search key of
	binary data as well as the length of that data.
	
	'dbtype' should be the dbtype ID registered for your module by
	register_dbtype().

	db_get returns a DBData structure which contains the returned data
	(or NULL if the key cannot be found in the database) as well as the
	length of that data. It is *your* responsibility to free() the data
	returned by db_get.

``` C
int db_put(DBData key, DBData data, unsigned int dbtype)
```

	Stores or replaces data in GDBM.

	'key' is a DBData structure that contains a search key of
	binary data as well as the length of that data.
	
	'data' is a DBData structure which contains the binary data to be
	stored and the length of that data.
 
	'dbtype' should be the dbtype ID registered for your module by
	register_dbtype().
	
	db_put returns 0 on success and 1 on failure.
	
``` C
int db_del(DBData key, unsigned int dbtype)
```
	Deletes data from GDBM.
	
	'key' is a DBData structure that contains a search key of
	binary data as well as the length of that data.

	'dbtype' should be the dbtype ID registered for your module by
	register_dbtype().

	db_del returns 0 on success and 1 on failure.

Your search key may be a string, a number, or binary data of any length.
For example, if you want your search key to be three integers, you will
create a buffer of length 'sizeof(int)*3' and copy your three integers into
it. If you want it to be a null-terminated string, make sure your key length
is 'strlen(string) + 1'. 

## STORING DATA IN GDBM WITH CACHING

This is the third of three different methods available to a module for
storing module-specific data. It is fundamentally similar to the second
method, except data is cached in memory.

In MUSH, the cache is essentially a front-end to GDBM, and interacting with
it is identical. In fact, the function calls match almost exactly. Instead
of db_get(), db_put(), and db_del(), you use cache_get(), cache_put(), and
cache_del(). The parameters to the cache functions are the same ones you
pass to their GDBM counterparts. The cache acts as a 'buffer', making sure
the most often accessed data is already in memory, and making writes more
efficient by buffering them until a number of entries may be written at
once. You don't need to worry about how the cache operates; it will make
sure everything gets read and written properly.

Note that you cannot pass a non-malloc()ed piece of memory to cache_put. It
must have been malloc()ed, and it will become the cache subsystem's
responsibility to free it.

## WHICH METHOD OF STORING DATA TO USE

If the amount of data your module works with is small and fits easily into
memory, use simple flatfiles with load_database() and dump_database() hooks.
You do not need to worry about database conversion since all of your data is
always in a flatfile!

If you need random access to your data, consider using GDBM. The main server
uses GDBM to store everything, from object structures to attribute names.
Since it can write individual entries, it only has to write entries that
have changed instead of writing them all, thus making the process more
efficient. It also allows you to easily store binary data.

If your module with large pieces of data that are needed infrequently,
considering using the cache. MUSH attributes are stored using the
cache, and data like mail messages or large pieces of text are perfect
candidates for it.

As an example, the main server does not cache object structures or
attribute names. It reads these completely in at startup in its own
db_read(), and writes all changes out during every db_write(). It does,
however, cache attribute text, and may read or write attributes using
the cache at any time while the MUSH process is running. A similar
process takes place during database conversion. When it is creating a
flatfile from GDBM, MUSH will first load all object structures and
attribute names into memory during db_read(). When db_write_flatfile()
executes, it will write a printable ASCII text representation of
object structures and attribute names to the flatfile, and it will
fetch attributes from GDBM and write them to the flatfile one at a
time. At no time does it have all attributes in memory, and it does
not even use the cache during database conversion, instead using the
direct to GDBM routines.

IMPORTANT NOTE: If you use GDBM, it is your responsibility to clean up your
entries if they are no longer used. Since GDBM is not a relational database
(you cannot do a wildcard search, you *must* search for an exact key), you
must keep track of every entry that you write to it. Look at the code in
db_rw.c that writes attribute names for an example of how to do this. If
your module writes entries to GDBM and never deletes them, they will clog
the database until it is converted to flatfile and back, dropping the unused
entries in the process.

## GETTING NOTIFIED WHEN DATA CHANGES

You can have your module notified when a key is stored or deleted in the
cache using the following API hooks:

``` C
int cache_put_notify(DBData key, unsigned int dbtype)
```
	This hook is called when a piece of data is added to or changed in
	the MUSH cache. Your module should check for an applicable data type
	before using the key.

``` C
int cache_put_notify(DBData key, unsigned int dbtype)
```
	This hook is called when a piece of data is deleted from the MUSH
	cache. Your module should check for an applicable data type before
	using the key.
