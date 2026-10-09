module std.metrics;

import std.io;

// ============================================================================
// Standard Library Prometheus Exposition Metrics (std.metrics) (SP 2.9)
// Provides Prometheus-compliant Counter, Gauge, Histogram, and MetricsRegistry
// for cloud-native telemetry and Grafana scraping (/metrics endpoint).
// ============================================================================

extern "C" {
    fn sp_rt_metrics_registry_create() -> ptr;
    fn sp_rt_metrics_registry_destroy(ptr reg);
    fn sp_rt_metrics_registry_default() -> ptr;
    fn sp_rt_metrics_counter_inc(ptr reg, string name, string help, string labels_json, double val);
    fn sp_rt_metrics_gauge_set(ptr reg, string name, string help, string labels_json, double val);
    fn sp_rt_metrics_gauge_add(ptr reg, string name, string help, string labels_json, double val);
    fn sp_rt_metrics_histogram_observe(ptr reg, string name, string help, string buckets_csv, string labels_json, double val);
    fn sp_rt_metrics_render_prometheus(ptr reg) -> string;
}

public class MetricsRegistry {
    public ptr handle;

    public MetricsRegistry() {
        this.handle = sp_rt_metrics_registry_create();
    }

    public static fn create() -> MetricsRegistry {
        return new MetricsRegistry();
    }

    public static fn getGlobal() -> MetricsRegistry {
        let r = new MetricsRegistry();
        if (r.handle != null) {
            sp_rt_metrics_registry_destroy(r.handle);
        }
        r.handle = sp_rt_metrics_registry_default();
        return r;
    }

    public fn counter(string name) -> Counter {
        return new Counter(this, name, "");
    }

    public fn counterWithHelp(string name, string help) -> Counter {
        return new Counter(this, name, help);
    }

    public fn gauge(string name) -> Gauge {
        return new Gauge(this, name, "");
    }

    public fn gaugeWithHelp(string name, string help) -> Gauge {
        return new Gauge(this, name, help);
    }

    public fn histogram(string name) -> Histogram {
        return new Histogram(this, name, "", "");
    }

    public fn histogramWithHelp(string name, string help) -> Histogram {
        return new Histogram(this, name, help, "");
    }

    public fn histogramWithBuckets(string name, string help, string bucketsCsv) -> Histogram {
        return new Histogram(this, name, help, bucketsCsv);
    }

    public fn render() -> string {
        if (this.handle == null) { return ""; }
        return sp_rt_metrics_render_prometheus(this.handle);
    }
}

public class Counter {
    public MetricsRegistry registry;
    public string name;
    public string help;

    public Counter(MetricsRegistry registry, string name, string help = "") {
        this.registry = registry;
        this.name = name;
        this.help = help;
    }

    public fn inc(double delta = 1.0) {
        if (this.registry.handle != null) {
            sp_rt_metrics_counter_inc(this.registry.handle, this.name, this.help, "", delta);
        }
    }

    public fn incWith(double delta, string labelsJson) {
        if (this.registry.handle != null) {
            sp_rt_metrics_counter_inc(this.registry.handle, this.name, this.help, labelsJson, delta);
        }
    }
}

public class Gauge {
    public MetricsRegistry registry;
    public string name;
    public string help;

    public Gauge(MetricsRegistry registry, string name, string help = "") {
        this.registry = registry;
        this.name = name;
        this.help = help;
    }

    public fn set(double val) {
        if (this.registry.handle != null) {
            sp_rt_metrics_gauge_set(this.registry.handle, this.name, this.help, "", val);
        }
    }

    public fn setWith(double val, string labelsJson) {
        if (this.registry.handle != null) {
            sp_rt_metrics_gauge_set(this.registry.handle, this.name, this.help, labelsJson, val);
        }
    }

    public fn inc(double delta = 1.0) {
        if (this.registry.handle != null) {
            sp_rt_metrics_gauge_add(this.registry.handle, this.name, this.help, "", delta);
        }
    }

    public fn incWith(double delta, string labelsJson) {
        if (this.registry.handle != null) {
            sp_rt_metrics_gauge_add(this.registry.handle, this.name, this.help, labelsJson, delta);
        }
    }

    public fn dec(double delta = 1.0) {
        if (this.registry.handle != null) {
            sp_rt_metrics_gauge_add(this.registry.handle, this.name, this.help, "", -delta);
        }
    }

    public fn decWith(double delta, string labelsJson) {
        if (this.registry.handle != null) {
            sp_rt_metrics_gauge_add(this.registry.handle, this.name, this.help, labelsJson, -delta);
        }
    }
}

public class Histogram {
    public MetricsRegistry registry;
    public string name;
    public string help;
    public string bucketsCsv;

    public Histogram(MetricsRegistry registry, string name, string help = "", string bucketsCsv = "") {
        this.registry = registry;
        this.name = name;
        this.help = help;
        this.bucketsCsv = bucketsCsv;
    }

    public fn observe(double val) {
        if (this.registry.handle != null) {
            sp_rt_metrics_histogram_observe(this.registry.handle, this.name, this.help, this.bucketsCsv, "", val);
        }
    }

    public fn observeWith(double val, string labelsJson) {
        if (this.registry.handle != null) {
            sp_rt_metrics_histogram_observe(this.registry.handle, this.name, this.help, this.bucketsCsv, labelsJson, val);
        }
    }
}

public class Metrics {
    public static fn counter(string name) -> Counter {
        return MetricsRegistry.getGlobal().counter(name);
    }

    public static fn counterWithHelp(string name, string help) -> Counter {
        return MetricsRegistry.getGlobal().counterWithHelp(name, help);
    }

    public static fn gauge(string name) -> Gauge {
        return MetricsRegistry.getGlobal().gauge(name);
    }

    public static fn gaugeWithHelp(string name, string help) -> Gauge {
        return MetricsRegistry.getGlobal().gaugeWithHelp(name, help);
    }

    public static fn histogram(string name) -> Histogram {
        return MetricsRegistry.getGlobal().histogram(name);
    }

    public static fn histogramWithHelp(string name, string help) -> Histogram {
        return MetricsRegistry.getGlobal().histogramWithHelp(name, help);
    }

    public static fn histogramWithBuckets(string name, string help, string bucketsCsv) -> Histogram {
        return MetricsRegistry.getGlobal().histogramWithBuckets(name, help, bucketsCsv);
    }

    public static fn render() -> string {
        return MetricsRegistry.getGlobal().render();
    }
}
