#ifndef TOOLS_H_
#define TOOLS_H_

// bit operation
#define bit_set(val, bitPos)           (val |= 1 << bitPos)
#define bit_clear(val, bitPos)         (val &= ~( 1 << bitPos))
#define bit_toggle(val, bitPos)        (val ^= 1 << bitPos)
#define bit_get(out, val, bitPos)      (out = (val >> bitPos) & 1)
#define bit_change(val, bitPos, to)    (val ^= (-to ^ val) & (1 << bitPos))

// Serial Print
#define SPT(str,value) Serial.print(String(str) + value)
#define SPL(str,value) Serial.println(String(str) + value)
#define SPH(str,value) { Serial.print(String("[HEX]") + str); Serial.println(value,HEX); }

// Others
#define DONT_CARE       0
#define between(valueToCompare, rangeMin, rangeMax) (( valueToCompare >= rangeMin ) && ( valueToCompare <= rangeMax ))


#endif