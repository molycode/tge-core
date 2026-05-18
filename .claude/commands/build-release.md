Build tge-core in release configuration (tge-core-linux-gcc_16-release preset).

```bash
!cmake --build --preset tge-core-linux-gcc_16-release $ARGUMENTS 2>&1 | tail -30
```

Report success or the first compile error (not the cascade).
