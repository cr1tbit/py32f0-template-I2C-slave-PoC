
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


// #define GPIOA 0

void HAL_GPIO_WritePin(uint8_t port, uint8_t pin, uint8_t state){
    printf("Setting %d to %s\n", pin, state ? "HI" : "LOW");
}
#include "../User/charlie.h"


void main(void){

    segment_t segments[4] = {
        {0, 1, 2, 3, 4, 5, 6},
        {7, 8, 9, 10, 11, 12, 13},
        {14, 15, 16, 17, 18, 19, 20},
        {21, 22, 23, 24, 25, 26, 27}
    };


    charlie_t charlie;
    charlie.pins = (uint8_t[]){0, 1, 2, 3, 4, 5};
    charlie.pinCount = 6;
    charlieInit(&charlie);

    // for (int i = 0; i < 10; i++){
    //     printf("Setting %d\n", i);
    //     setSevenSegment(&charlie, &segments[0], i);
    //     drawSevenSegment(&charlie, &segments[0]);
    // }
    for (int i = 0; i < 4; i++){
        setSevenSegment(&charlie, &segments[i], i);
        drawSevenSegment(&charlie, &segments[i]);
    }
    charliePrint(&charlie);
    charlieRender(&charlie);
}