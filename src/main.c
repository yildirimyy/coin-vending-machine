#include "stdint.h"
#include "driverlib/gpio.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"

static inline void disable_interrupts() { asm("CPSID I"); }
static inline void enable_interrupts() { asm("CPSIE I"); }
static inline void wait_for_interrupt() { asm("WFI"); }

volatile unsigned long delay; // compiler optimizasyonunu engellemek icin volatile degisken

volatile unsigned long tmp;

int button0;
int button1;
int button2;
int button3;
int button4;

int buttonHesapla;
int buttonResetle;

//---------------------------------------------------------------------------

/**
 *
 * port hazirlik fonksiyonlari baslangici
 *
 * */

void port_E_hazirla() {
	 // bu degisken gecikme yapmak icin gerekli

	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE; // E portunun osilatörünü etkinleştir
	tmp = SYSCTL_RCGCGPIO_R;    	// allow time for clock to start

	GPIO_PORTE_LOCK_R = 0x4C4F434B;   // unlock GPIO Port E
	GPIO_PORTE_CR_R = 0x3F; 		  // PE kilit ac sadece PE0 kilidinin açılması gerekir, diğer bitler kilitlenemez

	GPIO_PORTE_AMSEL_R = 0x00; // PE'de analog devre disi birak

	GPIO_PORTE_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PE4-0

	GPIO_PORTE_DIR_R = 0x00;      	// PE0,PE1,PE2,PE3,PE4,PE5 in
	GPIO_PORTE_AFSEL_R = 0x00;    	// afsel kapat on PE7-0
	GPIO_PORTE_PUR_R = 0x3F; 		// pull-up etkinlestir 0011 1111 0 1 2 3 4 5

	GPIO_PORTE_DEN_R = 0x3F; // enable digital I/O P // portE 5-0 giriş çıkış  etkinlerştir.
}

void port_A_hazirla(){

	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // A portunun osilatörünü etkinleştir
	tmp = SYSCTL_RCGCGPIO_R;
	GPIO_PORTA_CR_R |= 0x7F;

	GPIO_PORTA_AMSEL_R = 0x00;

	GPIO_PORTA_PCTL_R = 0x00000000;

	GPIO_PORTA_DIR_R |= 0x1FC; //PA2 PA3 PA4 PA5 PA6 PA7 out
	GPIO_PORTA_AFSEL_R |= 0x00;

	GPIO_PORTA_DEN_R |= 0x1FC;
}

void port_F_hazirla(){

	// PORTF'yi aktiflestir
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;
	tmp = SYSCTL_RCGC2_R; // zaman gecirmek icin yukarida degistirdigimiz registeri okuyoruz

//	GPIO_PORTF_LOCK_R = 0x4C4F434B;
//	GPIO_PORTF_CR_R = 0x1F;

	GPIO_PORTF_AMSEL_R = 0x00; // PF'de analog devre disi birak
	GPIO_PORTF_PCTL_R = 0x00000000;
	GPIO_PORTF_AFSEL_R = 0x00;

	// PORTF'nin 3. pinini cikis olarak ayarliyoruz
	GPIO_PORTF_DIR_R = 0x0E;

	GPIO_PORTF_PUR_R = 0x11; 		// pull-up etkinlestir PF4

	GPIO_PORTF_DEN_R = 0x1F;
}

void port_B_hazirla(){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;
	GPIO_PORTB_AFSEL_R &= ~0b00001111;
	GPIO_PORTB_DIR_R = 0b11111111;
	GPIO_PORTB_DEN_R = 0b11111111;

	duzenle(0b00101000);
	duzenle(0b00001100);
	duzenle(0b00000110);
}

