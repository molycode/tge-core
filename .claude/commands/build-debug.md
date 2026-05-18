Build tge-core in debug configuration (tge-core-linux-gcc_16-debug preset).

```bash
!cmake --build --preset tge-core-linux-gcc_16-debug $ARGUMENTS 2>&1 | tail -30
```

Report success or the first compile error (not the cascade). If `build/gcc_16-Debug` has not been configured yet, say so and show the configure command:

```
cmake --preset tge-core-linux-gcc_16-debug
```
