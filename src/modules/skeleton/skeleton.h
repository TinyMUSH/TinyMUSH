/**
 * @file skeleton.h
 * @brief Skeleton module header - use as template for new modules
 * @version 1.0
 */

#ifndef __MOD_SKELETON_H
#define __MOD_SKELETON_H

/* ============================================================================
 * MODULE CONFIGURATION STRUCTURE
 * ============================================================================
 * This structure holds all configuration parameters for the skeleton module.
 * Add your custom parameters here and reference them in the config table.
 */

struct mod_skeleton_confstorage
{
    int enabled;                    /* Is the module enabled? */
};

/* External configuration storage */
extern struct mod_skeleton_confstorage mod_skeleton_config;

/* ============================================================================
 * HASH TABLE DECLARATIONS
 * ============================================================================
 * Define any hash tables your module uses for fast lookups.
 */

extern HASHTAB mod_skeleton_data_htab;

/* ============================================================================
 * MODULE METADATA
 * ============================================================================
 */

extern MODVER mod_skeleton_version;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 * Declare all module functions here. Remember to use mod_skeleton_ prefix.
 */

void mod_skeleton_init(void);
void mod_skeleton_cleanup(void);
void mod_do_skeleton_command(dbref, dbref, int, char *);
void mod_do_skeleton_function(char *, char **, dbref, dbref, dbref, char *[], int, char *[], int ncargs);

int *mod_skeleton_get_data(char *);
CF_Result mod_skeleton_set_data(char *, int *);
bool mod_skeleton_delete_data(char *);
void mod_skeleton_list_data(dbref);
void mod_skeleton_reset_defaults(void);

#endif /* __MOD_SKELETON_H */
