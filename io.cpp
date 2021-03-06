#include "io.h"
#include "config.h"
#include "files.h"
#include "controls.h"
#include "strings.h"
#include <fstream>
#define WINVER 0x0501 //WINXP
#include <windows.h>

IO* IO::instance = NULL;

IO* IO::geti(){
    if(instance == NULL){
        instance = new IO();
    }
    return instance;
}

IO* IO::i(){
    return geti();
}

IO::IO(){
    instance = this;
    log_init = false;
    last_echo = "";
    repeated_echo = 0;
    clock_last = clock();
}


void IO::clear_log(){
    if(!Config::geti()->log_enabled) return;
    clear_file(Config::geti()->log_filename);
    //w��czenie mo�liwo�ci zapisu do loga
    log_init = true;
    log("Dziennik zdarze� uruchomiony.");
    if(log_buffer.size()>0){
        log("Wpisywanie starych zdarze� do dziennika...");
        for(unsigned int i=0; i<log_buffer.size(); i++){
            log(log_buffer.at(i)); //zapisanie starych log�w
        }
        log_buffer.clear();
    }
}

void IO::delete_log(){
    if(file_exists(Config::geti()->log_filename)){
        DeleteFile(Config::geti()->log_filename.c_str());
    }
}

void IO::log(string l){
    if(!Config::geti()->log_enabled) return;
    if(!log_init){
        log_buffer.push_back(l);
        return;
    }
    if(!file_exists(Config::geti()->log_filename)) clear_log();
    fstream plik;
    plik.open(Config::geti()->log_filename.c_str(), fstream::out|fstream::app);
    if(!plik.good()){
        plik.close();
        message_box("B��d", "B��d zapisu do pliku dziennika: "+Config::geti()->log_filename);
        return;
    }
    plik<<get_time()<<" - "<<l<<endl;
    plik.close();
}

void IO::log(int l){
    stringstream ss;
    ss<<l;
    log(ss.str());
}

void IO::log(string s, int l){
    stringstream ss;
    ss<<s<<": "<<l;
    log(ss.str());
}

void IO::error(string l, bool show_output){
    if(show_output && Controls::geti()->exists(Config::geti()->output_control)){
        echo("[B��D!] - "+l);
    }else{
        log("[B��D!] - "+l);
    }
}

void IO::critical_error(string l){
    log("[B��D KRYTYCZNY!] - "+l);
    message_box("B��d krytyczny", l);
}


void IO::echo(string s){
	//powtarzaj�ce si� komunikaty
	if(s==last_echo){
		if(repeated_echo==0) repeated_echo=1;
		repeated_echo++;
	}else{
		repeated_echo=0;
	}
	last_echo = s;
	if(repeated_echo>0){
		stringstream ss;
		ss<<s<<" ("<<repeated_echo<<")";
		s = ss.str();
	}
    //przesuni�cie starych output�w
    stringstream input, output;
    for(int i=Config::geti()->cmd_outputs_num-1; i>0; i--){
        ss_clear(input);
        ss_clear(output);
        output<<"cmd_output"<<i+1;
        input<<"cmd_output"<<i;
        Controls::geti()->set_text(output.str(), Controls::geti()->get_text(input.str()));
    }
    Controls::geti()->set_text("cmd_output1", s.c_str());
	log(s);
}

void IO::echo(int e){
    stringstream ss;
    ss<<e;
    echo(ss.str());
}


void IO::message_box(string title, string message){
    if(message.length()==0){
        message = title;
        title = "Wiadomo��";
    }
    MessageBox(NULL, message.c_str(), title.c_str(), MB_OK|MB_ICONINFORMATION);
}

int IO::message_box_yesnocancel(string title, string message){
    int answer = MessageBox(NULL, message.c_str(), title.c_str(), MB_YESNOCANCEL);
    if(answer == IDYES) return 1;
    if(answer == IDNO) return 2;
    if(answer == IDCANCEL) return 3;
    return 0;
}

int IO::message_box_yesno(string title, string message){
    int answer = MessageBox(NULL, message.c_str(), title.c_str(), MB_YESNO);
    if(answer == IDYES) return 1;
    if(answer == IDNO) return 2;
    return 0;
}


void IO::get_args(){
    get_args_from(GetCommandLine());
    stringstream ss;
	ss<<"Parametry uruchomienia ("<<args.size()<<"): ";
	for(unsigned int i=0; i<args.size(); i++){
		ss<<args.at(i);
		if(i<args.size()-1) ss<<", ";
	}
	log(ss.str());
}

void IO::get_args_from(string args_text){
    args.clear();
    //uci�cie spacji na ko�cu
    args_text = trim_spaces(args_text);
    bool cudzyslow = false;
    for(int i=0; i<(int)args_text.length(); i++){
        if(args_text[i]=='\"') cudzyslow = !cudzyslow;
        if(args_text[i]==' ' && !cudzyslow && i>0){
            args.push_back(args_text.substr(0, i));
            args_text = args_text.substr(i+1);
            i = -1;
        }
    }
    if(args_text.length()>0) args.push_back(args_text);
}

bool IO::is_arg(string parametr){
	for(unsigned int i=1; i<args.size(); i++){
		if(args.at(i)==parametr) return true;
	}
	return false;
}


void IO::clock_checkpoint(string comment){
    clock_t aktualny = clock();
    stringstream ss;
    ss<<"[Pomiar czasu] - "<<comment<<" - "<<((float)(aktualny-clock_last))*1000/CLOCKS_PER_SEC<<" ms ("<<(int)(aktualny-clock_last)<<" ticks)";
    clock_last = aktualny;
    log(ss.str());
}
