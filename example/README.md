## Some simple examples.

This directory contain some examples of how to save and load Numpy arrays with this library.

Compile the four examples with make.

    $ make

This will build four executables in the same directory.

#### `how_to_save`

Please study the code in `how_to_save.c`. If you run the code it will generate two `.npy` files.

 * `my_4_by_3_array.npy`
 * `my_4_by_3_array_shortcut.npy`

Please open a Python REPL, and make sure you can read these.

#### `how_to_save_npz`

Please study the code in `how_to_save_npz.c`. If you run the code it will generate a `.npz` file
containing two NumPy arrays. 

Please open a Python REPL, and make sure you can read the `.npz` file, and that the arrays
looks sensible.

#### `how_to_load`

If you study the code in `how_to_load.c`, you will see how simple it is to load an array from
a file. The example code even shows how you can inteact with the array data. To run the example
it takes a filename as a command line argument. If you ran the above `how_to_save`-example,
you can run:

    $ ./how_to_load my_4_by_3_array.npy

This will load the array and dump some info, and write if the number format is float32.

You can also try to generate a NumPy file from your Python REPL, and load it with the example.

#### `how_to_load_npz`

Please study the code in `how_to_load_npz.c`. It loads a `.npz` file into memory and returns
all arrays in a linked list.

You can run the example with the `.npz`file generated my `how_to_save_npz`:

    $ ./how_to_load_npz iarray_and_darray.npz

You can also create a `.npz` file from your Python REPL, and check that it can be read as well.

Loading with `npy_array_list_load()` allocate all memory for all holding list structures,
all npy_array structures and all the numeric data itself. Later calling `npy_array_list_free()` with
the head element of the list as argument, will free __everything__.
