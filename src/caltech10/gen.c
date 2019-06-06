/*-------------------------------------------------------------------------
  gen.c - source file for code generation for the Caltech10
-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "ralloc.h"
#include "gen.h"
#include "dbuf_string.h"

/** 1 if result is rematerializable */
static int resultRemat(iCode * ic) {
  if (SKIP_IC (ic) || ic->op == IFX)
    return 0;

  if (IC_RESULT (ic) && IS_ITEMP (IC_RESULT (ic))) {
    symbol *sym = OP_SYMBOL (IC_RESULT (ic));
    if (sym->remat && !POINTER_SET (ic))
      return 1;
  }
  return 0;
}


/*****************************************************************************/
/* asmop helpers                                                             */
/*****************************************************************************/

#define AOP(op) op->aop
#define AOP_TYPE(op) AOP(op)->type
#define AOP_SIZE(op) AOP(op)->size
#define AOP_OP(aop) aop->op

/** Create new asmop */
static asmop *newAsmop (short type) {
  asmop *aop = Safe_calloc (1, sizeof (asmop));
  aop->type = type;
  aop->op = NULL;
  return aop;
}


/** Get asmop of a true symbol */
static asmop *aopForSym (iCode * ic, symbol * sym, bool result) {
  asmop *aop;
  memmap *space;

  wassertl (ic != NULL, "Got a null iCode");
  wassertl (sym != NULL, "Got a null symbol");

//  printf("in aopForSym for symbol %s\n", sym->name);

  space = SPEC_OCLS (sym->etype);

  /* if already has one */
  if (sym->aop)
    return sym->aop;

  /* special case for a function */
  if (IS_FUNC (sym->type)) {
    sym->aop = aop = newAsmop(AOP_IMMD);
    aop->aopu.aop_immd.aop_immd1 = Safe_calloc (1, strlen (sym->rname) + 1);
    strcpy (aop->aopu.aop_immd.aop_immd1, sym->rname);
    aop->size = FARPTRSIZE;
    return aop;
  }

  /* if it is on the stack */
  if (sym->onStack) {
    sym->aop = aop = newAsmop(AOP_SOF);
    aop->aopu.aop_dir = sym->rname;
    aop->size = getSize (sym->type);
    aop->aopu.aop_stk = sym->stack;
    return aop;
  }

  /* if it is in direct space */
  if (IN_DIRSPACE(space)) {
    sym->aop = aop = newAsmop (AOP_DIR);
    aop->aopu.aop_dir = sym->rname;
    aop->size = getSize (sym->type);
    return aop;
  }

  /* default to far space */
  sym->aop = aop = newAsmop (AOP_EXT);
  aop->aopu.aop_dir = sym->rname;
  aop->size = getSize (sym->type);
  return aop;
}


