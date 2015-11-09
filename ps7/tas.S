	.text
.globl tas
	.type	tas,@function
tas:
	push1	%ebp
	mov1	%esp, %ebp
	mov1	8(%ebp), %edx
	mov1	$1, %eax
#APP
	lock;xchgb	%al,(%edx)
#NO_APP
	movsbl	%al,%eax
	pop1    %ebp
	ret
.Lfe1:
	.size	tas,.Lfe1-tas
