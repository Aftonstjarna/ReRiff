#include "gameplay.hpp"
#include <cassert>

int main() {
    using namespace reriff;
    assert(trackVolume({},0,2,true)==0);
    assert(trackVolume({0,1},0,2,true)==float(std::sqrt(0.5)));
    assert(trackVolume({0.5f,0.5f},-1,2,true)==0.5f);
    assert(trackVolume({0.5f},2,10,true)==0);
    assert(trackVolume({2},0,1,true)==1);
    assert(trackVolume({0.5f},0,1,false)==float(std::pow(0.5,double(0.8f))));
    assert(gearTorque(true,5,100,10,20,30,true)==0);
    assert(gearTorque(false,0,100,10,20,30,true)==0);
    assert(gearTorque(false,5,100,10,20,30,true)==87.5f);
    assert(gearTorque(false,10,100,10,20,30,true)==100);
    assert(gearTorque(false,20,100,10,20,30,true)==100);
    assert(gearTorque(false,25,100,10,20,30,true)==50);
    assert(gearTorque(false,35,100,10,20,30,true)==0);
    assert(gearTorque(false,35,100,10,20,30,false)==100);
}
