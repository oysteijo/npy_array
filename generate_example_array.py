import numpy as np

a = np.arange(11)
b = 1.0 / (1.0 + np.exp(a))
b = b.astype(np.float32)

c = np.random.rand(10,10)

## Saving
np.save( "singel_array.npy", a)
np.savez( "two_arrays.npz", a=a, b=b)
np.save( "singel_2d_array.npy", c)
