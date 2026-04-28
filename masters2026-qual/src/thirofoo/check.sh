#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"
pahcer run --freeze-best-scores
