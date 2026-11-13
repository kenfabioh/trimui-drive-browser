#include "navigation.hpp"
#include <iostream>
#include <stdexcept>

static int checks=0;
static void check(bool ok,const char* label) {++checks;if(!ok) throw std::runtime_error(label);}
int main() {
    try {
        Navigation n;using N=Navigation;
        check(n.repeat(0)==None,"idle does not repeat");
        check(n.set(N::ControllerButton,1,12,Down,100)==Down,"first press immediate");
        check(n.repeat(449)==None,"initial delay");
        check(n.repeat(450)==Down,"first held repeat");
        check(n.repeat(539)==None && n.repeat(540)==Down,"90ms interval");
        check(n.repeat(9000)==Down && n.repeat(9000)==None,"no catch-up bursts");
        check(n.set(N::ControllerButton,1,12,None,9001)==None && n.repeat(10000)==None,"release stops");
        check(n.set(N::ControllerButton,1,11,Up,11000)==Up,"up immediate");
        check(n.set(N::ControllerButton,1,11,Up,11200)==None,"duplicate press ignored");
        check(n.repeat(11350)==Up,"duplicate press does not reset timer");
        check(n.set(N::ControllerButton,1,12,Down,11400)==Down,"new opposite direction wins");
        check(n.set(N::ControllerButton,1,12,None,11401)==Up,"release restores other held direction");
        n.clear();check(n.repeat(20000)==None,"clear stops all sources");
        check(n.axis(N::ControllerStick,2,1,11999,0)==None,"stick engage deadzone");
        check(n.axis(N::ControllerStick,2,1,18000,10)==Down,"left stick down");
        check(n.axis(N::ControllerStick,2,1,25000,300)==None && n.repeat(360)==Down,"axis motion does not restart delay");
        check(n.axis(N::ControllerStick,2,1,9000,361)==None && n.repeat(450)==Down,"release hysteresis preserves hold");
        check(n.axis(N::ControllerStick,2,1,7999,451)==None && n.repeat(900)==None,"neutral stops drift");
        check(n.axis(N::ControllerStick,2,1,9000,901)==None,"noise cannot reengage stick");
        check(n.axis(N::ControllerStick,2,1,-32768,902)==Up,"full negative axis safe");
        n.axis(N::ControllerStick,2,1,0,903);
        check(n.axis(N::ControllerStick,2,0,-20000,904)==Left,"left stick left");
        check(n.axis(N::ControllerStick,2,0,20000,905)==Right,"direction reversal immediate");
        check(n.axis(N::ControllerStick,2,1,21000,906)==None,"diagonal hysteresis avoids jitter");
        check(n.axis(N::ControllerStick,2,1,27000,907)==Down,"dominant axis changes once");
        check(n.axis(N::ControllerStick,2,3,-30000,908)==None && n.repeat(1257)==Down,"right stick ignored");
        n.clear();
        check(n.set(N::ControllerButton,1,12,Down,0)==Down,"D-pad first source");
        check(n.axis(N::ControllerStick,1,1,25000,100)==None && n.repeat(350)==Down,"pad/stick same direction not doubled");
        check(n.set(N::ControllerButton,1,12,None,351)==None && n.repeat(440)==Down,"stick continues when pad released");
        n.axis(N::ControllerStick,1,1,0,441);
        check(n.repeat(1000)==None,"both sources released");
        check(n.set(N::KeyboardKey,0,42,A,1100)==None && n.repeat(2000)==None,"action buttons never auto-repeat");
        check(n.set(N::JoystickHat,3,0,Down,3000)==Down && n.repeat(3350)==Down,"raw hat repeat");
        n.set(N::JoystickHat,3,0,None,3351);
        check(n.axis(N::JoystickStick,3,1,-25000,4000)==Up && n.repeat(4350)==Up,"raw left stick repeat");
        n.clear();
        check(n.set(N::KeyboardKey,0,1,Right,5000)==Right && n.repeat(5350)==Right,"keyboard shares navigation timer");
        std::cout<<"PASS navigation "<<checks<<" checks\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL navigation after "<<checks<<": "<<e.what()<<"\n";return 1;}
}
