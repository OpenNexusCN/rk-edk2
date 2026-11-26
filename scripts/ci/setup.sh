#!/usr/bin/env bash
#
# Fetch and prepare the pinned edk2 tree.
#
# edk2 itself is not a git submodule of this repository: it is fetched at a
# pinned tag so CI and local builds are reproducible without cloning edk2's
# full history. Only the submodules required to build BaseTools and MdePkg's
# FDT library are fetched; DEC include paths for unused packages (OpenSSL,
# SPDM, TPM, unit-test frameworks) are created as empty stubs.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EDK2_TAG="${EDK2_TAG:-edk2-stable202608}"
EDK2_URL="${EDK2_URL:-https://github.com/tianocore/edk2.git}"
DEPS="${ROOT}/.deps"

if [[ ! -d "${ROOT}/edk2/.git" ]]; then
  echo "Cloning edk2 ${EDK2_TAG}"
  rm -rf "${ROOT}/edk2"
  GIT_HTTP_VERSION=HTTP/1.1 git clone --depth 1 --branch "${EDK2_TAG}" \
    --single-branch "${EDK2_URL}" "${ROOT}/edk2"
fi

fetch_module() {
  local path="$1" url="$2" commit="$3"
  local dest="${ROOT}/edk2/${path}"

  if [[ -e "${dest}/.fetched" ]] || [[ -d "${dest}" && -n "$(ls -A "${dest}" 2>/dev/null)" ]]; then
    return 0
  fi

  echo "Fetching ${path} @ ${commit}"
  rm -rf "${dest}"
  mkdir -p "${dest}"

  local tmp="${DEPS}/$(basename "${path}")"
  rm -rf "${tmp}" "${tmp}.tgz"

  # Preferred: codeload tarball (small, no history).
  local slug
  slug="$(sed -E 's#https://github.com/([^/]+)/([^/.]+)(\.git)?#\1/\2#' <<<"${url}")"
  if curl -fsSL --retry 3 --retry-delay 2 \
       "https://codeload.github.com/${slug}/tar.gz/${commit}" -o "${tmp}.tgz"; then
    tar -xzf "${tmp}.tgz" -C "${dest}" --strip-components=1
  else
    # Fallback: plain clone + checkout.
    GIT_HTTP_VERSION=HTTP/1.1 git clone --quiet "${url}" "${tmp}"
    git -C "${tmp}" checkout --quiet "${commit}"
    cp -a "${tmp}/." "${dest}/"
    rm -rf "${dest}/.git"
  fi

  rm -f "${tmp}.tgz"
  rm -rf "${tmp}"
  touch "${dest}/.fetched"
}

# BaseTools needs brotli; MdePkg's DEC requires mipisyst + libfdt paths.
fetch_module "BaseTools/Source/C/BrotliCompress/brotli" \
  "https://github.com/google/brotli.git" \
  "e230f474b87134e8c6c85b630084c612057f253e"
fetch_module "MdeModulePkg/Library/BrotliCustomDecompressLib/brotli" \
  "https://github.com/google/brotli.git" \
  "e230f474b87134e8c6c85b630084c612057f253e"
fetch_module "MdePkg/Library/MipiSysTLib/mipisyst" \
  "https://github.com/MIPI-Alliance/public-mipi-sys-t.git" \
  "370b5944c046bab043dd8b133727b2135af7747a"
fetch_module "MdePkg/Library/BaseFdtLib/libfdt" \
  "https://github.com/devicetree-org/pylibfdt.git" \
  "cfff805481bdea27f900c32698171286542b8d3c"

# Some edk2 DEC files list include paths that live in submodules we do not
# build. The build system only checks that the directory exists.
python3 - <<PY
import glob, os
root = "${ROOT}/edk2"
for dec in glob.glob(os.path.join(root, '**/*.dec'), recursive=True):
    base = os.path.dirname(dec)
    section = None
    for line in open(dec, encoding='utf-8', errors='ignore'):
        s = line.strip()
        if s.startswith('['):
            section = s
        if section and section.lower().startswith('[includes'):
            if not s or s.startswith('[') or s.startswith('#'):
                continue
            path = s.split('#')[0].strip()
            if path:
                os.makedirs(os.path.normpath(os.path.join(base, path)), exist_ok=True)
PY

# BaseTools needs libuuid headers. CI installs uuid-dev; for local builds
# without root, extract the Ubuntu package under .deps.
BASE_TOOLS_EXTRA_OPTFLAGS=""
BASE_TOOLS_EXTRA_LDFLAGS=""
if [[ ! -f /usr/include/uuid/uuid.h ]]; then
  UUID_DIR="${DEPS}/uuid-dev"
  if [[ ! -f "${UUID_DIR}/usr/include/uuid/uuid.h" ]]; then
    echo "uuid-dev not installed; extracting a local copy"
    mkdir -p "${UUID_DIR}"
    curl -fsSL --retry 3 \
      "https://archive.ubuntu.com/ubuntu/pool/main/u/util-linux/uuid-dev_2.37.2-4ubuntu3_amd64.deb" \
      -o "${DEPS}/uuid-dev.deb"
    dpkg-deb -x "${DEPS}/uuid-dev.deb" "${UUID_DIR}"
  fi
  BASE_TOOLS_EXTRA_OPTFLAGS="-I${UUID_DIR}/usr/include"
  BASE_TOOLS_EXTRA_LDFLAGS="-L${UUID_DIR}/usr/lib/x86_64-linux-gnu"
fi

if [[ ! -x "${ROOT}/edk2/BaseTools/Source/C/bin/GenFv" ]]; then
  echo "Building BaseTools"
  make -C "${ROOT}/edk2/BaseTools" -j"$(nproc)" \
    EXTRA_OPTFLAGS="${BASE_TOOLS_EXTRA_OPTFLAGS}" \
    EXTRA_LDFLAGS="${BASE_TOOLS_EXTRA_LDFLAGS}"
fi
