
/****************************************************************************************************************************************
***************************************** Programa de control de rotor TAGRA RT50 *******************************************************
*****************************************************************************************************************************************
última actualización: V2.7.  3 jun 2020
histórico de versiones:
-v2.7: añadido comandos de teclado goto stop
-V2.4:añadido teclado
-v2.0:añadido LCD
Comandos de Teclado:
-D: borra el comando introducido.
-C: Ejecuta el comado introducido (GOTO).
-A: Aborta el comando introducido (STOP).
-*: Punto decimal.
-#: Cambiar tiempo de rotación (no implementado)


// pinout.
//SDA A4  I2C
//SCL A5
 */

//*************************************** Definición de Variables y librerías *************************************************** 

String salida;

char customKey;

bool flagAuto=false;
bool flagAutoMas=false;
bool flagAutoMenos=false;
bool puls;
bool derechas=false; //
bool izquierdas=false; //

int tHeartBeat=1000;// tiempo de parpadeo led on
float tiempoAhora=0; // captura de tiempo para heartbeat
float tiempoDerechas=0; //
float tiempoIzquierdas=0; //
float millisVuelta=69900; //Milisegundos/vuelta
float actualPos; //posición actual
float dinActualPos;
float grados=0;
float comando;

//____________________________________

//************************ teclado

#include <Keypad.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the Symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
   { '1','2','3', 'A' },
   { '4','5','6', 'B' },
   { '7','8','9', 'C' },
   { '.','0','*', 'D' }
};
#define KBc1 3//Teclado matricial.columna 1// Definición de pines a los que conectamos el teclado.
#define KBc2 4//Teclado matricial.columna 2
#define KBc3 5//Teclado matricial.columna 3
#define KBc4 6//Teclado matricial.columna 4
#define KBf1 A0//Teclado matricial.fila 1
#define KBf2 A1//Teclado matricial.fila 2
#define KBf3 A2//Teclado matricial.fila 3
#define KBf4 A3//Teclado matricial.fila 4
byte rowPins[ROWS] = {KBf1, KBf2, KBf3, KBf4 }; //connect to the row pinouts of the keypad
byte colPins[COLS] = { KBc1, KBc2, KBc3, KBc4}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
//_____________________________________


//*************************** LCD
#include <LiquidCrystal_I2C.h>//libreria LCD
//definición de LCD
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
//_____________________________________
//definición de pines para pulsadores y led
#define pinPBi 7 //Pin pulsador Izquierda
#define pinPBd 8 //Pin pulsador Derecha
#define pinRM  9 //Pin Relé marcha. Conecta la alimetación del transformador.
#define pinRI  10 //Pin Relé derecha(+ grados)Norte0º/360º Eete 90º sur 180º Oeste270º. Relé off:giro derechas (sentido + azimuth).Relé On:Giro Izauierdas (sentido - azimuth)
#define pinOn  11 //Pin LED on
#define pinLedIzq 12 //Pin LED Derecha (sentido + azimuth)
#define pinLedDer 13 //Pin LED Izquierda (sentido - azimuth)


void setup() {  //******************** Definición de I/O
pinMode(pinPBi,INPUT_PULLUP) ;
pinMode(pinPBd,INPUT_PULLUP) ;
pinMode(pinRM,OUTPUT); 
pinMode(pinRI,OUTPUT); 
pinMode(pinOn,OUTPUT); 
pinMode(pinLedIzq,OUTPUT); 
pinMode(pinLedDer,OUTPUT); 
//*********************************** inicialización
  /* PORTS:
    B (pines digitales del 8 al 13)
    C (entradas analógicas)
    D (pines digitales del 0 al 7)
    */

PORTB=0x00; // Escribe 00 en el port B (pone todos los bits a 0)   --> todas las salidas a 0
PORTD=0x00; // Escribe 00 en el port B (pone todos los bits a 0)  

 
  Serial.begin(9600);// inicialización puerto serie integrado(sólo para debug)
  
  contaje();//inicialización del contaje. llama a la función contaje para inicializar la posición a cero
  
  //************* inicializa LCD
  lcd.init();    
  lcd.backlight(); //enciende retroiluminación.   
    lcd.setCursor(0,0);//columna, fila
  lcd.print("RT50 EA5LN");
  lcd.setCursor(0,1);
  lcd.print("CoronaTrol 2020");
  delay(2000)  ;    
//Pantalla principal
  lcd.setCursor(0,0);
  lcd.print("Actual   Comando");
  lcd.setCursor(0,1);
  lcd.print("         0      ");
//*************** inicialización de comando
 comando=0.00;//Poner a cero el comando
  salida="";
  gestionTeclado();
}// End setup

