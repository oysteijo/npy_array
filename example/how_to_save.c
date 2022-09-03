#include <npy_array.h>
#include <stdlib.h>
#include <assert.h>
int main()
{
    /* set some sizes */
    int n_rows = 4;
    int n_cols = 3;

    /* Allocate the raw data - this can be from a blas or another */
    float *arraydata = malloc( n_rows * n_cols * sizeof( float ));                  assert(arraydata);

    /* fill in some data */
    for( int i = 0; i < n_rows * n_cols; i++ )
        arraydata[i] = (float) i;

    npy_array_t array = {
        .data = (char*) arraydata,
        .shape = { n_rows, n_cols },
        .ndim = 2,
        .endianness = '<',
        .typechar = 'f',
        .elem_size = sizeof(float),
    };
    npy_array_save( "my_4_by_3_array.npy", &array );
    
    /*
       However...
       
    In the above code we create an array on the fly, allocated at the stack, using
    designated initalizers. But look at the above code a bit closer. There is pretty
    much redundancy there. Let's comment line-by-line:

    npy_array_t array = {             // Obviously it is a npy_array_t and I even have to give it a name. Why?
        .data = (char*) arraydata,    // I have to set the data pointer, but I do have to cast it to (char*). Silly?
        .shape = { n_rows, n_cols },  // This is necessary - I have to give the shape!
        .ndim = 2,                    // If I gave two elements in the shape, the number of dimensions must be 2! 
        .endianness = '<',            // Endianness (or byteorder) is seldom used and it can quite easily be guessed!
        .typechar = 'f',              // I know what type I have, but I have to tell the save algorithm.
        .elem_size = sizeof(float),   // However, if I know the type, I also know the size. So ...
    };

    To simplify this, we have written a set of preprocessor macros. The two important macros are SHAPE()
    and NPY_ARRAY_BUILDER(). NPY_ARRAY_BUILDER() is a variadic macro and it uses compound literals and
    hides the designated initializers of the above code. There's two mandatory arguments to
    NPY_ARRAY_BUILDER(). First argument is the pointer to the raw data, the macro does the designated
    initializing and the cast. The second argument is the shape. The second argument can be
    assisted further with the SHAPE() macro. This macro is also variadic, and can take up to 8 arguments
    to specify the shape of the array. The macro also counts the number of arguments, and the count will be
    set for the .ndim field.  
    
    For the element sizes and typechar, we have defined a set of preprocessor types which expands
    to the .elem_size and .typechar. All these are named NPY_DTYPE_<TYPE> according to the numpy dtype.
    See npy_array.h for details.

    If .endianness is not set, the system will make a guess based on element size and type. Basic
    guessing strategy is to check if element size is 1 - in that case the endianness doesn't matter.
    Or if elements size is not 1 - then we guess it is the endianess of the system.

    There it is, clear as mud!

    Or maybe you rather like an example? Here's the same as above with the macro magic.       
    */

    npy_array_save( "my_4_by_3_array_shortcut.npy",
            NPY_ARRAY_BUILDER( arraydata, SHAPE( n_rows, n_cols ), NPY_DTYPE_FLOAT32 ) );

    free ( arraydata );

    return 0;
}

