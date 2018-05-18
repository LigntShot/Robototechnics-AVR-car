﻿#include <avr/interrupt.h>
#include <avr/iom16.h>
#include <avr/sfr_defs.h>
#include <stdint.h>

//TODO: Снизить скорость, ЗАСТАВИТЬ ВСЕ ЭТО РАБОТАТЬ!

#define DETECTOR_DIR DDRB

#define DETECTOR_PORT PORTB

#define DETECTOR_PIN PINB

#define COM_DETECTOR_INPUT PB2 //ЧТЕНИЕ

//ЗАПИСЬ
#define COM_DETECTOR_OUTPUT_LEFT PB4
#define COM_DETECTOR_OUTPUT_RIGHT PB1

#define ENGINE_LEFT PD7
#define ENGINE_RIGHT PD5

#define ENGINE_DIR DDRD

#define ENGINE_PORT PORTD

#define LEFT ENGINE_PORT |= _BV(ENGINE_LEFT)
#define RIGHT ENGINE_PORT |= _BV(ENGINE_RIGHT)

#define STOP_LEFT ENGINE_PORT &= ~_BV(ENGINE_LEFT)
#define STOP_RIGHT ENGINE_PORT &= ~_BV(ENGINE_RIGHT)

#define INPUT_PIN PINA

#define pi 3.14
#define desired_speed 30
#define k 1
#define dt 1

volatile uint8_t cmd = 0;
volatile bool left_wh = 1;
volatile bool right_wh = 1;
volatile uint8_t detector = 0;
volatile uint8_t cnt0 = 0;
volatile uint8_t cnt1 = 0;
volatile uint8_t cnt2 = 0;
volatile bool left_engine_run = false;
volatile bool right_engine_run = false;

volatile uint8_t speed_cnt_left = 0;
volatile uint8_t speed_cnt_right = 0;
volatile uint8_t timer_cnt = 0;

volatile uint8_t speed_left = 0;
volatile uint8_t speed_right = 0;

volatile uint8_t pwm_left = 0;
volatile uint8_t pwm_right = 0;

volatile bool remove_counter = 0;

uint8_t desired_speed_left = desired_speed;
uint8_t desired_speed_right = desired_speed;

void
configure_pins()
{
	DETECTOR_DIR |= (_BV(COM_DETECTOR_OUTPUT_LEFT) | _BV(COM_DETECTOR_OUTPUT_RIGHT)); //0 - НАСТРОЙКА ПИНА НА ЗАПИСЬ
	DETECTOR_DIR &= ~_BV(COM_DETECTOR_INPUT); //1 - НАСТРОЙКА ПИНА НА ЧТЕНИЕ

	DETECTOR_PORT &= ~(_BV(COM_DETECTOR_OUTPUT_LEFT) | _BV(COM_DETECTOR_OUTPUT_RIGHT)); //ЗАПИСЫВАЕМ В ПОРТ НУЛИ
	//DETECTOR_PIN &= ~(_BV(COM_DETECTOR_INPUT)); //ЗАПИЫВАЕМ В ПИН НОЛЬ
	ENGINE_DIR |= _BV(ENGINE_LEFT) | _BV(ENGINE_RIGHT);
	ENGINE_PORT &= ~(_BV(ENGINE_LEFT) | _BV(ENGINE_RIGHT));
}

void
configure_timer()
{
	TCCR0 = _BV(CS02);
	TIMSK |= _BV(TOIE0);
}

bool
counter(uint8_t target, uint8_t max = 100) //ФУНКЦИЯ ОБНУЛЕНИЯ СЧЁТЧИКА
{
	/*cnt0++;
	if (cnt0 == 10) cnt0 = 0;
	return cnt0;*/

	if (cnt0 > max) cnt0 = 0;
	return cnt0++ > target;
}

bool
delay_counter(uint8_t target) //ФУНКЦИЯ ОБНУЛЕНИЯ СЧЁТЧИКА
{
	/*cnt2++;
	if (cnt2 == 92) cnt2 = 0;
	return cnt2;*/

	/*if (cnt2 > max) cnt2 = 0;
	return cnt2++ > target;*/

	if(remove_counter) {
			cnt0 = 0;
			remove_counter = false;
		}

		if (cnt0 > target) remove_counter = true;

		return cnt0++ > target;
}

bool
pwm_counter_left(uint8_t target, uint8_t max = 20)
{
	if (cnt1 > max) cnt1 = 0;
	return cnt1++ > target;
}

bool
pwm_counter_right(uint8_t target, uint8_t max = 20)
{
	if (cnt2 > max) cnt2 = 0;
	return cnt2++ > target;
}