/** Allocate asmop for operand */
static void aopOp (operand *op, iCode * ic, bool result) {
  asmop *aop = NULL;
  symbol *sym;

  if (!op)
    return;

//  printf("checking literal\n");
  /* if this a literal */
  if (IS_OP_LITERAL (op)) {
    op->aop = aop = newAsmop (AOP_LIT);
    aop->aopu.aop_lit = OP_VALUE (op);
    aop->size = getSize (operandType (op));
    aop->op = op;
    return;
  }

//  printf("checking pre-existing\n");
  /* if already has a asmop then continue */
  if (op->aop) {
    op->aop->op = op;
    return;
  }

//  printf("checking underlying sym\n");
  /* if the underlying symbol has a aop */
  if (IS_SYMOP (op) && OP_SYMBOL (op)->aop) {
    op->aop = aop = Safe_calloc (1, sizeof (*aop));
    memcpy (aop, OP_SYMBOL (op)->aop, sizeof (*aop));
    //op->aop = aop = OP_SYMBOL (op)->aop;
    aop->size = getSize (operandType (op));
    //printf ("reusing underlying symbol %s\n",OP_SYMBOL (op)->name);
    //printf (" with size = %d\n", aop->size);

    aop->op = op;
    return;
  }

//  printf("checking true sym\n");
  /* if this is a true symbol */
  if (IS_TRUE_SYMOP (op)) {
    op->aop = aop = aopForSym (ic, OP_SYMBOL (op), result);
    aop->op = op;
    //printf ("new symbol %s\n", OP_SYMBOL (op)->name);
    //printf (" with size = %d\n", aop->size);
    return;
  }

  /* this is a temporary : this has
     only five choices :
     a) register
     b) spillocation
     c) rematerialize
     d) conditional
     e) can be a return use only */

  if (!IS_SYMOP(op))
    piCode(ic, NULL);
  sym = OP_SYMBOL(op);

//  printf("checking conditional\n");
  /* if the type is a conditional */
  // if (sym->regType == REG_CND)
  //   {
  //     sym->aop = op->aop = aop = newAsmop (AOP_CRY);
  //     aop->size = 0;
  //     aop->op = op;
  //     return;
  //   }
  // Check removed, because registers not supported

//  printf("checking spilt\n");
  /* if it is spilt then two situations
     a) is rematerialize
     b) has a spill location */
  if (sym->isspilt || sym->nRegs == 0)
    {
//      printf("checking remat\n");
      /* rematerialize it NOW */
      wassertl(sym->remat, "Rematerializing not supported");

      wassertl(!sym->ruonly, "sym->ruonly not supported");

      /* else spill location  */
      if (sym->isspilt && sym->usl.spillLoc)
        {
          asmop *oldAsmOp = NULL;

          if (sym->usl.spillLoc->aop && sym->usl.spillLoc->aop->size != getSize (sym->type))
            {
              /* force a new aop if sizes differ */
              oldAsmOp = sym->usl.spillLoc->aop;
              sym->usl.spillLoc->aop = NULL;
              //printf ("forcing new aop\n");
            }
          sym->aop = op->aop = aop = aopForSym (ic, sym->usl.spillLoc, result);
          if (sym->usl.spillLoc->aop->size != getSize (sym->type))
            {
              /* Don't reuse the new aop, go with the last one */
              sym->usl.spillLoc->aop = oldAsmOp;
            }
          aop->size = getSize (sym->type);
          aop->op = op;
          //printf ("spill symbol %s\n", OP_SYMBOL (op)->name);
          //printf (" with size = %d\n", aop->size);
          return;
        }

      /* else must be a dummy iTemp */
      sym->aop = op->aop = aop = newAsmop (AOP_DUMMY);
      aop->size = getSize (sym->type);
      aop->op = op;
      return;
    }

//  printf("assuming register\n");
  /* must be in a register */
  wassert(sym->nRegs);
  wassertl(0, "Register operand not supported");
}


/** Free an operand's asmop, or aaop itself, whichever is non-null. */
static void freeAsmop (operand * op, asmop * aaop, iCode * ic, bool pop) {
  asmop *aop;
  if (!op)
    aop = aaop;
  else
    aop = op->aop;

  if (!aop)
    return;

  if (aop->freed)
    goto dealloc;

  aop->freed = 1;

  if (aop->stacked)
    {
      wassertl(0, "Stacked aop not supported");
      // int stackAdjust;
      // int loffset;
      // aop->stacked = 0;
      // stackAdjust = 0;
      // for (loffset = 0; loffset < aop->size; loffset++)
      //   if (aop->stk_aop[loffset])
      //     {
      //       transferAopAop (aop->stk_aop[loffset], 0, aop, loffset);
      //       stackAdjust++;
      //     }
      // pullNull (stackAdjust);
    }

dealloc:
  /* all other cases just dealloc */
  if (op)
    {
      op->aop = NULL;
      if (IS_SYMOP (op))
        {
          OP_SYMBOL (op)->aop = NULL;
          /* if the symbol has a spill */
          if (SPIL_LOC (op))
            SPIL_LOC (op)->aop = NULL;
        }
    }
}

/*****************************************************************************/
/* Generate code for specific operations                                     */
/*****************************************************************************/

