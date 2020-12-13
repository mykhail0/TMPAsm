#ifndef COMPUTER_H
#define COMPUTER_H

#include <array>
#include <utility>
#include <iostream>
#include <cassert>
#include <type_traits>

namespace {
    constexpr unsigned MAX_ID_LEN = 6;

    // [0..9], [a-z]
    constexpr int ALLOWED_CHAR_CNT = ('z' - 'a' + 1) + ('9' - '0' + 1);

    // Id helper function, encodes a single character.
    // No character maps to 0.
    constexpr int encode_char(char c) {
        if ('0' <= c && c <= '9')
            return static_cast<int>(c) - '0' + 1;
        if ('a' <= c && c <= 'z')
            return c - 'a' + 11;
        if ('A' <= c && c <= 'Z')
            return c - 'A' + 11;
        throw "Character out of range";
    }

/*
    template<size_t memSize, size_t sz, size_t var_count, uint64_t elem, uint64_t id, bool declaration>
    constexpr size_t get_addr(std::array<uint64_t, memSize> &addr) {
        if constexpr (elem == id)
            return var_count;
        else if constexpr (var_count + 1 < sz) {
            return get_addr<memSize, sz, var_count + 1, addr[var_count + 1], declaration>(addr);
        } else {
            static_assert(declaration);
            return var_count;
        }
    }
*/

    // struct D helper function, gets variable address whose Id is id.
    // Otherwise returns size.
    template<size_t memSize>
    constexpr size_t get_addr(uint64_t id, std::array<uint64_t, memSize> &addr, size_t sz) {
        for (size_t i = 0; i < sz; ++i) {
            if (addr[i] == id)
                return i;
        }
        return sz;
    }

    // Lea helper function, gets variable address whose Id is id.
    // Otherwise gets error because out of bounds.
    template<size_t memSize>
    constexpr size_t get_addr(uint64_t id, std::array<uint64_t, memSize> &addr) {
        size_t i = 0;
        while (true) {
            if (addr[i] == id)
                return i;
            i++;
        }
        //return sz;
    }

    // Describes Computer state.
    template<typename memType, size_t N>
    struct Env {
        std::array<memType, N> memory{};
        // addresses[i] = id <-> id value is in memory[i]
        std::array<uint64_t, N> addresses{};
        bool ZF = false, SF = false;
        // There could be less than N variables.
        size_t variables_cnt = 0;

        // Tells if variable declarations are parsed.
        bool vars_loaded = false;

        // Updates flags after an arithmetic operation.
        constexpr void update_flags(memType val) {
            ZF = val == 0;
            SF = val < 0;
        }
    };

    enum OpType {
        LABEL, JMP, JZ, JS, DECL, LEA, MEM, NUM, MOV,
        AND, OR, NOT, ADD, SUB, INC, DEC, CMP
    };
} // anonymous namespace

//---------------DEBUG----------------------
template<size_t N, typename Type>
inline void printMemory(const std::array<Type, N> &mem) {
    std::cout << "-----MEMORY STATE----" << std::endl;
    for (size_t i = 0; i < N; i++) {
        std::cout << mem[i] << std::endl;
    }
    std::cout << "---------" << std::endl;
}

template<size_t N>
inline void printAddr(std::array<uint64_t, N> &mem) {
    std::cout << "-----ADDRESS STATE----" << std::endl;
    for (size_t i = 0; i < N; i++) {
        std::cout << mem[i] << std::endl;
    }
    std::cout << "---------" << std::endl;
}


//-------------OPERATIONS---------------------------
// Encodes string for id as an integer.
constexpr uint64_t Id(const char s[]) {
    uint64_t ans = 0, base_power = 1;
    unsigned char_cnt = 0;

    while (s[char_cnt] != '\0') {
        int char_val = encode_char(s[char_cnt]);
        ans += char_val * base_power;

        char_cnt++;
        base_power *= ALLOWED_CHAR_CNT;
        assert(char_cnt <= MAX_ID_LEN);
    }
    assert(char_cnt != 0);
    return ans;
}