void PF4_Kesme_Hazirla() { // http://users.ece.utexas.edu/~valvano/Volume1/E-Book/C12_Interrupts.htm#12_3 Tablo 12.1
	GPIO_PORTF_IS_R &= ~0b10000; // (d) PC4 is edge-sensitive
	GPIO_PORTF_IBE_R &= ~0b10000; //PC4 is not both edges
	GPIO_PORTF_IEV_R &= ~0b10000; // PC4 falling edge event
	GPIO_PORTF_ICR_R = 0b10000; // (e) clear flag4
	GPIO_PORTF_IM_R |= 0b10000; // (f) arm interrupt on PC4

	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000; // (g) priority 5
	NVIC_EN0_R |= (1 << 30); // (h) enable interrupt 30 in NVIC
}

/**
 *
 * port hazirlik fonksiyonlari sonu
 *
 * */

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

/*
 * ekrana erisim fonksiyonlarin baslangici
 *
 * */

void karakter_yaz(unsigned char c) {

	GPIO_PORTB_DATA_R = c & 0xf0;
    rs_degistir(1);
    e_degistir();

    SysCtlDelay(1000);

    GPIO_PORTB_DATA_R = (c & 0x0f) << 4;
    rs_degistir(1);
    e_degistir();

    SysCtlDelay(1000);

}

void rs_degistir(int flag){

	// 0 kapalı 1 acik
	if(flag == 0){
		GPIO_PORTB_DATA_R &= ~0b00000001;
	}else{
		GPIO_PORTB_DATA_R |= 0b00000001;
	}
}

void e_degistir(){
	GPIO_PORTB_DATA_R |= 0b00000010;
	GPIO_PORTB_DATA_R &= ~0b00000010;
}

void ekran_temizle(void){
	duzenle(0x01);
	SysCtlDelay(10);
}

void duzenle(unsigned char c) {

	GPIO_PORTB_DATA_R = c & 0xf0;
	rs_degistir(0);
	e_degistir();
	SysCtlDelay(50000);
	GPIO_PORTB_DATA_R = (c & 0x0f) << 4;
	rs_degistir(0);
	e_degistir();

	SysCtlDelay(50000);
}

void imlec_ayarla(char x, char y){

	if (x == 1)
		duzenle(0x80 + ((y - 1) % 16));
	else
		duzenle(0xC0 + ((y - 1) % 16));
}

//ekrani temizleyip hem ust satir hem alt satir yazmak icin
void ekrana_dizi_yaz(char ustSatir[], char altSatir[]){

	ekran_temizle();
	imlec_ayarla(1, 1);
	int i = 0;
	for (i = 0; i < 16; i++) {
		karakter_yaz(ustSatir[i]);
	}

	imlec_ayarla(2, 1);
	for (i = 0; i < 16; i++) {
		karakter_yaz(altSatir[i]);
	}

}

//ekrana ust satir yazmak icin
void ekrana_ust_satir_yaz(char ustSatir[]){

	imlec_ayarla(1, 1);
	int i = 0;
	for (i = 0; i < 16; i++) {
		karakter_yaz(ustSatir[i]);
	}
}

/*
 * ekrana erisim fonksiyonlarinin bitisi
 *
 * */

//---------------------------------------------------------------------------

//resetle buttonunda cagirilen kesme fonksiyonu
void PortF_Button_Kesme_Fonksiyonu() {
	disable_interrupts();
	GPIO_PORTF_ICR_R = 0b10000; // clear interrupt flag4

	GPIO_PORTF_DATA_R |= 0b0100;

	SysCtlDelay(1000000);

	GPIO_PORTF_DATA_R &= ~(0b0100);

	//ResetISR();
	SysCtlReset();

	enable_interrupts();
}

