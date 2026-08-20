	.text
	.globl ifunc_a_resolver
	.type ifunc_a_resolver,@function
ifunc_a_resolver:
	mov ifunc_dependency_function@GOTPCREL(%rip), %rax
	ret
	.size ifunc_a_resolver,.-ifunc_a_resolver

	.globl ifunc_a_value
	.type ifunc_a_value,@gnu_indirect_function
	.hidden ifunc_a_value
	.set ifunc_a_value,ifunc_a_resolver

	.globl ifunc_a_exported
	.type ifunc_a_exported,@gnu_indirect_function
	.set ifunc_a_exported,ifunc_a_resolver

	.data
	.globl ifunc_a_pointer
	.type ifunc_a_pointer,@object
ifunc_a_pointer:
	.quad ifunc_a_value
	.size ifunc_a_pointer,.-ifunc_a_pointer
