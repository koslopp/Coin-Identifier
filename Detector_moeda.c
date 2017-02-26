///////////////////////////////////////////////////////////////////////////
////                             MAIN.C                                ////
////             Generic Microgenios PIC18F2550 main file              ////
////                                                                   ////
////          Daniel Koslopp / Talita Tobias Carneiro - 2013           ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////

#include <18f2550.h>    
#fuses HS,NOWDT,NOPROTECT,NOLVP 

#use delay(CLOCK=20000000)   //PICGENIOS PIC16F877A clock

#use fast_io(A)      //Permite leitura e escrita nos PORTs
#use fast_io(B)      //||
#use fast_io(C)      //||


//Defines

#define RS PIN_C0    //RS no pino C2
#define EN PIN_C1    //EN no pino C1
#define D7 PIN_B7    //D7 LCD
#define D6 PIN_B6    //D6 LCD
#define D5 PIN_B5    //D5 LCD
#define D4 PIN_B4    //D4 LCD
#define lcd_comando(x)  lcd_escreve(0,x)  //Escreve comando LCD
#define lcd_letra(x)    lcd_escreve(1,x)  //Escreve letra LCD
//Estabiliza LCD
#define lcd_clear()\
        lcd_comando(0x01)\    
        delay_ms(2)
//Move cursor LCD                        
#define lcd_gotoxy(l,c)\   
        lcd_comando(0x80+0x40*l+c)
//Desliga cursor LCD
#define lcd_cursor_off()\
        lcd_comando(0b00001100)
//Liga cursor LCD
#define lcd_cursor_on()\
        lcd_comando(0b00001110)
//Pisca cursor LCD
#define lcd_cursor_pisca()\
        lcd_comando(0b00001111)

//Define endereço registradores

#byte OPTION_REG=0x181
#byte INTCON=0xFF2
#byte INTCON2=0xFF1
#byte INTCON3=0xFF0
#byte T1CON=0xFCD
#byte TMR1H=0xFCF
#byte TMR1L=0xFCE

#bit GIE=INTCON.7
#bit PEIE=INTCON.6
#bit INT0IE=INTCON.4
#bit INTEDG0=INTCON2.6
#bit INT0IF=INTCON.1
#bit RD16=T1CON.7
#bit T1SYNC=T1CON.2
#bit TMR1CS=T1CON.1
#bit TMR1ON=T1CON.0
#bit T1CKPS1=T1CON.5
#bit T1CKPS0=T1CON.4

//Global variables

char ch0='0';
char ch1='0'; 
char ch2='0';
char ch3='0';
char ch4='0';
int16 TEMPO=0;

//Interruções



//System initialization

void init() //Condições Iniciais
{
   GIE=1;   //Habilita interrupções
   PEIE=1;  //Habilita periféricos (TMR1)
   INT0IE=1;   //Habilita intr. externa 0 (RB0)
   INTEDG0=1;  //Intr. externa 0 por borda de subida
   RD16=0;  //TMR1 em 16 bits
   T1CKPS1=0;  //T1CKPS1:T1CKPS0 = 00 (Prescale 1:1)
   T1CKPS0=0;
   T1SYNC=1;   //Sem sincronização externa
   TMR1CS=0;   //Clock = Fosc/4
   TMR1ON=1;   //Começa desligado
   set_tris_a(0xFF);    //PORTA como entrada
   set_tris_b(0x0F);    //PORTB como saída <7:4> e entrada <3:0>
   set_tris_c(0xFC);    //PORTC como entrada <7:2> e saída <1:0>
   output_b(0x00);      //Zera saidas PORTB
   output_c(0x00);      //Zera saidas PORTC
}

// Funções Auxiliares

void lcd_escreve(boolean BITRS, int8 ch)  //Escreve no lcd em 4 bits
{
   output_bit(RS,BITRS);            //Comando BITRS=0 ou dados BITRS=1)
   delay_us(100);                   //Estabiliza pino LCD
   output_bit(D7,bit_test(ch,7));   //Envia nibble (high) ao barramento LCD  
   output_bit(D6,bit_test(ch,6));
   output_bit(D5,bit_test(ch,5));
   output_bit(D4,bit_test(ch,4));
   output_high(EN);                 //Enable vai para 1
   delay_us(1);                     //Establiza Enable
   output_low(EN);                  //Enable vai para 0 (envia comando)
   delay_us(100);                   //Estabiliza LCD
   output_bit(D7,bit_test(ch,3));   //Envia nibble (low) ao barramento LCD 
   output_bit(D6,bit_test(ch,2));
   output_bit(D5,bit_test(ch,1));
   output_bit(D4,bit_test(ch,0));
   output_high(EN);                 //Enable vai para 1
   delay_us(1);                     //Establiza Enable
   output_low(EN);                  //Enable vai para 0 (envia comando)
   delay_us(100);                   //Estabiliza LCD e aguarda LCD
}

