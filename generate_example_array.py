import numpy as np

a = np.arange(10)
b = 1.0 / (1.0 + np.exp(a))
b = b.astype(np.float32)

## Saving
np.save( "singel_array.npy", a)
np.savez( "two_arrays.npz", a=a, b=b)

