#include <iostream>
#include <string>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

int main() {
    string line("None");
    cout << "Trying to connect to server: ";
	tcp::iostream s("127.0.0.1", "10002"); 
	getline(s, line); 
	cout << line;
    if ( line == "OK" ) {
        cout << "\n** Succeded, sending data\n";
        string line("70 70 130 200 360 52 230 200 100 400 300 300\n");
        s << line << endl;
    }
    else
        cout << "\n** Failed...\n";

	return 0;
}
