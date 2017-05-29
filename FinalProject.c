/*****************************************************
This program was produced by the
CodeWizardAVR V2.05.3 Standard
Automatic Program Generator
© Copyright 1998-2011 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : 
Version : 
Date    : 05-Feb-2017
Author  : Nemanja Djekic H1 3/2016 Lazar Vukasovic H1 7/2016
Company : 
Comments: 


Chip type               : ATmega32A
Program type            : Application
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*****************************************************/
/****************************************************
    Description:
    Home alarm system simulation on Mikroe EASY AVR7
    environment. PORTB represents keyboard, where user
    can enter password. PORTC.0 is button, which starts 
    alarm system, when user leaves object, that should 
    be covered by alarm system. ADC conversion (PORTA.5)
    represents movement detection in secure are, and it
    triggers alarm, if there is breakin. PWM on PORTD.7
    and Timer 2 represent alarm light. LCD display has 
    classic usage and it is used to printout system 
    status, if object is locked or unlocked.
*****************************************************/

#include <mega32a.h>

#include <delay.h>

// Alphanumeric LCD functions
#include <alcd.h>

#ifndef RXB8
#define RXB8 1
#endif

#ifndef TXB8
#define TXB8 0
#endif

#ifndef UPE
#define UPE 2
#endif

#ifndef DOR
#define DOR 3
#endif

#ifndef FE
#define FE 4
#endif

#ifndef UDRE
#define UDRE 5
#endif

#ifndef RXC
#define RXC 7
#endif

#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<DOR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)

// USART Receiver buffer
#define RX_BUFFER_SIZE 16
char rx_buffer[RX_BUFFER_SIZE];

#if RX_BUFFER_SIZE <= 256
unsigned char rx_wr_index,rx_rd_index,rx_counter;
#else
unsigned int rx_wr_index,rx_rd_index,rx_counter;
#endif

// This flag is set on USART Receiver buffer overflow
bit rx_buffer_overflow;

// USART Receiver interrupt service routine
interrupt [USART_RXC] void usart_rx_isr(void)
{
char status,data;
status=UCSRA;
data=UDR;
if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
   {
   rx_buffer[rx_wr_index++]=data;
#if RX_BUFFER_SIZE == 256
   // special case for receiver buffer size=256
   if (++rx_counter == 0) rx_buffer_overflow=1;
#else
   if (rx_wr_index == RX_BUFFER_SIZE) rx_wr_index=0;
   if (++rx_counter == RX_BUFFER_SIZE)
      {
      rx_counter=0;
      rx_buffer_overflow=1;
      }
#endif
   }
}

#ifndef _DEBUG_TERMINAL_IO_
// Get a character from the USART Receiver buffer
#define _ALTERNATE_GETCHAR_
#pragma used+
char getchar(void)
{
char data;
while (rx_counter==0);
data=rx_buffer[rx_rd_index++];
#if RX_BUFFER_SIZE != 256
if (rx_rd_index == RX_BUFFER_SIZE) rx_rd_index=0;
#endif
#asm("cli")
--rx_counter;
#asm("sei")
return data;
}
#pragma used-
#endif

// USART Transmitter buffer
#define TX_BUFFER_SIZE 16
char tx_buffer[TX_BUFFER_SIZE];

#if TX_BUFFER_SIZE <= 256
unsigned char tx_wr_index,tx_rd_index,tx_counter;
#else
unsigned int tx_wr_index,tx_rd_index,tx_counter;
#endif

// USART Transmitter interrupt service routine
interrupt [USART_TXC] void usart_tx_isr(void)
{
if (tx_counter)
   {
   --tx_counter;
   UDR=tx_buffer[tx_rd_index++];
#if TX_BUFFER_SIZE != 256
   if (tx_rd_index == TX_BUFFER_SIZE) tx_rd_index=0;
#endif
   }
}

