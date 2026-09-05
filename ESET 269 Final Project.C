#include <stdlib.h>
#include <stdio.h>
#include "MSP.h"
#include <string.h>

//UART functions
void UARTInit(void)
{
	//enable reset
	//use CTLW0 (control word 0 register)
	//use |= 1
	EUSCI_A0 -> CTLW0 |= 1;
	//disable parity(00), LSB first(0), 8-bits(0), one stop bit(0), UART mode(00), asynchronous(0), Subsystem master clock(10/11), D5-D1(00000), 
	//reset(0)
	//use CTLW0 (control word 0 register)
	//use |= 0x80
	EUSCI_A0 -> CTLW0 |= 0x80;
	//set MCTLW (modulation control register) to 0
	EUSCI_A0 -> MCTLW = 0;
	//assign a value(78 = 0x4E) to BRW (Baud Rate Word Register)
	//baud rate = clock/BRW
	//baud rate = 38400
	EUSCI_A0 -> BRW = 0x4E;
	//set port 1 pin 2 and 3 SEL0 to 1 (use |=) and SEL1 to 0 (use &= ~)
	//7654 3210
	//0000 1100 --> 0x0C
	P1 -> SEL0 |= 0x0C;
	P1 -> SEL1 &= ~0x0C;
	//disable reset
	//use CTLW0 (control word 0 register)
	//use &= ~1
	EUSCI_A0 -> CTLW0 &= ~1;
}

void UARTprint(char x[])
{
	//loop through each character
	for(int i=0;i<strlen(x);i++)
	{
		//transmit the character using the transmit buffer (TXBUF)
		EUSCI_A0 -> TXBUF = x[i];
		//wait until the character is transmitted before transmitting the next character
		//TXIFG is the 2nd bit in the IFG register
		//use & 2
		//TXIFG is 1 when all bits have been transmitted
		//TXIFG is 0 when bits are still being transmitted
		while((EUSCI_A0 -> IFG & 2) == 0) //while bits are being transmitted
		{
			//wait
		}
	}
}
void UARTscan(char *x)
{
	//create an int that keeps track of index
	int i = 0;
	//function takes in character pointer (or a character array pointer)
	while(1) //allows user to type character into the receive buffer
	{
		//check if receive buffer (RXBUF) has a character in it
		//RXIFG is the 1st bit in the IFG register
		//use & 1
		//RXIFG is 0 when there is nothing in the receive buffer 
		//RXIFG is 1 when there is a character in the receive buffer
		if ((EUSCI_A0 -> IFG & 1)!= 0)
		{
			//store that character into ith index of the character array
			//use RXBUF
			x[i] = EUSCI_A0 -> RXBUF;
			//echo the character back using transmit buffer
			//use TXBUF
			EUSCI_A0 -> TXBUF = x[i];
			//wait for the character to be transmitted 
			//TXIFG is the 2nd bit in the IFG register
			//use & 2
			//TXIFG is 1 when all bits have been transmitted
			//TXIFG is 0 when bits are still being transmitted
			while ((EUSCI_A0 -> IFG & 2)==0) //while bits are still being transmitted 
			{
				//wait
			}
			//if the character is '\r', change it to a '\0' (so that string functions can be used) and break
			if (x[i] == '\r')
			{
				x[i] = '\0';
				break;
			}
			//else increment i by 1
			else
			{
				i++;
			}
		}
	}
}

/*ADC Functions*/
//function prototypes

void ADCInit(void);//initialize the ADC

float tempRead(void); //read temperature sensor

