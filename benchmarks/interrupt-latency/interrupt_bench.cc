#include "../timing.h"
#include <timeout.h>
#include <compartment.h>
#include <debug.hh>
#include <thread.h>
#include <event.h>
#include <simulator.h>

using Debug = ConditionalDebug<DEBUG_INTERRUPT_BENCH, "Interrupt benchmark">;

/* XXX should be volatile, but we get away with it */
void *event_group;
volatile int start;

void __cheri_compartment("interrupt_bench") entry_high_priority()
{
	// high priority thread
	Timeout t = {0, UnlimitedTimeout};
	Debug::Invariant(event_create(&t, MALLOC_CAPABILITY, &event_group) == 0, "event create failed");
	int end = CHERI::with_interrupts_disabled([&]() {
		uint32_t bits = 0;
		event_bits_wait(&t, event_group, &bits, 1, EventWaitAll | EventWaitClearOnExit);
		return rdcycle();
	});
	MessageBuilder<ImplicitUARTOutput> out;
	out.format("Interrupt delivery took {} cycles / instructions\n", end - start);
	simulation_exit();
}

void __cheri_compartment("interrupt_bench") entry_low_priority()
{
	Debug::log("before yield");
	CHERI::with_interrupts_disabled([]() {
		uint32_t bits = 0;
		event_bits_set(event_group, &bits, 1, EventSetNoYield);
		start = rdcycle();
		yield();
	});
	// Should never reach
	Debug::log("after yield");
}
