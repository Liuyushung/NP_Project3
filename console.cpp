#include <strings.h>
#include <iostream>
#include <sstream>
#include <vector>

#define MAX_SERVER      5
#define ATTRIBUTES_SIZE 3

using namespace std;

typedef struct client_info {
    string host;
    string port;
    string fname;
} ClientInfo;

/* Global Values */
ClientInfo clients[MAX_SERVER];

void debug_clients() {
    for (auto c: clients) {
        cout << "Host: " << c.host << " Port: " << c.port << " File: " << c.fname << endl;
    }
}

void init_golbal() {
    bzero(clients, sizeof(ClientInfo)*5);
}

vector<string> my_split(string str, char delimeter) {
    stringstream ss(str);
    string token;
    vector<string> result;

    while (getline(ss, token, delimeter)) {
        result.push_back(token);
    }

    return result;
}

void parse_query() {
    vector<string> raw_queries;
    string query = getenv("QUERY_STRING");
    #if 0
    cerr << query << endl;
    #endif

    raw_queries = my_split(query, '&');

    for (auto &s: raw_queries) {
        int x = s.find("=");
        string value = s.substr(x+1, s.length() - x - 1);

        switch (s[0]) {
            case 'h':
                /* host */
                clients[(int) (s[x-1] - '0')].host = value;
                break;
            case 'p':
                /* port */
                clients[(int) (s[x-1] - '0')].port = value;
                break;
            case 'f':
                /* file */
                clients[(int) (s[x-1] - '0')].fname = value;
                break;
        }
    }

    return;
}

void print_html() {
    cout << "Content-type: text/html\r\n\r\n";
    cout << "\
<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"UTF-8\" />\
    <title>NP Project 3 Console</title>\
    <link\
      rel=\"stylesheet\"\
      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
      crossorigin=\"anonymous\"\
    />\
    <link\
      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
      rel=\"stylesheet\"\
    />\
    <link\
      rel=\"icon\"\
      type=\"image/png\"\
      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\
    />\
    <style>\
      * {\
        font-family: 'Source Code Pro', monospace;\
        font-size: 1rem !important;\
      }\
      body {\
        background-color: #212529;\
      }\
      pre {\
        color: #cccccc;\
      }\
      b {\
        color: #01b468;\
      }\
    </style>\
  </head>\
  <body>\
    <table class=\"table table-dark table-bordered\">\
      <thead>\
        <tr id=\"table_tr\">\
        </tr>\
      </thead>\
      <tbody>\
        <tr id=\"session\">\
        </tr>\
      </tbody>\
    </table>\
  </body>\
</html>";

    fflush(stdout);
}

int main(int argc, char *argv[]) {
    setenv("QUERY_STRING", "h0=nplinux1.cs.nctu.edu.tw&p0=65531&f0=t1.txt&h1=nplinux2.cs.nctu.edu.tw&p1=65532&f1=t2.txt&h2=nplinux3.cs.nctu.edu.tw&p2=65533&f2=t3.txt&h3=nplinux4.cs.nctu.edu.tw&p3=65534&f3=t4.txt&h4=nplinux5.cs.nctu.edu.tw&p4=65535&f4=t5.txt", 1);
    init_golbal();
    parse_query();

    #if 0
    debug_clients();
    #endif


    return 0;
}