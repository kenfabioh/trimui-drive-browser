#include "core.hpp"
#include <functional>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int checks=0;
static void setup(int result) {if(result!=0)throw std::runtime_error("fixture system call failed");}
static void check(bool ok,const std::string& message) {++checks;if(!ok)throw std::runtime_error(message);}
static void rejects(std::function<void()> f,const std::string& message) {
    bool thrown=false;try{f();}catch(const std::exception&){thrown=true;}check(thrown,message);
}
static std::string readFile(const std::string& p) {std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static void writeFile(const std::string& p,const std::string& s) {std::ofstream f(p,std::ios::binary);f<<s;if(!f)throw std::runtime_error("fixture write");}
static void wait(Process& p) {
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);
    while(p.tick()) {if(std::chrono::steady_clock::now()>end)throw std::runtime_error("child timed out");usleep(2000);}
}
static void wait(Download& d) {
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);
    while(d.active) {d.tick();if(std::chrono::steady_clock::now()>end)throw std::runtime_error("download timed out");usleep(2000);}
}
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("usage: core-test /absolute/path/to/rclone");
        char tmp[]="/tmp/drivebrowser-test-XXXXXX";auto* p=mkdtemp(tmp);if(!p)throw std::runtime_error("mkdtemp");
        std::string base=p, sd=base+"/sd", root=sd+"/downloads", source=base+"/source";
        setup(mkdir(sd.c_str(),0700));setup(mkdir(source.c_str(),0700));
        std::string conf=base+"/rclone.conf";
        writeFile(conf,"[fixture]\ntype = local\n[drive-example]\ntype = drive\nscope = drive.readonly\n");
        Backend backend={argv[1],conf,base+"/cache",""};
        check(validName("Olá 世界 ; $(touch PWN) ' \".zip"),"unicode filename accepted");
        for(const auto& bad:{"",".","..","a/b",".drivebrowser-test.part"}) check(!validName(bad),"unsafe filename rejected");
        check(!validName(std::string("nul\0name",8)),"embedded nul rejected");
        auto entries=parseEntries(R"([{"Name":"é 😀.zip","IsDir":false,"Size":4,"MimeType":"application/zip"},{"Name":"pasta","IsDir":true,"Size":-1}])");
        check(entries.size()==2 && entries[0].directory && entries[1].name=="é 😀.zip","JSON order unicode size");
        rejects([]{parseEntries(R"([{"Name":"../escape","IsDir":false}])");},"listing traversal");
        rejects([]{parseEntries("{}");},"wrong JSON shape");
        rejects([]{parseEntries("[bad");},"malformed JSON");
        rejects([]{parseEntries(R"([{"Name":"same","IsDir":false},{"Name":"same","IsDir":false}])");},"ambiguous duplicate Drive names");
        rejects([&]{backend.command("delete",{});},"remote mutation forbidden");
        check(configuredPageSize(Json::object())==50,"default 50 items per page");
        check(configuredPageSize(Json{{"page_size",17}})==17,"custom page size");
        for(const auto& j:{Json(0),Json(1001),Json("50"),Json(1.5),Json(true)})
            rejects([&]{configuredPageSize(Json{{"page_size",j}});},"invalid page size rejected");
        auto page=parseRemotePage(R"({"Entries":[{"Name":"x","IsDir":false,"Size":3}],"NextPageToken":"opaque+/="})",1);
        check(page.entries.size()==1 && page.next=="opaque+/=","page entries and opaque token parsed");
        rejects([]{parseRemotePage(R"({"Entries":[{"Name":"x","IsDir":false},{"Name":"y","IsDir":false}],"NextPageToken":""})",1);},"oversized page rejected");
        rejects([]{parseRemotePage(R"({"Entries":[],"NextPageToken":4})",50);},"invalid token type rejected");
        auto pageArgs=backend.command("page",{"fixture:","opaque+/=","50"});
        check(std::find(pageArgs.begin(),pageArgs.end(),"drivebrowser-page")!=pageArgs.end(),"page command limited to read-only extension");
        check(matchesSearch("ÁÇÃO Final.ZIP","acao"),"Latin case and accent folding");
        check(matchesSearch("ac\xCC\xA7" "a\xCC\x83o.zip","AÇÃO"),"decomposed Unicode accents");
        check(matchesSearch("世界 arquivo😀","世界") && matchesSearch("世界 arquivo😀","😀"),"Unicode exact search");
        check(!matchesSearch("file.zip","*") && matchesSearch("file*.zip","*"),"search metacharacters are literal");
        check(!matchesSearch("file.zip",""),"empty search does not match everything");
        check(initialGroup("Álbum.zip")=='A' && initialGroup("Ç.zip")=='C',"initial groups fold Latin accents");
        check(initialGroup("e\xCC\x81.zip")=='E',"initial group handles decomposed accent");
        check(initialGroup("9 game.zip")=='#',"numeric initial group");
        check(initialGroup("世界.zip")=='*' && initialGroup("_game.zip")=='*',"non Latin and symbol initial group");
        check(initialGroup("")=='*',"empty name initial group safe");
        std::string edit="aç😀";eraseLastUtf8(edit);check(edit=="aç","erase whole supplementary codepoint");
        eraseLastUtf8(edit);check(edit=="a","erase accented character");
        auto searchJson=R"([{"Name":"pasta","Path":"pasta","IsDir":true},{"Name":"ação.zip","Path":"pasta/ação.zip","IsDir":false,"Size":10},{"Name":"ação.zip","Path":"outra/ação.zip","IsDir":false,"Size":20}])";
        auto results=searchEntries(searchJson,"ACAO");
        check(results.size()==2 && results[0].path[0]=="outra" && results[1].entry.size==10,"recursive search preserves full distinct paths");
        check(searchEntries(searchJson,"missing").empty(),"no search results");
        rejects([]{searchEntries(R"([{"Name":"x","Path":"../x","IsDir":false}])","x");},"search traversal blocked");
        rejects([]{searchEntries(R"([{"Name":"x","Path":"/x","IsDir":false}])","x");},"absolute search path blocked");
        rejects([]{searchEntries(R"([{"Name":"x","Path":"folder//x","IsDir":false}])","x");},"empty search path component blocked");
        rejects([]{searchEntries(R"([{"Name":"x","Path":"folder/y","IsDir":false}])","x");},"mismatched search filename blocked");
        rejects([]{searchEntries(R"([{"Name":"pasta","Path":"pasta","IsDir":true},{"Name":"pasta","Path":"pasta","IsDir":true}])","x");},"ambiguous parent directories blocked");
        auto args=backend.command("cat");auto buffer=std::find(args.begin(),args.end(),"--buffer-size");
        check(buffer!=args.end() && *(buffer+1)=="16Mi","default download buffer explicit");
        auto tuned=backend;tuned.downloadBufferMiB=32;args=tuned.command("cat");buffer=std::find(args.begin(),args.end(),"--buffer-size");
        check(buffer!=args.end() && *(buffer+1)=="32Mi","configured download buffer passed to rclone");
        tuned.downloadBufferMiB=128;rejects([&]{tuned.command("cat");},"excessive buffer rejected");
        rejects([&]{Destination d(sd,base+"/outside",{},true);},"outside root");
        rejects([&]{Destination d(sd,sd+"-evil/downloads",{},true);},"prefix boundary");
        Destination dest(sd,root,{},true);
        setup(mkdir((root+"/subpasta ç").c_str(),0700));
        setup(symlink(base.c_str(),(root+"/escape").c_str()));
        check(dest.directories().size()==1,"symlinks excluded from local listing");
        rejects([&]{Destination d(sd,root,{"escape"});},"symlink traversal");
        rejects([&]{Destination d(sd,root,{".."});},"parent traversal");
        setup(symlink(base.c_str(),(sd+"/badroot").c_str()));
        rejects([&]{Destination d(sd,sd+"/badroot",{},true);},"symlink root");
        Process query;
        query.start(backend.command("listremotes",{"--json","--source","file"}));wait(query);
        auto remotes=Json::parse(query.output);
        check(query.result()==0 && remotes.size()==2,"real rclone remote discovery");
        std::string name="Olá 世界 ; $(touch PWN) ' \".bin";
        std::string payload(256000,'q');payload+="Unicode final ç😀\n";
        writeFile(source+"/"+name,payload);writeFile(source+"/empty","");
        setup(mkdir((source+"/folder").c_str(),0700));
        query.start(backend.command("lsjson",{"--","fixture:"+source}));wait(query);
        auto listing=parseEntries(query.output);
        check(query.result()==0 && listing.size()==3 && listing[0].directory,"real rclone folder list");
        Download d;
        d.start(backend,"fixture:"+source+"/"+name,name,payload.size(),dest);wait(d);
        check(d.success && readFile(root+"/"+name)==payload,"download exact Unicode/shell chars content");
        auto localFiles=dest.contents();
        auto file=std::find_if(localFiles.begin(),localFiles.end(),[&](const Entry& e){return e.name==name;});
        check(file!=localFiles.end() && !file->directory && file->size==static_cast<long long>(payload.size()),"local listing exposes completed file and size");
        check(d.message.find(name)!=std::string::npos,"completion identifies downloaded file");
        writeFile(root+"/.drivebrowser-fixture.part","partial");
        check(dest.contents().size()==localFiles.size(),"local contents hide partials and symlinks");
        setup(unlink((root+"/.drivebrowser-fixture.part").c_str()));
        writeFile(source+"/folder/Áção.zip","nested content");
        query.start(backend.command("lsjson",{"--recursive","--","fixture:"+source}));wait(query);
        auto found=searchEntries(query.output,"acao");
        check(query.result()==0 && found.size()==1 && found[0].path==std::vector<std::string>({"folder","Áção.zip"}),"real rclone recursive search");
        d.start(backend,"fixture:"+source+"/"+found[0].path[0]+"/"+found[0].path[1],found[0].entry.name,found[0].entry.size,dest);wait(d);
        check(d.success && readFile(root+"/Áção.zip")=="nested content","download located search result");
        check(access("PWN",F_OK)!=0,"no shell interpolation");
        rejects([&]{d.start(backend,"fixture:"+source+"/"+name,name,payload.size(),dest);},"no overwrite existing");
        check(readFile(root+"/"+name)==payload,"existing content unchanged");
        setup(symlink((base+"/outside").c_str(),(root+"/symlink-file").c_str()));
        rejects([&]{d.start(backend,"fixture:"+source+"/empty","symlink-file",0,dest);},"no symlink output");
        d.start(backend,"fixture:"+source+"/empty","zero",0,dest);wait(d);
        check(d.success && access((root+"/zero").c_str(),F_OK)==0,"zero byte file");
        d.start(backend,"fixture:"+source+"/missing","missing",0,dest);wait(d);
        check(!d.success && access((root+"/missing").c_str(),F_OK)!=0,"missing source fails without final file");
        d.start(backend,"fixture:"+source+"/"+name,"wrong-size",1,dest);wait(d);
        check(!d.success && access((root+"/wrong-size").c_str(),F_OK)!=0,"size mismatch not published");
        // Deterministic slow/error children test cancellation and partial cleanup.
        auto slow=base+"/slow";
        writeFile(slow,"#!/bin/sh\nprintf partial\nsleep 20\n");setup(chmod(slow.c_str(),0700));
        Backend slowBackend={slow,conf,base,""};
        d.start(slowBackend,"fixture:x","cancelled",-1,dest);
        usleep(60000);d.tick();check(d.active && d.bytes>0,"partial progress measured");
        d.cancel();wait(d);
        check(!d.success && access((root+"/cancelled").c_str(),F_OK)!=0,"cancel removes partial and final");
        writeFile(slow,"#!/bin/sh\nprintf partial\nexit 7\n");
        d.start(slowBackend,"fixture:x","interrupted",-1,dest);wait(d);
        check(!d.success && access((root+"/interrupted").c_str(),F_OK)!=0,"interrupted download not published");
        writeFile(slow,"#!/bin/sh\nprintf data\nsleep 0.1\n");
        d.start(slowBackend,"fixture:x","raced",4,dest);
        writeFile(root+"/raced","original");wait(d);
        check(!d.success && readFile(root+"/raced")=="original","concurrent file not overwritten at commit");
        rejects([&]{d.start(backend,"fixture:x","too-big",9223372036854775807LL,dest);},"free space checked before transfer");
        // Directory scan includes files so we can prove no app-owned partials remain.
        query.start({"/usr/bin/find",root,"-name",".drivebrowser-*"});wait(query);
        check(query.output.empty(),"all handled failure partials cleaned");
        auto noexec=backend;noexec.executable=base+"/noexec";
        d.start(noexec,"fixture:x","exec-failure",0,dest);wait(d);
        check(!d.success && d.message.find("127")!=std::string::npos,"exec failure propagated");
        check(transferError(1,"invalid_grant https://secret").find("https")==std::string::npos,"error redaction");
        // Directory FD remains confined if a path is replaced after selection.
        Destination pinned(sd,root,{"subpasta ç"});
        setup(rename((root+"/subpasta ç").c_str(),(root+"/moved").c_str()));
        setup(symlink(base.c_str(),(root+"/subpasta ç").c_str()));
        d.start(backend,"fixture:"+source+"/empty","pinned",0,pinned);wait(d);
        check(d.success && access((root+"/moved/pinned").c_str(),F_OK)==0 && access((base+"/pinned").c_str(),F_OK)!=0,"descriptor pinning prevents path swap escape");
        std::cout<<"PASS "<<checks<<" checks; fixtures: "<<base<<"\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<"\n";return 1;}
}
