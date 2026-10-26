#!/usr/bin/env bash
# Build the service locally, start it, and exercise every endpoint.
# Used by CI and for local verification without a cluster.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${PORT:-18080}"

echo ">> Building"
cmake -S "${ROOT}" -B "${ROOT}/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "${ROOT}/build" -j >/dev/null

echo ">> Starting service on :${PORT}"
PORT="${PORT}" "${ROOT}/build/ran_sim_service" &
SVC_PID=$!
trap 'kill ${SVC_PID} 2>/dev/null || true' EXIT

# Wait for readiness.
for _ in $(seq 1 30); do
  if curl -fsS "localhost:${PORT}/healthz" >/dev/null 2>&1; then break; fi
  sleep 0.3
done

echo ">> GET /healthz";  curl -fsS "localhost:${PORT}/healthz"
echo ">> GET /readyz";   curl -fsS "localhost:${PORT}/readyz"
echo ">> POST /simulate (pf)"
curl -fsS -X POST "localhost:${PORT}/simulate" \
  -H 'content-type: application/json' \
  -d '{"ues":10,"rbs":50,"ttis":500,"scheduler":"pf"}'
echo
echo ">> POST /simulate (maxci)"
curl -fsS -X POST "localhost:${PORT}/simulate" \
  -H 'content-type: application/json' \
  -d '{"scheduler":"maxci"}'
echo
echo ">> GET /metrics"; curl -fsS "localhost:${PORT}/metrics" | head -n 20
echo ">> smoke test passed"
