// * -*- Mode: java; c-basic-offset: 4; tab-width: 8; c-comment-only-line-offset: 0; -*-
/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.

*/
// **********************************  QOS_UTIL.JS  ********************************
// *
// * Author: Joergen Andreasen
// *
// * --------------------------------------------------------------------------
// *
// * Description:  Common QoS JavaScript functions.
// *
// * To include in HTML file use:
// *
// * <script type="text/javascript" src="lib/qos_util.js"></script>
// *
// * --------------------------------------------------------------------------

/*
 * Calculate the actual weight in percent.
 * This calculation includes the round off errors that is caused by the
 * conversion from weight to cost that is done in the API.
 * See TN1049.
 *
 * param weight     [IN]   Array of weights (integer 1..100)
 * param nr_of_bits [IN]   Nr of bits in cost (5 bits on L26, Serval and Jaguar line ports, 7 bits on Jaguar host ports)
 * param pct        [OUT]  Array of percent (integer 1..100)
 *
 * return true if conversion is possible, otherwise false.
 */

function qos_weight2pct(weight, nr_of_bits, pct)
{
    var i;
    var cost       = [];
    var new_weight = [];
    var w_min      = 100;
    var c_max_arch = Math.pow(2, nr_of_bits);
    var c_max      = 0;
    var w_sum      = 0;

    // Check input parameters and save the lowest weight for use in next round
    for (i = 0; i < weight.length; i++) {
        if (isNaN(weight[i]) || (weight[i] < 1) || (weight[i] > 100)) {
            return false;  // Bail out on invalid weight
        }
        else {
            w_min = Math.min(w_min, weight[i]);
        }
    }

    for (i = 0; i < weight.length; i++) {
        cost[i] = Math.max(1, Math.round((c_max_arch * w_min) / weight[i])); // Calculate cost for each weight (1..c_max_arch)
        c_max = Math.max(c_max, cost[i]); // Save the highest cost for use in next round
    }

    for (i = 0; i < weight.length; i++) {
        new_weight[i] = Math.round(c_max / cost[i]); // Calculate back to weight
        w_sum += new_weight[i]; // Calculate the sum of weights for use in next round
    }

    for (i = 0; i < weight.length; i++) {
        pct[i] = Math.max(1, Math.round((new_weight[i] * 100) / w_sum)); // Convert new_weight to percent (1..100)
    }
    return true;
}

/*
 * Convert frame_type to text.
 *
 * param frame_type [IN] Frame type as given by vtss_qce_type_t
 *
 * return text representation if frame type is valid, otherwise "????".
 */

function qos_frame_type2txt(frame_type)
{
    switch (frame_type) {
    case 0:
        return("Any");
    case 1:
        return("EtherType");
    case 2:
        return("LLC");
    case 3:
        return("SNAP");
    case 4:
        return("IPv4");
    case 5:
        return("IPv6");
    default:
        return("????");
    }
}

/*
 * Returns the smallest of two rates.
 * If one of the rates is zero, it is ignored and the other rate is returned.
 * Both ratess must not be zero!
 *
 * param r1 [IN] First rate to compare
 * param r2 [IN] Second rate to compare
 *
 * return The smallest of the two rates.
 */

function qos_rate_min(r1, r2)
{
    var min = 0xffffffff;

    if (r1) {
        min = r1;
    }
    if (r2) {
        min = Math.min(min, r2);
    }
    return min;
}

/*
 * Returns the largest of two rates.
 * If one of the rates is zero, it is ignored and the other rate is returned.
 * Both rates must not be zero!
 *
 * param r1 [IN] First rate to compare
 * param r2 [IN] Second rate to compare
 *
 * return The largest of the two rates.
 */

function qos_rate_max(r1, r2)
{
    var max = 0;

    if (r1) {
        max = r1;
    }
    if (r2) {
        max = Math.max(max, r2);
    }
    return max;
}

/*
 * Convert rate.
 *
 * param rate [IN] The rate
 *
 * return rate / 1000 if rate is dividable with 1000, else rate
 */
function qos_display_rate(rate)
{
    if (rate % 1000) {
        return rate;
    } else {
        return rate / 1000;
    }
}

/*
 * Convert rate and frame_rate to unit enum.
 *
 * param rate       [IN] The rate
 * param frame_rate [IN] False: rate unit is kbps. True: rate unit is fps
 *
 * return An enum coresponding to the rate unit:
 *  0: kbps
 *  1: Mbps
 *  2: fps
 *  3: kfps
 */
function qos_display_rate_unit(rate, frame_rate)
{
    if (rate % 1000) {
        if (frame_rate) {
            return 2; // ~ fps
        } else {
            return 0; // ~ kbps
        }
    } else {
        if (frame_rate) {
            return 3; // ~ kfps
        }
        else {
            return 1; // ~ Mbps
        }
    }
}

/*
 * Convert rate and frame_rate to text.
 *
 * param rate       [IN] The rate
 * param frame_rate [IN] False: rate unit is kbps. True: rate unit is fps
 *
 * return A text representation e.g. "367 kbps".
 */
function qos_display_rate_text(rate, frame_rate)
{
    if (rate % 1000) {
        if (frame_rate) {
            return rate + " fps";
        } else {
            return rate + " kbps";
        }
    } else {
        if (frame_rate) {
            return (rate / 1000) + " kfps";
        }
        else {
            return (rate / 1000) + " Mbps";
        }
    }
}

