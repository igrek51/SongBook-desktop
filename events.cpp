#include "app.h"
#include "system.h"
#include <commctrl.h>
#include <richedit.h>

void App::event_init(HWND *window){
	main_window = *window;
    //parametry
	IO::geti()->get_args();
	//katalog roboczy
	set_workdir();
	//ustawienia
    Config::geti()->load_from_file();
    //je�li aplikacja jest ju� uruchomiona
    if(IO::geti()->args.size()==2 && instancja2!=NULL){//jeden dodatkowy parametr - nazwa pliku do otwarcia
        IO::geti()->log("Wysy�anie pliku do otwartej instancji aplikacji...");
        for(unsigned int i=0; i<IO::geti()->args.at(1).length(); i++){
            SendMessage(instancja2, 0x0319, 69, (char)IO::geti()->args.at(1)[i]);
        }
        SendMessage(instancja2, 0x0319, 69, 0);
        IO::geti()->log("Zamykanie zb�dnej instancji aplikacji...");
        //SendMessage(main_window, WM_DESTROY, 0, 0);
        DestroyWindow(main_window);
        return;
    }
    //log
    if(Config::geti()->log_enabled){
        IO::geti()->clear_log();
    }
	//kontrolki
    IO::geti()->log("Tworzenie kontrolek...");
    Controls::geti()->create_static_center("Plik:", 0, 0, 0, 0, "filename");
    Controls::geti()->create_edit("", 0, 0, 0, 0, "filename_edit");

    Controls::geti()->create_edit_center(Config::i()->find_edit_placeholder, 0, 0, 0, 0, "find_edit");
    Controls::geti()->create_edit_center(Config::i()->replace_edit_placeholder, 0, 0, 0, 0, "replace_edit");
    Controls::geti()->create_edit_center("", 0, 0, 0, 0, "autoscroll_interval");
    Controls::geti()->create_edit_center("", 0, 0, 0, 0, "autoscroll_wait");
    Controls::geti()->create_button("Autoscroll: off", 0, 0, 0, 0, "autoscroll");
    Controls::geti()->create_button("Analizuj", 0, 0, 0, 0, "analyze");

    for(int i=0; i<Config::geti()->cmd_outputs_num; i++){
        stringstream ss;
        ss<<"cmd_output"<<i+1;
        Controls::geti()->create_static_center("", 0, 0, 0, 0, ss.str());
    }
    Controls::geti()->create_edit("", 0, 0, 0, 0, "cmd");
	//edytor
    IO::geti()->log("Tworzenie edytora tekstu...");
	if(LoadLibrary("RICHED32.DLL")==NULL){
		IO::geti()->critical_error("B��d: brak biblioteki RICHED32.DLL");
		return;
	}
    HWND editor_handle = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "", WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_DISABLENOSCROLL, 0, 0, 0, 0, main_window, (HMENU)100, *hInst, 0);
    Controls::geti()->controls.push_back(new Control(editor_handle, "editor"));
    //tworzenie menu
    IO::geti()->log("Tworzenie menu...");
    Menu* menu_plik = new Menu();
    menu_plik->add_option("Nowy [Ctrl + N]", "new");
    menu_plik->add_separator();
    menu_plik->add_option("Otw�rz", "load");
    menu_plik->add_option("Prze�aduj", "reload");
    menu_plik->add_option("Zapisz [Ctrl + S]", "save");
    menu_plik->add_separator();
    menu_plik->add_option("Baza akord�w [Ctrl + B]", "base");
    menu_plik->add_separator();
    menu_plik->add_option("Zako�cz", "exit");
    Menu* menu_edycja = new Menu();
    menu_edycja->add_option("Analizuj i napraw", "analyze");
    menu_edycja->add_separator();
    menu_edycja->add_option("Cofnij [Ctrl + Z]", "undo");
    menu_edycja->add_option("Szukaj [Ctrl + F]", "find");
    menu_edycja->add_option("Zamie�", "replace");
    menu_edycja->add_option("Szybka zamiana tekstu [Ctrl + T]", "quick_replace");
    menu_edycja->add_separator();
    menu_edycja->add_option("Utw�rz akord z zaznaczenia [Ctrl + D]", "add_chord");
    menu_edycja->add_option("Usu� akordy [Ctrl + Q]", "remove_chords");
    menu_edycja->add_option("Usu� alternatywne wersje akord�w", "remove_alt");
    menu_edycja->add_separator();
    menu_edycja->add_option("Zapisz schemat akord�w [Ctrl + W]", "save_pattern");
    menu_edycja->add_option("Wstaw schemat akord�w [Ctrl + E]", "insert_pattern");
    Menu* menu_widok = new Menu();
    menu_widok->add_option("Zwi�ksz czcionk� [Ctrl +]", "font++");
    menu_widok->add_option("Zmniejsz czcionk� [Ctrl -]", "font--");
    menu_widok->add_separator();
    menu_widok->add_option("Formatuj tekst [Ctrl + R]", "format_text");
    menu_widok->add_option("Przewi� na pocz�tek [F1]", "scroll_to_begin");
    menu_widok->add_option("Przewi� na koniec", "scroll_to_end");
    menu_widok->add_separator();
    menu_widok->add_option("Pe�ny ekran [F10, F11]", "fullscreen");
    Menu* menu_autoscroll = new Menu();
    menu_autoscroll->add_option("W��cz z op�nieniem [F7]", "autoscroll_wait");
    menu_autoscroll->add_option("W��cz bez op�nienia [F8]", "autoscroll_nowait");
    menu_autoscroll->add_option("Wy��cz", "autoscroll_off");
    menu_autoscroll->add_separator();
    menu_autoscroll->add_option("Zwolnij przewijanie [F5]", "autoscroll_slower");
    menu_autoscroll->add_option("Przyspiesz przewijanie [F6]", "autoscroll_faster");
    Menu* menu_transpozycja = new Menu();
    menu_transpozycja->add_option("Transponuj 5 p�ton�w w g�r�", "transpose+5");
    menu_transpozycja->add_option("Transponuj 1 p�ton w g�r� [Ctrl + prawo]", "transpose++");
    menu_transpozycja->add_separator();
    menu_transpozycja->add_option("Transponuj 1 p�ton w d� [Ctrl + lewo]", "transpose--");
    menu_transpozycja->add_option("Transponuj 5 p�ton�w w d�", "transpose-5");
    menu_transpozycja->add_separator();
    menu_transpozycja->add_option("Oryginalna tonacja [Ctrl + 0]", "transpose0");
    menu_transpozycja->add_separator();
    menu_transpozycja->add_option("Dodaj alternatywn� tonacj�", "alt");
    Menu* menu_ustawienia = new Menu();
    menu_ustawienia->add_option("Plik konfiguracyjny", "config");
    menu_ustawienia->add_option("Wiersz polece� [Ctrl + `]", "cmd_toggle");
    menu_ustawienia->add_option("Dziennik zdarze�", "log");
    menu_ustawienia->add_option("Skojarz pliki .crd z programem", "associate_files");
    Menu* menu_pomoc = new Menu();
    menu_pomoc->add_option("Polecenia i skr�ty klawiszowe", "help");
    menu_pomoc->add_option("O programie", "info");
    //g��wny pasek menu
    Menu* menu_bar = new Menu();
    menu_bar->add_menu(menu_plik, "Plik");
    menu_bar->add_menu(menu_edycja, "Edycja");
    menu_bar->add_menu(menu_widok, "Widok");
    menu_bar->add_option("Pasek narz�dzi", "toolbar_toggle");
    menu_bar->add_menu(menu_autoscroll, "Autoscroll");
    menu_bar->add_menu(menu_transpozycja, "Transpozycja");
    menu_bar->add_menu(menu_ustawienia, "Ustawienia");
    menu_bar->add_menu(menu_pomoc, "Pomoc");
    SetMenu(main_window, menu_bar->handle);
    //autoscroll edits
    IO::geti()->log("Wype�nianie kontrolek, zmiana czcionek...");
    Controls::geti()->set_text("autoscroll_interval", Config::geti()->autoscroll_interval);
    Controls::geti()->set_text("autoscroll_wait", Config::geti()->autoscroll_wait);
    //czcionki
    controls_fonts_set();
	Controls::geti()->set_focus("editor");
	//subclassing
    IO::geti()->log("Subclassing...");
    for(unsigned int i=0; i<Controls::geti()->controls.size(); i++){
        subclass(Controls::geti()->controls.at(i));
    }
	//pasek schowany przy starcie
	if(!Config::geti()->toolbar_show){
		toolbar_switch(0);
	}
    //baza akord�w na start (je�li nie by� otwierany wybrany plik)
    if(Config::geti()->chordsbase_on_startup && IO::geti()->args.size()<=1){
        chordsbase_start();
        SetForegroundWindow(main_window);
    }
	//okno na po�owie ekranu
	if(Config::geti()->halfscreen==1){
        IO::geti()->log("Rozmieszczanie okna na po�owie ekranu...");
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        int offset_x = - GetSystemMetrics(SM_CXSIZEFRAME);
        int w = (workArea.right - workArea.left) / 2 - offset_x*2;
        int h = workArea.bottom - workArea.top + GetSystemMetrics(SM_CYSIZEFRAME);
		SetWindowPos(main_window, HWND_TOP, workArea.left + offset_x, workArea.top, w, h, 0);
    }
    event_resize();
    //drag & drop
    IO::geti()->log("Uaktywanianie funkcji drag & drop...");
	DragAcceptFiles(main_window, true);
	Controls::geti()->set_text("editor", "");
	//wczytanie pliku zadanego parametrem
	if(IO::geti()->args.size()==2){ //jeden dodatkowy parametr - nazwa pliku do otwarcia
		open_chords_file(IO::geti()->args.at(1));
	}
	update_title();
    refresh_text();
	IO::geti()->echo("Got�w do pracy - wersja "+version);
}

