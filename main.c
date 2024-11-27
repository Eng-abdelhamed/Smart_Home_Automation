#include "STD_TYPEs.h"
#include "BIT_MATH.h"
#include "DIO_interface.h"
#include "PORT_interface.h"
#include "UART_interface.h"
#include "I2C_Interface.h"
#include"SPI_interface.h"
#include "EEPROM_INTERFACE.h"
#include "KEYBAD_interface.h"
#include "CLCD_interface.h"
#include "util/delay.h"

#define PASSWORD_LENGTH 4
#define MAX_ATTEMPTS 3
#define PASSWORD_ADDRESS 0x0000
#define CHECKER_ADDRESS  0x3111

void SetNewPassword(void);
u8 VerifyPassword(void);
void GetUserInput(u8* buffer, u8 length, u8 row, u8 start_col);
u8 ComparePasswords(const u8* password1, const u8* password2, u8 length);
void  SetNewPassWithConfirmation(void);

SPI_T spi_master =
{
    .MASTER_SLAVE = MASTER,
    .HIGH_SPEED   = HIGH_SPEED_ENABLE,
    .DTA          = LSB_FIRST,
    .PHASE        = LEADINGEDGE_SAMPLING_TRAILING_EDGE_SETUP,
    .CLK_POL      = LEADINGEDGE_RISING_TRAILING_EDGE_FALLING,
    .CLK          = DIVBY16
};
u8 spi_status;
void main(void)
{
    u8 checker = 0;
    u8 user_choice;
    //DIO_u8SetPinDirection(DIO_u8PORTA, DIO_u8PIN0, DIO_u8PIN_INPUT);
        DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN4, DIO_u8PIN_Output);  // ss
        DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN5, DIO_u8PIN_Output);  // MOSI
        DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN6, DIO_u8PIN_INPUT);   // MISO
        DIO_u8SetPinDirection(DIO_u8PORTB, DIO_u8PIN7, DIO_u8PIN_Output);  // sck
    // Initialization
    SPI_INIT(&spi_master);
    PORT_voidInit();
    CLCD_voidInitialize();
    I2C_INIT();


    EEPROM_ReadByte(CHECKER_ADDRESS, &checker);
    if (checker != 0x01)
    {
        SetNewPassword();
    } else
    {
        // Password verification loop
        while (1)
        {
        	DIO_u8SetPinValue(DIO_u8PORTB, DIO_u8PIN4, DIO_u8PIN_LOW);
        	spi_status = SPI_u8Send_Recieve('0');


        	CLCD_voidClearScreen();
        	CLCD_voidGotoX_Y(0,2);
        	CLCD_voidSendString("1:Login 2:set");
        	do
        	{
        		user_choice = KBD_u8GetPressedKey();
        	}while(user_choice == 0xff);

        	if(user_choice == '1')
        	{
        		if (VerifyPassword())
        		{
        		    CLCD_voidClearScreen();
        		    CLCD_voidGotoX_Y(0,2);
        		    CLCD_voidSendString("Access Granted");
        		   SPI_u8Send_Recieve('1');
        		    _delay_ms(100);
        		    break; // Exit after successful login
        		}
        	}
        	else if(user_choice == '2')
        	{
        		SetNewPassWithConfirmation();
        	}
        	else
        	{
        		CLCD_voidClearScreen();
        		CLCD_voidGotoX_Y(0,2);
        		CLCD_voidSendString("Invalid Choice");
        		_delay_ms(1000);

        	}
        }
        // open the door
        CLCD_voidClearScreen();
        CLCD_voidGotoX_Y(0,1);
        CLCD_voidSendString("Hello Sir :)");
        CLCD_voidGotoX_Y(1,1);
        CLCD_voidSendString("DOOR OPEN");
        for(int i = 0 ;i < 5 ;i++)
        {
        	DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN2,DIO_u8PIN_HIGH);
        	DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN4,DIO_u8PIN_HIGH);
        	DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN3,DIO_u8PIN_LOW);
        	DIO_u8SetPinValue(DIO_u8PORTB,DIO_u8PIN0,DIO_u8PIN_LOW);
        	CLCD_voidGotoX_Y(1,10+i);
            CLCD_voidSendString(".");
            _delay_ms(100);
        }
        DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN2,DIO_u8PIN_LOW);
        DIO_u8SetPinValue(DIO_u8PORTC,DIO_u8PIN4,DIO_u8PIN_LOW);
        _delay_ms(1000);


    }
}

void SetNewPassword(void)
{
    u8 Password_First_Enter[PASSWORD_LENGTH] = {0};

    CLCD_voidClearScreen();
    CLCD_voidGotoX_Y(0, 2);
    CLCD_voidSendString("Set Pass:");

    // Get the new password from the user
    GetUserInput(Password_First_Enter, PASSWORD_LENGTH, 0, 11);

    // Store the password in EEPROM
    for (u8 i = 0; i < PASSWORD_LENGTH; i++) {
        EEPROM_WriteByte(PASSWORD_ADDRESS + i, Password_First_Enter[i]);
        _delay_ms(10); // Ensure safe EEPROM write
    }

    // Mark password as set
    EEPROM_WriteByte(CHECKER_ADDRESS, 0x01);

    CLCD_voidClearScreen();
    CLCD_voidGotoX_Y(0, 3);
    CLCD_voidSendString("Pass Saved");
    _delay_ms(100);
}

