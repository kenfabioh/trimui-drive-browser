// Runs the production state machine and SDL controller mapping against real
// rclone file operations. Only remote discovery is stubbed by the fixture.
#define DRIVEBROWSER_TESTING
#define main drivebrowser_application_main
#include "../src/main.cpp"
#undef main
#include <stdexcept>

struct UiTest {
    static int checks;
    static void check(bool ok,const char* message) {++checks;if(!ok) throw std::runtime_error(message);}
    static void press(App& app,int b) {
        SDL_Event e={};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=b;
        app.handle(app.event(e));e.type=SDL_CONTROLLERBUTTONUP;app.handle(app.event(e));app.update();app.draw();
    }
    static void wait(App& app) {
        auto until=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(app.loading || app.download.active) {
            app.update();app.draw();SDL_Delay(5);
            if(std::chrono::steady_clock::now()>until) throw std::runtime_error("UI wait timeout");
        }
    }
    static void key(App& app,const std::string& key) {
        for(size_t r=0;r<app.keys.size();++r) for(size_t c=0;c<app.keys[r].size();++c)
            if(app.keys[r][c]==key) {
                while(app.keyRow!=r) press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                while(app.keyCol!=c) press(app,SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
                press(app,SDL_CONTROLLER_BUTTON_B);return;
            }
        throw std::runtime_error("missing keyboard key");
    }
    static void openSearch(App& app) {
        press(app,SDL_CONTROLLER_BUTTON_START);
        for(int i=0;i<4;++i) press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.screen==Screen::Keyboard,"search opened from menu");
    }
    static void focusName(App& app,const std::string& name) {
        if(app.letterMode) throw std::runtime_error("unexpected letter focus");
        for(size_t i=0;i<app.rows.size();++i) if(app.rows[i].name==name) {
            while(app.cursor<i) press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            while(app.cursor>i) press(app,SDL_CONTROLLER_BUTTON_DPAD_UP);
            return;
        }
        throw std::runtime_error("fixture file missing: "+name);
    }
    static void letter(App& app,char ch) {
        if(!app.letterMode) press(app,SDL_CONTROLLER_BUTTON_BACK);
        size_t target=app.alphabet.find(ch),n=app.alphabet.size();
        if(target==std::string::npos || !app.letterMode) throw std::runtime_error("letter selector unavailable");
        while(app.letterIndex!=target) {
            auto right=(target+n-app.letterIndex)%n,left=(app.letterIndex+n-target)%n;
            press(app,right<=left?SDL_CONTROLLER_BUTTON_DPAD_RIGHT:SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        }
    }
    static void capture(App& app,const char* path) {
        app.draw();auto* s=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);
        check(s && SDL_RenderReadPixels(app.renderer,nullptr,s->format->format,s->pixels,s->pitch)==0,"capture UI frame");
        check(SDL_SaveBMP(s,path)==0,"save UI capture");SDL_FreeSurface(s);
    }
    static void browseTests(App& app) {
        press(app,SDL_CONTROLLER_BUTTON_A);wait(app); // search -> jogos
        press(app,SDL_CONTROLLER_BUTTON_A);wait(app); // jogos -> root
        focusName(app,"zz-alfabeto");press(app,SDL_CONTROLLER_BUTTON_B);wait(app);
        check(app.path==std::vector<std::string>({"zz-alfabeto"}),"alphabet fixture opened");
        focusName(app,"M01.zip");auto index=app.cursor;
        check(index>=9,"selected file on a later page");
        press(app,SDL_CONTROLLER_BUTTON_B); // mark M01
        press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN); // cursor differs from marked file
        check(app.rows[app.cursor].name!="M01.zip","cursor moved after marking download");
        press(app,SDL_CONTROLLER_BUTTON_Y);press(app,SDL_CONTROLLER_BUTTON_B);press(app,SDL_CONTROLLER_BUTTON_B);
        press(app,SDL_CONTROLLER_BUTTON_Y);wait(app);
        check(app.download.success && app.rows[app.cursor].name=="M01.zip","normal folder file downloaded and focused locally");
        letter(app,'M');press(app,SDL_CONTROLLER_BUTTON_B);
        check(!app.letterMode && app.rows[app.cursor].name=="M01.zip","letter jump works in local destination");
        press(app,SDL_CONTROLLER_BUTTON_A);press(app,SDL_CONTROLLER_BUTTON_A);press(app,SDL_CONTROLLER_BUTTON_A);wait(app);
        check(app.screen==Screen::Files && app.rows[app.cursor].name=="M01.zip" && app.cursor==index,"B restores downloaded source and page, not last cursor or first row");
        press(app,SDL_CONTROLLER_BUTTON_START);press(app,SDL_CONTROLLER_BUTTON_A);wait(app);
        check(app.rows[app.cursor].name=="M01.zip","menu return preserves source focus");
        {std::ofstream f("source/zz-alfabeto/001-added.zip");f<<"new";}
        press(app,SDL_CONTROLLER_BUTTON_START);press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN);press(app,SDL_CONTROLLER_BUTTON_B);wait(app);
        check(app.rows[app.cursor].name=="M01.zip" && app.cursor!=index,"focus restored by identity after listing order changes");
        letter(app,'A');
        check(app.rows[app.cursor].name=="A00.zip" && !app.rows[app.cursor].directory,"initial jump prefers first file over same-letter folder");
        capture(app,"data/ui-letters.bmp");
        press(app,SDL_CONTROLLER_BUTTON_B);
        check(!app.letterMode && app.pendingSource.empty() && app.screen==Screen::Files,"A exits letter selector without opening or marking file");
        press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        check(app.rows[app.cursor].name=="A01.zip","list navigation resumes after letter selection");
        press(app,SDL_CONTROLLER_BUTTON_BACK);press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.rows[app.cursor].name=="A00.zip","confirming current letter jumps to its first file");
        letter(app,'#');
        check(app.rows[app.cursor].name=="001-added.zip","0-9 targets first numeric file");
        letter(app,'^');
        check(app.rows[app.cursor].name=="001-added.zip" && !app.rows[app.cursor].directory,"Inicio targets first file, skipping folders");
        letter(app,'*');check(app.rows[app.cursor].name=="_extra.zip","symbol and non Latin group");
        letter(app,'P');auto old=app.cursor;press(app,SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        check(app.cursor==old && app.status.find("Nenhum")!=std::string::npos,"missing letter leaves cursor unchanged and reports absence");
        press(app,SDL_CONTROLLER_BUTTON_Y);check(!app.download.active && app.letterMode,"X cannot accidentally download in letter mode");
        letter(app,'V');check(app.rows[app.cursor].name=="V-pasta" && app.rows[app.cursor].directory,"folder fallback for letter without files");
        press(app,SDL_CONTROLLER_BUTTON_A);
        check(!app.letterMode && app.path==std::vector<std::string>({"zz-alfabeto"}),"B leaves letters without navigating out of folder");
        focusName(app,"M01.zip");
        if(unlink("source/zz-alfabeto/M01.zip")) throw std::runtime_error("fixture unlink failed");
        press(app,SDL_CONTROLLER_BUTTON_START);press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN);press(app,SDL_CONTROLLER_BUTTON_B);wait(app);
        check(app.cursor<app.rows.size() && app.rows[app.cursor].name!="M01.zip","removed source falls back to a valid nearby row");
        press(app,SDL_CONTROLLER_BUTTON_A);wait(app);
        check(app.rows[app.cursor].name=="zz-alfabeto","parent folder restores child folder focus");
    }
    static void inputTests(App& app) {
        app.menu();
        SDL_Event e={};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.which=55;e.cbutton.button=SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        app.handle(app.event(e,1000));check(app.cursor==1,"D-pad press moves immediately");
        app.navigate(1349);check(app.cursor==1,"short press does not repeat early");
        app.navigate(1350);app.navigate(1440);check(app.cursor==3,"held D-pad advances continuously");
        e.type=SDL_CONTROLLERBUTTONUP;app.handle(app.event(e,1441));app.navigate(2000);
        check(app.cursor==3,"D-pad release stops list");
        app.cursor=app.rows.size()-1;e.type=SDL_CONTROLLERBUTTONDOWN;app.handle(app.event(e,2100));app.navigate(2450);
        check(app.cursor==app.rows.size()-1,"held down clamps at last item");
        e.type=SDL_CONTROLLERBUTTONUP;app.handle(app.event(e,2451));
        app.cursor=0;e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=SDL_CONTROLLER_BUTTON_DPAD_UP;app.handle(app.event(e,2460));app.navigate(2810);
        check(app.cursor==0,"held up clamps at first item");
        e.type=SDL_CONTROLLERBUTTONUP;app.handle(app.event(e,2811));
        app.cursor=0;e={};e.type=SDL_CONTROLLERAXISMOTION;e.caxis.which=55;e.caxis.axis=SDL_CONTROLLER_AXIS_LEFTY;e.caxis.value=24000;
        app.handle(app.event(e,3000));check(app.cursor==1,"left analog moves list");
        e.caxis.value=26000;app.handle(app.event(e,3100));app.navigate(3350);
        check(app.cursor==2,"moving held analog keeps original repeat deadline");
        e.caxis.value=0;app.handle(app.event(e,3400));app.navigate(4000);check(app.cursor==2,"centering analog stops");
        e.caxis.axis=SDL_CONTROLLER_AXIS_LEFTX;e.caxis.value=24000;app.handle(app.event(e,4100));
        check(app.cursor==5,"analog right uses page navigation");
        e.caxis.value=-24000;app.handle(app.event(e,4200));check(app.cursor==0,"analog left uses page navigation");
        e.caxis.value=0;app.handle(app.event(e,4201));
        e.caxis.axis=SDL_CONTROLLER_AXIS_RIGHTY;e.caxis.value=28000;app.handle(app.event(e,4300));app.navigate(5000);
        check(app.cursor==0,"right analog does not navigate");
        e={};e.type=SDL_JOYAXISMOTION;e.jaxis.which=777;e.jaxis.axis=1;e.jaxis.value=23000;
        app.handle(app.event(e,6000));app.navigate(6350);check(app.cursor==2,"raw joystick fallback moves and repeats");
        e.jaxis.value=0;app.handle(app.event(e,6351));
        e={};e.type=SDL_JOYHATMOTION;e.jhat.which=777;e.jhat.value=SDL_HAT_DOWN;app.handle(app.event(e,6400));app.navigate(6750);
        check(app.cursor==4,"raw hat moves and repeats");
        e.jhat.value=SDL_HAT_CENTERED;app.handle(app.event(e,6751));app.navigate(8000);check(app.cursor==4,"centered raw hat stops");
        app.cursor=0;e={};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_DOWN;app.handle(app.event(e,9000));
        e.key.repeat=1;app.handle(app.event(e,9200));check(app.cursor==1,"OS keyboard repeat ignored");
        app.navigate(9350);check(app.cursor==2,"keyboard uses controlled repeat");
        e.type=SDL_KEYUP;app.handle(app.event(e,9360));app.navigate(10000);check(app.cursor==2,"keyboard release stops");
        e={};e.type=SDL_CONTROLLERAXISMOTION;e.caxis.axis=SDL_CONTROLLER_AXIS_LEFTY;e.caxis.value=24000;
        app.handle(app.event(e,11000));
        e={};e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_FOCUS_LOST;app.handle(app.event(e,11010));app.navigate(12000);
        check(app.cursor==3,"focus loss cancels repeat");
        e.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;app.handle(app.event(e,12001));app.navigate(13000);
        check(app.cursor==3,"focus regain does not resume stale hold");
        e={};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.which=55;e.cbutton.button=SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        app.handle(app.event(e,14000));
        e={};e.type=SDL_CONTROLLERDEVICEREMOVED;e.cdevice.which=55;app.handle(app.event(e,14001));app.navigate(15000);
        check(app.cursor==4,"controller removal cancels repeat");
        e={};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.button=SDL_CONTROLLER_BUTTON_DPAD_DOWN;app.handle(app.event(e,16000));
        press(app,SDL_CONTROLLER_BUTTON_START);app.navigate(17000);check(app.cursor==0,"changing screens clears held directions");
        app.loading=true;e.cbutton.button=SDL_CONTROLLER_BUTTON_DPAD_DOWN;app.handle(app.event(e,18000));app.navigate(19000);
        app.loading=false;app.navigate(20000);check(app.cursor==0,"busy screen does not retain held navigation");
        app.remotes();wait(app);
    }
    static void run() {
        App app;wait(app);
        check(app.backend.downloadBufferMiB==32,"download buffer loaded from user settings");
        check(app.screen==Screen::Remotes && app.rows.size()==1,"fixture discovered");
        inputTests(app);
        press(app,SDL_CONTROLLER_BUTTON_B);wait(app);
        check(app.screen==Screen::Files,"remote root opened");
        openSearch(app);
        check(app.keys[0]==std::vector<std::string>({"q","w","e","r","t","y","u","i","o","p"}),"QWERTY first row");
        check(app.keys[1]==std::vector<std::string>({"a","s","d","f","g","h","j","k","l"}),"QWERTY home row");
        check(app.keys[2]==std::vector<std::string>({"z","x","c","v","b","n","m"}),"QWERTY lower row");
        SDL_Event stick={};stick.type=SDL_CONTROLLERAXISMOTION;stick.caxis.axis=SDL_CONTROLLER_AXIS_LEFTX;stick.caxis.value=25000;
        app.handle(app.event(stick,100));app.navigate(450);
        check(app.keyCol==2,"left analog hold navigates on-screen keyboard");
        stick.caxis.value=0;app.handle(app.event(stick,451));app.navigate(1000);
        check(app.keyCol==2,"centered analog stops keyboard navigation");
        press(app,SDL_CONTROLLER_BUTTON_START);openSearch(app);
        for(const auto& s:{"a","c","a","o"}) key(app,s);
        check(app.term=="acao","controller keyboard text entry");
        key(app,"ç");key(app,"Apagar");
        check(app.term=="acao","controller keyboard UTF-8 backspace");
        capture(app,"data/ui-qwerty.bmp");
        press(app,SDL_CONTROLLER_BUTTON_Y);wait(app);
        check(app.screen==Screen::SearchResults && app.hits.size()==2,"recursive search completed");
        check(app.hits[1].path[0]=="outros","same filename disambiguated by folder");
        press(app,SDL_CONTROLLER_BUTTON_DPAD_DOWN);press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.pendingSource=="fixture:outros/Áção final.zip","selected result retains its full source path");
        press(app,SDL_CONTROLLER_BUTTON_Y);
        check(app.screen==Screen::Roots,"result can choose destination");
        press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.screen==Screen::Destination,"root contents shown");
        check(app.rows[0].directory,"local folders before files");
        press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.destination.size()==1,"local folder entered");
        check(app.rows.size()==1 && app.rows[0].name=="manter.txt","existing local file displayed");
        press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.destination.size()==1,"A on a local file does not enter it");
        press(app,SDL_CONTROLLER_BUTTON_Y);wait(app);
        check(app.download.success,"result downloaded successfully");
        check(app.screen==Screen::Destination && app.rows.size()==2,"destination refreshed after download");
        check(app.rows[app.cursor].name=="Áção final.zip" && app.rows[app.cursor].size==19,"completed file focused with its size");
        check(app.status.find("Áção final.zip")!=app.status.npos,"completion status includes filename");
        check(app.pendingSource.empty(),"completed selection cleared");
        app.draw();
        auto* surface=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);
        check(surface && SDL_RenderReadPixels(app.renderer,nullptr,surface->format->format,surface->pixels,surface->pitch)==0,"local screen captured");
        check(SDL_SaveBMP(surface,"data/ui-download.bmp")==0,"screenshot saved");SDL_FreeSurface(surface);
        press(app,SDL_CONTROLLER_BUTTON_Y);
        check(!app.download.active,"download cannot accidentally repeat after completion");
        press(app,SDL_CONTROLLER_BUTTON_A);press(app,SDL_CONTROLLER_BUTTON_A);press(app,SDL_CONTROLLER_BUTTON_A);
        check(app.screen==Screen::SearchResults,"B restores search results after selecting destination");
        check(app.cursor==1 && app.sourceAt(app.cursor)=="fixture:outros/Áção final.zip","B restores exact downloaded search result, not same-name first result");
        letter(app,'A');press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.cursor==0 && app.hits[app.cursor].path[0]=="jogos","search initial jump uses basename and can return to first matching result");
        press(app,SDL_CONTROLLER_BUTTON_A);wait(app);
        check(app.screen==Screen::Files,"B from search returns to search origin folder");
        openSearch(app);key(app,"Limpar");key(app,"z");key(app,"z");key(app,"z");
        press(app,SDL_CONTROLLER_BUTTON_Y);wait(app);
        check(app.screen==Screen::SearchResults && app.hits.empty(),"empty search results handled");
        letter(app,'^');check(app.cursor==0 && app.rows.empty() && app.letterMode,"letter selector safe for empty list");
        press(app,SDL_CONTROLLER_BUTTON_B);check(!app.letterMode && app.rows.empty() && app.status.find("Nenhum")!=std::string::npos,"empty letter confirmation reports absence and returns to empty file view");
        press(app,SDL_CONTROLLER_BUTTON_A);wait(app);
        openSearch(app);press(app,SDL_CONTROLLER_BUTTON_A);wait(app);
        check(app.screen==Screen::Files,"keyboard cancel returns to Drive");
        openSearch(app);key(app,"Limpar");press(app,SDL_CONTROLLER_BUTTON_Y);
        check(!app.loading && app.screen==Screen::Keyboard,"empty query does not launch a recursive listing");
        key(app,"a");
        {std::ofstream f("data/slow-search");f<<"slow";}
        press(app,SDL_CONTROLLER_BUTTON_Y);
        check(app.loading,"slow recursive search started");
        press(app,SDL_CONTROLLER_BUTTON_A);wait(app);if(unlink("data/slow-search")) throw std::runtime_error("fixture unlink failed");
        check(app.screen==Screen::Keyboard && app.status.find("cancelada")!=app.status.npos,"B cancels running search without publishing partial results");
        press(app,SDL_CONTROLLER_BUTTON_A);wait(app);
        press(app,SDL_CONTROLLER_BUTTON_B);wait(app);
        check(app.path==std::vector<std::string>({"jogos"}),"entered Drive subfolder");
        openSearch(app);key(app,"Limpar");for(const auto& s:{"a","c","a","o"}) key(app,s);
        press(app,SDL_CONTROLLER_BUTTON_Y);wait(app);
        check(app.hits.size()==1,"search scope limited to current subtree");
        press(app,SDL_CONTROLLER_BUTTON_B);
        check(app.pendingSource=="fixture:jogos/Áção final.zip","subfolder search builds exact source without repeated prefix");
        browseTests(app);
        std::cout<<"PASS UI "<<checks<<" checks\n";
    }
};
int UiTest::checks=0;
int main() {
    try {UiTest::run();return 0;}
    catch(const std::exception& e) {std::cerr<<"FAIL UI after "<<UiTest::checks<<": "<<e.what()<<"\n";return 1;}
}
