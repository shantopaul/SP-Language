#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace sp::rt::obs {

using Labels = std::vector<std::pair<std::string, std::string>>;

std::string formatLabels(const Labels& labels, const std::string& extraKey = "", const std::string& extraVal = "");
Labels parseLabelsJson(const std::string& json);

class Metric {
public:
    virtual ~Metric() = default;
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const std::string& getHelp() const noexcept { return m_help; }
    virtual void render(std::string& out) = 0;

protected:
    Metric(std::string name, std::string help) : m_name(std::move(name)), m_help(std::move(help)) {}
    std::string m_name;
    std::string m_help;
};

class Counter : public Metric {
public:
    Counter(std::string name, std::string help);

    void inc(const Labels& labels = {}, double delta = 1.0);
    [[nodiscard]] double get(const Labels& labels = {});
    void render(std::string& out) override;

private:
    std::mutex m_mutex;
    std::map<std::string, double> m_values;
};

class Gauge : public Metric {
public:
    Gauge(std::string name, std::string help);

    void set(double value, const Labels& labels = {});
    void inc(const Labels& labels = {}, double delta = 1.0);
    void dec(const Labels& labels = {}, double delta = 1.0);
    void add(double delta, const Labels& labels = {});
    void sub(double delta, const Labels& labels = {});
    [[nodiscard]] double get(const Labels& labels = {});
    void render(std::string& out) override;

private:
    std::mutex m_mutex;
    std::map<std::string, double> m_values;
};

class Histogram : public Metric {
public:
    Histogram(std::string name, std::string help, std::vector<double> buckets = {});

    void observe(double value, const Labels& labels = {});
    void render(std::string& out) override;

    [[nodiscard]] const std::vector<double>& getBuckets() const noexcept { return m_buckets; }

private:
    struct BucketSeries {
        Labels labels;
        std::vector<uint64_t> counts; // counts for each bucket limit
        uint64_t countTotal{0};
        double sum{0.0};
    };

    std::vector<double> m_buckets;
    std::mutex m_mutex;
    std::map<std::string, BucketSeries> m_series;
};

class MetricsRegistry {
public:
    MetricsRegistry() = default;

    std::shared_ptr<Counter> counter(const std::string& name, const std::string& help = "");
    std::shared_ptr<Gauge> gauge(const std::string& name, const std::string& help = "");
    std::shared_ptr<Histogram> histogram(const std::string& name, const std::string& help = "", const std::vector<double>& buckets = {});

    [[nodiscard]] std::string renderPrometheusText();
    void clear();

    static MetricsRegistry& getGlobal();

private:
    std::mutex m_mutex;
    std::vector<std::shared_ptr<Metric>> m_metrics;
    std::map<std::string, std::shared_ptr<Metric>> m_metricsByName;
};

} // namespace sp::rt::obs
