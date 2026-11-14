#!/bin/sh
# Drives the tig executable against a repository in a temporary directory.
set -eu

TIG=$1
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

fail() { echo "FAIL: $*" >&2; exit 1; }
expect_exit() { # expected command...
  expected=$1; shift
  set +e
  "$@" >"$WORK/out" 2>"$WORK/err"
  actual=$?
  set -e
  [ "$actual" = "$expected" ] || { cat "$WORK/err" >&2; fail "expected exit $expected, got $actual: $*"; }
}

"$TIG" --version | grep -q '^tig ' || fail "version"

# usage errors
expect_exit 2 "$TIG"
expect_exit 2 "$TIG" frobnicate
expect_exit 2 "$TIG" init "$WORK/x"
expect_exit 2 "$TIG" -C "$WORK/none" state --at yesterday

# a repository that does not exist
expect_exit 3 "$TIG" -C "$WORK/none" state

# unsigned repository
REPO="$WORK/unsigned"
expect_exit 0 "$TIG" init "$REPO" --kind vehicle --vocabulary gtdb/1
grep -q "unsigned" "$WORK/out" || fail "init output"
expect_exit 0 "$TIG" -C "$REPO" state
grep -q "kind: vehicle" "$WORK/out" || fail "state shows kind"
expect_exit 0 "$TIG" -C "$REPO" state --json
grep -q '"kind":"vehicle"' "$WORK/out" || fail "state --json shows kind"
expect_exit 0 "$TIG" -C "$REPO" log --json
[ "$(cat "$WORK/out")" = "[]" ] || fail "empty log --json is []"
expect_exit 0 "$TIG" --json -C "$REPO" log
expect_exit 0 "$TIG" log -C "$REPO"
[ ! -s "$WORK/out" ] || fail "empty log prints nothing"

# a JSON error is JSON
expect_exit 3 "$TIG" --json -C "$WORK/none" state
grep -q '"error"' "$WORK/err" || fail "error is JSON"

# events written by hand are replayed
mkdir -p "$REPO/events/2024"
cat > "$REPO/events/2024/2024-03-11.7fd2ma.yaml" <<'YAML'
time: "2024-03-11"
type: service
actions:
  - type: replaced
    changes:
      - add: fluid/engine-oil
        set: { grade: 5W-30 }
YAML
cat > "$REPO/events/2024/2024-06-01.k3m9xq.yaml" <<'YAML'
time: "2024-06-01"
actions:
  - type: replaced
    changes:
      - update: fluid/engine-oil
        set: { grade: 0W-20 }
YAML
expect_exit 0 "$TIG" -C "$REPO" log
grep -q "7fd2ma" "$WORK/out" || fail "log lists the first event"
grep -q "inspected\|replaced" "$WORK/out" || fail "log summarises actions"
expect_exit 0 "$TIG" -C "$REPO" log --json
grep -q '"id":"7fd2ma"' "$WORK/out" || fail "log --json has ids"
expect_exit 0 "$TIG" -C "$REPO" state --at 2024-03-11
grep -q "5W-30" "$WORK/out" || fail "state at the first event"
expect_exit 0 "$TIG" -C "$REPO" state --at 2024-05
grep -q "5W-30" "$WORK/out" || fail "state in May"
expect_exit 0 "$TIG" -C "$REPO" state --json
grep -q '"grade":"0W-20"' "$WORK/out" || fail "latest state"
expect_exit 0 "$TIG" -C "$REPO" state --at 2024-01-01
grep -q "fluid" "$WORK/out" && fail "no oil before it was added"

# a replay that fails is rejected, not a crash
cat > "$REPO/events/2024/2024-07-01.zzzzzz.yaml" <<'YAML'
time: "2024-07-01"
actions:
  - type: removed
    changes:
      - remove: tyre/nothere
YAML
expect_exit 1 "$TIG" -C "$REPO" state
rm "$REPO/events/2024/2024-07-01.zzzzzz.yaml"

# signed repository
KEY="$WORK/key"
ssh-keygen -q -t ed25519 -N "" -C test -f "$KEY"
SIGNED="$WORK/signed"
expect_exit 0 "$TIG" init "$SIGNED" --kind device --key "$KEY"
grep -q "(signed)" "$WORK/out" || fail "signed init"
[ -f "$SIGNED/.tig/allowed_signers" ] || fail "allowed_signers written"
expect_exit 0 env TIG_SIGNING_KEY="$KEY" "$TIG" init "$WORK/signed2" --kind device --json
grep -q '"signed":true' "$WORK/out" || fail "TIG_SIGNING_KEY is used"
expect_exit 3 "$TIG" init "$WORK/signed3" --kind device --key "$WORK/missing"

echo "ok"
