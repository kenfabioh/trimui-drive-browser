#include "core.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <set>

static void fail(const std::string& s) { throw std::runtime_error(s + ": " + strerror(errno)); }
bool validName(const std::string& s) {
    return !s.empty() && s != "." && s != ".." && s.find('/') == s.npos &&
           s.find('\0') == s.npos && s.size() <= 255 && s.compare(0,14,".drivebrowser-") != 0;
}
std::vector<Entry> parseEntries(const std::string& data) {
    auto j = Json::parse(data);
    if (!j.is_array()) throw std::runtime_error("Listagem JSON invalida");
    std::vector<Entry> out;
    std::set<std::string> seen;
    for (const auto& x : j) {
        Entry e;
        e.name = x.at("Name").get<std::string>();
        if (!validName(e.name)) throw std::runtime_error("Nome inseguro na listagem");
        if (!seen.insert(e.name).second) throw std::runtime_error("Nomes duplicados na pasta do Drive; renomeie antes de baixar");
        e.directory = x.at("IsDir").get<bool>();
        e.size = x.value("Size", -1LL);
        e.mime = x.value("MimeType", std::string("application/octet-stream"));
        out.push_back(e);
    }
    std::stable_sort(out.begin(),out.end(),[](const Entry& a,const Entry& b) {
        return a.directory != b.directory ? a.directory > b.directory : a.name < b.name;
    });
    return out;
}
static std::vector<unsigned> searchKey(const std::string& text) {
    // Case/accent folding for Portuguese/Latin; preserve other Unicode exactly.
    // JSON validates listing UTF-8 before it reaches this routine.
    std::vector<unsigned> out;
    for(size_t i=0;i<text.size();) {
        unsigned c=static_cast<unsigned char>(text[i++]);
        int rest=0;
        if(c>=0xF0) {rest=3;c&=7;}
        else if(c>=0xE0) {rest=2;c&=15;}
        else if(c>=0xC0) {rest=1;c&=31;}
        while(rest-- && i<text.size()) c=(c<<6)|(static_cast<unsigned char>(text[i++])&63);
        if(c>='A' && c<='Z') c+=32;
        if((c>=0xC0 && c<=0xD6) || (c>=0xD8 && c<=0xDE)) c+=32;
        if(c>=0xE0 && c<=0xE5) c='a';
        else if(c==0xE7) c='c';
        else if(c>=0xE8 && c<=0xEB) c='e';
        else if(c>=0xEC && c<=0xEF) c='i';
        else if(c==0xF1) c='n';
        else if(c>=0xF2 && c<=0xF6) c='o';
        else if(c>=0xF9 && c<=0xFC) c='u';
        else if(c==0xFD || c==0xFF) c='y';
        if(c>=0x300 && c<=0x36F) continue; // decomposed accent marks
        out.push_back(c);
    }
    return out;
}
bool matchesSearch(const std::string& name,const std::string& term) {
    auto n=searchKey(name),t=searchKey(term);
    return !t.empty() && std::search(n.begin(),n.end(),t.begin(),t.end())!=n.end();
}
char initialGroup(const std::string& name) {
    auto key=searchKey(name);
    if(key.empty()) return '*';
    if(key[0]>='a' && key[0]<='z') return char(key[0]-'a'+'A');
    if(key[0]>='0' && key[0]<='9') return '#';
    return '*';
}
void eraseLastUtf8(std::string& text) {
    if(text.empty()) return;
    size_t p=text.size()-1;
    while(p && (static_cast<unsigned char>(text[p])&0xC0)==0x80) --p;
    text.erase(p);
}
std::vector<SearchResult> searchEntries(const std::string& data,const std::string& term) {
    auto j=Json::parse(data);
    if(!j.is_array()) throw std::runtime_error("Listagem JSON invalida");
    std::vector<SearchResult> out;
    std::set<std::vector<std::string>> seen;
    for(const auto& item:j) {
        bool directory=item.at("IsDir").get<bool>();
        auto name=item.at("Name").get<std::string>(), relative=item.at("Path").get<std::string>();
        std::vector<std::string> parts;
        size_t start=0;
        while(true) {
            auto end=relative.find('/',start);
            auto part=relative.substr(start,end==relative.npos?end:end-start);
            if(!validName(part)) throw std::runtime_error("Caminho inseguro na pesquisa");
            parts.push_back(part);
            if(end==relative.npos) break;
            start=end+1;
        }
        if(parts.back()!=name) throw std::runtime_error("Nome e caminho divergentes na pesquisa");
        if(!seen.insert(parts).second) throw std::runtime_error("Nomes duplicados na pasta do Drive; renomeie antes de baixar");
        if(directory) continue;
        if(matchesSearch(name,term)) out.push_back({{name,item.value("MimeType",std::string("arquivo")),item.value("Size",-1LL),false},parts});
    }
    std::sort(out.begin(),out.end(),[](const SearchResult& a,const SearchResult& b){return a.path<b.path;});
    return out;
}
std::string humanBytes(double n) {
    if (n < 0) return "?";
    const char* units[] = {"B","KiB","MiB","GiB","TiB"}; int u=0;
    while (n>=1024 && u<4) { n/=1024; ++u; }
    char b[80]; snprintf(b,sizeof b,"%.1f %s",n,units[u]); return b;
}
std::string transferError(int code,const std::string& raw) {
    std::string reason="Verifique Wi-Fi, rclone.conf, permissoes e espaco";
    for(const auto& item:std::vector<std::pair<std::string,std::string>>{
        {"invalid_grant","Token expirado/revogado; atualize rclone.conf"},
        {"unauthorized","Autorizacao recusada"},{"403","Permissao ou quota do Google Drive"},
        {"404","Arquivo ou pasta nao encontrado"},{"not found","Arquivo ou pasta nao encontrado"},
        {"timeout","Conexao excedeu o tempo limite"},{"no such host","Falha de DNS/conexao"},
        {"network is unreachable","Sem conexao de rede"},{"no space left","MicroSD sem espaco"},
        {"certificate","Certificado TLS recusado; confira data/hora e certificados"},
        {"encrypted","Configuracao criptografada nao suportada; use uma copia descriptografada"}})
        if(raw.find(item.first)!=std::string::npos) {reason=item.second;break;}
    return "rclone ("+std::to_string(code)+"): "+reason+". Start/X permite tentar novamente.";
}

