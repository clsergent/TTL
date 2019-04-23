/* ttl implementation for AVR (made for Attiny85)
 * tx function is based on Ralph Doncaster work, rx function is an original work
 * @author: clsergent
 * licence GNU GPLv3
 */

/*
 to use the TxByte function, set the TX_TTLPORT (default to PORTB4) and set it to output
 to use the RxByte function, set the RX_TTLPORT (default to PORTB3) and set it to input
*/
#ifndef BAUD_RATE
    #define BAUD_RATE 115200 //default value
#endif

#ifdef F_CPU
    #define TTLDELAY (((F_CPU/BAUD_RATE)-9)/3)
    #if TTLDELAY > 255
        #error low baud rates unsupported - use higher BAUD_RATE
    #endif
#else
    #error CPU frequency F_CPU undefined
#endif

//------ RX -------

#ifndef RX_TTLPORT
    #define RX_TTLPORT PINB3
#endif

#ifndef TX_TTLPORT
    #define TX_TTLPORT PORTB4
#endif

#define WRITE_1 PORTB |= (1<<TX_TTLPORT)
#define WRITE_0 PORTB &= (~(1<<TX_TTLPORT))

#define INIT_RX_TTLPORT DDRB &= ~(1<<RX_TTLPORT);
#define INIT_TX_TTLPORT DDRB |= (1<<TX_TTLPORT);

uint16_t rx_byte(uint16_t data) {
    //args: r25:24(, r23:r22)
    cli();
    asm("ldi r25, 9");
    asm("sbis %0, %1" :: "I" (_SFR_IO_ADDR(PINB)), "I" (RX_TTLPORT));     //0x16 = address of PINB      
    asm("rjmp RXDone");                     //si RX_TTPORT = 1, alors ce n'est pas un début de transmission (toujours 1er bit = 0)
    asm("rjmp RXEnd");

    asm("RXLoop:");
        asm("sbis %0, %1" :: "I" (_SFR_IO_ADDR(PINB)), "I" (RX_TTLPORT));        //si = 1, on saute l'étape suivante
        asm("clc");                         //carry = 0
        asm("sbic %0, %1" :: "I" (_SFR_IO_ADDR(PINB)), "I" (RX_TTLPORT));
        asm("sec");                         //carry = 1
        asm("ror r24");                     //ajout progressif via carry
    asm("RXDone:");
        asm("ldi r23, %0" :: "I" (TTLDELAY)); //met le délai
    asm("RXDelay:");
        asm("dec r23");
        asm("brne RXDelay");
        asm("dec r25");
        asm("brne RXLoop");
    asm("RXEnd:");
        sei();
        //data is returned through r25:r24 : r25 = char, r24 = correct if 0, incorrect otherwise
    return data;
}

#define RxByte() rx_byte(0x00)


//------ TX -------


void tx_byte(char byte, char delay) {
    //args: r25:r24, r23:r22
    cli();
    WRITE_1;
    asm("ldi r25, 10");         // 1 start + 8 bits (byte) + 1 end
    asm("com r24");             // 0xff + byte (+set carry)

    asm("TxLoop:");
        asm("brcc tx1");        //branch if carry clear
        WRITE_0;                //set 0
    asm("tx1:");
        asm("brcs TxDone");     //branch if carry set
        WRITE_1;                 //set 1
    asm("TxDone:");
        asm("mov r23, r22");        //set delay
    asm("TxDelay:");
        asm("dec r23");         //loop for delay
        asm("brne TxDelay");
        asm("lsr r24");         //byte>>1 used to get the carry
        asm("dec r25");         //next bit
        asm("brne TxLoop");
    sei();
}

#define TxByte(data) tx_byte(data, TTLDELAY)


