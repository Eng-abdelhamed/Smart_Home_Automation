#include "BIT_MATH.h"
#include "STD_TYPEs.h"
#include "MAP.h"
#include "PORT_interface.h"
#include "ADC_Interface.h"
#include "DIO_interface.h"
#include "TIMER_interface.h"
#include "EXTI_interface.h"
#include "GIE_interface.h"
#include "SPI_interface.h"
#include "CLCD_interface.h"
#include "DCMOTOR_interface.h"
#include "util/delay.h"

// Function prototypes
void LDR_Control_Light(u8 light_level);
void Lm35_Control_Fan(u16 temperature);
void Flame_Senor_ISR(void);

// SPI configuration
SPI_T spi_slave = {
    .MASTER_SLAVE = SLAVE,
    .HIGH_SPEED = HIGH_SPEED_DISABLE,
    .DTA = LSB_FIRST,
    .PHASE = LEADINGEDGE_SAMPLING_TRAILING_EDGE_SETUP,
    .CLK_POL = LEADINGEDGE_RISING_TRAILING_EDGE_FALLING
};

// ADC configuration
ADC_T adc = {
    .CLK = PRESCALALR_DIV128,
    .Volt_Select = AVCC,
    .Mode = FREE_Running_mode
};

// Motor configuration
Motor_t motor = {
    .motor_Pin[0] = { .Pin = DIO_u8PIN0, .Port = DIO_u8PORTB, .Motor_Status = MOTOR_OF },
    .motor_Pin[1] = { .Pin = DIO_u8PIN1, .Port = DIO_u8PORTB, .Motor_Status = MOTOR_OF }
};

u16 ldr_value = 0, lm35_value = 0;
u8 light_level = 0;
u16 temperature = 0;
volatile u8 flame_sensor = 0;
u8 spi_status = 0;
u8 pin_status;
// SPI Commands
#define FLAME_ALERT 'k'
#define CLEAR_ALERT 'o'
#define DATA_REQUEST '1'

void main(void)
{
    // Initialize pin directions
    DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN5, DIO_u8PIN_Output);  // MISO
    DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN4, DIO_u8PIN_INPUT);   // SS
    DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN6, DIO_u8PIN_INPUT);   // MOSI
    DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN7, DIO_u8PIN_INPUT);   // SCK
    DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN1, DIO_u8PIN_Output);  // Motor control
    DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN0, DIO_u8PIN_Output);

    PORT_voidInit();
    SPI_INIT(&spi_slave);
    //GIE_voidEnable();
    ADC_INIT(&adc);
    //EXTI_voidInt2init();
    //EXTI_u8INT2SetSenseCtrl(RISING_EDGE);
    //EXIT_u8int2CallBack(&Flame_Senor_ISR);

    DC_Motor_Init(&motor);
    TIMER0_voidInit();
    CLCD_voidInitialize();

    while (1)
    {
        spi_status = SPI_u8Send_Recieve('0');
        while(spi_status == '1')
        {
        	DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN1, DIO_u8PIN_LOW);
        	DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN0, DIO_u8PIN_LOW);
        	do
        	{
        	 DIO_u8GetPinValue(DIO_u8PORTB, DIO_u8PIN2, &pin_status);
             ADC_GETCONVERSION_POLLING(&adc, 0, &lm35_value);
             temperature = ((u32)lm35_value * 5000) / 1024 / 10;
             CLCD_voidGotoX_Y(0, 0);
             CLCD_voidSendString("Temp:     ");
             CLCD_voidWriteNumber(temperature);
             CLCD_voidSendString("C");
             ADC_GETCONVERSION_POLLING(&adc, 1, &ldr_value);
             light_level = Map(1019, 93, 0, 3, (u32)ldr_value);
             CLCD_voidGotoX_Y(1, 0);
             CLCD_voidSendString("Light:    ");
             CLCD_voidWriteNumber(light_level);
             Lm35_Control_Fan(temperature);
             LDR_Control_Light(light_level);
        	}while(pin_status == 0);
            while(pin_status == 1)
            {
            	CLCD_voidClearScreen();
            	DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN1, DIO_u8PIN_HIGH);
            	DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN0, DIO_u8PIN_HIGH);
            	CLCD_voidGotoX_Y(0,4);
            	CLCD_voidSendString("Flame Alert");
            	_delay_ms(500);
            	DIO_u8GetPinValue(DIO_u8PORTB, DIO_u8PIN2, &pin_status);
            }

        }


     /*   while (spi_status == DATA_REQUEST)
        {
        	DIO_u8GetPinValue(DIO_u8PORTB, DIO_u8PIN2, &pin_status);
            // Handle flame alert
            if (pin_status == 1)
            {
            	SPI_u8Send_Recieve(FLAME_ALERT);
                CLCD_voidClearScreen();
                CLCD_voidGotoX_Y(0, 4);
                CLCD_voidSendString("FLAME ALERT");

                DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN1, DIO_u8PIN_HIGH);
                DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN0, DIO_u8PIN_HIGH);

                while (pin_status == 1)
                {
                    DIO_u8GetPinValue(DIO_u8PORTB, DIO_u8PIN2, &pin_status);
                    _delay_ms(10);
                }

            }
            DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN1, DIO_u8PIN_LOW);
            DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN0, DIO_u8PIN_LOW);
            ADC_GETCONVERSION_POLLING(&adc, 0, &lm35_value);
            temperature = ((u32)lm35_value * 5000) / 1024 / 10;
            CLCD_voidGotoX_Y(0, 0);
            CLCD_voidSendString("Temp: ");
            CLCD_voidWriteNumber(temperature);
            CLCD_voidSendString("C");
            ADC_GETCONVERSION_POLLING(&adc, 1, &ldr_value);
            light_level = Map(1019, 93, 0, 3, (u32)ldr_value);
            CLCD_voidGotoX_Y(1, 0);
            CLCD_voidSendString("Light: ");
            CLCD_voidWriteNumber(light_level);
            Lm35_Control_Fan(temperature);
            LDR_Control_Light(light_level);
        }*/
    }
}

void Lm35_Control_Fan(u16 temp)
{
    if (temp < 30)
    {
        DC_Motor_Stop(&motor);
        timer0_voidSetCompareValue(0);
    }
    else if (temp < 40)
    {
        timer0_voidSetCompareValue(55); // Fan at 25% speed
    }
    else if (temp < 50)
    {
        timer0_voidSetCompareValue(75); // Fan at 50% speed
    }
    else
    {
        timer0_voidSetCompareValue(100); // Fan at 100% speed
    }
}

void LDR_Control_Light(u8 light_level)
{
    static const u8 port_values[] = { 0x00, 0x20, 0x60, 0xE0 };
    DIO_u8SetPortValue(DIO_u8PORTA, port_values[light_level]);
}