//function definitions
void ADCInit(void)
{
	//Ref_A settings
	REF_A ->CTL0 &= ~0x8; //enable temp sensor
	REF_A ->CTL0 |= 0x30; //set ref voltage
	REF_A ->CTL0 &= ~0x01; //enable ref voltage
	
	//do ADC stuff
	ADC14 ->CTL0 |= 0x10; //turn on the ADC
	ADC14 ->CTL0 &= ~0x02; //disable ADC
	ADC14 ->CTL0 |=0x4180700; //no prescale, mclk, 192 SHT
	ADC14 ->CTL1 &= ~0x1F0000; //configure memory register 0
	ADC14 ->CTL1 |= 0x800000; //route temp sense
	ADC14 ->MCTL[0] |= 0x100; //vref pos int buffer
	ADC14 ->MCTL[0] |= 22; //channel 22
	ADC14 ->CTL0 |=0x02; //enable adc
		
	return;
} 

float tempRead(void)
{
	float temp; //temperature variable
	uint32_t cal30 = TLV ->ADC14_REF2P5V_TS30C; //calibration constant
	uint32_t cal85 = TLV ->ADC14_REF2P5V_TS85C; //calibration constant
	float calDiff = cal85 - cal30; //calibration difference
	ADC14 ->CTL0 |= 0x01; //start conversion
	while((ADC14 ->IFGR0) ==0) 
	{
		//wait for conversion
	}
	temp = ADC14 ->MEM[0]; //assign ADC value
	temp = (temp - cal30) * 55; //math for temperature per manual
	temp = (temp/calDiff) + 30; //math for temperature per manual
	
	return temp; //return temperature in degrees C
}
/*END*/

