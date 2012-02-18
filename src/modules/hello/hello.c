/* hello.c - demonstration module */
/* $Id: hello.c,v 1.19 2001/12/13 04:39:28 dpassmor Exp $ */

#include "../../api.h"

#define MOD_HELLO_HELLO_INFORMAL	1
#define MOD_HELLO_FOOF_SHOW		1

/* --------------------------------------------------------------------------
 * Conf table.
 */

struct mod_hello_confstorage {
	int show_name;
	char *hello_string;
	int hello_times;
} mod_hello_config;

CONF mod_hello_conftable[] = {
{(char *)"hello_shows_name",		cf_bool,	CA_GOD,		CA_PUBLIC,	(int *)&mod_hello_config.show_name,	(long)"Greet players by name"},
{(char *)"hello_string",		cf_string,	CA_GOD,		CA_WIZARD,	(int *)&mod_hello_config.hello_string,	MBUF_SIZE},
{(char *)"hello_times",			cf_int,		CA_GOD,		CA_PUBLIC,	(int *)&mod_hello_config.hello_times,	5},
{ NULL,					NULL,		0,		0,		NULL,				0}};

/* --------------------------------------------------------------------------
 * Database.
 */

unsigned int mod_hello_dbtype;

typedef struct mod_hello_dbobj MOD_HELLO_OBJ;
struct mod_hello_dbobj {
    int greeted;		/* counter for @hello */
    int foofed;			/* counter for @foof */
};

MOD_HELLO_OBJ *mod_hello_db = NULL;

#define OBJ_INIT_MODULE(x) \
    mod_hello_db[x].greeted = 0; \
    mod_hello_db[x].foofed = 0;

void mod_hello_db_grow(newsize, newtop)
    int newsize, newtop;
{
    DB_GROW_MODULE(mod_hello_db, newsize, newtop, MOD_HELLO_OBJ);
}

/* --------------------------------------------------------------------------
 * API export.
 */

API_FUNCTION mod_hello_exports[] = {
{ "print_greeting",		NULL },
{ NULL,				NULL }};

typedef struct greeting_input HI_INPUT;
struct greeting_input {
    dbref player;
    char *name;
};

typedef struct greeting_output HI_OUTPUT;
struct greeting_output {
    int success_code;
};

void mod_hello_print_greeting(in_ptr, out_ptr)
    void *in_ptr, *out_ptr;
{
    dbref target;
    HI_INPUT *in_p;
    HI_OUTPUT *out_p;

    in_p = (HI_INPUT *) in_ptr;
    out_p = (HI_OUTPUT *) out_ptr;

    target = lookup_player(in_p->player, in_p->name, 0);
    if (target == NOTHING) {
	out_p->success_code = 0;
	return;
    }

    notify(target, "Greetings!");
    out_p->success_code = 1;
}   

/* --------------------------------------------------------------------------
 * Handlers.
 */

int mod_hello_process_command(player, cause, interactive, command, args, nargs)
	dbref player, cause;
	int interactive;
	char *command, *args[];
	int nargs;
{
	/* Command intercept before normal matching */

	if (!strcmp(command, "hiya")) {
		notify(player, "Got hiya.");
		return 1;
	}
	return 0;
}

int mod_hello_process_no_match(player, cause, interactive, 
				lc_command, raw_command, args, nargs)
	dbref player, cause;
	int interactive;
	char *lc_command, *raw_command, *args[];
	int nargs;
{
	/* Command intercept before 'Huh?' */

	if (!strcmp(lc_command, "heythere")) {
		notify(player, "Got heythere.");
		return 1;
	}
	return 0;
}

int mod_hello_did_it(player, thing, master, what, def, owhat, odef, awhat,
		     now, args, nargs)
    dbref player, thing, master;
    int what, owhat, awhat, now, nargs;
    const char *def, *odef;
    char *args[];
{
    /* Demonstrate the different ways we can intercept did_it() calls.
     *
     * We intercept 'look' (by trapping A_DESC), and return a message of
     * our own, preventing other modules from showing something, and 
     * preventing the normal server defaults from being run. (Return 1.)
     *
     * We intercept 'move' (by trapping A_MOVE), and return a message of
     * our own, but don't prevent other modules from doing something,
     * or preventing the normal server defaults from being run. (Return 0.)
     *
     * We intercept 'use' (by trapping A_USE), and return a message
     * of our own. We prevent other modules from doing something, but
     * not the normal server defaults from being run. (Return -1.)
     *
     * If we don't get one of these, we just pass. (Return 0.)
     */

    int f, g;

    switch (what) {
	case A_DESC:
	    f = mod_hello_db[thing].foofed;
	    g = mod_hello_db[thing].greeted;
	    notify(player,
		   tprintf("%s has been greeted %d %s and foofed %d %s.",
			   Name(thing), g, ((g == 1) ? "time" : "times"),
			   f, ((f == 1) ? "time" : "times")));
	    return 1;
	    break;		/* NOTREACHED */
	case A_MOVE:
	    notify(GOD,
		   tprintf("%s(#%d) just moved.", Name(thing), thing));
	    return 0;
	    break;		/* NOTREACHED */
	case A_USE:
	    notify(GOD,
		   tprintf("%s(#%d) was used!", Name(thing), thing));
	    return -1;
	    break;		/* NOTREACHED */
	default:
	    return 0;
    }
}

