/*
 * esp8266-pvoutput-logger project - data queue functions
 *
 * Copyright (C) 2015 Joey Loman <joey@binbash.org>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "queue.h"
#include "interrupt.h"
#include "config.h"

int queue_count = 0;

void
queue_post_items_to_pvoutput(void)
{
#ifdef DEBUG
    os_printf("[debug] queue_post_items_to_pvoutput\r\n");
#endif

    int i;

    if (queue_count == 0) {
        os_printf("[%s] [error] queue_post_items_to_pvoutput: post queue is empty!\r\n", date_time_get_ts());

        return;
    }

    for(i = 0; i < queue_count; i++) {
        os_printf("[%s] [info] queue_post_items_to_pvoutput: posting the following values to pvoutput -> date: %s / time: %s / average power: %ld / total energy: %ld\r\n",
            date_time_get_ts(), pq[i].q_date, pq[i].q_time, pq[i].q_power_gen, pq[i].q_total_energy_gen);
    }

    pvoutput_prepare_webserver_connection();
}

void
queue_post_items_to_thingspeak(void)
{
#ifdef DEBUG
    os_printf("[debug] queue_post_items_to_thingspeak\r\n");
#endif

    int i;

    if (queue_count == 0) {
        os_printf("[%s] [error] queue_post_items_to_thingspeak: post queue is empty!\r\n", date_time_get_ts());

        return;
    }

    for(i = 0; i < queue_count; i++) {
        os_printf("[%s] [info] queue_post_items_to_thingspeak: posting the following values to thingspeak -> date: %s / time: %s / average power: %ld / total energy: %ld\r\n",
            date_time_get_ts(), pq[i].q_date, pq[i].q_time, pq[i].q_power_gen, pq[i].q_total_energy_gen);
    }

    thingspeak_prepare_webserver_connection();
}

void
queue_update_post_queue(void)
{
#ifdef DEBUG
    os_printf("[debug] queue_update_post_queue\r\n");
#endif

    /* format the date for the queue */
    date_time_format_line();

    /* reset all vars */
    os_memset(pq[queue_count].q_date, 0, sizeof(pq[queue_count].q_date));
    os_memset(pq[queue_count].q_time, 0, sizeof(pq[queue_count].q_time));
    pq[queue_count].q_power_gen = 0;
    pq[queue_count].q_total_energy_gen = 0;

    /* fill them again */
    os_strncpy(pq[queue_count].q_date, date, sizeof(pq[queue_count].q_date));
    pq[queue_count].q_date[sizeof(pq[queue_count].q_date) - 1] = '\0';

    os_strncpy(pq[queue_count].q_time, time, sizeof(pq[queue_count].q_time));
    pq[queue_count].q_time[sizeof(pq[queue_count].q_time) - 1] = '\0';

    /* if the interval_pulse_count is 0 and we divide the total_watts by it the code will crash!
     * this happens when there is a timing issue between the interrupts and the scheduler.
     */
    if (interval_pulse_count == 0) {
        pq[queue_count].q_power_gen = total_watt;
    } else {
        /* calculate average watts */
        pq[queue_count].q_power_gen = total_watt / interval_pulse_count;
    }

    /* calculate the Watt hours:
     * 1000 Wh equals 1 kWh. if we know how many pulses equals 1kWh we can divide
     * it with the PULSE_FACTOR and multiply it by 1000 to get the Wh.
     * So in theory: (pulse_count / PULSE_FACTOR) * 1000
     * But to avoid floatingpoint numbers I use the following workaround:
     */
    pq[queue_count].q_total_energy_gen = ((pulse_count * 10000) / PULSE_FACTOR) / 10;

    queue_count++;

    /* reset the watt counter */
    total_watt = 0;
}
