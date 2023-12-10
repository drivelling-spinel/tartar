//================
//
// R_DrawCheckers/R_DrawCheckers2
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


 .align 8
 .globl _R_DrawCheckers2
_R_DrawCheckers2:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx

 movl _dc_x,%eax
 xorl _dc_yl,%eax
 movl %eax,_dc_faux 

 movl _dc_yh,%esi
 movl _dc_yl,%edx
 movl _dc_x,%eax
 incl %esi
 movl _ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle end_ch2
 addl _columnofs(,%eax,4),%ebx
 movl _dc_texheight,%eax
 subl _centery,%edx
 movl _dc_source,%ebp
 imull _dc_iscale,%edx
 leal -1(%eax),%ecx
 movl _dc_colormap,%edi
 addl _dc_texturemid,%edx

// killough 11/98: add hires support

 jmp hi_ch2

 .align 8
end_ch2:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

.align 8
hi_ch2:
 testl %eax,%ecx
 je powerof2_ch_hi2
 sall $16,%eax

red1_ch_hi2:
 subl %eax,%edx
 jge red1_ch_hi2

red2_ch_hi2:
 addl %eax,%edx
 jl red2_ch_hi2
 
 .align 8,0x90
nonp2loop_ch_hi2:
 movl _dc_faux,%ecx
 andb $1,%cl
 jz   nonp2loop_ch_skip_hi2
 movl %edx,%ecx
 sarl $16,%ecx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)

nonp2loop_ch_skip_hi2:
 addl _dc_iscale,%edx
 addl $1280,%ebx
 incl _dc_faux
 cmpl %eax,%edx
 jge wraparound_ch_hi2
 decl %esi
 jg nonp2loop_ch_hi2
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_ch_hi2:
 subl %eax,%edx
 decl %esi
 jg nonp2loop_ch_hi2
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_ch_hi2:
 movl _dc_faux,%eax
 andb $1,%al
 jz   p2loop_ch_skip_hi2
 movl %edx,%eax
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)

p2loop_ch_skip_hi2:
 addl _dc_iscale,%edx
 movl _dc_faux,%eax
 andb $1,%al
 jnz  p2loop_ch_skip2_hi2
 movl %edx,%eax
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,1280(%ebx)

p2loop_ch_skip2_hi2:
 addl _dc_iscale,%edx
 addl $2560,%ebx

powerof2_ch_hi2:
 addl $-2,%esi
 jge  p2loop_ch_hi2
 jnp  end_ch2
 movl _dc_faux,%eax
 andb $1,%al
 jz   end_ch2
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



 .align 8
 .globl _R_DrawCheckers
_R_DrawCheckers:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx

 movl _dc_x,%eax
 xorl _dc_yl,%eax
 movl %eax,_dc_faux 

 movl _dc_yh,%esi
 movl _dc_yl,%edx
 movl _dc_x,%eax
 incl %esi
 movl _ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle end_ch
 addl _columnofs(,%eax,4),%ebx
 movl _dc_texheight,%eax
 subl _centery,%edx
 movl _dc_source,%ebp
 imull _dc_iscale,%edx
 leal -1(%eax),%ecx
 movl _dc_colormap,%edi
 addl _dc_texturemid,%edx

 cmpl $0, _hires
 jne hi_ch

 testl %eax,%ecx
 je powerof2_ch
 sall $16,%eax

red1_ch:
 subl %eax,%edx
 jge red1_ch

red2_ch:
 addl %eax,%edx
 jl red2_ch

 .align 8,0x90
nonp2loop_ch:
 movl _dc_faux,%ecx
 andb $1,%cl
 jz   nonp2loop_ch_skip
 movl %edx,%ecx
 sarl $16,%ecx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)

nonp2loop_ch_skip:
 addl _dc_iscale,%edx
 addl $320,%ebx
 incl _dc_faux
 cmpl %eax,%edx
 jge  wraparound_ch
 decl %esi
 jg   nonp2loop_ch
 popl %ebx
 popl %ecx
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_ch:
 subl %eax,%edx
 decl %esi
 jg   nonp2loop_ch
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
end_ch:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_ch:
 movl _dc_faux,%eax
 andb $1,%al
 jz   p2loop_ch_skip
 movl %edx,%eax
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)

p2loop_ch_skip:
 addl _dc_iscale,%edx
 movl _dc_faux,%eax
 andb $1,%al
 jnz  p2loop_ch_skip2
 movl %edx,%eax
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,320(%ebx)
p2loop_ch_skip2:
 addl _dc_iscale,%edx
 addl $640,%ebx

powerof2_ch:
 addl $-2,%esi
 jge  p2loop_ch
 jnp  end_ch
 movl _dc_faux,%eax
 andb $1,%al
 jz   end_ch
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

// killough 11/98: add hires support

hi_ch:
 testl %eax,%ecx
 je powerof2_ch_hi
 sall $16,%eax

red1_ch_hi:
 subl %eax,%edx
 jge red1_ch_hi

red2_ch_hi:
 addl %eax,%edx
 jl red2_ch_hi

 .align 8,0x90
nonp2loop_ch_hi:
 movl _dc_faux,%ecx
 andb $1,%cl
 jz   nonp2loop_ch_skip_hi
 movl %edx,%ecx
 sarl $16,%ecx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)

nonp2loop_ch_skip_hi:
 addl _dc_iscale,%edx
 addl $640,%ebx
 incl _dc_faux
 cmpl %eax,%edx
 jge wraparound_ch_hi
 decl %esi
 jg nonp2loop_ch_hi
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_ch_hi:
 subl %eax,%edx
 jg nonp2loop_ch_hi
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_ch_hi:
 movl _dc_faux,%eax
 andb $1,%al
 jz   p2loop_ch_skip_hi
 movl %edx,%eax
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)

p2loop_ch_skip_hi:
 addl _dc_iscale,%edx
 movl _dc_faux,%eax
 andb $1,%al
 jnz  p2loop_ch_skip2_hi
 movl %edx,%eax
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,640(%ebx)

p2loop_ch_skip2_hi:
 addl _dc_iscale,%edx
 addl $1280,%ebx

powerof2_ch_hi:
 addl $-2,%esi
 jge  p2loop_ch_hi
 jnp  end_ch
 movl _dc_faux,%eax
 andb $1,%al
 jz   end_ch
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

