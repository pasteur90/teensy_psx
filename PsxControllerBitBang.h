#include "./PsxNewLib.h"

/** \brief Attention Delay
 *
 * Time between attention being issued to the controller and the first clock
 * edge (us).
 */
const byte ATTN_DELAY = 50;

/** \brief Clock Period
 *
 * Inverse of clock frequency, i.e.: time for a *full* clock cycle, from falling
 * edge to the next falling edge.
 */
const byte CLK_PERIOD = 40;

// Must be < CLK_PERIOD / 2
const byte HOLD_TIME = 2;


template <uint8_t PIN_ATT, uint8_t PIN_CMD, uint8_t PIN_DAT, uint8_t PIN_CLK>
class PsxControllerBitBang: public PsxController {
private:
	uint8_t att = PIN_ATT;
	uint8_t clk = PIN_CLK;
	uint8_t cmd = PIN_CMD;
	uint8_t dat = PIN_DAT;

protected:
	virtual void attention () override {
		digitalWrite(att, LOW);
		delayMicroseconds (ATTN_DELAY);
	}
	
	virtual void noAttention () override {
		// delayMicroseconds (5);
		digitalWrite(cmd, HIGH);
		digitalWrite(clk, HIGH);
		digitalWrite(att, HIGH);
		delayMicroseconds (ATTN_DELAY);
	}
	
	virtual byte shiftInOut (const byte out) override {
		byte in = 0;

		// 1. The clock is held high until a byte is to be sent.

		for (byte i = 0; i < 8; ++i) {
			// 2. When the clock edge drops low, the values on the line start to
			// change
			//digitalWrite(clk, LOW);

			//delayMicroseconds (HOLD_TIME);
			
			if (bitRead (out, i)) {
				digitalWrite(cmd, HIGH);

			} else {
				digitalWrite(cmd, LOW);
			}

			//delayMicroseconds (CLK_PERIOD / 2 - HOLD_TIME);
			digitalWrite(clk, LOW);
			delayMicroseconds(20);

			// 3. When the clock goes from low to high, value are actually read
			//digitalWrite(clk, HIGH);

			//delayMicroseconds (HOLD_TIME);
			
			byte temp = digitalRead(dat);
			if (temp) {
				bitSet (in, i);
			}

			digitalWrite(clk, HIGH);
			//delayMicroseconds (CLK_PERIOD / 2 - HOLD_TIME);
			delayMicroseconds(20);
		}

		return in;
	}

public:
	virtual boolean begin () override {
		pinMode(att, OUTPUT);
		digitalWrite(att, HIGH);
		pinMode(cmd, OUTPUT);
		pinMode(dat, INPUT_PULLUP);
		digitalWrite(dat, HIGH);
		pinMode(clk, OUTPUT);
		digitalWrite(clk, HIGH);
		return PsxController::begin ();
	}
};
