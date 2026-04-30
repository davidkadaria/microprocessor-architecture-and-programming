#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h> // for sleep

int IS_EXIT = 0;
int AAA = 0;

// Global state variables
uint32_t RegWrite   = 0;
uint32_t ImmSrc     = 0;
uint32_t ALUSrc     = 0;
uint32_t MemWrite   = 0;
uint32_t ResultSrc  = 0;
uint32_t Branch     = 0;
uint32_t ALUOp      = 0;
uint32_t ALUControl = 0;


uint32_t Extract_Bits(uint32_t input, uint32_t from, uint32_t to)
{
    uint32_t width = to - from + 1;
    uint32_t mask = (width == 32) ? 0xFFFFFFFFU : ((1U << width) - 1);

    return (input >> from) & mask;
}

int32_t Extend_Imm(uint32_t instruction, uint8_t immSrc)
{
    switch (immSrc)
    {
        case 0: // IMM_I
        {
            return ((int32_t)instruction) >> 20;
        }
        case 1:
        {
            int32_t imm =
                (Extract_Bits(instruction, 7, 11)) |
                (Extract_Bits(instruction, 25, 31) << 5);
        
            // sign extend
            if (imm & (1 << 11))
                imm |= 0xFFFFF000;
        
            return imm;
        }
        case 2:
        {
            int32_t imm =
                (Extract_Bits(instruction, 8, 11) << 1) |
                (Extract_Bits(instruction, 25, 30) << 5) |
                (Extract_Bits(instruction, 7, 7) << 11) |
                (Extract_Bits(instruction, 31, 31) << 12);
        
            // sign extend
            if (imm & (1 << 12))
                imm |= 0xFFFFE000;
        
            return imm;
        }
        default:
            return 0;
    }
}

int32_t ALU(int32_t A, int32_t B, uint32_t ALUControl)
{
    switch (ALUControl)
    {
        case 0: return A + B;
        case 1: return A - B;
        case 2: return A & B;
        case 3: return A | B;
        case 4: return (A < B) ? 1 : 0;
        default: return 0;
    }
}

void ALU_Decoder(uint32_t ALUOp, uint32_t funct3, uint32_t funct7)
{
    switch (ALUOp)
    {
        case 0: // lw, sw: ADD
            ALUControl = 0;
            break;

        case 1: // beq: SUB
            ALUControl = 1;
            break;

        case 2: // R-type / addi
            switch (funct3)
            {
                case 0x0:
                    if (funct7 == 0x20)
                        ALUControl = 1; // SUB (R-type only)
                    else
                        ALUControl = 0; // ADD / ADDI
                    break;
                    
                case 0x2:
                    ALUControl = 4; // SLT
                    break;
                    
                case 0x6:
                    ALUControl = 3; // OR
                    break;

                case 0x7:
                    ALUControl = 2; // AND
                    break;

                default:
                    ALUControl = 0;
                    break;
            }
            break;

        default:
            ALUControl = 0;
            break;
    }
}

void Main_Decoder(uint32_t instruction)
{
    uint32_t opcode = Extract_Bits(instruction, 0, 6);
    
    // reset all signals
    RegWrite  = 0;
    ImmSrc    = 0;
    ALUSrc    = 0;
    MemWrite  = 0;
    ResultSrc = 0;
    Branch    = 0;
    ALUOp     = 0;
    
    switch (opcode)
    {
        case 0x03: // lw (I-type)
            RegWrite  = 1;
            ALUSrc    = 1;
            ResultSrc = 1;
            ALUOp     = 0;
            ImmSrc    = 0;
            break;
    
        case 0x23: // sw (S-type)
            ALUSrc    = 1;
            MemWrite  = 1;
            ALUOp     = 0;
            ImmSrc    = 1;
            break;
    
        case 0x63: // beq (B-type)
            Branch    = 1;
            ALUOp     = 1;
            ImmSrc    = 2;
            break;
    
        case 0x33: // R-type
            RegWrite  = 1;
            ALUSrc    = 0;
            ALUOp     = 2;
            break;
    
        case 0x13: // addi (I-type ALU)
            RegWrite  = 1;
            ALUSrc    = 1;
            ALUOp     = 2;
            ImmSrc    = 0;
            break;
    }
}

