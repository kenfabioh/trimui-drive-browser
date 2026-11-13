#include "core.hpp"
#include "navigation.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <cstring>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t quitting=0;
static void stop(int) { quitting=1; }
static std::string join(const std::vector<std::string>& v) {
    std::string s; for(const auto& c:v) {if(!s.empty()) s+="/";s+=c;}return s;
}
static Json readJson(const std::string& p) {std::ifstream f(p);if(!f) throw std::runtime_error("Nao foi possivel ler "+p);Json j;f>>j;return j;}
enum class Screen { Remotes, Files, Roots, Destination, Menu, Help, Keyboard, SearchResults };

class App {
#ifdef DRIVEBROWSER_TESTING
    friend struct UiTest;
#endif
    SDL_Window* window=nullptr; SDL_Renderer* renderer=nullptr; TTF_Font* font=nullptr; TTF_Font* small=nullptr;
    std::vector<SDL_GameController*> controllers;
    std::vector<SDL_Joystick*> joysticks;
    Navigation input;
    bool focused=true;
    Json settings;
    Backend backend;
    std::string sd, remote, root, status="", pendingRemote, pendingSource;
    std::vector<std::string> roots, path, destination, pendingPath;
    std::vector<Entry> rows; Entry selected;
    std::string selectedViewKey;
    size_t selectedPosition=0;
    Screen screen=Screen::Remotes;
    struct Bookmark { std::string identity;size_t position; };
    std::map<std::string,Bookmark> bookmarks;
    bool letterMode=false;
    size_t letterIndex=0;
    const std::string alphabet="^#ABCDEFGHIJKLMNOPQRSTUVWXYZ*";
    bool searchView=false;
    std::vector<SearchResult> hits;
    std::string term, appliedTerm, pendingTerm;
    size_t keyRow=0, keyCol=0;
    const std::vector<std::vector<std::string>> keys={
        {"q","w","e","r","t","y","u","i","o","p"},
        {"a","s","d","f","g","h","j","k","l"},
        {"z","x","c","v","b","n","m"},
        {"0","1","2","3","4","5","6","7","8","9"},
        {"á","à","â","ã","é","ê","í","ó","ô","õ"},
        {"ú","ü","ç",".","-","_","*","(",")","[","]","+","&","@"},
        {"Espaco","Apagar","Limpar","Buscar"}};
    size_t cursor=0; bool loading=false; std::string operation;
    Process query; Download download;
    std::chrono::steady_clock::time_point queryStarted;
    bool queryTimedOut=false;
    bool canJump() const {
        return screen==Screen::Files || screen==Screen::SearchResults || screen==Screen::Destination;
    }
    std::string viewKey() const {
        if(screen==Screen::Files) return Json::array({"files",remote,path}).dump();
        if(screen==Screen::SearchResults) return Json::array({"search",remote,path,appliedTerm}).dump();
        if(screen==Screen::Destination) return Json::array({"local",root,destination}).dump();
        return "";
    }
    std::string rowIdentity(size_t i) const {
        return screen==Screen::Destination?rows.at(i).name:sourceAt(i);
    }
    void rememberCursor() {
        auto key=viewKey();
        if(!key.empty() && cursor<rows.size()) bookmarks[key]={rowIdentity(cursor),cursor};
    }
    void restoreCursor() {
        cursor=0;auto saved=bookmarks.find(viewKey());
        if(saved==bookmarks.end() || rows.empty()) return;
        cursor=std::min(saved->second.position,rows.size()-1);
        for(size_t i=0;i<rows.size();++i) if(rowIdentity(i)==saved->second.identity) {cursor=i;break;}
    }
    void jumpLetter() {
        char letter=alphabet[letterIndex];
        auto label=letter=='^'?std::string("Inicio"):letter=='#'?std::string("0-9"):letter=='*'?std::string("Outros"):std::string(1,letter);
        // Prefer files, even when folders are sorted first. If there is no file
        // for this initial, allow jumping to a folder instead.
        for(bool directory:{false,true}) for(size_t i=0;i<rows.size();++i)
            if(rows[i].directory==directory && (letter=='^' || initialGroup(rows[i].name)==letter)) {
                cursor=i;status=label+": "+rows[i].name+" | A/Select: navegar nos arquivos";return;
            }
        status="Nenhum item para "+label+". A/Select volta aos arquivos.";
    }
    void device(int index) {
        if(SDL_IsGameController(index)) {auto* c=SDL_GameControllerOpen(index);if(c) controllers.push_back(c);}
        else {auto* j=SDL_JoystickOpen(index);if(j) joysticks.push_back(j);}
    }
    void text(const std::string& s,int x,int y,SDL_Color color,int width=1200,bool large=false) {
        if(s.empty()) return;
        auto* surface=TTF_RenderUTF8_Blended_Wrapped(large?font:small,s.c_str(),color,width);
        if(!surface) return;
        auto* texture=SDL_CreateTextureFromSurface(renderer,surface);
        SDL_Rect dst={x,y,surface->w,surface->h};
        if(texture) {SDL_RenderCopy(renderer,texture,nullptr,&dst);SDL_DestroyTexture(texture);}
        SDL_FreeSurface(surface);
    }
    void request(const std::string& kind,const std::vector<std::string>& args,const std::string& purpose="") {
        input.clear();letterMode=false;
        query.start(backend.command(kind,args));operation=purpose.empty()?kind:purpose;loading=true;
        queryStarted=std::chrono::steady_clock::now();queryTimedOut=false;status="Carregando... B cancela";
    }
    void remotes() {request("listremotes",{"--json","--source","file"});}
    void list(const std::string& r,const std::vector<std::string>& p) {
        pendingRemote=r;pendingPath=p;
        request("lsjson",{"--drive-skip-gdocs","--",r+":"+join(p)});
    }
    void showRoots() {
        letterMode=false;
        rows.clear();for(const auto& r:roots) rows.push_back({r,"raiz permitida",0,true});
        screen=Screen::Roots;cursor=0;status="A abre a raiz. B volta ao Drive.";
    }
    void local(const std::string& highlight="") {
        Destination d(sd,root,destination,true); rows=d.contents();
        screen=Screen::Destination;letterMode=false;restoreCursor();
        for(size_t i=0;i<rows.size();++i) if(rows[i].name==highlight) cursor=i;
        status="Conteudo local: A abre pasta | X baixa nesta pasta | B sobe/volta";
    }
    void showRemote() {
        letterMode=false;
        if(searchView) {
            rows.clear();for(const auto& hit:hits) rows.push_back(hit.entry);
            screen=Screen::SearchResults;restoreCursor();
            status=std::to_string(hits.size())+" resultado(s) para: "+appliedTerm+". A seleciona | X baixar | B volta a pasta";
        } else if(remote.empty()) remotes();else list(remote,path);
    }
    void openSearch() {
        if(remote.empty()) {status="Abra primeiro uma pasta/remote do Google Drive.";return;}
        screen=Screen::Keyboard;letterMode=false;keyRow=0;keyCol=0;SDL_StartTextInput();
        status="Pesquisa por parte do nome, nesta pasta e subpastas. A digita | X busca | B volta";
    }
    void search() {
        if(term.find_first_not_of(" \t\r\n")==term.npos) {status="Digite parte do nome do arquivo.";return;}
        request("lsjson",{"--recursive","--drive-skip-gdocs","--",remote+":"+join(path)},"search");
        pendingTerm=term;SDL_StopTextInput();
        status="Pesquisando nesta pasta e subpastas... B cancela (limites: 5 min / 16 MiB).";
    }
    void keyboard(Action a) {
        if(a==Start) {SDL_StopTextInput();menu();return;}
        if(a==B) {SDL_StopTextInput();showRemote();return;}
        if(a==X) {search();return;}
        if(a==Up) keyRow=(keyRow+keys.size()-1)%keys.size();
        if(a==Down) keyRow=(keyRow+1)%keys.size();
        keyCol=std::min(keyCol,keys[keyRow].size()-1);
        if(a==Left) keyCol=(keyCol+keys[keyRow].size()-1)%keys[keyRow].size();
        if(a==Right) keyCol=(keyCol+1)%keys[keyRow].size();
        if(a==A) {
            auto key=keys[keyRow][keyCol];
            if(key=="Buscar") search();
            else if(key=="Apagar") eraseLastUtf8(term);
            else if(key=="Limpar") term.clear();
            else {if(key=="Espaco") key=" ";if(term.size()+key.size()<=128)term+=key;}
        }
    }
    std::string sourceAt(size_t index) const {
        auto p=path;
        if(screen==Screen::SearchResults) p.insert(p.end(),hits.at(index).path.begin(),hits.at(index).path.end());
        else p.push_back(rows.at(index).name);
        return remote+":"+join(p);
    }
    void menu() {
        screen=Screen::Menu;letterMode=false;cursor=0;rows.clear();
        for(const auto& n:{"Escolher destino","Atualizar pasta / tentar novamente","Trocar remote","Controles e informacoes","Pesquisar nesta pasta e subpastas","Sair"}) rows.push_back({n,"",0,false});
    }
    void handle(Action a) {
        if(a==None) return;
        rememberCursor();
        if(!isDirection(a)) input.clear();
        if(download.active) {if(a==B) download.cancel();return;}
        if(loading) {if(a==B) {query.cancel();status="Cancelando consulta...";queryTimedOut=true;}return;}
        if(a==Select) {
            if(canJump()) {
                letterMode=!letterMode;
                if(letterMode) {
                    letterIndex=cursor<rows.size()?alphabet.find(initialGroup(rows[cursor].name)):0;
                    status="Letras: direcional/analogico escolhe | A/Select volta aos arquivos | Inicio: primeiro arquivo | * outros";
                } else status="Arquivos: A abre/seleciona | X baixar | Select letras";
            }
            return;
        }
        if(screen==Screen::Keyboard) {keyboard(a);return;}
        if(screen==Screen::Help) {if(a==B || a==A || a==Start) menu();return;}
        if(a==Start) {menu();return;}
        if(letterMode) {
            if(a==Left || a==Up) letterIndex=(letterIndex+alphabet.size()-1)%alphabet.size();
            if(a==Right || a==Down) letterIndex=(letterIndex+1)%alphabet.size();
            if(isDirection(a)) jumpLetter();
            if(a==A) {jumpLetter();letterMode=false;}
            if(a==B) {letterMode=false;status="Arquivos: A abre/seleciona | X baixar | Select letras";}
            return;
        }
        if(a==Up && cursor) --cursor;
        if(a==Down && cursor+1<rows.size()) ++cursor;
        if(a==Left) cursor=cursor>=10?cursor-10:0;
        if(a==Right && !rows.empty()) cursor=std::min(cursor+10,rows.size()-1);
        if(a==B) {
            if(screen==Screen::Files) {if(path.empty()) remotes();else {auto p=path;p.pop_back();list(remote,p);}}
            else if(screen==Screen::SearchResults) list(remote,path);
            else if(screen==Screen::Destination) {if(destination.empty())showRoots();else{destination.pop_back();local();}}
            else if(screen==Screen::Menu || screen==Screen::Roots) {
                showRemote();
            }
            return;
        }
        if((screen==Screen::Files || screen==Screen::SearchResults) && a==X && !pendingSource.empty()) {showRoots();return;}
        if(screen==Screen::Destination && a==X) {
            if(pendingSource.empty()) {status="Selecione primeiro um arquivo no Drive com A ou X.";return;}
            Destination d(sd,root,destination,true);
            download.start(backend,pendingSource,selected.name,selected.size,d);return;
        }
        if(rows.empty() || cursor>=rows.size()) return;
        auto e=rows[cursor];
        if(screen==Screen::Menu && a==A) {
            if(cursor==0) showRoots();
            if(cursor==1) {if(remote.empty())remotes();else list(remote,path);}
            if(cursor==2) remotes();
            if(cursor==3) screen=Screen::Help;
            if(cursor==4) openSearch();
            if(cursor==5) quitting=1;
        } else if(screen==Screen::Remotes && a==A) {
            if(e.mime!="drive") {status="Remote detectado, mas somente type=drive e suportado nesta versao.";return;}
            pendingSource.clear();list(e.name,{});
        } else if(screen==Screen::Roots && a==A) {root=e.name;destination.clear();local();}
        else if(screen==Screen::Destination && a==A) {
            if(!e.directory) {status="Arquivo local: "+e.name+" | "+humanBytes(e.size);return;}
            destination.push_back(e.name);try{local();}catch(...){destination.pop_back();throw;}
        }
        else if((screen==Screen::Files || screen==Screen::SearchResults) && (a==A || a==X)) {
            if(e.directory) {if(a==A){auto p=path;p.push_back(e.name);list(remote,p);}}
            else {
                selected=e;pendingSource=sourceAt(cursor);
                selectedViewKey=viewKey();selectedPosition=cursor;
                status="Selecionado: "+e.name+". X escolhe destino e inicia download.";
                if(a==X) showRoots();
            }
        }
    }
    Action button(int b) {
        const char* names[]={"up","down","left","right","a","b","x","start","select"};
        int defaults[]={11,12,13,14,1,0,3,6,4};
        Action actions[]={Up,Down,Left,Right,A,B,X,Start,Select};
        auto map=settings.value("buttons",Json::object());
        for(int i=0;i<9;++i) if(b==map.value(names[i],defaults[i])) return actions[i];
        return None;
    }
    Action event(const SDL_Event& e,Uint64 now=SDL_GetTicks64()) {
        if(e.type==SDL_QUIT) {quitting=1;return None;}
        if(e.type==SDL_WINDOWEVENT) {
            if(e.window.event==SDL_WINDOWEVENT_FOCUS_LOST || e.window.event==SDL_WINDOWEVENT_MINIMIZED) {focused=false;input.clear();}
            if(e.window.event==SDL_WINDOWEVENT_FOCUS_GAINED || e.window.event==SDL_WINDOWEVENT_RESTORED) focused=true;
        }
        if(e.type==SDL_CONTROLLERDEVICEADDED) {device(e.cdevice.which);return None;}
        if(e.type==SDL_JOYDEVICEADDED && !SDL_IsGameController(e.jdevice.which)) {device(e.jdevice.which);return None;}
        if(e.type==SDL_CONTROLLERDEVICEREMOVED || e.type==SDL_JOYDEVICEREMOVED) {
            input.clear();
            int id=e.type==SDL_CONTROLLERDEVICEREMOVED?e.cdevice.which:e.jdevice.which;
            for(auto it=controllers.begin();it!=controllers.end();) {
                if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(*it))==id) {SDL_GameControllerClose(*it);it=controllers.erase(it);} else ++it;
            }
            for(auto it=joysticks.begin();it!=joysticks.end();) {
                if(SDL_JoystickInstanceID(*it)==id) {SDL_JoystickClose(*it);it=joysticks.erase(it);} else ++it;
            }
            return None;
        }
        if(!focused) return None;
        if(e.type==SDL_CONTROLLERBUTTONDOWN || e.type==SDL_CONTROLLERBUTTONUP) {
            auto a=button(e.cbutton.button);bool down=e.type==SDL_CONTROLLERBUTTONDOWN;
            if(isDirection(a)) return input.set(Navigation::ControllerButton,e.cbutton.which,e.cbutton.button,down?a:None,now);
            return down?a:None;
        }
        if((e.type==SDL_JOYBUTTONDOWN || e.type==SDL_JOYBUTTONUP) && !SDL_GameControllerFromInstanceID(e.jbutton.which)) {
            auto a=button(e.jbutton.button);bool down=e.type==SDL_JOYBUTTONDOWN;
            if(isDirection(a)) return input.set(Navigation::JoystickButton,e.jbutton.which,e.jbutton.button,down?a:None,now);
            return down?a:None;
        }
        if(e.type==SDL_CONTROLLERAXISMOTION) return input.axis(Navigation::ControllerStick,e.caxis.which,e.caxis.axis,e.caxis.value,now);
        if(e.type==SDL_JOYAXISMOTION && !SDL_GameControllerFromInstanceID(e.jaxis.which))
            return input.axis(Navigation::JoystickStick,e.jaxis.which,e.jaxis.axis,e.jaxis.value,now);
        if(e.type==SDL_JOYHATMOTION && !SDL_GameControllerFromInstanceID(e.jhat.which)) {
            Action a=None;
            if(e.jhat.value&SDL_HAT_UP) a=Up;
            else if(e.jhat.value&SDL_HAT_DOWN) a=Down;
            else if(e.jhat.value&SDL_HAT_LEFT) a=Left;
            else if(e.jhat.value&SDL_HAT_RIGHT) a=Right;
            return input.set(Navigation::JoystickHat,e.jhat.which,e.jhat.hat,a,now);
        }
        if(e.type==SDL_KEYDOWN || e.type==SDL_KEYUP) {
            if(e.type==SDL_KEYDOWN && e.key.repeat) return None;
            Action a=None;
            switch(e.key.keysym.sym) {
                case SDLK_UP:a=Up;break;case SDLK_DOWN:a=Down;break;
                case SDLK_LEFT:a=Left;break;case SDLK_RIGHT:a=Right;break;
            }
            if(a!=None) return input.set(Navigation::KeyboardKey,0,e.key.keysym.sym,e.type==SDL_KEYDOWN?a:None,now);
        }
        if(e.type==SDL_KEYUP) return None;
        if(screen==Screen::Keyboard && !loading) {
            if(e.type==SDL_TEXTINPUT && term.size()+strlen(e.text.text)<=128) term+=e.text.text;
            if(e.type==SDL_KEYDOWN) {
                if(e.key.keysym.sym==SDLK_BACKSPACE) eraseLastUtf8(term);
                if(e.key.keysym.sym==SDLK_RETURN) return X;
                if(e.key.keysym.sym==SDLK_x || e.key.keysym.sym==SDLK_TAB) return None;
            }
        }
        if(e.type==SDL_KEYDOWN) switch(e.key.keysym.sym) {
            case SDLK_UP:return Up;case SDLK_DOWN:return Down;case SDLK_LEFT:return Left;case SDLK_RIGHT:return Right;
            case SDLK_RETURN:return A;case SDLK_ESCAPE:return B;case SDLK_x:return X;case SDLK_TAB:return Start;
            case SDLK_F1:return Select;
        }
        return None;
    }
    void navigate(Uint64 now) {
        if(loading || download.active || !focused) {input.clear();return;}
        handle(input.repeat(now));
    }
    void update() {
        if(loading || download.active) input.clear();
        if(loading) {
            if(std::chrono::steady_clock::now()-queryStarted>std::chrono::seconds(operation=="search"?300:90)) {query.cancel();queryTimedOut=true;}
            if(!query.tick()) {
                loading=false;
                if(queryTimedOut || query.result()!=0) {
                    status=queryTimedOut?"Consulta cancelada/tempo esgotado. Tente pesquisar em uma subpasta.":
                        query.result()==125?"Listagem excedeu 16 MiB. Pesquise em uma subpasta menor; nenhum resultado parcial foi usado.":transferError(query.result(),query.error);
                    if(screen==Screen::Keyboard) SDL_StartTextInput();
                } else {
                    try {
                        std::vector<Entry> result;
                        if(operation=="listremotes") {
                            for(const auto& j:Json::parse(query.output)) {
                                auto n=j.at("name").get<std::string>();
                                if(n.empty() || n.find(':')!=n.npos || n.find('/')!=n.npos) throw std::runtime_error("Nome de remote invalido");
                                result.push_back({n,j.at("type").get<std::string>(),0,true});
                            }
                            remote.clear();path.clear();searchView=false;screen=Screen::Remotes;
                        } else if(operation=="search") {
                            auto results=searchEntries(query.output,pendingTerm);
                            hits=std::move(results);appliedTerm=pendingTerm;searchView=true;pendingSource.clear();showRemote();
                            query.output.clear();return;
                        } else {result=parseEntries(query.output);remote=pendingRemote;path=pendingPath;searchView=false;pendingSource.clear();screen=Screen::Files;}
                        rows=std::move(result);restoreCursor();
                        status=rows.empty()?"Lista vazia. Copie seu rclone.conf para config/ e use Start para atualizar.":"A abre/seleciona | X escolhe destino | Start menu";
                    } catch(const std::exception& ex) {
                        status=std::string(ex.what()).find("duplicados")!=std::string::npos ? ex.what() :
                            "Listagem invalida ou grande demais (limite 16 MiB). Start para tentar novamente.";
                        if(screen==Screen::Keyboard) SDL_StartTextInput();
                    }
                }
            }
        }
        bool wasActive=download.active; download.tick();
        if(wasActive) status=download.message;
        if(wasActive && !download.active && download.success) {
            auto message=download.message;
            if(!selectedViewKey.empty() && !pendingSource.empty()) bookmarks[selectedViewKey]={pendingSource,selectedPosition};
            pendingSource.clear();
            try {local(selected.name);status=message;}
            catch(const std::exception&) {status=message+". Nao foi possivel atualizar a pasta local.";}
        }
    }
    void draw() {
        SDL_SetRenderDrawColor(renderer,15,21,34,255);SDL_RenderClear(renderer);
        SDL_Color white={235,240,250,255},muted={155,174,196,255},accent={89,225,184,255};
        text("DRIVE BROWSER",36,24,accent,900,true);
        text("v1.0.1  |  somente leitura",780,35,muted,460);
        std::string location="Remotes do rclone";
        if(screen==Screen::Files) location=remote+":"+join(path);
        if(screen==Screen::Roots) location="Escolher raiz permitida no microSD";
        if(screen==Screen::Destination) location="Destino: "+root+"/"+join(destination);
        if(screen==Screen::Menu) location="Menu";
        if(screen==Screen::Help) location="Controles e informacoes";
        if(screen==Screen::Keyboard || screen==Screen::SearchResults) location="Pesquisa em: "+remote+":"+join(path);
        SDL_Rect headingClip={36,80,1200,45};SDL_RenderSetClipRect(renderer,&headingClip);
        text(location,36,80,white,1200);
        SDL_RenderSetClipRect(renderer,nullptr);
        if(screen==Screen::Keyboard) {
            SDL_Rect inputClip={36,128,1200,45};SDL_RenderSetClipRect(renderer,&inputClip);
            text(term+"|",36,128,accent,8000,true);SDL_RenderSetClipRect(renderer,nullptr);
            for(size_t r=0;r<keys.size();++r) for(size_t c=0;c<keys[r].size();++c) {
                int width=keys[r].size()<=10 && r<6?118:1200/int(keys[r].size());
                int x=36+(1200-int(keys[r].size())*width)/2+int(c)*width,y=185+int(r)*55;
                SDL_SetRenderDrawColor(renderer,r==keyRow && c==keyCol?35:23,r==keyRow && c==keyCol?100:40,65,255);
                SDL_Rect box={x,y,width-6,46};SDL_RenderFillRect(renderer,&box);
                text(keys[r][c],x+12,y+8,white,width-20);
            }
        } else if(screen==Screen::Help) {
            text("Direcional/analogico esquerdo: navegar; segure para repetir\nA: abrir pasta ou selecionar arquivo\nB: voltar / cancelar consulta ou download\nX: escolher destino; X no destino confirma a pasta\nStart: menu, pesquisa, sair | Select: faixa de letras\nInicio: primeiro arquivo | 0-9: numeros | A-Z: inicial\nPesquisa QWERTY: direcional + A digita; X busca; B volta.\nA pasta local mostra arquivos, tamanhos e o download concluido.\nWi-Fi e rclone.conf valido sao necessarios. Sem login na tela.\nMapeamento e buffer em config/settings.json. Buffer atual: "+std::to_string(backend.downloadBufferMiB)+" MiB\nSDL: "+std::string(SDL_GetCurrentVideoDriver()?SDL_GetCurrentVideoDriver():"?")+" | controles: "+std::to_string(SDL_NumJoysticks())+"\nB volta ao menu.",36,140,white,1200);
        } else {
            size_t first=cursor/9*9;
            for(size_t i=first;i<rows.size() && i<first+9;++i) {
                int y=132+int(i-first)*45;
                if(i==cursor) {SDL_SetRenderDrawColor(renderer,35,68,85,255);SDL_Rect r={24,y-4,1232,43};SDL_RenderFillRect(renderer,&r);}
                const auto& e=rows[i];
                SDL_Rect clip={36,y,900,40};SDL_RenderSetClipRect(renderer,&clip);
                bool remoteFile=screen==Screen::Files || screen==Screen::SearchResults;
                bool marked=remoteFile && pendingSource==sourceAt(i);
                auto label=screen==Screen::SearchResults?join(hits.at(i).path):e.name;
                text((e.directory?"[+] ":marked?"[*] ":"    ")+label,36,y,i==cursor?accent:white,8000);
                SDL_RenderSetClipRect(renderer,nullptr);
                text(e.directory?e.mime:humanBytes(e.size),964,y,muted,280);
            }
            if(!rows.empty() && cursor<rows.size()) {
                SDL_Rect clip={36,550,1200,50};SDL_RenderSetClipRect(renderer,&clip);
                auto label=screen==Screen::SearchResults?join(hits.at(cursor).path):rows[cursor].name;
                text(label+"  |  "+rows[cursor].mime,36,550,muted,1200);
                SDL_RenderSetClipRect(renderer,nullptr);
            }
        }
        int footerY=canJump() && !download.active?605:625;
        SDL_SetRenderDrawColor(renderer,23,33,49,255);SDL_Rect footer={0,footerY,1280,720-footerY};SDL_RenderFillRect(renderer,&footer);
        if(download.active) {
            double fraction=download.expected>0?std::min(1.0,double(download.bytes)/download.expected):0;
            SDL_SetRenderDrawColor(renderer,60,177,145,255);SDL_Rect bar={0,620,int(1280*fraction),5};SDL_RenderFillRect(renderer,&bar);
            text(humanBytes(download.bytes)+" / "+humanBytes(download.expected)+"   "+humanBytes(download.speed)+"/s (media)   B cancela",36,635,accent,1200);
            text(download.message,36,673,white,1200);
        } else if(canJump()) {
            text(letterMode?"LETRAS":"Select",30,618,letterMode?accent:muted,120);
            size_t active=letterMode?letterIndex:cursor<rows.size()?alphabet.find(initialGroup(rows[cursor].name)):alphabet.npos;
            int x=160;
            for(size_t i=0;i<alphabet.size();++i) {
                auto label=alphabet[i]=='^'?std::string("Inicio"):alphabet[i]=='#'?std::string("0-9"):std::string(1,alphabet[i]);
                int width=alphabet[i]=='^'?72:alphabet[i]=='#'?50:34;
                if(i==active) {
                    SDL_SetRenderDrawColor(renderer,letterMode?35:32,letterMode?100:57,65,255);
                    SDL_Rect box={x,610,width-2,38};SDL_RenderFillRect(renderer,&box);
                }
                text(label,x+5,616,i==active?accent:muted,width-6);
                x+=width;
            }
            SDL_Rect clip={36,660,1200,60};SDL_RenderSetClipRect(renderer,&clip);
            text(status,36,660,white,1200);SDL_RenderSetClipRect(renderer,nullptr);
        } else text(status,36,636,white,1200);
        SDL_RenderPresent(renderer);
    }
