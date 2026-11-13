#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <tuple>

enum Action { None, Up, Down, Left, Right, A, B, X, Start, Select };
inline bool isDirection(Action a) { return a>=Up && a<=Right; }

// One timer for all navigation sources. Analog motion must not restart repeat;
// overlapping D-pad/axis reports must not produce two steps in one direction.
class Navigation {
public:
    enum Source { ControllerButton, JoystickButton, KeyboardKey, JoystickHat,
                  ControllerStick, JoystickStick };
    static constexpr uint64_t delayMs=350, intervalMs=90;
    static constexpr int engage=12000, release=8000;
private:
    using Key=std::tuple<int,int,int>;
    struct Held { Action direction; uint64_t order; };
    struct Stick { int x=0,y=0; };
    std::map<Key,Held> held;
    std::map<std::pair<int,int>,Stick> sticks;
    uint64_t order=0, next=0;
    Action active=None;
public:
    Action set(Source source,int device,int control,Action direction,uint64_t now) {
        Key key{source,device,control};
        auto found=held.find(key);
        if(direction==None) held.erase(key);
        else if(isDirection(direction)) {
            if(found!=held.end() && found->second.direction==direction) return None;
            held[key]={direction,++order};
        } else return None;
        Action selected=None;uint64_t latest=0;
        for(const auto& h:held) if(h.second.order>latest) {latest=h.second.order;selected=h.second.direction;}
        if(selected==active) return None;
        active=selected;next=now+delayMs;
        return active;
    }
    Action axis(Source source,int device,int axis,int value,uint64_t now) {
        if(axis!=0 && axis!=1) return None; // only the left stick
        auto& s=sticks[{source,device}];
        if(axis==0) s.x=value;else s.y=value;
        auto previous=held.find(Key{source,device,0});
        Action direction=previous==held.end()?None:previous->second.direction;
        // Separate engage/release thresholds and axis hysteresis suppress drift
        // and diagonal oscillation. A diagonal moves along one axis at a time.
        bool keep=(direction==Up && s.y<=-release && std::abs(s.x)<=-s.y+4000) ||
                  (direction==Down && s.y>=release && std::abs(s.x)<=s.y+4000) ||
                  (direction==Left && s.x<=-release && std::abs(s.y)<=-s.x+4000) ||
                  (direction==Right && s.x>=release && std::abs(s.y)<=s.x+4000);
        if(!keep) {
            if(std::max(std::abs(s.x),std::abs(s.y))<engage) direction=None;
            else if(std::abs(s.y)>=std::abs(s.x)) direction=s.y<0?Up:Down;
            else direction=s.x<0?Left:Right;
        }
        return set(source,device,0,direction,now);
    }
    Action repeat(uint64_t now) {
        if(active==None || now<next) return None;
        next=now+intervalMs; // no catch-up bursts after a slow frame
        return active;
    }
    void clear() {held.clear();sticks.clear();active=None;next=0;}
};
