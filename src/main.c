#include <stdio.h>
#include <stdint.h>
#include <assert.h>


/* 
what does this macro do, here are the steps that are happening and why
we can call like we do below (asreal) = 1.0;

1. define x to be type real (uint32_t)      // uint32_t x;
2. next we cast this to be float pointer    // (float*) &x;
3. last we dereference to assign it a value // *x = 1.0;

I don't know why but this was confusing, also why isn't this
just a regular function?

*/
#define asreal(x) (*( ( float* ) &x ) )


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
    return ((from & 0x7fffff) | 0x800000);
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

    static unsigned masks[24] = 
    {
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
    static unsigned HO_mask[24] = 
    {
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

inline real pack_fp(int sign, int exponent, int mantisa) 
{
    return(
        (real)
        (
            (sign << 31) |
            ((exponent + 127) << 23) | 
            (mantisa & 0x7fffff)
        ));
}

void fpadd(real left, real right, real* dest)
{
    // extrasct different components of the real value inputs into our addition function
    int         left_exponent;
    uint32_t    left_mantissa;
    int         left_sign;

    int         right_exponent;
    uint32_t    right_mantissa;
    int         right_sign;

    int         dest_exponent;
    uint32_t    dest_mantissa;
    int         dest_sign;

    left_exponent = extract_exponent(left);
    left_mantissa = extract_mantisa(left);
    left_sign = extract_sign(left);
    
    right_exponent = extract_exponent(right);
    right_mantissa = extract_mantisa(right);
    right_sign = extract_sign(right);
    
    // Inifinity Cases ----------------------------------------------
    /*
    infinity: when the exponent contains all 1 buts, and the matissa contails all 0's
    then we either have +inf or -inf depending on the sign bit

    From wiki: 
    0 11111111 000000000000000000000002 = 7f80 000016 = infinity
    1 11111111 000000000000000000000002 = ff80 000016 = −infinity
    */
    if (left_exponent == 127) { // all the bits are 1's 

        if (left_mantissa == 0) {
            // if the corresponding mantissa is zero, then we have an infinity 
            // either + or - depending on the sign bit

            if (right_exponent == 127) { 

                if (right_mantissa == 0) {
                    // if the correpodning right mantissa is all zeros then we have 
                    // either + or - inf.
                    if (left_sign == right_sign) {

                        *dest = right;
                    } 
                    else {
                        *dest = 0x7fc00000; // NAN
                    }
                }
                else {
                    *dest = right; // its some type of NAN since it has all 1's in the exponent and at least one 
                                    // position on the mantissa in non-zero
                }
            }
        }
        else {
            *dest = left; // propogate the NAN
        }

        return; 
    }
    else if (right_exponent == 127) {
        *dest = right;
        return;
    }


    // Actual number addition ------------------------------------------

    /*
    In order to add the two values we must first make sure that their 
    exponents are the same, if they are not we will shift (denormalize) the 
    smaller of the two values and do the appropriate rounding.
    */
    dest_exponent = right_exponent;

    if (right_exponent > left_exponent) {
        shift_and_round(&left_mantissa, (right_exponent - left_exponent));
    } 
    else if (right_exponent < left_exponent) {
        shift_and_round(&right_mantissa, (left_exponent - right_exponent));
        dest_exponent = left_exponent;
    }

    if (right_sign ^ left_sign) { // XOR is true only if the sign are different
        if (left_mantissa > right_mantissa) {
            dest_mantissa = right_mantissa - left_mantissa;
            dest_sign = left_sign;
        }
        else {
            dest_mantissa = left_mantissa - right_mantissa;
            dest_sign = right_sign;
        }
    }
    else {
        dest_sign = left_sign;
        dest_mantissa = left_mantissa + right_mantissa;
    }

    // Normalize the result --------------------------------------

    // when overflow happens during addition
    if (dest_mantissa >= 0x1000000) {
        shift_and_round(&dest_mantissa, 1);
        ++dest_exponent;
    }
    else {

        if (dest_mantissa != 0) {
            while ((dest_mantissa < 0x800000) && (dest_exponent > -127)) {
                dest_mantissa = dest_mantissa << 1;
                --dest_exponent;
            }
        }
        else {
            dest_sign = 0;
            dest_exponent = 0;
        }
    }

    *dest = pack_fp(dest_sign, dest_exponent, dest_mantissa);

}

int main(int argc, char* argv[])
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

    printf("---------------------------------------------\n");
    printf("Real value stuff\n");

    real l, r, d;

    asreal(l) = 1.3; // changing the pointer to be of float type
    asreal(r) = 2.7; // changing the pointer to be of float type
    fpadd(l, r, &d);

    printf("left:           0x%X    se: %E\n", l, asreal(l));
    printf("right:          0x%X    se: %E +\n", r, asreal(r));

    printf("                ----------------------------------\n");

    printf("dest:           0x%X    se: %E\n", d, asreal(d));

    return(0);
}

