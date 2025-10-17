\
    // scanner.cpp
    // Asenkron Boost.Asio tabanlı thread pool ile TCP connect port scanner (timeout destekli)

    #include <boost/asio.hpp>
    #include <boost/algorithm/string.hpp>
    #include <iostream>
    #include <vector>
    #include <string>
    #include <thread>
    #include <mutex>
    #include <atomic>
    #include <chrono>
    #include <queue>
    #include <set>
    #include <memory>

    using namespace boost::asio;
    using ip::tcp;
    using namespace std::chrono_literals;

    struct Result {
        unsigned short port;
        bool open;
        std::string banner;
    };

    std::vector<unsigned short> parse_ports_arg(const std::string& s) {
        std::vector<unsigned short> out;
        std::vector<std::string> parts;
        boost::split(parts, s, boost::is_any_of(","));
        for (auto &p : parts) {
            boost::trim(p);
            if (p.empty()) continue;
            auto dash = p.find('-');
            if (dash != std::string::npos) {
                unsigned int a = std::stoul(p.substr(0, dash));
                unsigned int b = std::stoul(p.substr(dash+1));
                if (a < 1) a = 1;
                if (b > 65535) b = 65535;
                for (unsigned int x = a; x <= b; ++x) out.push_back(static_cast<unsigned short>(x));
            } else {
                unsigned int v = std::stoul(p);
                if (v >= 1 && v <= 65535) out.push_back(static_cast<unsigned short>(v));
            }
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
        return out;
    }

    int main(int argc, char* argv[]) {
        if (argc < 3) {
            std::cout << "Kullanım: " << argv[0] << " <hedef> <portlar> [threads] [timeout_ms]\n";
            std::cout << "Örnek: " << argv[0] << " 192.168.1.10 1-1024 200 300\n";
            return 1;
        }

        std::string target = argv[1];
        std::string ports_arg = argv[2];
        int threads = (argc >= 4) ? std::stoi(argv[3]) : 100;
        int timeout_ms = (argc >= 5) ? std::stoi(argv[4]) : 300;

        if (threads < 1) threads = 1;
        if (threads > 2000) threads = 2000;
        if (timeout_ms < 10) timeout_ms = 10;

        auto ports = parse_ports_arg(ports_arg);
        if (ports.empty()) {
            std::cerr << "Geçerli port listesi bulunamadı.\n";
            return 2;
        }

        io_context io_ctx;
        tcp::resolver resolver(io_ctx);
        boost::system::error_code ec;
        auto endpoints = resolver.resolve(target, "", ec);
        if (ec) {
            std::cerr << "Hedef çözümlenemedi: " << ec.message() << "\n";
            return 3;
        }

        std::string ip_str;
        for (auto &e : endpoints) { ip_str = e.endpoint().address().to_string(); break; }
        if (ip_str.empty()) { std::cerr << "Adres bulunamadı.\n"; return 4; }

        std::cout << "Hedef: " << target << " -> " << ip_str << "\n";
        std::cout << "Port sayısı: " << ports.size() << ", Threads: " << threads << ", Timeout: " << timeout_ms << " ms\n";

        std::queue<unsigned short> q;
        for (auto p : ports) q.push(p);

        std::mutex q_mtx; std::mutex out_mtx;
        std::vector<Result> results;
        std::atomic<int> active{0};

        // Worker function: çalıştırılacak iş: port al, async connect ile dene, timer ile timeout
        auto worker = [&](int id) {
            while (true) {
                unsigned short port = 0;
                {
                    std::lock_guard<std::mutex> lg(q_mtx);
                    if (q.empty()) break;
                    port = q.front(); q.pop();
                }

                active++;
                // create socket and timer on heap to share with handlers
                auto sock = std::make_shared<tcp::socket>(io_ctx);
                auto timer = std::make_shared<steady_timer>(io_ctx);
                tcp::endpoint ep(ip::address::from_string(ip_str), port);
                bool was_open = false;
                std::string banner;

                // Set timer
                timer->expires_after(std::chrono::milliseconds(timeout_ms));

                // Handler for connect
                auto connect_handler = [&, sock, timer, port, &was_open, &banner](const boost::system::error_code& ec_connect) mutable {
                    if (!ec_connect) {
                        was_open = true;
                        // attempt small banner grab (non-blocking, simple)
                        boost::system::error_code write_ec;
                        std::string probe = "HEAD / HTTP/1.0\r\n\r\n";
                        boost::asio::write(*sock, boost::asio::buffer(probe), write_ec);
                        if (!write_ec) {
                            boost::system::error_code read_ec;
                            char buf[256];
                            size_t n = sock->read_some(boost::asio::buffer(buf), read_ec);
                            if (n > 0) {
                                banner = std::string(buf, buf + (n>200?200:n));
                            }
                        }
                    }
                    // cancel timer (if still pending)
                    boost::system::error_code ignored;
                    timer->cancel(ignored);
                    // close socket
                    boost::system::error_code close_ec; sock->close(close_ec);
                };

                // Handler for timeout
                auto timeout_handler = [&, sock, timer, port](const boost::system::error_code& ec_timer) mutable {
                    if (ec_timer != boost::asio::error::operation_aborted) {
                        // timer expired -> cancel socket
                        boost::system::error_code ignored;
                        sock->close(ignored);
                    }
                };

                #if defined(__APPLE__)
                // On macOS sometimes sockets need options; leave as-is for portability
                #endif

                // Start async operations
                boost::system::error_code open_ec;
                sock->open(ep.protocol(), open_ec);
                if (open_ec) {
                    // failed to open socket on this platform/limit
                } else {
                    // start connect
                    sock->async_connect(ep, connect_handler);
                    timer->async_wait(timeout_handler);

                    #if defined(BOOST_ASIO_HAS_IO_URING) || defined(BOOST_ASIO_HAS_IOCP) || defined(BOOST_ASIO_HAS_EPOLL)
                    // platform-optimized io will be used by run_for
                    #endif

                    // run the io_context *until this connection completes or timer fires*
                    // To avoid running global io_ctx entirely (affecting other threads), we'll run
                    // one run_for with timeout slightly above timeout_ms. This is a small simplification.
                    io_ctx.run_for(std::chrono::milliseconds(timeout_ms + 50));
                    // Reset io_ctx's running state so other threads can continue using it
                    io_ctx.restart();
                }

                {
                    std::lock_guard<std::mutex> lg(out_mtx);
                    results.push_back({port, was_open, banner});
                    if (was_open) {
                        std::cout << "[OPEN] " << port;
                        if (!banner.empty()) std::cout << "  banner: " << banner;
                        std::cout << "\n";
                    }
                }

                active--;
            }
        };

        // Launch threads that pump the same io_context and perform tasks
        std::vector<std::thread> pool;
        for (int i = 0; i < threads; ++i) {
            pool.emplace_back(worker, i);
        }

        for (auto &t : pool) t.join();

        std::cout << "\n=== Tarama özeti ===\n";
        int open_count = 0;
        for (auto &r : results) {
            if (r.open) {
                ++open_count;
                std::cout << r.port << " OPEN";
                if (!r.banner.empty()) std::cout << " | banner: " << r.banner;
                std::cout << "\n";
            }
        }
        std::cout << "Toplam açık port: " << open_count << "\n";

        return 0;
    }