//-------------------------------------------//
//
//-------------------------------------------//
void* SOFT_CPU_THREAD()
{
    uint32_t PC = 0;
    uint32_t Instruction_Memory[0x2000];
    uint32_t Data_Memory[0x3000];
    uint32_t Register_File[32];
    
    for (int i = 0; i < 32; i++)
        Register_File[i] = 0;
    
    for (int i = 0; i < 0x3000; i++)
        Data_Memory[i] = 0;
    printf("Hello from SOFT_CPU_THREAD\n");

    // prerequisites start
    PC = 0x1000;
    Instruction_Memory[0x1000] = 0xFFC4A303;
    Instruction_Memory[0x1004] = 0x0064A423;
    Instruction_Memory[0x1008] = 0x0062E233;
    Instruction_Memory[0x100C] = 0xFE420AE3;
    Data_Memory[0x2000] = 10;
    Register_File[5] = 6;
    Register_File[9] = 0x2004;
    // prerequisites stop
    
    int steps = 0;
    
    while (!IS_EXIT)
    {
        uint32_t instruction = Instruction_Memory[PC];
    
        uint32_t rs1 = Extract_Bits(instruction, 15, 19);
        uint32_t rs2 = Extract_Bits(instruction, 20, 24);
        uint32_t rd  = Extract_Bits(instruction, 7, 11);
        uint32_t funct3 = Extract_Bits(instruction, 12, 14);
        uint32_t funct7 = Extract_Bits(instruction, 25, 31);
    
        Main_Decoder(instruction);
    
        int32_t imm = Extend_Imm(instruction, ImmSrc);
    
        ALU_Decoder(ALUOp, funct3, funct7);
    
        int32_t SrcA = Register_File[rs1];
        int32_t SrcB = (ALUSrc) ? imm : Register_File[rs2];
    
        int32_t alu_result = ALU(SrcA, SrcB, ALUControl);
        
        if (MemWrite)
        {
            uint32_t addr = alu_result >> 2; // byte → word address
        
            if (addr < 0x3000)
            {
                Data_Memory[addr] = Register_File[rs2];
                printf("Memory[0x%X] = %d\n", alu_result, Register_File[rs2]);
            }
        }
        
        if (RegWrite && rd != 0)
        {
            if (ResultSrc == 1) // lw
            {
                uint32_t addr = alu_result >> 2;
        
                if (addr < 0x3000)
                    Register_File[rd] = Data_Memory[addr];
            }
            else // ALU result
            {
                Register_File[rd] = alu_result;
            }
        }
    
        printf("\nPC = 0x%X\n", PC);
        printf("Instruction: 0x%08X\n", instruction);
        printf("ALU Result = %d (0x%X)\n", alu_result, alu_result);
    
        // --- PC UPDATE ---
        if (Branch && alu_result == 0)
            PC = PC + imm;
        else
            PC = PC + 4;
            
        if (steps++ > 20)
        {
            printf("Stopped after 20 steps\n");
            break;
        }
    
        sleep(1); // slow down execution
    }
    
    return NULL;
}

//-------------------------------------------//
//
//-------------------------------------------//
void* TASK_2(void* arg)
{
    int id = *(int*)arg;
    printf("Hello from TASK_2 %d!\n", id);
    return NULL;
}

//-------------------------------------------//
//
//-------------------------------------------//
void* TASK_keyboard()
{
    printf("Hello from TASK_keyboard\n");
    char keyboard_DATA[50];
    for (int i = 0; i < 500; i++)
    {
        scanf("%s", keyboard_DATA);
        if(keyboard_DATA[0]=='q'){IS_EXIT=1;return NULL;}
    }
    return NULL;
}

int main()
{
    const int NUM_THREADS = 1;
    pthread_t threads[NUM_THREADS];
    //---------------------------------------------
    if (pthread_create(&threads[0], NULL, SOFT_CPU_THREAD, NULL) != 0)
    {perror("Failed to create thread"); return 1;}
    //---------------------------------------------
    // int id=888;
    // if (pthread_create(&threads[1], NULL, TASK_2, &id) != 0)
    // { perror("Failed to create thread");return 1;}
    // //---------------------------------------------
    // if (pthread_create(&threads[2], NULL, TASK_keyboard, NULL) != 0)
    // { perror("Failed to create thread");return 1;}
    //---------------------------------------------
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);printf("XXXXX \n");
    }
    printf("All threads finished!\n");
    return 0;
}
