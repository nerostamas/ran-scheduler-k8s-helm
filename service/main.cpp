#include <atomic>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

#include <httplib.h>

#include "Json.hpp"
#include "RequestParams.hpp"
#include "ran/Simulator.hpp"
#include "ran/mac/Scheduler.hpp"

namespace {

/// Read an integer environment variable, falling back to `fallback`.
int envInt(const char* name, int fallback) {
    if (const char* v = std::getenv(name)) {
        try {
            return std::stoi(v);
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

/// Read a string environment variable, falling back to `fallback`.
std::string envStr(const char* name, const std::string& fallback) {
    if (const char* v = std::getenv(name)) {
        return v;
    }
    return fallback;
}

/// Build the default SimConfig from environment variables (populated by the
/// Kubernetes ConfigMap). These are the defaults a /simulate request can override.
ran::SimConfig defaultsFromEnv() {
    ran::SimConfig cfg;
    cfg.numUes = envInt("SIM_UES", cfg.numUes);
    cfg.resourceBlocks = envInt("SIM_RBS", cfg.resourceBlocks);
    cfg.numTtis = static_cast<ran::Tti>(envInt("SIM_TTIS", static_cast<int>(cfg.numTtis)));
    cfg.seed = static_cast<unsigned>(envInt("SIM_SEED", static_cast<int>(cfg.seed)));
    cfg.scheduler = ran::mac::parseSchedulerType(envStr("SIM_SCHEDULER", "pf"));
    return cfg;
}

std::string schedulerName(ran::mac::SchedulerType t) {
    return std::string(ran::mac::makeScheduler(t)->name());
}

/// Serialize a SimResult + its config to the JSON response body.
std::string resultToJson(const ran::SimConfig& cfg, const ran::SimResult& r) {
    ran::svc::JsonWriter w;
    w.beginObject()
        .key("scheduler").value(schedulerName(cfg.scheduler))
        .key("ues").value(static_cast<long long>(cfg.numUes))
        .key("rbs").value(static_cast<long long>(cfg.resourceBlocks))
        .key("ttis").value(static_cast<long long>(cfg.numTtis))
        .key("cell_throughput_mbps").value(r.cellThroughputMbps)
        .key("jains_fairness").value(r.jainsFairness)
        .key("harq_retx_rate").value(r.harqRetxRate)
        .key("per_ue_throughput_mbps").array(r.perUeThroughputMbps)
        .endObject();
    return w.str();
}

/// Prometheus text exposition for the most recent run.
std::string metricsText(const ran::SimResult& r, const std::string& sched,
                        std::uint64_t runs) {
    std::ostringstream m;
    m << "# HELP ran_sim_runs_total Total number of simulations executed.\n"
      << "# TYPE ran_sim_runs_total counter\n"
      << "ran_sim_runs_total " << runs << "\n"
      << "# HELP ran_sim_cell_throughput_mbps Cell throughput of the last run.\n"
      << "# TYPE ran_sim_cell_throughput_mbps gauge\n"
      << "ran_sim_cell_throughput_mbps{scheduler=\"" << sched << "\"} "
      << r.cellThroughputMbps << "\n"
      << "# HELP ran_sim_jains_fairness Jain's fairness index of the last run.\n"
      << "# TYPE ran_sim_jains_fairness gauge\n"
      << "ran_sim_jains_fairness{scheduler=\"" << sched << "\"} " << r.jainsFairness << "\n"
      << "# HELP ran_sim_harq_retx_rate HARQ retransmission rate of the last run.\n"
      << "# TYPE ran_sim_harq_retx_rate gauge\n"
      << "ran_sim_harq_retx_rate{scheduler=\"" << sched << "\"} " << r.harqRetxRate << "\n";
    return m.str();
}

}  // namespace

int main(int argc, char** argv) {
    int port = envInt("PORT", 8080);
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
    }

    ran::SimConfig defaults;
    try {
        defaults = defaultsFromEnv();
    } catch (const std::exception& e) {
        std::cerr << "invalid default config: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    httplib::Server server;

    // Shared state for the /metrics snapshot of the most recent run.
    std::mutex stateMutex;
    ran::SimResult lastResult;
    std::string lastScheduler = schedulerName(defaults.scheduler);
    std::atomic<std::uint64_t> runCount{0};
    std::atomic<bool> ready{false};

    // Warm up with one run so /metrics has data and readiness reflects a working engine.
    {
        ran::Simulator warm(defaults);
        lastResult = warm.run();
        ++runCount;
        ready = true;
    }

    server.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("ok\n", "text/plain");
    });

    server.Get("/readyz", [&](const httplib::Request&, httplib::Response& res) {
        if (ready.load()) {
            res.set_content("ready\n", "text/plain");
        } else {
            res.status = 503;
            res.set_content("not ready\n", "text/plain");
        }
    });

    server.Post("/simulate", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            const ran::SimConfig cfg = ran::svc::RequestParams::parse(req.body, defaults);
            ran::Simulator sim(cfg);
            const ran::SimResult r = sim.run();
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                lastResult = r;
                lastScheduler = schedulerName(cfg.scheduler);
            }
            ++runCount;
            res.set_content(resultToJson(cfg, r), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("{\"error\":\"") + e.what() + "\"}",
                            "application/json");
        }
    });

    server.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        ran::SimResult snapshot;
        std::string sched;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            snapshot = lastResult;
            sched = lastScheduler;
        }
        res.set_content(metricsText(snapshot, sched, runCount.load()), "text/plain");
    });

    std::cout << "ran-sim-service listening on :" << port
              << " (default scheduler=" << lastScheduler << ")\n";
    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "failed to bind port " << port << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
