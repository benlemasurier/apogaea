#include <XBee.h>

#define SOUND_PIN A7
#define CALL_PIN A2

XBee xbee = XBee();

struct el_wire {
	bool on;
	uint8_t pin;
	uint8_t delay;
	uint8_t delay_step;
};

struct chaser {
	uint8_t pins[3];
	uint8_t step;

	uint8_t mod;
	uint8_t threshold;

	int delay;
	int delay_step;
};

typedef uint8_t EFFECT[3][3];

struct el_wire el_wire_new(uint8_t pin, int delay);
void el_wire_step(struct el_wire *wire, int intensity);

void chaser_low(struct chaser *wire);
void chaser_high(struct chaser *wire);
void chaser_output(struct chaser wire);
void chaser_step(struct chaser *wire, EFFECT e, int intensity);
struct chaser chaser_new(uint8_t a, uint8_t b, uint8_t c, int delay, uint8_t mod, uint8_t threshold);

EFFECT forward = {
	{ HIGH, LOW,  LOW  },
	{ LOW,  HIGH, LOW  },
	{ LOW,  LOW,  HIGH },
};

EFFECT backward = {
	{ LOW,  LOW,  HIGH },
	{ LOW,  HIGH, LOW  },
	{ HIGH, LOW,  LOW  },
};

EFFECT blink = {
	{ HIGH, HIGH, HIGH },
	{ LOW,  LOW,  LOW  },
	{ LOW,  LOW,  LOW  },
};

EFFECT twitch = {
	{ HIGH, LOW,  HIGH },
	{ LOW,  LOW,  LOW  },
	{ LOW,  HIGH, LOW  },
};

EFFECT single = {
	{ LOW, LOW,  LOW },
	{ LOW, HIGH, LOW },
	{ LOW, LOW,  LOW },
};

EFFECT dual = {
	{ HIGH, LOW, HIGH },
	{ LOW,  LOW, LOW  },
	{ LOW,  LOW, LOW  },
};

EFFECT triple = {
	{ HIGH, HIGH, HIGH },
	{ LOW,  LOW,  LOW  },
	{ LOW,  LOW,  LOW  },
};

void
chaser_output(struct chaser wire)
{
	uint8_t i;
	for (i = 0; i < 3; i++) {
		pinMode(wire.pins[i], OUTPUT);
	}
}

void
chaser_step(struct chaser *wire, EFFECT e, int intensity)
{
	for(uint8_t i = 0; i < 3; i++) {
		digitalWrite(wire->pins[i], e[wire->step][i]);
	}

	wire->delay_step += intensity;
	if(wire->delay_step >= wire->delay) {
		wire->delay_step = 0;

		wire->step++;
		if(wire->step >= 3) {
			wire->step = 0;
		}
	}
}

void
chaser_low(struct chaser *wire)
{
	for(uint8_t i = 0; i < 3; i++) {
		digitalWrite(wire->pins[i], LOW);
	}
}

void
chaser_high(struct chaser *wire)
{
	for(uint8_t i = 0; i < 3; i++) {
		digitalWrite(wire->pins[i], HIGH);
	}
}

struct chaser
chaser_new(uint8_t a, uint8_t b, uint8_t c, int delay, uint8_t mod, uint8_t threshold)
{
	struct chaser wire;
	wire.pins[0] = a;
	wire.pins[1] = b;
	wire.pins[2] = c;
	wire.step = 0;

	wire.delay = delay;
	wire.delay_step = 0;

	wire.mod = mod;
	wire.threshold = threshold;

	chaser_output(wire);

	return wire;
}

EFFECT *
random_effect(void)
{
	return &forward;

	//switch(random(0, 4)) {
	//	case 0:
	//		return &forward;
	//	case 1:
	//		return &backward;
	//	case 2:
	//		return &blink;
	//	case 3:
	//		return &twitch;
	//}
}

struct el_wire
el_wire_new(uint8_t pin, int delay)
{
	struct el_wire wire;
	wire.on = true;
	wire.pin = pin;
	wire.delay = delay;
	wire.delay_step = 0;

	pinMode(wire.pin, OUTPUT);

	return wire;
}

void
el_wire_step(struct el_wire *wire, int intensity)
{
	wire->delay_step += intensity;
	if(wire->delay_step >= wire->delay) {
		if(wire->on) {
			digitalWrite(wire->pin, LOW);
			wire->on = false;
			wire->delay_step = wire->delay - (wire->delay / 1.2);
		} else {
			digitalWrite(wire->pin, HIGH);
			wire->on = true;
			wire->delay_step = 0;
		}
	}
}

struct chaser wire_one;
struct chaser wire_two;
struct el_wire el_one;
struct el_wire el_two;
EFFECT *mode = &forward;
char incoming_call = 'n';

void
setup()
{
	Serial.begin(115200);
	randomSeed(analogRead(0));
	
	wire_one = chaser_new(2, 3, 4, 3000, 10, 0);
	wire_two = chaser_new(5, 6, 7, 3000, 10, 0);
	
	el_one = el_wire_new(8, 2000);
	el_two = el_wire_new(9, 2000);

	chaser_low(&wire_one);
	chaser_low(&wire_two);

	// sound sensor
	pinMode(SOUND_PIN, INPUT);

	// call switch
	pinMode(CALL_PIN, INPUT);

	// status LED
	pinMode(13, OUTPUT);

	Serial.println("sequencer 0: ready");

	// wait for xbee
	delay(1000);
}

void
loop()
{
	//EFFECT *mode;
	int soundLevel;
	bool outgoing_call = false;

	outgoing_call = (digitalRead(CALL_PIN) == HIGH);
	if(outgoing_call) { 
		Serial.print('y');
	} else {
		Serial.print('n');
	}
	
	soundLevel = (analogRead(SOUND_PIN) - 370);
	if(soundLevel < 0) {
		soundLevel = 0;
	}

	if(Serial.available()) {
		incoming_call = Serial.read();
		if(incoming_call == 'y') {
			Serial.println(incoming_call);
		}
	}

	if(incoming_call == 'y') {
		mode = &blink;
	} else {
		mode = &forward;
	}

	chaser_step(&wire_one, *mode, soundLevel);
	chaser_step(&wire_two, *mode, soundLevel);
	el_wire_step(&el_one, soundLevel);
	el_wire_step(&el_two, soundLevel);
}
