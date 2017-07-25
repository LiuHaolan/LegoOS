/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _ASM_X86_ALTERNATIVE_H_
#define _ASM_X86_ALTERNATIVE_H_

#ifndef __ASSEMBLY__

#include <asm/asm.h>
#include <lego/types.h>
#include <lego/stddef.h>
#include <lego/stringify.h>

struct alt_instr {
	s32 instr_offset;	/* original instruction */
	s32 repl_offset;	/* offset to replacement instruction */
	u16 cpuid;		/* cpuid bit set for replacement */
	u8  instrlen;		/* length of original instruction */
	u8  replacementlen;	/* length of new instruction */
	u8  padlen;		/* length of build-time padding */
} __packed;

/*
 * Debug flag that can be tested to see whether alternative
 * instructions were patched in already:
 */
extern int alternatives_patched;

void alternative_instructions(void);
void apply_alternatives(struct alt_instr *start, struct alt_instr *end);

#ifdef CONFIG_SMP
void alternatives_enable_smp(void);
int alternatives_text_reserved(void *start, void *end);
bool skip_smp_alternatives;
#else
static inline void alternatives_enable_smp(void) {}
static inline int alternatives_text_reserved(void *start, void *end)
{
	return 0;
}
#endif	/* CONFIG_SMP */

#define b_replacement(num)	"664"#num
#define e_replacement(num)	"665"#num

#define alt_end_marker		"663"
#define alt_slen		"662b-661b"
#define alt_pad_len		alt_end_marker"b-662b"
#define alt_total_slen		alt_end_marker"b-661b"
#define alt_rlen(num)		e_replacement(num)"f-"b_replacement(num)"f"

#define __OLDINSTR(oldinstr, num)					\
	"661:\n\t" oldinstr "\n662:\n"					\
	".skip -(((" alt_rlen(num) ")-(" alt_slen ")) > 0) * "		\
		"((" alt_rlen(num) ")-(" alt_slen ")),0x90\n"

#define OLDINSTR(oldinstr, num)						\
	__OLDINSTR(oldinstr, num)					\
	alt_end_marker ":\n"

/*
 * max without conditionals. Idea adapted from:
 * http://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
 *
 * The additional "-" is needed because gas works with s32s.
 */
#define alt_max_short(a, b)	"((" a ") ^ (((" a ") ^ (" b ")) & -(-((" a ") - (" b ")))))"

/*
 * Pad the second replacement alternative with additional NOPs if it is
 * additionally longer than the first replacement alternative.
 */
#define OLDINSTR_2(oldinstr, num1, num2) \
	"661:\n\t" oldinstr "\n662:\n"								\
	".skip -((" alt_max_short(alt_rlen(num1), alt_rlen(num2)) " - (" alt_slen ")) > 0) * "	\
		"(" alt_max_short(alt_rlen(num1), alt_rlen(num2)) " - (" alt_slen ")), 0x90\n"	\
	alt_end_marker ":\n"

#define ALTINSTR_ENTRY(feature, num)					      \
	" .long 661b - .\n"				/* label           */ \
	" .long " b_replacement(num)"f - .\n"		/* new instruction */ \
	" .word " __stringify(feature) "\n"		/* feature bit     */ \
	" .byte " alt_total_slen "\n"			/* source len      */ \
	" .byte " alt_rlen(num) "\n"			/* replacement len */ \
	" .byte " alt_pad_len "\n"			/* pad len */

#define ALTINSTR_REPLACEMENT(newinstr, feature, num)	/* replacement */     \
	b_replacement(num)":\n\t" newinstr "\n" e_replacement(num) ":\n\t"

/* alternative assembly primitive: */
#define ALTERNATIVE(oldinstr, newinstr, feature)			\
	OLDINSTR(oldinstr, 1)						\
	".pushsection .altinstructions,\"a\"\n"				\
	ALTINSTR_ENTRY(feature, 1)					\
	".popsection\n"							\
	".pushsection .altinstr_replacement, \"ax\"\n"			\
	ALTINSTR_REPLACEMENT(newinstr, feature, 1)			\
	".popsection"

