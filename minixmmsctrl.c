/* -*- mode: C; c-basic-offset: 2; -*- */
/* xmmsctrl : simple small program
 * to control xmms from the command line
 * xmms provides some basic control command
 * but nothing to control the sound volume,
 * thus this program.
 * 
 * author: Alexandre David
 * e-mail: adavid@cs.aau.dk
 * Old web page: http://user.it.uu.se/~adavid/utils/
 * license: GPL
 *
 * See the Changelog file for the change log.
 * --- END OLD XMMSCTRL SHIT ---
 * MiniXMMSctrl: A version of xmmsctrl which only
 * implements those functions found on the multi- 
 * media keys on one's keyboard.
 * author: Segin
 * email: segin2005@gmail.com
 * web plage: http://segin/no-ip.org/mxc

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Publice License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <xmms/xmmsctrl.h>  /* provided by xmms-devel */

#ifdef PRETTY_PRINT
#define COLOR  "\033[1;34m"
#define NORMAL "\033[0;0m"
#else
#define COLOR  ""
#define NORMAL ""
#endif

/* returns true if xmms is playing a stream */
static gboolean is_stream_playing(gint);

/* simple commands, argument is the session number */
static void print_help(gint);          /* print help   */
static void print_volume(gint);        /* print volume */

/* commands needing an argument, arguments are session number and the read string argument */
static void set_vol(gint,char*);       /* set the volume */

/* type for conditional print */
typedef struct {
  char *eq;
  char *neq;
} CondString;

CondString *condStrings = NULL;
int nbCondStrings = 0;
int nbEq = 0;
int nbNEq = 0;


/* type for simple commands */
typedef struct {
  const char *name;               /* command line argument function */
  void (*command)(gint);          /* xmms API function to use       */
  const char *help;               /* help for this command          */
} Command;

static gint launch_xmms(void);
static void print_version(void);


/* simple command list */
Command com[]={
  {
    "eject" ,
    xmms_remote_eject ,
    "open xmms \"Load file(s)\" dialog window."
  },
  {
    "help",
    print_help ,
    "print this help message."
  },
  {
    "next" , 
    xmms_remote_playlist_next ,
    "xmms next song command, go to the next song."
  },
  {
    "pause" ,
    xmms_remote_pause ,
    "xmms pause command, pause the playing song."
  },
  {
    "play" ,
    xmms_remote_play ,
    "xmms play command, play the current song."
  },
  {
    "prev" ,
    xmms_remote_playlist_prev ,
    "xmms previous song command, go to the previous song."
  },
  {
    "stop" ,
    xmms_remote_stop ,
    "xmms stop command, stop playing."
  },
  {
    "quit", 
    xmms_remote_quit ,
    "terminate xmms."
  },
  {
    "--help",
    print_help ,
    "print this help message."
  },
  {
    "start",
    launch_xmms,
    "starts xmms."
  },
  {
    "--version",
    print_version,
    "shows version and exits."
  }
};


/* type for test commands */
typedef struct {
  const char *name;
  gboolean (*test)(gint);
  const char *help;
} Test;


/* test command list */
Test test[]={
  {
    "paused" , 
    xmms_remote_is_paused ,
    "returns OK if xmms is paused."
  },
  {
    "playing" , 
    xmms_remote_is_playing ,
    "returns OK if xmms is playing a song."
  },
  {
    "is_main" , 
    xmms_remote_is_main_win ,
    "returns OK if xmms has its main window open."
  },
  {
    "is_stream" ,
    is_stream_playing ,
    "returns OK if xmms is playing a stream (http://somewhere)."
  },
  {
    "running" , 
    xmms_remote_is_running ,
    "returns OK if xmms is running."
  }
};


/* type for toggle commands */
typedef struct {
  const char *name;
  void (*command)(gint,gboolean);
  const char *help;
} ToggleCommand;


