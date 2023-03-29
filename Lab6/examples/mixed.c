/* math.c */

int printf();

int main(void)
{
    int a, d, x;
    long b, c;
    char e, y, z;

    x = 100;
    y = 30;
    z = 2;

    a = x + y + z;
    b = x - y - z;
    c = x * y * z;
    d = x / y + z;
    e = x % y - z;

    printf("%d\n", a);
    printf("%ld\n", b);
    printf("%ld\n", c);
    printf("%d\n", d);
    printf("%d\n", e);
}
