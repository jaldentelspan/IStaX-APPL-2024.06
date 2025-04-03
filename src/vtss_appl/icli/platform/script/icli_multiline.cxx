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

#include "icli_multiline.h"
#include "icli_cmd_func.h"
#include "icli_porting_trace.h"
#include "vtss_icli_type.h"
#include "vtss_icli_session.h"

extern BOOL _submode_enter(u32 session_id, icli_cmd_mode_t mode);

static void icli_multiline_exit(u32 session_id)
{
    if (vtss_icli_session_mode_exit(session_id) < 0) {
        T_W("Failed to exit the multiline mode.\n");
        if (icli_config_go_to_exec_mode(session_id) == FALSE ) {
            T_E("Failed to end the multiline mode.\n");
        }
    }
    // clear any multiline state for this session
    vtss_icli_session_multiline_data_clear(session_id);

    return;
}

static mesa_rc icli_multiline_append_buffer(char *const buffer,
                                            const char *const s,
                                            uint max_len,
                                            uint src_len,
                                            u32 session_id)
{
    icli_cmd_mode_t mode;

    if (vtss_icli_session_mode_get(session_id, &mode) != ICLI_RC_OK) {
        return VTSS_RC_ERROR;
    }

    T_D("Append: %s, len %d", s, src_len);
    if (strlen(buffer) + src_len > max_len) {
        ICLI_PRINTF("%% The effective buffer size, i.e. excluding the delimiting "
                    "characters but including any newline characters (e.g. from "
                    "potential multi-line input), cannot be longer than %d. "
                    "Thus the entire buffer was rejected.\n"
                    , ICLI_MULTILINE_BUFFER_MAX_LEN - 1);
        if (mode == ICLI_CMD_MODE_MULTILINE) {
            icli_multiline_exit(session_id);
        }
        return VTSS_RC_ERROR;
    }
    strncat(buffer, s, src_len);

    return VTSS_RC_OK;
}

mesa_rc icli_multiline_process_line(u32 session_id, const char *line)
{
    uint len = 0;
    const char *tmp_char = NULL;
    BOOL delimiter_found = FALSE, inside_multiline = FALSE;
    icli_session_multiline_data_t state = {{ 0 }};
    icli_cmd_mode_t mode;

    T_D("Process line: %s, len " VPRIz, line, strlen(line));

    if (vtss_icli_session_mode_get(session_id, &mode) != ICLI_RC_OK) {
        return VTSS_RC_ERROR;
    }
    inside_multiline = mode == ICLI_CMD_MODE_MULTILINE;

    if (vtss_icli_session_multiline_data_get(session_id, &state) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (!inside_multiline) {
        state.delimiter = *line;
        line++;
    }

    // find the end char or EoS and measure the "effective" length at the same time
    for (tmp_char = line; *tmp_char != state.delimiter && *tmp_char != 0; tmp_char++) {
        len++;
    }
    // check if the end delimiter is present
    delimiter_found = *tmp_char == state.delimiter;

    if (icli_multiline_append_buffer(state.buffer, line, ICLI_MULTILINE_BUFFER_MAX_LEN - 1, len, session_id) \
        != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (delimiter_found) {
        if (state.callback(state.buffer) != VTSS_RC_OK) {
            T_W("The multiline set callback has failed.\n");
        }
        if (inside_multiline) {
            icli_multiline_exit(session_id);
        } else {
            // Single-line multilines (sigh...) are (MUST be) always in global config.
            // That way the "exec global config cmd from submode returns to global config"
            // feature keeps working.
            if (mode != ICLI_CMD_MODE_GLOBAL_CONFIG) {
                if (vtss_icli_session_mode_exit(session_id) < 0) {
                    T_E("Failed to return to global config mode!");
                    return VTSS_RC_ERROR;
                }
            }
        }
    } else {
        if (icli_multiline_append_buffer(state.buffer, "\n", ICLI_MULTILINE_BUFFER_MAX_LEN - 1, 1, session_id) \
            != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
        if (vtss_icli_session_multiline_data_set(session_id, &state) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
        if (!inside_multiline) {
            if (_submode_enter(session_id, ICLI_CMD_MODE_MULTILINE) == TRUE) {
                icli_session_handle_t *handle = NULL;
                handle = vtss_icli_session_handle_get(session_id);
                if (handle == NULL) {
                    T_W("Failed to get the handle for session %d.\n", session_id);
                    return VTSS_RC_ERROR;
                } else {
                    if (handle->runtime_data.line_number <= 0) {
                        ICLI_PRINTF("%% Entering multi-line text input mode. "
                                    "Type in text and exit the mode using the delimiting "
                                    "character '%c'. All input after that character will "
                                    "be silently ignored. The effective buffer size, "
                                    "i.e. excluding the delimiting characters but "
                                    "including any newline characters (e.g. from "
                                    "multi-line input), cannot be longer than %d.\n",
                                    state.delimiter, ICLI_MULTILINE_BUFFER_MAX_LEN - 1);
                    }
                }
            } else {
                // clear any multiline state for this session
                vtss_icli_session_multiline_data_clear(session_id);
                T_E("Failed to enter multiline input mode!"
                    " Buffer was thus rejected.\n");
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc icli_multiline_input_start(u32 session_id,
                                   icli_multiline_callback_t cb,
                                   const char *line)
{
    icli_cmd_mode_t mode;

    if (icli_session_mode_get(session_id, &mode) != ICLI_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (mode == ICLI_CMD_MODE_MULTILINE) {
        T_E("Already in multiline input mode!\n");
        return VTSS_RC_ERROR;
    }

    // Possible start of multiline mode
    // We store the callback to the multiline state for this session
    icli_session_multiline_data_t state = {{ 0 }};
    state.callback = cb;
    // We also clear the multiline state for this session
    icli_multiline_data_clear(session_id);
    if (icli_multiline_data_set(session_id, &state) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    return icli_multiline_process_line(session_id, line);
}


BOOL icli_multiline_parsing_begin(u32 session_id,
                                  const char *line)
{
    if (strchr(line + 1, *line) != NULL) {
        return FALSE;
    }
    icli_session_multiline_data_t state = {{ 0 }};
    state.delimiter = *line;
    if (vtss_icli_session_multiline_data_set(session_id, &state) != VTSS_RC_OK) {
        T_D("Failed to store the multiline parsing delimiter.\n");
        return FALSE;
    }
    T_D("Beginning Multiline parsing: <%s>", line);
    return TRUE;
}

BOOL icli_multiline_parsing_complete(u32 session_id,
                                     const char *line)
{
    icli_session_multiline_data_t state = {{ 0 }};
    if (vtss_icli_session_multiline_data_get(session_id, &state) != VTSS_RC_OK) {
        T_D("Failed to fetch the multiline parsing delimiter.\n");
        return FALSE;
    }
    T_D("Continuing Multiline parsing: <%s>", line);
    return strchr(line, state.delimiter) != NULL;
}
