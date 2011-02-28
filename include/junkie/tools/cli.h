// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#ifndef CLI_H_110124
#define CLI_H_110124
#include <stdbool.h>
#include <junkie/cpp.h>

/** @file
 * @brief Simple replacement for getopt
 *
 * We want to be able to add dynamically new option blocs available from command line so
 * that newly loaded plugins can have command line options too.
 */

/** @file
 * @brief Simple replacement for getopt
 *
 * We want to be able to add dynamically new option blocs available from command line so
 * that newly loaded plugins can have command line options too.
 */

struct cli_opt {
    char const *arg[2];
    bool need_argument;
    char const *help;
    enum cli_action { CLI_CALL, CLI_SET_UINT, CLI_SET_BOOL, CLI_DUP_STR } action;
    union {
        int (*call)(char const *option);   // return 0 or error code
        unsigned *uint;
        bool *boolean;
        char **str;
    } u;
};

/// Register a new bloc of options (under the heading /name/).
/** @param name the title under which these options are presented if not NULL
 * @param opt the cli_opt array to add
 * @param nb_cli_opts size of the previous array
 */
int cli_register(char const *name, struct cli_opt *opt, unsigned nb_cli_opts);

/// Unregister a bloc of options
int cli_unregister(struct cli_opt *);

int cli_parse(unsigned nb_args, char **args);
int cli_2_enum(bool case_sensitive, char const *value, ...) sentinel_;

void cli_init(void);
void cli_fini(void);

#endif