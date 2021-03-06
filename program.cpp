#include "app.h"
#include "strings.h"
#include "system.h"
#include <richedit.h>
#include <fstream>

void App::update_title(){
	stringstream ss;
	ss<<Config::geti()->program_name<<" v"<<version<<": ";
	if(Config::geti()->opened_file.length()==0){
		ss<<"[Nowy plik]";
	}else{
		ss<<Config::geti()->opened_file;
	}
	SetWindowText(main_window, ss.str().c_str());
}

void App::controls_fonts_set(){
    for(unsigned int i=0; i<Controls::geti()->controls.size(); i++){
        string fontface = Config::geti()->buttons_fontface;
        int fontsize = Config::geti()->buttons_fontsize;
        if(Controls::geti()->controls.at(i)->name == "editor"){
            fontface = Config::geti()->editor_fontface;
            fontsize = Config::geti()->editor_fontsize;
        }
        Controls::geti()->set_font(Controls::geti()->controls.at(i)->handle, fontface, fontsize);
    }
}

void App::change_font_size(int change){
	Config::geti()->editor_fontsize += change;
	if(Config::geti()->editor_fontsize<1) Config::geti()->editor_fontsize=1;
	Controls::geti()->set_font("editor", Config::geti()->editor_fontface, Config::geti()->editor_fontsize);
    stringstream ss;
	ss<<"Rozmiar czcionki: "<<Config::geti()->editor_fontsize;
	if(Config::geti()->autoscroll_scaling){
		Config::geti()->autoscroll_interval = Config::geti()->autoscroll_interval * (Config::geti()->editor_fontsize-2*change)/(Config::geti()->editor_fontsize-change);
        Controls::geti()->set_text("autoscroll_interval", Config::geti()->autoscroll_interval);
		ss<<", Interwa� autoscrolla: "<<Config::geti()->autoscroll_interval<<" ms";
	}
    refresh_text();
	IO::geti()->echo(ss.str());
}

void App::toolbar_switch(int change){
    if(change==1){
        Config::geti()->toolbar_show = true;
    }else if(change==0){
        Config::geti()->toolbar_show = false;
    }else if(change==2){
        Config::geti()->toolbar_show = !Config::geti()->toolbar_show;
    }
    if(Config::geti()->toolbar_show){
        for(unsigned int i=0; i<Controls::geti()->controls.size(); i++){
            Control* kontrolka = Controls::geti()->controls.at(i);
            if(kontrolka->name=="cmd") continue;
            if(string_begins(kontrolka->name, "cmd_output")) continue;
            if(kontrolka->name=="editor") continue;
            ShowWindow(kontrolka->handle, SW_SHOW);
        }
    }else{
        for(unsigned int i=0; i<Controls::geti()->controls.size(); i++){
            Control* kontrolka = Controls::geti()->controls.at(i);
            if(kontrolka->name=="cmd") continue;
            if(string_begins(kontrolka->name, "cmd_output")) continue;
            if(kontrolka->name=="editor") continue;
            ShowWindow(kontrolka->handle,SW_HIDE);
        }
    }
    event_resize();
}

void App::cmd_switch(int change){
    if(change==1){
        Config::geti()->cmd_show = true;
    }else if(change==0){
        Config::geti()->cmd_show = false;
    }else if(change==2){
        Config::geti()->cmd_show = !Config::geti()->cmd_show;
    }
    if(Config::geti()->cmd_show){
        IO::geti()->log("Ods�anianie konsoli...");
        ShowWindow(Controls::geti()->find("cmd"), SW_SHOW);
        for(int i=1; i<Config::geti()->cmd_outputs_num; i++){
            stringstream ss;
            ss<<"cmd_output"<<i+1;
            ShowWindow(Controls::geti()->find(ss.str()), SW_SHOW);
        }
    }else{
        IO::geti()->log("Ukrywanie konsoli...");
        ShowWindow(Controls::geti()->find("cmd"), SW_HIDE);
        for(int i=1; i<Config::geti()->cmd_outputs_num; i++){
            stringstream ss;
            ss<<"cmd_output"<<i+1;
            ShowWindow(Controls::geti()->find(ss.str()), SW_HIDE);
        }
    }
    event_resize();
}

