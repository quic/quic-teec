# Coding Style Guide

This document describes the code formatting standards for the **QUIC-TEEC** project.

This project primarily follows the [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html), achieved by running
the [clang-format](https://clang.llvm.org/docs/ClangFormat.html) tool with the
`.clang-format` file provided at the top of the repository.

However, we deviate slightly with the following exceptions:
* `CamelCase` is allowed for `types` and `public API` names.
* All variables must be initialized to a valid default value for it's `type`.
In particular,
  * Standard integer types must be initialized to a sensible value such as `0`
or `-1`.
  * Pointers must be initialized to `NULL`.
  * `structs` must be initialized via `{ 0 }` if composed to all scalars,
or via `memset()`.
* Header files are **not formatted** via the `.clang-format` file to preserve
white-spaces in macros definitions.

## Code Formatting Guidelines


### Headers and Includes

- Typically every header file has the same base filename as an associated source file.
- The body of each header file must be surrounded by an include guard (aka "header guard"). These guards shall be given the same name as the path in which they reside. Their names shall be all caps, with words separated by the underscore character "_". For example, *_QCOM_TEE_SUPP_H*
- Header files should never define functions or variables.
- Header files should only `#include` what is necessary to allow a file that includes it to compile. Associated source files will always `#include` the header of the same name.
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

- **API documentation:**
  ```c
  /**
   * This function calculates the square of a number
   * used in the context of distance calculations.
   *
   * @param    num  Number to be squared.
   *
   * @return   int  The square of the number.
   */
  int square(int num) {
    return num * num;
  }
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