void led_yak(int led){

	if(led == 1){
		GPIO_PORTA_DATA_R |= 0b0100; // ilk ledi yak
		GPIO_PORTA_DATA_R &= ~(0b01000); //ikinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000); //ucuncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b0100000); //dorduncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000000); //besinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000000); //altinci ledi sondur

	}else if(led == 2){

		GPIO_PORTA_DATA_R &= ~(0b0100); // ilk ledi sondur
		GPIO_PORTA_DATA_R |= 0b01000; // ikinci ledi yak
		GPIO_PORTA_DATA_R &= ~(0b010000); //ucuncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b0100000); //dorduncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000000); //besinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000000); //altinci ledi sondur

	}else if(led == 3){

		GPIO_PORTA_DATA_R &= ~(0b0100); // ilk ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000); //ikinci ledi sondur

		GPIO_PORTA_DATA_R |= 0b10000; // ucuncu ledi yak

		GPIO_PORTA_DATA_R &= ~(0b0100000); //dorduncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000000); //besinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000000); //altinci ledi sondur

	}else if(led == 4){

		GPIO_PORTA_DATA_R &= ~(0b0100); // ilk ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000); //ikinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000); //ucuncu ledi sondur
		GPIO_PORTA_DATA_R |= 0b0100000; // dorduncu ledi yak
		GPIO_PORTA_DATA_R &= ~(0b01000000); //besinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000000); //altinci ledi sondur

	}else if(led == 5){

		GPIO_PORTA_DATA_R &= ~(0b0100); // ilk ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000); //ikinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000); //ucuncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b0100000); //dorduncu ledi sondur
		GPIO_PORTA_DATA_R |= 0b01000000; // besinci ledi yak
		GPIO_PORTA_DATA_R &= ~(0b010000000); //altinci ledi sondur

	}else if(led == 6){

		GPIO_PORTA_DATA_R &= ~(0b0100); // ilk ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000); //ikinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000); //ucuncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b0100000); //dorduncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000000); //besinci ledi sondur

		GPIO_PORTA_DATA_R |= 0b010000000; // altinci ledi yak

	}else if(led == 7){

		GPIO_PORTA_DATA_R &= ~(0b0100); // ilk ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000); //ikinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000); //ucuncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b0100000); //dorduncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000000); //besinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000000); //altinci ledi sondur

	}else {

		GPIO_PORTA_DATA_R &= ~(0b0100); // ilk ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000); //ikinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000); //ucuncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b0100000); //dorduncu ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b01000000); //besinci ledi sondur
		GPIO_PORTA_DATA_R &= ~(0b010000000); //altinci ledi sondur
	}
}


// 1 yirmilik gibi tam sayi girilirse sonuc ekrani icin ust satir temizligi
void ekran_kontrol(char ustSatir[]){

	int flag = 0;
	int i = 0;

	//ust satirda sayi kalip kalmadigina bakiliyor
	for (i = 0; i< 16; i++) {
		if(ustSatir[i] != '-' && ustSatir[i] != '0' && ustSatir[i] != '.'){
			flag = 1;
			break;
		}
	}

	//ust satirda sayi kalmamissa tam bir sayi girilmistir 20.00 gibi o yuzden 0.00 temizleniyor
	if(flag == 0){
		int i = 0;
		for(i = 0; i < 16; i++){
			ustSatir[i] = '-';
		}
		ekrana_ust_satir_yaz(ustSatir);
	}
}

