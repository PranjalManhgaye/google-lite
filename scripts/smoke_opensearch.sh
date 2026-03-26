#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-8080}"

echo "[1] health"
curl -s "http://localhost:${PORT}/health"; echo

echo "[2] url preview ingest"
curl -s -X POST "http://localhost:${PORT}/search" -H 'Content-Type: application/json' \
  -d '{"query":"https://example.com"}'; echo

echo "[3] text query"
curl -s -X POST "http://localhost:${PORT}/search" -H 'Content-Type: application/json' \
  -d '{"query":"example domain"}'; echo

echo "[4] suggest"
curl -s "http://localhost:${PORT}/suggest?q=exa"; echo
