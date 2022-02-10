# npy_array

A simple library for reading and writing _Numpy_ arrays in C code. It is independent
of _Python_ both compile time and runtime.

This tiny C library can read and write _Numpy_ arrays files (`.npy`) into memory and keep it
in a C structure. There is no matrix operations available, just reading and
writing. There is not even methods to set and get elements of the array.

The idea is that you can use _cblas_ or something similar for the matrix
operations. I therefore have no intentions of adding such features.

I wrote this to be able to pass _Keras_ saved neural network weight into a format
that can be opened in a C implemented neural network.

Credit should also go to _Just Jordi Castells_, and [his blogpost](http://jcastellssala.com/2014/02/01/npy-in-c/),
which inspired me to write this.

### New of summer 2021

The archive (`.npz`) files are now handled by [_libzip_](https://libzip.org/). This redesign
creates a dependency of _libzip_ of course, but it simplifies the code a lot. It also makes it
possible to read and save compressed _Numpy_ arrays. It is therefore added a new public function:

    int
    npy_array_list_save_compressed( const char       *filename,
                                    npy_array_list_t *array_list,
                                    zip_int32_t       comp,
                                    zip_uint32_t      comp_flags);

This new public function will save a `.npz` file using compression based on `comp` and
`comp_flags` which are the same parameters as in _libzip_. 

### Important message if you've used this library before 15th Feb 2020.
I have made some changes huge changes to this library mid February 2020. The main
data structure is renamed from `cmatrix_t` to `npy_array_t` to illustrate better that
this is a _Numpy_ n-dimensional array that is available in C. The structures members
are all the same when it comes to names and types.

The API calls has been changed to reflect the data structure name change. All functions
are renamed.

| Old name               | New name       |
|------------------------|----------------|
| c_npy_matrix_read_file | npy_array_load |
| c_npy_matrix_dump      | npy_array_dump |
| c_npy_matrix_write_file| npy_array_save |
| c_npy_matrix_free      | npy_array_free |

The new names are shorter and more descriptive.

The next big change is that loading `.npz` files no longer returns an array of pointers to
npy_arrays. It will now return a special linked list structure of _Numpy_ arrays, `npy_array_list_t`.

The API calls for `.npz`  has also been changed accordingly.

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
        struct _npy_array_list_t *next;
    } npy_array_list_t;

## API
The API is really simple. There is only ten public functions:

    /* These are the four functions for loading and saving .npy files */
    npy_array_t*      npy_array_load        ( const char *filename);
    void              npy_array_dump        ( const npy_array_t *m );
    void              npy_array_save        ( const char *filename, const npy_array_t *m );
    void              npy_array_free        ( npy_array_t *m );
    
    /* These are the six functions for loading and saving .npz files and lists of numpy arrays */
    npy_array_list_t* npy_array_list_load   ( const char *filename );
    int               npy_array_list_save   ( const char *filename, npy_array_list_t *array_list );
    size_t            npy_array_list_length ( npy_array_list_t *array_list);
    void              npy_array_list_free   ( npy_array_list_t *array_list);
    
    npy_array_list_t* npy_array_list_prepend( npy_array_list_t *list, npy_array_t *array, const char *filename, ...);
    npy_array_list_t* npy_array_list_append ( npy_array_list_t *list, npy_array_t *array, const char *filename, ...);

## Example usage.
Here is a really simple example. You can compile this with:

    gcc -std=gnu99 -Wall -Wextra -O3 -c example.c
    gcc -o example example.o npy_array.o

You can then run example with a _Numpy_ file as argument.

    #include "npy_array.h"
    int main(int argc, char *argv[])
    {
        if( argc != 2 ) return -1;
        npy_array_t *m = npy_array_load( argv[1] );
        npy_array_dump( m );
        npy_array_save( "tester_save.npy", m);
        npy_array_free( m );
        return 0;
    }

## Compilation/Install
There is now a simple configure file provided (NOT autoconf/automake generated). From scratch:

    ./configure --prefix=/usr/local/
    make
    sudo make install

Please see the `INSTALL.md` file for further compilation options.

## Status
This is written in a full hurry one afternoon, and then modified over some time.
There isn't much of testing performed, and you can read the code to see what is does.
All errors are written to STDERR. So, reading and writing of both `.npy` and `.npz`
files seems to work OK -- some obvious bugs of course -- 

## TODO
 * Bugfixes
 * Documentation
 * Cleanup
 * Refactorisation
 