//virgulden once hesabi yazdirmak icin
void hesap_yazdir1(int v_once_arananlar[], char ustSatir[], char altSatir[]){

		int i = 0;
		int gosterilenSayi = 0;
	    int tmpSayi = 0;
	    int tmpSayi2 = 0;
	    for(i=0; i < 4; i++){

			tmpSayi = v_once_arananlar[i];
			if(tmpSayi != 0){

	    		altSatir[0] = (char)(tmpSayi + '0');
	    		altSatir[1] = '-';
		    	if(i == 0){

		    		gosterilenSayi = (int)(ustSatir[11] - '0'); // ornek 9

		    		tmpSayi2 = tmpSayi * 2; //yirmilik oldugu icin carpi 2, ornek 4 yirmilik olsun 8 yapar

		    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 9 - 8 den 1 yapar yeni gosterilen sayi 1 olmali

		    		gosterilenSayi = tmpSayi2;

		    		if(gosterilenSayi == 0)
		    			ustSatir[11] = '-';
		    		else
		    			ustSatir[11] = (char)(gosterilenSayi + '0');


		    		altSatir[2] = 'y'; altSatir[3] = 'i'; altSatir[4] = 'r'; altSatir[5] = 'm';
		    		altSatir[6] = 'i'; altSatir[7] = 'l'; altSatir[8] = 'i'; altSatir[9] = 'k';
		    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
		    		altSatir[14] = '-'; altSatir[15] = '-';

		    	}else if(i == 1){

		    		gosterilenSayi = (int)(ustSatir[11] - '0'); // sadece 1 olabilir 2 onluk 1 yirmilik ustte elendi

		    		tmpSayi2 = tmpSayi; //onluk oldugu icin carpi 1, ornek 1 onluk olsun 1 yapar 2 onluk olamaz

		    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 1 - 1 den 0 yapar yeni gosterilen sayi 0 olmali

		    		gosterilenSayi = tmpSayi2;

		    		if(gosterilenSayi == 0)
		    			ustSatir[11] = '-';
		    		else
		    			ustSatir[11] = (char)(gosterilenSayi + '0');


		    		altSatir[2] = 'o'; altSatir[3] = 'n'; altSatir[4] = 'l'; altSatir[5] = 'u';
		    		altSatir[6] = 'k'; altSatir[7] = '-'; altSatir[8] = '-'; altSatir[9] = '-';
		    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
		    		altSatir[14] = '-'; altSatir[15] = '-';

		    	}else if(i == 2){

		    		gosterilenSayi = (int)(ustSatir[12] - '0'); // ornek 99dan 9 olabilir

		    		tmpSayi2 = tmpSayi * 5; //beslik oldugu icin carpi 5 tmpsayi maks 1 olabilir 2 olursa onluk olurdu onluk yukarida elendi

		    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 9 - 5 den 4 yapar yeni gosterilen sayi 4 olmali

		    		gosterilenSayi = tmpSayi2;

		    		ustSatir[11] = '-';
		    		if(gosterilenSayi == 0)
		    			ustSatir[12] = '-';
		    		else
		    			ustSatir[12] = (char)(gosterilenSayi + '0');


		    		altSatir[2] = 'b'; altSatir[3] = 'e'; altSatir[4] = 's'; altSatir[5] = 'l';
		    		altSatir[6] = 'i'; altSatir[7] = 'k'; altSatir[8] = '-'; altSatir[9] = '-';
		    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
		    		altSatir[14] = '-'; altSatir[15] = '-';

		    	}else if(i == 3){

		    		gosterilenSayi = (int)(ustSatir[12] - '0'); // ornek 4 maks olabilir

		    		tmpSayi2 = tmpSayi; //birlik oldugu icin carpi yok

		    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 4 - 4 den 0 yapar yeni gosterilen sayi 0 olmali

		    		gosterilenSayi = tmpSayi2;

		    		ustSatir[11] = '-';
		    		if(gosterilenSayi == 0)
		    			ustSatir[12] = '-';
		    		else
		    			ustSatir[12] = (char)(gosterilenSayi + '0');


		    		altSatir[2] = 'b'; altSatir[3] = 'i'; altSatir[4] = 'r'; altSatir[5] = 'l';
		    		altSatir[6] = 'i'; altSatir[7] = 'k'; altSatir[8] = '-'; altSatir[9] = '-';
		    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
		    		altSatir[14] = '-'; altSatir[15] = '-';

		    	}

		    	ekran_kontrol(ustSatir);

		    	ekrana_dizi_yaz(ustSatir, altSatir);

		    	SysCtlDelay(20000000);
			}

	    }

}

