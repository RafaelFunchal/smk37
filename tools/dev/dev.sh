#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run a command in the Linux build container (tools/dev/Dockerfile), at the repository's top, e.g.
#   tools/dev/dev.sh 'BOARD=smk37 sh fw/build.sh /opt/pi32v2/bin'
#   tools/dev/dev.sh 'make test'
# The image (sloop3-dev) is built on first use. The browser end-to-end tests (Playwright) are skipped here.
set -eu
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd)
docker image inspect sloop3-dev > /dev/null 2>&1 || docker build -t sloop3-dev "$here"
exec docker run --rm -v "$repo":/w -w /w -e INSTALLER_E2E=0 -e EDITOR_E2E=0 sloop3-dev sh -c "$*"
