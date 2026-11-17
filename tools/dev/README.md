# Building and testing in a container

`tools/dev/dev.sh` runs a command in a Debian container with everything SLOOP 3's build and tests need (gcc 14,
Python with NumPy and Pillow, Node.js, the pi32v2 LLVM toolchain `pi32v2-23.1.5-2`). It works on Linux and on macOS
with Docker (Docker Desktop, OrbStack, Colima), x86-64 or Apple Silicon. On macOS the host's own compiler can't run
the tests (they use ELF section names), hence the container.

```sh
tools/dev/dev.sh 'BOARD=smk37 sh fw/build.sh /opt/pi32v2/bin'   # the SMK-37: build/fw-smk37/installer-smk37.html
tools/dev/dev.sh 'sh fw/build.sh /opt/pi32v2/bin && sha256sum -c tests/fm1-reference.sha256'   # the FM-1, unchanged
tools/dev/dev.sh 'make test'                                    # SLOOP 3's tests (and the SMK-37's controls)
tools/dev/dev.sh 'make twin-smk'                                # the SMK-37 on the PC twin (after its build)
```

`tests/fm1-reference.sha256`: the FM-1 build of SLOOP 3.3.7 (915b54f) with that toolchain. Every SMK-37 change sits
behind `BOARD_SMK37`, so the FM-1 build must still match it byte for byte. Build the FM-1 last before `make test` or
`make twin`: both boards write some of the same intermediate files.

`make` across a Docker bind mount can misjudge a file's age and keep a stale binary: delete the binary (`rm -f
build/twin-smk`) or `build/` when a result looks old.
