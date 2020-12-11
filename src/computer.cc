#include "computer.h"

int main() {
    constexpr std::array<int,10> x = Computer<10, int>::boot<Program<
            TestOp<0, 3>,
            TestOp<0, 2>,
            Jz<Id("2")>,
            Js<Id("2")>,
            TestOp<0, 2>,
            TestOp<0, 2>,
            Jmp<Id("2")>,
            Label<Id("2")>,
            TestOp<8, 2137>>>();
    return 0;
}