void App::event_button(WPARAM wParam){
    string name = "";
    if(wParam>=1 && wParam<=Controls::geti()->controls.size()){
        name = Controls::geti()->get_button_name(wParam);
    }else{
        name = Controls::geti()->get_menu_name(wParam);
    }
    if(name.length()==0) return;
	if(name == "new"){ //nowy
		new_file();
	}else if(name == "load"){ //wczytaj
        if(!Config::geti()->toolbar_show){
            toolbar_switch(1);
        }
        string str2 = Controls::geti()->get_text("filename_edit");
		if(str2.length()==0){
			IO::geti()->echo("Podaj nazw� pliku.");
		}else{
			open_chords_file(str2);
		}
	}else if(name == "save"){ //zapisz
		save_chords_file();
	}else if(name == "analyze"){ //analizuj
		analyze();
	}else if(name == "replace"){ //zamie�
        if(!Config::geti()->toolbar_show){
            toolbar_switch(1);
            return;
        }
		zamien();
	}else if(name == "find"){ //znajd�
        if(!Config::geti()->toolbar_show){
            toolbar_switch(1);
            return;
        }
		znajdz();
	}else if(name == "undo"){
        undo->revert();
	}else if(name == "base"){ //baza akord�w
        chordsbase_start();
	}else if(name == "autoscroll"){ //autoscroll
		autoscroll_switch();
	}else if(name == "reload"){
        open_chords_file(Config::geti()->opened_file);
    }else if(name == "config"){
        ShellExecute(0,"open",Config::geti()->config_filename.c_str(),"",0,SW_SHOW);
    }else if(name == "quick_replace"){
        quick_replace();
    }else if(name == "remove_chords"){
        usun_akordy();
    }else if(name == "remove_alt"){
        usun_wersje();
    }else if(name == "add_chord"){
        dodaj_nawias();
    }else if(name == "font++"){
        change_font_size(+1);
    }else if(name == "font--"){
        change_font_size(-1);
    }else if(name == "format_text"){
        refresh_text();
    }else if(name == "fullscreen"){
        fullscreen_toggle();
    }else if(name == "autoscroll_wait"){
        autoscroll_on();
    }else if(name == "autoscroll_nowait"){
        autoscroll_nowait();
    }else if(name == "autoscroll_off"){
        autoscroll_off();
        IO::geti()->echo("Autoscroll wy��czony");
    }else if(name == "autoscroll_slower"){
        autoscroll_nowait(+Config::geti()->autoscroll_interval*0.25);
    }else if(name == "autoscroll_faster"){
        autoscroll_nowait(-Config::geti()->autoscroll_interval*0.2);
    }else if(name == "transpose+5"){
        transpose(+5);
    }else if(name == "transpose++"){
        transpose(+1);
    }else if(name == "transpose--"){
        transpose(-1);
    }else if(name == "transpose-5"){
        transpose(-5);
    }else if(name == "transpose0"){
        transpose(-Config::geti()->transposed);
    }else if(name == "alt"){
        dodaj_alternatywne();
    }else if(name == "log"){
        ShellExecute(0, "open", Config::geti()->log_filename.c_str(), "", 0, SW_SHOW);
    }else if(name == "associate_files"){
        associate_files();
    }else if(name == "help"){
        show_help();
    }else if(name == "info"){
        stringstream ss;
        ss<<Config::geti()->program_name<<endl;
        ss<<"wersja "<<version<<endl;
        IO::geti()->message_box("O programie",ss.str());
    }else if(name == "cmd_toggle"){
        cmd_switch();
    }else if(name == "toolbar_toggle"){
        toolbar_switch();
    }else if(name == "scroll_to_begin"){
        set_scroll(0);
    }else if(name == "scroll_to_end"){
        SendMessage(Controls::geti()->find("editor"), WM_VSCROLL, SB_BOTTOM, 0);
    }else if(name == "save_pattern"){
        save_pattern();
    }else if(name == "insert_pattern"){
        insert_pattern();
    }else if(name == "exit"){
        DestroyWindow(main_window);
    }else{
        IO::geti()->error("Zdarzenie nie zosta�o obs�u�one: "+name);
    }
}

