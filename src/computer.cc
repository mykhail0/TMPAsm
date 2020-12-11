#include "computer.h"

int main() {
    Computer<10, int>::boot<Program<
            TestOp<0, 3>,
            Label<Id("2")>,
            TestOp<0, 2>,
            Jz<Id("2")>,
            Js<Id("2")>,
            Jmp<Id("2")>,
            TestOp<8, 2137>>>();
    return 0;
}