//virgulden sonra hesabi yazdirmak icin
void hesap_yazdir2(int v_sonra_arananlar[], char ustSatir[], char altSatir[]){

			ustSatir[11] = '-';
			ustSatir[12] = '-';
			int i = 0;
			int gosterilenSayi = 0;
		    int tmpSayi = 0;
		    int tmpSayi2 = 0;
		    for(i=0; i < 5; i++){

				tmpSayi = v_sonra_arananlar[i];
				if(tmpSayi != 0){

		    		altSatir[0] = (char)(tmpSayi + '0');
		    		altSatir[1] = '-';
			    	if(i == 0){

			    		gosterilenSayi = (int)(ustSatir[14] - '0'); // ornek 9

			    		tmpSayi2 = tmpSayi * 5; //1 yarimlik olsun carpi 5

			    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 9 - 5 den 4 yapar yeni gosterilen sayi 4 olmali

			    		gosterilenSayi = tmpSayi2;

			    		if(gosterilenSayi == 0)
			    			ustSatir[14] = '-';
			    		else
			    			ustSatir[14] = (char)(gosterilenSayi + '0');


			    		altSatir[2] = 'y'; altSatir[3] = 'a'; altSatir[4] = 'r'; altSatir[5] = 'i';
			    		altSatir[6] = 'm'; altSatir[7] = 'l'; altSatir[8] = 'i'; altSatir[9] = 'k';
			    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
			    		altSatir[14] = '-'; altSatir[15] = '-';

			    	}else if(i == 1){

			    		gosterilenSayi = (int)(ustSatir[14] - '0'); // ceyreklik 25 oldugu icin son hane de onemli
			    		int gosterilen2 = (int)(ustSatir[15] - '0'); // ornegin 49 gosteriliyor gosterilen 4, gosterilen2 9 olur

			    		tmpSayi2 = gosterilenSayi * 10 + gosterilen2; // 4 * 10 + 9 = 49 olur
			    		tmpSayi2 = tmpSayi2 - 25; // 49 - 25 = 24 olur

			    		gosterilenSayi = tmpSayi2 / 10; // 24:10 dan 2 olur

			    		tmpSayi2 = tmpSayi2 - (10 * gosterilenSayi); // 24 - (10 * 2) = 4
			    		gosterilen2 = tmpSayi2; // son haneye kalan sayi yazilir

			    		if(gosterilenSayi == 0)
			    			ustSatir[14] = '-';
			    		else
			    			ustSatir[14] = (char)(gosterilenSayi + '0');

			    		if(gosterilen2 == 0)
			    			ustSatir[15] = '-';
			    		else
			    			ustSatir[15] = (char)(gosterilen2 + '0');

			    		altSatir[2] = 'c'; altSatir[3] = 'e'; altSatir[4] = 'y'; altSatir[5] = 'r';
			    		altSatir[6] = 'e'; altSatir[7] = 'k'; altSatir[8] = 'l'; altSatir[9] = 'i';
			    		altSatir[10] = 'k'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
			    		altSatir[14] = '-'; altSatir[15] = '-';

			    	}else if(i == 2){

			    		gosterilenSayi = (int)(ustSatir[14] - '0'); // ornek 2 zaten 3 olamaz ceyreklik olurdu

			    		tmpSayi2 = tmpSayi ; // 2 tane metelik

			    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 2 - 2 den 0 yapar yeni gosterilen sayi 0 olmali

			    		gosterilenSayi = tmpSayi2;

			    		ustSatir[14] = (char)(gosterilenSayi + '0');

			    		altSatir[2] = 'm'; altSatir[3] = 'e'; altSatir[4] = 't'; altSatir[5] = 'e';
			    		altSatir[6] = 'l'; altSatir[7] = 'i'; altSatir[8] = 'k'; altSatir[9] = '-';
			    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
			    		altSatir[14] = '-'; altSatir[15] = '-';

			    	}else if(i == 3){

			    		gosterilenSayi = (int)(ustSatir[15] - '0'); // ornek 5

			    		tmpSayi2 = tmpSayi * 5; //1 delik oldugu icin carpi 5

			    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 5 - 5 den 0 yapar yeni gosterilen sayi 0 olmali

			    		gosterilenSayi = tmpSayi2;

			    		ustSatir[14] = '-';
			    		if(gosterilenSayi == 0)
			    			ustSatir[15] = '-';
			    		else
			    			ustSatir[15] = (char)(gosterilenSayi + '0');


			    		altSatir[2] = 'd'; altSatir[3] = 'e'; altSatir[4] = 'l'; altSatir[5] = 'i';
			    		altSatir[6] = 'k'; altSatir[7] = '-'; altSatir[8] = '-'; altSatir[9] = '-';
			    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
			    		altSatir[14] = '-'; altSatir[15] = '-';

			    	}else if(i == 4){

			    		gosterilenSayi = (int)(ustSatir[15] - '0'); // ornek 4 zaten 5 olamaz deliklik olurdu

			    		tmpSayi2 = tmpSayi; //3 kurus 4 kurus gibi carpi yok

			    		tmpSayi2 = gosterilenSayi - tmpSayi2; // 4 - 4 den 0 yapar yeni gosterilen sayi 0 olmali

			    		gosterilenSayi = tmpSayi2;

			    		ustSatir[14] = '-';
			    		if(gosterilenSayi == 0)
			    			ustSatir[15] = '-';
			    		else
			    			ustSatir[15] = (char)(gosterilenSayi + '0');

			    		altSatir[2] = 'k'; altSatir[3] = 'u'; altSatir[4] = 'r'; altSatir[5] = 'u';
			    		altSatir[6] = 's'; altSatir[7] = 'l'; altSatir[8] = 'u'; altSatir[9] = 'k';
			    		altSatir[10] = '-'; altSatir[11] = '-'; altSatir[12] = '-'; altSatir[13] = '-';
			    		altSatir[14] = '-'; altSatir[15] = '-';

			    	}

			    	ekran_kontrol(ustSatir);

			    	ekrana_dizi_yaz(ustSatir, altSatir);

			    	SysCtlDelay(20000000);
				}

		    }
}