/* toggle command list */
ToggleCommand toggle[]={
  {
    "main" , 
    xmms_remote_main_win_toggle ,
    "hide/show xmms main window."
  }
};


/* type for commands wanting an argument */
typedef struct {
  const char *name;
  void (*command)(gint, char*);
  const char *help;
} ArgCommand;


/* one-argument-command list */
ArgCommand argcom[]={
  {
    "vol" ,
    set_vol ,
    COLOR"vol [+|-]percent"NORMAL", with the following effects\n"
    "\t percent : set the volume to percent,\n"
    "\t+percent : increase the volume with percent,\n"
    "\t-percent : decrease the volume with percent.\n"
    "    Examples : xmmsctrl vol 40, xmmsctrl vol +5, xmmsctrl vol -5."
  }
};


/* sizes of the lists */
#define NCOM (sizeof(com)/sizeof(Command))
#define NTST (sizeof(test)/sizeof(Test))
#define NTOG (sizeof(toggle)/sizeof(ToggleCommand))
#define NARG (sizeof(argcom)/sizeof(ArgCommand))


/*
 * displays version number and exits 
 */
static void print_version(void)
{
	puts("MiniXMMSctrl 0.1 -- A micro-remote for XMMS.\n"
	     "Written by Segin, and based on XMMSCTRL.\n");
	exit(0);
}


/*
 * returns OK if xmms is playing a stream
 */
static gboolean is_stream_playing(gint session) {
  return !strncmp("http://",
		  xmms_remote_get_playlist_file(session,
						 xmms_remote_get_playlist_pos(session)),
		  7);
}


/*
 * go to previous song, wrap to the last song if the position
 * in the play list is the first. Note that in case of CD,
 * the first position is 1 otherwise it is 0.
 */
static void play_prev (gint session) {
  const gchar DEV[] = "/dev/";
  gint first = 0;

  if (!strncmp(DEV,
	       xmms_remote_get_playlist_file(session,
					     xmms_remote_get_playlist_pos(session)),
	       5)) {
    /*puts ("It appears you're playing a CD");*/
    first = 1;
  }

  if (first != xmms_remote_get_playlist_pos (session))
    xmms_remote_playlist_prev(session);
  else
    xmms_remote_set_playlist_pos(session, xmms_remote_get_playlist_length(session) - 1);
}

/*
 * vol command: needs number | +number | -number
 */
static void set_vol(gint session, char *arg) {
  gint vol = xmms_remote_get_main_volume( session );

  switch( arg[0] ) {

  case '+': /* argument is positive */
  case '-': /* argument is negative */
    /* no test on the validity of the argument,
     * not critical: in the worst case 0 is returned */
    vol += atoi(arg);
    break;

  default:
    vol = atoi(arg);
  }

  /* check bounds */
  if (vol<0)
    vol = 0;
  else if (vol>100)
    vol = 100;
  
  xmms_remote_set_main_volume( session, vol );
}


/*
 * time command: needs number | +number | -number | /number
 */
static void set_time(gint session, char *arg) {
  gint ptime = xmms_remote_get_output_time( session );
  gint pos = xmms_remote_get_playlist_pos( session );
  gint length = xmms_remote_get_playlist_time( session, pos );
	
  switch( arg[0] ) {

  case '+': /* argument is positive */
  case '-': /* argument is negative */
    /* no test on the validity of the argument,
     * not critical: in the worst case 0 is returned */
    ptime += atoi(arg) * 1000;
    break;

  case '/':
    /* if arg[i] is a recognized string argument, by
     * definition it has at least one character, therefore
     * argv[i]+1 is at worst \0 and atoi returns in the
     * worst case 0 */
    ptime = length - atoi(arg+1) * 1000;
    break;

  default:
    ptime = atoi(arg) * 1000;
  }

  /* check bounds */
  if ( ptime < 0 )
    ptime = 0;
  else if ( ptime > length )
    ptime = length;
  
  xmms_remote_jump_to_time( session, ptime );
}



