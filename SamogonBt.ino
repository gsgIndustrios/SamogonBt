/*
 Name:		SamogonBt.ino
 Created:	07.02.2020 21:39:13
 Author:	KENNY
*/
bool BtMsg = false;
unsigned long Timer;
//#########################################Температура#################################################//
//****************************************Расчет температуры*******************************************//

#define NTC10K 10000.0
#define NTC100K 100000.0
#define Rdel10K 10000.0
#define Rdel100K 100000.0

//Сопротивления по даташиту датчиков NTC10K и NTC100K
double Resistance10[37] = { 1214600,844390,592430,419380,299480,215670,156410,114660,84510,62927,47077,35563,27119,20860,16204,12683,10000,7942,6327,5074,4103,3336,2724,2237,1846,1530,1275,1068,899.3,760.7,645.2,549.4,470,403.6,347.4,300.1,260.1 };
double Resistance100[37] = { 12022000,8548000,6100400,4371200,3145900,2274600,1649000,1207100,884550,654460,488520,367810,279440,213910,165070,128230,100000,78393,61822,49053,39116,31371,25338,20565,16762,13726,11279,9305,7718,6426,5368,4500,3792,3206,2716,2308,1968 };
double Temper[37] = { 1,6,11,16,21,26,31,36,41,46,51,56,61,66,71,76,81,86,91,96,101,106,111,116,121,126,131,136,141,146,151,156,161,166,171,176,181 };


double Thermister(int Pin, int NTC, double Rterm) {
	double TERMAL;
	double ResI;
	double ResH;
	double ResL;
	double TH;
	double TL;
	//Расчет сопротивления
	ResI = analogRead(Pin);
	ResI = (1023 / ResI) - 1;
	ResI = (Rterm / ResI);
	//Выбор датчика и ближних значений сопротивлений
	int i = 0;
	if (NTC == 10) {
		while (ResI < Resistance10[i]) {
			i++;
		}
		ResH = Resistance10[i - 1];
		ResL = Resistance10[i];
	}
	else {
		while (ResI < Resistance100[i]) {
			i++;
		}
		ResH = Resistance100[i - 1];
		ResL = Resistance100[i];
	}
	TH = Temper[i - 1];
	TL = Temper[i];
	
	//Расчет среднего значения сопротивления
	TERMAL = (ResI - ResL) / ((ResH - ResL) / 5);
	
	//Расчет среднего значения температуры
	TERMAL = ((((TH - TL) / 5) * TERMAL) + TL) - 56;
	return TERMAL;
}


//#########################################Alarm#################################################//
#define AlarmLed 8
#define BuzzPin 3
bool AlarmSw = false;

void AlarmSetup() {
	pinMode(AlarmLed, OUTPUT);
	pinMode(BuzzPin, OUTPUT);
	digitalWrite(AlarmLed, HIGH);
	digitalWrite(BuzzPin, HIGH);
	delay(300);
	digitalWrite(AlarmLed, LOW);
	digitalWrite(BuzzPin, LOW);
}

//Аварийная мигалка и пищалка
unsigned long ZumTime;
bool BuzzMode=false;

void AlarmBuzzSw(bool Sw, unsigned long Time, unsigned long BuzzPeriod) {
	if (Sw) {
		if (ZumTime < Time) {
			if (BuzzMode) {
				digitalWrite(BuzzPin, LOW);
				digitalWrite(AlarmLed, HIGH);
				BuzzMode = false;
			}
			else {
				digitalWrite(BuzzPin, HIGH);
				digitalWrite(AlarmLed, LOW);
				BuzzMode = true;
			}
			ZumTime = Time + BuzzPeriod;
		}
	}
}





//#########################################ТЭН#################################################//
#include <EEPROM.h>

//#include <CyberLib.h>

#define HeatLed 7
#define HeatPWM 4
#define ZeroSw 2

int Ustavka = int(EEPROM.read(1)); //Уставка температура поддержания
int Kdim = int(EEPROM.read(2));    //Коэффициент

int Mode = 0; //Режим 1 - Нагрев, 2 - Перегонка, 0 - Выкл
bool HeatOn = false; //Включение нагрева
int HeatVal = 0; //Значение в процентах на экран

volatile int tic, Dimmer; //Переменные для PWM

