#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// #include "py32f0xx_bsp_printf.h"


typedef struct {
    uint16_t* pins;
    uint8_t pinCount;

    uint8_t* matrixBuf;
} charlie_t;

typedef struct {
    uint8_t segmentNumber[7];
} segment_t;

bool isSegmentActiveForNumber(int segmentNum, int number){
    switch (number){
        case 0:
            return segmentNum != 3;
        case 1:
            return segmentNum == 5 || segmentNum == 2;
        case 2:
            return segmentNum != 1 && segmentNum != 5;
        case 3:
            return segmentNum != 1 && segmentNum != 4;
        case 4:
            return segmentNum == 1 || segmentNum == 2 || segmentNum == 3 || segmentNum == 5;
        case 5:
            return segmentNum != 2 && segmentNum != 4;
        case 6:
            return segmentNum != 2;
        case 7:
            return segmentNum == 0 || segmentNum == 2 || segmentNum == 5;
        case 8:
            return true;
        case 9:
            return segmentNum != 4;
    }
    return false;
}

void drawSevenSegment(charlie_t* charlie, segment_t* segment){
    // charlie->matrixBuf[segment->segmentNumber[i]] = 1;
    printf(" %c \n", charlie->matrixBuf[segment->segmentNumber[0]] ? '=' : ' ');

    printf("%c %c\n",
        charlie->matrixBuf[segment->segmentNumber[1]] ? '|' : ' ',
        charlie->matrixBuf[segment->segmentNumber[2]] ? '|' : ' '
    );
    printf(" %c \n", charlie->matrixBuf[segment->segmentNumber[3]] ? '=' : ' ');

    printf("%c %c\n",
        charlie->matrixBuf[segment->segmentNumber[4]] ? '|' : ' ',
        charlie->matrixBuf[segment->segmentNumber[5]] ? '|' : ' '
    );
    printf(" %c \n", charlie->matrixBuf[segment->segmentNumber[6]] ? '=' : ' ');
}

void setSevenSegment(charlie_t* charlie, segment_t* segment, int value){
    for (int i = 0; i < 7; i++){
        charlie->matrixBuf[segment->segmentNumber[i]] = isSegmentActiveForNumber(i, value);
    }
}


int charlieGetBufSize(charlie_t* charlie){
    return charlie->pinCount * (charlie->pinCount -1);
}


void charlieClear(charlie_t* charlie){
    int bufSize = charlieGetBufSize(charlie);
    for (int i = 0; i < bufSize; i++){
        charlie->matrixBuf[i] = 0;
    }
}

void charlieInit(charlie_t* charlie){
    int bufSize = charlieGetBufSize(charlie);
    charlie->matrixBuf = malloc(bufSize * sizeof(uint8_t));
    charlieClear(charlie);
}


void charlieSetPixel(charlie_t* charlie, int x, int y, int value){
    charlie->matrixBuf[x * charlie->pinCount + y] = value;
}

void charlieSet(charlie_t* charlie, segment_t* segment){
    for (int i = 0; i < 7; i++){
        charlie->matrixBuf[segment->segmentNumber[i]] = 1;
    }
}

void charliePrint(charlie_t* charlie){
    int pinCount = charlie->pinCount;
    for (int i = 0; i < pinCount-1; i++){
        for (int j = 0; j < (pinCount); j++){
            printf("%d " , charlie->matrixBuf[i * pinCount + j]);
        }
        printf("\n\r");
    }
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 


void charlieRender(charlie_t* charlie, int debug){
    int offs = 0;

    for (int i = 0; i < charlie->pinCount; i++){
        int pinMaskHi = 0;
        int pinMaskZ = 0;

        // printf("\n");
        if (debug) printf("\n\rpin %d (%d)\n\r", i, charlie->pins[i]);
        for (int j = 0; j < charlie->pinCount; j++){
            if (i == j){
                offs++;
            } else {
                if (charlie->matrixBuf[i * charlie->pinCount + j - offs]){
                    pinMaskHi |= charlie->pins[j];
                } else {
                    pinMaskZ |= charlie->pins[j];
                }
            }
        }

        GPIO_InitTypeDef GPIO_InitStruct;

        GPIO_InitStruct.Pin = pinMaskZ;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        if (debug) printf("ZZ "BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN"\n\r",
            BYTE_TO_BINARY(pinMaskZ>>8),
            BYTE_TO_BINARY(pinMaskZ)
        );

        GPIO_InitStruct.Pin = pinMaskHi | charlie->pins[i];
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        if (debug) printf("HI "BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN"\n\r",
            BYTE_TO_BINARY(pinMaskHi>>8),
            BYTE_TO_BINARY(pinMaskHi)
        );
        HAL_GPIO_WritePin(GPIOA, pinMaskHi, 1);
        
        if (debug) printf("LO "BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN"\n\r",
            BYTE_TO_BINARY(charlie->pins[i]>>8),
            BYTE_TO_BINARY(charlie->pins[i])
        );
        HAL_GPIO_WritePin(GPIOA, charlie->pins[i], 0);


        if (debug) HAL_Delay(2000);
        HAL_GPIO_WritePin(GPIOA, charlie->pins[i], 1);
    }
    if (debug) printf("\n");
}