	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 11
	.globl	_main
	.align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp0:
	.cfi_def_cfa_offset 16
Ltmp1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp2:
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
Ltmp3:
	.cfi_offset %rbx, -56
Ltmp4:
	.cfi_offset %r12, -48
Ltmp5:
	.cfi_offset %r13, -40
Ltmp6:
	.cfi_offset %r14, -32
Ltmp7:
	.cfi_offset %r15, -24
	movq	_iList@GOTPCREL(%rip), %r12
	movl	$4, %edi
	callq	*320(%r12)
	movq	%rax, %r15
	xorl	%r13d, %r13d
	leaq	-44(%rbp), %r14
	.align	4, 0x90
LBB0_1:                                 ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB0_2 Depth 2
                                        ##     Child Loop BB0_3 Depth 2
	movl	$0, -44(%rbp)
	.align	4, 0x90
LBB0_2:                                 ##   Parent Loop BB0_1 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	movq	%r15, %rdi
	movq	%r14, %rsi
	callq	*160(%r12)
	movl	-44(%rbp), %eax
	incl	%eax
	movl	%eax, -44(%rbp)
	cmpl	$10000, %eax            ## imm = 0x2710
	movl	$10000, %ebx            ## imm = 0x2710
	jl	LBB0_2
	.align	4, 0x90
LBB0_3:                                 ##   Parent Loop BB0_1 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	decq	%rbx
	movq	%r15, %rdi
	movq	%rbx, %rsi
	callq	*200(%r12)
	testq	%rbx, %rbx
	jg	LBB0_3
## BB#4:                                ##   in Loop: Header=BB0_1 Depth=1
	incl	%r13d
	cmpl	$10000, %r13d           ## imm = 0x2710
	jne	LBB0_1
## BB#5:
	xorl	%eax, %eax
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
	.cfi_endproc


.subsections_via_symbols
