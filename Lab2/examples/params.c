/* params.c */

int foo(), bar();

int bar(void **q)
{
    return 0;
}

int foo(int x, int y, void *p)
{
    return -x + !y * 10 != 3;
}
