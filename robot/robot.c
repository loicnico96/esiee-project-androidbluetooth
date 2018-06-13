
#include "p18f2480.h"

// Configuration generale
#pragma config OSC=IRCIO67
#pragma config WDT=OFF
#pragma config LVP=OFF
#pragma config MCLRE=OFF

void ProgIT ();

//----------------------------------------------------------------------------
// High priority interrupt vector
//----------------------------------------------------------------------------
#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh () {
  _asm
    goto ProgIT // jump to interrupt routine
  _endasm
}

//----------------------------------------------------------------------------
// Commande moteurs: Arreter
//----------------------------------------------------------------------------
void motors_stop () {
    PORTA = 0x00;
}

//----------------------------------------------------------------------------
// Commande moteurs: Avancer
//----------------------------------------------------------------------------
void motors_forwards () {
    PORTA = 0x0A;   // Bit 3 + Bit 1
}

//----------------------------------------------------------------------------
// Commande moteurs: Reculer
//----------------------------------------------------------------------------
void motors_backwards () {
    PORTA = 0x05;   // Bit 2 + Bit 0
}

//----------------------------------------------------------------------------
// Commande moteurs: Tourner a gauche
//----------------------------------------------------------------------------
void motors_turn_left () {
    PORTA = 0x06;   // Bit 2 + Bit 1
}

//----------------------------------------------------------------------------
// Commande moteurs: Tourner a droite
//----------------------------------------------------------------------------
void motors_turn_right () {
    PORTA = 0x09;   // Bit 3 + Bit 0
}

//----------------------------------------------------------------------------
// Initialisation et demarrage du timer TMR0: 
// - Le timer compte sur 16 bits et genere une interruption quand il atteint
//   sa valeur maximale (2^5-1 = 65536). 
// - Le timer compte avec une frequence de 500 kHz (8 MHz d'horloge, divises
//   par un prescaler de 16). La duree totale du timer est donc de 131,072
//   millisecondes. 
// - L'application Android envoie une nouvelle instruction toutes les 100 
//   millisecondes et le timer est redemarre a chaque instruction recue. De 
//   cette maniere, le robot a un mouvement continu lorsqu'on maintient un 
//   bouton et s'arrete peu de temps apres que le bouton soit relache. 
// - La marge supplementaire permet de prendre en compte d'eventuels delais 
//   de transmission de l'application. 
//----------------------------------------------------------------------------
void timer_restart () {
    TMR0H   = 0;
    TMR0L   = 0;			// Initialisation du timer a 0
	//------------------------------------------------------------------------
	// Registre T0CON: 
	// TMR0ON	T08BIT	T0CS	T0SE	PSA		T0PS2	T0PS1	T0PS0
	// 1		0		0		-		0		0		1		1
	// 
	// TMR0N			Activation du timer
	// T08BIT			Configuration a 16 bits
	// T0CS				Utilisation de l'horloge interne
	// PSA				Activation du prescaler
	// T0PS2:TP0PS0		Configuration du prescaler a 16
	//------------------------------------------------------------------------
    T0CON   = 0x83;         // Activation du timer
	//------------------------------------------------------------------------
	// Registre INTCON: 
	// GIE		PEIE	TMR0IE	RBIE	INT0IE	TMR0IF	INT0IF	RBIF
	// 1		-		1		-		-		0		-		-
	// 
	// GIE				Activation des interruptions
	// TMR0IE			Activation de l'interruption du timer 0
	// TMR0IF			Flag indiquant l'interruption du timer 0
	//------------------------------------------------------------------------
    INTCON  = 0xA0;         // Activation des interruptions
}

//----------------------------------------------------------------------------
// Interruption du timer: 
// - Permet d'arreter les moteurs si aucune nouvelle instruction n'est recue 
//   apres un certain delai.
//----------------------------------------------------------------------------
#pragma code
#pragma interrupt ProgIT
void ProgIT () {
    INTCON  = 0x00;         // Desactivation des interruptions
    T0CON   = 0x00;         // Desactivation du timer
    motors_stop ();         // Arret des moteurs
}

//----------------------------------------------------------------------------
// Programme principal
//----------------------------------------------------------------------------
void main () {
	// Etat actuel de communication Bluetooth
    char rx_mode = 0;		// 1 = communication en cours

    // Initialisation des ports
    OSCCON	= 0x72; 		// Configuration de l'horloge a 8 MHz
    TRISA   = 0x00;			// Configuration du port A en sortie
    TRISC   = 0xFF; 		// Configuration du port B en entree
    ADCON1  = 0x0F;
    
    // Initialisation de la communication Bluetooth
    SPBRG   = 12; 			// Baud rate de 9600 / Frequence de 8 MHz
    RCSTA   = 0x90;			// Activation de la reception Bluetooth
    TXSTA   = 0x20; 
    
    // Arret des moteurs
    motors_stop ();
    
    do {
        if (PIR1bits.RCIF) {            // Lecture de PIR1.RCIF (vaut 1 en cas de reception)
            char code = RCREG;          // Lecture de RCREG (contient l'octet recu)
            if (rx_mode == 1) {
                switch (code) {
                    case 0x30:			// '0'	-> Avancer
                        motors_forwards ();
                        timer_restart ();
                        break;
                    case 0x31:			// '1'	-> Reculer
                        motors_backwards ();
                        timer_restart ();
                        break;
                    case 0x32:			// '2'	-> Tourner a gauche
                        motors_turn_left ();
                        timer_restart ();
                        break;
                    case 0x33:			// '3'	-> Tourner a droite
                        motors_turn_right ();
                        timer_restart ();
                        break;
                    case 0x0D:			// '\r'	-> Fin de la communication
                        rx_mode = 0; 
                }
            } else {
                switch (code) {
                    case 0x3A:			// ':'	-> Debut de la communication
                        rx_mode = 1; 
                }
            }
        }
	// La boucle est executee indefiniment
    }  while (1);
}
