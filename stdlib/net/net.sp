module std.net;

// ============================================================================
// Standard Library Non-Blocking Networking & Sockets (SP 1.6)
// ============================================================================

extern "C" {
    fn sp_rt_net_init();
    fn sp_rt_net_shutdown();
    fn sp_rt_net_resolve(string host) -> string;

    fn sp_rt_tcp_connect(string host, int port, int timeoutMs) -> ptr;
    fn sp_rt_tcp_write(ptr sock, string str) -> int;
    fn sp_rt_tcp_read(ptr sock, int maxBytes) -> string;
    fn sp_rt_tcp_read_all(ptr sock, int timeoutMs) -> string;
    fn sp_rt_tcp_set_nonblocking(ptr sock, bool nb) -> bool;
    fn sp_rt_tcp_set_timeout(ptr sock, int ms) -> bool;
    fn sp_rt_tcp_close(ptr sock);

    fn sp_rt_tcp_listen(string host, int port, int backlog) -> ptr;
    fn sp_rt_tcp_accept(ptr listener) -> ptr;
    fn sp_rt_tcp_accept_timeout(ptr listener, int timeoutMs) -> ptr;
    fn sp_rt_tcp_listener_port(ptr listener) -> int;
    fn sp_rt_tcp_listener_close(ptr listener);

    fn sp_rt_udp_bind(string host, int port) -> ptr;
    fn sp_rt_udp_send_to(ptr sock, string data, string host, int port) -> int;
    fn sp_rt_udp_close(ptr sock);

    fn sp_rt_eventloop_create() -> ptr;
    fn sp_rt_eventloop_add(ptr loop, ptr sock, int events) -> bool;
    fn sp_rt_eventloop_remove(ptr loop, ptr sock) -> bool;
    fn sp_rt_eventloop_poll(ptr loop, int timeoutMs) -> int;
    fn sp_rt_eventloop_destroy(ptr loop);
}

public class Dns {
    public static fn resolve(string host) -> string {
        return sp_rt_net_resolve(host);
    }
}

public class TcpStream {
    public ptr handle;

    public static fn connect(string host, int port) -> TcpStream {
        let stream = new TcpStream();
        stream.handle = sp_rt_tcp_connect(host, port, 5000);
        return stream;
    }

    public static fn connectTimeout(string host, int port, int timeoutMs) -> TcpStream {
        let stream = new TcpStream();
        stream.handle = sp_rt_tcp_connect(host, port, timeoutMs);
        return stream;
    }

    public fn isConnected() -> bool {
        return this.handle != null;
    }

    public fn write(string str) -> int {
        return sp_rt_tcp_write(this.handle, str);
    }

    public fn read(int maxBytes) -> string {
        return sp_rt_tcp_read(this.handle, maxBytes);
    }

    public fn readAll(int timeoutMs) -> string {
        return sp_rt_tcp_read_all(this.handle, timeoutMs);
    }

    public fn setNonBlocking(bool nb) -> bool {
        return sp_rt_tcp_set_nonblocking(this.handle, nb);
    }

    public fn setTimeout(int ms) -> bool {
        return sp_rt_tcp_set_timeout(this.handle, ms);
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_tcp_close(this.handle);
            this.handle = null;
        }
    }
}

public class TcpListener {
    public ptr handle;

    public static fn bind(string host, int port) -> TcpListener {
        let listener = new TcpListener();
        listener.handle = sp_rt_tcp_listen(host, port, 128);
        return listener;
    }

    public fn isListening() -> bool {
        return this.handle != null;
    }

    public fn port() -> int {
        return sp_rt_tcp_listener_port(this.handle);
    }

    public fn accept() -> TcpStream {
        let client = new TcpStream();
        client.handle = sp_rt_tcp_accept(this.handle);
        return client;
    }

    public fn acceptTimeout(int timeoutMs) -> TcpStream {
        let client = new TcpStream();
        client.handle = sp_rt_tcp_accept_timeout(this.handle, timeoutMs);
        return client;
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_tcp_listener_close(this.handle);
            this.handle = null;
        }
    }
}

public class UdpSocket {
    public ptr handle;

    public static fn bind(string host, int port) -> UdpSocket {
        let sock = new UdpSocket();
        sock.handle = sp_rt_udp_bind(host, port);
        return sock;
    }

    public fn sendTo(string data, string host, int port) -> int {
        return sp_rt_udp_send_to(this.handle, data, host, port);
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_udp_close(this.handle);
            this.handle = null;
        }
    }
}

public class EventLoop {
    public ptr handle;

    public static fn create() -> EventLoop {
        let el = new EventLoop();
        el.handle = sp_rt_eventloop_create();
        return el;
    }

    public fn add(TcpStream stream, int events) -> bool {
        return sp_rt_eventloop_add(this.handle, stream.handle, events);
    }

    public fn remove(TcpStream stream) -> bool {
        return sp_rt_eventloop_remove(this.handle, stream.handle);
    }

    public fn poll(int timeoutMs) -> int {
        return sp_rt_eventloop_poll(this.handle, timeoutMs);
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_eventloop_destroy(this.handle);
            this.handle = null;
        }
    }
}
