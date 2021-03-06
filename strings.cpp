#include "strings.h"
#include <sstream>
#include <ctime>

char string_char(string* str, int pos){
	if(pos<0 || pos>=(int)str->length()) return 0;
	return str->at(pos);
}

void string_char_set(string* str, int pos, char c){
    if(pos<0 || pos>=(int)str->length()) return;
	str->at(pos) = c;
}

void string_delete(string*& str, int &pos){
	if(pos<0 || pos>=(int)str->length()) return;
    str->erase(pos, 1);
	pos--;
	//if(pos<0) pos=0;
}

void string_insert(string*& str, int pos, char c){
	if(pos<0 || pos>(int)str->length()) return;
    str->insert(str->begin()+pos, c);
}

void string_insert_string(string*& str, int pos, string s){
	if(pos<0 || pos>(int)str->length()) return;
    str->insert(pos, s);
}


string string_cutfromend(string s, unsigned int from_end){
    if(s.length() < from_end) from_end = s.length();
    s = s.substr(0, s.length()-from_end);
    return s;
}


string trim_spaces(string s){
    //obci�cie spacji na ko�cu
    while(s.length() > 0  && s[s.length()-1] == ' '){
        s = s.substr(0, s.length()-1);
    }
    //obci�cie spacji na pocz�tku
    while(s.length() > 0  && s[0] == ' '){
        s = s.substr(1);
    }
    return s;
}

string trim_1crlf(string s){
    //je�li zawiera tylko 1 enter na ko�cu
    if(s.length()>1){
        if(s[s.length()-1]=='\n' || s[s.length()-1]=='\r'){
            bool znaleziono = false;
            for(unsigned int i=0; i<s.length()-1; i++){
                if(s[i]==s[s.length()-1]){ //znak tego samego typu jak na ko�cu (\n lub \r)
                    znaleziono = true;
                    break;
                }
            }
            if(!znaleziono){
                s = s.substr(0, s.length()-1);
            }
        }
    }
    return s;
}

string trim_quotes(string s){
    if(s.length()>=3){
        //je�li cudzys�owy s� na poczatku i na ko�cu
        if(s[0]=='\"' && s[s.length()-1]=='\"'){
            //je�li w ca�ym stringu znajduj� si� tylko 2 cudzys�owy
            if(s.substr(1, s.length()-2).find('\"')==string::npos){
                s = s.substr(1, s.length()-2); //usu� je
            }
        }
    }
    return s;
}

bool string_begins(string s1, string s2){
    if(s1.length() < s2.length()) return false;
    for(unsigned int i=0; i<s2.length(); i++){
        if(s1.at(i) != s2.at(i)) return false;
    }
    return true;
}

char to_lowercase(char in){
	if(in>='A' && in<='Z') return in-'A'+'a';
	if(in=='�') return '�';
	if(in=='�') return '�';
	if(in=='�') return '�';
	if(in=='�') return '�';
	if(in=='�') return '�';
	if(in=='�') return '�';
	if(in=='�') return '�';
	if(in=='�') return '�';
	if(in=='�') return '�';
	return in;
}

void ss_clear(stringstream &sstream){
	sstream.str("");
	sstream.clear();
}

string get_time(){
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    stringstream ss2;
    if(timeinfo->tm_hour<10) ss2<<"0";
    ss2<<timeinfo->tm_hour<<":";
    if(timeinfo->tm_min<10) ss2<<"0";
    ss2<<timeinfo->tm_min<<":";
    if(timeinfo->tm_sec<10) ss2<<"0";
    ss2<<timeinfo->tm_sec;
    return ss2.str();
}