void mod_hello_create_obj(player, obj)
	dbref player, obj;
{
	notify(player, tprintf("You created #%d -- hello says so.", obj));
}

void mod_hello_destroy_obj(player, obj)
	dbref player, obj;
{
	notify(GOD, tprintf("Destroyed #%d -- hello says so.", obj));
}

void mod_hello_destroy_player(player, victim)
	dbref player, victim;
{
	notify(player, tprintf("Say goodbye to %s!", Name(victim)));
}

void mod_hello_announce_connect(player, reason, num)
	dbref player;
	const char *reason;
	int num;
{
	notify(GOD, tprintf("%s(#%d) just connected -- hello says so.",
				Name(player), player));
}

void mod_hello_announce_disconnect(player, reason, num)
	dbref player;
	const char *reason;
	int num;
{
	notify(GOD, tprintf("%s(#%d) just disconnected -- hello says so.",
				Name(player), player));
}

void mod_hello_examine(player, cause, thing, control, key)
    dbref player, cause, thing;
    int control, key;
{
    if (control) {
	notify(player,
	       tprintf("Greeted: %d  Foofed: %d",
		       mod_hello_db[thing].greeted,
		       mod_hello_db[thing].foofed));
    }
}

/* --------------------------------------------------------------------------
 * Commands.
 */

DO_CMD_NO_ARG(mod_hello_do_hello)
{
    int i;


    if (key & MOD_HELLO_HELLO_INFORMAL) {
	for (i = 0; i < mod_hello_config.hello_times; i++) 
	    notify(player, "Hi there!");
    } else {
	if (mod_hello_config.show_name)
	    notify(player, tprintf("Hello, %s!", Name(player)));
	else
	    notify(player, mod_hello_config.hello_string);
    }

    mod_hello_db[player].greeted += 1;
    notify(player, tprintf("You have been greeted %d times.", 
			   mod_hello_db[player].greeted));
}

DO_CMD_ONE_ARG(mod_hello_do_foof)
{
    DBData dbkey;
    DBData data;

    /* Demonstrate what we can do:
     *   @foof greets you with a generic message.
     *
     *   @foof <message> greets you with a customized message that is
     *	 preserved in the database.
     *
     *   @foof/show will show you the message you were last 'foofed' with.
     *   This illustrates the use of the database cache.
     */

    /* Set up our DB key. We'll either be getting, adding or deleting an
     * entry */
    
    dbkey.dptr = &player;
    dbkey.dsize = sizeof(dbref);
    
    if (key & MOD_HELLO_FOOF_SHOW) {
    	data = cache_get(dbkey, mod_hello_dbtype);
    	if (data.dptr) {
    		notify(player, tprintf("You were last foofed with: %s", data.dptr));
    	} else {
    		notify(player, "You have not been foofed with a message.");
    	}
    	return;
    }
    
    if (arg1 && *arg1) {
    	notify(player, tprintf("Yay: \"%s\"", arg1));
    	
    	/* Set up data and store it in cache */
    	
    	data.dptr = XSTRDUP(arg1, "mod_hello_do_foof");
    	data.dsize = strlen(arg1) + 1;
    	cache_put(dbkey, data, mod_hello_dbtype);
    } else {
	notify(player, "Yay.");
	
	/* Delete the entry from cache if it exists */
	
	cache_del(dbkey, mod_hello_dbtype);
    }
    
    mod_hello_db[player].foofed += 1;
    notify(player, tprintf("You have been foofed %d times.", 
			   mod_hello_db[player].foofed));
}

NAMETAB mod_hello_hello_sw[] = {
{(char *)"informal",	1,	CA_PUBLIC,	MOD_HELLO_HELLO_INFORMAL},
{ NULL,			0,	0,		0}};

NAMETAB mod_hello_foof_sw[] = {
{(char *)"show",	1,	CA_PUBLIC,	MOD_HELLO_FOOF_SHOW},
{ NULL,			0,	0,		0}};

