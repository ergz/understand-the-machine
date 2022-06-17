#include <stdio.h>
#include <stdint.h>

#define asreal(x) (*((float *) &x))

typedef uint32_t real;

void fpadd(real left, real right, real* dest);

void fpsub(real left, real right, real* dest) 
{
    right = right ^ 0x80000000; // negate the right operand
    fpadd(left, right, dest); // fpadd does all the work
}

inline int extract_sign(real from) 
{
    return (from >> 31);
}

inline int extract_exponent(real from) 
{
    return ((from >> 23) & 0xff) - 127;
}

inline int extract_mantisa(real from) 
{
    if ((from & 0x7fffffff) == 0) return 0;
    return ((from ))
}

void shift_and_round(uint32_t* val_to_shift, int bit_to_shift)
{
    static unsigned masks[24] = {
        0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f,

    }
}

int main(void)
{

    printf("Bit operations -----------------------------\n");
    // printf("--------------------------------------------\n");

    unsigned char x = 0b00000010; //2 
    unsigned char y = 0b00001000; // 2^3 = 8
    unsigned char z = 0b11111101;

    printf("the size of a 'char' is: %llu byte\n", sizeof(char));
    printf("the size of a 'int' is: %llu byte\n", sizeof(int));
    printf("the size of a 'short int' is: %llu byte\n", sizeof(short int));

    printf("the value of x is %u\n", x);
    printf("the value of y is %u\n", y);

    printf("the sum of these values is %u\n", (y + x));
    printf("the AND of these values is %u\n", (y & x));
    printf("the OR of these values is %u\n", (y | x));
    printf("the XOR of these values is %u\n", (y ^ x));
    printf("the value of x is: %u and the negation of x is %u\n", x, ~x);
    printf("the value of z: %u\n", z);

    printf("the value of x: %u, shifted one to the left is: %u\n", x, (x<<1));
    printf("the value of x: %u, shifted two to the left is: %u\n", x, (x<<2));

    printf("Packing -----------------------------\n");

    /* in the book they talk about ssn as an example for packing data */
    // 111 22 3333
    int ssn = 0b00011011110011010000010100010110;
    printf("the value of the ssn is: %u\n", ssn);

    // to get the second value out of this, we can mask it with an binary number 
    // that has all zeros except 1's in the last 8 bits, this is just the hex 0x7f
    char second_field = ssn & 0x7f;
    short third_field = (ssn >> 8) & 0x3FFF;
    short first_field = ssn >> 22;
    printf("the value of the second_field is: %u\n", second_field);
    printf("the value of the third_field is: %u\n", third_field);
    printf("the value of the first_field is: %u\n", first_field);


    return(0);
}