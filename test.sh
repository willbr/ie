#!/usr/bin/env bash
# Golden-file test runner for the ie parser (src/c/parse.c).
#
#   ./test.sh          parse each fixture's in.txt and diff it against out.txt
#   ./test.sh bless    regenerate out.txt from the current parser output
#
# Fixtures: src/tests/tokeniser/<group>/<name>/{in,out}.txt
# A <name> beginning with "err-" is expected to be rejected (non-zero exit).

cd "$(dirname "$0")" || exit 1

mode=${1:-check}
make all >/dev/null || exit 1

parse=build/parse
tests=src/tests/tokeniser

pass=0
fail=0
blessed=0
fails=()

for in in "$tests"/*/*/in.txt; do
    dir=${in%/in.txt}
    name=${dir#"$tests"/}
    out=$dir/out.txt
    base=${dir##*/}

    # err-* fixtures should be rejected; only meaningful in check mode
    if [[ $base == err-* ]]; then
        [[ $mode == bless ]] && continue
        if "$parse" "$in" >/dev/null 2>&1; then
            fail=$((fail + 1))
            fails+=("$name — expected rejection, parser accepted it")
        else
            pass=$((pass + 1))
        fi
        continue
    fi

    if ! got=$("$parse" "$in" 2>/dev/null); then
        [[ $mode == bless ]] && { echo "skip (parser failed): $name"; continue; }
        fail=$((fail + 1))
        fails+=("$name — parser exited non-zero")
        continue
    fi

    if [[ $mode == bless ]]; then
        printf '%s\n' "$got" > "$out"
        blessed=$((blessed + 1))
        continue
    fi

    want=$(cat "$out" 2>/dev/null)
    if [[ $got == "$want" ]]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        fails+=("$name")
        printf 'FAIL %s\n' "$name"
        diff <(printf '%s\n' "$want") <(printf '%s\n' "$got") | sed 's/^/    /'
    fi
done

if [[ $mode == bless ]]; then
    printf 'blessed %d fixtures\n' "$blessed"
    exit 0
fi

echo
if (( fail > 0 )); then
    printf '%d passed, %d FAILED:\n' "$pass" "$fail"
    printf '  %s\n' "${fails[@]}"
    exit 1
fi
printf 'all %d tests passed\n' "$pass"
