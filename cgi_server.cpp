#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <strings.h>
#include <boost/asio.hpp>
#include <sys/stat.h>

#define MAX_SERVER  5

using boost::asio::ip::tcp;
using boost::asio::io_service;
using namespace std;

boost::asio::io_service my_io_service;
string TEST_CASE_DIR("./test_case");

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
        memset(data, '\0', max_length);
        memset(empty, '\0', 1);
        memset(request_method, '\0', 8);
        memset(request_uri, '\0', 1024);
        memset(query_string, '\0', 1024);
        memset(server_protocol, '\0', 32);
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
    string http_host;
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
        sscanf(data, "%s %s %s\r\n", request_method, request_uri, server_protocol);
        #if 1
        cout << "Method: " << request_method << endl
                << "Protocol: " << server_protocol << endl;
        #endif

        // Parse query string
        char tmp[64];
        memset(tmp, '\0', 64);
        sscanf(request_uri, "%[^?]?%s", tmp, query_string);
        prog_name = "." + string(tmp);
        #if 1
        cout << "URI: " << request_uri << endl
                << "Program Name: " << prog_name << endl
                << "Query String: " << query_string << endl;
        #endif 

        // Get server and remote address information
        server_addr = sock.local_endpoint().address().to_string();
        server_port = to_string(static_cast<unsigned short>(sock.local_endpoint().port()));
        remote_addr = sock.remote_endpoint().address().to_string();
        remote_port = to_string(static_cast<unsigned short>(sock.remote_endpoint().port()));
        http_host = server_addr + ":" + server_port;
        #if 1
        cout << "Server: " << server_addr << ":" << server_port << endl;
        cout << "Remote: " << remote_addr << ":" << remote_port << endl;
        #endif
    }

    void do_read() {
        // cout << "Do Read" << endl;
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
        // cout << "Do Write" << endl;
        auto self(shared_from_this());
        boost::asio::async_write(sock, boost::asio::buffer(http_200, strlen(http_200)),
            [this, self](boost::system::error_code ec, size_t length) {
                /* Write Handler */
                if (!ec) {
                    parse_request();

                    if (prog_name.compare("./panel.cgi") == 0) {
                        handle_panel_cgi();
                    } else if (prog_name.compare("./console.cgi") == 0) {
                        handle_console_cgi();
                    } else {
                        // Ignore other CGI program
                    }

                }
            }
        );
    }

    void do_close() {
        sock.close();
    }

    void handle_panel_cgi() {
        ostringstream html_content;
        string test_case_menu;
        string host_menu;
        char tmp[128];

        // Create test case menu
        for (int x=0; x < 5; ++x) {
            memset(tmp, '\0', 128);
            sprintf(tmp, "<option value=\"t%d.txt\">t%d.txt</option>", x+1, x+1);
            test_case_menu += tmp;
        }

        // Create host menu
        for (int x=1; x < 13; ++x) {
            memset(tmp, '\0', 128);
            sprintf(tmp, "<option value=\"nplinux%d.cs.nctu.edu.tw\">nplinux%d</option>", x, x);
            host_menu += tmp;
        }

        // Build html content
        html_content << "Content-type: text/html\r\n\r\n";
        html_content << "<!DOCTYPE html>"
            << "<html lang=\"en\">"
            << "<head>"
            << "<title>NP Project 3 Panel</title>"
            << "<link"
            << "    rel=\"stylesheet\""
            << "    href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\""
            << "    integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\""
            << "    crossorigin=\"anonymous\""
            << "/>"
            << "<link"
            << "    href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\""
            << "    rel=\"stylesheet\""
            << "/>"
            << "<link"
            << "    rel=\"icon\""
            << "    type=\"image/png\""
            << "    href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\""
            << "/>"
            << "<style>"
            << "* {"
            << "font-family: 'Source Code Pro', monospace;"
            << "}"
            << "</style>"
            << "</head>"
            << "<body class=\"bg-secondary pt-5\">"
            << "<form action=\"console.cgi\" method=\"GET\">"
            << "<table class=\"table mx-auto bg-light\" style=\"width: inherit\">"
            << "<thead class=\"thead-dark\">"
            << "    <tr>"
            << "        <th scope=\"col\">#</th>"
            << "        <th scope=\"col\">Host</th>"
            << "        <th scope=\"col\">Port</th>"
            << "        <th scope=\"col\">Input File</th>"
            << "    </tr>"
            << "</thead>"
            << "<tbody>";
        
        for (int x=0; x < MAX_SERVER; ++x) {
            html_content << "<tr>"
                << "<th scope=\"row\" class=\align-middle\">Session " << x+1 << "</th>"
                << "<td>"
                << "<div class=\"input-group\">"
                << "<select name=\"h" << x << "\" class=\"custom-select\">"
                << "<option></option>"
                << host_menu
                << "</select>"
                << "<div class=\"input-group-append\">"
                << "<span class=\"input-group-text\">.cs.nctu.edu.tw</span>"
                << "</div>"
                << "</div>"
                << "</td>"
                << "<td>"
                << "<input name=\"p" << x << "\" type=\"text\" class=\"form-control\" size=\"5\" />"
                << "</td>"
                << "<td>"
                << "<select name=\"f" << x << "\" class=\"custom-select\">"
                << "<option></option>"
                << test_case_menu
                << "</select>"
                << "</td>"
                << "</tr>";
        }

        html_content << "<tr>"
            << "<td colspan=\"3\"></td>"
            << "<td>"
            << "<button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>"
            << "</td>"
            << "</tr>"
            << "</tbody>"
            << "</table>"
            << "</form>"
            << "</body>"
            << "</html>";

        // Write the html content to the browser
        string html_content_string = html_content.str();
        auto self(shared_from_this());
        boost::asio::async_write(sock, boost::asio::buffer(html_content_string.c_str(), html_content_string.length()),
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    do_close();
                } else {
                    perror("Handle panei CGI");
                }
            }
        );
    }

    void handle_console_cgi() {
        cout << "Handle Console CGI" << endl;
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

        boost::asio::io_context io_context;

        server s(io_context, atoi(argv[1]));

        io_context.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}