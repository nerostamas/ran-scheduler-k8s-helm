#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "ran/Simulator.hpp"

namespace ran::svc {

/// Extremely small, forgiving extractor for the handful of fields the
/// /simulate endpoint accepts. It is NOT a general JSON parser: it scans for
/// `"key"` and reads the following number or quoted string. Unknown or missing
/// fields fall back to the provided defaults. This keeps the service free of a
/// heavy JSON dependency while handling the fixed request shape.
class RequestParams {
public:
    /// Parse a /simulate request body onto a base config (the ConfigMap
    /// defaults). Throws std::invalid_argument on a malformed scheduler value.
    static SimConfig parse(std::string_view body, SimConfig base);

private:
    static std::optional<long long> findNumber(std::string_view body,
                                               std::string_view key);
    static std::optional<std::string> findString(std::string_view body,
                                                  std::string_view key);
};

}  // namespace ran::svc