void lcd_init()   // Inicializacao LCD
{
   delay_ms(30);
   lcd_escreve(0,0b00110010); //Function set RS-->0 D7AO D0-->00110010
   lcd_escreve(0,0b00101000); //Function set RS-->0 D7AO D0-->00101000
   lcd_escreve(0,0b00101000); //Function set RS-->0 D7AO D0-->00101000
   lcd_escreve(0,0b00001111); //Display on/off control RS-->0 D7AO D0-->00001111
   lcd_escreve(0,0b00000001); //Display clear  RS-->0 D7AO D0-->00000001
   delay_ms(2);               //Aguarda tempo do LCD para display clear
   lcd_escreve(0,0b00000110); //Entry mode set  RS-->0 D7AO D0-->00000110
}

void lcd_string(char ch)      //Escreve string no LCD
{
   switch(ch){
      case'\n':lcd_escreve(0,0xc0);   //Pula linha
               break;
      case'\f':lcd_escreve(0,0x01);   //Limpa
               delay_ms(2);
               break;   
      default:lcd_escreve(1,ch);
   }   
}

void  lcd_bintodec(int16 val)   //Escreve numero binario na forma decimal no LCD
{
   ch0='0';
   ch1='0';
   ch2='0';
   ch3='0';
   ch4='0';
   while(val>9999)
   {
      val=val-10000;
      ch4++;
   }
   while(val>999)
   {
      val=val-1000;
      ch3++;
   }
   while(val>99)
   {
      val=val-100;
      ch2++;
   }
   while(val>9)
   {
      val=val-10;
      ch1++;
   }
   ch0=val+'0';
}

#int_ext
void trata_int_ext() //Trata a interrupção externa (pulso)
{
   INT0IF=0;   //Zera flag interrupção
   if(INTEDG0==1) //Se interrupção por borda se subida
   {
      TMR1H=0; //Zera TMR1H e TMR1L
      TMR1L=0; 
      TMR1ON=1;   //Liga TMR1
      INTEDG0=0;  //Próxima interrupção por borda de descida
   }
   else
   {
      TMR1ON=0;   //Para TMR1
      TEMPO=TMR1L+256*TMR1H;  //Manda tempo do TMR1 para variável de 16 bits
      INTEDG0=1;  //Próxima interrupção por borda de subida
   }
}

void determina_valor()
{
   lcd_gotoxy(1,0);
   lcd_string("MOEDA=");
   if((TEMPO>=0) && (TEMPO<=9999))
      lcd_string("0,05");
   if((TEMPO>=10000) && (TEMPO<=19999))
      lcd_string("0,10");
   if((TEMPO>=20000) && (TEMPO<=29999))
      lcd_string("0,25");
   if((TEMPO>=30000) && (TEMPO<=39999))
      lcd_string("0,50");
   if((TEMPO>=40000) && (TEMPO<=49999))
      lcd_string("1,00");
   if((TEMPO>=50000) && (TEMPO<=65535))
      lcd_string("ERRO");
}


// Main function: Code execution starts here 

void main()
{
   init();  //Condições iniciais
   lcd_init(); //Inicia LCD
   lcd_cursor_off(); //Desliga cursor LCD
   while(1) //Rotina
   {
      lcd_bintodec(TEMPO); //Converte para char decimal o tempo
      lcd_gotoxy(0,0);  //Posiciona cursor
      lcd_string("TEMPO="); //Escreve LCD
      lcd_string(ch4);  //Escreve algarismo serial mais significativo
      lcd_string(ch3);  //5 digitos decimais
      lcd_string(ch2);
      lcd_string(ch1);
      lcd_string(ch0);
      determina_valor();
      delay_ms(50);
   } 
}
