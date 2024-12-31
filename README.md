# QCOM-TEE Library
**QCOM-TEE Library** provides an interface for communication to the **Qualcomm® Trusted Execution Environment (QTEE)**[1] via the **QCOM-TEE driver** registered with the Linux TEE subsystem.

QCOM-TEE Library supports Object-based IPC with QTEE for user-space clients via the newly introduced `TEE_IOC_OBJECT_INVOKE` IOCTL for the Linux TEE subsystem in the Linux Kernel patch series
titled [Trusted Execution Environment (TEE) driver for Qualcomm TEE (QTEE)](https://lore.kernel.org/all/20241202-qcom-tee-using-tee-ss-without-mem-obj-v1-0-f502ef01e016@quicinc.com/).

### Introduction

QTEE enables Trusted Applications (TAs) and Services to run securely. It uses an object-based interface, where each service is an object with sets of operations. Clients can invoke these
operations on objects, which can generate results, including other objects. For example, an object can load a TA and return another object that represents the loaded TA, allowing access to its services.

### Features

The QCOM-TEE Library exposes the following APIs to clients to enable Object-based IPC with QTEE:

`GetRootObject()` - Returns a `RootObject` to the client which can be used to initiate Object-based IPC with QTEE.

`InvokeObject()` - Invoke a QTEE Object which represents a service hosted in QTEE.

After obtaining a `RootObject`, the client can invoke an operation on it via the `InvokeObject()` API. The library marshals the object and operation parameters into a format suitable for
`TEE_IOC_OBJECT_INVOKE` IOCTL, which are then transported to QTEE via the QCOM-TEE back-end driver registered with the Linux TEE subsystem. In response, QTEE can return a result, such as
another object (called a `RemoteObject`) or Buffer. The `RemoteObject` can be invoked again to request another service from QTEE.

```
         User space                             Kernel                       Secure world
         ~~~~~~~~~~                             ~~~~~~                       ~~~~~~~~~~~~
   +--------++---------+                                                     +--------------+
   | Client || QCOMTEE |                                                     | Trusted      |
   +--------+| Library |                                                     | Application  |
      /\     +---------+                                                     +--------------+
      ||            /\                                                            /\
      ||            ||                                                            ||
      ||            ||                                                            \/
      ||            ||                                                     +--------------+
      \/            ||                                                     | TEE Internal |
   +-------+        ||                                                     | API          |
   | TEE   |        ||                    +--------+---------+             +--------------+
   | Client|        ||                    | TEE    | QCOMTEE |             | QTEE         |
   | API   |        \/                    | subsys | driver  |             | Trusted OS   |
   +-------+------------------------------+----+--------+----+-------------+--------------+
   |      Generic TEE API                 |    |        |   QTEE MSG                      |
   |      IOCTL (TEE_IOC_*)               |    |        |   SMCCC (QCOM_SCM_SMCINVOKE_*)  |
   +--------------------------------------+----+        +---------------------------------+
```
## Dependencies

### Build-time dependencies

For the building, QCOM-TEE package depends on:
1. The pthread library.
3. QCBOR v1.3 package.

QCBOR v1.3 package is available [here](https://github.com/laurencelundblade/QCBOR/tree/v1.3).

### Run-time dependencies

Currently, QCOM-TEE has the following run-time dependencies:
1. QCOM-TEE driver registered with the Linux TEE subsystem.
2. Qualcomm® Trusted Execution Environment.
3. Trusted Application (TA) 64-bit Test binary.

## Build Instructions

This project builds using Autotools. Assuming you have the cross-build toolchain along with all the build-time dependencies available,
you can build this package on an x86-64 Ubuntu machine by running the following shell commands:

#### 1. Get the toolchain

To generate **ARM binaries** obtain the Linaro tool chain
```
wget -c https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz
tar xf gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz
export PATH="$PATH:gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu/bin/"
```

#### 2. Configure your Build Environment:

```
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
export AS=aarch64-linux-gnu-as
export LD=aarch64-linux-gnu-ld
export RANLIB=aarch64-linux-gnu-ranlib
export STRIP=aarch64-linux-gnu-strip
```

#### 3. Build

```
./gitcompile --host=aarch64-linux-gnu --with-uapi-headers=<Optional path to Kernel UAPI headers> --with-qcbor-package=<Path to installed QCBOR package with QCBOR headers and shared QCBOR library>
```

**Please ensure**
1. The QCBOR package is built with `-DBUILD_SHARED_LIBS=ON` flag
2. Your kernel headers carry the changes from [Trusted Execution Environment (TEE) driver for Qualcomm TEE (QTEE)](https://lore.kernel.org/all/20241202-qcom-tee-using-tee-ss-without-mem-obj-v1-0-f502ef01e016@quicinc.com/) patch series.

## Unit Tests

You can run the `qcomtee_client` provided Unit tests with the following commands:

1. Run the diagnostics Unit Test:
```
qcomtee_client -d 1
```

This Unit test first obtains a `diagnostics_service_object` from QTEE,
and then invokes it to query diagnostics information on QTEE heap usage.
It then prints the received diagnostics on the console.

Passing the test implies that we are able to communicate with embedded
services in QTEE.

```
Run TZ Diagnostics test...
Retrieve TZ heap info Iteration 0
831360 = Total bytes as heap
106083 = Total bytes allocated from heap
705808 = Total bytes free on heap
15184 = Total bytes overhead
4285 = Total bytes wasted
454160 = Largest free block size
```

2. Load the 'Skeleton' Trusted application and send addition command `0`:
```
qcomtee_client -l /data/smcinvoke_skeleton_ta64.mbn <num 1> <num 2> 1
```

This Unit test first loads the 'Skeleton' TA binary in a buffer, and sends it to QTEE
by invoking the `app_loader_object` (which represents a TA loading service within QTEE).
QTEE in turn returns an `app_object` which can now be used to send commands to the TA.
The test sends command `0` to the TA to request addition of `num 1` and `num 2` and checks
for response `num 1 + num 2` from the TA.

Passing the test implies that we are able to load a TA in QTEE and communicate with it.

```
Run Skeleton TA adder test...
Sum 1 + 1 = 2
send_add_cmd succeeded! ret: 0
```

## References
[1]. [Qualcomm Trusted Execution Environment](https://docs.qualcomm.com/bundle/publicresource/topics/80-70015-11/qualcomm-trusted-execution-environment.html)

[2]. [ARM TrustZone](https://developer.arm.com/documentation/102418/0101/What-is-TrustZone-)

## Contributions

Thanks for your interest in contributing to QUIC-TEEC! Please read our [Contributions Page](CONTRIBUTING.md) for more information on contributing features or bug fixes. We look forward to your participation!

## License

**QUIC-TEEC** Project is licensed under the [BSD-3-clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.
