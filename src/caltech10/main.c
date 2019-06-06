/*-------------------------------------------------------------------------
  main.c - caltech10 general setup
-------------------------------------------------------------------------*/

#include "common.h"
#include "main.h"
#include "ralloc.h"
#include "gen.h"
#include "dbuf_string.h"

static const ASM_MAPPING caltech10_asm_mapping_arr[] = {
  {"labeldef", "%s:"},
  {"slabeldef", "%s:"},
  {"area", "; .area %s"},
  {"areacode", "; .areacode"},
  {"areadata", "; .areadata"},
  {"areahome", "; .areahome"},
  {"ds", "; .ds %d"},
  {"module", "; .file \"%s.c\""},
  {"fileprelude", ""},
  {"global", "; .globl %s"},
  {NULL, NULL}
};

static const ASM_MAPPINGS caltech10_asm_mappings = {
  NULL,
  caltech10_asm_mapping_arr
};

static const char *caltech10_asmCmd[] = {"caltech10as", "$1.asm", NULL};

static void caltech10_init() {
  // Initialize assembler tree
  asm_addTree(&caltech10_asm_mappings);
}

static bool caltech10_parseOptions(int *pargc, char **argv, int *i) {
  return FALSE;  /* No options to parse */
}
static void caltech10_finaliseOptions() {
  // Set local/global memmaps
  port->mem.default_local_map = data;
  port->mem.default_globl_map = data;
}
static void caltech10_setDefaultOptions() {}
static void caltech10_reset_regparams(struct sym_link *s) {}
// 1 if symbol can be passed in register
static int caltech10_reg_parm(struct sym_link *s, bool reentrant) {
  return 0;
}

/* Indicate which extended bit operations this port supports */
static bool hasExtBitOp (int op, int size) {
  return op == RRC || op == RLC;
}

/* Indicate the expense of an access to an output storage class */
static int oclsExpense (struct memmap *oclass) {
  return 0;
}

// Caltech10 does not have native multiplication
static bool hasNativeMulFor (iCode *ic, sym_link *left, sym_link *right) {
  return false;
}

PORT caltech10_port =
{
  TARGET_ID_CALTECH10,          // Unique ID for target
  "caltech10",                  // Target name for -m
  "Caltech10",                  // Target name in --help
  NULL,                         // Processor name
  { // General
    glue,                       // Use default glue fn
    FALSE,                      /* Emit glue around main */
    MODEL_SMALL,                // Supported models
    MODEL_SMALL,                // Default model
    NULL,                       /* model == target */
  },
  { // Assembler
    caltech10_asmCmd,
    NULL,
    NULL,
    NULL,
    0,        // Print externs as global
    ".asm",   // Asm file extension
    NULL,     // Assembler executing fn
  },
  { // Linker, not supported
  },
  { // Peephole optimizer, not supported
  },
  { // Sizes
    // char, short, int, long, long long
    1, 2, 2, 4, 8,
    // near ptr, far ptr, gptr, func ptr, banked func ptr, bit, float
    2, 2, 2, 2, 0, 1, 4
  },
  { // Tags for generic pointers
    0x00, 0x00, 0x00, 0x00    /* far, near, xstack, code */
  },
  { // Memory regions. See comments in SDCCmem.h
    NULL,     // External stack
    "STACK",  // Internal stack
    "CODE",   // Code
    "DATA",   // Internal data
    NULL,     // Initialized data
    NULL,     // Paged data
    NULL,     // external data
    NULL,     // Bit space
    NULL,     // Registers
    "GSINIT  (CODE)", // Static
    "OSEG    (PAG, OVR)", // Overlay
    "GSFINAL (CODE)", // Post-static
    "HOME    (CODE)", // HOME
    NULL,     // initialized xdata
    NULL,     // a code copy of xiseg
    NULL,     // const_name - const data (code or not)
    "CABS    (ABS,CODE)", // cabs_name - const absolute data (code or not)
    NULL,     // xabs_name - absolute xdata
    NULL,     // iabs_name - absolute data
    NULL,     // name of segment for initialized variables
    NULL,     // name of segment for copies of initialized variables in code space
    NULL,     // default memmap for local vars (set when initializing port)
    NULL,     // default memmap for global vars (set when initializing port)
    1,        // Code-space is read only
    1         // No extended alignments supported.
  },
  { // No extra areas
  },
  { // Stack
    -1,         /* direction (-1 = stack grows down) */
    0,          /* bank_overhead (switch between register banks) */
    0,          /* isr_overhead. Interrupts not supported */
    2,          /* call_overhead */
    0,          /* reent_overhead */
    0,          /* banked_overhead (switch between code banks) */
    1           /* sp is offset by 1 from last item pushed */
  },
  { // Support
    1,      // Largest possible shift
    FALSE   // No int * int support
  },
  /* No debugger support */
  {},
  { /* Jump table. TODO how to fill? */
    256,        /* maxCount */
    2,          /* sizeofElement */
    {8,16,32},  /* sizeofMatchJump[] */
    {8,16,32},  /* sizeofRangeCompare[] */
    5,          /* sizeofSubtract */
    10,         /* sizeofDispatch */
  },
  "_",
  caltech10_init,
  caltech10_parseOptions,
  NULL,     // list of automatically parsed options
  NULL,     // port-specific paths
  caltech10_finaliseOptions,
  caltech10_setDefaultOptions,
  caltech10_assignRegisters,
  NULL,     // Get name of reg
  NULL,     // Get reg by name
  NULL,     // Keep track of reg contents
  NULL,     // List of keywords
  NULL,     // Assembler preamble
  NULL,     // Assembler postfix
  NULL,     // gen IVT
  NULL,     // gen XINIT
  NULL,     // Port specific startup
  // Parameter passing in register-related functions
  caltech10_reset_regparams,
  caltech10_reg_parm,
  NULL,                         /* process_pragma */
  NULL,                         /* getMangledFunctionName */
  hasNativeMulFor,             /* hasNativeMulFor */
  hasExtBitOp,                  /* hasExtBitOp */
  oclsExpense,                  /* oclsExpense */
  TRUE,                         /* use_dw_for_init */
  TRUE,                         /* little_endian */
  0,                            /* leave < */
  0,                            /* leave > */
  0,                            /* leave <= */
  0,                            /* leave >= */
  0,                            /* leave != */
  0,                            /* leave == */
  FALSE,                        /* No array initializer support. */
  NULL,
  NULL,                         /* no builtin functions */
  GPOINTER,                     /* treat unqualified pointers as "generic" pointers */
  1,                            /* reset labelKey to 1 */
  1,                            /* globals & local statics allowed */
  0,                            /* Number of registers handled in the tree-decomposition-based register allocator in SDCCralloc.hpp */
  PORT_MAGIC
};
