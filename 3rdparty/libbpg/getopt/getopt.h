#pragma once

#include <tchar.h>


/*
 *  getopt.h
 *  
 *  Copyright (C) 1998, 2009, 2013
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 *
 *  Description:
 *      This file defines the 'getopt()' function found on UNIX systems.
 *      This version was written to facilitate porting UNIX-based software
 *      to other platforms which do not contain this function, including
 *      Windows NT.
 *
 *  Portability Issues:
 *      None.
 *
 *  Caveats:
 *      As with the UNIX version of this routine, this function is not
 *      thread-safe, nor should it ever be called once it returns EOF.
 */

/*
 *  Define the function prototype only.
 */
int getopt( int             argc,
            TCHAR * const *argv,
            const TCHAR   *option_string);


extern TCHAR *optarg;
extern int     optind;



struct option		/* specification for a long form option...	*/
{
  const char *name;		/* option name, without leading hyphens */
  int         has_arg;		/* does it take an argument?		*/
  int        *flag;		/* where to save its status, or NULL	*/
  int         val;		/* its associated status value		*/
};


enum    		/* permitted values for its `has_arg' field...	*/
{
  no_argument = 0,      	/* option never takes an argument	*/
  required_argument,		/* option always requires an argument	*/
  optional_argument		/* option may take an argument		*/
};



//extern int getopt_long_only(int nargc, char * const *nargv, const char *options,
  //  const struct option *long_options, int *idx);


