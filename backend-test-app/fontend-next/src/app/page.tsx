"use client";

import React, { useState, useEffect, useCallback } from "react";
import { api, Item, HealthStatus, CreateItemInput, UpdateItemInput } from "@/lib/api";
import {
  Server,
  Activity,
  Database,
  CheckCircle2,
  Clock,
  AlertCircle,
  Plus,
  Trash2,
  Edit2,
  RefreshCw,
  ExternalLink,
  Search,
  Check,
  X,
  Layers,
  Code2,
  ShieldCheck,
  Terminal,
} from "lucide-react";

interface ApiLog {
  id: string;
  timestamp: string;
  method: string;
  endpoint: string;
  status: number | string;
  latencyMs: number;
}

export default function DashboardPage() {
  const [items, setItems] = useState<Item[]>([]);
  const [health, setHealth] = useState<HealthStatus | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [error, setError] = useState<string | null>(null);
  const [backendOnline, setBackendOnline] = useState<boolean>(false);
  const [backendLatency, setBackendLatency] = useState<number>(0);

  // Filters & Search
  const [searchQuery, setSearchQuery] = useState("");
  const [statusFilter, setStatusFilter] = useState<string>("all");
  const [priorityFilter, setPriorityFilter] = useState<string>("all");

  // Modal State
  const [isModalOpen, setIsModalOpen] = useState(false);
  const [editingItem, setEditingItem] = useState<Item | null>(null);
  const [formData, setFormData] = useState<CreateItemInput>({
    title: "",
    description: "",
    status: "pending",
    priority: "normal",
  });
  const [formSubmitting, setFormSubmitting] = useState(false);

  // API Activity Logs
  const [logs, setLogs] = useState<ApiLog[]>([]);

  const addLog = (method: string, endpoint: string, status: number | string, latencyMs: number) => {
    const newLog: ApiLog = {
      id: Math.random().toString(36).substring(7),
      timestamp: new Date().toLocaleTimeString(),
      method,
      endpoint,
      status,
      latencyMs,
    };
    setLogs((prev) => [newLog, ...prev.slice(0, 19)]);
  };

  // Fetch Health & Items
  const fetchData = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      // 1. Health check
      try {
        const healthRes = await api.getHealth();
        setHealth(healthRes.data);
        setBackendOnline(true);
        setBackendLatency(healthRes.latencyMs);
        addLog("GET", "/api/health", 200, healthRes.latencyMs);
      } catch (err: any) {
        setBackendOnline(false);
        addLog("GET", "/api/health", "FAILED", 0);
        setError("SP Lang backend is not running at http://127.0.0.1:8080. Start server.exe to connect.");
      }

      // 2. Fetch items
      const itemsRes = await api.getItems();
      setItems(itemsRes.data);
      addLog("GET", "/api/items", 200, itemsRes.latencyMs);
    } catch (err: any) {
      if (backendOnline) {
        setError(err.message || "Failed to load tasks");
      }
    } finally {
      setLoading(false);
    }
  }, [backendOnline]);

  useEffect(() => {
    fetchData();
    const interval = setInterval(() => {
      api.getHealth().then(
        (res) => {
          setHealth(res.data);
          setBackendOnline(true);
          setBackendLatency(res.latencyMs);
        },
        () => {
          setBackendOnline(false);
        }
      );
    }, 5000);
    return () => clearInterval(interval);
  }, [fetchData]);

  // Open modal for Create
  const handleOpenCreate = () => {
    setEditingItem(null);
    setFormData({
      title: "",
      description: "",
      status: "pending",
      priority: "normal",
    });
    setIsModalOpen(true);
  };

  // Open modal for Edit
  const handleOpenEdit = (item: Item) => {
    setEditingItem(item);
    setFormData({
      title: item.title,
      description: item.description,
      status: item.status,
      priority: item.priority,
    });
    setIsModalOpen(true);
  };

  // Submit Create or Edit
  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!formData.title.trim()) return;

    setFormSubmitting(true);
    try {
      if (editingItem) {
        // Update
        const res = await api.updateItem(editingItem.id, formData);
        addLog("PUT", `/api/items/${editingItem.id}`, 200, res.latencyMs);
      } else {
        // Create
        const res = await api.createItem(formData);
        addLog("POST", "/api/items", 201, res.latencyMs);
      }
      setIsModalOpen(false);
      await fetchData();
    } catch (err: any) {
      alert(`Error saving item: ${err.message}`);
    } finally {
      setFormSubmitting(false);
    }
  };

  // Toggle item status between completed and pending
  const handleToggleStatus = async (item: Item) => {
    const nextStatus = item.status === "completed" ? "pending" : "completed";
    try {
      const res = await api.updateItem(item.id, { status: nextStatus });
      addLog("PUT", `/api/items/${item.id}`, 200, res.latencyMs);
      setItems((prev) =>
        prev.map((it) => (it.id === item.id ? { ...it, status: nextStatus as any } : it))
      );
    } catch (err: any) {
      alert(`Failed to update status: ${err.message}`);
    }
  };

  // Delete item
  const handleDelete = async (id: number) => {
    if (!confirm("Are you sure you want to delete this task?")) return;
    try {
      const res = await api.deleteItem(id);
      addLog("DELETE", `/api/items/${id}`, 200, res.latencyMs);
      setItems((prev) => prev.filter((it) => it.id !== id));
    } catch (err: any) {
      alert(`Failed to delete item: ${err.message}`);
    }
  };

  // Filter items
  const filteredItems = items.filter((item) => {
    const matchesSearch =
      item.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.description.toLowerCase().includes(searchQuery.toLowerCase());
    const matchesStatus = statusFilter === "all" || item.status === statusFilter;
    const matchesPriority = priorityFilter === "all" || item.priority === priorityFilter;
    return matchesSearch && matchesStatus && matchesPriority;
  });

  const countPending = items.filter((i) => i.status === "pending").length;
  const countInProgress = items.filter((i) => i.status === "in_progress").length;
  const countCompleted = items.filter((i) => i.status === "completed").length;

  return (
    <div className="min-h-screen text-slate-100 flex flex-col">
      {/* Top Navigation */}
      <header className="border-b border-slate-800 bg-slate-950/80 backdrop-blur-md sticky top-0 z-40">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-indigo-500 to-purple-600 flex items-center justify-center font-bold text-white shadow-lg shadow-indigo-500/20">
              SP
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <span className="font-bold text-lg text-white tracking-tight">SP Lang 3.0</span>
                <span className="text-xs px-2 py-0.5 rounded-full bg-indigo-500/10 text-indigo-400 border border-indigo-500/20 font-medium">
                  Fullstack Enterprise
                </span>
              </div>
              <p className="text-xs text-slate-400">Next.js App Router + Native High-Throughput REST Engine</p>
            </div>
          </div>

          <div className="flex items-center space-x-3">
            {/* Backend Connectivity Status */}
            <div
              className={`flex items-center space-x-2 px-3 py-1.5 rounded-full text-xs font-medium border ${
                backendOnline
                  ? "bg-emerald-500/10 border-emerald-500/30 text-emerald-400"
                  : "bg-rose-500/10 border-rose-500/30 text-rose-400"
              }`}
            >
              <span
                className={`w-2 h-2 rounded-full animate-pulse ${
                  backendOnline ? "bg-emerald-400" : "bg-rose-400"
                }`}
              />
              <span>{backendOnline ? `Native Backend Online (${backendLatency}ms)` : "Backend Offline"}</span>
            </div>

            {/* Docs Links */}
            <a
              href="http://localhost:8000/docs"
              target="_blank"
              rel="noreferrer"
              className="flex items-center space-x-1.5 text-xs text-slate-300 hover:text-white px-3 py-1.5 rounded-lg bg-slate-800/80 hover:bg-slate-700/80 border border-slate-700 transition"
            >
              <Code2 className="w-3.5 h-3.5 text-indigo-400" />
              <span>Swagger UI</span>
              <ExternalLink className="w-3 h-3 text-slate-400" />
            </a>

            <a
              href="http://localhost:8000/redoc"
              target="_blank"
              rel="noreferrer"
              className="hidden sm:flex items-center space-x-1.5 text-xs text-slate-300 hover:text-white px-3 py-1.5 rounded-lg bg-slate-800/80 hover:bg-slate-700/80 border border-slate-700 transition"
            >
              <span>Redoc</span>
              <ExternalLink className="w-3 h-3 text-slate-400" />
            </a>
          </div>
        </div>
      </header>

      {/* Main Container */}
      <main className="flex-1 max-w-7xl mx-auto w-full px-4 sm:px-6 lg:px-8 py-8 space-y-8">
        {/* System Telemetry & Metadata Cards */}
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
          <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-5 shadow-sm hover:border-slate-700 transition">
            <div className="flex items-center justify-between text-slate-400 mb-2">
              <span className="text-xs font-semibold uppercase tracking-wider">Engine Runtime</span>
              <Server className="w-4 h-4 text-indigo-400" />
            </div>
            <div className="text-xl font-bold text-white">SP Lang 3.0</div>
            <div className="text-xs text-slate-400 mt-1 flex items-center space-x-1">
              <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
              <span>Native LLVM Ahead-of-Time</span>
            </div>
          </div>

          <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-5 shadow-sm hover:border-slate-700 transition">
            <div className="flex items-center justify-between text-slate-400 mb-2">
              <span className="text-xs font-semibold uppercase tracking-wider">Storage Layer</span>
              <Database className="w-4 h-4 text-purple-400" />
            </div>
            <div className="text-xl font-bold text-white">PostgreSQL / SQLite</div>
            <div className="text-xs text-slate-400 mt-1">Neon Cloud Connection Ready</div>
          </div>

          <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-5 shadow-sm hover:border-slate-700 transition">
            <div className="flex items-center justify-between text-slate-400 mb-2">
              <span className="text-xs font-semibold uppercase tracking-wider">Active Tasks</span>
              <Activity className="w-4 h-4 text-emerald-400" />
            </div>
            <div className="text-2xl font-bold text-white">{items.length}</div>
            <div className="text-xs text-slate-400 mt-1">
              {countCompleted} completed · {countInProgress} active
            </div>
          </div>

          <div className="bg-slate-900/60 border border-slate-800 rounded-2xl p-5 shadow-sm hover:border-slate-700 transition">
            <div className="flex items-center justify-between text-slate-400 mb-2">
              <span className="text-xs font-semibold uppercase tracking-wider">API Health</span>
              <Layers className="w-4 h-4 text-blue-400" />
            </div>
            <div className="text-xl font-bold text-emerald-400 flex items-center space-x-2">
              <span>{backendOnline ? "Operational" : "Offline"}</span>
            </div>
            <div className="text-xs text-slate-400 mt-1">
              {backendOnline ? `Response time: ${backendLatency}ms` : "Port 8080 unreachable"}
            </div>
          </div>
        </div>

        {/* Error Banner */}
        {error && (
          <div className="p-4 rounded-xl bg-rose-500/10 border border-rose-500/20 text-rose-300 flex items-start space-x-3">
            <AlertCircle className="w-5 h-5 flex-shrink-0 mt-0.5 text-rose-400" />
            <div className="flex-1 text-sm">
              <p className="font-semibold">Backend Connection Issue</p>
              <p className="text-rose-300/80 mt-0.5">{error}</p>
            </div>
            <button
              onClick={fetchData}
              className="px-3 py-1 bg-rose-500/20 hover:bg-rose-500/30 rounded-lg text-xs font-semibold text-rose-200"
            >
              Retry
            </button>
          </div>
        )}

        {/* Toolbar & Filter Section */}
        <div className="flex flex-col sm:flex-row gap-4 items-stretch sm:items-center justify-between bg-slate-900/40 p-4 rounded-2xl border border-slate-800">
          <div className="flex flex-1 flex-wrap items-center gap-3">
            {/* Search */}
            <div className="relative flex-1 min-w-[200px]">
              <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-slate-400" />
              <input
                type="text"
                placeholder="Search tasks, descriptions..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="w-full bg-slate-950/80 border border-slate-800 rounded-xl pl-9 pr-4 py-2 text-sm text-slate-200 placeholder:text-slate-500 focus:outline-none focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500"
              />
            </div>

            {/* Status Filter */}
            <select
              value={statusFilter}
              onChange={(e) => setStatusFilter(e.target.value)}
              className="bg-slate-950/80 border border-slate-800 rounded-xl px-3 py-2 text-sm text-slate-300 focus:outline-none focus:border-indigo-500"
            >
              <option value="all">All Statuses ({items.length})</option>
              <option value="pending">Pending ({countPending})</option>
              <option value="in_progress">In Progress ({countInProgress})</option>
              <option value="completed">Completed ({countCompleted})</option>
            </select>

            {/* Priority Filter */}
            <select
              value={priorityFilter}
              onChange={(e) => setPriorityFilter(e.target.value)}
              className="bg-slate-950/80 border border-slate-800 rounded-xl px-3 py-2 text-sm text-slate-300 focus:outline-none focus:border-indigo-500"
            >
              <option value="all">All Priorities</option>
              <option value="high">High Priority</option>
              <option value="normal">Normal Priority</option>
              <option value="low">Low Priority</option>
            </select>
          </div>

          {/* Action Buttons */}
          <div className="flex items-center space-x-2">
            <button
              onClick={fetchData}
              disabled={loading}
              className="p-2.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl border border-slate-700 transition"
              title="Refresh Tasks"
            >
              <RefreshCw className={`w-4 h-4 ${loading ? "animate-spin text-indigo-400" : ""}`} />
            </button>

            <button
              onClick={handleOpenCreate}
              className="flex items-center space-x-2 bg-gradient-to-r from-indigo-600 to-purple-600 hover:from-indigo-500 hover:to-purple-500 text-white px-4 py-2 rounded-xl text-sm font-semibold shadow-lg shadow-indigo-600/20 transition transform active:scale-95"
            >
              <Plus className="w-4 h-4" />
              <span>Create Task</span>
            </button>
          </div>
        </div>

        {/* Task Cards Grid */}
        <div>
          {loading && items.length === 0 ? (
            <div className="text-center py-20 bg-slate-900/30 rounded-2xl border border-slate-800/80">
              <RefreshCw className="w-8 h-8 animate-spin text-indigo-500 mx-auto mb-3" />
              <p className="text-slate-400 text-sm">Querying SP Lang backend on port 8080...</p>
            </div>
          ) : filteredItems.length === 0 ? (
            <div className="text-center py-16 bg-slate-900/20 rounded-2xl border border-dashed border-slate-800">
              <p className="text-slate-400 text-sm">No tasks found matching your filter criteria.</p>
              <button
                onClick={handleOpenCreate}
                className="mt-3 text-xs text-indigo-400 hover:text-indigo-300 font-medium"
              >
                + Create your first task
              </button>
            </div>
          ) : (
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {filteredItems.map((item) => {
                const isDone = item.status === "completed";
                const isInProgress = item.status === "in_progress";

                return (
                  <div
                    key={item.id}
                    className={`group bg-slate-900/70 border rounded-2xl p-5 flex flex-col justify-between transition-all duration-200 hover:shadow-xl hover:shadow-indigo-950/20 ${
                      isDone
                        ? "border-emerald-500/20 bg-emerald-950/10"
                        : "border-slate-800 hover:border-slate-700"
                    }`}
                  >
                    <div>
                      {/* Card Header: Badges & ID */}
                      <div className="flex items-center justify-between mb-3">
                        <span className="text-xs font-mono text-slate-500">#{item.id}</span>
                        <div className="flex items-center space-x-2">
                          {/* Priority Pill */}
                          <span
                            className={`text-xs px-2 py-0.5 rounded-full font-medium ${
                              item.priority === "high"
                                ? "bg-rose-500/10 text-rose-400 border border-rose-500/20"
                                : item.priority === "normal"
                                ? "bg-blue-500/10 text-blue-400 border border-blue-500/20"
                                : "bg-slate-500/10 text-slate-400 border border-slate-500/20"
                            }`}
                          >
                            {item.priority}
                          </span>

                          {/* Status Pill */}
                          <span
                            className={`text-xs px-2.5 py-0.5 rounded-full font-semibold flex items-center space-x-1 ${
                              isDone
                                ? "bg-emerald-500/15 text-emerald-400 border border-emerald-500/30"
                                : isInProgress
                                ? "bg-amber-500/15 text-amber-400 border border-amber-500/30"
                                : "bg-slate-800 text-slate-400 border border-slate-700"
                            }`}
                          >
                            <span>{item.status.replace("_", " ")}</span>
                          </span>
                        </div>
                      </div>

                      {/* Title & Description */}
                      <h3
                        className={`text-base font-semibold leading-snug transition ${
                          isDone ? "text-slate-400 line-through" : "text-white group-hover:text-indigo-300"
                        }`}
                      >
                        {item.title}
                      </h3>
                      <p className="text-sm text-slate-400 mt-2 line-clamp-3 leading-relaxed">
                        {item.description || "No description provided."}
                      </p>
                    </div>

                    {/* Card Footer: Metadata & Actions */}
                    <div className="pt-4 mt-4 border-t border-slate-800/80 flex items-center justify-between">
                      <div className="flex items-center space-x-1.5 text-xs text-slate-500 font-mono">
                        <Clock className="w-3.5 h-3.5" />
                        <span>{item.created_at ? item.created_at.split(" ")[0] : "Recent"}</span>
                      </div>

                      <div className="flex items-center space-x-1">
                        {/* Quick Toggle Status */}
                        <button
                          onClick={() => handleToggleStatus(item)}
                          className={`p-1.5 rounded-lg border transition ${
                            isDone
                              ? "bg-emerald-500/20 border-emerald-500/30 text-emerald-300 hover:bg-emerald-500/30"
                              : "bg-slate-800 border-slate-700 text-slate-400 hover:text-white hover:bg-slate-700"
                          }`}
                          title={isDone ? "Mark Pending" : "Mark Completed"}
                        >
                          <Check className="w-3.5 h-3.5" />
                        </button>

                        {/* Edit Button */}
                        <button
                          onClick={() => handleOpenEdit(item)}
                          className="p-1.5 rounded-lg bg-slate-800 border border-slate-700 text-slate-400 hover:text-white hover:bg-slate-700 transition"
                          title="Edit Task"
                        >
                          <Edit2 className="w-3.5 h-3.5" />
                        </button>

                        {/* Delete Button */}
                        <button
                          onClick={() => handleDelete(item.id)}
                          className="p-1.5 rounded-lg bg-slate-800 border border-slate-700 text-slate-400 hover:text-rose-400 hover:border-rose-500/30 hover:bg-rose-500/10 transition"
                          title="Delete Task"
                        >
                          <Trash2 className="w-3.5 h-3.5" />
                        </button>
                      </div>
                    </div>
                  </div>
                );
              })}
            </div>
          )}
        </div>

        {/* Live HTTP Request / Event Inspector */}
        <div className="bg-slate-950/80 border border-slate-800 rounded-2xl overflow-hidden shadow-xl">
          <div className="px-5 py-3.5 bg-slate-900/80 border-b border-slate-800 flex items-center justify-between">
            <div className="flex items-center space-x-2">
              <Terminal className="w-4 h-4 text-indigo-400" />
              <span className="text-sm font-semibold text-white">Live SP Lang Backend HTTP Inspector</span>
              <span className="text-xs text-slate-400 font-mono bg-slate-800/60 px-2 py-0.5 rounded">
                http://127.0.0.1:8080
              </span>
            </div>
            <button
              onClick={() => setLogs([])}
              className="text-xs text-slate-500 hover:text-slate-300 transition"
            >
              Clear Log
            </button>
          </div>

          <div className="p-4 font-mono text-xs max-h-48 overflow-y-auto space-y-2">
            {logs.length === 0 ? (
              <p className="text-slate-500 italic">No network traffic yet. Actions will appear here in real-time.</p>
            ) : (
              logs.map((log) => (
                <div key={log.id} className="flex items-center space-x-3 text-slate-300">
                  <span className="text-slate-500">{log.timestamp}</span>
                  <span
                    className={`font-bold px-1.5 py-0.5 rounded text-[10px] ${
                      log.method === "GET"
                        ? "bg-blue-500/20 text-blue-400"
                        : log.method === "POST"
                        ? "bg-emerald-500/20 text-emerald-400"
                        : log.method === "PUT"
                        ? "bg-amber-500/20 text-amber-400"
                        : "bg-rose-500/20 text-rose-400"
                    }`}
                  >
                    {log.method}
                  </span>
                  <span className="text-slate-200">{log.endpoint}</span>
                  <span
                    className={`font-semibold ${
                      log.status === 200 || log.status === 201
                        ? "text-emerald-400"
                        : "text-rose-400"
                    }`}
                  >
                    {log.status}
                  </span>
                  <span className="text-slate-500 ml-auto">{log.latencyMs}ms</span>
                </div>
              ))
            )}
          </div>
        </div>
      </main>

      {/* Modal for Create / Edit Task */}
      {isModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/60 backdrop-blur-sm animate-fadeIn">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl w-full max-w-md shadow-2xl overflow-hidden">
            <div className="px-6 py-4 border-b border-slate-800 flex items-center justify-between">
              <h2 className="text-lg font-bold text-white">
                {editingItem ? `Edit Task #${editingItem.id}` : "Create New Task"}
              </h2>
              <button
                onClick={() => setIsModalOpen(false)}
                className="p-1 text-slate-400 hover:text-white rounded-lg transition"
              >
                <X className="w-5 h-5" />
              </button>
            </div>

            <form onSubmit={handleSubmit} className="p-6 space-y-4">
              <div>
                <label className="block text-xs font-semibold uppercase tracking-wider text-slate-400 mb-1.5">
                  Title *
                </label>
                <input
                  type="text"
                  required
                  placeholder="e.g. Implement Redis Caching"
                  value={formData.title}
                  onChange={(e) => setFormData({ ...formData, title: e.target.value })}
                  className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3.5 py-2.5 text-sm text-slate-200 focus:outline-none focus:border-indigo-500"
                />
              </div>

              <div>
                <label className="block text-xs font-semibold uppercase tracking-wider text-slate-400 mb-1.5">
                  Description
                </label>
                <textarea
                  rows={3}
                  placeholder="Task details and scope..."
                  value={formData.description}
                  onChange={(e) => setFormData({ ...formData, description: e.target.value })}
                  className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3.5 py-2.5 text-sm text-slate-200 focus:outline-none focus:border-indigo-500"
                />
              </div>

              <div className="grid grid-cols-2 gap-4">
                <div>
                  <label className="block text-xs font-semibold uppercase tracking-wider text-slate-400 mb-1.5">
                    Status
                  </label>
                  <select
                    value={formData.status}
                    onChange={(e) => setFormData({ ...formData, status: e.target.value })}
                    className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3.5 py-2.5 text-sm text-slate-200 focus:outline-none focus:border-indigo-500"
                  >
                    <option value="pending">Pending</option>
                    <option value="in_progress">In Progress</option>
                    <option value="completed">Completed</option>
                  </select>
                </div>

                <div>
                  <label className="block text-xs font-semibold uppercase tracking-wider text-slate-400 mb-1.5">
                    Priority
                  </label>
                  <select
                    value={formData.priority}
                    onChange={(e) => setFormData({ ...formData, priority: e.target.value })}
                    className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3.5 py-2.5 text-sm text-slate-200 focus:outline-none focus:border-indigo-500"
                  >
                    <option value="low">Low</option>
                    <option value="normal">Normal</option>
                    <option value="high">High</option>
                  </select>
                </div>
              </div>

              <div className="pt-4 border-t border-slate-800 flex items-center justify-end space-x-3">
                <button
                  type="button"
                  onClick={() => setIsModalOpen(false)}
                  className="px-4 py-2 rounded-xl text-sm font-semibold text-slate-400 hover:text-white transition"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  disabled={formSubmitting}
                  className="bg-indigo-600 hover:bg-indigo-500 text-white px-5 py-2 rounded-xl text-sm font-semibold transition flex items-center space-x-2"
                >
                  {formSubmitting && <RefreshCw className="w-4 h-4 animate-spin" />}
                  <span>{editingItem ? "Save Changes" : "Create Task"}</span>
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* Footer */}
      <footer className="border-t border-slate-800/80 bg-slate-950 py-6 mt-12">
        <div className="max-w-7xl mx-auto px-4 text-center text-xs text-slate-500">
          <p>
            Powered by <span className="text-indigo-400 font-semibold">SP Lang 3.0</span> Native Ahead-of-Time Backend & Next.js App Router.
          </p>
        </div>
      </footer>
    </div>
  );
}
