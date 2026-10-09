#include "sp/runtime/Metrics.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace sp::rt::obs {

namespace {

std::string escapePrometheus(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '\"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::string formatDouble(double d) {
    if (std::isinf(d)) {
        return (d > 0) ? "+Inf" : "-Inf";
    }
    if (std::isnan(d)) {
        return "NaN";
    }
    std::ostringstream ss;
    ss << std::defaultfloat << d;
    return ss.str();
}

} // namespace

std::string formatLabels(const Labels& labels, const std::string& extraKey, const std::string& extraVal) {
    std::map<std::string, std::string> sorted;
    for (const auto& [k, v] : labels) {
        sorted[k] = v;
    }
    if (!extraKey.empty()) {
        sorted[extraKey] = extraVal;
    }

    if (sorted.empty()) {
        return "";
    }

    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& [k, v] : sorted) {
        if (!first) ss << ",";
        first = false;
        ss << k << "=\"" << escapePrometheus(v) << "\"";
    }
    ss << "}";
    return ss.str();
}

Labels parseLabelsJson(const std::string& json) {
    Labels labels;
    if (json.empty() || json[0] != '{') return labels;

    // Lightweight parsing of {"key":"val",...}
    size_t i = 1;
    while (i < json.size()) {
        while (i < json.size() && (json[i] == ' ' || json[i] == ',' || json[i] == '\t' || json[i] == '\n' || json[i] == '\r')) i++;
        if (i >= json.size() || json[i] == '}') break;

        if (json[i] == '"') {
            i++;
            size_t keyStart = i;
            while (i < json.size() && json[i] != '"') i++;
            std::string key = json.substr(keyStart, i - keyStart);
            if (i < json.size() && json[i] == '"') i++;

            while (i < json.size() && (json[i] == ' ' || json[i] == ':')) i++;
            if (i < json.size() && json[i] == '"') {
                i++;
                size_t valStart = i;
                while (i < json.size() && json[i] != '"') i++;
                std::string val = json.substr(valStart, i - valStart);
                if (i < json.size() && json[i] == '"') i++;
                labels.emplace_back(std::move(key), std::move(val));
            } else {
                // Numeric or boolean or bare value
                size_t valStart = i;
                while (i < json.size() && json[i] != ',' && json[i] != '}' && json[i] != ' ') i++;
                std::string val = json.substr(valStart, i - valStart);
                labels.emplace_back(std::move(key), std::move(val));
            }
        } else {
            i++;
        }
    }
    return labels;
}

// ----------------------------------------------------------------------------
// Counter
// ----------------------------------------------------------------------------
Counter::Counter(std::string name, std::string help)
    : Metric(std::move(name), std::move(help)) {}

void Counter::inc(const Labels& labels, double delta) {
    if (delta <= 0.0) return;
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[key] += delta;
}

double Counter::get(const Labels& labels) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(key);
    return (it != m_values.end()) ? it->second : 0.0;
}

void Counter::render(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_help.empty()) {
        out += "# HELP " + m_name + " " + m_help + "\n";
    }
    out += "# TYPE " + m_name + " counter\n";

    if (m_values.empty()) {
        out += m_name + " 0\n";
    } else {
        for (const auto& [lblStr, val] : m_values) {
            out += m_name + lblStr + " " + formatDouble(val) + "\n";
        }
    }
}

// ----------------------------------------------------------------------------
// Gauge
// ----------------------------------------------------------------------------
Gauge::Gauge(std::string name, std::string help)
    : Metric(std::move(name), std::move(help)) {}

void Gauge::set(double value, const Labels& labels) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[key] = value;
}

void Gauge::inc(const Labels& labels, double delta) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[key] += delta;
}

void Gauge::dec(const Labels& labels, double delta) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[key] -= delta;
}

void Gauge::add(double delta, const Labels& labels) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[key] += delta;
}

void Gauge::sub(double delta, const Labels& labels) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[key] -= delta;
}

double Gauge::get(const Labels& labels) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(key);
    return (it != m_values.end()) ? it->second : 0.0;
}

