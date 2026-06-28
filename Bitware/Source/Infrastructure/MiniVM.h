#pragma once
#include <cstdint>
#include <cstring>
#include <array>
#include <utility>
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace VM {

    enum Opcode : std::uint8_t {
        NOP, PUSH_IMM, PUSH_REG, POP_REG, ADD, SUB, MUL, DIV,
        XOR, AND, OR, SHL, SHR, CMP, JMP, JE, JNE, JG, JL,
        CALL, RET, MOV_REG, MOV_MEM_REG, MOV_REG_MEM,
        LOAD_EFLAGS, STORE_EFLAGS, EXIT,
        CUSTOM_READ, CUSTOM_WRITE, CUSTOM_CALL,
        OPCODE_COUNT
    };

    struct OpcodeMapping {
        std::uint8_t Encode[OPCODE_COUNT];
        std::uint8_t Decode[256];

        constexpr OpcodeMapping() : Encode{}, Decode{} {
            for (int i = 0; i < 256; ++i) Decode[i] = NOP;
            for (int i = 0; i < OPCODE_COUNT; ++i) {
                std::uint8_t enc = static_cast<std::uint8_t>((i * 53 + 0x17) % 256);
                Encode[i] = enc;
                Decode[enc] = static_cast<std::uint8_t>(i);
            }
        }
    };
    inline const OpcodeMapping OpMap;

    enum Reg : std::uint8_t {
        R0, R1, R2, R3, R4, R5, R6, R7,
        IP, SP, FP, FLAGS,
        REG_COUNT
    };

    struct VMContext {
        std::uint64_t Registers[REG_COUNT];
        std::uint64_t Stack[256];
        std::uint64_t StackPtr;
        const std::uint8_t* Bytecode;
        std::uint64_t BytecodeSize;
        std::uint64_t Ip;
        bool Running;
        void* UserData;
        std::array<std::uint64_t, 4096> Sandbox;

        VMContext() : StackPtr(0), Bytecode(nullptr), BytecodeSize(0xFFFFFFFFFFFFFFFFULL), Ip(0), Running(false), UserData(nullptr) {
            memset(Registers, 0, sizeof(Registers));
            memset(Stack, 0, sizeof(Stack));
            Sandbox.fill(0);
        }
    };

    using UserCallback = void(*)(VMContext&, int, const std::uint8_t*, std::uint64_t);

    inline void Push(VMContext& Ctx, std::uint64_t Val) {
        if (Ctx.StackPtr < 256) {
            Ctx.Stack[Ctx.StackPtr++] = Val;
        }
    }

    inline std::uint64_t Pop(VMContext& Ctx) {
        if (Ctx.StackPtr > 0) {
            return Ctx.Stack[--Ctx.StackPtr];
        }
        return 0;
    }

    static void HandleNop(VMContext&) {}
    static void HandlePushImm(VMContext& Ctx) {
        if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        std::uint64_t val;
        memcpy(&val, &Ctx.Bytecode[Ctx.Ip], sizeof(val));
        Ctx.Ip += sizeof(val);
        Push(Ctx, val);
    }
    static void HandlePushReg(VMContext& Ctx) {
        if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
        auto reg = Ctx.Bytecode[Ctx.Ip++];
        if (reg < REG_COUNT) Push(Ctx, Ctx.Registers[reg]);
    }
    static void HandlePopReg(VMContext& Ctx) {
        if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
        auto reg = Ctx.Bytecode[Ctx.Ip++];
        if (reg < REG_COUNT) Ctx.Registers[reg] = Pop(Ctx);
    }
    static void HandleAdd(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, a + b); }
    static void HandleSub(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, a - b); }
    static void HandleMul(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, a * b); }
    static void HandleDiv(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, b ? (a / b) : 0); }
    static void HandleXor(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, a ^ b); }
    static void HandleAnd(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, a & b); }
    static void HandleOr(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, a | b); }
    static void HandleShl(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, (b < 64) ? (a << b) : 0); }
    static void HandleShr(VMContext& Ctx) { auto b = Pop(Ctx), a = Pop(Ctx); Push(Ctx, (b < 64) ? (a >> b) : 0); }
    static void HandleCmp(VMContext& Ctx) {
        auto b = Pop(Ctx), a = Pop(Ctx);
        if (a == b) Ctx.Registers[FLAGS] = 0;
        else if (a > b) Ctx.Registers[FLAGS] = 1;
        else Ctx.Registers[FLAGS] = 2;
    }
    static void HandleJmp(VMContext& Ctx) {
        if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        std::uint64_t addr;
        memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
        if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
        Ctx.Ip = addr;
    }
    static void HandleJe(VMContext& Ctx) {
        if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        std::uint64_t addr;
        memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
        Ctx.Ip += sizeof(std::uint64_t);
        if (Ctx.Registers[FLAGS] == 0) {
            if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
            Ctx.Ip = addr;
        }
    }
    static void HandleJne(VMContext& Ctx) {
        if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        std::uint64_t addr;
        memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
        Ctx.Ip += sizeof(std::uint64_t);
        if (Ctx.Registers[FLAGS] != 0) {
            if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
            Ctx.Ip = addr;
        }
    }
    static void HandleJg(VMContext& Ctx) {
        if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        std::uint64_t addr;
        memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
        Ctx.Ip += sizeof(std::uint64_t);
        if (Ctx.Registers[FLAGS] == 1) {
            if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
            Ctx.Ip = addr;
        }
    }
    static void HandleJl(VMContext& Ctx) {
        if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        std::uint64_t addr;
        memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
        Ctx.Ip += sizeof(std::uint64_t);
        if (Ctx.Registers[FLAGS] == 2) {
            if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
            Ctx.Ip = addr;
        }
    }
    static void HandleCall(VMContext& Ctx) {
        if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        std::uint64_t addr;
        memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
        Ctx.Ip += sizeof(addr);
        Push(Ctx, Ctx.Ip);
        if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
        Ctx.Ip = addr;
    }
    static void HandleRet(VMContext& Ctx) {
        auto addr = Pop(Ctx);
        if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
        Ctx.Ip = addr;
    }
    static void HandleMovReg(VMContext& Ctx) {
        if (Ctx.Ip + 2 > Ctx.BytecodeSize) { Ctx.Running = false; return; }
        auto dst = Ctx.Bytecode[Ctx.Ip++];
        auto src = Ctx.Bytecode[Ctx.Ip++];
        if (dst < REG_COUNT && src < REG_COUNT)
            Ctx.Registers[dst] = Ctx.Registers[src];
    }
    static void HandleMovMemReg(VMContext& Ctx) {
        if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
        auto dst = Ctx.Bytecode[Ctx.Ip++];
        if (dst < REG_COUNT) {
            auto addr = Ctx.Registers[dst] & 0xFFF;
            Ctx.Sandbox[addr] = Pop(Ctx);
        }
    }
    static void HandleMovRegMem(VMContext& Ctx) {
        if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; return; }
        auto dst = Ctx.Bytecode[Ctx.Ip++];
        if (dst < REG_COUNT) {
            auto addr = Pop(Ctx) & 0xFFF;
            Ctx.Registers[dst] = Ctx.Sandbox[addr];
        }
    }
    static void HandleLoadEflags(VMContext& Ctx) { Push(Ctx, Ctx.Registers[FLAGS]); }
    static void HandleStoreEflags(VMContext& Ctx) { Ctx.Registers[FLAGS] = Pop(Ctx); }
    static void HandleExit(VMContext& Ctx) { Ctx.Running = false; }

    using HandlerFn = void(*)(VMContext&);
    inline constexpr HandlerFn Handlers[OPCODE_COUNT] = {
        HandleNop, HandlePushImm, HandlePushReg, HandlePopReg,
        HandleAdd, HandleSub, HandleMul, HandleDiv,
        HandleXor, HandleAnd, HandleOr, HandleShl, HandleShr,
        HandleCmp,
        HandleJmp, HandleJe, HandleJne, HandleJg, HandleJl,
        HandleCall, HandleRet, HandleMovReg, HandleMovMemReg, HandleMovRegMem,
        HandleLoadEflags, HandleStoreEflags, HandleExit,
        nullptr, nullptr, nullptr
    };

    template <typename UserFunc>
    inline void Execute(VMContext& Ctx, UserFunc UserFn) {
        Ctx.Running = true;

        while (Ctx.Running) {
            if (Ctx.Ip >= Ctx.BytecodeSize) {
                Ctx.Running = false;
                break;
            }
            auto encOp = Ctx.Bytecode[Ctx.Ip++];
            auto op = OpMap.Decode[encOp];

            if (op >= CUSTOM_READ && op <= CUSTOM_CALL) {
                UserFn(Ctx, op, Ctx.Bytecode, Ctx.Ip);
                continue;
            }

            if (op < OPCODE_COUNT && Handlers[op]) {
                Handlers[op](Ctx);
            } else {
                Ctx.Running = false;
            }
        }
    }

    template <size_t N>
    struct BytecodeBuilder {
        std::array<std::uint8_t, N> Code;
        size_t Offset;

        BytecodeBuilder() : Offset(0) { Code.fill(0); }

        void EmitOp(Opcode op) {
            if (Offset < N) Code[Offset++] = OpMap.Encode[static_cast<int>(op)];
        }

        void Emit64(std::uint64_t val) {
            if (Offset + sizeof(val) <= N) {
                memcpy(&Code[Offset], &val, sizeof(val));
                Offset += sizeof(val);
            }
        }

        void Emit8(std::uint8_t val) {
            if (Offset < N) Code[Offset++] = val;
        }

        void Patch64(size_t pos, std::uint64_t val) {
            if (pos + sizeof(val) <= N) {
                memcpy(&Code[pos], &val, sizeof(val));
            }
        }

        size_t GetOffset() const { return Offset; }

        const std::uint8_t* Data() const { return Code.data(); }

        void EncryptBytecode(std::uint64_t key) {
            std::uint64_t state = key;
            for (size_t i = 0; i < Offset; ++i) {
                state = state * 0x9E3779B97F4A7C15ULL + 0xBF58476D1CE4E5B9ULL;
                Code[i] ^= static_cast<std::uint8_t>(state >> ((i * 8) % 56));
            }
        }

        void DecryptBytecode(std::uint64_t key) {
            std::uint64_t state = key;
            for (size_t i = 0; i < Offset; ++i) {
                state = state * 0x9E3779B97F4A7C15ULL + 0xBF58476D1CE4E5B9ULL;
                Code[i] ^= static_cast<std::uint8_t>(state >> ((i * 8) % 56));
            }
        }
    };
}

