# Coding Style Guide

This document describes the code formatting standards for the **QUIC-TEEC** project.

This project primarily follows the [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html), achieved by running
the [clang-format](https://clang.llvm.org/docs/ClangFormat.html) tool with the
`.clang-format` file provided at the top of the repository.

## Code Formatting Guidelines


### Headers and Includes

- The body of each header file must be surrounded by an include guard (aka "header guard"). Their names shall be all caps, with words separated by the underscore character "_". For example, *_QCOMTEE_OBJECT_H*
- Header files should only `#include` what is necessary to allow a file that includes it to compile.
- Ordering header files as follows:
    - Standard headers
    - C system headers
    - Other libraries' headers
    - Related and local project's headers.
- Headers in each block must listed in alphabetical order.
- Separate each group with a blank line.

```
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "qcbor.h"

#include "qcom_tee_api.h"
#include "qcom_tee_client.h"
  ...
```

## Commenting Styles

- **General Documentation:**
Use [Doxygen](https://www.doxygen.nl/index.html) format.

- **Inline Comments**: Use `//` for short, explanatory comments at the end of a line.

  ```c
  int y = 4; // Initialize y to 4
  ```

- **Above-line Comments**: Place descriptive comments above the code block they describe.

  ```c
  ...
  
  // Calculate the cube of z
  if (z > 0) {
    return z * z * z;
  }
  ...
  ```

- **Block Comments**: Use `/* */` for detailed explanations of code.
  ```c
  ...
  int arrayRaw[4] = { 1, 2, 3, 5 };

  /* This piece of code is primarily used for marshalling the
   * the contents of the array in a byte blob suitable for
   * transmission across a a transport channel.
   */
  for ( int i = 0; i < 4; i++ ) {
     marshalIntoPacket(arrayRaw[i]);
  }
  ...
  ```
