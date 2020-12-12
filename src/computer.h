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
    constexpr int encode_char(char c) {
        if ('0' <= c && c <= '9')
            return static_cast<int>(c) - '0';
        if ('a' <= c && c <= 'z')
            return c - 'a' + 10;
        if ('A' <= c && c <= 'Z')
            return c - 'A' + 10;
        throw "Character out of range";
    }

    // Lea helper function, gets variable address whose Id is id.
    template<size_t memSize>
    constexpr size_t get_addr(uint64_t id, std::array<uint64_t, memSize> &addr, size_t sz) {
        for (size_t i = 0; i < sz; ++i) {
            if (addr[i] == id)
                return i;
        }
        return sz;
    }

    // Describes Computer state.
    template<typename memType, size_t N>
    struct Env {
        std::array<memType, N> memory{};
        // addresses[i] = id <-> id value is in memory[i]
        std::array<uint64_t, N> addresses{};
        bool ZF = false, SF = false;
        size_t variables_cnt = 0;

        // Tells if variable declarations are parsed.
        bool vars_loaded = false;

        // Updates flags after an arithmetic operation.
        constexpr void update_flags(memType val) {
            ZF = val == 0;
            SF = val < 0;
        }
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
    static_assert(std::is_integral<decltype(N)>::value, "Integral required.");
    template<typename memType, size_t memSize>
    constexpr static auto get([[maybe_unused]] Env<memType, memSize>& env) {
       return N;
    }
};

// Gets address of id Id.
template<uint64_t id>
struct Lea {
    template<typename memType, size_t memSize>
    constexpr static size_t get(Env<memType, memSize>& env) {
        return get_addr(id, env.addresses, env.variables_cnt);
    }
};

template<typename pvalue>
struct Mem {
    // Gets a pointer to assigned pvalue.
    template<typename memType, size_t memSize>
    constexpr static memType* get_pointer(Env<memType, memSize>& env){
        //std::cout << "MEM VAL: " << env.memory[pvalue::template get<memType, memSize>(env)] << std::endl;
        return &(env.memory[pvalue::template get<memType, memSize>(env)]);
    }

    // Gets a value of assigned pvalue.
    template<typename memType, size_t memSize>
    constexpr static memType get(Env<memType, memSize> &env) {
        return env.memory[pvalue::template get<memType, memSize>(env)];
    }
};

template<uint64_t id, typename T>
struct D {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute([[maybe_unused]] Env<memType, memSize> &env) {
        throw "Value is not Num.";
    }
};

template<uint64_t id, auto val>
struct D<id, Num<val>> {
    // Adds id to addresses array and assigns value to the according memory cell.
    // Does not execute if there is no free memory. (Executes if used in the expression
    // to evaluate some constexpr variable)
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize> &env) {
        assert(env.variables_cnt < memSize);
        // Redeclaration does not change addresses.
        if (get_addr<memSize>(id, env.addresses, env.variables_cnt) == env.variables_cnt) {
            env.addresses[env.variables_cnt] = id;
        }
        env.memory[env.variables_cnt++] = val;
    }
};

template<typename Lvalue, typename Pvalue>
struct Mov {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        (*Lvalue::template get_pointer<memType, memSize>(env)) = Pvalue:: template get<memType,memSize>(env);
    }
};

template<typename Lvalue, typename Pvalue>
struct Add {
    // Lvalue += Pvalue
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        memType pval = Pvalue::template get<memType, memSize>(env);
        *lval += pval;
        env.update_flags(*lval);
    }
};

template<typename Lvalue, typename Pvalue>
struct Sub {
    // Lvalue -= Pvalue
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        memType pval = Pvalue::template get<memType, memSize>(env);
        *lval -= pval;
        env.update_flags(*lval);
    }
};

template<typename Arg1, typename Arg2>
struct Cmp {
    // Same as Cmp but Arg1 does not change.
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType val = Arg1::template get<memType, memSize>(env) - Arg2::template get<memType, memSize>(env);
        env.update_flags(val);
    }
};

template<typename Lvalue>
struct Inc {
    // Increases Lvalue by one.
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        ++(*lval);
        env.update_flags(*lval);
    }
};

template<typename Lvalue>
struct Dec {
    // Decreases Lvalue by one.
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        --(*lval);
        env.update_flags(*lval);
    }
};

template<typename Lvalue, typename Pvalue>
struct And {
    // Lvalue &= Pvalue
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval &= Pvalue::template get<memType, memSize>(env);
        env.ZF = *lval == 0;
    }
};

template<typename Lvalue, typename Pvalue>
struct Or {
    // Lvalue |= Pvalue
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval |= Pvalue::template get<memType, memSize>(env);
        env.ZF = *lval == 0;
    }
};

template<typename Lvalue>
struct Not {
    // Lvalue ~= Lvalue
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        memType* lval = Lvalue::template get_pointer<memType, memSize>(env);
        *lval = ~(*lval);
        env.ZF = *lval == 0;
    }
};

// LABELS
template<typename Label, typename Program>
struct LabelHolder {
    using program = Program;
    using label = Label;
};

template<uint64_t Id>
struct Label {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute([[maybe_unused]] Env<memType, memSize>& env) {}

    static constexpr uint64_t id = Id;
};

// JUMPS
template<uint64_t Id>
struct Jmp {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        labels::template find_and_run<Id,memSize, memType, labels>(env);
    }

    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Jz {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        labels::template find_and_run<Id,memSize, memType, labels>(env);
    }

    static constexpr uint64_t id = Id;
};