void Process::start(const std::vector<std::string>& args, int outputFd) {
    if (pid_ > 0 || args.empty()) throw std::runtime_error("Processo ocupado/sem comando");
    output.clear(); error.clear(); status_=-1; stopping_=false; overflow_=false;
    int o[2], e[2];
    if (pipe(o)) fail("pipe");
    if (pipe(e)) { close(o[0]); close(o[1]); fail("pipe"); }
    std::vector<char*> argv;
    for (const auto& s:args) argv.push_back(const_cast<char*>(s.c_str()));
    argv.push_back(nullptr);
    pid_=fork();
    if (pid_==0) {
        setpgid(0,0);
        int nullfd=open("/dev/null",O_RDONLY);
        dup2(nullfd,STDIN_FILENO); close(nullfd);
        dup2(outputFd>=0 ? outputFd : o[1],STDOUT_FILENO);
        dup2(e[1],STDERR_FILENO);
        close(o[0]); close(o[1]); close(e[0]); close(e[1]);
        execv(argv[0],argv.data());
        const char msg[]="Nao foi possivel executar rclone\n";
        write(STDERR_FILENO,msg,sizeof(msg)-1); _exit(127);
    }
    close(o[1]); close(e[1]);
    if (pid_<0) { close(o[0]); close(e[0]); fail("fork"); }
    setpgid(pid_,pid_);
    out_=o[0]; err_=e[0];
    fcntl(out_,F_SETFL,O_NONBLOCK); fcntl(err_,F_SETFL,O_NONBLOCK);
    fcntl(out_,F_SETFD,FD_CLOEXEC); fcntl(err_,F_SETFD,FD_CLOEXEC);
}
void Process::drain(int& fd,std::string& target,size_t limit,bool tail) {
    if (fd<0) return;
    char b[8192];
    // Per-frame budget prevents a verbose child starving SDL input.
    for (int i=0;i<32;++i) {
        ssize_t n=read(fd,b,sizeof b);
        if (!n) { close(fd); fd=-1; break; }
        if (n<0) { if (errno==EINTR) continue; if (errno!=EAGAIN) {close(fd);fd=-1;} break; }
        if (target.size()+n>limit && !tail) { overflow_=true; cancel(); continue; }
        target.append(b,n);
        if (target.size()>limit) target.erase(0,target.size()-limit);
    }
}
bool Process::tick() {
    drain(out_,output,16*1024*1024,false); drain(err_,error,64*1024,true);
    if (pid_>0) {
        if (stopping_ && std::chrono::steady_clock::now()-stopAt_>std::chrono::seconds(2)) kill(-pid_,SIGKILL);
        int s; pid_t r=waitpid(pid_,&s,WNOHANG);
        if (r==pid_) { status_=overflow_ ? 125 : (WIFEXITED(s)?WEXITSTATUS(s):128+WTERMSIG(s)); pid_=-1; }
    }
    return pid_>0 || out_>=0 || err_>=0;
}
void Process::cancel() {
    if (pid_>0 && !stopping_) { stopping_=true; stopAt_=std::chrono::steady_clock::now(); kill(-pid_,SIGTERM); }
}
Process::~Process() {
    if (pid_>0) { kill(-pid_,SIGKILL); while (waitpid(pid_,nullptr,0)<0 && errno==EINTR) {} }
    if (out_>=0) close(out_);
    if (err_>=0) close(err_);
}
std::vector<std::string> Backend::command(const std::string& verb,const std::vector<std::string>& args) const {
    if (verb!="listremotes" && verb!="lsjson" && verb!="cat") throw std::runtime_error("Operacao remota proibida");
    std::vector<std::string> a={executable,"--config",config,"--ask-password=false","--cache-dir",cache,
        "--contimeout","15s","--timeout","30s","--retries","2","--low-level-retries","2",
        "--use-json-log","--stats","1s","--stats-log-level","NOTICE"};
    if (!ca.empty()) { a.push_back("--ca-cert"); a.push_back(ca); }
    if(verb=="cat") {
        if(downloadBufferMiB<1 || downloadBufferMiB>64) throw std::runtime_error("Buffer deve estar entre 1 e 64 MiB");
        a.push_back("--buffer-size");a.push_back(std::to_string(downloadBufferMiB)+"Mi");
    }
    a.push_back(verb); a.insert(a.end(),args.begin(),args.end()); return a;
}

