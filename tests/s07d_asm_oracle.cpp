// S07-D test oracle: the original Westwood assembly, transcribed instruction for instruction from
// TASM IDEAL syntax into GNU assembler Intel syntax. See s07d_asm_oracle.h.
//
// Sources (deleted from the tree in session S07-D; see git history):
//   CODE/LCWCOMP.ASM           LCW_Comp
//   WIN32LIB/IFF/LCWUNCMP.ASM  LCW_Uncompress
//   WIN32LIB/WSA/XORDELTA.ASM  Apply_XOR_Delta, Apply_XOR_Delta_To_Page_Or_Viewport,
//                              XOR_Delta_Buffer, Copy_Delta_Buffer
//   WIN32LIB/MISC/RANDOM.ASM   Random (state passed by pointer instead of the global RandNumb)
//   WIN32LIB/MISC/REVERSE.ASM  Reverse_Long, Reverse_Short, Swap_Long
//
// Transcription rules: same instructions in the same order; TASM PROC/ARG/LOCAL/USES expanded into an
// explicit ebp frame (ARGs at [ebp+8], [ebp+12], ...; LOCALs at [ebp-4], [ebp-8], ... in declaration
// order; USES registers pushed after the frame, popped before 'ret'); '??label' -> '.Lxx_label';
// hex 'NNh' -> '0xNN'. Original comments are kept where they help.
//
// Copyright of the transcribed code: see the original files (Electronic Arts, GPLv3).
#include "s07d_asm_oracle.h"

#if S07D_HAVE_ASM_ORACLE

