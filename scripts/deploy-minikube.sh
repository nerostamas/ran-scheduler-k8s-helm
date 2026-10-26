#!/usr/bin/env bash
# One-shot: build the image into minikube and deploy the Helm chart.
set -euo pipefail

IMAGE_REPO="${IMAGE_REPO:-ran-sim-service}"
IMAGE_TAG="${IMAGE_TAG:-0.1.0}"
RELEASE="${RELEASE:-ran-sim}"
CHART_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/helm/ran-sim"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo ">> Ensuring minikube is running"
minikube status >/dev/null 2>&1 || minikube start

echo ">> Building image ${IMAGE_REPO}:${IMAGE_TAG} into minikube's Docker daemon"
eval "$(minikube docker-env)"
docker build -t "${IMAGE_REPO}:${IMAGE_TAG}" "${PROJECT_DIR}"

echo ">> Installing/upgrading Helm release '${RELEASE}'"
helm upgrade --install "${RELEASE}" "${CHART_DIR}" \
  --set image.repository="${IMAGE_REPO}" \
  --set image.tag="${IMAGE_TAG}" \
  --set image.pullPolicy=Never \
  --wait --timeout 120s

echo ">> Waiting for rollout"
kubectl rollout status "deployment/${RELEASE}" --timeout=120s

cat <<EOF

Deployed. In another terminal run:

  kubectl port-forward svc/${RELEASE} 8080:8080

Then:

  curl localhost:8080/healthz
  curl -X POST localhost:8080/simulate -H 'content-type: application/json' \\
       -d '{"ues":10,"rbs":50,"ttis":1000,"scheduler":"pf"}'
  curl localhost:8080/metrics

Tear down with: helm uninstall ${RELEASE}
EOF
