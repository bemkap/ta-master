#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

extern char **environ;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd (struct tokens *tokens);
int cmd_cd  (struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?"   , "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd , "pwd" , "show current directory"},
  {cmd_cd  , "cd"  , "change current directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(struct tokens *tokens) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(struct tokens *tokens) {
  exit(0);
}

int cmd_pwd(struct tokens *tokens) {
  char curdir[1024];
  if (getcwd(curdir, 1024)) {
    printf("%s\n", curdir);
    return 1;
  } else {
    return -1;
  }
}

int cmd_cd(struct tokens *tokens) {
  chdir(tokens_get_token(tokens,1));
  return 1;
}  

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int iscolon(int c) {
  return c == ':';
}

int takargs(int c) {
  return c != '<' && c != '>';
}

int main(int argc, char *argv[]) {
  init_shell();

  static char line[4096];
  static char *args[16];
  int i, line_num = 0;

  char *ps=getenv("PATH");

  struct tokens *path_tokens = NULL;
  if (ps != NULL) path_tokens = tokenize(ps, iscolon);
  
  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%04d: ", line_num);

  chdir("./");

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line, isspace);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      int pid, status;
      pid = fork();
      if (pid > 0) {
	wait(&status);
      } else if (pid == 0) {
	for (i = 0; i < tokens_get_length(tokens) && takargs(*tokens_get_token(tokens, i)); ++i)
	  args[i] = tokens_get_token(tokens, i);
	args[i] = NULL;
	for (; i < tokens_get_length(tokens); ++i)
	  switch (*tokens_get_token(tokens, i)) {
	  case '>':
	    if (tokens_get_length(tokens) > i+1)
	      freopen(tokens_get_token(tokens, ++i), "w", stdout);
	    else {
	      perror("Output redirect error");
	      return -1;
	    }
	    break;
	  case '<':
	    if (tokens_get_length(tokens) > i+1)
	      freopen(tokens_get_token(tokens, ++i), "r", stdin);
	    else {
	      perror("Input redirect error");
	      return -1;
	    }
	    break;
	  }
	execv(tokens_get_token(tokens, 0), args);
	if (path_tokens != NULL)
	  for (i = 0; i < tokens_get_length(path_tokens); ++i) {
	    char cmd[1024];
	    strcpy(cmd, tokens_get_token(path_tokens, i));
	    strcat(cmd, "/");
	    strcat(cmd, tokens_get_token(tokens, 0));
	    execv(cmd, args);
	  }
	printf("Command not found\n");
	exit(0);
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%04d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
