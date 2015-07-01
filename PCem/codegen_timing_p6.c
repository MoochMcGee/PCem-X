#ifdef DYNAREC

#include "ibm.h"
#include "cpu.h"
#include "x86.h"
#include "x86_ops.h"
#include "x87.h"
#include "mem.h"
#include "codegen.h"

#define CYCLES(c) (int *)c
#define CYCLES2(c16, c32) (int *)((-1 & ~0xffff) | c16 | (c32 << 8))

static int *opcode_timings[256] =
{
/*00*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(2),      CYCLES(8),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(2),      NULL,
/*10*/  CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),      CYCLES(8),      CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),      CYCLES(8),
/*20*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),
/*30*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),

/*40*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),
/*50*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),
/*60*/  CYCLES(8),     CYCLES(8),     CYCLES(7),     CYCLES(9),     CYCLES(0),     CYCLES(0),     CYCLES(0),      CYCLES(0),      CYCLES(1),     CYCLES(4),     CYCLES(1),     CYCLES(4),     CYCLES(18),    CYCLES(18),    CYCLES(18),     CYCLES(18),
/*70*/  &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,    &timing_bnt,    &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,    &timing_bnt,

/*80*/  CYCLES(0),     CYCLES(0),     CYCLES(0),     CYCLES(0),     CYCLES(1),     CYCLES(1),     CYCLES(4),      CYCLES(4),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(7),      CYCLES(5),
/*90*/  CYCLES(0),     CYCLES(3),     CYCLES(3),     CYCLES(3),     CYCLES(3),     CYCLES(3),     CYCLES(3),      CYCLES(3),      CYCLES(1),     CYCLES(1),     CYCLES(28),    CYCLES(2),     CYCLES(11),    CYCLES(10),    CYCLES(1),      CYCLES(1),
/*a0*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(3),     CYCLES(3),     CYCLES(4),      CYCLES(4),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(2),     CYCLES(2),     CYCLES(2),      CYCLES(2),
/*b0*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),

/*c0*/  CYCLES(4),     CYCLES(4),     CYCLES(3),     CYCLES(2),     CYCLES(0),     CYCLES(0),     CYCLES(1),      CYCLES(1),      CYCLES(18),    CYCLES(2),     CYCLES(23),    CYCLES(23),    CYCLES(28),    CYCLES(28),    CYCLES(28),     CYCLES(23),
/*d0*/  CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(1),      CYCLES(1),      NULL,          NULL,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,
/*e0*/  CYCLES(8),     CYCLES(8),     CYCLES(8),     CYCLES(1),     CYCLES(18),    CYCLES(18),    CYCLES(18),     CYCLES(18),     CYCLES(1),     CYCLES(1),     CYCLES(21),    CYCLES(1),     CYCLES(18),    CYCLES(18),    CYCLES(18),     CYCLES(18),
/*f0*/  CYCLES(0),     CYCLES(28),    CYCLES(12),    CYCLES(12),    CYCLES(0),     CYCLES(1),     NULL,           NULL,           CYCLES(1),     CYCLES(1),     CYCLES(9),     CYCLES(17),    CYCLES(4),     CYCLES(4),     CYCLES(1),      NULL
};

static int *opcode_timings_mod3[256] =
{
/*00*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(2),      CYCLES(8),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(2),      NULL,
/*10*/  CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),      CYCLES(8),      CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),      CYCLES(8),
/*20*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),
/*30*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(0),      CYCLES(1),

/*40*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),
/*50*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),
/*60*/  CYCLES(8),     CYCLES(8),     CYCLES(7),     CYCLES(9),     CYCLES(0),     CYCLES(0),     CYCLES(0),      CYCLES(0),      CYCLES(1),     CYCLES(4),     CYCLES(1),     CYCLES(4),     CYCLES(18),    CYCLES(18),    CYCLES(18),     CYCLES(18),
/*70*/  &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,    &timing_bnt,    &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,   &timing_bnt,    &timing_bnt,

/*80*/  CYCLES(0),     CYCLES(0),     CYCLES(0),     CYCLES(0),     CYCLES(1),     CYCLES(1),     CYCLES(4),      CYCLES(4),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(7),      CYCLES(5),
/*90*/  CYCLES(0),     CYCLES(3),     CYCLES(3),     CYCLES(3),     CYCLES(3),     CYCLES(3),     CYCLES(3),      CYCLES(3),      CYCLES(1),     CYCLES(1),     CYCLES(28),    CYCLES(2),     CYCLES(11),    CYCLES(10),    CYCLES(1),      CYCLES(1),
/*a0*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(3),     CYCLES(3),     CYCLES(4),      CYCLES(4),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(2),     CYCLES(2),     CYCLES(2),      CYCLES(2),
/*b0*/  CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1),      CYCLES(1),