#ifndef _DEBUG_TERMINAL_IO_
// Write a character to the USART Transmitter buffer
#define _ALTERNATE_PUTCHAR_
#pragma used+
void putchar(char c)
{
while (tx_counter == TX_BUFFER_SIZE);
#asm("cli")
if (tx_counter || ((UCSRA & DATA_REGISTER_EMPTY)==0))
   {
   tx_buffer[tx_wr_index++]=c;
#if TX_BUFFER_SIZE != 256
   if (tx_wr_index == TX_BUFFER_SIZE) tx_wr_index=0;
#endif
   ++tx_counter;
   }
else
   UDR=c;
#asm("sei")
}
#pragma used-
#endif

// Standard Input/Output functions
#include <stdio.h>

#define FIRST_ADC_INPUT 0
#define LAST_ADC_INPUT 7
unsigned int adc_data[LAST_ADC_INPUT-FIRST_ADC_INPUT+1];
#define ADC_VREF_TYPE 0x00

// ADC interrupt service routine
// with auto input scanning
interrupt [ADC_INT] void adc_isr(void)
{
static unsigned char input_index=0;
// Read the AD conversion result
adc_data[input_index]=ADCW;
// Select next ADC input
if (++input_index > (LAST_ADC_INPUT-FIRST_ADC_INPUT))
   input_index=0;
ADMUX=(FIRST_ADC_INPUT | (ADC_VREF_TYPE & 0xff))+input_index;
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
// Start the AD conversion
ADCSRA|=0x40;
}

// Functions

    // ADC USART Log
void WriteUART1dec2string(unsigned int data)
{
    unsigned char temp;

    temp=data/1000;
    putchar(temp+'0');
    data=data-temp*1000;
    temp=data/100;
    putchar(temp+'0');
    data=data-temp*100;
    temp=data/10;
    putchar(temp+'0');
    data=data-temp*10;
    putchar(temp+'0');
}
    
    // Activate PWM on PB3 diode 
    void alarm(int status)
    {      
        if(status == 1)
        {
            OCR2=0x00;
            OCR2=0x63;
        }
        else
        {   
            OCR2=0x00;
        }
    }                      
    
    // Print out locked status on LCD
    void printLCDStatusLocked()
    {
        lcd_gotoxy(0,0);
        lcd_puts("SYSTEM LOCKED");  
        delay_ms(500);  
        lcd_clear();    
    }    
    
    // Print out unlocked status on LCD 
     void printLCDStatusUnlocked()
    {
        lcd_gotoxy(0,0);
        lcd_puts("SYSTEM UNLOCKED");  
        delay_ms(500);  
        lcd_clear();    
    }     
    
// Declare your global variables here
// Check pin value
int value = 0;
// Counts how many buttons were pressed
int counter = 0;
int ADC_Pin_Switch = 0;

// Check if system is active, init state is inactive
int systemActive = 0;

// Set alarm on
int alarmOn = 1;
// Set alarm off
int alarmOff = 0;

// Check if correct password is entered, init state is false
int passwordFlag = 0;


