#include "RequestParams.hpp"

#include <cctype>
#include <cstdlib>

namespace ran::svc {

std::optional<long long> RequestParams::findNumber(std::string_view body,
                                                   std::string_view key) {
    const std::string needle = "\"" + std::string(key) + "\"";
    const auto pos = body.find(needle);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    auto i = body.find(':', pos + needle.size());
    if (i == std::string_view::npos) {
        return std::nullopt;
    }
    ++i;
    while (i < body.size() && (std::isspace(static_cast<unsigned char>(body[i])) != 0)) {
        ++i;
    }
    const auto start = i;
    while (i < body.size() &&
           ((std::isdigit(static_cast<unsigned char>(body[i])) != 0) || body[i] == '-')) {
        ++i;
    }
    if (i == start) {
        return std::nullopt;
    }
    return std::strtoll(std::string(body.substr(start, i - start)).c_str(), nullptr, 10);
}

std::optional<std::string> RequestParams::findString(std::string_view body,
                                                     std::string_view key) {
    const std::string needle = "\"" + std::string(key) + "\"";
    const auto pos = body.find(needle);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    auto i = body.find(':', pos + needle.size());
    if (i == std::string_view::npos) {
        return std::nullopt;
    }
    i = body.find('"', i);
    if (i == std::string_view::npos) {
        return std::nullopt;
    }
    ++i;
    const auto start = i;
    while (i < body.size() && body[i] != '"') {
        ++i;
    }
    if (i >= body.size()) {
        return std::nullopt;
    }
    return std::string(body.substr(start, i - start));
}

SimConfig RequestParams::parse(std::string_view body, SimConfig base) {
    if (const auto v = findNumber(body, "ues")) {
        base.numUes = static_cast<int>(*v);
    }
    if (const auto v = findNumber(body, "rbs")) {
        base.resourceBlocks = static_cast<int>(*v);
    }
    if (const auto v = findNumber(body, "ttis")) {
        base.numTtis = static_cast<Tti>(*v);
    }
    if (const auto v = findNumber(body, "seed")) {
        base.seed = static_cast<unsigned>(*v);
    }
    if (const auto s = findString(body, "scheduler")) {
        base.scheduler = mac::parseSchedulerType(*s);  // may throw std::invalid_argument
    }
    return base;
}

}  // namespace ran::svc