void App::event_dropfiles(string filename){
	if(file_exists(filename)){
		open_chords_file(filename);
		Controls::geti()->set_focus("editor");
	}else if(dir_exists(filename)){
		new_file();
		Config::geti()->opened_file = filename;
        if(Config::geti()->opened_file[Config::geti()->opened_file.length()-1]!='\\'){
            Config::geti()->opened_file+="\\";
        }
        Controls::geti()->set_text("filename_edit", Config::geti()->opened_file);
		Controls::geti()->set_focus("filename_edit");
		IO::geti()->echo("Nowy plik w folderze: \""+filename+"\"");
	}else{
		IO::geti()->error("nieprawid�owa �cie�ka: \""+filename+"\"");
	}
	SetForegroundWindow(main_window);
}

void App::event_resize(){
    IO::geti()->log("Resize okna - Od�wie�anie uk�adu kontrolek...");
    if(!Controls::geti()->exists("cmd_output1")) return;
    if(!Controls::geti()->exists("editor")) return;
    int ch = Config::geti()->control_height;
	RECT wnd_rect;
	GetClientRect(main_window, &wnd_rect);
	int w = wnd_rect.right-wnd_rect.left;
	int h = wnd_rect.bottom-wnd_rect.top;
    Config::geti()->window_w = w;
    Config::geti()->window_h = h;
    //rozmieszczenie kontrolek
    int editor_h = h-ch;
    int editor_y = 0;
    if(Config::geti()->cmd_show){
        editor_h -= ch*Config::geti()->cmd_outputs_num;
    }
	if(Config::geti()->toolbar_show){
        editor_h -= ch*2;
        editor_y += ch*2;
    }
    Controls::i()->resize("editor", 0,editor_y,w,editor_h);
    Controls::i()->resize("filename", 0,0,Config::geti()->static_filename_width,ch);
    Controls::i()->resize("filename_edit", Config::geti()->static_filename_width,0,w-Config::geti()->static_filename_width,ch);
    //1. rz�d
    Controls::i()->resize("find_edit", w*0/12,ch*1,w*3/12,ch);
    Controls::i()->resize("replace_edit", w*3/12,ch*1,w*3/12,ch);
    Controls::i()->resize("autoscroll_interval", w*6/12,ch*1,w/12,ch);
    Controls::i()->resize("autoscroll_wait", w*7/12,ch*1,w/12,ch);
    Controls::i()->resize("autoscroll", w*8/12,ch*1,w*2/12,ch);
    Controls::i()->resize("analyze", w*10/12,ch*1,w*2/12,ch);
    //konsola
    if(Config::geti()->cmd_show){
        for(int i=0; i<Config::geti()->cmd_outputs_num; i++){
            stringstream ss;
            ss<<"cmd_output"<<i+1;
            Controls::i()->resize(ss.str(), Config::geti()->cmd_outputs_space,h-ch*(i+2),w-Config::geti()->cmd_outputs_space,ch);
        }
        Controls::i()->resize("cmd", 0,h-ch,w,ch);
    }else{
        Controls::i()->resize("cmd_output1", 0,h-ch,w,ch);
    }
}

