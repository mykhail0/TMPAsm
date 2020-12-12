#include "computer.h"

int main() {
    //constexpr std::array<int,10> x =
            Computer<10, int>::boot<Program<
            D<Id("ABC"), Num<2137>>,
            D<Id("X"), Num<321>>,
            //D<Id("ABC"), Num<2137>>,
            //D<Id("ABC"), Num<2137>>,
            Mov<Mem<Lea<Id("ABC")>>, Num<1>>,
            Jz<Id("2")>,
            Js<Id("2")>,
            Jmp<Id("2")>,
            Label<Id("2")>,
            Not<Mem<Lea<Id("ABC")>>>,
            And<Mem<Lea<Id("X")>>,Num<2>>>>();
//    printMemory<10,int>(x);
    return 0;

}
