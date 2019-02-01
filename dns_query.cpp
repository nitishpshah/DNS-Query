/*
	TODO: make function for preparing socket structures
	TODO: make function for query
*/
#include <iostream>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
using namespace std;

int string_to_integer(string s);
string host_to_qname(string s);
string integer_to_string(int s);
string num_to_bin(int x);
string read_name_at_index(char* buffer, int &index);

struct dns_header
{
	//total size: 12 bytes

	//2
	unsigned short id; // identification number random //2

	//1
	unsigned char rd : 1;     // recursion desired
	unsigned char tc : 1;     // truncated message
	unsigned char aa : 1;     // authoritive answer
	unsigned char opcode : 4; // purpose of message 	0: standard

	//1
	unsigned char qr : 1;     // query/response flag
	unsigned char rcode : 4;  // response code 		0: no error
	unsigned char cd : 1;     // checking disabled
	unsigned char ad : 1;     // authenticated data
	unsigned char z : 1;      // its z! reservedunsigned char ra :1; // recursion available
	unsigned char ra : 1;     // recursion available

	//8
	unsigned short q_count;    // number of question entries
	unsigned short ans_count;  // number of answer entries
	unsigned short auth_count; // number of authority entries
	unsigned short add_count;  // number of resource entries
};


struct question
{
	//4 bytes
	unsigned short qtype;  // 1:A, 2:authoritative NS, 5:CNAME
	unsigned short qclass; // IN: 1 for internet
};

//Constant sized fields of the resource record structure
//pragma pack(push, 1) tightly packs the structure
//i.e. it will now assign 10 bytes to a structure of type quesion 
//instead of 12 bytes

#pragma pack(push, 1)
struct rr_data
{
	//10 bytes
	unsigned short type;
	unsigned short _class;
	unsigned int ttl;
	unsigned short data_len;
};
#pragma pack(pop)

