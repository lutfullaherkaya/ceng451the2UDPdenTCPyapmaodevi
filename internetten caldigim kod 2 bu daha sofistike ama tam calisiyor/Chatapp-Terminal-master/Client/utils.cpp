#include "utils.h"

/* Split a string using delimitter */
vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

inline bool isInteger(const std::string & s)
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtol(s.c_str(), &p, 10) ;

   return (*p == 0) ;
}

int getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

string getMaskedInput(){
	const char BACKSPACE=127;
  	const char RETURN=10;
  	string inp;
  	char ch=0;

  	while((ch=getch()) != RETURN){
  		if(ch == BACKSPACE){
  			if(inp.length()!=0){
  				inp.resize(inp.length()-1);
  			}
  		}
  		else{
  			inp+=ch;
  		}
  	}
  	cout << endl;
  	return inp;
}

int connectToServer(char* servIP, unsigned short servPort){

/********************** CONNECT TO SERVER ***********************/

  int sock;             /* Socket Descriptor */
  struct sockaddr_in servAddr;      /* Server Address */

  if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
    cout<< "Error: socket() failed"<<endl;
    return 0;
  }

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = inet_addr(servIP);
  servAddr.sin_port = htons(servPort);

  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
    cout<< "Connection to server failed"<<endl;
    return -1;
  }

  return sock;

  /******************************************************************/

}

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void readXBytes(int socket, unsigned int x, void* buffer)
{
    unsigned int bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = recv(socket, (uint32_t*) buffer + bytesRead, x - bytesRead, 0);
        if(result == 0)
        {
            cout<<"Server Disconnected"<<endl;
            result = 1;
            pthread_exit(&result);
        }
        if (result < 0 )
        {
            cout<<"Error: read() failed"<<endl;
            result = 1;
            pthread_exit(&result);
            return;
        }

        bytesRead += result;
    }
}

string login(int sock)
{

  /* LOGIN */
  string username, pwd, auth;
  cout << "Enter your registered username: ";
  getline(cin,username);
  cout << "Enter the password: ";
  pwd = getMaskedInput();

  /* PROTOCOL: AUTHENTICATE <USERNAME> <PASSWORD> */
  auth = "AUTHENTICATE " + username + " " + pwd;
  sendDataToServer(auth, sock);

  /* READ RESPONSE */

  unsigned int length = 0;
  char* buffer = 0;

  readXBytes(sock, sizeof(length), (void*)(&length));
  length = ntohl(length);
  buffer = new char[length];
  readXBytes(sock, length, (void*)buffer);

  return (string)buffer;

}

string regist(int sock)
{

  /* REGISTER */
  string username = "", pwd1 = "", pwd2 = "", auth = "", email = "";
  while(email == ""){
    cout << "Enter your email address: ";
    getline(cin, email);
  }

  while(username == ""){
    cout << "Enter your desired username: ";
    getline(cin,username);
  }

  while(pwd1 == ""){
    cout << "Enter the password: ";
    pwd1 = getMaskedInput();
  }

  while(pwd2 != pwd1){
    cout << "Enter the password again: ";
    pwd2 = getMaskedInput();
  }

  /* PROTOCOL: AUTHENTICATE <USERNAME> <PASSWORD> */
  auth = "REGISTER " + email + " " + username + " " + pwd1;

  sendDataToServer(auth, sock);  

  /* READ RESPONSE */
  unsigned int length = 0;
  char* buffer = 0;

  readXBytes(sock, sizeof(length), (void*)(&length));
  length = ntohl(length);
  buffer = new char[length];
  readXBytes(sock, length, (void*)buffer);
  buffer[length] = '\0';

  return (string)buffer;

}

void sendDataToServer(string str, int sock){

  unsigned int length = htonl(str.length());

  if(send(sock, &length, sizeof(length), 0) != sizeof(length)){
    cout<<"Error: Sent different number of bytes than expected"<<endl;
    return;
  }

  int size = str.size();
  while(size > 0){
      size_t written = send(sock, str.c_str(), size, 0);
      if(written == -1)
            return;
       size -= written;
  }

  return;
}