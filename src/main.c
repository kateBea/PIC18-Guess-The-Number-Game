#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"

/*******************************************************************************/
/*                                 DEFINES                                     */
/*******************************************************************************/
#define LOGIC_HIGH 1
#define LOGIC_LOW 0
#define _XTAL_FREQ 8000000
#define DIS0 PORTAbits.RA0
#define DIS1 PORTAbits.RA1
#define DIS2 PORTAbits.RA2
#define DIS3 PORTAbits.RA3

enum status
{
    RUNNING,    // Game is running (can't toggle)
    IDLE,       // Game stopped, can start another round
};

enum toggle
{
    RB0_FUNCTION,   // RB0 function(+1) functionality
    INT0_FUNCTION,  // Interrupt INT0 functionality
};

const unsigned char mapping[] = {
    /* space !     "     #     $     %     &     '     [     ]     *     +  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x0F, 0x00, 0x00,

    /* ,     -     .     /                                                  */
    0x04, 0x40, 0x80, 0x00,

    /* 0     1     2     3     4     5     6     7     8     9     :     ;  */
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00, 0x00,

    /* <     =     >     ?     @     A     B     C     D     E     F     G  */
    0x00, 0x48, 0x00, 0xD3, 0x00, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x3D,

    /* H     I     J     K     L     M     N     O     P     Q     R     S */
    0x76, 0x30, 0x1E, 0x76, 0x38, 0x76, 0x54, 0x3F, 0x73, 0x67, 0x50, 0x6D,

    /* T     U     V     W     X     Y     Z     [     \     ]     ^     _   */
    0x78, 0x3E, 0x3E, 0x3E, 0x76, 0x6E, 0x5B, 0x39, 0x00, 0x0F, 0x00, 0x08,

    /* '     a     b     c     d     e     f     g     h     i     j     k   */
    0x00, 0x77, 0x7C, 0x58, 0x5E, 0x79, 0x71, 0x3D, 0x74, 0x30, 0x1E, 0x76,

    /* l     m     n     o     p     q     r     s     t     u     v     w   */
    0x30, 0x76, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78, 0x1C, 0x3E, 0x3E,

    /* x     y     z                                                         */
    0x76, 0x6E, 0x5B};

const char NUMBERS[] = {0b00111111, 0b00000110,
                        0b01011011, 0b01001111,
                        0b01100110, 0b01101101,
                        0b01111101, 0b00000111,
                        0b01111111, 0b01101111};

const char* loser_str   = "LOSER";
const char* winner_str  = "WINNER";
const char* to_start    = "GAME STOPPED PRESS RB1 THEN RE0 TO START GAME";

int GAME_STATUS = IDLE;             // Default game status is IDLE
int FUNCTION_STATUS = RB0_FUNCTION; // Default functionality is RB0 function(+1)
int counter = 0;                    // Counter is reseted at start of the program
int DELAY_AMOUNT = 150;


// Print number to 7SEG displays
// pre: pointPos > 0
void imprimir_segments(int number, bool is_decimal, char pointPos)
{
    for (int currDisplay = 0; currDisplay < 4; ++currDisplay)
    {
        int value = number % 10;
        // If decimal NUMBERS[] or 0b10000000 (8th bit is dp bit)
        // Otherwise just print value loaded from array
        if (is_decimal)
        {
            if (currDisplay == pointPos)
                LATD = NUMBERS[value] | 0x80;
            else
                LATD = NUMBERS[value];
        }
        else
            LATD = NUMBERS[value];

        switch (currDisplay)
        {
        case 0:
            DIS0 = LOGIC_HIGH;
            break;
        case 1:
            DIS1 = LOGIC_HIGH;
            break;
        case 2:
            DIS2 = LOGIC_HIGH;
            break;
        case 3:
            DIS3 = LOGIC_HIGH;
            break;
        }
        __delay_us(50);

        DIS0 = DIS1 = DIS2 = DIS3 = 0x00;
        number = number / 10;
    }
}

// Display "digit" in 7SEG displays
void displayDigit(char display, int digit)
{
    // If decimal NUMBERS[] or 0b10000000 (8th bit is dp bit)
    // Otherwise just print value loaded from array
    LATD = NUMBERS[digit];

    switch (display)
    {
    case 0:
        DIS0 = LOGIC_HIGH;
        break;
    case 1:
        DIS1 = LOGIC_HIGH;
        break;
    case 2:
        DIS2 = LOGIC_HIGH;
        break;
    case 3:
        DIS3 = LOGIC_HIGH;
        break;
    }
    __delay_us(50);
    DIS0 = DIS1 = DIS2 = DIS3 = 0x0;
}

