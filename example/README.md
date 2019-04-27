### Simple example.

Generate some example numpy arrays and save them. This can be done by running the python script
`generate_example_array.py`.

    $ python generate_example_array.py

Now you can compile and run the example code.

    $ gcc -I.. -Wall -Wextra -O3 -c example.c
    $ gcc -o example example.o -L.. -lc_npy

Now run the example with a file as command line argument.

    $ ./example two_arrays.npz 
    Length: 2
    number of objects in file: 2
    Dimensions   : 1
    Shape        : ( 11)
    Type         : 'i' (8 bytes each element)
    Fortran order: False
    Dimensions   : 1
    Shape        : ( 11)
    Type         : 'f' (4 bytes each element)

The example can read both `.npy` (single array) and `.npz` files.
