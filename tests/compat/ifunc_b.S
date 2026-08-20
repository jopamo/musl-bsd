	.text
	.globl ifunc_b_resolver
	.type ifunc_b_resolver,@function
ifunc_b_resolver:
	mov ifunc_dependency_function@GOTPCREL(%rip), %rax
	ret
	.size ifunc_b_resolver,.-ifunc_b_resolver

	.globl ifunc_b_value
	.type ifunc_b_value,@gnu_indirect_function
	.hidden ifunc_b_value
	.set ifunc_b_value,ifunc_b_resolver

	.globl ifunc_b_exported
	.type ifunc_b_exported,@gnu_indirect_function
	.set ifunc_b_exported,ifunc_b_resolver

	.data
	.globl ifunc_b_pointer
	.type ifunc_b_pointer,@object
ifunc_b_pointer:
	.quad ifunc_b_value
	.size ifunc_b_pointer,.-ifunc_b_pointer