#define ALTERNATIVE_2(oldinstr, newinstr1, feature1, newinstr2, feature2)\
	OLDINSTR_2(oldinstr, 1, 2)					\
	".pushsection .altinstructions,\"a\"\n"				\
	ALTINSTR_ENTRY(feature1, 1)					\
	ALTINSTR_ENTRY(feature2, 2)					\
	".popsection\n"							\
	".pushsection .altinstr_replacement, \"ax\"\n"			\
	ALTINSTR_REPLACEMENT(newinstr1, feature1, 1)			\
	ALTINSTR_REPLACEMENT(newinstr2, feature2, 2)			\
	".popsection"

/*
 * Alternative instructions for different CPU types or capabilities.
 *
 * This allows to use optimized instructions even on generic binary
 * kernels.
 *
 * length of oldinstr must be longer or equal the length of newinstr
 * It can be padded with nops as needed.
 *
 * For non barrier like inlines please define new variants
 * without volatile and memory clobber.
 */
#define alternative(oldinstr, newinstr, feature)			\
	asm volatile (ALTERNATIVE(oldinstr, newinstr, feature) : : : "memory")

#define alternative_2(oldinstr, newinstr1, feature1, newinstr2, feature2) \
	asm volatile(ALTERNATIVE_2(oldinstr, newinstr1, feature1, newinstr2, feature2) ::: "memory")

/*
 * Alternative inline assembly with input.
 *
 * Pecularities:
 * No memory clobber here.
 * Argument numbers start with 1.
 * Best is to use constraints that are fixed size (like (%1) ... "r")
 * If you use variable sized constraints like "m" or "g" in the
 * replacement make sure to pad to the worst case length.
 * Leaving an unused argument 0 to keep API compatibility.
 */
#define alternative_input(oldinstr, newinstr, feature, input...)	\
	asm volatile (ALTERNATIVE(oldinstr, newinstr, feature)		\
		: : "i" (0), ## input)

/*
 * This is similar to alternative_input. But it has two features and
 * respective instructions.
 *
 * If CPU has feature2, newinstr2 is used.
 * Otherwise, if CPU has feature1, newinstr1 is used.
 * Otherwise, oldinstr is used.
 */
#define alternative_input_2(oldinstr, newinstr1, feature1, newinstr2,	     \
			   feature2, input...)				     \
	asm volatile(ALTERNATIVE_2(oldinstr, newinstr1, feature1,	     \
		newinstr2, feature2)					     \
		: : "i" (0), ## input)

/* Like alternative_input, but with a single output argument */
#define alternative_io(oldinstr, newinstr, feature, output, input...)	\
	asm volatile (ALTERNATIVE(oldinstr, newinstr, feature)		\
		: output : "i" (0), ## input)

/* Like alternative_io, but for replacing a direct call with another one. */
#define alternative_call(oldfunc, newfunc, feature, output, input...)	\
	asm volatile (ALTERNATIVE("call %P[old]", "call %P[new]", feature) \
		: output : [old] "i" (oldfunc), [new] "i" (newfunc), ## input)

/*
 * Like alternative_call, but there are two features and respective functions.
 * If CPU has feature2, function2 is used.
 * Otherwise, if CPU has feature1, function1 is used.
 * Otherwise, old function is used.
 */
#define alternative_call_2(oldfunc, newfunc1, feature1, newfunc2, feature2,   \
			   output, input...)				      \
{									      \
	register void *__sp asm(_ASM_SP);				      \
	asm volatile (ALTERNATIVE_2("call %P[old]", "call %P[new1]", feature1,\
		"call %P[new2]", feature2)				      \
		: output, "+r" (__sp)					      \
		: [old] "i" (oldfunc), [new1] "i" (newfunc1),		      \
		  [new2] "i" (newfunc2), ## input);			      \
}

/*
 * use this macro(s) if you need more than one output parameter
 * in alternative_io
 */
#define ASM_OUTPUT2(a...) a

/*
 * use this macro if you need clobbers but no inputs in
 * alternative_{input,io,call}()
 */
#define ASM_NO_INPUT_CLOBBER(clbr...) "i" (0) : clbr

#endif /* __ASSEMBLY__ */

#endif /* _ASM_X86_ALTERNATIVE_H_ */
