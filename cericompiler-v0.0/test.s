			# This code was produced by the CERI Compiler
	.text		# The following lines contain the program
	.globl main	# The main function must be visible from outside
main:			# The main function body :
	movq %rsp, %rbp	# Save the position of the stack's top
	push $3
	push $2
	pop %rbx
	pop %rax
	addq	%rbx, %rax
	push %rax
	push $4
	pop %rbx
	pop %rax
	cmp %rbx, %rax
	setge %al
	movzbq %al, %rax
	push %rax
	movq %rbp, %rsp		# Restore the position of the stack's top
	ret			# Return from main function
