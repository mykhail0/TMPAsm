#include "computer.h"

int main() {
    Computer<10, int>::boot<Program<
            TestOp<0, 3>,
            Label<2>,
            TestOp<0, 2>,
            Jz<2>,
            Js<2>,
            Jmp<2>,
            TestOp<8, 2137>>>();
    return 0;
}