/*
 * ensure capacity of the string table
 */
static void ensure_capa(int i) {
  if (i >= nbCondStrings) {
    condStrings = (CondString*) realloc(condStrings, (i+10)*sizeof(CondString));
    memset(&condStrings[i], 0, (i+10-nbCondStrings)*sizeof(CondString));
    nbCondStrings = i+10;
  }
}



/*
 * print xmmsctrl help
 * the dummy variable is used just for convenience
 */
static void print_help(gint dummy) {
  unsigned int i;

  /* The string is cut to conform to ISO C89 */
  puts(COLOR"MiniXMMSctrl version "VERSION NORMAL" (C) Segin <segin2005@gmail.com>.\n"
       "'minixmmsctrl' is a simple tool designed to be used at the shell level,\n"
       "typically in a small shell script associated to a keyboard shortcut. There\n"
       "are 4 different command types:\n"
       "- simple commands, e.g. \"xmmsctrl play\", which perform a simple task,\n"
       "- commands with a flag argument, e.g. \"xmmsctrl main 1\", which set\n"
       "  a particular state,");
  puts("- condition testing, e.g. \"xmmsctrl playing\", which can be used in\n"
       "  if statements in shells. Something to notice: this was designed to be\n"
       "  used simply, which is, directly in if statements: if <command>; then\n"
       "  <command>; else <command>; fi. There you put directly \"xmmsctrl playing\"\n"
       "  to test if xmms is playing. Notice how the if statement works: if the\n"
       "  command succeeds, it returns a 0, which means OK, otherwise it returns\n"
       "  an error code,\n"
       "- more specific commands with particular arguments.");

  /** simple commands, 2 special and the rest from the list */
  puts("\n"
       "The simple commands are:\n"
       " "COLOR"launch"NORMAL" : launch a xmms instance if none is running\n"
       " "COLOR"not"NORMAL" : negate the next condition test");
  for ( i = 0 ; i < NCOM ; i++ )
    printf(" "COLOR"%s"NORMAL" : %s\n", com[i].name, com[i].help);

  /** toggle commands from the list **/
  puts("\n"
       "The flag setting commands are used with 0 or 1:");
  for ( i = 0 ; i< NTOG ; i++ )
    printf(" "COLOR"%s"NORMAL" : %s\n", toggle[i].name, toggle[i].help);

  /** test commands from the list **/
  puts("\n"
       "The condition testing commands are:");
  for ( i = 0 ; i < NTST ; i++ )
    printf(" "COLOR"%s"NORMAL" : %s\n", test[i].name, test[i].help);

  /** argument commands, one special and the rest from the list **/
  puts("\n"
       "The other specific commands are:\n"
       " "COLOR"session number"NORMAL" : use the session number 'number', xmmsctrl looks\n"
       "                  automatically for the first working session.\n");
  for ( i = 0 ; i < NARG ; i++ )
    printf(" %s\n\n", argcom[i].help); /* special custom format here */

  /** batch mode */
  printf("Except for 'session', these command now support a\n"
	 "batch mode. You can give a list of arguments beginning\n"
	 "with BEGIN and ending with END, e.g.,\n"
	 " \txmmsctrl +file BEGIN dir1 dir2 song1 song2 END\n");

  /** examples **/
  puts("\n"
       "Examples of shell scripts to define simple functions:\n"
       " Play/Stop :\n"
       " \tsh -c \"if xmmsctrl playing;\\\n"
       " \t       then xmmsctrl stop;\\\n"
       " \t       else xmmsctrl play; fi\"\n"
       " Play/Pause :\n"
       " \tsh -c \"if xmmsctrl playing;\\\n"
       " \t       then xmmsctrl pause;\\\n"
       " \t       else xmmsctrl play; fi\"\n"
       "(with xmmsctrl in your path). See more examples in the bindings\n"
       "for twm in the distribution.\n"
       "Have fun. Alexandre");
}


