/* malloc.c */

void *malloc();

int main(void)
{
    int n;
    int *p;

    n = 10;
    p = malloc(sizeof *p * n);
}
