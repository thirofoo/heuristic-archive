#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

for cfg in pahcer_config_a.toml pahcer_config_b.toml pahcer_config_c.toml; do
  echo "[RUN] $cfg (update best scores)"
  pahcer run --setting-file "$cfg"
 done
