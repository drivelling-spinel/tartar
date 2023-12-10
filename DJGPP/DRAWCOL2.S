//================
//
// R_DrawColumn2
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
 jle end
 addl _columnofs(,%eax,4),%ebx
 movl _dc_texheight,%eax
 subl _centery,%edx
 movl _dc_source,%ebp
 imull _dc_iscale,%edx
 leal -1(%eax),%ecx
 movl _dc_colormap,%edi
 addl _dc_texturemid,%edx

// killough 11/98: add hires support

 .align 8
hi:
 testl %eax,%ecx
 je powerof2_hi
 sall $16,%eax

red1_hi:
 subl %eax,%edx
 jge red1_hi

red2_hi:
 addl %eax,%edx
 jl red2_hi

 .align 8,0x90
nonp2loop_hi:
 movl %edx,%ecx
 sarl $16,%ecx
 addl _dc_iscale,%edx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)
 addl $640,%ebx
 cmpl %eax,%edx
 jge wraparound_hi
 decl %esi
 jg nonp2loop_hi
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_hi:
 subl %eax,%edx
 decl %esi
 jg nonp2loop_hi
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_hi:
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
 addl $1280,%ebx
 movb (%eax,%edi),%al
 movb %al,-640(%ebx)

powerof2_hi:
 addl $-2,%esi
 jge p2loop_hi
 jnp end
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
 jle end_tl
 addl _columnofs(,%eax,4),%ebx
 movl _dc_texheight,%eax
 subl _centery,%edx
 movl _dc_source,%ebp
 imull _dc_iscale,%edx
 leal -1(%eax),%ecx
 movl _dc_colormap,%edi
 addl _dc_texturemid,%edx

// killough 11/98: add hires support
 testl %eax,%ecx
 pushl %ecx
 je powerof2_tl_hi
 sall $16,%eax

red1_tl_hi:
 subl %eax,%edx
 jge red1_tl_hi

red2_tl_hi:
 addl %eax,%edx
 jl red2_tl_hi
 pushl %esi

 .align 8,0x90
nonp2loop_tl_hi:
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
 addl $640,%ebx
 cmpl %eax,%edx
 jge wraparound_tl_hi
 decl (%esp)
 jg nonp2loop_tl_hi
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_tl_hi:
 subl %eax,%edx
 decl (%esp)
 jg nonp2loop_tl_hi
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_tl_hi:
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
 movb 640(%ebx),%cl
 movl %edx,%eax
 addl _dc_iscale,%edx
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl _tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 movb %al,640(%ebx)
 addl $1280,%ebx

powerof2_tl_hi:
 addl $-2,%esi
 jge p2loop_tl_hi
 jnp end_tl
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

 .align 8
end_tl:
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret
