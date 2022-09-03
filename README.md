# npy_array
## An ANSI C library for handling NumPy arrays

A simple library for reading and writing _NumPy_ arrays in C code. It is independent
of _Python_ both compile time and runtime.

This tiny C library can read and write _NumPy_ arrays files (`.npy`) into memory and keep it
in a C structure. There is no matrix operations available, just reading and
writing. There is not even methods to set and get elements of the array.

The idea is that you can use _cblas_ or something similar for the matrix
operations. I therefore have no intentions of adding such features.

I wrote this to be able to pass _Keras_ saved neural network weight into a format
that can be opened in a C implemented neural network.

Credit should also go to _Just Jordi Castells_, and [his blogpost](http://jcastellssala.com/2014/02/01/npy-in-c/),
which inspired me to write this.

### License

This software is licensed under BSD (3 clause) license.

### New of summer 2022

Added support for memory mapping (`mmap()`) of arrays instead of reading them into memory.
So far this feature will only map a `.npy` file read-only in shared and protected memory.
In that sense it is useful for retrieving data from large pre-calculated arrays. There are
several advantages of this: The memory for the file is mapped by the OS, such that the
memory footprint of the running process becomes much smaller, several processes can share
the mapped memory as several processes reads from the same file, and it is much faster as
the file is not read into virtual memory.

The API for mapping, is similar to to loading a file. It's just one more construction function:

    npy_array_t * npy_array_mmap( const char *filename );

It cannot be simpler than that. If you are sure you only need the data to be read only, you can
actually just use this function as a drop-in replacement to `npy_array_load()`. When you are
done with the array, you should be release its resources by calling `npy_array_free( array );`.

There is also a new member in the `npy_array_t` structure: `void *map_addr;`. Do not use this.
Consider it _private_. Do not alter it, as it is used for unmapping when cleaning up.

There are currently no plan to support writing to mmap()'ed arrays. If you need such feature,
please make a pull request, and I will probably merge.

There is also no plan to support memory mapping for `.npz` files.

(Also: `mmap()` is actually POSIX standard and not ANSI. If ANSI compatibility 
is important to you, maybe compile with out these feature.)

### New of summer 2021

The archive (`.npz`) files are now handled by [_libzip_](https://libzip.org/). This redesign
creates a dependency of _libzip_ of course, but it simplifies the code a lot. It also makes it
possible to read and save compressed _NumPy_ arrays. It is therefore added a new public function:

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
this is a _NumPy_ n-dimensional array that is available in C. The structures members
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
npy_arrays. It will now return a special linked list structure of _NumPy_ arrays, `npy_array_list_t`.

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
The API is really simple. There is only ~~ten~~eleven public functions:

    /* These are the four functions for loading and saving .npy files */
    npy_array_t*      npy_array_load        ( const char *filename);
    npy_array_t*      npy_array_mmap        ( const char *filename);
    void              npy_array_dump        ( const npy_array_t *m );
    void              npy_array_save        ( const char *filename, const npy_array_t *m );
    void              npy_array_free        ( npy_array_t *m );
    
    /* These are the six functions for loading and saving .npz files and lists of NumPy arrays */
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

You can then run example with a _NumPy_ file as argument.

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

## Saving other arraylike data as _NumPy_ format.
You may have a pointer to an N-dimensional array, which you want to store as NumPy format, such
that you can load it in Python/Jupiter and plot in matplotlib or whatever you find more
convenient in Python.

The data structure for `npy_array_t` is open and for convenience you can make a new structure
by stack allocation. Here is some example code on how you can save a `.npy` file:

    #include <npy_array.h>
    #include <stdlib.h>
    int main()
    {
        /* set some sizes */
        int n_rows = 4;
        int n_cols = 3;
    
        /* Allocate the raw data - this can be from a blas or another */
        float *arraydata = malloc( n_rows * n_cols * sizeof( float ));
    
        /* fill in some data */
        for( int i = 0; i < n_rows * n_cols; i++ )
            arraydata[i] = (float) i;
    
        npy_array_save( "my_4_by_3_array.npy", 
            NPY_ARRAY_BUILDER( arraydata, SHAPE( n_rows, n_cols ), NPY_DTYPE_FLOAT32 ) );
        
        free( arraydata );
        return 0;
    }

Compile:

    gcc -std=c99 -Wall -Wextra -O3 how_to_save.c -o how_to_save `pkg-config --libs npy_array`

When this is then executed, you can verify that you got the save `.npy` file and that
it's possible to read this in Python/NumPy.

    >>> import numpy as np
    >>> a = np.load("my_4_by_3_array.npy")
    >>> a
    array([[ 0.,  1.,  2.],
           [ 3.,  4.,  5.],
           [ 6.,  7.,  8.],
           [ 9., 10., 11.]], dtype=float32)

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
 
