# npy_array

A simple library for reading and writing numpy arrays in C code. It is independent
of Python both compile time and runtime.

This tiny C library can read and write Numpy arrays files (`.npy`) into memory and keep it
in a C structure. There is no matrix operations available, just reading and
writing. There is not even methods to set and get elements of the array.

The idea is that you can use `cblas` or something similar for the matrix
operations. I therefore have no intentions of adding such features.

I wrote this to be able to pass Keras saved neural network weight into a format
that can be opened in a C implemented neural network.

Credit should also go to Just Jordi Castells, and [his blogpost](http://jcastellssala.com/2014/02/01/npy-in-c/),
which inspired me to write this.

## The C structure
The structure is pretty self explanatory.

    #define NPY_ARRAY_MAX_DIMENSIONS 8
    typedef struct _npy_array_t {
        char    *data;
        size_t   shape[ NPY_ARRAY_MAX_DIMENSIONS ];
        int32_t  ndim;
        char     endianness;
        char     typechar;
        size_t   elem_size;
        bool     fortran_order;
    } npy_array_t;

## API
The API is really simple. There is only eight public functions:

    npy_array_t * npy_array_matrix_read_file  ( const char *filename);
    void        npy_array_matrix_dump       ( const npy_array_t *m );
    void        npy_array_matrix_write_file ( const char *filename, const npy_array_t *m );
    void        npy_array_matrix_free       ( npy_array_t *m );

    /* Reading an array of matrices from a .npz file. */
    npy_array_t ** npy_array_matrix_array_read  ( const char *filename );
    int          npy_array_matrix_array_write ( const char *filename, const npy_array_t * const *array );
    size_t       npy_array_matrix_array_length( const npy_array_t * const *arr);
    void         npy_array_matrix_array_free  ( npy_array_t **arr );

## Example usage.
Here is a really simple example. You can compile this with:

    gcc -std=gnu99 -Wall -Wextra -O3 -c example.c
    gcc -o example example.o npy_array.o

You can the run example with a numpy file as argument.

    #include "npy_array.h"
    int main(int argc, char *argv[])
    {
        if( argc != 2 ) return -1;
        npy_array_t *m = npy_array_matrix_read_file( argv[1] );
        npy_array_matrix_dump( m );
        npy_array_matrix_write_file( "tester_save.npy", m);
        npy_array_matrix_free( m );
        return 0;
    }

## Compilation/Install
There is now a simple configure file provided (NOT autoconf/automake generated). From scratch:

    ./configure
    make

This will build a static library `libnpy_array.a` which can be linked in to your executable
with the `-lnpy_array` option to the linker. Since this is such alpha stage, I do not
recommend to install this.

## Status
This is written in a full hurry one afternoon, and then modified over some time.
There isn't much of testing performed, and you can read the code to see what is does.
All errors are written to STDERR. Consider this alpha. So, reading and writing of
both `.npy` and `.npz` files seems to work OK -- some obvious bugs of course -- however
there is still no support for `np.savez_compressed()` saved arrays, nor support for saving
such files. (I guess I need to use MiniZip (from zlib) for that, and that creates a dependency.)

## TODO
 * Bugfixes
 * Documentation
 * Cleanup
 * Refactorisation
 