//-------------------------------------------Загрузка------------------------------------------//
void HeatSetup() {
	pinMode(HeatLed, OUTPUT);
	pinMode(HeatPWM, OUTPUT);
	//pinMode(ZeroSw, INPUT);
	digitalWrite(HeatPWM, LOW);
	digitalWrite(HeatLed, LOW);
	attachInterrupt(0, light, FALLING);
	/*
	attachInterrupt(0, detect_up, FALLING);  // настроить срабатывание прерывания interrupt0 на pin 2 на низкий уровень
	StartTimer1(timer_interrupt, 40);        // время для одного разряда ШИМ
	StopTimer1();                            // остановить таймер
	*/
}

//------------------------------------------Работа------------------------------------------//


void Heat(bool Sw, int Mod, int Ust,  int TempVal, int Kd) {
	if (Sw) {
		switch (Mod){
		case 1://Нагрев
			Dimmer = 255;
			break;
		case 2://Перегонка
			Dimmer = 0;
			int Power = (((Ust * 10) - (TempVal * 10)) * Kd) / 10;
			if (Power > 255) { Power = 255; }
			else if (Power < 0) { Power = 0; }
			//HeatVal = map(Power, 0, 255, 0, 100);
			Dimmer = Power;
			HeatVal = Dimmer;
			break;
		case 0:
			Dimmer = 0;
			break;
		default://Выкл
			Dimmer = 0;
			break;
		}
	}
	else {
		digitalWrite(HeatLed, LOW);
		Dimmer = 0;
	}

}

//------------------------------------Обработка прерываний-------------------------------------//

void light() {
	if (Dimmer > 0 && BtMsg == false) {
		delayMicroseconds(33 * (255-Dimmer));
		digitalWrite(HeatPWM, HIGH);
		digitalWrite(HeatLed, HIGH);
		delayMicroseconds(500);
		digitalWrite(HeatPWM, LOW);
		digitalWrite(HeatLed, LOW);
	}
}

/*
void timer_interrupt() {       // прерывания таймера срабатывают каждые 40 мкс
	tic++;                       // счетчик
	if (tic > Dimmer)           // если настало время включать ток
		digitalWrite(HeatPWM, 1);   // врубить ток
		//Drive();
	
}

void  detect_up() {    // обработка внешнего прерывания на пересекание нуля снизу
	tic = 0;                                  // обнулить счетчик
	ResumeTimer1();                           // перезапустить таймер
	attachInterrupt(0, detect_down, RISING);  // перенастроить прерывание
}

void  detect_down() {  // обработка внешнего прерывания на пересекание нуля сверху
	tic = 0;                                  // обнулить счетчик
	StopTimer1();                             // остановить таймер
	digitalWrite(HeatPWM, 0);                  // вырубить ток
	attachInterrupt(0, detect_up, FALLING);   // перенастроить прерывание
}
*/
//#########################################Помпа#################################################//
#define PumpLed 9
#define PumpSw 5
bool PumpOn = false;

void PumpSetup() {
pinMode(PumpLed, OUTPUT);
pinMode(PumpSw, OUTPUT);
digitalWrite(PumpLed, HIGH);
delay(500);
digitalWrite(PumpLed, LOW);
digitalWrite(PumpSw, LOW);
}


void Pump(bool Sw) {
	if (Sw) {
		digitalWrite(PumpLed, HIGH);
		digitalWrite(PumpSw, HIGH);
	}
	else {
		digitalWrite(PumpLed, LOW);
		digitalWrite(PumpSw, LOW);
	}
}


//#########################################Блюпуп#################################################//
//Загрузка

#define BtLed 6



void BluetoothSetup() {
	BtMsg = true;
	pinMode(BtLed, OUTPUT);
	digitalWrite(BtLed, LOW);
	Serial.begin(115200);
	BtMsg = false;
}

//Прием команд
String VarNameIn;
int VarValIn;
bool Incoming = false;

void Bluetooth() {
	if (Serial.available() > 0) {
		BtMsg = true;
		digitalWrite(BtLed, HIGH);
		String InputString;
		while (Serial.available() > 0) {
			char inputchar;
			inputchar = Serial.read();
			if (inputchar == '.') {
				VarNameIn = InputString;
				InputString = "";
			}
			else if (inputchar == ';') {
				VarValIn = InputString.toInt();
				InputString = "";
				Incoming = true;
			}
			else {
				InputString += inputchar;
			}
			delay(20);
		}
	}
	BtMsg = false;
	digitalWrite(BtLed, LOW);
	Incoming = true;
}

