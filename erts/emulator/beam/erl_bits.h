/*
 * %CopyrightBegin%
 *
 * Copyright Ericsson AB 1999-2023. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * %CopyrightEnd%
 */

#ifndef __ERL_BITS_H__
#define __ERL_BITS_H__

/*
 * This structure represents a SUB_BINARY.
 *
 * Note: The last field (orig) is not counted in arityval in the header.
 * This simplifies garbage collection.
 */

typedef struct erl_sub_bin {
    Eterm thing_word;		/* Subtag SUB_BINARY_SUBTAG. */
    Uint size;			/* Binary size in bytes. */
    Uint offs;			/* Offset into original binary. */
    byte bitsize;
    byte bitoffs;
    byte is_writable;		/* The underlying binary is writable */
    Eterm orig;			/* Original binary (REFC or HEAP binary). */
} ErlSubBin;

/*
 * This structure represents a binary to be matched.
 */

typedef struct erl_bin_match_buffer {
    Eterm orig;			/* Original binary term. */
    byte* base;			/* Current position in binary. */
    Uint offset;		/* Offset in bits. */
    size_t size;		/* Size of binary in bits. */
} ErlBinMatchBuffer;

struct erl_bits_state {
    /*
     * Temporary buffer sometimes used by erts_new_bs_put_integer().
     */
    byte *byte_buf_;
    Uint byte_buf_len_;

    /*
     * Pointer to the beginning of the current binary.
     */
    byte* erts_current_bin_;

    /*
     * Offset in bits into the current binary.
     */
    Uint erts_bin_offset_;
};

typedef struct erl_bin_match_struct{
  Eterm thing_word;
  ErlBinMatchBuffer mb;		/* Present match buffer */
  Eterm save_offset[1];         /* Saved offsets, only valid for contexts
                                 * created through bs_start_match2. */
} ErlBinMatchState;

#define ERL_BIN_MATCHSTATE_SIZE(_Max) \
    ((offsetof(ErlBinMatchState, save_offset) + (_Max)*sizeof(Eterm))/sizeof(Eterm))
#define HEADER_BIN_MATCHSTATE(_Max) \
    _make_header(ERL_BIN_MATCHSTATE_SIZE((_Max)) - 1, _TAG_HEADER_BIN_MATCHSTATE)
#define HEADER_NUM_SLOTS(hdr) \
    (header_arity(hdr) - (offsetof(ErlBinMatchState, save_offset) / sizeof(Eterm)) + 1)

#define make_matchstate(_Ms) make_boxed((Eterm*)(_Ms))  
#define ms_matchbuffer(_Ms) &(((ErlBinMatchState*) boxed_val(_Ms))->mb)


/*
 * Reentrant API with the state passed as a parameter.
 * (Except when the current Process* already is a parameter.)
 */
/* the state resides in the current process' scheduler data */
#define ERL_BITS_DECLARE_STATEP struct erl_bits_state *EBS

#define ERL_BITS_RELOAD_STATEP(P)                                              \
    do {                                                                       \
        EBS = &erts_proc_sched_data((P))->registers->aux_regs.d.erl_bits_state;  \
    } while(0)

#define ERL_BITS_DEFINE_STATEP(P) \
    struct erl_bits_state *EBS = \
        &erts_proc_sched_data((P))->registers->aux_regs.d.erl_bits_state

#define ErlBitsState				(*EBS)

#define ERL_BITS_PROTO_0			struct erl_bits_state *EBS
#define ERL_BITS_PROTO_1(PARM1)			struct erl_bits_state *EBS, PARM1
#define ERL_BITS_PROTO_2(PARM1,PARM2)		struct erl_bits_state *EBS, PARM1, PARM2
#define ERL_BITS_PROTO_3(PARM1,PARM2,PARM3)	struct erl_bits_state *EBS, PARM1, PARM2, PARM3
#define ERL_BITS_ARGS_0				EBS
#define ERL_BITS_ARGS_1(ARG1)			EBS, ARG1
#define ERL_BITS_ARGS_2(ARG1,ARG2)		EBS, ARG1, ARG2
#define ERL_BITS_ARGS_3(ARG1,ARG2,ARG3)		EBS, ARG1, ARG2, ARG3

#define erts_bin_offset		(ErlBitsState.erts_bin_offset_)
#define erts_current_bin	(ErlBitsState.erts_current_bin_)

#define copy_binary_to_buffer(DstBuffer, DstBufOffset, SrcBuffer, SrcBufferOffset, NumBits) \
  do {											    \
    if (BIT_OFFSET(DstBufOffset) == 0 && (SrcBufferOffset == 0) &&			    \
        (BIT_OFFSET(NumBits)==0) && (NumBits != 0)) {					    \
      sys_memcpy(((byte*)DstBuffer)+BYTE_OFFSET(DstBufOffset),					    \
		 SrcBuffer, NBYTES(NumBits));						    \
    } else {										    \
      erts_copy_bits(SrcBuffer, SrcBufferOffset, 1,					    \
        (byte*)DstBuffer, DstBufOffset, 1, NumBits);					    \
    }											    \
  }  while (0)

void erts_init_bits(void);	/* Initialization once. */
void erts_bits_init_state(ERL_BITS_PROTO_0);
void erts_bits_destroy_state(ERL_BITS_PROTO_0);


