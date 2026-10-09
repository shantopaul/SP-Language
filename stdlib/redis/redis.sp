module std.redis;

import std.io;

// ============================================================================
// Standard Library Redis (RESP2 / RESP3) Client & Connection Pool (SP 2.8)
// Provides string caching, TTL, atomic counters, hashes, lists, and pub/sub.
// ============================================================================

extern "C" {
    fn sp_rt_redis_create(string host, int port) -> ptr;
    fn sp_rt_redis_destroy(ptr client);
    fn sp_rt_redis_connect(ptr client) -> bool;
    fn sp_rt_redis_ping(ptr client) -> string;
    fn sp_rt_redis_get(ptr client, string key) -> string;
    fn sp_rt_redis_set(ptr client, string key, string val) -> bool;
    fn sp_rt_redis_setex(ptr client, string key, int seconds, string val) -> bool;
    fn sp_rt_redis_del(ptr client, string key) -> bool;
    fn sp_rt_redis_exists(ptr client, string key) -> bool;
    fn sp_rt_redis_ttl(ptr client, string key) -> int;
    fn sp_rt_redis_incr(ptr client, string key) -> int;
    fn sp_rt_redis_decr(ptr client, string key) -> int;
    fn sp_rt_redis_hget(ptr client, string key, string field) -> string;
    fn sp_rt_redis_hset(ptr client, string key, string field, string val) -> bool;
    fn sp_rt_redis_hdel(ptr client, string key, string field) -> bool;
    fn sp_rt_redis_lpush(ptr client, string key, string val) -> int;
    fn sp_rt_redis_rpush(ptr client, string key, string val) -> int;
    fn sp_rt_redis_lpop(ptr client, string key) -> string;
    fn sp_rt_redis_rpop(ptr client, string key) -> string;
    fn sp_rt_redis_llen(ptr client, string key) -> int;
    fn sp_rt_redis_publish(ptr client, string channelName, string message) -> int;

    fn sp_rt_redis_pool_create(string host, int port, int min_conn, int max_conn) -> ptr;
    fn sp_rt_redis_pool_destroy(ptr pool);
    fn sp_rt_redis_pool_acquire(ptr pool) -> ptr;
    fn sp_rt_redis_pool_release(ptr pool, ptr client);
    fn sp_rt_redis_pool_available(ptr pool) -> int;
    fn sp_rt_redis_pool_total(ptr pool) -> int;
}

public class RedisClient {
    public ptr handle;

    public RedisClient() {
        this.handle = sp_rt_redis_create("127.0.0.1", 6379);
    }

    public fn init(string host, int port) {
        if (this.handle != null) {
            sp_rt_redis_destroy(this.handle);
        }
        this.handle = sp_rt_redis_create(host, port);
    }

    public static fn connect(string host = "127.0.0.1", int port = 6379) -> RedisClient {
        let client = new RedisClient();
        client.init(host, port);
        return client;
    }

    public fn ping() -> string {
        if (this.handle != null) {
            return sp_rt_redis_ping(this.handle);
        }
        return "";
    }

    public fn get(string key) -> string {
        if (this.handle != null) {
            return sp_rt_redis_get(this.handle, key);
        }
        return "";
    }

    public fn set(string key, string val) -> bool {
        if (this.handle != null) {
            return sp_rt_redis_set(this.handle, key, val);
        }
        return false;
    }

    public fn setEx(string key, int seconds, string val) -> bool {
        if (this.handle != null) {
            return sp_rt_redis_setex(this.handle, key, seconds, val);
        }
        return false;
    }

    public fn del(string key) -> bool {
        if (this.handle != null) {
            return sp_rt_redis_del(this.handle, key);
        }
        return false;
    }

    public fn exists(string key) -> bool {
        if (this.handle != null) {
            return sp_rt_redis_exists(this.handle, key);
        }
        return false;
    }

    public fn ttl(string key) -> int {
        if (this.handle != null) {
            return sp_rt_redis_ttl(this.handle, key);
        }
        return -2;
    }

    public fn incr(string key) -> int {
        if (this.handle != null) {
            return sp_rt_redis_incr(this.handle, key);
        }
        return 0;
    }

    public fn decr(string key) -> int {
        if (this.handle != null) {
            return sp_rt_redis_decr(this.handle, key);
        }
        return 0;
    }

    public fn hget(string key, string field) -> string {
        if (this.handle != null) {
            return sp_rt_redis_hget(this.handle, key, field);
        }
        return "";
    }

    public fn hset(string key, string field, string val) -> bool {
        if (this.handle != null) {
            return sp_rt_redis_hset(this.handle, key, field, val);
        }
        return false;
    }

    public fn hdel(string key, string field) -> bool {
        if (this.handle != null) {
            return sp_rt_redis_hdel(this.handle, key, field);
        }
        return false;
    }

    public fn lpush(string key, string val) -> int {
        if (this.handle != null) {
            return sp_rt_redis_lpush(this.handle, key, val);
        }
        return 0;
    }

    public fn rpush(string key, string val) -> int {
        if (this.handle != null) {
            return sp_rt_redis_rpush(this.handle, key, val);
        }
        return 0;
    }

    public fn lpop(string key) -> string {
        if (this.handle != null) {
            return sp_rt_redis_lpop(this.handle, key);
        }
        return "";
    }

    public fn rpop(string key) -> string {
        if (this.handle != null) {
            return sp_rt_redis_rpop(this.handle, key);
        }
        return "";
    }

    public fn llen(string key) -> int {
        if (this.handle != null) {
            return sp_rt_redis_llen(this.handle, key);
        }
        return 0;
    }

    public fn publish(string channelName, string message) -> int {
        if (this.handle != null) {
            return sp_rt_redis_publish(this.handle, channelName, message);
        }
        return 0;
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_redis_destroy(this.handle);
            this.handle = null;
        }
    }
}

public class RedisPool {
    public ptr handle;

    public RedisPool(string host = "127.0.0.1", int port = 6379, int minConn = 2, int maxConn = 10) {
        this.handle = sp_rt_redis_pool_create(host, port, minConn, maxConn);
    }

    public fn init(string host, int port, int minConn, int maxConn) {
        if (this.handle != null) {
            sp_rt_redis_pool_destroy(this.handle);
        }
        this.handle = sp_rt_redis_pool_create(host, port, minConn, maxConn);
    }

    public static fn create(string host = "127.0.0.1", int port = 6379, int minConn = 2, int maxConn = 10) -> RedisPool {
        return new RedisPool(host, port, minConn, maxConn);
    }

    public fn acquire() -> RedisClient {
        let client = new RedisClient();
        if (this.handle != null) {
            let clientPtr = sp_rt_redis_pool_acquire(this.handle);
            client.handle = clientPtr;
        }
        return client;
    }

    public fn release(RedisClient client) {
        if (this.handle != null && client.handle != null) {
            sp_rt_redis_pool_release(this.handle, client.handle);
        }
    }

    public fn available() -> int {
        if (this.handle != null) {
            return sp_rt_redis_pool_available(this.handle);
        }
        return 0;
    }

    public fn total() -> int {
        if (this.handle != null) {
            return sp_rt_redis_pool_total(this.handle);
        }
        return 0;
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_redis_pool_destroy(this.handle);
            this.handle = null;
        }
    }
}