CMDENT mod_hello_cmdtable[] = {
{(char *)"@hello",		mod_hello_hello_sw,	CA_PUBLIC,
	0,		CS_NO_ARGS,
	NULL,		NULL,	NULL,			{mod_hello_do_hello}},
{(char *)"@foof",		mod_hello_foof_sw,	CA_PUBLIC,
	0,		CS_ONE_ARG,
	NULL,		NULL,	NULL,			{mod_hello_do_foof}},
{(char *)NULL,			NULL,		0,
	0,		0,
	NULL,		NULL,	NULL,		{NULL}}};

/* --------------------------------------------------------------------------
 * Functions.
 */

FUNCTION(mod_hello_fun_hello)
{
    safe_str("Hello, world!", buff, bufc);
}

FUNCTION(mod_hello_fun_hi)
{
    /* Normally we would not call our own API, but here's just an
     * example.
     */

    static void (*handler)(void *, void *) = NULL;
    HI_INPUT in_info;
    HI_OUTPUT out_info;

    if (!handler)
	handler = request_api_function("hi", "print_greeting");

    if (!handler) {
	safe_str("#-1 API FUNCTION MISSING", buff, bufc);
	return;
    }

    in_info.player = player;
    in_info.name = fargs[0];
    handler(&in_info, &out_info);

    if (out_info.success_code == 0)
	safe_str("#-1 NO SUCH PLAYER", buff, bufc);
}


FUN mod_hello_functable[] = {
{"HELLO",	mod_hello_fun_hello,	0,	0,	CA_PUBLIC,	NULL},
{"HI",		mod_hello_fun_hi,	1,	0,	CA_PUBLIC,	NULL},
{NULL,		NULL,			0,	0,	0,		NULL}};

/* --------------------------------------------------------------------------
 * Hash tables.
 * (We don't use any of this data. It's just here for demo purposes.)
 */

HASHTAB mod_hello_greetings;
HASHTAB mod_hello_farewells;

MODHASHES mod_hello_hashtable[] = {
{ "Hello greetings",	&mod_hello_greetings,	5,	8},
{ "Hello farewells",	&mod_hello_farewells,	15,	32},
{ NULL,			NULL,			0,	0}};

NHSHTAB mod_hello_numbers;

MODNHASHES mod_hello_nhashtable[] = {
{ "Hello numbers",	&mod_hello_numbers,	5,	16},
{ NULL,			NULL,			0,	0}};

/* --------------------------------------------------------------------------
 * Initialization.
 */
	
void mod_hello_init()
{
    /* Give our configuration some default values. */

    mod_hello_config.show_name = 0;
    mod_hello_config.hello_string = XSTRDUP("Hello, world!", "mod_hello_init");
    mod_hello_config.hello_times = 1;

    /* Register everything we have to register. */

    register_hashtables(mod_hello_hashtable, mod_hello_nhashtable); 
    register_commands(mod_hello_cmdtable);
    register_functions(mod_hello_functable);
    register_api("hello", "hi", mod_hello_exports);
}

void mod_hello_cleanup_startup()
{
    mod_hello_dbtype = register_dbtype("hello");
}

/* --------------------------------------------------------------------------
 * Database routines: read and write a flatfile at db conversion time.
 */

void mod_hello_db_write_flatfile(f)
FILE *f;
{
	unsigned int dbtype;
	dbref thing;
	DBData key;
	DBData data;
	
	/* Find out our dbtype */

	mod_hello_dbtype = register_dbtype("hello");
		
	/* Write out our version number */
	
	fprintf(f, "+V1\n");
	
	DO_WHOLE_DB(thing) {
		key.dptr = &thing;
		key.dsize = sizeof(dbref);
		data = cache_get(key, mod_hello_dbtype);
		if (data.dptr) {
			fprintf(f, "!%d\n", thing);
			putstring(f, data.dptr);
		}
	}
	
	fputs("***END OF DUMP***\n", f);
}

void mod_hello_db_read_flatfile(f)
FILE *f;
{
	unsigned int dbtype, version;
	char c;
	dbref thing;
	DBData key;
	DBData data;
	
	/* Find out our dbtype */

	mod_hello_dbtype = register_dbtype("hello");

	/* Load entries */
	
	while(1) {
		switch(c = getc(f)) {
		case '+':	/* Header */
			switch(c = getc(f)) {
				case 'V':	/* Version number */
					version = getref(f);
					break;
				default:
					(void)getstring_noalloc(f, 1);
				}
				break;
		case '!':	/* DBref */
			thing = getref(f);
			
			/* Grab the string and put it in cache */
			
			data.dptr = XSTRDUP(getstring_noalloc(f, 1), "mod_hello_db_read_flatfile");
			data.dsize = strlen(data.dptr) + 1;
			key.dptr = &thing;
			key.dsize = sizeof(dbref);
			
			cache_put(key, data, mod_hello_dbtype);
			break;
		case '*':	/* EOF marker */
			return;
		}
	}
}	
