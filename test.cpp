#include <iostream>
#include<sstream>
//Header Files
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<stdlib.h>    //malloc
#include<sys/socket.h>    //you know what this is for
#include<arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>    //getpid

using namespace std;
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};


//SE
int string_to_integer(string s)
{
	int res = 0;
	int pow = 10;
	for (int i = 0; i < s.length(); ++i)
	{
		if(s[i]=='0'){res = res*pow + 0;}
		else if(s[i]=='1'){res = res*pow+1;}
		else if(s[i]=='2'){res = res*pow+2;}
		else if(s[i]=='3'){res = res*pow+3;}
		else if(s[i]=='4'){res = res*pow+4;}
		else if(s[i]=='5'){res = res*pow+5;}
		else if(s[i]=='6'){res = res*pow+6;}
		else if(s[i]=='7'){res = res*pow+7;}
		else if(s[i]=='8'){res = res*pow+8;}
		else if(s[i]=='9'){res = res*pow+9;}
	}
	return res;
}

string integer_to_string(int s){
	string ret;
	while(s > 0){
		int x = s%10;
		if(x == 0){ ret += "0";}
		else if(x == 1){ ret += "1";}
		else if(x == 2){ ret += "2";}
		else if(x == 3){ ret += "3";}
		else if(x == 4){ ret += "4";}
		else if(x == 5){ ret += "5";}
		else if(x == 6){ ret += "6";}
		else if(x == 7){ ret += "7";}
		else if(x == 8){ ret += "8";}
		else if(x == 9){ ret += "9";}
		s = s/10;
	}
	return ret;
}


string host_to_qname(string s){
	string ret;
	int count = 0;
	for (int i = s.length()-1 ; i>=0 ; i--)
	{
		if(s[i] == '.'){
			ret = integer_to_string(count) + ret;
			count = 0;
		}else{
			count+=1;
			ret = s[i] + ret;
		}
	}
	ret = integer_to_string(count) + ret;
	return ret;

}
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host) 
{
    int lock = 0 , i;
    strcat((char*)host,".");
     
    for(i = 0 ; i < strlen((char*)host) ; i++) 
    {
        if(host[i]=='.') 
        {
            *dns++ = i-lock;
            for(;lock<i;lock++) 
            {
                *dns++=host[lock];
            }
            lock++; //or lock=i+1;
        }
    }
    *dns++='\0';
}

int main(int argc, char const *argv[])
{
	unsigned char host[]="www.google.com", q[64];
	ChangetoDnsNameFormat(&q[0], &host[0]);
	cout<<q<<endl;
	for (int i = 0; i < 64; ++i)
	{
		cout<<(short)q[i]<<" "<<q[i]<<endl;;
	}cout<<endl;
	
}