template<auto N>
struct Num {
    static constexpr OpType type = NUM;
    static_assert(std::is_integral<decltype(N)>::value, "Integral required.");

    template<typename memType, size_t memSize>
    constexpr static auto get(Env<memType, memSize> &) {
        return N;
    }
    constexpr static void check_pvalue() {}
};

// Gets address of id Id.
template<uint64_t id>
struct Lea {
    static constexpr OpType type = LEA;

    template<typename memType, size_t memSize>
    constexpr static size_t get(Env<memType, memSize> &env) {
        return get_addr<memSize>(id, env.addresses);
    }
    constexpr static void check_pvalue() {}
};

template<typename pvalue>
struct Mem {
    static constexpr OpType type = MEM;

    // Gets a pointer to assigned pvalue.
    template<typename memType, size_t memSize>
    constexpr static memType* get_pointer(Env<memType, memSize> &env) {
        return &(env.memory[pvalue::template get<memType, memSize>(env)]);
    }

    // Gets a value of assigned pvalue.
    template<typename memType, size_t memSize>
    constexpr static memType get(Env<memType, memSize> &env) {
        return env.memory[pvalue::template get<memType, memSize>(env)];
    }
    constexpr static void check_lvalue() {
        pvalue::check_pvalue();
    }
    constexpr static void check_pvalue() {
        pvalue::check_pvalue();
    }
};

template<uint64_t id, typename T>
struct D {
    static constexpr OpType type = DECL;
    // Declaration with types different from Num is not valid.
    static constexpr bool valid = false;
};

template<uint64_t id, auto val>
struct D<id, Num<val>> {
    static constexpr OpType type = DECL;

    // Only declaration with Num is valid
    static constexpr bool valid = true;

    // Adds id to addresses array and assigns value to the according memory cell.
    // Does not execute if there is no free memory.
    template<size_t memSize, typename memType, size_t var_count>
    static constexpr void load_variable(Env<memType, memSize>& env) {
        static_assert(var_count < memSize);
        if (get_addr<memSize>(id, env.addresses, var_count) == var_count)
            env.addresses[var_count] = id;
        env.memory[var_count] = static_cast<memType>(val);
    }
    constexpr static void check() {}
};

template<typename Lvalue, typename Pvalue>
struct Mov {
    static constexpr OpType type = MOV;

    // Lvalue = Pvalue
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        (*Lvalue::template get_pointer<memType, memSize>(env)) = static_cast<memType>(Pvalue::template get<memType, memSize>(env));
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
        Pvalue::check_pvalue();
    }
};

template<typename Lvalue, typename Pvalue>
struct Add {
    static constexpr OpType type = ADD;

    // Lvalue += Pvalue
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        auto pval = static_cast<memType>(Pvalue::template get<memType, memSize>(env));
        *lval += pval;
        env.update_flags(*lval);
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
        Pvalue::check_pvalue();
    }
};

template<typename Lvalue, typename Pvalue>
struct Sub {
    static constexpr OpType type = SUB;

    // Lvalue -= Pvalue
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        auto pval = static_cast<memType>(Pvalue::template get<memType, memSize>(env));
        *lval -= pval;
        env.update_flags(*lval);
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
        Pvalue::check_pvalue();
    }
};

template<typename Arg1, typename Arg2>
struct Cmp {
    static constexpr OpType type = CMP;

    // Same as Cmp but Arg1 does not change.
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        auto val = static_cast<memType>(Arg1::template get<memType, memSize>(env)) - static_cast<memType>(Arg2::template get<memType, memSize>(env));
        env.update_flags(val);
    }
    constexpr static void check() {
        Arg1::check_pvalue();
        Arg2::check_pvalue();
    }
};

template<typename Lvalue>
struct Inc {
    static constexpr OpType type = INC;

