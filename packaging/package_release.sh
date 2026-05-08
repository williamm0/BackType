#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="0.1.2-pre.1"
DIST_DIR="$ROOT_DIR/dist"
STAGING_DIR="$ROOT_DIR/build/release-staging"

WINDOWS_ARTIFACT="${WINDOWS_ARTIFACT:-}"
MACOS_ARTIFACT="${MACOS_ARTIFACT:-}"

find_artifact() {
  local pattern="$1"
  while IFS= read -r path; do
    printf '%s\n' "$path"
    return 0
  done < <(find "$ROOT_DIR/build" \
    \( -path "$STAGING_DIR" -o -path "$STAGING_DIR/*" \) -prune -o \
    -name "$pattern" -print 2>/dev/null)
  return 0
}

if [[ -z "$WINDOWS_ARTIFACT" ]]; then
  WINDOWS_ARTIFACT="$(find_artifact 'BackType.aex')"
fi

if [[ -z "$MACOS_ARTIFACT" ]]; then
  MACOS_ARTIFACT="$(find_artifact 'BackType.plugin')"
fi

rm -rf "$STAGING_DIR"
mkdir -p "$DIST_DIR" "$STAGING_DIR"

created=0

if [[ -n "$WINDOWS_ARTIFACT" && -f "$WINDOWS_ARTIFACT" ]]; then
  win_stage="$STAGING_DIR/windows"
  mkdir -p "$win_stage"
  cp "$WINDOWS_ARTIFACT" "$win_stage/BackType.aex"
  cp "$ROOT_DIR/README.md" "$win_stage/README.md"
  [[ -f "$ROOT_DIR/LICENSE" ]] && cp "$ROOT_DIR/LICENSE" "$win_stage/LICENSE"
  (cd "$win_stage" && zip -qr "$DIST_DIR/BackType-v${VERSION}-Windows.zip" .)
  created=1
else
  echo "Missing Windows artifact: BackType.aex"
fi

if [[ -n "$MACOS_ARTIFACT" && -d "$MACOS_ARTIFACT" ]]; then
  mac_stage="$STAGING_DIR/macos"
  mkdir -p "$mac_stage"
  ditto "$MACOS_ARTIFACT" "$mac_stage/BackType.plugin"
  cp "$ROOT_DIR/README.md" "$mac_stage/README.md"
  [[ -f "$ROOT_DIR/LICENSE" ]] && cp "$ROOT_DIR/LICENSE" "$mac_stage/LICENSE"
  rm -f "$DIST_DIR/BackType-v${VERSION}-macOS.zip"
  (cd "$mac_stage" && zip -qry "$DIST_DIR/BackType-v${VERSION}-macOS.zip" BackType.plugin README.md LICENSE)
  created=1
else
  echo "Missing macOS artifact: BackType.plugin"
fi

if [[ "$created" -eq 0 ]]; then
  echo "No release zips were created because no compiled plugin artifacts were found."
  exit 1
fi

echo "Created:"
[[ -f "$DIST_DIR/BackType-v${VERSION}-Windows.zip" ]] && echo "  $DIST_DIR/BackType-v${VERSION}-Windows.zip"
[[ -f "$DIST_DIR/BackType-v${VERSION}-macOS.zip" ]] && echo "  $DIST_DIR/BackType-v${VERSION}-macOS.zip"