ISR(INT0_vect)
{
	speed_cnt_left++;
}

ISR(INT1_vect)
{
	speed_cnt_right++;
}

ISR(TIMER0_OVF_vect)
{
	/*speed_left = speed_cnt_left * dt;
	speed_right = speed_cnt_right * dt;

	speed_cnt_left = 0;
	speed_cnt_right = 0;

	pwm_left  += k * (desired_speed_left - speed_left);
	pwm_right += k * (desired_speed_right - speed_right);*/

	switch(cmd) {
	case 0: //ВКЛЮЧЕНИЕ ЛЕВОГО ДАТЧИКА

		DETECTOR_PORT |= _BV(COM_DETECTOR_OUTPUT_LEFT);

		cmd = 1;

		pwm_left = 10;
		pwm_right = 10;

		break;

	case 1: //НИЧЕГО НЕ ДЕЛАТЬ

		if (delay_counter(8)) cmd = 2;

		break;

	case 2: // СЧИТЫВАЕНИЕ ИЗ ЛЕВОГО ДАТЧИКА
		/*if (DETECTOR_PIN & _BV(COM_DETECTOR_INPUT)) right_wh = true;//ENGINE_PORT |= _BV(ENGINE_RIGHT)
		//if (detector == 0b00000100) right_wh = true;
		if(!right_wh) right_engine_run = false;
		cmd = 3;*/

		if (DETECTOR_PIN & _BV(COM_DETECTOR_INPUT)) right_wh = true;

						if(!right_wh) {
							pwm_left = 20;
							pwm_right = 0;
						}

						cmd = 3;
						/*if(delay_counter(pwm_left) && !right_wh) {
									pwm_left = 10;
									pwm_right = 10;
									cmd = 3;
								}*/

		break;

	case 3: //ВЫКЛЮЧЕНИЕ ЛЕВОГО ДАТЧИКА
		DETECTOR_PORT &= ~(_BV(COM_DETECTOR_OUTPUT_LEFT));
		cmd = 4;
		break;
	case 4:
		DETECTOR_PORT |= _BV(COM_DETECTOR_OUTPUT_RIGHT);
		cmd = 5;
		break;
	case 5:
		if(delay_counter(8)) cmd = 6;
		break;
	case 6:
		if (DETECTOR_PIN & _BV(COM_DETECTOR_INPUT)) left_wh = true;
		/*//if (detector == 0b00000100) left_wh = true;
		if(!left_wh) left_engine_run = false;
		cmd = 7;*/

		if(!left_wh) {
					pwm_left = 0;
					if(pwm_right != 0) pwm_right = 20;
				}
		cmd = 7;

								/*if(delay_counter(8) && !left_wh) {
											pwm_left = 10;
											pwm_right = 10;
											cmd = 7;
										}*/

		break;
	case 7:
		DETECTOR_PORT &= ~(_BV(COM_DETECTOR_OUTPUT_RIGHT));
		cmd = 8;
		break;
	case 8:
		/*if(left_wh) {
			left_engine_run = true;
			//ENGINE_PORT |= _BV(ENGINE_LEFT);
		}
		if(right_wh) {
			right_engine_run = true;
			//ENGINE_PORT |= _BV(ENGINE_RIGHT);
		}
		if(delay_counter(pwm_left)) {
			cmd = 0;
			left_wh = false;
		}
		if (delay_counter(pwm_right))
		{
			cmd = 0;
			right_wh = false;
		}*/

		if (!pwm_counter_left(pwm_left)) ENGINE_PORT |= _BV(ENGINE_LEFT);
		else ENGINE_PORT &= ~_BV(ENGINE_LEFT);

		if (!pwm_counter_right(pwm_right)) ENGINE_PORT |= _BV(ENGINE_RIGHT);
		else ENGINE_PORT &= ~_BV(ENGINE_RIGHT);

		if(delay_counter(100) && (!right_wh || !left_wh)) cmd = 0;
		else cmd = 0;

	}
	/*if (left_engine_run & pwm_counter(pwm_left)) ENGINE_PORT |= _BV(ENGINE_LEFT);
	else ENGINE_PORT &= ~_BV(ENGINE_LEFT);

	if (right_engine_run & pwm_counter(pwm_right)) ENGINE_PORT |= _BV(ENGINE_RIGHT);
	else ENGINE_PORT &= ~_BV(ENGINE_RIGHT);*/
}

int
main(void)
{
	cli();

	configure_pins();
	configure_timer();

	sei();

	while (1)
	{
		detector = DETECTOR_PIN;
		detector &= 0b00000100;
	}
	return 0;
}
