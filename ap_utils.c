/** \file ap_utils.c
 * \brief Part of AP's Toolkit. Miscellaneous utility functions module
 */
#define AP_UTILS_C
#include "ap_utils.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*=========================================================*/
/** \brief Compares struct timeval against current time, returning result of then <=> now
 *
 * \param tv struct timeval *
 * \return int - -1 if less than now, 0 if equal, 1 if past current time
 */
int ap_utils_timeval_cmp_to_now(struct timeval *tv)
{
    struct timeval now;

    gettimeofday(&now, NULL);

    if ( timercmp(tv, &now, >) )
        return 1;
    else if ( timercmp(tv, &now, <) )
        return -1;

    return 0;
}

/*=========================================================*/
/** \brief Set struct timeval value based on mode flag
 *
 * \param tv struct timeval *
 * \param mode int - AP_UTILS_TIME_*
 * \param msec int - milliseconds. Is it absolute value or relative depends on mode
 * \return int - true/false
 *
 * AP_UTILS_TIME_ADD - Adds specified milliseconds to the current tv value
 * AP_UTILS_TIME_SUB - Subtracts specified milliseconds from the current tv value
 * AP_UTILS_TIME_SET_FROM_NOW - Adds specified milliseconds to the current time and store it in tv
 * AP_UTILS_TIME_SET_FROMZERO - Sets tv value exactly to milliseconds value
 *
 */
int ap_utils_timeval_set(struct timeval *tv, int mode, int msec)
{
	  struct timeval tmp;


	  if ( msec < 1 )
		  return 0;

	  switch( mode )
	  {
		  case AP_UTILS_TIME_ADD:
			  tmp.tv_sec = msec / 1000;
			  tmp.tv_usec = 1000 * (msec % 1000);
			  timeradd(tv, &tmp, tv);
			  break;

		  case AP_UTILS_TIME_SUB:
			  tmp.tv_sec = msec / 1000;
			  tmp.tv_usec = 1000 * (msec % 1000);
			  timersub(tv, &tmp, tv);
			  break;

		  case AP_UTILS_TIME_SET_FROM_NOW:
			  gettimeofday(tv, NULL);
			  tmp.tv_sec = msec / 1000;
			  tmp.tv_usec = 1000 * (msec % 1000);
			  timeradd(tv, &tmp, tv);
			  break;

		  case AP_UTILS_TIME_SET_FROMZERO:
			  tv->tv_sec = msec / 1000;
			  tv->tv_usec = 1000 * (msec % 1000);
			  break;

		  default:
			  return 0;
	  }

	  return 1;
}

#define MAX_NSEC 1000000000l

/*=========================================================*/
/** \brief Compares struct timespec against current time, returning result of then <=> now
 *
 * \param ts struct timespec *
 * \return int - -1 if less than now, 0 if equal, 1 if past current time
 */
int ap_utils_timespec_cmp_to_now(struct timespec *ts)
{
    struct timespec now;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    if ( ts->tv_sec > now.tv_sec )
        return 1;

    if ( ts->tv_sec < now.tv_sec )
    	return -1;

    if ( ts->tv_nsec > now.tv_nsec )
        return 1;

    if ( ts->tv_nsec < now.tv_nsec )
        return -1;

    return 0;
}

/*=========================================================*/
/** \brief Set struct timespec value based on mode flag
 *
 * \param ts struct timespec *
 * \param mode int - AP_UTILS_TIME_*
 * \param msec int - milliseconds. Is it absolute value or relative depends on mode
 * \return int - true/false
 *
 * AP_UTILS_TIME_ADD - Adds specified milliseconds to the current ts value
 * AP_UTILS_TIME_SUB - Subtracts specified milliseconds from the current ts value
 * AP_UTILS_TIME_SET_FROM_NOW - Adds specified milliseconds to the current time and store it in ts
 * AP_UTILS_TIME_SET_FROMZERO - Sets ts value exactly to milliseconds value
 *
 */
int ap_utils_timespec_set(struct timespec *ts, int mode, int msec)
{
	long tmp;


	if ( msec < 0 )
		return 0;

	switch( mode )
	{
		case AP_UTILS_TIME_ADD:
			ts->tv_nsec += msec * 1000000l;
			break;

		case AP_UTILS_TIME_SUB:
			ts->tv_nsec -= msec * 1000000l;
			break;

    	case AP_UTILS_TIME_SET_FROM_NOW:
    		clock_gettime(CLOCK_MONOTONIC_RAW, ts);
    		ts->tv_nsec += msec * 1000000l;
    		break;

    	case AP_UTILS_TIME_SET_FROMZERO:
    		ts->tv_sec = msec / 1000;
    		ts->tv_nsec = MAX_NSEC * (msec % 1000l);
    		return 1;

    	default:
    		return 0;
	}

    if ( ts->tv_nsec >= MAX_NSEC )
    {
    	tmp = ts->tv_nsec / MAX_NSEC;
        ts->tv_nsec -= tmp * MAX_NSEC;
        ts->tv_sec += tmp;
    }
    else if ( ts->tv_nsec < 0l )
    {
    	tmp = labs(ts->tv_nsec) / MAX_NSEC;
        ts->tv_nsec = MAX_NSEC + ts->tv_nsec + tmp * MAX_NSEC;
        ts->tv_sec -= tmp;
    }

    return 1;
}

