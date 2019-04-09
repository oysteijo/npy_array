AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libc_npy.a
libc_npy_a_SOURCES = c_npy.c zipcontainer.c dostime.c crc32.c

bin_PROGRAMS = example
example_SOURCES = example.c
example_LDADD = libc_npy.a
