Build tge-core in release configuration (linux-gcc_16-release preset).

```bash
!${TGE_CMAKE:-cmake} --build --preset linux-gcc_16-release $ARGUMENTS 2>&1 | tail -30
```

Report success or the first compile error (not the cascade).