void hesapla(char ustSatir[], char altSatir[]){

	int sayiOnlar = (int)(ustSatir[11] - '0') * 10;
	int sayiBirler = (int)(ustSatir[12] - '0');

	int sayiVsonraOnlar = (int)(ustSatir[14] - '0') * 10;
	int sayiVsonraBirler = (int)(ustSatir[15] - '0');

	int vOnce = sayiOnlar + sayiBirler;
	int vSonra = sayiVsonraOnlar + sayiVsonraBirler;

    unsigned int v_once_sayi = vOnce;
    unsigned int v_sonra_sayi = vSonra;

    unsigned int v_once_arananlar[] = {20, 10, 5, 1};
    //"yirmilik" "onluk" "beslik" "birlik"

    unsigned int v_sonra_arananlar[] = {50, 25, 10, 5, 1};
    //"yarimlik" "ceyreklik" "metelik" "delik" "kurusluk"

    unsigned int i = 0, tmp_sayi = 0, adet = 0;

    for(i = 0; i < 4; i++){

        tmp_sayi = v_once_arananlar[i];
        adet = (v_once_sayi / tmp_sayi);
        v_once_arananlar[i] = adet;

        v_once_sayi = v_once_sayi - (tmp_sayi * adet);
    }

    tmp_sayi = 0;
    adet = 0;
    for(i = 0; i < 5; i++){

        tmp_sayi = v_sonra_arananlar[i];
        adet = (v_sonra_sayi / tmp_sayi);
        v_sonra_arananlar[i] = adet;

        v_sonra_sayi = v_sonra_sayi - (tmp_sayi * adet);
    }

    hesap_yazdir1(v_once_arananlar, ustSatir, altSatir);
    hesap_yazdir2(v_sonra_arananlar, ustSatir, altSatir);

}

