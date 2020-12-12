#include "computer.h"

int main() {
//    constexpr std::array<int,10> x =
            Computer<10, int>::boot<Program<
//Computer<10, int>::boot<Program<
            D<Id("ABC"), Num<2137>>,
            //D<Id("ABC"), Num<2137>>,
            //D<Id("ABC"), Num<2137>>,
            TestOp<1, 2137>,
            Mov<Mem<Lea<Id("ABC")>>, Num<1>>,
            TestOp<0, 2>,
            Jz<Id("2")>,
            Js<Id("2")>,
            TestOp<0, 2>,
            TestOp<0, 2>,
            Jmp<Id("2")>,
            Label<Id("2")>,
            TestOp<8, 2137>>>();
//    printMemory<10,int>(x);
    return 0;

}
