#include <stdio.h>
#include <stdint.h>
#include <assert.h>

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
    // return ((from ))
    return(0);
}

void shift_and_round(uint32_t* val_to_shift, int bits_to_shift)
{

    /*
    This array give us the following table for mask lookups:

    MASK INDEX | MASK VALUE | MASK BINARYVALUE
    ---------------------------------------------
    0            0           0000_0000
    1            1           0000_0001 
    2            3           0000_0011
    3            7           0000_0111
    4            0xf         0000_1111
    -----------------------
    and so on until we cover the 23 bits possible to shift
    */

    static unsigned masks[24] = {
        0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f,
        0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff,
        0xffff, 0x1ffff, 0x3ffff, 0x7ffff, 0xfffff, 0x1fffff, 0x3fffff, 0x7fffff,
        
    };

    /*
    This mask array give us the following:
    MASK INDEX    |    MASK VALUE  | BINARY
    -----------------------------------------------
    0                    0            0000_0000
    1                    1            0000_0001
    2                    2            0000_0010
    3                    4            0000_0100         
    4                    0x8          0000_1000
    5                    0x10         0001_0000
    ------------------------------------------------
    and so on. 
    */
    static unsigned HO_mask[24] = {
        0, 1, 2, 4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000,
        0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000
    };


    // store the value that will be shifted out here
    int shifted_out;

    // since we are doing a single precission floating point
    // value we can only shift max 23 bits
    assert(bits_to_shift <= 23);
    
    /*
    to get the value that will be shifted it out we mask 
    the value to be shifted with a value that will give us
    this value as a result, for example:
    
    suppose we have the value 0000_1011 and we want to shift
    to the right by 3, so >> 3. We use the mask array above 
    indexed at 3 whic is 7, in binary representation this is:

    0000_00111 and so if we can mask our value to shift with this

    0000_1011 &
    0000_0111 
    -----------
    0000_0011 <----- this is the value to be shifted out 
    */
    
    shifted_out = *val_to_shift & masks[bits_to_shift];

    // perform the shift, only after we have stored the 
    // value that go shifted out
    *val_to_shift = *val_to_shift >> bits_to_shift;


    /*
    Based on the IEEE standards we do following to determine how to round:

    1. if the last bit shifted out was a 1 and there was at least one other 1 in the 
        bits shifted out then increment the mantisa by 1
    2. if the last bit shifted out was a 1 and all the other bits in the shift were
        zero then round the mantisa up by 1 if the mantisas LO bit contains a 1.

    
    building on the previous example we want to shift (to the right) the value: 0000_1011
    we obtain the value shifted out by masking this with 0000_0111 therefore the value 
    to be shifted out is: 0000_0011

    */
    if (shifted_out > HO_mask[bits_to_shift]) {

        // to test whether the values shifted out contained a 1 and AT LEAST one other
        // one in the shifted out bits, we can simply test whether the shifted out value is larger
        // than HO mask value, this can only be true if in fact the shifted out value contained 
        // a 1 in HO bit and an additional 1 somewhere in the remaining shifted out bits 
        *val_to_shift = *val_to_shift + 1;
    } 
    else if (shifted_out == HO_mask[bits_to_shift]) {
        *val_to_shift = *val_to_shift + (*val_to_shift & 1);
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
