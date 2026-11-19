#!/usr/bin/env bash
# Sets or checks the release version in every file that pins it.
#   release-version.sh check <version>   fail if a file holds another value
#   release-version.sh bump  <version>   write the version, then check
# Run it from the repository root. release.yaml and prepare-release.yaml use it.
set -euo pipefail

# "<file> <key>": the first "<key>:" line of <file> holds the version.
PLACES=(
  "packages/core/base.yaml respeaker_lite_ref"
  "packages/lite/device.yaml version"
  "packages/xvf3800/base.yaml version"
  "packages/koala/device.yaml version"
  "config/respeaker-satellite-dashboard-example.yaml respeaker_lite_ref"
  "config/respeaker-xvf-satellite-example.yaml respeaker_lite_ref"
  "config/koala-dashboard.yaml respeaker_lite_ref"
  "config/respeaker-satellite-factory.yaml respeaker_lite_ref"
  "config/respeaker-xvf-satellite-factory.yaml respeaker_lite_ref"
  "config/koala-satellite.yaml respeaker_lite_ref"
  "readme/xvf3800/README.md respeaker_lite_ref"
)

[ $# -eq 2 ] && { [ "$1" = check ] || [ "$1" = bump ]; } || { echo "usage: $0 check|bump <version>" >&2; exit 2; }
cmd=$1 V=$2

if [ "$cmd" = bump ]; then
  for place in "${PLACES[@]}"; do
    read -r file key <<< "$place"
    awk -v key="$key" -v v="$V" '
      !done && $0 ~ "^[ \t]*" key ":" { sub(/:.*/, ": " v); done = 1 }
      { print }
    ' "$file" > "$file.tmp"
    mv "$file.tmp" "$file"
  done
fi

fail=0
for place in "${PLACES[@]}"; do
  read -r file key <<< "$place"
  value=$(grep -E "^[[:space:]]*$key:" "$file" | head -n1 | sed -E "s/.*$key:[[:space:]]*\"?([^\"]+)\"?[[:space:]]*\$/\\1/" || true)
  if [ "$value" != "$V" ]; then
    echo "::error file=$file::$key is '$value'. Set it to $V."
    fail=1
  fi
done
exit $fail