asm(R"ASM(
	.pushsection .text
	.intel_syntax noprefix

# ---------------------------------------------------------------------------------------------
# CODE/LCWCOMP.ASM -- int LCW_Comp(void const *source, void *dest, int length)
# ARG source [ebp+8], dest [ebp+12], datasize [ebp+16]
# LOCAL inlen [ebp-4], a1stdest [ebp-8], a1stsrc [ebp-12], lenoff [ebp-16], ndest [ebp-20],
#       count [ebp-24], matchoff [ebp-28], end_of_data [ebp-32]
# ---------------------------------------------------------------------------------------------
	.globl	ref_LCW_Comp
	.type	ref_LCW_Comp, @function
ref_LCW_Comp:
	push	ebp
	mov	ebp,esp
	sub	esp,32
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi

	cld
	mov	edi,[ebp+12]
	mov	esi,[ebp+8]
	mov	edx,[ebp+16]		# get length of data to compress
	cld
	mov	ebx,esi
	add	ebx,edx
	mov	[ebp-32],ebx		# end_of_data
	mov	DWORD PTR [ebp-4],1	# set the in-length flag
	mov	[ebp-8],edi		# save original dest offset for size calc
	mov	[ebp-12],esi		# save offset of first byte of data
	mov	[ebp-16],edi		# save the offset of the legth of this len
	sub	eax,eax
	mov	al,0x81			# the first byte is always a len
	stosb
	lodsb
	stosb
.Lc_loop:
	mov	[ebp-20],edi		# save offset of compressed data
	mov	edi,[ebp-12]		# get the offset to the first byte of data
	mov	DWORD PTR [ebp-24],1	# set the count of run to 0
.Lc_searchloop:
	sub	eax,eax
	mov	al,[esi]		# get the current byte of data
	cmp	al,[esi+64]
	jne	.Lc_notrunlength

	mov	ebx,edi
	mov	edi,esi
	mov	ecx,[ebp-32]
	sub	ecx,edi
	repe	scasb
	dec	edi
	mov	ecx,edi
	sub	ecx,esi
	cmp	ecx,65
	jb	.Lc_notlongenough

	mov	DWORD PTR [ebp-4],0	# clear the in-length flag
	mov	esi,edi
	mov	edi,[ebp-20]		# get the offset of our compressed data
	mov	ah,al
	mov	al,0xFE
	stosb
	xchg	ecx,eax
	stosw
	mov	al,ch
	stosb
	mov	[ebp-20],edi		# save offset of compressed data
	mov	edi,ebx
	jmp	.Lc_searchloop
.Lc_notlongenough:
	mov	edi,ebx
.Lc_notrunlength:
.Lc_oploop:
	mov	ecx,esi			# get the address of the last byte +1
	sub	ecx,edi			# get the total number of bytes left to comp
	jz	.Lc_searchdone
	repne	scasb			# look for a match
	jne	.Lc_searchdone		# if we don't find one we're done
	mov	ebx,[ebp-24]
	mov	ah,[esi+ebx-1]
	cmp	ah,[edi+ebx-2]
	jne	.Lc_oploop
	mov	edx,esi			# save this spot for the next search
	mov	ebx,edi			# save this spot for the length calc
	dec	edi			# back up one for compare
	mov	ecx,[ebp-32]		# get the end of data
	sub	ecx,esi			# sub current source for max len
	repe	cmpsb			# see how many bytes match
	jne	.Lc_notend		# if found mismatch then di - bx = match count
	inc	edi			# else cx = 0 and di + 1 - bx = match count
.Lc_notend:
	mov	esi,edx			# restore si
	mov	eax,edi			# get the dest
	sub	eax,ebx			# sub the start for total bytes that match
	mov	edi,ebx			# restore dest
	cmp	eax,[ebp-24]		# see if its better than before
	jb	.Lc_searchloop		# if not keep looking
	mov	[ebp-24],eax		# if so keep the count
	dec	ebx			# back it up for the actual match offset
	mov	[ebp-28],ebx		# save the offset for later
	jmp	.Lc_searchloop		# loop until we searched it all
.Lc_searchdone:
	mov	ecx,[ebp-24]		# get the count of the longest run
	mov	edi,[ebp-20]		# get the offset of our compressed data
	cmp	ecx,2			# see if its not enough run to matter
	jbe	.Lc_lenin		# if its 0,1, or 2 its too small
	cmp	ecx,10			# if not, see if it would fit in a short
	ja	.Lc_medrun		# if not, see if its a medium run
	mov	eax,esi			# if its short get the current address
	sub	eax,[ebp-28]		# sub the offset of the match
	cmp	eax,0x0FFF		# if its less than 12 bits its a short
	ja	.Lc_medrun		# if its not, its a medium
.Lc_shortrun:
	sub	ebx,ebx
	mov	bl,cl			# get the length (3-10)
	sub	bl,3			# sub 3 for a 3 bit number 0-7
	shl	bl,4			# shift it left 4
	add	ah,bl			# add in the length for the high nibble
	xchg	ah,al			# reverse the bytes for a word store
	jmp	.Lc_srunnxt		# do the run fixup code
.Lc_medrun:
	cmp	ecx,64			# see if its a short run
	ja	.Lc_longrun		# if not, oh well at least its long
	sub	cl,3			# back down 3 to keep it in 6 bits
	or	cl,0xC0			# the highest bits are always on
	mov	al,cl			# put it in al for the stosb
	stosb				# store it
	jmp	.Lc_medrunnxt		# do the run fixup code
.Lc_lenin:
	cmp	DWORD PTR [ebp-4],0	# is it doing a length?
	jnz	.Lc_len			# if so, skip code
.Lc_lenin1:
	mov	[ebp-16],edi		# save the length code offset
	mov	al,0x80			# set the length to 0
	stosb				# save it
.Lc_len:
	mov	ebx,[ebp-16]		# get the offset of the length code
	cmp	BYTE PTR [ebx],0xBF	# see if its maxed out
	je	.Lc_lenin1		# if so put out a new len code
.Lc_stolen:
	inc	BYTE PTR [ebx]		# inc the count code
	lodsb				# get the byte
	stosb				# store it
	mov	DWORD PTR [ebp-4],1	# we are now in a length so save it
	jmp	.Lc_nxt			# do the next code
.Lc_longrun:
	mov	al,0xFF			# its a long so set a code of FF
	stosb				# store it
	mov	eax,[ebp-24]		# send out the count
	stosw				# store it
.Lc_medrunnxt:
	mov	eax,[ebp-28]		# get the offset
	sub	eax,[ebp-12]		# make it relative tot he start of data
.Lc_srunnxt:
	stosw				# store it
	add	esi,[ebp-24]		# add in the length of the run to the source
	mov	DWORD PTR [ebp-4],0	# set the in leght flag to false
.Lc_nxt:
	cmp	esi,[ebp-32]		# see if we did the whole pic
	jae	.Lc_out			# if so, cool! were done
	jmp	.Lc_loop
.Lc_out:
	mov	ax,0x80			# remember to send an end of data code
	stosb				# store it
	mov	eax,edi			# get the last compressed address
	sub	eax,[ebp-8]		# sub the first for the compressed size
	pop	esi
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx
	mov	esp,ebp
	pop	ebp
	ret
	.size	ref_LCW_Comp, .-ref_LCW_Comp

# ---------------------------------------------------------------------------------------------
# WIN32LIB/IFF/LCWUNCMP.ASM -- ULONG LCW_Uncompress(BYTE *source, BYTE *dest, ULONG length)
# ARG source [ebp+8], dest [ebp+12], length [ebp+16]
# LOCAL a1stdest [ebp-4], maxlen [ebp-8], lastbyte [ebp-12], lastcom [ebp-16], lastcom1 [ebp-20]
# ---------------------------------------------------------------------------------------------
	.globl	ref_LCW_Uncompress
	.type	ref_LCW_Uncompress, @function
ref_LCW_Uncompress:
	push	ebp
	mov	ebp,esp
	sub	esp,20
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi

	mov	edi,[ebp+12]
	mov	esi,[ebp+8]
	mov	edx,[ebp+16]
	mov	[ebp-4],edi
	add	edx,edi
	mov	[ebp-12],edx
	cld				# make sure all lod and sto are forward
	mov	ebx,esi			# save the source offset
.Lu_loop:
	mov	eax,[ebp-12]
	sub	eax,edi			# get the remaining byte to uncomp
	jz	.Lu_out			# were done
	mov	[ebp-8],eax		# save for string commands
	mov	esi,ebx			# mov in the source index
	xor	eax,eax
	mov	al,[esi]
	inc	esi
	test	al,al			# see if its a short run
	js	.Lu_notshort
	mov	ecx,eax			# put count nibble in cl
	mov	ah,al			# put rel offset high nibble in ah
	and	ah,0x0F			# only 4 bits count
	shr	cl,4			# get run -3
	add	ecx,3			# get actual run length
	cmp	ecx,[ebp-8]		# is it too big to fit?
	jbe	.Lu_rsok		# if not, its ok
	mov	ecx,[ebp-8]		# if so, max it out so it dosen't overrun
.Lu_rsok:
	mov	al,[esi]		# get rel offset low byte
	lea	ebx,[esi+1]		# save the source offset
	mov	esi,edi			# get the current dest
	sub	esi,eax			# get relative offset
	rep	movsb
	jmp	.Lu_loop
.Lu_notshort:
	test	al,0x40			# is it a length?
	jne	.Lu_notlength		# if not it could be med or long run
	cmp	al,0x80			# is it the end?
	je	.Lu_out			# if so its over
	mov	cl,al			# put the byte in count register
	and	ecx,0x3F		# and off the extra bits
	cmp	ecx,[ebp-8]		# is it too big to fit?
	jbe	.Lu_lenok		# if not, its ok
	mov	ecx,[ebp-8]		# if so, max it out so it dosen't overrun
.Lu_lenok:
	rep	movsb
	mov	ebx,esi			# save the source offset
	jmp	.Lu_loop
.Lu_out:
	mov	eax,edi
	sub	eax,[ebp-4]
	jmp	.Lu_exit
.Lu_notlength:
	mov	cl,al			# get the entire code
	and	ecx,0x3F		# and off all but the size -3
	add	ecx,3			# add 3 for byte count
	cmp	al,0xFE
	jne	.Lu_notrunlength
	xor	ecx,ecx
	mov	cx,[esi]
	xor	eax,eax
	mov	al,[esi+2]
	lea	ebx,[esi+3]		# save the source offset
	cmp	ecx,[ebp-8]		# is it too big to fit?
	jbe	.Lu_runlenok		# if not, its ok
	mov	ecx,[ebp-8]		# if so, max it out so it dosen't overrun
.Lu_runlenok:
	test	ecx,0xffe0
	jnz	.Lu_dont_use_stosb
	rep	stosb
	jmp	.Lu_loop
.Lu_dont_use_stosb:
	mov	ah,al
	mov	edx,eax
	shl	eax,16
	or	eax,edx
	test	edi,3
	jz	.Lu_aligned
	mov	[edi],eax
	mov	edx,edi
	and	edi,0xfffffffc
	lea	edi,[edi+4]
	and	edx,3
	dec	dl
	xor	dl,3
	sub	ecx,edx
.Lu_aligned:
	mov	edx,ecx
	shr	ecx,2
	rep	stosd
	and	edx,3
	jz	.Lu_loop
	mov	ecx,edx
	rep	stosb
	jmp	.Lu_loop
.Lu_notrunlength:
	cmp	al,0xFF			# is it a long run?
	jne	.Lu_notlong		# if not use the code as the size
	xor	ecx,ecx
	xor	eax,eax
	mov	cx,[esi]		# if so, get the size
	lea	esi,[esi+2]
.Lu_notlong:
	mov	ax,[esi]		# get the real index
	add	eax,[ebp-4]		# add in the 1st index
	lea	ebx,[esi+2]		# save the source offset
	cmp	ecx,[ebp-8]		# compare for overrun
	mov	esi,eax			# use eax as new source
	jbe	.Lu_runok		# if not, its ok
	mov	ecx,[ebp-8]		# if so, max it out so it dosen't overrun
.Lu_runok:
	test	ecx,0xffe0
	jnz	.Lu_dont_use_movsb
	rep	movsb
	jmp	.Lu_loop
.Lu_dont_use_movsb:
	lea	edx,[edi-4]
	cmp	esi,edx
	ja	.Lu_use_movsb
	test	edi,3
	jz	.Lu_aligned2
	mov	eax,[esi]
	mov	[edi],eax
	mov	edx,edi
	and	edi,0xfffffffc
	lea	edi,[edi+4]
	and	edx,3
	dec	dl
	xor	dl,3
	sub	ecx,edx
	add	esi,edx
.Lu_aligned2:
	mov	edx,ecx
	shr	ecx,2
	and	edx,3
	rep	movsd
	mov	ecx,edx
.Lu_use_movsb:
	rep	movsb
	jmp	.Lu_loop
.Lu_exit:
	mov	eax,edi
	mov	ebx,[ebp+12]
	sub	eax,ebx
	pop	esi
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx
	mov	esp,ebp
	pop	ebp
	ret
	.size	ref_LCW_Uncompress, .-ref_LCW_Uncompress

# ---------------------------------------------------------------------------------------------
# WIN32LIB/WSA/XORDELTA.ASM -- Apply_XOR_Delta(char *target, char *delta)
# ARG target [ebp+8], delta [ebp+12]
# ---------------------------------------------------------------------------------------------
	.globl	ref_Apply_XOR_Delta
	.type	ref_Apply_XOR_Delta, @function
ref_Apply_XOR_Delta:
	push	ebp
	mov	ebp,esp
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi

	mov	edi,[ebp+8]		# get our pointers into offset registers.
	mov	esi,[ebp+12]
	cld				# make sure we go forward
	xor	ecx,ecx			# use cx for loop
.Lx_top_loop:
	xor	eax,eax			# clear out eax.
	mov	al,[esi]		# get delta source byte
	inc	esi
	test	al,al			# check for a SHORTDUMP
	je	.Lx_short_run
	js	.Lx_check_others
	mov	ecx,eax			# stick count in cx
.Lx_dump_loop:
	mov	al,[esi]		# get delta XOR byte
	xor	[edi],al		# xor that byte on the dest
	inc	esi
	inc	edi
	dec	ecx
	jnz	.Lx_dump_loop
	jmp	.Lx_top_loop
.Lx_short_run:
	mov	cl,[esi]		# get count
	inc	esi			# inc delta source
.Lx_do_run:
	mov	al,[esi]		# get XOR byte
	inc	esi
.Lx_run_loop:
	xor	[edi],al		# xor that byte.
	inc	edi			# go to next dest pixel
	dec	ecx			# one less to go.
	jnz	.Lx_run_loop
	jmp	.Lx_top_loop
.Lx_check_others:
	sub	eax,0x80		# opcode -= 0x80
	jnz	.Lx_do_skip		# if zero then get next word, otherwise use remainder.
	mov	ax,[esi]
	lea	esi,[esi+2]		# get word code in ax
	test	ax,ax			# set flags. (not 32bit register so neg flag works)
	jle	.Lx_not_long_skip
.Lx_do_skip:
	add	edi,eax			# do the skip.
	jmp	.Lx_top_loop
.Lx_not_long_skip:
	jz	.Lx_stop		# long count of zero means stop
	sub	eax,0x8000		# opcode -= 0x8000
	test	eax,0x4000		# is it a LONGRUN (code & 0x4000)?
	je	.Lx_long_dump
	sub	eax,0x4000		# opcode -= 0x4000
	mov	ecx,eax			# use cx as loop count
	jmp	.Lx_do_run		# jump to run code.
.Lx_long_dump:
	mov	ecx,eax			# use cx as loop count
	jmp	.Lx_dump_loop		# go to the dump loop.
.Lx_stop:
	pop	esi
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx
	pop	ebp
	ret
	.size	ref_Apply_XOR_Delta, .-ref_Apply_XOR_Delta

# ---------------------------------------------------------------------------------------------
# Apply_XOR_Delta_To_Page_Or_Viewport(void *target, void *delta, int width, int nextrow, int copy)
# ARG target [ebp+8], delta [ebp+12], width [ebp+16], nextrow [ebp+20], copy [ebp+24]
# ---------------------------------------------------------------------------------------------
	.globl	ref_Apply_XOR_Delta_To_Page_Or_Viewport
	.type	ref_Apply_XOR_Delta_To_Page_Or_Viewport, @function
ref_Apply_XOR_Delta_To_Page_Or_Viewport:
	push	ebp
	mov	ebp,esp
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi

	mov	edi,[ebp+8]		# Get the target pointer.
	mov	esi,[ebp+12]		# Get the destination pointer.
	xor	eax,eax			# clear eax, later put them into ecx and edx.
	cld				# make sure we go forward
	mov	ebx,[ebp+20]		# get the amount to add to get to next row from end.
	mov	ecx,eax			# use cx for loop
	mov	edx,eax			# use dx to count the relative column.
	push	ebx			# push nextrow onto the stack for Copy/XOR_Delta_Buffer.
	mov	ebx,[ebp+16]		# bx will hold the max column for speed compares
	cmp	DWORD PTR [ebp+24],0	# Do we want to copy or XOR (DO_XOR = 0)
	je	.Lv_xorfunct		# Jump to XOR if not copy
	call	s07d_ref_Copy_Delta_Buffer
	jmp	.Lv_didcopy		# jump past XOR
.Lv_xorfunct:
	call	s07d_ref_XOR_Delta_Buffer
.Lv_didcopy:
	pop	ebx			# remove the push done to pass a value.
	pop	esi
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx
	pop	ebp
	ret
	.size	ref_Apply_XOR_Delta_To_Page_Or_Viewport, .-ref_Apply_XOR_Delta_To_Page_Or_Viewport

# XOR_Delta_Buffer, ARG nextrow [ebp+8]. Registers in: edi target, esi delta, ebx width, edx column.
	.type	s07d_ref_XOR_Delta_Buffer, @function
s07d_ref_XOR_Delta_Buffer:
	push	ebp
	mov	ebp,esp
.Lvx_top_loop:
	xor	eax,eax			# clear out eax.
	mov	al,[esi]		# get delta source byte
	inc	esi
	test	al,al			# check for a SHORTDUMP
	je	.Lvx_short_run
	js	.Lvx_check_others
	mov	ecx,eax			# stick count in cx
.Lvx_dump_loop:
	mov	al,[esi]		# get delta XOR byte
	xor	[edi],al		# xor that byte on the dest
	inc	esi
	inc	edx			# increment our count on current column
	inc	edi
	cmp	edx,ebx			# are we at the final column
	jne	.Lvx_end_col1		# if not the jmp over the code
	sub	edi,edx			# get our column back to the beginning.
	xor	edx,edx			# zero out our column counter
	add	edi,[ebp+8]		# jump to start of next row
.Lvx_end_col1:
	dec	ecx
	jnz	.Lvx_dump_loop
	jmp	.Lvx_top_loop
.Lvx_short_run:
	mov	cl,[esi]		# get count
	inc	esi			# inc delta source
.Lvx_do_run:
	mov	al,[esi]		# get XOR byte
	inc	esi
.Lvx_run_loop:
	xor	[edi],al		# xor that byte.
	inc	edx			# increment our count on current column
	inc	edi			# go to next dest pixel
	cmp	edx,ebx			# are we at the final column
	jne	.Lvx_end_col2		# if not the jmp over the code
	sub	edi,ebx			# get our column back to the beginning.
	xor	edx,edx			# zero out our column counter
	add	edi,[ebp+8]		# jump to start of next row
.Lvx_end_col2:
	dec	ecx
	jnz	.Lvx_run_loop
	jmp	.Lvx_top_loop
.Lvx_check_others:
	sub	eax,0x80		# opcode -= 0x80
	jnz	.Lvx_do_skip		# if zero then get next word, otherwise use remainder.
	mov	ax,[esi]		# get word code in ax
	lea	esi,[esi+2]
	test	ax,ax			# set flags. (not 32bit register so neg flag works)
	jle	.Lvx_not_long_skip
.Lvx_do_skip:
	sub	edi,edx			# go back to beginning or row.
	add	edx,eax			# incriment our count on current row
.Lvx_recheck3:
	cmp	edx,ebx			# are we past the end of the row
	jb	.Lvx_end_col3		# if not the jmp over the code
	sub	edx,ebx			# Subtract width from the col counter
	add	edi,[ebp+8]		# jump to start of next row
	jmp	.Lvx_recheck3		# jump up to see if we are at the right row
.Lvx_end_col3:
	add	edi,edx			# get to correct position in row.
	jmp	.Lvx_top_loop
.Lvx_not_long_skip:
	jz	.Lvx_stop		# long count of zero means stop
	sub	eax,0x8000		# opcode -= 0x8000
	test	eax,0x4000		# is it a LONGRUN (code & 0x4000)?
	je	.Lvx_long_dump
	sub	eax,0x4000		# opcode -= 0x4000
	mov	ecx,eax			# use cx as loop count
	jmp	.Lvx_do_run		# jump to run code.
.Lvx_long_dump:
	mov	ecx,eax			# use cx as loop count
	jmp	.Lvx_dump_loop		# go to the dump loop.
.Lvx_stop:
	pop	ebp
	ret
	.size	s07d_ref_XOR_Delta_Buffer, .-s07d_ref_XOR_Delta_Buffer

# Copy_Delta_Buffer, ARG nextrow [ebp+8]. Same as above but stores instead of XORing.
	.type	s07d_ref_Copy_Delta_Buffer, @function
s07d_ref_Copy_Delta_Buffer:
	push	ebp
	mov	ebp,esp
.Lvc_top_loop:
	xor	eax,eax			# clear out eax.
	mov	al,[esi]		# get delta source byte
	inc	esi
	test	al,al			# check for a SHORTDUMP
	je	.Lvc_short_run
	js	.Lvc_check_others
	mov	ecx,eax			# stick count in cx
.Lvc_dump_loop:
	mov	al,[esi]		# get delta XOR byte
	mov	[edi],al		# store that byte on the dest
	inc	edx			# increment our count on current column
	inc	esi
	inc	edi
	cmp	edx,ebx			# are we at the final column
	jne	.Lvc_end_col1		# if not the jmp over the code
	sub	edi,edx			# get our column back to the beginning.
	xor	edx,edx			# zero out our column counter
	add	edi,[ebp+8]		# jump to start of next row
.Lvc_end_col1:
	dec	ecx
	jnz	.Lvc_dump_loop
	jmp	.Lvc_top_loop
.Lvc_short_run:
	mov	cl,[esi]		# get count
	inc	esi			# inc delta source
.Lvc_do_run:
	mov	al,[esi]		# get XOR byte
	inc	esi
.Lvc_run_loop:
	mov	[edi],al		# store the byte (instead of XOR against current color)
	inc	edx			# increment our count on current column
	inc	edi			# go to next dest pixel
	cmp	edx,ebx			# are we at the final column
	jne	.Lvc_end_col2		# if not the jmp over the code
	sub	edi,ebx			# get our column back to the beginning.
	xor	edx,edx			# zero out our column counter
	add	edi,[ebp+8]		# jump to start of next row
.Lvc_end_col2:
	dec	ecx
	jnz	.Lvc_run_loop
	jmp	.Lvc_top_loop
.Lvc_check_others:
	sub	eax,0x80		# opcode -= 0x80
	jnz	.Lvc_do_skip		# if zero then get next word, otherwise use remainder.
	mov	ax,[esi]		# get word code in ax
	lea	esi,[esi+2]
	test	ax,ax			# set flags. (not 32bit register so neg flag works)
	jle	.Lvc_not_long_skip
.Lvc_do_skip:
	sub	edi,edx			# go back to beginning or row.
	add	edx,eax			# incriment our count on current row
.Lvc_recheck3:
	cmp	edx,ebx			# are we past the end of the row
	jb	.Lvc_end_col3		# if not the jmp over the code
	sub	edx,ebx			# Subtract width from the col counter
	add	edi,[ebp+8]		# jump to start of next row
	jmp	.Lvc_recheck3		# jump up to see if we are at the right row
.Lvc_end_col3:
	add	edi,edx			# get to correct position in row.
	jmp	.Lvc_top_loop
.Lvc_not_long_skip:
	jz	.Lvc_stop		# long count of zero means stop
	sub	eax,0x8000		# opcode -= 0x8000
	test	eax,0x4000		# is it a LONGRUN (code & 0x4000)?
	je	.Lvc_long_dump
	sub	eax,0x4000		# opcode -= 0x4000
	mov	ecx,eax			# use cx as loop count
	jmp	.Lvc_do_run		# jump to run code.
.Lvc_long_dump:
	mov	ecx,eax			# use cx as loop count
	jmp	.Lvc_dump_loop		# go to the dump loop.
.Lvc_stop:
	pop	ebp
	ret
	.size	s07d_ref_Copy_Delta_Buffer, .-s07d_ref_Copy_Delta_Buffer

# ---------------------------------------------------------------------------------------------
# WIN32LIB/MISC/RANDOM.ASM -- UBYTE Random(VOID), on a caller-supplied copy of RandNumb.
# The original did 'lea esi,[RandNumb]'; here the address is the first argument.
# ---------------------------------------------------------------------------------------------
	.globl	ref_Random
	.type	ref_Random, @function
ref_Random:
	push	esi
	mov	esi,[esp+8]		# (was: lea esi,[RandNumb])
	xor	eax,eax
	mov	al,[esi]
	shr	al,1			# shift right 1 bit (bit0 in carry)
	shr	al,1
	rcl	BYTE PTR [esi+2],1	# rcl byte 3 of RandNumb
	rcl	BYTE PTR [esi+1],1	# rcl byte 2 of RandNumb
	cmc				# complement carry
	sbb	al,[esi]		# sbb byte 1 of RandNumb
	shr	al,1			# sets carry
	rcr	BYTE PTR [esi],1	# rcr byte 1 of RandNumb
	mov	al,[esi]		# reload byte 1 of RandNumb
	xor	al,[esi+1]		# xor with byte 2 of RandNumb
	pop	esi
	ret
	.size	ref_Random, .-ref_Random

# ---------------------------------------------------------------------------------------------
# WIN32LIB/MISC/REVERSE.ASM
# ---------------------------------------------------------------------------------------------
	.globl	ref_Reverse_Long
	.type	ref_Reverse_Long, @function
ref_Reverse_Long:
	mov	eax,[esp+4]
	xchg	al,ah
	ror	eax,16
	xchg	al,ah
	ret
	.size	ref_Reverse_Long, .-ref_Reverse_Long

	.globl	ref_Reverse_Short
	.type	ref_Reverse_Short, @function
ref_Reverse_Short:
	mov	ax,[esp+4]
	xchg	ah,al
	movsx	eax,ax			# (added: GCC/Clang ABI for a 'short' return value)
	ret
	.size	ref_Reverse_Short, .-ref_Reverse_Short

	.globl	ref_Swap_Long
	.type	ref_Swap_Long, @function
ref_Swap_Long:
	mov	eax,[esp+4]
	ror	eax,16
	ret
	.size	ref_Swap_Long, .-ref_Swap_Long

	.att_syntax prefix
	.popsection
)ASM");

#endif // S07D_HAVE_ASM_ORACLE
