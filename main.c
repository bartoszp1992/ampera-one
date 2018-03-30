/*
 * main.c
 *
 *  Created on: 12 lut 2018
 *      Author: bartosz
 *     	Ampera ONE
 *
 *      initial pre-alpha release
 *      v0.6 - added lights
 *
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "hd44780.h"
#include <avr/interrupt.h>

int itoa();
int sprintf(char *str, const char *format, ...);


int main(void) {

	//debug led(kers ON)
	DDRC |= (1 << PC5);
	DDRC |= (1 << PC4);

	//PWM out
	DDRB |= (1 << PB1);
	DDRB |= (1<<PB2);

	//button
	DDRB &= ~(1<<PB4);
	PORTB |= (1<<PB4);

	//lights
	DDRC |= (1<<PC4);
	PORTC &= ~(1<<PC4);


	//hardware PWM define
	TCCR1A |= (1 << WGM10); //mode-fastPWM
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << CS12); //prescaler: 256
	OCR1A = 0; //pwm out LOW
	TCCR1A |= (1 << COM1A1); //pwm on

	//ADC define
	ADCSRA |= (1 << ADPS1); //prescaler
	ADCSRA |= (1 << ADPS2);
	ADCSRA |= (1 << ADEN); //ON
	ADMUX |= (1 << REFS0); //reference voltage
	//pin select
	ADMUX |= (1 << MUX1);	//ADC3(battery)
	ADMUX |= (1 << MUX0);

	ADMUX |= (1 << MUX1);	//ADC2(throttle)
	ADMUX &= ~(1 << MUX0);

	ADMUX |= (1 << MUX0);	//ADC1(kers)
	ADMUX &= ~(1 << MUX1);

	//ADMUX &= ~(1<<MUX0);//ADC0(kers -charge)
	//ADMUX &= ~(1<<MUX1);

	//variables
	//main
	int throttle = 0;
	int duty = 0;

	//modes
	int mode = 3;
	int kers = 0;

	//kers
	int dispertionDuty;

	//voltages
	float voltage;
	float kersVoltage;
	float difference;
	float minVoltage = 13;

	//lcd
	int modeDisplay = 0;
	int voltageDisplay = 10;
	int throttleDisplay = 73;
	int kersDisplay = 65;
	char buffer[10];
	int counter = 1;

	lcd_init();

	lcd_clrscr();
	lcd_puts("Bart's");
	lcd_goto(64);
	lcd_puts("Ampera ONE");
	_delay_ms(5000);
	lcd_clrscr();
	//lcd_goto(9);
	//lcd_puts("[");
	//lcd_goto(15);
	//lcd_puts("]");
	lcd_goto(64);
	lcd_puts("[");
	lcd_goto(71);
	lcd_puts("][");
	lcd_goto(79);
	lcd_puts("]");

	while (1) {

		//PORTC ^= (1<<PC5);
		//_delay_ms(200);

		//mode select
		if(!(PINB & (1<<PB4))){
			_delay_ms(200);

			if(PINB & (1<<PB4)){
				PORTC ^= (1<<PC4);
			}

			if(mode <= 2 && !(PINB & (1<<PB4))){
				mode++;
				_delay_ms(800);
			}

			if(mode > 2 && !(PINB & (1<<PB4))){
				mode = 1;
				_delay_ms(800);
			}
		}

		//mode
		if (mode == 1) {
			lcd_goto(modeDisplay);
			lcd_puts("eco  ");
		}

		if (mode == 2) {
					lcd_goto(modeDisplay);
					lcd_puts("smart");
				}

		if (mode == 3) {
					lcd_goto(modeDisplay);
					lcd_puts("sport");
				}

		//battery
		ADMUX |= (1 << MUX1);		//ADC3(battery)
		ADMUX |= (1 << MUX0);
		ADCSRA |= (1 << ADSC); //start
		while (ADCSRA & (1 << ADSC))
			;
		voltage = ADC / 18.2;

		//kers
		ADMUX |= (1 << MUX0); //ADC1(kers)
		ADMUX &= ~(1 << MUX1);
		ADCSRA |= (1 << ADSC); //start
		while (ADCSRA & (1 << ADSC))
			;
		kersVoltage = ADC / 18.2;

		difference = kersVoltage - voltage;


		//drive
		//check throttle

		ADMUX |= (1 << MUX1);		//ADC2(throttle)
		ADMUX &= ~(1 << MUX0);
		ADCSRA |= (1 << ADSC); //start
		while (ADCSRA & (1 << ADSC))
			;
		throttle = ADC;

		//send signal
		if (throttle < 200) {
			duty = 10;
			OCR1A = duty;

			if(kersVoltage > 6) kers = 1;
			else kers = 0;

			//dispertioner
			if(kers == 1){
				dispertionDuty = (voltage - 25.2)*256;
				if(dispertionDuty < 0) dispertionDuty = 0;
				if(dispertionDuty > 255) dispertionDuty = 255;
				OCR1B = dispertionDuty;

			} else {
				OCR1B = 0;
			}


		} else {
			kers = 0;
			OCR1B = 0;
			if (voltage > minVoltage && throttle >= 200)
				duty = 11;
			if (voltage > minVoltage && throttle >= 250)
				duty = 12;
			if (voltage > minVoltage && throttle >= 300)
				duty = 13;
			if (voltage > minVoltage && throttle >= 350)
				duty = 14;
			if (voltage > minVoltage && throttle >= 400)
				duty = 15;
			if (voltage > minVoltage && throttle >= 450)
				duty = 16;
			if (voltage > minVoltage && throttle >= 500 && mode > 1) //mid
				duty = 17;
			if (voltage > minVoltage && throttle >= 550 && mode > 1)
				duty = 18;
			if (voltage > minVoltage && throttle >= 600 && mode > 1)
				duty = 19;
			if (voltage > minVoltage && throttle >= 650 && mode > 1)
				duty = 20;
			if (voltage > minVoltage && throttle >= 700 && mode > 2) //pro
				duty = 21;
			if (voltage > minVoltage && throttle >= 750 && mode > 2)
				duty = 22;
			if (voltage > minVoltage && throttle >= 800 && mode > 2)
				duty = 23;
			if (voltage > minVoltage && throttle >= 820 && mode > 2)
				duty = 24;
			OCR1A = duty;

			ADMUX |= (1 << MUX1);		//ADC3(battery)
			ADMUX |= (1 << MUX0);
			ADCSRA |= (1 << ADSC); //start
			while (ADCSRA & (1 << ADSC))
				;
			voltage = ADC / 18.2;

					while(voltage <= minVoltage && throttle > 200){
						OCR1A = duty - 1;
						ADMUX |= (1 << MUX1);		//ADC3(battery)
						ADMUX |= (1 << MUX0);
						ADCSRA |= (1 << ADSC); //start
						while (ADCSRA & (1 << ADSC))
						;
						voltage = ADC / 18.2;


						ADMUX |= (1 << MUX1);		//ADC2(throttle)
						ADMUX &= ~(1 << MUX0);
						ADCSRA |= (1 << ADSC); //start
						while (ADCSRA & (1 << ADSC))
							;
						throttle = ADC;
					}

		}


		counter++;
		if(counter >32000) counter = 1;


		if((counter % 80) == 0){
			//display voltage
					if(voltage >= 10){
						lcd_goto(voltageDisplay);
					sprintf(buffer, "%2.1fV", voltage);
					lcd_puts(buffer);
					lcd_puts("   ");
					}

					if(voltage < 10){
							lcd_goto(voltageDisplay);
							lcd_puts(" ");
						sprintf(buffer, "%1.1fV", voltage);
						lcd_puts(buffer);
						lcd_puts("   ");
					}
		}


		//display throttle

		lcd_goto(throttleDisplay);
		if ((throttle < 200) && (throttle < 300))
			lcd_puts("      ");
		if ((throttle >= 200) && (throttle < 400))
			lcd_puts("|     ");
		if ((throttle >= 400) && (throttle < 500))
			lcd_puts("||    ");
		if ((throttle >= 500) && (throttle < 600))
			lcd_puts("|||   ");
		if ((throttle >= 600) && (throttle < 700))
			lcd_puts("||||  ");
		if ((throttle >= 700) && (throttle < 800))
			lcd_puts("||||| ");
		if ((throttle >= 800))
			lcd_puts("||||||");

		//display kers


		if (kers == 1) {
					PORTC = (1<<PC5);
					lcd_goto(kersDisplay);
					if ((difference < 0.1))
						lcd_puts("      ");
					if ((difference >= 0.1) && (difference < 0.2))
						lcd_puts("     |");
					if ((difference >= 0.2) && (difference < 0.3))
						lcd_puts("    ||");
					if ((difference >= 0.3) && (difference < 0.4))
						lcd_puts("   |||");
					if ((difference >= 0.4) && (difference < 0.5))
						lcd_puts("  ||||");
					if ((difference >= 0.5) && (difference < 0.6))
						lcd_puts(" |||||");
					if ((difference >= 0.6))
						lcd_puts("||||||");


		} else {
			PORTC &= ~(1<<PC5);
			lcd_goto(kersDisplay);
			lcd_puts("      ");
		}

	}
}