void Gauge::render(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_help.empty()) {
        out += "# HELP " + m_name + " " + m_help + "\n";
    }
    out += "# TYPE " + m_name + " gauge\n";

    if (m_values.empty()) {
        out += m_name + " 0\n";
    } else {
        for (const auto& [lblStr, val] : m_values) {
            out += m_name + lblStr + " " + formatDouble(val) + "\n";
        }
    }
}

// ----------------------------------------------------------------------------
// Histogram
// ----------------------------------------------------------------------------
Histogram::Histogram(std::string name, std::string help, std::vector<double> buckets)
    : Metric(std::move(name), std::move(help)), m_buckets(std::move(buckets)) {
    if (m_buckets.empty()) {
        m_buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
    }
    std::sort(m_buckets.begin(), m_buckets.end());
}

void Histogram::observe(double value, const Labels& labels) {
    std::string key = formatLabels(labels);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& series = m_series[key];
    series.labels = labels;
    if (series.counts.size() != m_buckets.size()) {
        series.counts.resize(m_buckets.size(), 0);
    }

    for (size_t i = 0; i < m_buckets.size(); ++i) {
        if (value <= m_buckets[i]) {
            series.counts[i]++;
        }
    }
    series.countTotal++;
    series.sum += value;
}

void Histogram::render(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_help.empty()) {
        out += "# HELP " + m_name + " " + m_help + "\n";
    }
    out += "# TYPE " + m_name + " histogram\n";

    if (m_series.empty()) {
        // Output default empty histogram
        for (double b : m_buckets) {
            out += m_name + "_bucket{le=\"" + formatDouble(b) + "\"} 0\n";
        }
        out += m_name + "_bucket{le=\"+Inf\"} 0\n";
        out += m_name + "_sum 0\n";
        out += m_name + "_count 0\n";
        return;
    }

    for (const auto& [lblStr, series] : m_series) {
        const Labels& origLabels = series.labels;
        for (size_t i = 0; i < m_buckets.size(); ++i) {
            std::string bLbl = formatLabels(origLabels, "le", formatDouble(m_buckets[i]));
            out += m_name + "_bucket" + bLbl + " " + std::to_string(series.counts[i]) + "\n";
        }
        std::string infLbl = formatLabels(origLabels, "le", "+Inf");
        out += m_name + "_bucket" + infLbl + " " + std::to_string(series.countTotal) + "\n";

        std::string sumLbl = formatLabels(origLabels);
        out += m_name + "_sum" + sumLbl + " " + formatDouble(series.sum) + "\n";
        out += m_name + "_count" + sumLbl + " " + std::to_string(series.countTotal) + "\n";
    }
}

// ----------------------------------------------------------------------------
// MetricsRegistry
// ----------------------------------------------------------------------------
std::shared_ptr<Counter> MetricsRegistry::counter(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_metricsByName.find(name);
    if (it != m_metricsByName.end()) {
        return std::dynamic_pointer_cast<Counter>(it->second);
    }
    auto c = std::make_shared<Counter>(name, help);
    m_metrics.push_back(c);
    m_metricsByName[name] = c;
    return c;
}

std::shared_ptr<Gauge> MetricsRegistry::gauge(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_metricsByName.find(name);
    if (it != m_metricsByName.end()) {
        return std::dynamic_pointer_cast<Gauge>(it->second);
    }
    auto g = std::make_shared<Gauge>(name, help);
    m_metrics.push_back(g);
    m_metricsByName[name] = g;
    return g;
}

std::shared_ptr<Histogram> MetricsRegistry::histogram(const std::string& name, const std::string& help, const std::vector<double>& buckets) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_metricsByName.find(name);
    if (it != m_metricsByName.end()) {
        return std::dynamic_pointer_cast<Histogram>(it->second);
    }
    auto h = std::make_shared<Histogram>(name, help, buckets);
    m_metrics.push_back(h);
    m_metricsByName[name] = h;
    return h;
}

std::string MetricsRegistry::renderPrometheusText() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string out;
    for (const auto& m : m_metrics) {
        m->render(out);
    }
    return out;
}

void MetricsRegistry::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.clear();
    m_metricsByName.clear();
}

static MetricsRegistry s_globalRegistry;

MetricsRegistry& MetricsRegistry::getGlobal() {
    return s_globalRegistry;
}

} // namespace sp::rt::obs