u8 VerifyPassword(void)
{
    u8 Password[PASSWORD_LENGTH] = {0};
    u8 StoredPassword[PASSWORD_LENGTH] = {0};
    u8 attempts = 0;

    while (attempts < MAX_ATTEMPTS) {
        CLCD_voidClearScreen();
        CLCD_voidGotoX_Y(0, 3);
        CLCD_voidSendString("Enter Pass:");

        // Get user input
        GetUserInput(Password, PASSWORD_LENGTH, 1, 4);

        // Read stored password from EEPROM
        for (u8 i = 0; i < PASSWORD_LENGTH; i++) {
            EEPROM_ReadByte(PASSWORD_ADDRESS + i, &StoredPassword[i]);
            _delay_ms(10); // Ensure safe EEPROM read
        }

        // Compare entered and stored passwords
        if (ComparePasswords(Password, StoredPassword, PASSWORD_LENGTH)) {
            return 1; // Password is correct
        }

        // Incorrect password handling
        attempts++;
        CLCD_voidClearScreen();
        CLCD_voidGotoX_Y(0, 3);
        CLCD_voidSendString("Wrong Pass!");
        _delay_ms(1000);

        if (attempts >= MAX_ATTEMPTS) {
            CLCD_voidClearScreen();
            CLCD_voidGotoX_Y(0, 2);
            CLCD_voidSendString("Locked Out!");
            while (1); // Lock the system indefinitely
        }
    }
    return 0;
}

void GetUserInput(u8* buffer, u8 length, u8 row, u8 start_col) {
    u8 counter = 0;
    u8 key;

    while (counter < length) {
        do {
            key = KBD_u8GetPressedKey();
        } while (key == 0xFF); // Wait for valid key press

        buffer[counter] = key;
        CLCD_voidGotoX_Y(row, start_col + counter);
        CLCD_voidSendData(key);
        _delay_ms(200);
        CLCD_voidGotoX_Y(row, start_col + counter);
        CLCD_voidSendData('*'); // Replace with asterisk for security
        counter++;
    }
}

u8 ComparePasswords(const u8* password1, const u8* password2, u8 length) {
    for (u8 i = 0; i < length; i++) {
        if (password1[i] != password2[i]) {
            return 0; // Passwords do not match
        }
    }
    return 1; // Passwords match
}
void SetNewPassWithConfirmation(void)
{
	u8 current_Pass[PASSWORD_LENGTH]={0};
	u8 Stored_pass[PASSWORD_LENGTH] ={0};
	u8 New_Pass[PASSWORD_LENGTH]={0};
	u8 Re_enter_pass[PASSWORD_LENGTH]={0};
	//Enter CurrentPass
	CLCD_voidClearScreen();
	CLCD_voidGotoX_Y(0,2);
	CLCD_voidSendString("CurrentPass:");
	GetUserInput(current_Pass,PASSWORD_LENGTH,1,4);

	for(int i = 0 ;i  < PASSWORD_LENGTH ;i++)
	{
		EEPROM_ReadByte(PASSWORD_ADDRESS+i,&Stored_pass[i]);
		_delay_ms(10);
	}
    // Check For Pass
	if(!ComparePasswords(current_Pass,Stored_pass,PASSWORD_LENGTH))
	{
		CLCD_voidClearScreen();
		CLCD_voidGotoX_Y(0,2);
		CLCD_voidSendString("WrongPass:");
		_delay_ms(100);
		return ;  // Exit for Wrong Pass
	}
	// Prompt for a new password
	CLCD_voidClearScreen();
	CLCD_voidGotoX_Y(0,2);
	CLCD_voidSendString("New Pass :");
	GetUserInput(New_Pass,PASSWORD_LENGTH,1,4);

	// Confirm the new password
	CLCD_voidClearScreen();
	CLCD_voidGotoX_Y(0,2);
	CLCD_voidSendString("Re-Enter Pass:");
	GetUserInput(Re_enter_pass,PASSWORD_LENGTH,1,4);
	// Compare the two new passwords
	if(!ComparePasswords(current_Pass,Stored_pass,PASSWORD_LENGTH))
	{
		CLCD_voidClearScreen();
		CLCD_voidGotoX_Y(0,3);
		CLCD_voidSendString("MissMatch:");
		_delay_ms(100);
		return ;  // Exit for Wrong Pass
	}
	// save NewPass in EEprom
	for(int i =0 ;i < PASSWORD_LENGTH ;i++)
	{
		EEPROM_WriteByte(PASSWORD_ADDRESS+i,New_Pass[i]);
		_delay_ms(10);
	}
	CLCD_voidClearScreen();
	CLCD_voidGotoX_Y(0,3);
	CLCD_voidSendString("PassUpdated:)");
	_delay_ms(100);
}
