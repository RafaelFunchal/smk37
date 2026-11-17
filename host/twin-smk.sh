#!/bin/sh
# The SMK-37's twin (make twin-smk, after BOARD=smk37 fw/build.sh): the SMK-37's own scenarios, then the FM-1's that
# do not depend on the FM-1's panel or its SLOOP 2.8 data, on the SMK-37's build.
set -u
T=./build/twin-smk; O=build/tw-smk
export ASAN_OPTIONS=detect_leaks=0:handle_segv=0:allow_user_segv_handler=1:handle_sigfpe=0
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
fail=0
run() {   # name scenario flash_in
  mkdir -p $O/$1
  if timeout 900 $T $2 $O/$1 $3 1 > $O/$1/log.txt 2>&1; then r=0; else r=$?; fi
  printf '%-22s %s  %s\n' "$1" "$([ $r = 0 ] && echo ok || echo "FAIL ($r)")" "$(grep '^TWIN:' $O/$1/log.txt | tail -1 | cut -c7-)"
  [ $r = 0 ] || { fail=1; grep -E "FAIL|ERROR|runtime error|TWIN: the|!! TWIN|TWIN: SMK" $O/$1/log.txt | head -12; }
}
same() {
  if diff -q $O/$1/end-state.txt $O/$2/boot-state.txt > /dev/null 2>&1; then printf '%-22s ok  the state saved came back\n' "$1 = $2"
  else printf '%-22s FAIL\n' "$1 = $2"; diff $O/$1/end-state.txt $O/$2/boot-state.txt | head -6; fail=1; fi
}
run smk smk -
run smk-reboot smk-reboot $O/smk/flash.bin
run smk-leftover smk-leftover -
run smk-autosave smk-autosave -
same smk smk-reboot
run boot boot -
run reboot reboot $O/boot/flash.bin
same boot reboot
run rescue rescue $O/boot/flash.bin                # (OCTAVE- held at power-on: RESCUE, the saved project kept)
run pages pages -
run record record -
run sounds sounds -
for s in 1 2 3 11 12 13 14 15 16 17 18; do         # (a power cut at a random point of a save, over a saved project)
  mkdir -p $O/cut-$s $O/aftercut-$s
  if timeout 900 $T powercut $O/cut-$s $O/record/flash.bin $s > $O/cut-$s/log.txt 2>&1; then :; else echo "cut-$s FAIL"; fail=1; fi
  run aftercut-$s aftercut $O/cut-$s/flash.bin
done
run editor editor -
run midi midi -
run fx fx -
run monkey "monkey" -
exit $fail