/*=========================================================*/
/** \brief Set struct timespec value to zero
 *
 * \param ts struct timespec *
 * \return void
 */
void ap_utils_timespec_clear(struct timespec *ts)
{
	ts->tv_nsec = 0l; ts->tv_sec = 0;
}

/*=========================================================*/
/** \brief Check if struct timespec value is non-zero
 *
 * \param ts struct timespec *
 * \return int - true/false
 */
int ap_utils_timespec_is_set(struct timespec *ts)
{
	return (ts->tv_sec > 0 || ts->tv_nsec > 0l);
}

/*=========================================================*/
/** \brief Like timeradd() for timespec: adds value of a to b, placing result in destination
 *
 * \param a struct timespec * - Operand 1
 * \param b struct timespec * - Operand 2
 * \param destination struct timespec * - Destination for sum a+b
 * \return void
 */
void ap_utils_timespec_add(struct timespec *a, struct timespec *b, struct timespec *destination)
{
	long tmp;


	destination->tv_sec = a->tv_sec + b->tv_sec;
	destination->tv_nsec = a->tv_nsec + b->tv_nsec;

	if ( destination->tv_nsec >= MAX_NSEC )
    {
    	tmp = destination->tv_nsec / MAX_NSEC;
    	destination->tv_nsec -= tmp * MAX_NSEC;
    	destination->tv_sec += tmp;
    }
}

/*=========================================================*/
/** \brief Like timersub() for timespec: subtracts value of b from a, placing result in destination
 *
 * \param a struct timespec * - Minuend
 * \param b struct timespec * - Subtrahend
 * \param destination struct timespec * - Destination for result of a-b
 * \return void
 */
void ap_utils_timespec_sub(struct timespec *a, struct timespec *b, struct timespec *destination)
{
	long tmp;


	destination->tv_sec = a->tv_sec - b->tv_sec;
	destination->tv_nsec = a->tv_nsec - b->tv_nsec;

    if ( destination->tv_nsec <= -MAX_NSEC )
    {
    	tmp = destination->tv_nsec / MAX_NSEC;
    	destination->tv_nsec = destination->tv_nsec + tmp * MAX_NSEC;
    	destination->tv_sec += tmp;
    }
}

/*=========================================================*/
/** \brief Counts elapsed time in various ranges
 *
 * \param begin struct timespec * - Beginning of time period. Can be NULL to take the current time instead
 * \param end struct timespec * - End of time period. Can be NULL to take the current time instead
 * \param destination struct timespec * - Destination for result. Can be NULL if user needs only returned milliseconds amount
 * \return int Milliseconds amount in given period
 *
 * If *begin and *end both is not NULL then counting amount of time between begin and end.
 * If begin is NULL then counting from current clock to *end
 * If end is NULL then counting from *begin to current clock.
 */
long ap_utils_timespec_elapsed(struct timespec *begin, struct timespec *end, struct timespec *destination)
{
	struct timespec now;


	if ( begin == NULL && end == NULL )
		return 0;

	if ( begin == NULL )
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		begin = &now;
	}
	else if ( end == NULL )
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		end = &now;
	}

	if ( destination == NULL )
		destination = &now;

	ap_utils_timespec_sub(end, begin, destination);

	return destination->tv_sec * 1000l + destination->tv_nsec / 1000000l;
}

/*=========================================================*/
/** \brief Converts struct timespec value to milliseconds
 *
 * \param ts struct timespec * - Beginning of time period. Must be set always
 * \return int Milliseconds amount
 */
long ap_utils_timespec_to_milliseconds(struct timespec *ts)
{
	long retval;


	retval = ts->tv_sec * 1000l;

    if ( ts->tv_nsec > 0l )
    	retval += ts->tv_nsec / 1000000l;

    return retval;
}

/*=========================================================*/
/** \brief Standard CRC 16
 *
 * \param mem void * - Memory block to process
 * \param len int - length of data
 * \return uint16_t counted CRC 16
 */
uint16_t count_crc16(void *mem, int len)
{
  uint16_t a, crc16;
  uint8_t *pch;

  pch = (uint8_t *)mem;
  crc16 = 0;

  while(len--)
  {
    crc16 ^= *pch;
    a = (crc16 ^ (crc16 << 4)) & 0x00FF;
    crc16 = (crc16 >> 8) ^ (a << 8) ^ (a << 3) ^ (a >> 4);
    ++pch;
  }

  return(crc16);
}
