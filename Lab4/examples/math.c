int x, y, *p;
long m, n, *q;
char c, d, *s, *t;

int main(void)
{
    x || c && m;
    c && m || x;
    q || s && p;

    x == y != n;
    c != n == d;

    x < y > m;
    c > n < x;
    x <= n >= x;
    c >= n <= d;

    c + x - n;
    x - n + c;

    c * x / n;
    x / y * n;
    x % d * m;

    s + x - m;
    m + p - d;
    p + q;			/* invalid operands to binary + */

    p - q;			/* invalid operands to binary - */
    s - p + x;			/* invalid operands to binary - */
    s - t + x;

    - x;
    - m;
    - p;			/* invalid operand to unary - */

    ! x;
    ! m;
    ! p;
    ! x * m;
    ! p * n;

    &p + x;
    &s - x;

    x + sizeof p;
    x + sizeof sizeof c;
}