void main(void)
{
// Declare your local variables here

// Input/Output Ports initialization
// Port A initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTA=0x00;
DDRA=0x00;

// Port B initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTB=0x00;
DDRB=0x00; //0x08

// Port C initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTC=0x00;
DDRC=0x00;

// Port D initialization
// Func7=Out Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=0 State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTD=0x00;
DDRD=0x80;
//DDRD=0xB0;


// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: Timer 0 Stopped
// Mode: Normal top=0xFF
// OC0 output: Disconnected
//TCCR0=0x63;//6B; 63 true value
//TCNT0=0x00;
//OCR0=0x7F;

TCCR0=0x00;
TCNT0=0x00;
OCR0=0x00;


// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: Timer1 Stopped
// Mode: Normal top=0xFFFF
// OC1A output: Discon.
// OC1B output: Discon.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=0x00;
TCCR1B=0x00;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;    

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: 125.000 kHz
// Mode: Fast PWM top=0xFF
// OC2 output: Non-Inverted PWM
ASSR=0x00;
TCCR2=0x67; //0x6C
TCNT2=0x00;
//OCR2=0x00; //0x07

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: Off
MCUCR=0x00;
MCUCSR=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x00;

// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver: On
// USART Transmitter: On
// USART Mode: Asynchronous
// USART Baud Rate: 76800
UCSRA=0x00;
UCSRB=0xD8;
UCSRC=0x86; //0x06
UBRRH=0x00;
UBRRL=0x0C; //0x06

// Analog Comparator initialization
// Analog Comparator: Off
// Analog Comparator Input Capture by Timer/Counter 1: Off
ACSR=0x80;
SFIOR=0x00;

// ADC initialization
// ADC Clock frequency: 1000.000 kHz
// ADC Voltage Reference: AREF pin
ADMUX=FIRST_ADC_INPUT | (ADC_VREF_TYPE & 0xff);
ADCSRA=0xCB;

// SPI initialization
// SPI disabled
SPCR=0x00;

// TWI initialization
// TWI disabled
TWCR=0x00;

// Alphanumeric LCD initialization
// Connections are specified in the
// Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
// RS - PORTA Bit 2
// RD - PORTA Bit 1
// EN - PORTD Bit 6
// D4 - PORTC Bit 4
// D5 - PORTC Bit 5
// D6 - PORTC Bit 6
// D7 - PORTC Bit 7
// Characters/line: 16
lcd_init(16);

// Global enable interrupts
#asm("sei")

while (1)
      {
      // Place your code here
      // Password flow
     
      
      // Read the pin register, write to port register
         
      if(PINC.0 && (1<<PORTC.0))
      {
        delay_ms(50);
        systemActive = 1; // System activated
        passwordFlag = 0; // Correct password not entered
        printf("\tLOG: System locked.");  
        
        // LCD Output
        printLCDStatusLocked();
      }
       
      if(systemActive == 1)
      {        
        if(PINB.1 &&(1<<PORTB.1))
        {   
            delay_ms(50);
            counter++; 
            if(value == 0) value = 1;
        }         
        
        // Cause of password flow, 7 is here 
        if(PINB.7 &&(1<<PORTB.7))
        {     
            delay_ms(50);
            counter++;    
            if(value == 1) value = 2;
        }
                 
        // Cause of password flow, 3 is here
        if(PINB.3 &&(1<<PORTB.3))
        {      
            delay_ms(50);
            counter++;  
            if(value == 2) value = 3;
        }     
          
        // Cause of password flow, 5 is here
        if(PINB.5 && (1<<PORTB.5))
        {  
            delay_ms(50);
            counter++; 
            if(value == 3) value = 4;
        } 
        
        if(PINB.0 &&(1<<PORTB.0))
        {   
            delay_ms(50);
            counter++;   
        }
        
        if(PINB.2 &&(1<<PORTB.2))
        {       
            delay_ms(50);
            counter++; 
        }
         
        if(PINB.4 &&(1<<PORTB.4))
        {     
            delay_ms(50);
            counter++; 
        } 
        
        if(PINB.6 &&(1<<PORTB.6))
        {   
            delay_ms(50);
            counter++; 
        }  
                        
        //Unlock signal. System inactive
        if(counter == 4 && value == 4)
        {   
           printf("\tLOG: Alarm deactivated!"); 
            //Disable alarm
            alarm(alarmOff);    
            
            // Correct password entered
            passwordFlag = 1; 
            
            value = 0;
            counter = 0;  
                          
            // System deactivated
            systemActive = 0;   
            
            // LCD output
            printLCDStatusUnlocked();   
        }            
        
        //Warning>Wrong password            
        else if (counter == 4 && value != 4)
        {       
            printf("\tLOG: Alarm activated!"); 
            alarm(alarmOn);  
            
            value = 0;
            counter = 0; 
            
            // LCD output
            printLCDStatusLocked();     
        } 

      // System breakin
       else if((adc_data[5] >= 450) && passwordFlag == 0)
        {    
            alarm(alarmOn); 
            printf("\tLOG: System breakin!");        
        }       
        
     }  
          
     else if(systemActive == 0)
     {
        alarm(alarmOff); 
         printLCDStatusUnlocked();
     }
               
   }
}
