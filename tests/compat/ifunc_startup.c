extern int (*ifunc_a_pointer)(void);

int main(void) {
	if (ifunc_a_pointer == 0)
		return 1;
	return ifunc_a_pointer() == 41 ? 0 : 1;
}