/*c0*/  CYCLES(4),     CYCLES(4),     CYCLES(3),     CYCLES(2),     CYCLES(0),     CYCLES(0),     CYCLES(1),      CYCLES(1),      CYCLES(18),    CYCLES(2),     CYCLES(23),    CYCLES(23),    CYCLES(28),    CYCLES(28),    CYCLES(28),     CYCLES(23),
/*d0*/  CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(2),     CYCLES(1),      CYCLES(1),      NULL,          NULL,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,
/*e0*/  CYCLES(8),     CYCLES(8),     CYCLES(8),     CYCLES(1),     CYCLES(18),    CYCLES(18),    CYCLES(18),     CYCLES(18),     CYCLES(1),     CYCLES(1),     CYCLES(21),    CYCLES(1),     CYCLES(18),    CYCLES(18),    CYCLES(18),     CYCLES(18),
/*f0*/  CYCLES(0),     CYCLES(28),    CYCLES(12),    CYCLES(12),    CYCLES(0),     CYCLES(1),     NULL,           NULL,           CYCLES(1),     CYCLES(1),     CYCLES(9),     CYCLES(17),    CYCLES(4),     CYCLES(4),     CYCLES(1),      NULL
};

/*TODO: Fix the two-byte opcode timings*/

static int *opcode_timings_0f[256] =
{
/*00*/  CYCLES(20),     CYCLES(11),     CYCLES(11),     CYCLES(10),     NULL,           CYCLES(195),    CYCLES(7),      NULL,           CYCLES(1000),   CYCLES(10000),  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*10*/  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*20*/  CYCLES(6),      CYCLES(6),      CYCLES(6),      CYCLES(6),      CYCLES(6),      CYCLES(6),      NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*30*/  CYCLES(9),      CYCLES(1),      CYCLES(9),      NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,

/*40*/  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*50*/  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*60*/  &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     NULL,           NULL,           &timing_rm,     &timing_rm,
/*70*/  NULL,           &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     &timing_rm,     CYCLES(100),    NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           &timing_rm,     &timing_rm,

/*80*/  &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,
/*90*/  CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),
/*a0*/  CYCLES(3),      CYCLES(3),      CYCLES(14),     CYCLES(8),      CYCLES(3),      CYCLES(4),      NULL,           NULL,           CYCLES(3),      CYCLES(3),      NULL,           CYCLES(13),     CYCLES(3),      CYCLES(3),      NULL,           CYCLES2(18,30),
/*b0*/  CYCLES(10),     CYCLES(10),     CYCLES(6),      CYCLES(13),     CYCLES(6),      CYCLES(6),      CYCLES(3),      CYCLES(3),      NULL,           NULL,           CYCLES(6),      CYCLES(13),     CYCLES(7),      CYCLES(7),      CYCLES(3),      CYCLES(3),

/*c0*/  CYCLES(4),      CYCLES(4),      NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),
/*d0*/  NULL,           &timing_rm,     &timing_rm,     &timing_rm,     NULL,           &timing_rm,     NULL,           NULL,           &timing_rm,     &timing_rm,     NULL,           &timing_rm,     &timing_rm,     &timing_rm,     NULL,           &timing_rm,
/*e0*/  NULL,           &timing_rm,     &timing_rm,     NULL,           NULL,           &timing_rm,     NULL,           NULL,           &timing_rm,     &timing_rm,     NULL,           &timing_rm,     &timing_rm,     &timing_rm,     NULL,           &timing_rm,
/*f0*/  NULL,           &timing_rm,     &timing_rm,     &timing_rm,     NULL,           &timing_rm,     NULL,           NULL,           &timing_rm,     &timing_rm,     &timing_rm,     NULL,           &timing_rm,     &timing_rm,     &timing_rm,     NULL,
};
static int *opcode_timings_0f_mod3[256] =
{
/*00*/  CYCLES(20),     CYCLES(11),     CYCLES(11),     CYCLES(10),     NULL,           CYCLES(195),    CYCLES(7),      NULL,           CYCLES(1000),   CYCLES(10000),  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*10*/  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*20*/  CYCLES(6),      CYCLES(6),      CYCLES(6),      CYCLES(6),      CYCLES(6),      CYCLES(6),      NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*30*/  CYCLES(9),      CYCLES(1),      CYCLES(9),      NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,

/*40*/  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*50*/  NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*60*/  &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     NULL,           NULL,           &timing_rr,     &timing_rr,
/*70*/  NULL,           &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     &timing_rr,     CYCLES(100),    NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           &timing_rr,     &timing_rr,

/*80*/  &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,    &timing_bnt,
/*90*/  CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),      CYCLES(3),
/*a0*/  CYCLES(3),      CYCLES(3),      CYCLES(14),     CYCLES(8),      CYCLES(3),      CYCLES(4),      NULL,           NULL,           CYCLES(3),      CYCLES(3),      NULL,           CYCLES(13),     CYCLES(3),      CYCLES(3),      NULL,           CYCLES2(18,30),
/*b0*/  CYCLES(10),     CYCLES(10),     CYCLES(6),      CYCLES(13),     CYCLES(6),      CYCLES(6),      CYCLES(3),      CYCLES(3),      NULL,           NULL,           CYCLES(6),      CYCLES(13),     CYCLES(7),      CYCLES(7),      CYCLES(3),      CYCLES(3),

