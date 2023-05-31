#ifndef CORE_TIME_H
#define CORE_TIME_H

#include <stdint.h>

/**
 * Gets the current time
 * @return Current time in milliseconds
 */
uint32_t time_get_millis(void);

/**
 * Sets the current time
 * @param millis Current milliseconds
 */
void time_set_millis(uint32_t millis);

#endif // CORE_TIME_H
