#include <stdio.h>

int main(void)
{

    printf("Bit operations -----------------------------\n");
    // printf("--------------------------------------------\n");

    unsigned char x = 0b00000010; //2 
    unsigned char y = 0b00001000; // 2^3 = 8
    unsigned char z = 0b11111101;

    printf("the size of a 'char' is: %llu byte\n", sizeof(char));
    printf("the size of a 'int' is: %llu byte\n", sizeof(int));

    printf("the value of x is %u\n", x);
    printf("the value of y is %u\n", y);

    printf("the sum of these values is %u\n", (y + x));
    printf("the AND of these values is %u\n", (y & x));
    printf("the OR of these values is %u\n", (y | x));
    printf("the XOR of these values is %u\n", (y ^ x));
    printf("the value of x is: %u and the negation of x is %u\n", x, ~x);
    printf("the value of z: %u\n", z);

    return(0);
}