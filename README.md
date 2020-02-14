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

### Important message if you've used this library before 15th Feb 2020.
I have made some changes huge changes to this library mid february 2020. The main
data structure is renamed from `cmatrix_t` to `npy_array_t` to illistrate better that
this is a numpy n-dimentional array that is available in C. The structures members
are all the same when it comes to names and types.

The API calls has been change to reflect the datastructure name change. All functions
are renamed.

| Old name               | New name       |
|------------------------|----------------|
| c_npy_matrix_read_file | npy_array_load |
| c_npy_matrix_dump      | npy_array_dump |
| c_npy_matrix_write_file| npy_array_save |
| c_npy_matrix_free      | npy_array_free |

The new names are shorter and more decriptive.

The next big change is that loading `.npz` files no longer returns an array of pointers to
npy_arrays. It will now return a special linked list structure of numpy arrays, `npy_array_list_t`.

The API calls for `.npz`  has also been changed accoringly.

| Old name                 | New name             |
|--------------------------|----------------------|
| c_npy_matrix_array_read  | npy_array_list_load  |
| c_npy_matrix_array_write | npy_array_list_save  |
| c_npy_matrix_array_length| npy_array_list_length|
| c_npy_matrix_array_free  | npy_array_list_free  |


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

And the linked list structure for `.npz` files:

    typedef struct _npy_array_list_t {
        npy_array_t      *array;
        char             *filename;
        uint32_t          crc32;
        struct _npy_array_list_t *next;
    } npy_array_list_t;

## API
The API is really simple. There is only ten public functions:

    npy_array_t*      npy_array_load        ( const char *filename);
    void              npy_array_dump        ( const npy_array_t *m );
    void              npy_array_save        ( const char *filename, const npy_array_t *m );
    void              npy_array_free        ( npy_array_t *m );
    
    npy_array_list_t* npy_array_list_load   ( const char *filename );
    int               npy_array_list_save   ( const char *filename, npy_array_list_t *array_list );
    size_t            npy_array_list_length ( npy_array_list_t *array_list);
    void              npy_array_list_free   ( npy_array_list_t *array_list);
    
    npy_array_list_t* npy_array_list_prepend( npy_array_list_t *list, npy_array_t *array, char *filename);
    npy_array_list_t* npy_array_list_append ( npy_array_list_t *list, npy_array_t *array, char *filename);

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
 
