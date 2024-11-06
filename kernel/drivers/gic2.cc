#include "gic2.h"

namespace wwos::kernel {

template <typename T>
void write_volatile(size_t addr, T value) {
    *reinterpret_cast<volatile T*>(addr) = value;
}

template <typename T>
T read_volatile(size_t addr) {
    return *reinterpret_cast<volatile T*>(addr);
}


static constexpr size_t GICD_CTLR = 0x0000;
static constexpr size_t GICD_ISENABLER = 0x0100;
static constexpr size_t GICD_ICPENDR = 0x0280;
static constexpr size_t GICD_ITARGETSR = 0x0800;
static constexpr size_t GICD_IPRIORITYR = 0x0400;
static constexpr size_t GICD_ICFGR = 0x0c00;

static constexpr size_t GICD_CTLR_ENABLE = 1;
static constexpr size_t GICD_ISENABLER_SIZE = 32;
static constexpr size_t GICD_ICPENDR_SIZE = 32;
static constexpr size_t GICD_ITARGETSR_SIZE = 4;
static constexpr size_t GICD_ITARGETSR_BITS = 8;

static constexpr size_t GICD_IPRIORITY_SIZE = 4;
static constexpr size_t GICD_IPRIORITY_BITS = 8;
static constexpr size_t GICD_ICFGR_SIZE = 16;
static constexpr size_t GICD_ICFGR_BITS = 2;

static constexpr size_t GICC_CTLR = 0x0000;
static constexpr size_t GICC_PMR = 0x0004;
static constexpr size_t GICC_BPR = 0x0008;
static constexpr size_t GICC_IAR = 0x000c; // https://developer.arm.com/documentation/ddi0488/h/generic-interrupt-controller-cpu-interface/gic-programmers-model/cpu-interface-register-summary?lang=en
static constexpr size_t GICC_EOIR = 0x0010; // https://developer.arm.com/documentation/ddi0488/h/generic-interrupt-controller-cpu-interface/gic-programmers-model/cpu-interface-register-summary?lang=en

static constexpr size_t GICC_CTLR_ENABLE = 1;

static constexpr size_t GICC_PMR_PRIO_LOW = 0xff;
static constexpr size_t GICC_BPR_NO_GROUP = 0x00;


gic02_driver::gic02_driver(size_t gicd_base, size_t gicc_base) : gicd_base(gicd_base), gicc_base(gicc_base) {}

void gic02_driver::initialize() {
    write_volatile<uint32_t>(gicd_base + GICD_CTLR, GICD_CTLR_ENABLE);
    write_volatile<uint32_t>(gicc_base + GICC_CTLR, GICC_CTLR_ENABLE);
    write_volatile<uint32_t>(gicc_base + GICC_PMR, GICC_PMR_PRIO_LOW);
    write_volatile<uint32_t>(gicc_base + GICC_BPR, GICC_BPR_NO_GROUP);
}

void gic02_driver::enable(size_t interrupt) {
    write_volatile<uint32_t>(
        gicd_base + GICD_ISENABLER + (interrupt / GICD_ISENABLER_SIZE) * 4,
        1 << (interrupt % GICD_ISENABLER_SIZE)
    );
}

void gic02_driver::disable(size_t interrupt) {
    write_volatile<uint32_t>(
        gicd_base + GICD_ISENABLER + (interrupt / GICD_ISENABLER_SIZE) * 4,
        1 << (interrupt % GICD_ISENABLER_SIZE)
    );
}

void gic02_driver::clear(size_t interrupt) {
    write_volatile<uint32_t>(
        gicd_base + GICD_ICPENDR + (interrupt / GICD_ICPENDR_SIZE) * 4,
        1 << (interrupt % GICD_ICPENDR_SIZE)
    );
}

void gic02_driver::set_core(size_t interrupt, size_t core) {
    size_t shift = (interrupt % GICD_ITARGETSR_SIZE) * GICD_ITARGETSR_BITS;
    size_t addr = gicd_base + GICD_ITARGETSR + (interrupt / GICD_ITARGETSR_SIZE) * 4;
    uint32_t value = read_volatile<uint32_t>(addr);
    value &= ~(0xff << shift);
    value |= core << shift;
    write_volatile<uint32_t>(addr, value);
}

void gic02_driver::set_priority(size_t interrupt, size_t priority) {
    size_t shift = (interrupt % GICD_IPRIORITY_SIZE) * GICD_IPRIORITY_BITS;
    size_t addr = gicd_base + GICD_IPRIORITYR + (interrupt / GICD_IPRIORITY_SIZE) * 4;
    uint32_t value = read_volatile<uint32_t>(addr);
    value &= ~(0xff << shift);
    value |= priority << shift;
    write_volatile<uint32_t>(addr, value);
}

void gic02_driver::set_config(size_t interrupt, size_t config) {
    size_t shift = (interrupt % GICD_ICFGR_SIZE) * GICD_ICFGR_BITS;
    size_t addr = gicd_base + GICD_ICFGR + (interrupt / GICD_ICFGR_SIZE) * 4;
    uint32_t value = read_volatile<uint32_t>(addr);
    value &= ~(0x03 << shift);
    value |= config << shift;
    write_volatile<uint32_t>(addr, value);
}

// GICC_IAR & GICC_EOIR contains CPUID && interrupt ID
// currently support only CPU 0

uint32_t gic02_driver::get_interrupt_id() {
    return read_volatile<uint32_t>(gicc_base + GICC_IAR);
}

void gic02_driver::finish_interrupt(uint32_t interrupt_id) {
    write_volatile<uint32_t>(gicc_base + GICC_EOIR, interrupt_id);
}

}