/*c0*/  CYCLES(4),      CYCLES(4),      NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),
/*d0*/  NULL,           &timing_rr,     &timing_rr,     &timing_rr,     NULL,           &timing_rr,     NULL,           NULL,           &timing_rr,     &timing_rr,     NULL,           &timing_rr,     &timing_rr,     &timing_rr,     NULL,           &timing_rr,
/*e0*/  NULL,           &timing_rr,     &timing_rr,     NULL,           NULL,           &timing_rr,     NULL,           NULL,           &timing_rr,     &timing_rr,     NULL,           &timing_rr,     &timing_rr,     &timing_rr,     NULL,           &timing_rr,
/*f0*/  NULL,           &timing_rr,     &timing_rr,     &timing_rr,     NULL,           &timing_rr,     NULL,           NULL,           &timing_rr,     &timing_rr,     &timing_rr,     NULL,           &timing_rr,     &timing_rr,     &timing_rr,     NULL,
};

static int *opcode_timings_shift[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(4),     CYCLES(4),     CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1)
};
static int *opcode_timings_shift_mod3[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(4),     CYCLES(4),     CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1)
};

static int *opcode_timings_f6[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(4),      CYCLES(4),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_f6_mod3[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(4),      CYCLES(4),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_f7[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(4),      CYCLES(4),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_f7_mod3[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(1),     CYCLES(1),     CYCLES(4),      CYCLES(4),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_ff[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(4),     CYCLES(28),    CYCLES(1),      CYCLES(21),     CYCLES(1),      NULL
};
static int *opcode_timings_ff_mod3[8] =
{
        CYCLES(1),      CYCLES(1),      CYCLES(4),     CYCLES(28),    CYCLES(1),      CYCLES(21),     CYCLES(1),      NULL
};

static int *opcode_timings_d8[8] =
{
/*      FADDil          FMULil          FCOMil          FCOMPil         FSUBil          FSUBRil         FDIVil          FDIVRil*/
        CYCLES(3),      CYCLES(5),     CYCLES(1),      CYCLES(1),      CYCLES(3),      CYCLES(3),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_d8_mod3[8] =
{
/*      FADD            FMUL            FCOM            FCOMP           FSUB            FSUBR           FDIV            FDIVR*/
        CYCLES(3),      CYCLES(5),     CYCLES(1),      CYCLES(1),      CYCLES(3),      CYCLES(3),      CYCLES(39),     CYCLES(39)
};

static int *opcode_timings_d9[8] =
{
/*      FLDs                            FSTs            FSTPs           FLDENV          FLDCW           FSTENV          FSTCW*/
        CYCLES(1),      NULL,           CYCLES(1),      CYCLES(2),      CYCLES(10),     CYCLES(10),     CYCLES(8),     CYCLES(1)
};
static int *opcode_timings_d9_mod3[64] =
{
        /*FLD*/
        CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),
        /*FXCH*/
        CYCLES(0),      CYCLES(0),      CYCLES(0),      CYCLES(0),      CYCLES(0),      CYCLES(0),      CYCLES(0),      CYCLES(0),
        /*FNOP*/
        CYCLES(1),      NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
        /*FSTP*/
        CYCLES(2),      CYCLES(2),      CYCLES(2),      CYCLES(2),      CYCLES(2),      CYCLES(2),      CYCLES(2),      CYCLES(2),
/*      opFCHS          opFABS                                          opFTST          opFXAM*/
        CYCLES(2),      CYCLES(1),      NULL,           NULL,           CYCLES(1),      CYCLES(2),      NULL,           NULL,
/*      opFLD1          opFLDL2T        opFLDL2E        opFLDPI         opFLDEG2        opFLDLN2        opFLDZ*/
        CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      CYCLES(1),      NULL,
/*      opF2XM1         opFYL2X         opFPTAN         opFPATAN                                        opFDECSTP       opFINCSTP,*/
        CYCLES(66),     CYCLES(103),    CYCLES(143),    CYCLES(143),    NULL,           NULL,           CYCLES(1),      CYCLES(1),
/*      opFPREM                         opFSQRT         opFSINCOS       opFRNDINT       opFSCALE        opFSIN          opFCOS*/
        CYCLES(23),     NULL,           CYCLES(69),     CYCLES(130),    CYCLES(30),     CYCLES(56),     CYCLES(103),    CYCLES(103)
};

static int *opcode_timings_da[8] =
{
/*      FADDil          FMULil          FCOMil          FCOMPil         FSUBil          FSUBRil         FDIVil          FDIVRil*/
        CYCLES(3),      CYCLES(5),      CYCLES(1),      CYCLES(1),      CYCLES(3),      CYCLES(3),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_da_mod3[8] =
{
        NULL,           NULL,           NULL,           NULL,           NULL,           CYCLES(3),      NULL,           NULL
};


static int *opcode_timings_db[8] =
{
/*      FLDil                           FSTil           FSTPil                          FLDe                            FSTPe*/
        CYCLES(1),      NULL,           CYCLES(1),      CYCLES(1),      NULL,           CYCLES(1),      NULL,           CYCLES(1)
};
static int *opcode_timings_db_mod3[64] =
{
        NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
        NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
        NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
        NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
/*                      opFNOP          opFCLEX         opFINIT         opFNOP          opFNOP*/
        NULL,           CYCLES(1),      CYCLES(3),      CYCLES(13),     CYCLES(1),      CYCLES(1),      NULL,
        NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
        NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
        NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,           NULL,
};

static int *opcode_timings_dc[8] =
{
/*      opFADDd_a16     opFMULd_a16     opFCOMd_a16     opFCOMPd_a16    opFSUBd_a16     opFSUBRd_a16    opFDIVd_a16     opFDIVRd_a16*/
        CYCLES(3),      CYCLES(5),      CYCLES(1),      CYCLES(1),      CYCLES(3),      CYCLES(3),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_dc_mod3[8] =
{
/*      opFADDr         opFMULr                                         opFSUBRr        opFSUBr         opFDIVRr        opFDIVr*/
        CYCLES(3),      CYCLES(5),      NULL,           NULL,           CYCLES(3),      CYCLES(3),      CYCLES(39),     CYCLES(39)
};

static int *opcode_timings_dd[8] =
{
/*      FLDd                            FSTd            FSTPd           FRSTOR                           FSAVE          FSTSW*/
        CYCLES(1),      NULL,           CYCLES(1),      CYCLES(1),      CYCLES(72),      NULL,           CYCLES(141),   CYCLES(7)
};
static int *opcode_timings_dd_mod3[8] =
{
/*      FFFREE                          FST             FSTP            FUCOM            FUCOMP*/
        CYCLES(1),      NULL,           CYCLES(1),      CYCLES(1),      CYCLES(1),       CYCLES(1),      NULL,          NULL
};

static int *opcode_timings_de[8] =
{
/*      FADDiw          FMULiw          FCOMiw          FCOMPiw         FSUBil          FSUBRil         FDIVil          FDIVRil*/
        CYCLES(3),      CYCLES(5),      CYCLES(1),      CYCLES(1),      CYCLES(3),      CYCLES(3),      CYCLES(39),     CYCLES(39)
};
static int *opcode_timings_de_mod3[8] =
{
/*      FADD            FMUL                            FCOMPP          FSUB            FSUBR           FDIV            FDIVR*/
        CYCLES(3),      CYCLES(5),      NULL,           CYCLES(1),      CYCLES(3),      CYCLES(3),      CYCLES(39),     CYCLES(39)
};

static int *opcode_timings_df[8] =
{
/*      FILDiw                          FISTiw          FISTPiw                          FILDiq          FBSTP          FISTPiq*/
        CYCLES(5),     NULL,            CYCLES(5),      CYCLES(2),      NULL,            CYCLES(5),      CYCLES(165),   CYCLES(2)
};
static int *opcode_timings_df_mod3[8] =
{
/*      FFREE                           FST             FSTP            FUCOM            FUCOMP*/
        CYCLES(1),      NULL,           CYCLES(1),      CYCLES(1),      CYCLES(1),       CYCLES(1),      NULL,          NULL
};

static int *opcode_timings_8x[8] =
{
        CYCLES(1),     CYCLES(1),     CYCLES(3),     CYCLES(3),     CYCLES(1),     CYCLES(1),     CYCLES(1),     CYCLES(1)
};

static int timing_count;
static uint8_t last_prefix;

static inline int COUNT(int *c, int op_32)
{
        if ((uintptr_t)c <= 10000)
                return (int)c;
        if (((uintptr_t)c & ~0xffff) == (-1 & ~0xffff))
        {
                if (op_32 & 0x100)
                        return ((uintptr_t)c >> 8) & 0xff;
                return (uintptr_t)c & 0xff;
        }
        return *c;
}

void codegen_timing_p6_block_start()
{
}

void codegen_timing_p6_start()
{
        timing_count = 0;
        last_prefix = 0;
}

void codegen_timing_p6_prefix(uint8_t prefix, uint32_t fetchdat)
{
        timing_count += COUNT(opcode_timings[prefix], 0);
        last_prefix = prefix;
}

void codegen_timing_p6_opcode(uint8_t opcode, uint32_t fetchdat, int op_32)
{
        int **timings;
        int mod3 = ((fetchdat & 0xc0) == 0xc0);

        switch (last_prefix)
        {
                case 0x0f:
                timings = mod3 ? opcode_timings_0f_mod3 : opcode_timings_0f;
                break;

                case 0xd8:
                timings = mod3 ? opcode_timings_d8_mod3 : opcode_timings_d8;
                opcode = (opcode >> 3) & 7;
                break;
                case 0xd9:
                timings = mod3 ? opcode_timings_d9_mod3 : opcode_timings_d9;
                opcode = mod3 ? opcode & 0x3f : (opcode >> 3) & 7;
                break;
                case 0xda:
                timings = mod3 ? opcode_timings_da_mod3 : opcode_timings_da;
                opcode = (opcode >> 3) & 7;
                break;
                case 0xdb:
                timings = mod3 ? opcode_timings_db_mod3 : opcode_timings_db;
                opcode = mod3 ? opcode & 0x3f : (opcode >> 3) & 7;
                break;
                case 0xdc:
                timings = mod3 ? opcode_timings_dc_mod3 : opcode_timings_dc;
                opcode = (opcode >> 3) & 7;
                break;
                case 0xdd:
                timings = mod3 ? opcode_timings_dd_mod3 : opcode_timings_dd;
                opcode = (opcode >> 3) & 7;
                break;
                case 0xde:
                timings = mod3 ? opcode_timings_de_mod3 : opcode_timings_de;
                opcode = (opcode >> 3) & 7;
                break;
                case 0xdf:
                timings = mod3 ? opcode_timings_df_mod3 : opcode_timings_df;
                opcode = (opcode >> 3) & 7;
                break;

                default:
                switch (opcode)
                {
                        case 0x80: case 0x81: case 0x82: case 0x83:
                        timings = mod3 ? opcode_timings_mod3 : opcode_timings_8x;
                        if (!mod3)
                                opcode = (fetchdat >> 3) & 7;
                        break;

                        case 0xc0: case 0xc1: case 0xd0: case 0xd1: case 0xd2: case 0xd3:
                        timings = mod3 ? opcode_timings_shift_mod3 : opcode_timings_shift;
                        opcode = (fetchdat >> 3) & 7;
                        break;

                        case 0xf6:
                        timings = mod3 ? opcode_timings_f6_mod3 : opcode_timings_f6;
                        opcode = (fetchdat >> 3) & 7;
                        break;
                        case 0xf7:
                        timings = mod3 ? opcode_timings_f7_mod3 : opcode_timings_f7;
                        opcode = (fetchdat >> 3) & 7;
                        break;
                        case 0xff:
                        timings = mod3 ? opcode_timings_ff_mod3 : opcode_timings_ff;
                        opcode = (fetchdat >> 3) & 7;
                        break;

                        default:
                        timings = mod3 ? opcode_timings_mod3 : opcode_timings;
                        break;
                }
        }

        timing_count += COUNT(timings[opcode], op_32);
        codegen_block_cycles += timing_count;
}

void codegen_timing_p6_block_end()
{
}

codegen_timing_t codegen_timing_p6 =
{
        codegen_timing_p6_start,
        codegen_timing_p6_prefix,
        codegen_timing_p6_opcode,
        codegen_timing_p6_block_start,
        codegen_timing_p6_block_end
};
#endif
