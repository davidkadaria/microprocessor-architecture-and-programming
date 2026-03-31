#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // for sleep

int IS_EXIT = 0;
int AAA = 0;

void Main_Decoder(void)
{
    int op;
    switch (op)
    {
    case 3:
        break;
    case 19:
        break;
    default:
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

    // for (int i = 0; i < 50; i++)
    // {

    // }
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
    const int NUM_THREADS = 3;
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