//buttona bastikca sayiyi arttirmak 9dan buyuk olursa 0 yapmak bekleme süresi buttonun düsen kenari icin
void islem_yap(char ustSatir[], int index){
	int sayi = (int)(ustSatir[index] - '0');
	sayi++;

	if(sayi > 9)
		sayi = 0;

	char sayiC = (char)(sayi + '0');

	ustSatir[index] = sayiC;

	imlec_ayarla(1, index+1);
	karakter_yaz(sayiC);

	SysCtlDelay(1000000);

}

//hangi buttona basildigini okuyan fonksiyon
void buttonlari_oku(){
	button0 = GPIO_PORTE_DATA_R & 0b00001;
	button1 = GPIO_PORTE_DATA_R & 0b00010;
	button2 = GPIO_PORTE_DATA_R & 0b00100;
	button3 = GPIO_PORTE_DATA_R & 0b01000;
	button4 = GPIO_PORTE_DATA_R & 0b10000;

	buttonHesapla = GPIO_PORTE_DATA_R & 0b100000;
	buttonResetle = GPIO_PORTF_DATA_R & 0b10000;
}


void setup(){

	port_E_hazirla();
	port_B_hazirla();
	port_F_hazirla();
	port_A_hazirla();
	PF4_Kesme_Hazirla();
}

int main(void) {

	//portlari ayarla
	setup();

	char ustSatir[]= "yagmur yildirim ";
	char altSatir[]= "merve akdag     ";

	ekrana_dizi_yaz(ustSatir, altSatir);

	int i = 0;
	for (i = 0; i < 16; i++) {
		ustSatir[i] = '-';
		altSatir[i] = '-';
	}

	SysCtlDelay(25000000);

	ekrana_dizi_yaz(ustSatir, altSatir);

	ustSatir[11] = '0';
	ustSatir[12] = '0';
	ustSatir[14] = '0';
	ustSatir[15] = '0';

	enable_interrupts();
	while (1) {

		buttonlari_oku();

		if(button0 == 0){
			led_yak(1);

			islem_yap(ustSatir, 11);

			led_yak(0); //sondur

		}else if(button1 == 0){
			led_yak(2);

			islem_yap(ustSatir, 12);

			led_yak(0);  //sondur

		}else if(button2 == 0){

			led_yak(3);

			ustSatir[13] = '.';
			imlec_ayarla(1, 14);
			karakter_yaz('.');

			SysCtlDelay(1000000);

			led_yak(0);  //sondur

		}else if(button3 == 0){

			led_yak(4);

			islem_yap(ustSatir, 14);

			led_yak(0);  //sondur

		}else if(button4 == 0){

			led_yak(5);

			islem_yap(ustSatir, 15);

			led_yak(0); //sondur

		}else if(buttonHesapla == 0){

			led_yak(6);

			SysCtlDelay(1000000);

			led_yak(0); //sondur

			hesapla(ustSatir, altSatir);

		}else if(buttonResetle == 0){

			//button kesmesi olusturulup resetle kullanilmistir

//			int i = 0;
//			for(i = 0; i < 16; i++){
//				ustSatir[i] = '-';
//				altSatir[i] = '-';
//			}
//			ekrana_dizi_yaz(ustSatir, altSatir);
//
//			ustSatir[11] = '0';
//			ustSatir[12] = '0';
//			ustSatir[14] = '0';
//			ustSatir[15] = '0';

		}else{


		}


	}
}


//double paralar[] = {20.0, 10.0, 5.0, 1.0, 0.5, 0.25, 0.1, 0.05, 0.01};

//    for(i = 0; i< 9; i++){
//
//        double tmpSayi = paralar[i];
//        int sonuc = (sayi2 / tmpSayi);
//        paralar[i] = (double)sonuc;
//
//        printf("sayi %f: %d tane %f\n", sayi2, sonuc, tmpSayi);
//
//        sayi2 = sayi2 - (tmpSayi * (double)sonuc);
//    }
//
//
//    for(i = 0; i< 9; i++){
//        printf("%f\n", paralar[i]);
//    }