/** Load from aop */
static void loadFromAop(asmop *aop) {
  wassert(aop->size == 1);
  if (aop->type == AOP_LIT) {
    emitcode("LDI", "%02x", byteOfVal(aop->aopu.aop_lit, 0));
    return;
  }
  if (aop->type == AOP_DIR) {
    emitcode("LDD", "$%s", aop->aopu.aop_dir);
    return;
  }
  // Support literal and direct only
  wassertl(0, "Unsupported aop type");
}

/** Store to aop */
static void storeToAop(asmop *aop) {
  wassert(aop->size == 1);
  if (aop->type == AOP_DIR) {
    emitcode("STD", "$%s", aop->aopu.aop_dir);
    return;
  }
  // Support storing to direct only
  wassertl(0, "Unsupported aop type");
}

/** Generate code that copies from one operand to another */
static void genCopy(operand *result, operand *source) {
  int size = AOP_SIZE(result);
  int srcsize = AOP_SIZE(source);
  wassert(size == 1); // No support for big types yet
  wassert(srcsize == 1);

  // Only support copying from these types.
  wassert(AOP_TYPE(source) == AOP_LIT || AOP_TYPE(source) == AOP_DIR);
  loadFromAop(AOP(source));
  // Only support copying to direct
  wassert(AOP_TYPE(result) == AOP_DIR);
  storeToAop(AOP(result));
}

/** Generate code for assign */
static void genAssign(iCode *ic) {
  operand *result = IC_RESULT(ic);
  operand *right = IC_RIGHT(ic);
  aopOp(right, ic, FALSE);
  aopOp(result, ic, TRUE);
  genCopy(result, right);
  freeAsmop(right, NULL, ic, TRUE);
  freeAsmop(result, NULL, ic, TRUE);
}


static void genLabel(iCode *ic) {
  // Special case, never generate
  if (IC_LABEL (ic) == entryLabel)
    return;
  emitLabel(IC_LABEL(ic));
}

static void genFunction(iCode *ic) {
}


static void genReturn(iCode *ic) {
}


static void genEndFunction(iCode *ic) {
}


/*****************************************************************************/
/* Overall code generation                                                   */
/*****************************************************************************/

/** Generate code for a single iCode */
void genCaltech10iCode(iCode *ic) {
  if (resultRemat(ic) || ic->generated) {
    wassertl(0, "Rematerializable icode");
  }

  initGenLineElement();
  genLine.lineElement.ic = ic;

  switch (ic->op) {
    case '=':
      if (POINTER_SET (ic))
        wassertl(0, "Pointer set not supported");
      genAssign(ic);
      break;
    case LABEL:
      genLabel(ic);
      break;
    case FUNCTION:
      genFunction(ic);
      break;
    case RETURN:
      genReturn(ic);
      break;
    case ENDFUNCTION:
      genEndFunction(ic);
      break;
    default:
      piCode(ic, NULL);
      wassertl(0, "unrecognized iCode");
  }
}

/** Generate code for list of iCodes */
void genCaltech10Code (iCode *lic)
{
  int cln = 0; // Current line #

  // Print the allocation information
  if (currFunc)
    printAllocInfo(currFunc, codeOutBuf);

  // Mark all ics as not generated
  for (iCode *ic = lic; ic; ic = ic->next)
    ic->generated = FALSE;

  for (iCode *ic = lic; ic; ic = ic->next) {
    initGenLineElement ();
    genLine.lineElement.ic = ic;
    if (ic->lineno && cln != ic->lineno) {
      if (!options.noCcodeInAsm) {
        emitcode("", ";%s:%d: %s", ic->filename, ic->lineno, printCLine (ic->filename, ic->lineno));
      }
      cln = ic->lineno;
    }
    if (options.iCodeInAsm) {
      const char *iLine = printILine (ic);
      emitcode("", "; ic:%d: %s", ic->seq, iLine);
      dbuf_free (iLine);
    }
    genCaltech10iCode(ic);
  }
  // Now do the actual printing
  printLine(genLine.lineHead, codeOutBuf);

  // Destroy the line list
  destroy_line_list();
}

