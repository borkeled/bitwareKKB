#pragma once
#include <cstdint>
#include <cstring>
#include <array>
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>

namespace VM {

    enum Opcode : std::uint8_t {
        NOP,
        PUSH_IMM,
        PUSH_REG,
        POP_REG,
        ADD,
        SUB,
        MUL,
        DIV,
        XOR,
        AND,
        OR,
        SHL,
        SHR,
        CMP,
        JMP,
        JE,
        JNE,
        JG,
        JL,
        CALL,
        RET,
        MOV_REG,
        MOV_MEM_REG,
        MOV_REG_MEM,
        LOAD_EFLAGS,
        STORE_EFLAGS,
        EXIT,
        CUSTOM_READ,
        CUSTOM_WRITE,
        CUSTOM_CALL,
    };

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

        VMContext() : StackPtr(0), Bytecode(nullptr), BytecodeSize(0xFFFFFFFFFFFFFFFFULL), Ip(0), Running(false), UserData(nullptr) {
            memset(Registers, 0, sizeof(Registers));
            memset(Stack, 0, sizeof(Stack));
        }
    };

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

    template <typename UserFunc>
    inline void Execute(VMContext& Ctx, UserFunc UserFn) {
        Ctx.Running = true;

        while (Ctx.Running) {
            if (Ctx.Ip >= Ctx.BytecodeSize) {
                Ctx.Running = false;
                break;
            }
            auto op = static_cast<Opcode>(Ctx.Bytecode[Ctx.Ip++]);
            std::uint8_t dst, src, reg;

            switch (op) {
            case NOP:
                break;

            case PUSH_IMM: {
                if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                std::uint64_t val;
                memcpy(&val, &Ctx.Bytecode[Ctx.Ip], sizeof(val));
                Ctx.Ip += sizeof(val);
                Push(Ctx, val);
                break;
            }

            case PUSH_REG:
                if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                reg = Ctx.Bytecode[Ctx.Ip++];
                if (reg < REG_COUNT) Push(Ctx, Ctx.Registers[reg]);
                break;

            case POP_REG:
                if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                reg = Ctx.Bytecode[Ctx.Ip++];
                if (reg < REG_COUNT) Ctx.Registers[reg] = Pop(Ctx);
                break;

            case ADD: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, a + b);
                break;
            }

            case SUB: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, a - b);
                break;
            }

            case MUL: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, a * b);
                break;
            }

            case DIV: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                if (b != 0) Push(Ctx, a / b);
                else Push(Ctx, 0);
                break;
            }

            case XOR: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, a ^ b);
                break;
            }

            case AND: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, a & b);
                break;
            }

            case OR: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, a | b);
                break;
            }

            case SHL: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, b < 64 ? (a << b) : 0);
                break;
            }

            case SHR: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                Push(Ctx, b < 64 ? (a >> b) : 0);
                break;
            }

            case CMP: {
                auto b = Pop(Ctx);
                auto a = Pop(Ctx);
                if (a == b) Ctx.Registers[FLAGS] = 0;
                else if (a > b) Ctx.Registers[FLAGS] = 1;
                else Ctx.Registers[FLAGS] = 2;
                break;
            }

            case JMP: {
                if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                std::uint64_t addr;
                memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
                if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                Ctx.Ip = addr;
                break;
            }

            case JE:
                if (Ctx.Registers[FLAGS] == 0) {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    std::uint64_t addr;
                    memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
                    if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip = addr;
                } else {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip += sizeof(std::uint64_t);
                }
                break;

            case JNE:
                if (Ctx.Registers[FLAGS] != 0) {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    std::uint64_t addr;
                    memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
                    if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip = addr;
                } else {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip += sizeof(std::uint64_t);
                }
                break;

            case JG:
                if (Ctx.Registers[FLAGS] == 1) {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    std::uint64_t addr;
                    memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
                    if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip = addr;
                } else {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip += sizeof(std::uint64_t);
                }
                break;

            case JL:
                if (Ctx.Registers[FLAGS] == 2) {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    std::uint64_t addr;
                    memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
                    if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip = addr;
                } else {
                    if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                    Ctx.Ip += sizeof(std::uint64_t);
                }
                break;

            case CALL: {
                if (Ctx.Ip + sizeof(std::uint64_t) > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                std::uint64_t addr;
                memcpy(&addr, &Ctx.Bytecode[Ctx.Ip], sizeof(addr));
                Ctx.Ip += sizeof(addr);
                Push(Ctx, Ctx.Ip);
                if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                Ctx.Ip = addr;
                break;
            }

            case RET: {
                auto addr = Pop(Ctx);
                if (addr >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                Ctx.Ip = addr;
                break;
            }

            case MOV_REG:
                if (Ctx.Ip + 2 > Ctx.BytecodeSize) { Ctx.Running = false; break; }
                dst = Ctx.Bytecode[Ctx.Ip++];
                src = Ctx.Bytecode[Ctx.Ip++];
                if (dst < REG_COUNT && src < REG_COUNT)
                    Ctx.Registers[dst] = Ctx.Registers[src];
                break;

            case MOV_MEM_REG: {
                if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                dst = Ctx.Bytecode[Ctx.Ip++];
                if (dst < REG_COUNT) {
                    auto addr = Ctx.Registers[dst];
                    if (addr) {
#if defined(_MSC_VER)
                        __try {
                            *reinterpret_cast<std::uint64_t*>(addr) = Pop(Ctx);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            Ctx.Running = false;
                        }
#else
                        *reinterpret_cast<std::uint64_t*>(addr) = Pop(Ctx);
#endif
                    }
                }
                break;
            }

            case MOV_REG_MEM: {
                if (Ctx.Ip >= Ctx.BytecodeSize) { Ctx.Running = false; break; }
                dst = Ctx.Bytecode[Ctx.Ip++];
                if (dst < REG_COUNT) {
                    auto addr = Pop(Ctx);
                    if (addr) {
#if defined(_MSC_VER)
                        __try {
                            Ctx.Registers[dst] = *reinterpret_cast<std::uint64_t*>(addr);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            Ctx.Running = false;
                        }
#else
                        Ctx.Registers[dst] = *reinterpret_cast<std::uint64_t*>(addr);
#endif
                    }
                }
                break;
            }

            case CUSTOM_READ:
                UserFn(Ctx, CUSTOM_READ, Ctx.Bytecode, Ctx.Ip);
                break;

            case CUSTOM_WRITE:
                UserFn(Ctx, CUSTOM_WRITE, Ctx.Bytecode, Ctx.Ip);
                break;

            case CUSTOM_CALL:
                UserFn(Ctx, CUSTOM_CALL, Ctx.Bytecode, Ctx.Ip);
                break;

            case EXIT:
                Ctx.Running = false;
                break;

            default:
                Ctx.Running = false;
                break;
            }
        }
    }

    template <size_t N>
    struct BytecodeBuilder {
        std::array<std::uint8_t, N> Code;
        size_t Offset;

        BytecodeBuilder() : Offset(0) { Code.fill(0); }

        void EmitOp(Opcode op) {
            if (Offset < N) Code[Offset++] = static_cast<std::uint8_t>(op);
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
    };

    template <typename Builder>
    inline void EncryptBytecode(Builder& B, std::uint64_t key) {
        for (size_t i = 0; i < B.Offset; ++i) {
            B.Code[i] ^= static_cast<std::uint8_t>((key + i) & 0xFF);
        }
    }

    template <typename Builder>
    inline void DecryptBytecode(Builder& B, std::uint64_t key) {
        EncryptBytecode(B, key);
    }
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
#define VM_JMP(label) name.EmitOp(VM::JMP); name.Emit64(0)
#define VM_JE(label) name.EmitOp(VM::JE); name.Emit64(0)
#define VM_JNE(label) name.EmitOp(VM::JNE); name.Emit64(0)
#define VM_JG(label) name.EmitOp(VM::JG); name.Emit64(0)
#define VM_JL(label) name.EmitOp(VM::JL); name.Emit64(0)
#define VM_LABEL(label) label = name.GetOffset()
#define VM_CUSTOM_CALL name.EmitOp(VM::CUSTOM_CALL)
#define VM_EXIT name.EmitOp(VM::EXIT)
#define VM_NOP name.EmitOp(VM::NOP)
#define VM_RET name.EmitOp(VM::RET)
