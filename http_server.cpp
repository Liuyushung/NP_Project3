#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <strings.h>
#include <boost/asio.hpp>
#include <sys/stat.h>
#include <sys/wait.h>

using boost::asio::ip::tcp;
using boost::asio::io_service;
using namespace std;

boost::asio::io_service my_io_service;

void signal_server_handler(int sig) {
    if (sig == SIGCHLD) {
        int stat;
        while(waitpid(-1, &stat, WNOHANG) > 0) {
            // Remove zombie process
        }
    }
}

bool is_file_excutable(string fname) {
    struct stat st;

    if (stat(fname.c_str(), &st) < 0) {
        // perror("is_file_excutable");
        return false;
    }
    if ((st.st_mode & S_IEXEC) != 0) {
        return true;
    }
    
    return false;
}

class session: public enable_shared_from_this<session> {
public:
    session(tcp::socket socket): sock(move(socket)) {
        bzero(data, max_length);
        bzero(empty, 1);
        bzero(request_method, 8);
        bzero(request_uri, 1024);
        bzero(query_string, 1024);
        bzero(server_protocol, 32);
        bzero(http_host, 128);
    }

    void start() {
        do_read();
    }

private:

    tcp::socket sock;
    enum { max_length = 4096 };
    char data[max_length];
    char empty[1];
    char http_200[64] = "HTTP/1.1 200 OK\r\n";
    char request_method[8];
    char request_uri[1024];
    char query_string[1024];
    char server_protocol[32];
    char http_host[128];
    string prog_name;
    string server_addr;
    string server_port;
    string remote_addr;
    string remote_port;

    void parse_request() {
        #if 0
        cout << "Parse Request: " << data << endl;
        #endif

        // Parse first line
        stringstream data_ss(data);
        string tmp_s;
        sscanf(data, "%s %s %s\r\n", request_method, request_uri, server_protocol);
        while(getline(data_ss, tmp_s)) {
            if (sscanf(tmp_s.c_str(), "Host: %s", http_host) != 0) {
                break;
            }
        }
        #if 0
        cout << "Method: " << request_method << endl
            << "Protocol: " << server_protocol << endl
            << "Host: " << http_host << " " << strlen(http_host) << endl;
        #endif

        // Parse query string
        char tmp[64];
        bzero(tmp, 64);
        sscanf(request_uri, "%[^?]?%s", tmp, query_string);
        prog_name = "." + string(tmp);
        #if 0
        cout << "URI: " << request_uri << endl
                << "Program Name: " << prog_name << endl
                << "Query String: " << query_string << endl;
        #endif 

        // Get server and remote address information
        server_addr = sock.local_endpoint().address().to_string();
        server_port = to_string(static_cast<unsigned short>(sock.local_endpoint().port()));
        remote_addr = sock.remote_endpoint().address().to_string();
        remote_port = to_string(static_cast<unsigned short>(sock.remote_endpoint().port()));
        #if 0
        cout << "Server: " << server_addr << ":" << server_port << endl;
        cout << "Remote: " << remote_addr << ":" << remote_port << endl;
        #endif
    }

    void setup_environment() {
        // Do this after fork
        setenv("REQUEST_METHOD", request_method, 1);
        setenv("REQUEST_URI", request_uri, 1);
        setenv("QUERY_STRING", query_string, 1);
        setenv("SERVER_PROTOCOL", server_protocol, 1);
        setenv("HTTP_HOST", http_host, 1);
        setenv("SERVER_ADDR", server_addr.c_str(), 1);
        setenv("SERVER_PORT", server_port.c_str(), 1);
        setenv("REMOTE_ADDR", remote_addr.c_str(), 1);
        setenv("REMOTE_PORT", remote_port.c_str(), 1);
    }

    void do_read() {
        auto self(shared_from_this());
        sock.async_read_some(boost::asio::buffer(data, max_length),
            [this, self](boost::system::error_code ec, size_t length) {
                /* Read Handler */
                if (!ec) {
                    do_write();
                }
            }
        );
    }

    void do_write() {
        auto self(shared_from_this());
        boost::asio::async_write(sock, boost::asio::buffer(http_200, strlen(http_200)),
            [this, self](boost::system::error_code ec, size_t length) {
                /* Write Handler */
                if (!ec) {
                    my_io_service.notify_fork(io_service::fork_prepare);
                    pid_t pid = fork();

                    if (pid < 0) {
                        perror("Session fork");
                        exit(1);
                    } else if (pid > 0) {
                        /* Parent */
                        my_io_service.notify_fork(io_service::fork_parent);
                        sock.close();
                    } else {
                        /* Child */
                        my_io_service.notify_fork(io_service::fork_child);

                        // Parse Request
                        parse_request();

                        // Setup environment variables
                        setup_environment();

                        // Duplicate FD
                        int sock_fd = sock.native_handle();
                        dup2(sock_fd, STDIN_FILENO);
                        dup2(sock_fd, STDOUT_FILENO);
                        sock.close();

                        // Exec CGI program
                        if (is_file_excutable(prog_name)) {
                            if (execlp(prog_name.c_str(), prog_name.c_str(), NULL) < 0) {
                                perror("Exec Fail");
                                cout << "HTTP/1.1 500 Internal Server Error\r\n\r\n";
                            }
                        } else {
                            // cerr << prog_name << " is not executable" << endl;
                        }
                    }

                    // Read the next request
                    do_read();
                }
            }
        );
    }
};

class server {
public:
    server(boost::asio::io_context& io_context, short port): acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        boost::asio::socket_base::reuse_address option(true);
        acceptor_.set_option(option);

        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    make_shared<session>(move(socket))->start();
                }

                do_accept();
            }
        );
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            cerr << "Usage: http_server <port>\n";
            return 1;
        }
        signal(SIGCHLD, signal_server_handler);

        boost::asio::io_context io_context;

        server s(io_context, atoi(argv[1]));

        io_context.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}