    // Increases Lvalue by one.
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval += 1;
        env.update_flags(*lval);
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
    }
};

template<typename Lvalue>
struct Dec {
    static constexpr OpType type = DEC;

    // Decreases Lvalue by one.
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval -= 1;
        env.update_flags(*lval);
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
    }
};

template<typename Lvalue, typename Pvalue>
struct And {
    static constexpr OpType type = AND;

    // Lvalue &= Pvalue
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval &= static_cast<memType>(Pvalue::template get<memType, memSize>(env));
        env.ZF = *lval == 0;
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
        Pvalue::check_pvalue();
    }
};

template<typename Lvalue, typename Pvalue>
struct Or {
    static constexpr OpType type = OR;

    // Lvalue |= Pvalue
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval |= static_cast<memType>(Pvalue::template get<memType, memSize>(env));
        env.ZF = *lval == 0;
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
        Pvalue::check_pvalue();
    }
};

template<typename Lvalue>
struct Not {
    static constexpr OpType type = NOT;

    // Lvalue ~= Lvalue
    template<size_t memSize, typename memType>
    static constexpr void execute(Env<memType, memSize> &env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval = ~(*lval);
        env.ZF = *lval == 0;
    }
    constexpr static void check() {
        Lvalue::check_lvalue();
    }
};

template<uint64_t Id>
struct Label {
    static constexpr OpType type = LABEL;
    static constexpr uint64_t id = Id;
    constexpr static void check() {}
};

// JUMPS
template<uint64_t Id>
struct Jmp {
    static constexpr OpType type = JMP;

    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize> &env) {
        labels::template find_and_run<Id, memSize, memType, labels>(env);
    }
    constexpr static void check() {}
};

template<uint64_t Id>
struct Jz {
    static constexpr OpType type = JZ;

    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize> &env) {
        labels::template find_and_run<Id, memSize, memType, labels>(env);
    }
    constexpr static void check() {}
};

template<uint64_t Id>
struct Js {
    static constexpr OpType type = JS;

    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize> &env) {
        labels::template find_and_run<Id, memSize, memType, labels>(env);
    }
    constexpr static void check() {}
};

//----------------HANDLING LABELS-------------------

template<typename Label, typename Program>
struct LabelHolder {
    using program = Program;
    using label = Label;
};

template<typename... Labels>
struct LabelList;

// Create list

// Recursion if label
template<template<typename...> class Program, typename... Ops, typename... Labels, uint64_t n>
struct LabelList<Program<Label<n>, Ops...>, Labels...> {
    using result =
    typename LabelList<Program<Ops...>, Labels...,
            LabelHolder<Label<n>, Program<Ops...>>>::result;
};

// Recursion otherwise
template<template<typename...> class Program, typename... Ops, typename Op, typename... Labels>
struct LabelList<Program<Op, Ops...>, Labels...> {
    using result = typename LabelList<Program<Ops...>, Labels...>::result;
};

// Base case if label
template<template<typename...> class Program, typename... Labels, uint64_t n>
struct LabelList<Program<Label<n>>, Labels...> {
    using result =
    LabelList<Program<>, Labels..., LabelHolder<Label<n>, Program<>>>;
};

// Base case otherwise
template<template<typename...> class Program, typename Op, typename... Labels>
struct LabelList<Program<Op>, Labels...> {
    using result = LabelList<Program<>, Labels...>;
};

// Empty program
template<template<typename...> class Program>
struct LabelList<Program<>> {
    template<uint64_t id, size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(Env<memType, memSize> &) {}

    using result = LabelList<Program<>>;
};

// Handle Jump operations

// Recursion
template<template<typename...> class Program, typename Label, typename... Labels>
struct LabelList<Program<>, Label, Labels...> {
    template<uint64_t id, size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(Env<memType, memSize> &env) {
        if constexpr (Label::label::id == id) {
            Label::program::template run<memSize, memType, labels>(env);
        } else {
            LabelList<Program<>, Labels...>::template
            find_and_run<id, memSize, memType, labels>(env);
        }
    }
};