void loop() {//***************************** Programa principal
  //*********************** límites de recorrido
  bool limitMinus=false;
  bool limitPlus=false;
if (grados <=0) {limitMinus=true;}//limite menos alcanzado //comentar para anular límites de recorrido
if (grados >=365) {limitPlus=true;}//limite mas alcanzado  //comentar para anular límites de recorrido
//*************** visualización de posición
  lcd.setCursor(0,1);
  lcd.print(grados,0);
  lcd.print("   ");
//*********** Pulsadores de Giro
if((digitalRead(pinPBi)==LOW && digitalRead(pinPBd)==HIGH && limitPlus==false)||flagAutoMas) //Derechas

      {  //Código girar derechas

      digitalWrite(pinRI,LOW);
      delay(100);
      digitalWrite(pinRM,HIGH);
      digitalWrite(pinLedIzq,HIGH);
      digitalWrite(pinLedDer,LOW);
      digitalWrite(pinOn,LOW);
      if (derechas==false) {
            tiempoDerechas=millis(); 

             derechas=true;
            }
       contajeDinamico();
      }    
else if((digitalRead(pinPBi)==HIGH && digitalRead(pinPBd) ==LOW && limitMinus==false)||flagAutoMenos) //Izquierdas
      {  //Código girar Izquierdas
            digitalWrite(pinRI,HIGH);//activa relé de inversión (giro izquierdas)
            delay(100);
            digitalWrite(pinRM,HIGH);
            digitalWrite(pinLedIzq,LOW);
            digitalWrite(pinLedDer,HIGH);
            digitalWrite(pinOn,LOW);     
      if (izquierdas==false) {// capturamos el valor de millis actual, para empezar a contar
          tiempoIzquierdas=millis(); 
          izquierdas=true;
          }
      contajeDinamico();//actualizar contaje
      }
//*********************** Paro (ningún pulsador de giro pulsado o se ha llegado a límite de recorrido en ese sentido)      
else if((digitalRead(pinPBi)==LOW && digitalRead(pinPBd) ==LOW&&flagAuto==false)//parada
            ||(digitalRead(pinPBi)==HIGH && limitMinus==true)//Pulsador menos y límite menos
            ||(digitalRead(pinPBd)==HIGH && limitPlus==true)//Pulsador mas y límite mas
            ) //Paro
      {
         if (derechas == true or izquierdas ==true) {
          contaje();//Paramos el contaje
      digitalWrite(pinRM,LOW);//Quitamos relé de marcha primero para evitar que invierta sentido de giro
      delay(100);//esperamos 0.1 seg a que el relé caiga
      digitalWrite(pinRI,LOW);//Quitames el relé de sentido izquierdas (si estba activado)
      digitalWrite(pinLedIzq,LOW);//apaga LED izquierda
      digitalWrite(pinLedDer,LOW);//apaga LED Derecha
          }

heartBeat();//Parpadea el led ON 

      }
 gestionTeclado(); // si no hacemos nada con los botones (ambos a low) leemos el teclado     
if (flagAuto==true){
    automatico();
    }
}//End loop


void heartBeat() //parpadea el led on
{

     if (millis() > tiempoAhora + tHeartBeat){
              digitalWrite(pinOn,!digitalRead(pinOn));
               tiempoAhora = millis();
               }
}


void contaje(){
  
  if(derechas == true){
          float ahora=millis();
          float transcurrido=ahora-tiempoDerechas;
          actualPos += transcurrido ;              
          derechas=false;
          }
  if(izquierdas == true){
        float ahora=millis();
        float transcurrido=ahora-tiempoIzquierdas;
        actualPos -= transcurrido;
        izquierdas=false;
        }
grados=(actualPos/millisVuelta)*365;
}



void contajeDinamico(){
  
  if(derechas == true){
          float ahora=millis();
          float transcurrido=ahora-tiempoDerechas;
          dinActualPos =actualPos+transcurrido ;
          grados=(dinActualPos/millisVuelta)*360;
          }
          
  if(izquierdas == true){
        float ahora=millis();
        float transcurrido=ahora-tiempoIzquierdas;
        dinActualPos =actualPos-transcurrido ;
        grados=(dinActualPos/millisVuelta)*365;
        }
  }
 
  
  
  
  void gestionTeclado(){
  
  customKey = customKeypad.getKey();
  
  if (customKey){// si se pulsa alguna tecla...
          // para poder comparar con una tecla, hemos de hacerlo con su valor ascii, porque getKey() devuelve el valor ascii, no el carácter
          if (customKey==65)
          tecladoA();
          if (customKey==66)
          tecladoB();         
          if (customKey==67)
          tecladoC();          
          if (customKey==68) 
          tecladoD();
          if (customKey==46||customKey==48||customKey==49||customKey==50||customKey==51||customKey==52||customKey==53||customKey==54||customKey==55||customKey==56||customKey==57) 
          tecladoNumeros();
 
          lcd.setCursor(9,1);
          lcd.print(comando,0);
          lcd.print("   ");
          }  
  }
  
  
  
  void tecladoA(){// Abort
    flagAuto=false;
flagAutoMenos=false;
flagAutoMas=false;
  } 
  
   void tecladoB(){// sin función
  } 
  
    void tecladoC(){// ejecutar comando
flagAuto=true;
automatico();
salida="";

  } 
void tecladoD(){
 
salida="";// en este caso la Tecla "D" (ascii 68) borra el comando introducido
comando=0;
lcd.setCursor(9,1);
lcd.print("       ");
}


void tecladoNumeros(){
    
salida=salida + customKey; //vamos formando la cadena de caracteres.
comando=salida.toFloat(); // la convertimos a numero en coma flotante

if (comando<0.00||comando>360||salida.length()>6){ //si tenemos mas de seis caracteres.

        lcd.setCursor(9,1); //provocamos un error
        lcd.print("error  ");
        delay(3000);
        lcd.setCursor(9,1);
        lcd.print("       ");
       comando=0.00;
        salida="";
        }
  }
  
void automatico(){
  if (flagAutoMenos==false){
  
           if (grados<comando){ //gira a +...
              flagAutoMas=true;  
           }
           if (flagAutoMas && (grados>=comando)){
              flagAutoMas=false;
              flagAuto=false;
              return;//Termina la funcion
           }
                   
  }
  
  if (flagAutoMas==false){
          if (grados>comando){ //gira a -...
              flagAutoMenos=true;  
           }
           if (flagAutoMenos && grados<=comando){
              flagAutoMenos=false;
              flagAuto=false;
              return;//Termina la funcion
            }      
  }
  
}