template<uint64_t Id>
struct Js {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void execute(Env<memType, memSize>& env) {
        labels::template find_and_run<Id,memSize, memType, labels>(env);
    }

    static constexpr uint64_t id = Id;
};

//----------------HANDLING LABELS-------------------
template<typename... Labels>
struct LabelList;

// Create list

// recursion if label
template<template<typename...> class Program, typename... Ops, typename... Labels, uint64_t n>
struct LabelList<Program<Label<n>, Ops...>, Labels...> {
    using result =
    typename LabelList<Program<Ops...>, Labels...,
            LabelHolder<Label<n>, Program<Ops...>>>::result;
};

// recursion otherwise
template<template<typename...> class Program, typename... Ops, typename Op, typename... Labels>
struct LabelList<Program<Op, Ops...>, Labels...> {
    using result = typename LabelList<Program<Ops...>, Labels...>::result;
};

// base case if label
template<template<typename...> class Program, typename... Labels, uint64_t n>
struct LabelList<Program<Label<n>>, Labels...> {
    using result =
    LabelList<Program<>, Labels..., LabelHolder<Label<n>, Program<>>>;
};

// base case otherwise
template<template<typename...> class Program, typename Op, typename... Labels>
struct LabelList<Program<Op>, Labels...> {
    using result = LabelList<Program<>, Labels...>;
};

// Handle Jump operations

// recursion
template<template<typename...> class Program, typename Label, typename... Labels>
struct LabelList<Program<>, Label, Labels...> {
    template<uint64_t id, size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(Env<memType,memSize>& env) {
        // std::cout << Label::label::type << std::endl;
        if (Label::label::id == id) {
            Label::program::template run<memSize, memType, labels>(env);
        } else {
            LabelList<Program<>, Labels...>::template
            find_and_run<id, memSize, memType, labels>(env);
        }
    }
};

// base case
template<template<typename...> class Program, typename Label>
struct LabelList<Program<>, Label> {
    template<uint64_t id, size_t memSize, typename memType, typename labels>
    static constexpr void find_and_run(Env<memType, memSize>& env) {
        static_assert(Label::label::id == id, "Label doesn't exist");
        Label::program::template run<memSize, memType, labels>(env);
    }
};

//------------------PROGRAM----------------------
// Below are various Program specializations based on the instruction
// to be executed next.
template<typename... Ops>
struct Program {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize>& env);
};

template<typename Op, typename... Ops>
struct Program<Op, Ops...> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize> &env) {
        if (env.vars_loaded) {
            Op::template execute<memSize, memType, labels>(env);
            Program<Ops...>::template run<memSize, memType, labels>(env);
        } else {
            Program<Ops...>::template run<memSize, memType, labels>(env);
        }
    }
};

template<uint64_t id, typename... Ops>
struct Program<Jmp<id>, Ops...> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize> &env) {
        if (env.vars_loaded) {
            Jmp<id>::template execute<memSize, memType, labels>(env);
        } else {
            Program<Ops...>::template run<memSize, memType, labels>(env);
        }
    }
};

template<uint64_t id, typename... Ops>
struct Program<Jz<id>, Ops...> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize> &env) {
        if (env.vars_loaded && env.ZF) {
            Jz<id>::template execute<memSize, memType, labels>(env);
        } else {
            Program<Ops...>::template run<memSize, memType, labels>(env);
        }
    }
};

template<uint64_t id, typename... Ops>
struct Program<Js<id>, Ops...> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize> &env) {
        if (env.vars_loaded && env.SF) {
            Js<id>::template execute<memSize, memType, labels>(env);
        } else {
            Program<Ops...>::template run<memSize, memType, labels>(env);
        }
    }
};

template<uint64_t id, auto val, typename... Ops>
struct Program<D<id, Num<val>>, Ops...> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize>& env) {
        if (!env.vars_loaded) {
            D<id, Num<val>>::template execute<memSize, memType, labels>(env);
        }
        Program<Ops...>::template run<memSize, memType, labels>(env);
    }
};

template<uint64_t id>
struct Program<Jz<id>> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize>& env) {
        if (env.vars_loaded && env.ZF) {
            Jz<id>::template execute<memSize, memType, labels>(env);
        }
    }
};

template<uint64_t id>
struct Program<Js<id>> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize>& env) {
        if (env.vars_loaded && env.SF) {
            Js<id>::template execute<memSize, memType, labels>(env);
        }
    }
};

template<uint64_t id, auto val>
struct Program<D<id, Num<val>>> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize>& env) {
        if (!env.vars_loaded) {
            D<id, Num<val>>::template execute<memSize, memType, labels>(env);
        }
    }
};

template<typename Op>
struct Program<Op> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run(Env<memType, memSize>& env) {
        if (env.vars_loaded) {
            Op::template execute<memSize, memType, labels>(env);
        }
    }
};

//empty program
template<>
struct Program<> {
    template<size_t memSize, typename memType, typename labels>
    static constexpr void run([[maybe_unused]] Env<memType, memSize>& env) {}
};

template<size_t N, typename Type>
struct Computer {
    template<typename T>
    static constexpr std::array<Type, N> boot() {
        static_assert(std::is_integral<Type>::value, "Computer requires integral types.");

        Env<Type,N> env;

        // Parsing labels.
        using labels = typename LabelList<T>::result;

        // Loading variables.
        T::template run<N, Type, labels>(env);
        env.vars_loaded = true;

        // Executing the program.
        T::template run<N, Type, labels>(env);
        return env.memory;
    }
};

#endif // COMPUTER_H