/*
 * launch a new xmms and return the session number
 * exit if error
 */
static gint launch_xmms(void) {
  gint session;
  unsigned int tries;

  switch( fork() ) {

  case -1:
    perror("fork");
    exit(1);

  case 0:
    execlp("xmms", "xmms", NULL);
    fprintf(stderr, "xmms not found!\n");
    exit(1);

  default:
    for( tries = 0 ; tries < 10 ; tries++ ) {
      usleep( 500000 ); /* in usec */
      for( session = 0 ; session < 16 ; session++ )
	if ( xmms_remote_is_running( session ) )
	  return session;
    }
    exit(1); /* if no session found, abort */
  }
}


int main(int argc, char *argv[]) {
  int i = 1;
  unsigned int negate = 0;
  gint session;

  if ( argc == 1 ) {
    print_help(0);
    return 1;
  }

  /* try to find automatically xmms session. */
  for( session = 0 ; session < 16 ; session++ )
    if ( xmms_remote_is_running( session ) )
      break;

  if (session == 16) {  /* no session found     */
    if (strcmp( argv[1], "launch" )) {
      return 1;         /* error return = false */
    } else {
      i++;
      session = launch_xmms();
    }
  }

  for( ; i < argc ; i++ ) {

    /* special command tests first */
    /* use a given session number */
    if ( !strcmp( argv[i], "session" ) ) {

       /* if argument left */
      if ( ++i < argc )
        session = atoi( argv[i] );
      else /* no argument left */
        fprintf(stderr, "Command usage: session number\n");

    }
    /* negation handling */
    else if ( !strcmp( argv[i], "not" ) ) {
      negate ^= 1;
    /* handle generic commands if the command is not launch */
    } else if ( strcmp( argv[i], "launch" ) ) {
      unsigned int j;
      /* I don't need this, but this improves readability since
       * I avoid 3 nested if statements with it */
      int matched = 0;

      /* test functions */
      for( j = 0 ; j < NTST ; j++ )
        if ( !strcmp( argv[i], test[j].name ) )
          return (!test[j].test( session ))^negate;

      /* simple commands, we are here only if no test was found */
      for( j = 0 ; j < NCOM ; j++ )
        if ( !strcmp( argv[i], com[j].name ) ) {

          com[j].command( session );

	  matched = 1;
          break; /* command matched -> stop for loop */
        }

      if ( !matched )
	/* toggle commands */
        for( j = 0 ; j < NTOG ; j++ ) {
          if ( !strcmp( argv[i], toggle[j].name ) ) {

            if ( ++i < argc ) /* if argument left */
              toggle[j].command( session, atoi( argv[i] ) );
            else
              fprintf(stderr, "Command usage: %s 0|1\n",
                      toggle[j].name);

	    matched = 1;
            break; /* command matched -> stop for loop */
          }
	}

      if ( !matched )
	/* argument commands */
	for( j = 0 ; j < NARG ; j++ ) {
	  if ( !strcmp( argv[i], argcom[j].name ) ) {

	    if ( ++i < argc ) { /* if argument left */
	      if (!strcmp(argv[i], "BEGIN")) {
		/* enter batch mode */
		while (++i < argc && strcmp(argv[i], "END"))
		  argcom[j].command( session, argv[i] );
	      } else {
		argcom[j].command( session, argv[i] ); /* i < argc here implies argv[i] != NULL */
	      }
	    } else {
	      fprintf(stderr, "Command %s needs an argument. Usage:\n%s\n",
		      argcom[j].name, argcom[j].help);
	    }

	    matched = 1;
	    break;
	  }
	}
      
      if ( !matched )
	  fprintf(stderr, "Invalid command '%s' is ignored\n", argv[i] );
    }
  }
  
  if (condStrings) free(condStrings);

  return 0; /* OK result */
}