int main(int argc, char *argv[]){
	if (argc != 3){
		cout << "use as: ./dns server_ip port" << endl;
		return -1;
	}

	string str_port = argv[2];
	int PORT = string_to_integer(str_port), sockfd;

	//DNS server data -----------------------------------------------
	//prepare structure for socket data 
	struct sockaddr_in server_addr, my_addr;
	server_addr.sin_family = AF_INET;   //address family
	server_addr.sin_port = htons(PORT); //host to network short
	//inet_addr: converts a string containing an IPv4 dotted-decimal address into a proper address for the IN_ADDR structure.
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	//set the rest 8 its to zero
	//signature: void * memset ( void * ptr, int value, size_t num );
	memset(&(server_addr.sin_zero), '\0', 8);
	//--------------------------------------------------------

	socklen_t slen = sizeof(server_addr), *sin_size; //for use in binding

	//own data -----------------------------------------------
	sockaddr *their_addr;
	my_addr.sin_family = AF_INET;   //address family
	my_addr.sin_port = htons(5000); //host to network short
	//inet_addr: converts a string containing an IPv4 dotted-decimal address into a proper address for the IN_ADDR structure.
	my_addr.sin_addr.s_addr = INADDR_ANY;
	//set the rest 8 its to zero
	memset(&(server_addr.sin_zero), '\0', 8);
	//--------------------------------------------------------

	//make socket and bind
	sockfd = socket(AF_INET, SOCK_DGRAM, 0); //UDP
	if (sockfd == -1){
		cout << "socket making error" << endl;
		return -1;
	}
	int b = bind(sockfd, (sockaddr *)&my_addr, slen);
	//cout<<"bind return value: "<<b<<endl;
	if (b < 0){
		cout << "couldnt bind: " << endl;
		return -1;
	}
	//--------------------------------------------------------

	const int MAXDATASIZE = 1024;

	//structure sizes
	int header_size = sizeof(dns_header), ques_head_size = sizeof(question);
	//header size = 12; question header size = 10

	//----total size gives the total size of the question formed till that line
	int total_size = 0;
	string hostname1;

	//character arrays for making the question and recieving the answers
	char buff[MAXDATASIZE];
	char answer[MAXDATASIZE];
	memset(&buff[0], '\0', MAXDATASIZE);
	memset(&answer[0], '\0', MAXDATASIZE);

	//--------------------------------------------------------
	//take input
	cout << "Enter host name: ";
	getline(cin, hostname1);
	int len = hostname1.length();
	//--------------------------------------------------------
	//only proceed if some iput given
	if (len > 0){
		//--------------------------------------------------------
		//cast the DNS header structure on the buffer and start filling it up
		dns_header *head = (dns_header *)&buff[0];
		srand(time(NULL));
		unsigned short id = rand() % 65536;
		head->id = htons(id); // identification number random
		head->rd = 1;	// recursion desired
		head->tc = 0;	// truncated message
		head->z = 0;
		head->ad = 0;
		head->cd = 0;
		head->opcode = 0;	 // purpose of message 	0: standard
		head->aa = 0;		  // authoritive answer
		head->qr = 0;		  // query/response flag
		head->rcode = 0;	  // response code 		0: no error
		head->ra = 0;		  // recursion available
		head->q_count = htons(1); // number of question entries
		head->ans_count = 0;      // number of answer entries
		head->auth_count = 0;     // number of authority entries
		head->add_count = 0;      // number of resource entries

		total_size = header_size;

		//--------------------------------------------------------
		//convert to DNS question form
		string hostname = host_to_qname(hostname1);
		//and add to the buffer
		for (int i = 0; i < hostname.length(); ++i){
			buff[total_size + i] = hostname[i];
		}

		total_size += hostname.length();

		//--------------------------------------------------------
		//add question length to the total size
		//cast the question structure on the buffer 
		question *ques = (question *)&buff[total_size];
		ques->qtype = htons(1);
		ques->qclass = htons(1);

		total_size += ques_head_size;
		//--------------------------------------------------------
		int sen = sendto(sockfd, &buff[0], total_size, 0, (sockaddr *)&server_addr, slen);
		int nbytes = recvfrom(sockfd, &answer[0], MAXDATASIZE - 1, 0, (sockaddr *)&their_addr, &slen);
		//--------------------------------------------------------
		cout<<"total bytes sent: "<<sen<<"\ntotal bytes recieved: "<<nbytes<<endl;
		//--------------------------------------------------------
		//read answer headers
		//do not continue if errors in the answer
		dns_header *ans = (dns_header *)&answer[0];
		cout<<endl;
		if (id != ntohs(ans->id)){
			cout<<"the ids don't match"<<endl;
		}
		if ((short)ans->tc == 1){
			cout<<"the answer was truncated"<<endl;
		}
		if ((short)ans->aa == 0){
			cout<<"the answer is non-authoritative"<<endl;
		}
		else if ((short)ans->aa == 1){
			cout<<"the answer is authoritative"<<endl;
		}
		if ((short)ans->ra == 0){
			cout<<"recursion not available"<<endl;
			if ((short)ans->rd == 1){
				cout<<"recursion was desired"<<endl;
			}
		}
		if ((short)ans->rcode == 0){
			cout<<"the query was answered without errors"<<endl;
		}
		else if ((short)ans->rcode == 1){
			cout << "format error, could not be read by server" << endl;
			return 0;
		}
		else if ((short)ans->rcode == 2){
			cout << "server failure" << endl;
			return 0;
		}
		else if ((short)ans->rcode == 3){
			cout << "Name queries doesn't exist in the domain " << endl;
			return 0;
		}
		else if ((short)ans->rcode == 4){
			cout << "Query type is not supported by server" << endl;
			return 0;
		}
		else if ((short)ans->rcode == 5){
			cout << "refused" << endl;
			return 0;
		}
		else{
			cout<<"unknown error"<<endl;
			return 0;
		}
		//--------------------------------------------------------
		//stats
		cout<<endl;
		cout << "total question(s) asked:   " << ntohs(ans->q_count) << endl;
		cout << "total answer(s):  " << ntohs(ans->ans_count) << endl;
		cout << "authoritative answers(s): " << ntohs(ans->auth_count) << endl;
		cout << "additional answer(s): " << ntohs(ans->add_count) << endl <<endl;

		//--------------------------------------------------------
		//parse RRs
		//currently can only work with type CNAME, NS and type A
		int cursor = total_size;

		while (cursor < nbytes){
			cout<<"answer to query: ";
			//read pointer
			cout<<read_name_at_index(answer,cursor)<<endl;		//cursor += 2;
			//--------------------------------------------------------
			//cast
			rr_data* rr = (rr_data* )&answer[cursor];
			cursor += sizeof(rr_data);
			//--------------------------------------------------------
			if (ntohs(rr->type ) == 1){
				cout<<"Type A IP Address: ";
				for (size_t i = 0; i < ntohs(rr->data_len); i++){
					if(i != 0){
						cout<<".";
					}
					//
					cout << (unsigned short)(unsigned char)answer[cursor+ i];
				}
				cout<<endl;
			cursor += ntohs(rr->data_len);
			} 
			//--------------------------------------------------------
			else if (ntohs(rr->type) == 5){
				cout<<"CNAME: ";
				//the function increments the index appropriately
				cout<<read_name_at_index(answer,cursor);
				cout<<endl;
			}
			else if (ntohs(rr->type) == 2)
			{
				cout << "type NS: ";
				//the function increments the index appropriately
				cout << read_name_at_index(answer,cursor);
				cout << endl;
			}
			//--------------------------------------------------------
			else{
				cout << "unknown type " << ntohs(rr->type) << endl;
			cursor += ntohs(rr->data_len);
			}
			cout<<endl;
		}
	}

	close(sockfd);
	cout << endl << "connection closed" << endl;
}

