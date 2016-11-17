#include <inttypes.h>

#define SPEC_CHANNELS    288

#define SPEC_TRG         A0
#define SPEC_ST          A1
#define SPEC_CLK         A2
#define SPEC_VIDEO       A3
#define WHITE_LED        A4
#define LASER_404        A5

#define BITSET(reg, bit) { *((volatile uint8_t*)&reg) |= (1 << bit); }
#define BITCLR(reg, bit) { *((volatile uint8_t*)&reg) &= ~(1 << bit); }

#define ST_BIT  1
#define CLK_BIT 2

// 4 cpu cycles total (sbi/cbi instructions)
__attribute__((always_inline))
static inline void clk_1(void) {
	BITSET(PORTC, CLK_BIT);
	BITCLR(PORTC, CLK_BIT);
}
__attribute__((always_inline))
static inline void clk_2(void) { clk_1(); clk_1(); }
__attribute__((always_inline))
static inline void clk_4(void) { clk_2(); clk_2(); }
__attribute__((always_inline))
static inline void clk_8(void) { clk_4(); clk_4(); }
__attribute__((always_inline))
static inline void clk_16(void) { clk_8(); clk_8(); }
__attribute__((always_inline))
static inline void clk_32(void) { clk_16(); clk_16(); }
__attribute__((always_inline))
static inline void clk_64(void) { clk_32(); clk_32(); }
__attribute__((always_inline))
static inline void clk_88(void) { clk_32(); clk_32(); clk_16(); clk_8(); }
__attribute__((always_inline))
static inline void clk_100(void) { clk_64(); clk_32(); clk_4(); }
__attribute__((always_inline))
static inline void clk_500(void) { clk_100(); clk_100(); clk_100(); clk_100(); clk_100(); }
static inline void clk_1000(void) { clk_500(); clk_500(); }
static inline void clk_4000(void) { clk_1000(); clk_1000(); clk_1000(); clk_1000(); }

static uint16_t data[SPEC_CHANNELS];

static void readData(int integration_time_ms)
{
	uint8_t sreg = SREG;
	// Disable interrupts ... to not get interrupted while integration
	cli();

	// Start clock cycle and set start pulse to signal start
	// SPEC_CLK -> LOW
	BITCLR(PORTC, CLK_BIT);
	clk_1();

	// SPEC_ST -> HIGH
	BITSET(PORTC, ST_BIT);

	// Sample for a period of ms (4000 clks is 1 ms)
	// F_CPU = 16 MHz -> 62.5 ns
	// clk_1 = 4 instructions -> 250 ns
	// clks_per_ms = 1000000 / 250 -> 4000 clks
	while (integration_time_ms-- > 0)
		clk_4000();

	// SPEC_ST -> LOW
	BITCLR(PORTC, ST_BIT);

	// Sample for a period of time (87 clocks)
	// One more clock pulse before the actual read
	clk_88();

	// Read from SPEC_VIDEO
	for(int i = 0; i < SPEC_CHANNELS; i++) {
		data[i] = analogRead(SPEC_VIDEO);
		clk_1();
	}

	// Sample for a small amount of time (7 clocks)
	clk_4();

	// SPEC_CLK -> HIGH
	BITSET(PORTC, CLK_BIT);

	// restore interrupts
	SREG = sreg;
}

static void printData(void)
{
	for (int i = 0; i < SPEC_CHANNELS; i++) {
		Serial.print(data[i]);
		Serial.print(',');
	}
	Serial.print("\n");
}

void setup(void)
{
	// configure pins
	pinMode(SPEC_TRG, INPUT);
	pinMode(SPEC_VIDEO, INPUT);
	pinMode(SPEC_CLK, OUTPUT);
	pinMode(SPEC_ST, OUTPUT);
	pinMode(LASER_404, OUTPUT);
	pinMode(WHITE_LED, OUTPUT);

	digitalWrite(SPEC_CLK, HIGH);
	digitalWrite(SPEC_ST, LOW);

	// configure serial
	Serial.begin(115200);
}

void loop(void)
{
	readData(20);
	printData();
}
