/* commands.h - the felight subcommands; each returns 0 or an exit code. */
#ifndef COMMANDS_H
#define COMMANDS_H

int cmd_list(int argc, char **argv);
int cmd_get(int argc, char **argv);
int cmd_set(int argc, char **argv);
int cmd_save(int argc, char **argv);
int cmd_apply(int argc, char **argv);
int cmd_version(int argc, char **argv);

#endif