// Base case
template<template<typename...> class Program, typename Label>
struct LabelList<Program<>, Label> {
    template<uint64_t id, size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(Env<memType, memSize>& env) {
        static_assert(Label::label::id == id, "Label doesn't exist");
        Label::program::template run<memSize, memType, labels>(env);
    }
};

//------------------PROGRAM----------------------
template<typename... Ops>
struct Program;

// Recursion
template<typename Op, typename... Ops>
struct Program<Op, Ops...> {

    static constexpr void check_program() {
        Op::check();
        Program<Ops...>::check_program();
    }

    template<size_t memSize, typename memType, size_t var_count>
    static constexpr void load_variables(Env<memType,memSize>& env) {
        if constexpr (Op::type == DECL) {
            // Error if declaration doesn't have Num as argument.
            static_assert(Op::valid);
            Op::template load_variable<memSize ,memType, var_count>(env);
            //There is now one more variable, so execute next call with var_count + 1.
            Program<Ops...>::template load_variables<memSize, memType, var_count+1>(env);
        } else {
            Program<Ops...>::template load_variables<memSize, memType, var_count>(env);
        }
    }

    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize> &env) {
        if constexpr(Op::type == JMP) {
            // Jump and then don't execute current program again.
            Op::template execute<memSize, memType, labels>(env);
        } else if constexpr(Op::type == JZ) {
            // Jump or execute current program.
            if (env.ZF) {
                Op::template execute<memSize, memType, labels>(env);
            } else {
                Program<Ops...>::template run<memSize, memType, labels>(env);
            }
        } else if constexpr(Op::type == JS) {
            // Jump or execute current program.
            if (env.SF) {
                Op::template execute<memSize, memType, labels>(env);
            } else {
                Program<Ops...>::template run<memSize, memType, labels>(env);
            }
        } else if constexpr(Op::type == DECL || Op::type == LABEL) {
            // Skip these instructions.
            Program<Ops...>::template run<memSize, memType, labels>(env);
        } else {
            Op::template execute<memSize, memType>(env);
            Program<Ops...>::template run<memSize, memType, labels>(env);
        }
    }
};

// Base case
template<typename Op>
struct Program<Op> {

    static constexpr void check_program() {
        Op::check();
    }

    template<size_t memSize, typename memType, size_t var_count>
    static constexpr void load_variables(Env<memType, memSize>& env) {
        if constexpr (Op::type == DECL) {
            static_assert(Op::valid);
            Op::template load_variable<memSize, memType, var_count>(env);
        }
    }

    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize> &env) {
        if constexpr(Op::type == JZ) {
            if (env.ZF) {
                Op::template execute<memSize, memType, labels>(env);
            }
        } else if constexpr(Op::type == JS) {
            if (env.SF) {
                Op::template execute<memSize, memType, labels>(env);
            }
        } else if constexpr(Op::type != DECL && Op::type != LABEL) {
            Op::template execute<memSize, memType>(env);
        }
    }
};

// Empty program
template<>
struct Program<> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize> &) {}

    template<size_t memSize, typename memType, size_t var_count>
    static constexpr void load_variables(Env<memType, memSize>&){};
};

template<size_t N, typename Type>
struct Computer {
    template<typename T>
    static constexpr std::array<Type, N> boot() {
        static_assert(std::is_integral<Type>::value, "Computer requires integral types.");

        Env<Type, N> env;
        //check syntax
        T::check_program();

        // Parsing labels.
        using labels = typename LabelList<T>::result;

        // Loading variables.
        T::template load_variables<N, Type, 0>(env);

        // Executing the program.
        T::template run<N, Type, labels>(env);
        return env.memory;
    }
};

#endif // COMPUTER_H