//Отправка команд
void Output(String Name, int Val) {
	BtMsg = true;
	String ValS = String(Val);
	Serial.print(Name + '.' + ValS + ';');
	BtMsg = false;
}
//****************************************Считывание датчиков*******************************************//
//пины датчиков
#define Term1_10 14
#define Term2_1050 15
#define Term3_10 16
#define Term4_100 17

//Интервал опроса
unsigned long TimerTemp = 1000;
unsigned long TempTic = 0;

//Расчетные значения
int ColdTemp, DefTemp, ResTemp;

//Загрузка
void TemperatureSetup() {
	pinMode(Term1_10, INPUT);
	pinMode(Term2_1050, INPUT);
	pinMode(Term3_10, INPUT);
	pinMode(Term4_100, INPUT);
}

//Управление
void TemperatureRead() {
	if (Timer > TempTic) {
		double Temp;
		Temp = Thermister(Term2_1050, 10, NTC10K);
		ColdTemp = int(Temp);
		Temp = Thermister(Term4_100, 100, NTC100K);
		DefTemp = int(Temp);
		//Temp = Thermister(ResTempPin, 100, NTC100K);
		//ResTemp = int(Temp);
		TempTic = Timer + TimerTemp;
		Heat(HeatOn, Mode, Ustavka, DefTemp, Kdim);
	}
}

//#########################################Управление#################################################//

int Twa = int(EEPROM.read(3));	   //Уставка температуры охладителя
unsigned long RefreshTime;
bool BtConnect = false;

void Drive() {
	if (Incoming) {
		if (VarNameIn == "a") {
			Ustavka = VarValIn;
			EEPROM.update(1, byte(VarValIn));
		}
		else if (VarNameIn == "b") {
			Kdim = VarValIn;
			EEPROM.update(2, byte(VarValIn));
		}
		else if (VarNameIn == "c") {
			if (VarValIn == 1|| VarValIn == 2) {
				HeatOn = true;
				Mode = VarValIn;
			}
			else {
				Mode = 0;
				HeatOn = false;
				Heat(HeatOn, Mode, Ustavka, DefTemp, Kdim);
				
			}
			
		}
		else if (VarNameIn == "e") {
			PumpOn = VarValIn;
		}
/*		else if (VarNameIn == "Twa") {
			Twa = VarValIn;
			EEPROM.update(2, byte(VarValIn));
		}*/
		else if (VarNameIn == "k") {
			BtConnect = true;
			DispRefresh();
		}
		else if (VarNameIn == "l") {
			BtConnect = false;
		}
		VarNameIn = "";
		VarValIn = 0;
		Incoming = false;
	}
	if (DefTemp>95|| ColdTemp>40) {
		if (Mode > 0) {
			AlarmSw = true;
		}
	}
	else {
		AlarmSw = false;
	}
	if (Mode == 1 || Mode ==2) {
		if (DefTemp > 40) {
			PumpOn = true;
		}
	}
	if (Timer>RefreshTime) {
		if (BtConnect) {
			DispRefresh();
		}
		RefreshTime = Timer + 3000;
	}
}

void DispRefresh() {
	digitalWrite(BtLed, HIGH);
	Output("a", Ustavka);
	delay(20);
	Output("b", Kdim);
	delay(20);
	Output("c", Mode);
	delay(20);
	Output("d", HeatVal);
	delay(20);
	Output("e", PumpOn);
	delay(20);
	Output("f", ColdTemp);
	delay(20);
	Output("g", DefTemp);
	delay(20);
	Output("h", ResTemp);
	delay(20);
	Output("i", Twa);
	delay(20);
	Output("j", AlarmSw);
	delay(20);
	digitalWrite(BtLed, LOW);
}


/*

int Ustavka = int(EEPROM.read(1)); //Уставка температура поддержания
int Kdim = int(EEPROM.read(2));    //Коэффициент

int Mode = 0; //Режим 1 - Нагрев, 2 - Перегонка, 0 - Выкл

int HeatVal = 0; //Значение в процентах на экран

bool PumpOn = false;

bool AlarmSw = false;

int ColdTemp, DefTemp, ResTemp;

int Twa = int(EEPROM.read(3));

	*/




// the setup function runs once when you press reset or power the board
void setup() {
	AlarmSetup();
	PumpSetup();
	HeatSetup();
	BluetoothSetup();
	TemperatureSetup();
}

// the loop function runs over and over again until power down or reset
void loop() {
	Timer = millis();
	TemperatureRead();
	Bluetooth();
	Drive();
	Pump(PumpOn);
	AlarmBuzzSw(AlarmSw, 500, 500);
	
}
