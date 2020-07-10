
#include "login_client.h"


void read ( const string& filename, string& data ) {
	ifstream file ( filename.c_str (), ios::in );
	if ( file.is_open () )
	{
		stringstream ss;
		ss << file.rdbuf ();

		file.close ();

		data = ss.str ();
	}

	return;
}

int main(int argc, char** argv) {
  string cert;
	string key;
	string root;
	string server_address {"localhost:50051"};

	read ( "../../../../../client.crt", cert );
	read ( "../../../../../client.key", key );
	read ( "../../../../../ca.crt", root );
  LoginClient client(cert, key, root, server_address);
  bool flag = true;
  while (flag) {
    cout << "Please choose service: 1. Sign up; 2. Log in; 0. quit" << endl;
    int num = 0;
    cin >> num;
    if (num == 1) {
      cout << "Enter your username: " << endl;
      string user;
      cin >> user;
      cout << "Enter your password: " << endl;
      string pwd;
      cin >> pwd;
      string reply = client.SignUp(user, pwd);
      cout << "sign up received: " << reply << endl;
    } else if (num == 2) {
      cout << "Enter your username: " << endl;
      string user;
      cin >> user;
      cout << "Enter your password: " << endl;
      string pwd;
      cin >> pwd;
      string reply = client.Login(user, pwd);
      cout << "log in received: " << reply << endl;
    } else if (num == 0) {
      flag = false;
    } else {
      cout << "Error number" << endl;
    }
  }
  return 0;
}