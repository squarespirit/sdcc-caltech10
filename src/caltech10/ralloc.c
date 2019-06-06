/*------------------------------------------------------------------------

  SDCCralloc.c - source file for register allocation. 68HC08 specific

                Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/

#include "common.h"
#include "ralloc.h"
#include "gen.h"
#include "dbuf_string.h"

/*-----------------------------------------------------------------*/
/* assignRegisters - assigns registers to each live range as need  */
/* Caltech10 has no registers                                      */
/*-----------------------------------------------------------------*/
void caltech10_assignRegisters (ebbIndex * ebbi)
{
  if (options.dump_i_code) {
    dumpEbbsToFileExt (DUMP_PACK, ebbi);
  }

  if (options.dump_i_code)
  {
    dumpEbbsToFileExt (DUMP_RASSGN, ebbi);
    dumpLiveRanges (DUMP_LRANGE, liveRanges);
  }

  // Convert basic blocks to icode list
  eBBlock ** ebbs = ebbi->bbOrder;
  int count = ebbi->count;
  iCode *ic = iCodeLabelOptimize(iCodeFromeBBlock(ebbs, count));

  genCaltech10Code(ic);
}