void App::event_screensave(){
	IO::geti()->log("Zatrzymywanie wygaszacza ekranu");
	mouse_event(MOUSEEVENTF_MOVE,1,0,0,0);
	mouse_event(MOUSEEVENTF_MOVE,-1,0,0,0);
}

void App::event_timer(){
	autoscroll_exec();
}

void App::event_appcommand(WPARAM wParam, LPARAM lParam){
    if(wParam==69){
        char newc = (char)lParam;
        if(newc==0){ //koniec przesy�ania nazwy pliku
            stringstream ss;
            ss<<"Otwieranie pliku z zewn�trznego polecenia: "<<Config::geti()->file_to_open;
            IO::geti()->log(ss.str());
            open_chords_file(Config::geti()->file_to_open);
            Config::geti()->file_to_open = "";
            SetForegroundWindow(main_window);
            return;
        }
        Config::geti()->file_to_open += newc;
    }
}

bool App::event_syskeydown(WPARAM wParam){
	if(wParam==VK_F10){
		event_keydown(VK_F10);
        return true;
	}
    return false;
}

bool App::event_keydown(WPARAM wParam){
	if(wParam==VK_ESCAPE){
        if(Config::geti()->fullscreen_on){ //wyj�cie z fullscreena
            fullscreen_set(false);
        }else{
            Controls::geti()->set_focus("editor");
        }
	}else if(wParam==VK_F1){
		set_scroll(0);
	}else if(wParam==VK_F2){
		change_scroll(-35);
	}else if(wParam==VK_F3){
		change_scroll(+35);
	}else if(wParam==VK_F5){
		autoscroll_nowait(+Config::geti()->autoscroll_interval*0.25);
	}else if(wParam==VK_F6){
		autoscroll_nowait(-Config::geti()->autoscroll_interval*0.2);
	}else if(wParam==VK_F7){
		autoscroll_switch();
	}else if(wParam==VK_F8){
		if(Config::geti()->autoscroll){
			autoscroll_off();
			IO::geti()->echo("Autoscroll wy��czony");
		}else{
			autoscroll_nowait();
		}
	}else if(wParam==VK_F9){
		toolbar_switch();
	}else if(wParam==VK_F10 || wParam==VK_F11){
        fullscreen_toggle();
	}
    //ctrl
	if(is_control_pressed()){
		if(wParam=='S'){
			save_chords_file();
		}else if(wParam=='F'){
            if(!Config::geti()->toolbar_show){
                toolbar_switch(1);
            }
			Controls::geti()->set_focus("find_edit");
		}else if(wParam=='N'){
            new_file();
        }else if(wParam=='B'){
            chordsbase_start();
        }else if(wParam==VK_ADD){
			change_font_size(+1);
		}else if(wParam==VK_SUBTRACT){
			change_font_size(-1);
		}else if(wParam==VK_OEM_3){ // znaczek `
            cmd_switch();
            if(Config::geti()->cmd_show){
                Controls::geti()->set_focus("cmd");
                Controls::geti()->select_all("cmd");
            }else{
                Controls::geti()->set_focus("editor");
            }
		}else if(wParam==VK_LEFT){
			transpose(-1);
		}else if(wParam==VK_RIGHT){
			transpose(+1);
		}else if(wParam=='0'||wParam==VK_NUMPAD0){
			transpose(-Config::geti()->transposed);
		}
	}
    return true; //przechwycenie
}

bool App::event_close(){
    if(undo->changed){
        int answer = IO::geti()->message_box_yesnocancel("Zapisywanie zmian", "Zmiany wprowadzone w pliku nie zosta�y zapisane?\r\nCzy chcesz zapisa� zmiany?");
        if(answer == 3) return true; //przechwycenie
        if(answer == 1){ //zapisywanie zmian
            if(!save_chords_file()){ //je�li nie uda�o si� zapisa�
                return true; //nie zamykaj programu
            }
        }
    }
    return false;
}