static std::vector<std::string> components(const std::string& p) {
    if (p.empty() || p[0]!='/') throw std::runtime_error("Caminho deve ser absoluto");
    std::vector<std::string> out; size_t i=1;
    while(i<p.size()) { auto end=p.find('/',i); if(end==p.npos) end=p.size();
        auto c=p.substr(i,end-i); if(!validName(c)) throw std::runtime_error("Componente de caminho invalido");
        out.push_back(c); i=end+1;
    } return out;
}
Destination::Destination(const std::string& sd,const std::string& root,const std::vector<std::string>& relative,bool create) {
    auto s=components(sd), r=components(root);
    if(r.size()<=s.size() || !std::equal(s.begin(),s.end(),r.begin()))
        throw std::runtime_error("Destino deve estar dentro do microSD");
    int fd=open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if(fd<0) fail("Abrir raiz");
    try {
        for(size_t i=0;i<r.size();++i) {
            if(create && i>=s.size() && mkdirat(fd,r[i].c_str(),0755)<0 && errno!=EEXIST) fail("Criar destino");
            int next=openat(fd,r[i].c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
            if(next<0) fail("Abrir destino (links nao permitidos)");
            close(fd); fd=next;
        }
        for(const auto& c:relative) {
            if(!validName(c)) throw std::runtime_error("Subpasta invalida");
            int next=openat(fd,c.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
            if(next<0) fail("Abrir subpasta");
            close(fd); fd=next;
        }
        fd_=fd;
    } catch(...) {close(fd); throw;}
}
Destination::~Destination() {if(fd_>=0) close(fd_);}
std::vector<Entry> Destination::directories() const {
    auto out=contents();
    out.erase(std::remove_if(out.begin(),out.end(),[](const Entry& e){return !e.directory;}),out.end());
    return out;
}
std::vector<Entry> Destination::contents() const {
    int f=openat(fd_,".",O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if(f<0) fail("Listar destino");
    DIR* d=fdopendir(f); if(!d) {close(f);fail("Listar destino");}
    std::vector<Entry> out; struct dirent* ent;
    int err=0;
    while(true) {
        errno=0;ent=readdir(d);if(!ent) {err=errno;break;}
        std::string n=ent->d_name; struct stat st;
        if(validName(n) && fstatat(fd_,n.c_str(),&st,AT_SYMLINK_NOFOLLOW)==0) {
            if(S_ISDIR(st.st_mode)) out.push_back({n,"pasta",0,true});
            else if(S_ISREG(st.st_mode)) out.push_back({n,"arquivo",st.st_size,false});
        }
    }
    closedir(d); if(err) {errno=err;fail("Listar destino");}
    std::sort(out.begin(),out.end(),[](const Entry& a,const Entry& b){return a.directory!=b.directory?a.directory>b.directory:a.name<b.name;}); return out;
}
void Download::start(const Backend& b,const std::string& source,const std::string& name,long long size,const Destination& d) {
    if(active || !validName(name)) throw std::runtime_error("Download ocupado/nome invalido");
    struct stat st;
    if(fstatat(d.fd(),name.c_str(),&st,AT_SYMLINK_NOFOLLOW)==0 || errno!=ENOENT)
        throw std::runtime_error("Destino ja existe ou nao pode ser verificado; nao sera sobrescrito");
    struct statvfs v;
    if(fstatvfs(d.fd(),&v)) fail("Verificar espaco");
    if(size>=0 && static_cast<unsigned long long>(size)>static_cast<unsigned long long>(v.f_bavail)*v.f_frsize)
        throw std::runtime_error("Espaco insuficiente no microSD");
    dir_=fcntl(d.fd(),F_DUPFD_CLOEXEC,3); if(dir_<0) fail("Fixar destino");
    name_=name; expected=size; bytes=0; speed=0; cancelled_=false; success=false; copying_=false;
    static unsigned sequence=0;
    for(int i=0;i<100;++i) {
        temporary_=".drivebrowser-"+std::to_string(getpid())+"-"+std::to_string(++sequence)+".part";
        part_=openat(dir_,temporary_.c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if(part_>=0 || errno!=EEXIST) break;
    }
    if(part_<0) {cleanup();fail("Criar arquivo parcial");}
    try { child_.start(b.command("cat",{"--",source}),part_); }
    catch(...) {cleanup();throw;}
    started_=std::chrono::steady_clock::now(); active=true; message="Baixando... B cancela";
}
void Download::cancel() { if(active) { cancelled_=true; child_.cancel(); } }
void Download::cleanup() {
    if(part_>=0) {close(part_);part_=-1;}
    if(dir_>=0) {
        if(!temporary_.empty()) unlinkat(dir_,temporary_.c_str(),0);
        if(publish_>=0) {close(publish_);publish_=-1;unlinkat(dir_,name_.c_str(),0);}
        close(dir_);dir_=-1;
    }
}
Download::~Download() {
    cancel();
    // Reap before closing/removing output; Process destructor is otherwise later.
    while(child_.tick()) usleep(1000);
    cleanup();
}
void Download::tick() {
    if(!active) return;
    try {
        if(copying_) {
            if(cancelled_) throw std::runtime_error("Cancelado");
            char buf[65536];
            // Bounded fallback commit for filesystems without renameat2.
            for(int i=0;i<4;++i) {
                ssize_t n=read(part_,buf,sizeof buf);
                if(n<0) {if(errno==EINTR) continue;fail("Ler parcial");}
                if(!n) {
                    if(fsync(publish_)) fail("Sincronizar destino");
                    close(publish_);publish_=-1;
                    success=true;active=false;message="Download concluido: "+name_;cleanup();return;
                }
                ssize_t p=0; while(p<n) {ssize_t w=write(publish_,buf+p,n-p);if(w<0){if(errno==EINTR)continue;fail("Gravar destino");}p+=w;}
            } return;
        }
        bool running=child_.tick();
        struct stat st; if(fstat(part_,&st)) fail("Medir download"); bytes=st.st_size;
        double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started_).count();
        speed=seconds>0 ? bytes/seconds : 0;
        if(running) return;
        if(cancelled_) throw std::runtime_error("Cancelado; parcial removido");
        if(child_.result()!=0) {
            // Do not persist raw provider errors, URLs or OAuth data.
            throw std::runtime_error(transferError(child_.result(),child_.error));
        }
        if(expected>=0 && bytes!=expected) throw std::runtime_error("Tamanho mudou ou transferencia incompleta; tente atualizar a lista");
        if(fsync(part_)) fail("Sincronizar parcial");
        // Atomic and no overwrite on Linux >=3.15; tested fallback for FAT/older kernels.
        int committed;
#ifdef DRIVEBROWSER_TEST_COPY_COMMIT
        committed=-1;errno=ENOSYS;
#else
        committed=syscall(SYS_renameat2,dir_,temporary_.c_str(),dir_,name_.c_str(),1);
#endif
        if(committed==0) {
            temporary_.clear(); success=true;active=false;message="Download concluido: "+name_;cleanup();return;
        }
        if(errno!=ENOSYS && errno!=EINVAL && errno!=EOPNOTSUPP) fail("Publicar arquivo (sem sobrescrever)");
        publish_=openat(dir_,name_.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if(publish_<0) fail("Criar destino final");
        if(lseek(part_,0,SEEK_SET)<0) fail("Reabrir parcial");
        copying_=true; message="Finalizando no microSD... B cancela";
    } catch(const std::exception& e) {
        message=e.what(); active=false;success=false;cleanup();
    }
}
