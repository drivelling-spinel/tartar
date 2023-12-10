//================
//
// R_DrawColumn
//
//================
//
// 2/15/98 Lee Killough
//
// Converted C code with TFE fix to assembly and tuned
//
// 2/21/98 killough: added translucency support
//
// 11/98 killough: added hires support
//
//================

 .text
 .align 8
 .globl _R_DrawColumn2
_R_DrawColumn2:
 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl _dc_yh,%esi
 movl _dc_yl,%edx
 movl _dc_x,%eax
 incl %esi
 movl _ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle end2
 addl _columnofs(,%eax,4),%ebx
 movl _dc_texheight,%eax
 subl _centery,%edx
 movl _dc_source,%ebp
 imull _dc_iscale,%edx
 leal -1(%eax),%ecx
 movl _dc_colormap,%edi
 addl _dc_texturemid,%edx

// killough 11/98: add hires support
 jmp hi2

 .align 8
end2:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
hi2:
 testl %eax,%ecx
 je powerof2_hi2
 sall $16,%eax

red1_hi2:
 subl %eax,%edx
 jge red1_hi2

red2_hi2:
 addl %eax,%edx
 jl red2_hi2

 .align 8,0x90
nonp2loop_hi2:
 movl %edx,%ecx
 sarl $16,%ecx
 addl _dc_iscale,%edx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)
 addl $1280,%ebx
 cmpl %eax,%edx
 jge wraparound_hi2
 decl %esi
 jg nonp2loop_hi2
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_hi2:
 subl %eax,%edx
 decl %esi
 jg nonp2loop_hi2
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_hi2:
 movl %edx,%eax
 sarl $16,%eax
 addl _dc_iscale,%edx
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 movl %edx,%eax
 addl _dc_iscale,%edx
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 addl $2560,%ebx
 movb (%eax,%edi),%al
 movb %al,-1280(%ebx)

powerof2_hi2:
 addl $-2,%esi
 jge p2loop_hi2
 jnp end2
 sarl $16,%edx
 andl %ecx,%edx
 xorl %eax,%eax
 movb (%edx,%ebp),%al
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

//================
//
// R_DrawTLColumn
//
// Translucency support
//
//================

 .align 8
 .globl _R_DrawTLColumn2
_R_DrawTLColumn2:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl _dc_yh,%esi
 movl _dc_yl,%edx
 movl _dc_x,%eax
 incl %esi
 movl _ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle end_tl2
 addl _columnofs(,%eax,4),%ebx
 movl _dc_texheight,%eax
 subl _centery,%edx
 movl _dc_source,%ebp
 imull _dc_iscale,%edx
 leal -1(%eax),%ecx
 movl _dc_colormap,%edi
 addl _dc_texturemid,%edx

// killough 11/98: add hires support

 jmp hi_tl2

 .align 8
end_tl2:
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

.align 8
hi_tl2:

 testl %eax,%ecx
 pushl %ecx
 je powerof2_tl_hi2
 sall $16,%eax

red1_tl_hi2:
 subl %eax,%edx
 jge red1_tl_hi2

red2_tl_hi2:
 addl %eax,%edx
 jl red2_tl_hi2
 pushl %esi

 .align 8,0x90
nonp2loop_tl_hi2:
 xorl %ecx,%ecx
 movl %edx,%esi
 movb (%ebx),%cl
 shll $8,%ecx
 sarl $16,%esi
 addl _tranmap,%ecx
 addl _dc_iscale,%edx
 movzbl (%esi,%ebp),%esi
 movzbl (%edi,%esi),%esi
 movb (%ecx,%esi),%cl
 movb %cl,(%ebx)
 addl $1280,%ebx
 cmpl %eax,%edx
 jge wraparound_tl_hi2
 decl (%esp)
 jg nonp2loop_tl_hi2
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_tl_hi2:
 subl %eax,%edx
 decl (%esp)
 jg nonp2loop_tl_hi2
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_tl_hi2:
 movl %edx,%eax
 xorl %ecx,%ecx
 addl _dc_iscale,%edx
 movb (%ebx),%cl
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl _tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 xorl %ecx,%ecx
 movb %al,(%ebx)
 movb 1280(%ebx),%cl
 movl %edx,%eax
 addl _dc_iscale,%edx
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl _tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 movb %al,1280(%ebx)
 addl $2560,%ebx

powerof2_tl_hi2:
 addl $-2,%esi
 jge p2loop_tl_hi2
 jnp end_tl2
 xorl %ecx,%ecx
 sarl $16,%edx
 movb (%ebx),%cl
 andl (%esp),%edx
 shll $8,%ecx
 xorl %eax,%eax
 addl _tranmap,%ecx
 movb (%edx,%ebp),%al
 movzbl (%eax,%edi),%eax
 movb (%ecx,%eax),%al 
 movb %al,(%ebx)
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

//----------------------------------------------------------------------------
// $Id: drawcol.s,v 1.1.1.1 2000-07-29 13:20:39 fraggle Exp $
//----------------------------------------------------------------------------
//
// $Log: drawcol.s,v $
// Revision 1.1.1.1  2000-07-29 13:20:39  fraggle
// imported sources
//
// Revision 1.3  1998/03/04  12:33:29  killough
// Fix problem with last translucent pixel being drawn
//
// Revision 1.2  1998/02/23  04:18:24  killough
// Add translucency support, more tuning
//
// Revision 1.1  1998/02/17  06:37:37  killough
// Initial version
//
//
//----------------------------------------------------------------------------