void App::fullscreen_set(bool full){
	Config::geti()->fullscreen_on = full;
	if(Config::geti()->fullscreen_on){
        int window_style = GetWindowLong(main_window, GWL_STYLE);
        SetWindowLong(main_window, GWL_STYLE, window_style & ~(WS_CAPTION | WS_THICKFRAME));
        ShowWindow(main_window, SW_MAXIMIZE);
        //dla pewno�ci
		SetWindowPos(main_window, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
        IO::geti()->log("Fullscreen w��czony.");
	}else{
        int window_style = GetWindowLong(main_window, GWL_STYLE);
        SetWindowLong(main_window, GWL_STYLE, window_style | (WS_CAPTION | WS_THICKFRAME));
        ShowWindow(main_window, SW_RESTORE);
        IO::geti()->log("Fullscreen wy��czony.");
	}
}

void App::fullscreen_toggle(){
    fullscreen_set(!Config::geti()->fullscreen_on);
    //schowanie paska narz�dzi
    if(Config::geti()->fullscreen_on && Config::geti()->toolbar_show){
        toolbar_switch(0);
    }
}


void App::chordsbase_start(){
    IO::geti()->log("Otwieranie bazy akord�w...");
    if(Config::geti()->songs_dir.length()==0){
        IO::geti()->error("Brak zdefiniowanej bazy akord�w.");
        return;
    }
    vector<HWND>* lista_przed = windows_list(Config::geti()->explorer_classname);
    SHELLEXECUTEINFO ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = main_window;
    ShExecInfo.lpVerb = "open";
    ShExecInfo.lpFile = "explorer.exe";
    ShExecInfo.lpParameters = Config::geti()->songs_dir.c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    if(!ShellExecuteEx(&ShExecInfo)){
        IO::geti()->error("B��d otwierania bazy akord�w.");
        return;
    }
    if(!Config::geti()->halfscreen) return;
    IO::geti()->log("Szukanie okna bazy akord�w...");
    WaitForInputIdle(ShExecInfo.hProcess, 1000);
    vector<HWND>* lista_po;
    vector<HWND>* diff;
    for(int i=Config::geti()->chordsbase_max_waits; i>=0; i--){
        if(i==0){
            IO::geti()->error("Nie znaleziono okna bazy akord�w.");
            delete lista_przed;
            return;
        }
        lista_po = windows_list(Config::geti()->explorer_classname);
        diff = windows_diff(lista_przed, lista_po);
        delete lista_po;
        if(diff->size()>0) break;
        delete diff;
        IO::geti()->log("Brak okna bazy akord�w - czekam...");
        Sleep(Config::geti()->chordsbase_wait);
    }
    delete lista_przed;
    HWND okno_bazy = diff->at(0);
    delete diff;
    IO::geti()->log("Znaleziono okno bazy akord�w - rozmieszczanie na po�owie ekranu");
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int cx = GetSystemMetrics(SM_CXSIZEFRAME);
    int offset_x = (workArea.right - workArea.left) / 2 - cx;
    int w = (workArea.right - workArea.left) / 2 + cx*2;
    int h = workArea.bottom - workArea.top + GetSystemMetrics(SM_CYSIZEFRAME);
    SetWindowPos(okno_bazy, HWND_TOP, workArea.left + offset_x, workArea.top, w, h, 0);
}


void App::quick_replace(){
    if(!Config::geti()->toolbar_show){
        toolbar_switch(1);
    }
    string selected = get_selected_text();
	selected = trim_spaces(trim_1crlf(selected));
    if(selected.length()==0){
        IO::geti()->error("Pusty tekst do zamiany.");
        return;
    }
    unsigned int sel_start, sel_end;
    get_selected_1(sel_start, sel_end);
    set_selected_1(sel_end, sel_end); //brak zaznaczenia - replace obejmuje ca�o��
    Controls::geti()->set_text("find_edit", selected);
    Controls::geti()->set_text("replace_edit", selected);
    Controls::geti()->select_all("replace_edit");
    Controls::geti()->set_focus("replace_edit");
}


void App::new_file(){
    Controls::geti()->set_text("editor", "");
	refresh_text();
    //usuni�cie nazwy samego pliku
    while(Config::geti()->opened_file.length()>0 && Config::geti()->opened_file[Config::geti()->opened_file.length()-1]!='\\'){
        Config::geti()->opened_file = string_cutfromend(Config::geti()->opened_file, 1);
    }
    Controls::i()->set_text("filename_edit", Config::geti()->opened_file);
	Config::geti()->opened_file = "";
	update_title();
    undo->reset();
	IO::geti()->echo("Nowy plik");
}

void App::open_chords_file(string filename){
    filename = trim_quotes(filename);
    int file_size;
	char *file_content = open_file(filename, file_size);
    if(file_content==NULL) return;
	SetWindowText(Controls::geti()->find("editor"), file_content);
    delete[] file_content;
	set_selected_1(0, 0);
	SendMessage(Controls::geti()->find("editor"), EM_SCROLLCARET, 0, 0);
	Config::geti()->opened_file = filename;
	stringstream ss;
	ss<<"Odczytano plik: "<<filename;
	IO::geti()->echo(ss.str());
    Controls::geti()->set_text("filename_edit", filename);
	refresh_text();
	update_title();
	Config::geti()->transposed = 0;
	autoscroll_off();
    undo->reset(); //reset historii zmian
}

bool App::save_chords_file(){
    string new_filename = Controls::geti()->get_text("filename_edit");
	if(new_filename.length()==0){
        if(!Config::geti()->toolbar_show){
            toolbar_switch(1);
        }
		IO::geti()->error("Podaj nazw� pliku!");
		return false;
	}
    if(Config::geti()->transposed%12 != 0){
        stringstream ss;
        ss<<"Plik zapisywany jest w nowej tonacji: ";
        if(Config::geti()->transposed>0) ss<<"+";
		ss<<Config::geti()->transposed;
        ss<<"\r\nCzy chcesz kontynuowa�?";
        int answer = IO::geti()->message_box_yesno("Ostrze�enie", ss.str());
        if(answer==2){
            IO::geti()->echo("Przerwano zapisywanie pliku.");
            return false;
        }
	}
	Config::geti()->opened_file = new_filename;
	string* str = load_text();
    if(!save_file(Config::geti()->opened_file, *str)){
        delete str;
        return false;
    }
	delete str;
	update_title();
    undo->changed = false;
	IO::geti()->echo("Zapisano plik");
    return true;
}

void App::analyze(){
    int licznik = 0;
    undo->save();
    while(skanuj_1()) licznik++;
    if(licznik==0) IO::geti()->echo("Brak zmian");
    else IO::geti()->echo("Wprowadzono zmiany");
}

void App::transpose(int transponuj){
	if(transponuj==0) return;
	Config::geti()->transposed += transponuj;
    string* str = load_text();
    undo->save(str);
    string trans = transpose_string(*str, transponuj);
    delete str;
	Controls::geti()->set_text("editor", trans);
	refresh_text();
	stringstream ss;
	ss<<"Transpozycja akord�w: ";
	if(Config::geti()->transposed>0) ss<<"+";
	ss<<Config::geti()->transposed;
	IO::geti()->echo(ss.str());
}

void App::associate_files(){
    string exe_path = get_app_path();
    IO::geti()->echo("exe_path: "+exe_path);
    string command_value = "\""+exe_path+"\" \"%1\"";
    string icon = exe_path+",0";
    //HKEY_CURRENT_USER
    if(!set_registry_default_value(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\crd_auto_file\\shell\\open\\command", command_value)) return;
    if(!set_registry_default_value(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\crd_auto_file\\DefaultIcon", icon)) return;
    if(!set_registry_default_value(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\.crd", "crd_auto_file")) return;
    //HKEY_CLASSES_ROOT
    if(!set_registry_default_value(HKEY_CLASSES_ROOT, "crd_auto_file\\shell\\open\\command", command_value)) return;
    if(!set_registry_default_value(HKEY_CLASSES_ROOT, "crd_auto_file\\DefaultIcon", icon)) return;
    if(!set_registry_default_value(HKEY_CLASSES_ROOT, ".crd", "crd_auto_file")) return;
    //HKEY_LOCAL_MACHINE
    if(!set_registry_default_value(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\crd_auto_file\\shell\\open\\command", command_value)) return;
    if(!set_registry_default_value(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\crd_auto_file\\DefaultIcon", icon)) return;
    if(!set_registry_default_value(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\.crd", "crd_auto_file")) return;
    //Applications/SongBook.exe
    if(!set_registry_default_value(HKEY_CLASSES_ROOT, "Applications\\SongBook.exe\\shell\\open\\command", command_value)) return;
    if(!set_registry_default_value(HKEY_CURRENT_USER, "SOFTWARE\\Classes\\Applications\\SongBook.exe\\shell\\open\\command", command_value)) return;
    //user choice
    if(!set_registry_value(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.crd\\UserChoice", "ProgId", "Applications\\SongBook.exe")) return;
    IO::geti()->echo("Pliki .crd zosta�y skojarzone z programem.");
}