// Display number in 7 segment display
void displayNumber(int number)
{
    for (int currDisplay = 0; currDisplay < 4; ++currDisplay)
    {
        int value = number % 10;
        displayDigit(currDisplay, value);
        number = number / 10;
    }
}

// Display a character to 7SEG display
void displayChar(char ch, char display)
{
    PORTD = mapping[ch - ' '];
    switch (display)
    {
    case 0:
        PORTAbits.RA0 = 1;
        break;
    case 1:
        PORTAbits.RA1 = 1;
        break;
    case 2:
        PORTAbits.RA2 = 1;
        break;
    case 3:
        PORTAbits.RA3 = 1;
        break;
    }
    __delay_us(50);
    PORTA = 0;
}

// Display 4 letters to all 4 7SEG displays in roder
// if character is '\n', turn of the corresponding display instead
void displayText4(char a, char b, char c, char d)
{
    PORTA = 0;
    for (char display = 0; display < 4; ++display)
    {
        char ch2;
        switch (display)
        {
        case 0:
            ch2 = d;
            break;
        case 1:
            ch2 = c;
            break;
        case 2:
            ch2 = b;
            break;
        case 3:
            ch2 = a;
            break;
        }
        if (ch2 != '\n')
            displayChar(ch2, display);
    }
}

// Display text in the 7SEG display
void displayText(const char *str)
{
    char output[128];
    sprintf(output, "\n\n\n%s\n\n\n", str);
    for (int i = 0; i < strlen(output) - 3; ++i)
    {
        for (int j = 0; j < DELAY_AMOUNT; ++j)
            displayText4(output[i], output[i + 1], output[i + 2], output[i + 3]);
    }
}

void interrupt ISR_other(void)
{
    if (INTCONbits.INT0IE && INTCONbits.INT0IF)
    {
        INTCONbits.INT0IF = 0;
        ++counter;
        return;
    }
}

void interrupt low_priority ISR_low(void)
{
    INTCONbits.RBIF = 0;

    if (INTCON3bits.INT1IE && INTCON3bits.INT1IF)
    {
        // Disable INT1 as we don't want to process
        // INT1 interrups while game is in RUNNING state
        INTCON3bits.INT1IE = 0;
        INTCON3bits.INT1IF = 0;
        DELAY_AMOUNT = 20;
        GAME_STATUS = RUNNING;
    }

    if (INTCONbits.RBIE && INTCONbits.RBIF && PORTBbits.RB7)
    {
        DELAY_AMOUNT = 20;
        // RB7 toggle to switch funtionalities
        FUNCTION_STATUS = (FUNCTION_STATUS == RB0_FUNCTION) ? INT0_FUNCTION : RB0_FUNCTION;
    }
}

// increase counter according to pin_number value
void addPORTB(int *counter, char pin_number)
{
    switch (pin_number)
    {
    case 0:
        *counter = *counter + 1;
        break;
    case 1:
        *counter = *counter + 10;
        break;
    case 2:
        *counter = *counter + 100;
        break;
    case 3:
        *counter = *counter + 1000;
        break;
    }
}

// return 1 if player wins the game
int winner()
{
    displayText(winner_str);
    return 0b00000001;
}

// return 2  if player loses the game
int loser()
{
    displayText(loser_str);
    return 0b00000010;
}

// Start the game
void game_start(int *_game_stat) 
{ 
    *_game_stat = RUNNING; 
}

// Stop the game
void game_stop(int *_game_stat) 
{ 
    *_game_stat = IDLE; 
}

// setup PIC17 for I/O operations
void config_PIC(void)
{
    ANSELA = 0b00000000; // Set pins for digital I/O
    ANSELB = 0b00000000; // Set pins for digital I/O
    ANSELC = 0b00000000; // Set pins for digital I/O
    ANSELD = 0b00000000; // Set pins for digital I/O
    ANSELE = 0b00000000; // Set pins for digital I/O

    TRISBbits.RB0 = LOGIC_HIGH; // Set pin as input
    TRISBbits.RB1 = LOGIC_HIGH; // Set pin as input
    TRISBbits.RB2 = LOGIC_HIGH; // Set pin as input
    TRISBbits.RB3 = LOGIC_HIGH; // Set pin as input
    TRISBbits.RB7 = LOGIC_HIGH; // Set pin as input

    TRISCbits.RC0 = LOGIC_LOW; // Set pin as output
    TRISCbits.RC1 = LOGIC_LOW; // Set pin as output

    TRISAbits.RA0 = LOGIC_LOW; // Set pin as output
    TRISAbits.RA1 = LOGIC_LOW; // Set pin as output
    TRISAbits.RA2 = LOGIC_LOW; // Set pin as output
    TRISAbits.RA3 = LOGIC_LOW; // Set pin as output

    TRISEbits.RE0 = LOGIC_HIGH; // Set pin as input
    TRISEbits.RE1 = LOGIC_HIGH; // Set pin as input
    TRISEbits.RE2 = LOGIC_HIGH; // Set pin as input

    TRISD = 0x00; // Set all pins from PORTD as output
}

