#include "utils.h"

pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
unordered_map <string, int> clientMap;

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

string formatted_time(){

	time_t t = time(0);
	struct tm * now = localtime( & t );
	string time = to_string((now->tm_year + 1900)) + "-";
	if((now->tm_mon + 1)<10) time += "0" + to_string((now->tm_mon + 1)) + "-";
	else time+=to_string((now->tm_mon + 1)) + "-";

	if(now->tm_mday<10) time += "0" + to_string(now->tm_mday) + " ";
	else time+=to_string(now->tm_mday) + " ";

	if(now->tm_hour<10) time += "0" + to_string((now->tm_hour)) + ":";
	else time+=to_string((now->tm_hour)) + ":";

	if(now->tm_min<10) time += "0" + to_string((now->tm_min));
	else time+=to_string((now->tm_min));

	return time;
}

void *threadHandler(void* threadargs)
{
	int clientSock;

	pthread_detach(pthread_self());

	clientSock = ((struct ThreadArgs *)threadargs)->clientSock;
	free(threadargs);

	handleTCPClient (clientSock);

	return (NULL);
}

void readXBytes(int socket, unsigned int x, void* buffer)
{
    unsigned int bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        int ret;
        if(result == 0)
        {
            cout<<"Client Disconnected"<<endl;
            ret = 0;
            pthread_exit(&ret);
        }
        if (result < 0 )
        {
            cout<<"Error: read() failed"<<endl;
            ret = 1;
            pthread_exit(&ret);
        }

        bytesRead += result;
    }

}