public:
    App() {
        settings=readJson(access("config/settings.json",R_OK)==0?"config/settings.json":"config/settings.example.json");
        sd=settings.value("sd_root",std::string("/mnt/SDCARD"));
        roots=settings.at("allowed_roots").get<std::vector<std::string>>();
        if(roots.empty()) throw std::runtime_error("Nenhuma raiz de destino permitida");
        // Validate all roots before browsing; no service or firmware changes.
        for(const auto& r:roots) {Destination check(sd,r,{},true);}
        backend={"./bin/rclone","config/rclone.conf","data/cache","res/cacert.pem"};
        backend.downloadBufferMiB=settings.value("download_buffer_mib",16);
        if(backend.downloadBufferMiB<1 || backend.downloadBufferMiB>64) throw std::runtime_error("download_buffer_mib deve estar entre 1 e 64");
        if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_JOYSTICK)!=0) throw std::runtime_error(SDL_GetError());
        if(TTF_Init()!=0) throw std::runtime_error(TTF_GetError());
        window=SDL_CreateWindow("Drive Browser",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1280,720,SDL_WINDOW_SHOWN);
        if(!window) throw std::runtime_error(SDL_GetError());
        renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
        if(!renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
        if(!renderer) throw std::runtime_error(SDL_GetError());
        SDL_RenderSetLogicalSize(renderer,1280,720);SDL_ShowCursor(SDL_DISABLE);
        font=TTF_OpenFont("res/DejaVuSans.ttf",32);small=TTF_OpenFont("res/DejaVuSans.ttf",23);
        if(!font || !small) throw std::runtime_error(TTF_GetError());
        // DEVICEADDED events from SDL_Init will open controllers once in event().
        if(access("config/rclone.conf",R_OK)!=0) {status="Adicione config/rclone.conf. Start > Atualizar para carregar. Botoes: A abrir, B voltar, X baixar.";}
        else remotes();
    }
    ~App() {
        for(auto* c:controllers) SDL_GameControllerClose(c);
        for(auto* j:joysticks) SDL_JoystickClose(j);
        if(small)TTF_CloseFont(small);
        if(font)TTF_CloseFont(font);
        if(renderer)SDL_DestroyRenderer(renderer);
        if(window)SDL_DestroyWindow(window);
        TTF_Quit();SDL_Quit();
    }
    void run(int smoke) {
        int frames=0;
        while(!quitting) {
            // Deterministic synthetic controller input for headless smoke checks.
            if(smoke && frames>=1 && frames<=5) {
                SDL_Event injected={};injected.type=SDL_CONTROLLERBUTTONDOWN;
                injected.cbutton.button=frames==1?SDL_CONTROLLER_BUTTON_START:
                    frames==5?SDL_CONTROLLER_BUTTON_B:SDL_CONTROLLER_BUTTON_DPAD_DOWN;
                SDL_PushEvent(&injected);
                injected.type=SDL_CONTROLLERBUTTONUP;SDL_PushEvent(&injected);
            }
            SDL_Event e;
            while(SDL_PollEvent(&e)) {try{handle(event(e));}catch(const std::exception& ex){status=ex.what();}}
            try {navigate(SDL_GetTicks64());} catch(const std::exception& ex) {status=ex.what();}
            update();draw();SDL_Delay(16);
            if(smoke && ++frames>=smoke) {
                if(screen!=Screen::Help) throw std::runtime_error("Smoke: menu/controles nao responderam");
                auto* surface=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);
                if(!surface || SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch))
                    throw std::runtime_error("Falha captura smoke");
                if(SDL_SaveBMP(surface,"data/smoke.bmp")) throw std::runtime_error(SDL_GetError());
                SDL_FreeSurface(surface);break;
            }
        }
    }
};
int main(int argc,char** argv) {
    signal(SIGINT,stop);signal(SIGTERM,stop);
    if(argc==2 && std::string(argv[1])=="--diagnose") {
        SDL_version v;SDL_GetVersion(&v);auto* t=TTF_Linked_Version();
        Json drivers=Json::array();
        for(int i=0;i<SDL_GetNumVideoDrivers();++i) drivers.push_back(SDL_GetVideoDriver(i));
        std::cout<<Json{{"SDL",std::to_string(v.major)+"."+std::to_string(v.minor)+"."+std::to_string(v.patch)},
            {"SDL_ttf",std::to_string(t->major)+"."+std::to_string(t->minor)+"."+std::to_string(t->patch)},
            {"video_drivers",drivers},{"hardware_tested_by_agent",false},{"validation","maintainer-reported; see docs/VALIDATION.md"}}.dump(2)<<"\n";
        return 0;
    }
    try { App app;app.run(argc==3 && std::string(argv[1])=="--smoke"?std::stoi(argv[2]):0);return 0; }
    catch(const std::exception& e) {std::cerr<<"Drive Browser: "<<e.what()<<"\n";return 1;}
}