int is_pointer(char* buff, int index){
	//returns offset from the answer start (pointer to the data)
	
	//cast to a short
	unsigned short *pointer_info = (unsigned short *)&buff[index];
	//then reverse order
	unsigned int point = ntohs(*pointer_info);
	// directly casting on an integer doesnt work
	// if it is a pointer, first 2 bits will be ones
	// point = 11-- ---- ---- ----
	
	unsigned int x = 3;
	x = x << 14;
	//x = 1100 0000 0000 0000
	if ((x & point) == x){
		//if the first two bits of the point are ones
		//the rest of the value is the offset
		//i.e. if point = 1100 0000 0000 0110
		// the the offset = 110 = 6
		// 0000000000110 = 110000000000 xor 1100000000110
		//i.e. x xor point
		return (x ^ point);
	}
	else{
		return 0;
	}
}

string read_name_at_index(char* buffer, int &index){
	//increments index appropriately

	//name regex: (x -----)*(0 + (pointer to another name))
	//no pointers at the start, if pointer at end, break after reading at the pointer
	string ret = "";
	while (true)
	{
		/*
			breaks when it encounters a pointer or a zero
			so the pointers can only be at the end (beginning can be empty)
			so the form of the qname is (x--)*(0 + pointer)
		*/

		//buffer[index] is now a zero, a number or a pointer

		if (buffer[index] != 0){
			int offset = is_pointer(buffer, index);
			if(offset>0){
				//if it is a pointer
				ret += read_name_at_index(buffer, offset);
				//pointer size is always 2 bytes
				index += 2;
				break;
			}else{
				//it isnt a pointer
				//read l, l characters follow
				//i.e. reads 5abcde
				int l = (short)buffer[index]; //number of characters
				for (int i = 1; i <= l; i++){
					ret += (char)buffer[index + i];
				}
				ret += ".";
				index += l + 1;
				//increment index appropriately
			}
		}else{
			//the last call to this function will always break here
			index += 1;
			break;
		}
	}
	return ret;
}

string host_to_qname(string s){
	//converts www.amazon.in to 3www6amazon2in
	string ret;
	int count = 0;
	for (int i = s.length() - 1; i >= 0; i--){
		if (s[i] == '.'){
			ret = (char)count + ret;
			count = 0;
		}
		else{
			count += 1;
			ret = s[i] + ret;
		}
	}
	ret = (char)count + ret;
	ret += (char)0;
	return ret;
}

//SE
int string_to_integer(string s){
	int res = 0;
	int pow = 10;
	for (int i = 0; i < s.length(); ++i){
		if (s[i] == '0'){
			res = res * pow + 0;
		}
		else if (s[i] == '1'){
			res = res * pow + 1;
		}
		else if (s[i] == '2'){
			res = res * pow + 2;
		}
		else if (s[i] == '3'){
			res = res * pow + 3;
		}
		else if (s[i] == '4'){
			res = res * pow + 4;
		}
		else if (s[i] == '5'){
			res = res * pow + 5;
		}
		else if (s[i] == '6'){
			res = res * pow + 6;
		}
		else if (s[i] == '7'){
			res = res * pow + 7;
		}
		else if (s[i] == '8'){
			res = res * pow + 8;
		}
		else if (s[i] == '9'){
			res = res * pow + 9;
		}
	}
	return res;
}

string integer_to_string(int s){
	string ret;
	while (s > 0){
		int x = s % 10;
		if (x == 0){
			ret += "0";
		}
		else if (x == 1){
			ret += "1";
		}
		else if (x == 2){
			ret += "2";
		}
		else if (x == 3){
			ret += "3";
		}
		else if (x == 4){
			ret += "4";
		}
		else if (x == 5){
			ret += "5";
		}
		else if (x == 6){
			ret += "6";
		}
		else if (x == 7){
			ret += "7";
		}
		else if (x == 8){
			ret += "8";
		}
		else if (x == 9){
			ret += "9";
		}
		s = s / 10;
	}
	return ret;
}