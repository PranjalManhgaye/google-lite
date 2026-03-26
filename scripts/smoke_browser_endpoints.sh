#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-8080}"
BASE="http://localhost:${PORT}"

echo "[1] OpenSearch descriptor"
curl -s "${BASE}/opensearch.xml" | sed -n '1,4p'

echo "[2] browser suggest format"
curl -s "${BASE}/suggest?q=exa"; echo

echo "[3] URL preview via GET /search"
curl -s "${BASE}/search?q=https%3A%2F%2Fexample.com&page=1&size=10" | sed -n '1,8p'

echo "[4] text query via GET /search (timed)"
curl -s -o /tmp/googlelite_search.html -w "time_total=%{time_total}\n" "${BASE}/search?q=example+domain&page=1&size=10"

echo "[5] POST /search latency sample"
tmp="$(mktemp)"
for i in 1 2 3 4 5; do
  curl -s -o /dev/null -w "%{time_total}\n" -X POST "${BASE}/search" -H 'Content-Type: application/json' -d '{"query":"example domain"}'
done > "$tmp"
sorted="$(sort -n "$tmp")"
count="$(echo "$sorted" | wc -l | tr -d ' ')"
p50_idx=$(( (count + 1) / 2 ))
p95_idx=$(( (count * 95 + 99) / 100 ))
p50="$(echo "$sorted" | sed -n "${p50_idx}p")"
p95="$(echo "$sorted" | sed -n "${p95_idx}p")"
echo "p50_s=${p50}"
echo "p95_s=${p95}"
rm -f "$tmp"
