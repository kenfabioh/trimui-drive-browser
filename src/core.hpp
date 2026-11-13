#pragma once
#include "json.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <sys/types.h>

using Json = nlohmann::json;
struct Entry { std::string name, mime; long long size = 0; bool directory = false; };
bool validName(const std::string& s);
std::vector<Entry> parseEntries(const std::string& data);
struct RemotePage { std::vector<Entry> entries;std::string next;bool resident=true; };
RemotePage parseRemotePage(const std::string& data, size_t limit);
size_t configuredPageSize(const Json& settings);
struct SearchResult { Entry entry; std::vector<std::string> path; };
std::vector<SearchResult> searchEntries(const std::string& data, const std::string& term);
bool matchesSearch(const std::string& name, const std::string& term);
char initialGroup(const std::string& name);
void eraseLastUtf8(std::string& text);
std::string humanBytes(double n);
std::string transferError(int code, const std::string& raw);

// Single-threaded, nonblocking child with separate bounded stdout/stderr.
class Process {
    pid_t pid_ = -1;
    int out_ = -1, err_ = -1, status_ = -1;
    bool stopping_ = false, overflow_ = false;
    std::chrono::steady_clock::time_point stopAt_;
    void drain(int& fd, std::string& target, size_t limit, bool tail);
public:
    std::string output, error;
    ~Process();
    void start(const std::vector<std::string>& args, int outputFd = -1);
    bool tick(); // true while running or pipe output remains
    void cancel();
    int result() const { return status_; }
    bool running() const { return pid_ > 0; }
};

struct Backend {
    std::string executable, config, cache, ca;
    int downloadBufferMiB = 16;
    std::vector<std::string> command(const std::string& verb,
                                    const std::vector<std::string>& args = {}) const;
};

// Pins each destination directory using descriptors; never follows a symlink.
class Destination {
    int fd_ = -1;
public:
    Destination(const std::string& sd, const std::string& allowedRoot,
                const std::vector<std::string>& relative = {}, bool createRoot = false);
    ~Destination();
    Destination(const Destination&) = delete;
    Destination& operator=(const Destination&) = delete;
    int fd() const { return fd_; }
    std::vector<Entry> directories() const;
    std::vector<Entry> contents() const;
};

class Download {
    Process child_;
    int dir_ = -1, part_ = -1, publish_ = -1;
    std::string temporary_, name_;
    bool cancelled_ = false, copying_ = false;
    std::chrono::steady_clock::time_point started_;
    void cleanup();
public:
    bool active = false, success = false;
    long long bytes = 0, expected = -1;
    double speed = 0;
    std::string message;
    ~Download();
    void start(const Backend& backend, const std::string& source,
               const std::string& name, long long size, const Destination& dest);
    void cancel();
    void tick();
};