#define VM_BYTECODE(name, size) \
    static VM::BytecodeBuilder<size> name##_Builder; \
    static bool name##_Built = false; \
    static auto& name = name##_Builder;

#define VM_PUSH(val) name.EmitOp(VM::PUSH_IMM); name.Emit64(static_cast<std::uint64_t>(val))
#define VM_PUSH_REG(r) name.EmitOp(VM::PUSH_REG); name.Emit8(VM::r)
#define VM_POP_REG(r) name.EmitOp(VM::POP_REG); name.Emit8(VM::r)
#define VM_ADD name.EmitOp(VM::ADD)
#define VM_SUB name.EmitOp(VM::SUB)
#define VM_MUL name.EmitOp(VM::MUL)
#define VM_XOR name.EmitOp(VM::XOR)
#define VM_AND name.EmitOp(VM::AND)
#define VM_OR name.EmitOp(VM::OR)
#define VM_CMP name.EmitOp(VM::CMP)
#define VM_JMP_VAR(label) size_t label##_patch
#define VM_JMP(label) name.EmitOp(VM::JMP); label##_patch = name.GetOffset(); name.Emit64(0)
#define VM_JE(label) name.EmitOp(VM::JE); label##_patch = name.GetOffset(); name.Emit64(0)
#define VM_JNE(label) name.EmitOp(VM::JNE); label##_patch = name.GetOffset(); name.Emit64(0)
#define VM_JG(label) name.EmitOp(VM::JG); label##_patch = name.GetOffset(); name.Emit64(0)
#define VM_JL(label) name.EmitOp(VM::JL); label##_patch = name.GetOffset(); name.Emit64(0)
#define VM_LABEL(label) const size_t label = name.GetOffset()
#define VM_PATCH_JMP(label) name.Patch64(label##_patch, label)
#define VM_CUSTOM_CALL name.EmitOp(VM::CUSTOM_CALL)
#define VM_EXIT name.EmitOp(VM::EXIT)
#define VM_NOP name.EmitOp(VM::NOP)
#define VM_RET name.EmitOp(VM::RET)