// setup interrupts for PIC18
void setupInterruptions(void)
{
    RCONbits.IPEN = 1;   // Interrupt priority enable bit
    INTCONbits.GIEH = 1; // Global Interrupt enable bit
    INTCONbits.GIEL = 1; // Enable low priority interrupts

    INTCON2bits.INTEDG0 = 1; // Interrupt on INT1 rising edge
    INTCONbits.INT0IE = 1;   // External interrupt enable INT0
    INTCONbits.INT0IF = 0;   // Clear IF flag of INT0 interrupt

    // INT1 setup to adress game start latency issues
    INTCON3bits.INT1IE = 1;  // External interrupt enable INT1
    INTCON2bits.INTEDG1 = 1; // Interrupt on INT1 rising edge
    INTCON3bits.INT1IP = 0;  // External interrupt priority low INT1
    INTCON3bits.INT1IF = 0;  // Clear IF flag of INT1 interrupt

    INTCONbits.RBIE = 1;  // Enables the IOCx port change interrupt
    INTCONbits.RBIF = 0;  // Port B Interrupt-On-Change (IOCx) Interrupt Flag bit
    INTCON2bits.RBIP = 0; // RB Port Change Interrupt Priority low
}

void main(void)
{
    config_PIC();
    setupInterruptions();

    while (true)
    {
        // INT0 interrupt function (counter)
        if (FUNCTION_STATUS == INT0_FUNCTION)
            displayNumber(counter);

        // RB0 function(+1)
        if (GAME_STATUS == IDLE && FUNCTION_STATUS == RB0_FUNCTION)
        {
            DELAY_AMOUNT = 150;
            displayText(to_start);
        }

        if (PORTEbits.RE0 && FUNCTION_STATUS == RB0_FUNCTION)
        {
            int contador = 0;
            int random_number = 0;
            random_number = rand() % 10000;

            // busy wait until user press RE1
            // (key button to start the game)
            while (!PORTEbits.RE1)
                displayNumber(random_number);

            game_start(&GAME_STATUS);
            bool RB0, RB1, RB2, RB3;
            RB0 = RB1 = RB2 = RB3 = false;

            bool can_stop = false;
            bool RE2_down = false;
            while (GAME_STATUS == RUNNING)
            {
                // Increase counter by 1 on RB0 falling edge
                if (PORTBbits.RB0)
                {
                    RB0 = true;
                }
                else if (!PORTBbits.RB0 && RB0)
                {
                    RB0 = false;
                    addPORTB(&contador, 0);
                }

                // Increase counter by 10 on RB1 falling edge
                if (PORTBbits.RB1)
                {
                    RB1 = true;
                }
                else if (!PORTBbits.RB1 && RB1)
                {
                    RB1 = false;
                    addPORTB(&contador, 1);
                }

                // Increase counter by 100 on RB2 falling edge
                if (PORTBbits.RB2)
                {
                    RB2 = true;
                }
                else if (!PORTBbits.RB2 && RB2)
                {
                    RB2 = false;
                    addPORTB(&contador, 2);
                }

                // Increase counter by 1000 on RB3 falling edge
                if (PORTBbits.RB3)
                {
                    RB3 = true;
                }
                else if (!PORTBbits.RB3 && RB3)
                {
                    RB3 = false;
                    addPORTB(&contador, 3);
                }

                if (PORTEbits.RE2)
                    RE2_down = true;
                else if (!PORTEbits.RE2 && RE2_down)
                    can_stop = true;

                if (can_stop && !PORTEbits.RE2)
                    game_stop(&GAME_STATUS);

                displayNumber(contador);
            }

            // check if player won
            DELAY_AMOUNT = 150;
            PORTC = (contador == random_number) ? winner() : loser();

            // Display correct number
            for (int i = 0; i < 300; ++i)
                displayNumber(random_number);

            // Enable INT1 interrupt so
            // we can start game again
            INTCON3bits.INT1IE = 1;
            INTCON3bits.INT1IF = 0;

            PORTC = 0;
        }
    }
}