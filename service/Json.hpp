#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ran::svc {

/// Minimal JSON output helper (no external dependency). Builds compact JSON
/// objects/arrays for the service responses. This is intentionally tiny: the
/// service only ever serializes a fixed, known result shape.
class JsonWriter {
public:
    JsonWriter& beginObject() {
        out_ << '{';
        first_.push_back(true);
        return *this;
    }

    JsonWriter& endObject() {
        out_ << '}';
        first_.pop_back();
        return *this;
    }

    JsonWriter& key(std::string_view k) {
        comma();
        out_ << '"' << k << "\":";
        return *this;
    }

    JsonWriter& value(std::string_view s) {
        out_ << '"' << s << '"';
        return *this;
    }

    JsonWriter& value(double d) {
        out_ << round2(d);
        return *this;
    }

    JsonWriter& value(long long i) {
        out_ << i;
        return *this;
    }

    JsonWriter& array(const std::vector<double>& xs) {
        out_ << '[';
        for (std::size_t i = 0; i < xs.size(); ++i) {
            if (i != 0) {
                out_ << ',';
            }
            out_ << round2(xs[i]);
        }
        out_ << ']';
        return *this;
    }

    [[nodiscard]] std::string str() const { return out_.str(); }

private:
    void comma() {
        if (!first_.empty() && !first_.back()) {
            out_ << ',';
        }
        if (!first_.empty()) {
            first_.back() = false;
        }
    }

    static std::string round2(double d) {
        std::ostringstream s;
        s.precision(4);
        s << d;
        return s.str();
    }

    std::ostringstream out_;
    std::vector<bool> first_;
};

}  // namespace ran::svc