/*
 * NBYTES(x) returns the number of bytes needed to store x bits.
 */

#define NBYTES(x)  (((Uint64)(x) + (Uint64) 7) >> 3) 
#define BYTE_OFFSET(ofs) ((Uint) (ofs) >> 3)
#define BIT_OFFSET(ofs) ((ofs) & 7)

/*
 * Return number of Eterm words needed for allocation with HAlloc(),
 * given a number of bytes.
 */
#define WSIZE(n) ((n + sizeof(Eterm) - 1) / sizeof(Eterm))

/*
 * Define the maximum number of bits in a unit for the binary syntax.
 */
#define ERL_UNIT_BITS 8

/*
 * Binary matching.
 */

Eterm erts_bs_start_match_2(Process *p, Eterm Bin, Uint Max);
ErlBinMatchState *erts_bs_start_match_3(Process *p, Eterm Bin);
Eterm erts_bs_get_integer_2(Process *p, Uint num_bits, unsigned flags, ErlBinMatchBuffer* mb);
Eterm erts_bs_get_float_2(Process *p, Uint num_bits, unsigned flags, ErlBinMatchBuffer* mb);

/* These will create heap binaries when appropriate, so they require free space
 * up to EXTRACT_SUB_BIN_HEAP_NEED. */
Eterm erts_bs_get_binary_2(Process *p, Uint num_bits, unsigned flags, ErlBinMatchBuffer* mb);
Eterm erts_bs_get_binary_all_2(Process *p, ErlBinMatchBuffer* mb);

/*
 * Binary construction, new instruction set.
 */

int erts_new_bs_put_integer(ERL_BITS_PROTO_3(Eterm Integer, Uint num_bits, unsigned flags));
int erts_bs_put_utf8(ERL_BITS_PROTO_1(Eterm Integer));
int erts_bs_put_utf16(ERL_BITS_PROTO_2(Eterm Integer, Uint flags));
int erts_new_bs_put_binary(Process *c_p, Eterm Bin, Uint num_bits);
int erts_new_bs_put_binary_all(Process *c_p, Eterm Bin, Uint unit);
Eterm erts_new_bs_put_float(Process *c_p, Eterm Float, Uint num_bits, int flags);
void erts_new_bs_put_string(ERL_BITS_PROTO_2(byte* iptr, Uint num_bytes));

Uint erts_bits_bufs_size(void);
Uint32 erts_bs_get_unaligned_uint32(ErlBinMatchBuffer* mb);
Eterm erts_bs_get_utf8(ErlBinMatchBuffer* mb);
Eterm erts_bs_get_utf16(ErlBinMatchBuffer* mb, Uint flags);
Eterm erts_bs_append(Process* p, Eterm* reg, Uint live, Eterm build_size_term,
		     Uint extra_words, Uint unit);
Eterm erts_bs_append_checked(Process* p, Eterm* reg, Uint live, Uint size,
                             Uint extra_words, Uint unit);
Eterm erts_bs_private_append(Process* p, Eterm bin, Eterm sz, Uint unit);
Eterm erts_bs_private_append_checked(Process* p, Eterm bin, Uint size, Uint unit);
Eterm erts_bs_init_writable(Process* p, Eterm sz);

/*
 * Common utilities.
 */
void erts_copy_bits(byte* src, size_t soffs, int sdir,
		    byte* dst, size_t doffs,int ddir, size_t n);
int erts_cmp_bits(byte* a_ptr, size_t a_offs, byte* b_ptr, size_t b_offs, size_t size); 


/* Extracts a region from base_bin as a sub-binary or heap binary, whichever
 * is the most appropriate.
 *
 * The caller must ensure that there's enough free space at *hp */
Eterm erts_extract_sub_binary(Eterm **hp, Eterm base_bin, byte *base_data,
                              Uint bit_offset, Uint num_bits);

/* Pessimistic estimate of the words required for erts_extract_sub_binary */
#define EXTRACT_SUB_BIN_HEAP_NEED (heap_bin_size(ERL_ONHEAP_BIN_LIMIT))

/*
 * Flags for bs_create_bin / bs_get_* / bs_put_* / bs_init* instructions.
 */

#define BSF_ALIGNED 1		/* Field is guaranteed to be byte-aligned. */
#define BSF_LITTLE 2		/* Field is little-endian (otherwise big-endian). */
#define BSF_SIGNED 4		/* Field is signed (otherwise unsigned). */
#define BSF_EXACT 8		/* Size in bs_init is exact. */
#define BSF_NATIVE 16		/* Native endian. */

/*
 * Binary construction operations.
 */

#define BSC_APPEND              0
#define BSC_PRIVATE_APPEND      1
#define BSC_BINARY              2
#define BSC_BINARY_FIXED_SIZE   3
#define BSC_BINARY_ALL          4
#define BSC_FLOAT               5
#define BSC_FLOAT_FIXED_SIZE    6
#define BSC_INTEGER             7
#define BSC_INTEGER_FIXED_SIZE  8
#define BSC_STRING              9
#define BSC_UTF8               10
#define BSC_UTF16              11
#define BSC_UTF32              12

#define BSC_NUM_ARGS            5

#endif /* __ERL_BITS_H__ */
