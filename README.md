# ran-scheduler-k8s-helm

[![CI](https://github.com/nerostamas/ran-scheduler-k8s-helm/actions/workflows/ci.yml/badge.svg)](https://github.com/nerostamas/ran-scheduler-k8s-helm/actions/workflows/ci.yml)
[![Helm](https://img.shields.io/badge/Helm-3-0f1689.svg)](https://helm.sh/)
[![Kubernetes](https://img.shields.io/badge/Kubernetes-minikube-326ce5.svg)](https://minikube.sigs.k8s.io/)

Cloud-native deployment of the
[RAN MAC Scheduler](https://github.com/nerostamas/Radio-Access-Network-RAN-MAC-Scheduler):
the C++ simulator is wrapped in a small **HTTP microservice**, containerized with a multi-stage
**Docker** build, and deployed to **Kubernetes (minikube)** with a production-shaped **Helm**
chart — liveness/readiness probes, a ConfigMap, resource limits, a HorizontalPodAutoscaler, and
Prometheus metrics.

> Docker (multi-stage) · Kubernetes (Deployment, Service, ConfigMap, HPA,
> probes) · Helm (chart, values, templating, `NOTES.txt`) · minikube workflow · Prometheus
> metrics · CI (`helm lint`, `kubeconform`, image build).

---

## Architecture

```
                 ┌───────────────────────── Kubernetes (minikube) ─────────────────────────┐
                 │                                                                          │
  curl / UI ───► │  Service (ClusterIP)  ──►  Deployment: ran-sim-service (N replicas)      │
                 │        :8080                   │  C++ HTTP server (cpp-httplib)           │
                 │                                │  reuses ran_core sim library             │
                 │                                │  /healthz /readyz /simulate /metrics     │
                 │                                ▼                                          │
                 │                           ConfigMap (default sim params)                 │
                 │                           HPA (CPU-based autoscaling)                     │
                 └──────────────────────────────────────────────────────────────────────────┘
```

The service keeps the simulation engine (`ran_core`) untouched and adds a thin HTTP layer, so
the batch CLI becomes a long-running, probeable, autoscalable microservice.

## HTTP API

| Method | Path | Purpose |
| --- | --- | --- |
| `GET` | `/healthz` | Liveness probe — process is up |
| `GET` | `/readyz` | Readiness probe — ready to serve |
| `POST` | `/simulate` | Run a simulation, return KPIs as JSON |
| `GET` | `/metrics` | Prometheus exposition of the last run's KPIs |

`POST /simulate` body (all fields optional; defaults come from the ConfigMap):

```json
{ "ues": 10, "rbs": 50, "ttis": 1000, "seed": 42, "scheduler": "pf" }
```

Response:

```json
{
  "scheduler": "ProportionalFair",
  "ues": 10, "rbs": 50, "ttis": 1000,
  "cell_throughput_mbps": 22.99,
  "jains_fairness": 0.85,
  "harq_retx_rate": 0.293,
  "per_ue_throughput_mbps": [2.06, 1.15, 3.62, ...]
}
```

---

## Quick start (minikube)

```bash
# 0. Prereqs: docker, minikube, kubectl, helm
minikube start

# 1. Build the image straight into minikube's Docker daemon
eval $(minikube docker-env)
docker build -t ran-sim-service:0.1.0 .

# 2. Install the Helm chart
helm install ran-sim ./helm/ran-sim --set image.pullPolicy=Never

# 3. Reach the service
kubectl port-forward svc/ran-sim 8080:8080
curl localhost:8080/healthz
curl -X POST localhost:8080/simulate \
  -H 'content-type: application/json' \
  -d '{"ues":10,"rbs":50,"ttis":1000,"scheduler":"pf"}'
curl localhost:8080/metrics
```

A one-shot helper does all of the above:

```bash
./scripts/deploy-minikube.sh
```

Tear down:

```bash
helm uninstall ran-sim
minikube stop
```

## Helm chart

`helm/ran-sim/` is a standard chart:

```
helm/ran-sim/
├── Chart.yaml
├── values.yaml            # image, replicas, resources, autoscaling, sim defaults
└── templates/
    ├── _helpers.tpl
    ├── configmap.yaml     # default simulation parameters
    ├── deployment.yaml    # probes, resources, env from ConfigMap
    ├── service.yaml
    ├── hpa.yaml           # optional CPU autoscaler
    ├── servicemonitor.yaml# optional Prometheus Operator scrape target
    └── NOTES.txt
```

Common overrides:

```bash
helm install ran-sim ./helm/ran-sim \
  --set replicaCount=3 \
  --set autoscaling.enabled=true \
  --set simDefaults.scheduler=maxci \
  --set resources.limits.cpu=500m
```

Validate without a cluster:

```bash
helm lint ./helm/ran-sim
helm template ran-sim ./helm/ran-sim | kubeconform -strict -summary
```

## Local build (no cluster)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/ran_sim_service --port 8080 &
curl localhost:8080/healthz
```

## CI/CD

`.github/workflows/ci.yml` runs on every push/PR:

1. **Build** the C++ service (CMake, GCC).
2. **Smoke test** — start the server, hit `/healthz`, `/simulate`, `/metrics`.
3. **Docker** — build the multi-stage image.
4. **Helm** — `helm lint` and render + validate manifests with `kubeconform`.

## License

MIT © Nguyen Phuc Tran
