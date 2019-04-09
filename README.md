# c_npy

A simple library for reading and writing numpy arrays in C code. It is independent
of Python both compile time and runtime.

This tiny C library can read and write Numpy arrays files (`.npy`) into memory and keep it
in a C structure. There is no matrix operations available, just reading and
writing. There is not even methods to set and get elements of the array.

The idea is that you can use `cblas` or something similar for the matrix
operations. I therefore have no intentions of adding such features.

I wrote this to be able to pass Keras saved neural network weight into a format
that can be opened in a C implementd neural network.

Credit should also go to Just Jordi Castells, and [his blogpost](http://jcastellssala.com/2014/02/01/npy-in-c/),
which inspired me to write this.

## The C structure
The structure is pretty self explanatory.

    #define C_NPY_MAX_DIMENSIONS 8
    typedef struct _cmatrix_t {
        char    *data;
        size_t   shape[ C_NPY_MAX_DIMENSIONS ];
        int32_t  ndim;
        char     endianness;
        char     typechar;
        size_t   elem_size;
        bool     fortran_order;
    } cmatrix_t;

## API
The API is really simple. There is only seven functions:

    cmatrix_t * c_npy_matrix_read_file  ( const char *filename);
    void        c_npy_matrix_dump       ( const cmatrix_t *m );
    void        c_npy_matrix_write_file ( const char *filename, const cmatrix_t *m );
    void        c_npy_matrix_free       ( cmatrix_t *m );

    /* Reading an array of matrices from a .npz file. */
    cmatrix_t ** c_npy_matrix_array_read  ( const char *filename );
    int          c_npy_matrix_array_write ( const char *filename, const cmatrix_t **array );
    size_t       c_npy_matrix_array_length( const cmatrix_t **arr);
    void         c_npy_matrix_array_free  ( cmatrix_t **arr );

## Example usage.
Here is a really simple example. You can compile this with:

    gcc -std=gnu99 -Wall -Wextra -O3 -c example.c
    gcc -o example example.o c_npy.o

You can the run example with a numpy file as argument.

    #include "c_npy.h"
    int main(int argc, char *argv[])
    {
        if( argc != 2 ) return -1;
        cmatrix_t *m = c_npy_matrix_read_file( argv[1] );
        c_npy_matrix_dump( m );
        c_npy_matrix_write_file( "tester_save.npy", m);
        c_npy_matrix_free( m );
        return 0;
    }

## Compilation/Install
There is only one object file: `c_npy.o` if you only want single `.npy` file.
If you need reading and writing of `.npz` files, you need to compile the other files as well.
This will be compiled if you type `make`, however this is really primitive. The object files
can be statically linked in to your executable. TODO: Make a lib archive.

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
 * Nicer install/make a dynamic library or at least an archive lib.


