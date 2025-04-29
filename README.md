# QCOM-TEE Library
**QCOM-TEE Library** provides an interface for communication to the [Qualcomm® Trusted Execution Environment (QTEE)](https://docs.qualcomm.com/bundle/publicresource/topics/80-70015-11/qualcomm-trusted-execution-environment.html) via the QCOM-TEE driver registered with the [Linux TEE subsystem](https://www.kernel.org/doc/Documentation/tee.txt).

QCOM-TEE Library supports Object-based IPC with QTEE for user-space clients via the `TEE_IOC_OBJECT_INVOKE` IOCTL from the Linux TEE subsystem .

### Introduction

QTEE enables Trusted Applications (TAs) and Services to run securely. It uses an object-based interface, where each service is an object with sets of operations. Clients can invoke these
operations on objects, which can generate results, including other objects. For example, an object can load a TA and return another object that represents the loaded TA, allowing access to its services.

Kernel and userspace services are also available to QTEE through a similar approach. QTEE makes callback requests that are converted into object invocations. These objects can represent services hosted within the kernel or userspace process.

## Build Instructions

On an x86-64 Ubuntu machine, assuming you have a cross-build toolchain along with all the build-time dependencies available, such as CMake, you can build the project and run the unittest with

```
git clone https://github.com/quic/quic-teec.git
cd quic-teec
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=CMakeToolchain.txt -DBUILD_UNITTEST=ON
```
Edit `CMakeToolchain.txt` for the toolchain of your choice.

In order to build the _qcomtee_ library, you require QCBOR. It is not available using the standard Ubuntu repository. If you do not have it installed on your machine, you can obtain it from [here](https://github.com/laurencelundblade/QCBOR).

Use `-DQCBOR_DIR_HINT=/path/to/installed/dir` to specify the QCBOR dependency.


## Unittest
List of available tests are [here](tests/README.md)

## Contributions

Thanks for your interest in contributing to QUIC-TEEC! Please read our [Contributions Page](CONTRIBUTING.md) for more information on contributing features or bug fixes. We look forward to your participation!

## License

**QUIC-TEEC** Project is licensed under the [BSD-3-clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.
