#include "tcp_sponge_socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {
    CS144TCPSocket sock;
    
    try {
        sock.connect(Address(host, "http"));
        sock.write("GET " + path + " HTTP/1.1\r\n");
        sock.write("Host: " + host + "\r\n");
        sock.write("Connection: close\r\n");
        sock.write("\r\n");
        sock.shutdown(SHUT_WR);
    } catch (const exception &e) {
        cerr << "Error during connection or writing: " << e.what() << "\n";
        return;
    }

    try {
        while (!sock.eof()) {
            cout << sock.read();
        }
    } catch (const exception &e) {
        cerr << "Error during reading: " << e.what() << "\n";
    }

    try {
        sock.close();
        sock.wait_until_closed();
    } catch (const exception &e) {
        cerr << "Error during shutdown: " << e.what() << "\n";
    }

    cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();
        }

        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        const string host = argv[1];
        const string path = argv[2];

        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