void menu(void)
{
	/*When the program starts, it displays a menu to the 
	user as shown below. A user selects an option to 
	interact with the Launchpad.*/
	UARTprint("MSP432 Menu\n\n\r");
	UARTprint("1. RGB Control\n\r");
	UARTprint("2. Digital Input\n\r");
	UARTprint("3. Temperature Reading\n\r");
}
void digitalOutputInit(void)
{
	//port 2 pin 0, 1, and 2
	//SET SEL0 AND SEL1 TO 0
	//USE &= ~ 0x07
	//digitally enabled
	P2 -> SEL0 &= ~ 0x07;
	P2 -> SEL1 &= ~ 0x07;
	//SET DIR TO 1
	//USE |= 0x07
	//DEFAULT HIGH
	//outputs
	P2 -> DIR |= 0x07;
	//SET PINS TO LOW (OUT TO 0)
	//USE &= ~ 0x07
	P2 -> OUT &= ~ 0x07;
}
void SysTickInit(void)
{
	//use CTRL
	//use |=
	//system clock (1), disable interrupt (0), disable SysTick(0)
	SysTick -> CTRL |= 0x04;
}
void TIMER32Init(void)
{
	//Use CONTROL 
	//Use |= 
	//0x42
	//disable TIMER32 (0), periodic mode (1), disable interrupt (0), reserved (0), prescale to freq divided by 1 (00), 32 bit size (1), oneshot(0)
	TIMER32_1 -> CONTROL |= 0x42;
}
void digitalInputInit(void)
{
	//port 1 pin 1 and 4
	//SET SEL0 AND SEL1 TO 0
	//7654 3210
	//0001 0010 -> 0x12
	//use &= ~ 0x12
	//digitally enabled
	P1 -> SEL0 &= ~ 0x12;
	P1 -> SEL1 &= ~ 0x12;
	//SET DIR TO 0
	//use &= ~0x12
	//inputs
	P1 -> DIR &= ~ 0x12;
	//SET REN TO 1
	//use |= 0x12
	//resistor
	P1 -> REN |= 0x12;
	//SET OUT TO 1 (SINCE S1 AND S2 ARE ACTIVE LOW)
	//NEED A PULL UP RESISTOR
	//use |= 0x12
	P1 -> OUT |= 0x12;
}
void SysTickDelay(int time)
{
	//LOAD value on SysTick
	//load value = (time*3000000) - 1
	SysTick -> LOAD = (time*3000000) - 1;
	//enable SysTick
	//use CTRL
	//use |= 0x01
	SysTick -> CTRL |= 0x01;
	//poll
	//use CTRL
	//use & 0x10000
	//countflag is 1 when counting is done
	while ((SysTick -> CTRL & 0x10000) == 0) //while counting
	{
		//wait
	}
	//disable SysTick
	//use CTRL
	//use &= ~ 0x01
	SysTick -> CTRL &= ~ 0x01;
}
void TIMER32Delay(void)
{
	int time = 1;
	//load value = (time*clock freq) - 1
	TIMER32_1 -> LOAD = (time*3000000)-1;
	//enable TIMER32_1
	//use CONTROL
	//use |=
	//0x80
	TIMER32_1 -> CONTROL |= 0x80;
	//poll
	//check first bit of RIS register
	//if it is 1, then counting is done
	while ((TIMER32_1 -> RIS & 1) == 0) //while counting
	{
		//wait
	}
	//set INTCLR to 0
	TIMER32_1 -> INTCLR = 0;
	//disable TIMER32_1
	//use CONTROL
	//use &= ~ 0x80
	TIMER32_1 -> CONTROL &= ~ 0x80;
}
void RGBControl(void)
{
	char scan[5];
	int combination, toggle_time, number_of_blinks;
	UARTprint("Enter Combination of RGB (1-7):");
	UARTscan(scan);
	UARTprint("\n\r");
	combination = atoi(scan);
	UARTprint("Enter Toggle Time:");
	UARTscan(scan);
	UARTprint("\n\r");
	toggle_time = atoi(scan);
	UARTprint("Enter Number of Blinks:");
	UARTscan(scan);
	UARTprint("\n\r");
	number_of_blinks = atoi(scan);
	UARTprint("Blinking LED...\n\r");
	//number of blinks
	//1 blink -> 2 toggles
	for (int i = 0; i<(2*number_of_blinks); i++)
	{
		//toggle RGB combination on P2
		//use ^=
		P2 -> OUT ^= combination;
		//delay
		SysTickDelay(toggle_time);
	}
	UARTprint("Done\n\r");
}
void DigitalInput(void)
{
	//use IN register
	//0x12
	if ((P1 -> IN & 0x12) == 0)
	{
		UARTprint("Both buttons pressed.\n\n\r");
	}
	//0x10
	else if ((P1 -> IN & 0x10) == 0)
	{
		UARTprint("Button 2 pressed.\n\n\r");
	}
	//0x02
	else if ((P1 -> IN & 0x02) == 0)
	{
		UARTprint("Button 1 pressed.\n\n\r");
	}
	else
	{
		UARTprint("No Button pressed.\n\n\r");
	}
}
void TemperatureReading(void)
{
	char scan[5];
	char sentence[30];
	int number_of_temperature_reading;
	UARTprint("Enter Number of Temperature Reading (1-5):");
	UARTscan(scan);
	UARTprint("\n\r");
	number_of_temperature_reading = atoi(scan);
	for (int i = 0; i<number_of_temperature_reading;i++)
	{
		float tempInCelsuis = tempRead();
		float tempInFahrenheit = (9.0/5.0* tempInCelsuis) + 32;
		sprintf(sentence,"Reading %d: %.2f C & %.2f F\n\r",i+1,tempInCelsuis,tempInFahrenheit);
		UARTprint(sentence);
		//delay
		TIMER32Delay();
	}
	UARTprint("\n");
}
void Choice(int choice)
{
	switch(choice)
	{
		case(1):
			RGBControl();
			break;
		case(2):
			DigitalInput();
			break;
		case(3):
			TemperatureReading();
			break;
		default:
			break;
	}
}
int main(void)
{
	char scan[5];
	char choice;
	digitalOutputInit();
	SysTickInit();
	digitalInputInit();
	TIMER32Init();
	ADCInit();
	UARTInit();
	while(1)
	{
		menu();
		UARTscan(scan);
		UARTprint("\n\r");
		choice = atoi(scan);
		Choice(choice);
	}
}
