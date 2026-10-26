# syntax=docker/dockerfile:1

# ---- Stage 1: build ----
FROM debian:12-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential cmake git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY engine ./engine
COPY service ./service

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j "$(nproc)"

# ---- Stage 2: minimal runtime ----
FROM debian:12-slim AS runtime

# Run as a non-root user.
RUN groupadd --system app && useradd --system --gid app --no-create-home app

COPY --from=build /src/build/ran_sim_service /usr/local/bin/ran_sim_service

USER app
EXPOSE 8080
ENV PORT=8080

ENTRYPOINT ["/usr/local/bin/ran_sim_service"]
