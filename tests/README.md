### QCOM-TEE Library Tests

You can run the `unittest` binary with the following commands:

- _QTEE Diagnostics_ `unittest -d`
- _TA loading and running command_ `unittest -l <path to TA binary> <type> <command>`
  type is 0 to use TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_INPUT or
          1 to use TEE_IOC_SHM_ALLOC for memory sharing.
  command is 0