void sendDataToClient(string str, int sock){

  unsigned int length = htonl(str.size());

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

void handleTCPClient(int clientSocket){
	
  /* READ COMMANDS */

  unsigned int length = 0;
  char* buffer = 0;
  Json::Value users;
  Json::Reader reader;

  while(true){

  readXBytes(clientSocket, sizeof(length), (void*)(&length));
  length = ntohl(length);
  buffer = new char[length];
  memset(buffer,0,length);
  readXBytes(clientSocket, length, (void*)buffer);
  buffer[length] = '\0';

  pthread_mutex_lock(&mymutex);

  std::ifstream privatedb("private.json");
		bool parsingSuccessful = reader.parse(privatedb, users, false);
		if(!parsingSuccessful) {
		  	cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
		  	cout<<reader.getFormattedErrorMessages()<<endl;
		  	return;
		}
		
  privatedb.close();

  pthread_mutex_unlock(&mymutex);  

  vector<string> v = split(buffer,' ');

  if(v[0] == "AUTHENTICATE")
  {
  	if(v.size()==3){

		string id = "0";

		for(Json::Value::iterator it = users.begin(); it!=users.end(); it++){
			if((*it)["name"].asString() == v[1]){
				if((*it)["password"].asString() == v[2]){
					id = it.key().asString();
					break;
				}
			}
	    }

		if(id!="0"){
			users[id]["online"]="true";
		
  pthread_mutex_lock(&mymutex);  
        
			Json::StyledStreamWriter writer;
			std::ofstream update("private.json");
			writer.write(update,users);
			update.close();

  pthread_mutex_unlock(&mymutex);  
            
		}

		sendDataToClient(id, clientSocket);
  	}
  }

  if(v[0] == "REGISTER"){
        string email = v[1];
        string username = v[2];
        string pwd = v[3];
        string id = "-1";

        for(Json::Value::iterator it = users.begin(); it!=users.end(); it++){
			if((*it)["name"].asString() == v[2] || (*it)["email"].asString() == v[1]){
                id == "0";
			}
	    }

        if( id == "0"){
            sendDataToClient(id, clientSocket);
        }
        else{
            int num = users.size();
            id = to_string(num+1);
            Json::Value newuser;
            newuser["email"] = email;
            newuser["join_date"] = split(formatted_time(),' ')[0];
            newuser["last_seen"] = formatted_time();
            newuser["name"] = username;
            newuser["online"] = "true";
            newuser["password"] = pwd;
            users[id] = newuser;

  pthread_mutex_lock(&mymutex);  

            std::ofstream update("private.json");
	        Json::StyledStreamWriter writer;
	        writer.write(update,users);
	        update.close();

  pthread_mutex_unlock(&mymutex);  

            sendDataToClient(id, clientSocket);
            
        }
        
  }

  if(v[0] == "SENDFILE"){
      string id2 = v[1];
      string id = v[3];
      string filename = "cache/" + v[2];

      readXBytes(clientSocket, sizeof(length), (void*)(&length));
      length = ntohl(length);
      buffer = new char[length];
      memset(buffer,0,length);
      readXBytes(clientSocket, length, (void*)buffer);
      buffer[length] = '\0';

      std::ofstream outfile (filename,std::ofstream::binary);
      outfile.write(buffer,length);
      outfile.close();
      delete[] buffer;

  }

  if(v[0] == "ONLINE"){
  		string id = v[1];
  		string msg;
		for(Json::Value::iterator it = users[id]["friends"].begin(); it!=users[id]["friends"].end(); it++){
		  	string id1 = (*it)["id"].asString();
		  	if(users[id1]["online"]=="true") msg += users[id1]["name"].asString() + "\n";
		}

		sendDataToClient(msg, clientSocket);
  }

  if(v[0] == "FRIENDS"){
  		string id = v[1];
  		string msg = "";
  		
		for(Json::Value::iterator it = users[id]["friends"].begin(); it!=users[id]["friends"].end(); it++){
		  	msg += users[(*it)["id"].asString()]["name"].asString() + "\n";
		}

		sendDataToClient(msg, clientSocket);
  }

  if(v[0] == "LIST"){
      string msg = "";

      for(Json::Value::iterator it = users.begin(); it!=users.end(); it++){
		  	msg += (*it)["name"].asString() + "\n";
	  }

      sendDataToClient(msg, clientSocket);
  }

  if(v[0] == "REQUESTS"){
      string id = v[1];
      string msg = "";
      if(users[id]["requests"].isNull()){
          msg = "No pending friend requests\n";
          sendDataToClient(msg, clientSocket);
          continue;
      }
      for(Json::Value::iterator it = users[id]["requests"].begin(); it!=users[id]["requests"].end(); it++){
		  	msg = msg + "Request from: \033[1;35m" + users[(*it)["id"].asString()]["name"].asString() + "\033[0m\n";
	  }

      sendDataToClient(msg, clientSocket);
  }

  if(v[0] == "ADD"){
        string id = v[2];
        string id1 = "0";

        for(Json::Value::iterator it = users.begin(); it!=users.end(); it++){
			if((*it)["name"].asString() == v[1]){
                id1 = it.key().asString();
                if(id1==id){
                    id1 = "0";
                }
			}
	    }

        if(id1 != "0"){

            bool flag1 = 1;
            for(Json::Value::iterator it = users[id]["friends"].begin(); it != users[id]["friends"].end(); it++ ){
                if((*it)["id"].asString() == id1){
                    flag1 = 0;
                    sendDataToClient("-2", clientSocket);
                }
            }

            if(flag1){
                    bool flag = 0;

                    if(!users[id]["requests"].isNull())
                    for(Json::Value::iterator it = users[id]["requests"].begin(); it!=users[id]["requests"].end(); it++){
                        if(id1 == (*it)["id"].asString()) { flag = 1; break;}
	                }
                
                    if(flag){
                        Json::Value arr1(Json::arrayValue);
                        Json::Value temp;
                    
                        for(Json::Value::iterator it = users[id]["requests"].begin(); it!=users[id]["requests"].end(); it++){
                            temp["id"] = (*it)["id"].asString();
                            if((*it)["id"].asString() != id1){
                                arr1.append(temp);
                            }
	                    }
                    
                        temp["id"] = id1;
                        users[id]["requests"] = arr1;    
                        if(!users[id]["friends"].isNull()){
                            users[id]["friends"].append(temp);
                        }

                        else{
                            arr1.clear();
                            arr1.append(temp);
                            users[id]["friends"] = arr1;
                        }

                        temp["id"] = id;
                        if(!users[id1]["friends"].isNull()){
                            users[id1]["friends"].append(temp);
                        }

                        else{
                            arr1.clear();
                            arr1.append(temp);
                            users[id1]["friends"] = arr1;
                        }

                    
                pthread_mutex_lock(&mymutex);              

                        std::ofstream update("private.json");
	                    Json::StyledStreamWriter writer;
	                    writer.write(update,users);
	                    update.close();
                    
                pthread_mutex_unlock(&mymutex);              

                        sendDataToClient("-1", clientSocket);
                    }
                
                    else{
                    
                    Json::Value arr(Json::arrayValue);
                    Json::Value req;
                    Json::Value elements;
                
                    req["id"] = id;
                    arr.append(req);
                
                    elements = users[id]["requests"];
                    if(elements.isNull()){
                        users[id1]["requests"] = arr;
                    } 
                    else{
                        users[id1]["requests"].append(req);
                    }
                
            pthread_mutex_lock(&mymutex);  

                    std::ofstream update("private.json");
	                Json::StyledStreamWriter writer;
	                writer.write(update,users);
	                update.close();
                
            pthread_mutex_unlock(&mymutex);          

                    sendDataToClient(id1, clientSocket);
                
                    }
            } 
        
        }

        else {
            sendDataToClient(id1,clientSocket);
        }

        
  }

  if(v[0] == "LAST_SEEN_ALL"){
  		string id = v[1];
  		string msg;

		for(Json::Value::iterator it = users[id]["friends"].begin(); it!=users[id]["friends"].end(); it++){
		    string id1 = (*it)["id"].asString();
		    if(users[id1]["online"]=="true")  msg += users[id1]["name"].asString() + ": \033[1;32mActive now\033[0m\n";
		    else msg += users[id1]["name"].asString() + ": \033[1;32mLast seen at \033[0m" + users[id1]["last_seen"].asString() + "\n";
		}

		sendDataToClient(msg, clientSocket);
  }

  if(v[0] == "LAST_SEEN"){
  		string id = v[2];
  		string msg;
		bool flag=1;
		for(Json::Value::iterator it = users[id]["friends"].begin(); it!=users[id]["friends"].end(); it++){
		  	string id1 = (*it)["id"].asString();
		  	if(users[id1]["online"]=="true" && users[id1]["name"]==v[1]) {msg += v[1] + ": \033[1;32mActive now\033[0m\n";flag=0;}
		  	else if(users[id1]["name"]==v[1]) {msg += v[1] + ": \033[1;32mLast seen at \033[0m" + users[id1]["last_seen"].asString() + "\n";flag=0;}
		}
		
		if(flag) msg += "\033[1;31mHe/She is not your friend\033[0m\n";

		sendDataToClient(msg, clientSocket);
  }

  if(v[0] == "LISTGROUPS"){
        string id = v[1];

        pthread_mutex_lock(&mymutex);

        Json::Value groupjson;
        std::ifstream groups("groups.json");
        bool parsingSuccessful = reader.parse(groups, groupjson, false);
        if(!parsingSuccessful) {
          cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
          cout<<reader.getFormattedErrorMessages()<<endl;
          return;
        }
      
        groups.close();

        pthread_mutex_unlock(&mymutex);

        string msg = "";
        for(Json::Value::iterator it = groupjson.begin(); it != groupjson.end(); it++){
            for(Json::Value::iterator it1 = (*it).begin(); it1 != (*it).end(); it1++){
                if((*it1)["id"].asString() == id){
                    msg += it.key().asString() + "\n";
                    break;
                }
            }
        }

        sendDataToClient(msg, clientSocket);

  }

  if(v[0] == "CHAT"){

  		string id = v[2];
  		string id2 = "0";
		  
        for(Json::Value::iterator it = users[id]["friends"].begin(); it!=users[id]["friends"].end(); it++){
		  	string id1 = (*it)["id"].asString();
		  	if(users[id1]["name"]==v[1]) id2 = id1;
        }
		
        if(id2 == "0"){

            pthread_mutex_lock(&mymutex);

            Json::Value groupjson;
            std::ifstream groups("groups.json");
            bool parsingSuccessful = reader.parse(groups, groupjson, false);
            if(!parsingSuccessful) {
              cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
              cout<<reader.getFormattedErrorMessages()<<endl;
              return;
            }
        
            groups.close();
        
            pthread_mutex_unlock(&mymutex);

            if(!groupjson[v[1]].isNull()){
                id2 = v[1];
            }

        }
		sendDataToClient(id2, clientSocket);
  }

  if(v[0] == "SEND"){

  	  string msg = "";
      unsigned int i;
      for(i=1; i<v.size(); i++){
        if(v[i] == "TO") break;
        msg = msg + v[i] + " ";
      }

      string id = v[i+3];
      string id2 = v[i+1];

      if(isInteger(id2)){

                if(users[id2]["online"]!="true"){
                                
                    pthread_mutex_lock(&mymutex);  

                    Json::Value msgjson;                    
                    std::ifstream messages("messages.json");
                    bool parsingSuccessful = reader.parse(messages, msgjson, false);
                    if(!parsingSuccessful) {
                      cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
                      cout<<reader.getFormattedErrorMessages()<<endl;
                      return;
                    }
                    messages.close();
                
                    pthread_mutex_unlock(&mymutex);  

                    Json::Value msgformat;
                    Json::Value elements;
                    Json::Value arr(Json::arrayValue);
                    msgformat["message"] = msg;
                    msgformat["time"] = (formatted_time());
                    arr.append(msgformat);

                    elements = msgjson[id][id2];
                    if(elements.isNull()){
                      msgjson[id2][id] = arr;
                    }
                    else{
                      msgjson[id2][id].append(msgformat);
                    }
                
                    pthread_mutex_lock(&mymutex);  

                    std::ofstream update("messages.json");
                    Json::StyledStreamWriter writer;
                    writer.write(update,msgjson);
                    update.close();
                    
                    pthread_mutex_unlock(&mymutex);  

                    }
                
                else{
                  	string data = "\033[1;96m" + users[id]["name"].asString() +":\033[0m " + msg + "\n";
                  	sendDataToClient(data, clientMap[id2]);
                }
      }

      else{

            pthread_mutex_lock(&mymutex);

            Json::Value groupjson;
            std::ifstream groups("groups.json");
            bool parsingSuccessful = reader.parse(groups, groupjson, false);
            if(!parsingSuccessful) {
              cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
              cout<<reader.getFormattedErrorMessages()<<endl;
              return;
            }
        
            groups.close();
        
            pthread_mutex_unlock(&mymutex);

            for(Json::Value::iterator it = groupjson[id2].begin(); it != groupjson[id2].end(); it++){
                string id3 = (*it)["id"].asString();
                if(id3 == id){
                    continue;
                }
                if(users[id3]["online"]!="true"){
                
                    Json::Value msgjson;
                
                    pthread_mutex_lock(&mymutex);  

                    std::ifstream messages("messages.json");
                    bool parsingSuccessful = reader.parse(messages, msgjson, false);
                    if(!parsingSuccessful) {
                      cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
                      cout<<reader.getFormattedErrorMessages()<<endl;
                      return;
                    }
                    messages.close();
                
                    pthread_mutex_unlock(&mymutex);  

                    Json::Value msgformat;
                    Json::Value elements;
                    Json::Value arr(Json::arrayValue);
                    msgformat["message"] = msg;
                    msgformat["time"] = (formatted_time());
                    arr.append(msgformat);

                    elements = msgjson[id][id3];
                    if(elements.isNull()){
                      msgjson[id3][id] = arr;
                    }
                    else{
                      msgjson[id3][id].append(msgformat);
                    }
                
                    pthread_mutex_lock(&mymutex);  

                    std::ofstream update("messages.json");
                    Json::StyledStreamWriter writer;
                    writer.write(update,msgjson);
                    update.close();
                    
                    pthread_mutex_unlock(&mymutex);  

                    }
                
                else{
                  	string data = "\033[1;96m" + id2 + "| " + users[id]["name"].asString() +":\033[0m " + msg + "\n";
                  	sendDataToClient(data, clientMap[id3]);
                }
            }
      }
  }

  if(v[0] == "MAP"){
      string id = v[1];
      clientMap[id] = clientSocket;
  }

  if(v[0] == "PENDING"){
      Json::Value msgjson;
      string msg = "\n";
      string id = v[1];

      std::ifstream messages("messages.json");
      bool parsingSuccessful = reader.parse(messages, msgjson, false);
      if(!parsingSuccessful) {
        cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
        cout<<reader.getFormattedErrorMessages()<<endl;
        return;
      }
    
      messages.close();

      for(Json::Value::iterator it = msgjson[id].begin(); it!=msgjson[id].end(); it++){
        for(Json::Value::iterator it1 = msgjson[id][it.key().asString()].begin(); it1!=msgjson[id][it.key().asString()].end(); it1++)
          msg = users[it.key().asString()]["name"].asString() + ": " +(*it1)["message"].asString() + " at " + (*it1)["time"].asString() + "\n"; 
      }
      sendDataToClient(msg, clientMap[v[1]]);

      msgjson[id].clear();

  pthread_mutex_lock(&mymutex);  
      
      std::ofstream update("messages.json");
      Json::StyledStreamWriter writer;
      writer.write(update,msgjson);
      update.close();

  pthread_mutex_unlock(&mymutex);  
      
  }

  if(v[0] == "GROUP"){
      string id = v[2];
      string groupname = v[1];
      int n = v.size();

      Json::Value groupjson(Json::arrayValue);

      pthread_mutex_lock(&mymutex);

      std::ifstream groups("groups.json");
      bool parsingSuccessful = reader.parse(groups, groupjson, false);
      if(!parsingSuccessful) {
        cout<<"\033[1;31mError Parsing Json File\033[0m\n\n";
        cout<<reader.getFormattedErrorMessages()<<endl;
        return;
      }
    
      groups.close();

      pthread_mutex_unlock(&mymutex);

      if(groupjson[groupname].isNull()){
          Json::Value arr(Json::arrayValue);
          Json::Value temp;
          for(int i=3; i<n; i++){
              for(Json::Value::iterator it = users[id]["friends"].begin(); it!=users[id]["friends"].end(); it++){
		  	    string id1 = (*it)["id"].asString();
		  	    if(users[id1]["name"]==v[i]){
                    temp["id"] = id1;
                    arr.append(temp);
                  }
              }
              temp["id"] = id;
              arr.append(temp);
          }
          groupjson[groupname] = arr;

        pthread_mutex_lock(&mymutex);  
          
              std::ofstream update("groups.json");
              Json::StyledStreamWriter writer;
              writer.write(update,groupjson);
              update.close();

        pthread_mutex_unlock(&mymutex);  

          sendDataToClient("1", clientSocket);
      }

      else{
          sendDataToClient("0", clientSocket);
      }

  }

  if(v[0] == "EXIT"){
  	  string id = v[1];
  	  string time = formatted_time();
	  users[id]["last_seen"]=time;
	  users[id]["online"]="false";

  pthread_mutex_lock(&mymutex);  
      
	  std::ofstream update("private.json");
	  Json::StyledStreamWriter writer;
	  writer.write(update,users);
	  update.close();

  pthread_mutex_unlock(&mymutex);  
      
      close(clientSocket);
	  return;
  }

}

  return;

}
