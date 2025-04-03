/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <vtss/basics/mutex.hxx>

#include "web.h"
#include "web_api.h"
#include "vtss/appl/ssh.h"
#include "port_api.h" // For port_count_max()

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

static vtss_trace_reg_t trace_reg =
{
    VTSS_TRACE_MODULE_ID, "web", "Web server"
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/*
 * Pseudo CLI IO interface layer
 */
/*lint -esym(459,cli_io_fifo)*/
struct cli_io_fifo {
    // Base IO layer - must be first in this struct definition!
    cli_iolayer_t base;

    // Is instance currently in use?
    int in_use = FALSE;

    // Used by remote end to signal that it is done with this IO layer instance
    int process_done = FALSE;
    // Time when instance was set into use
    time_t start_time;

    int fd_web;
    char tmp_fifo_name[128];

    // Mutex to protect the struct members as they may be used by different threads.
    vtss::Critd mutex;

    // Default ctor - implemented below
    cli_io_fifo();

    // Set the instance to "in use" state and reset the IO buffer
    void set_inuse(int i)
    {
        if (in_use) return;
        T_D("begin");
        base.cli_init(&base);
        in_use = TRUE;
        base.icli_session_id = i;
        start_time = time(NULL);

        sprintf(tmp_fifo_name, "/tmp/web_cli_io_%d", base.icli_session_id);
        int rc = mkfifo(tmp_fifo_name, 0666);
        if (rc < 0) {
            T_E("Failed creating fifo: %s", strerror(errno));
            return;
        }
        T_D("Created fifo: %s", tmp_fifo_name);


        fd_web = -1;
    }

    // Reset buffer to start values
    void reset_buf()
    {
        T_D("begin");
    }
};

static
void cli_io_fifo_init(cli_iolayer_t *pIO)
{
    T_D("pIO: %p", pIO);
    struct cli_io_fifo *pMIO = (struct cli_io_fifo *) pIO;
    pMIO->reset_buf();
}

static
int cli_io_fifo_getch(struct cli_iolayer *pIO, int timeout, char *ch)
{
    T_D("pIO: %p", pIO);
    return CLI_ERROR_IO_TIMEOUT;
}

static
int cli_io_fifo_vprintf(cli_iolayer_t *pIO, const char *fmt, va_list ap)
{
    struct cli_io_fifo *pMIO = (struct cli_io_fifo *) pIO;
    T_D("pIO: %p, base.fd: %d", pIO, pMIO->base.fd);
    if (pMIO->base.fd < 0) {
        T_E("No fifo open");
        return 0;
    }
    vtss::lock_guard<vtss::Critd> iolock(__FILE__, __LINE__, pMIO->mutex);
    char buf[1024];
    int ct = vsnprintf(buf, sizeof(buf), fmt, ap);
    if (ct < 0) {
        T_E("Printing error: %s", strerror(errno));
        return 0;
    }
    if (ct >= sizeof(buf)) {
        // Output truncated
        ct = sizeof(buf);
    }
    T_D("Writing %d bytes: %s\n", ct, buf);
    ct = write(pMIO->base.fd,buf,ct);
    if (ct < 0) {
        T_E("Failed writing: %s. %s\n", buf, strerror(errno));
    }
    return ct;
}

static
void cli_io_fifo_putchar(cli_iolayer_t *pIO, char ch)
{
    T_D("%s pIO: %p", __FUNCTION__, pIO);
    struct cli_io_fifo *pMIO = (struct cli_io_fifo *) pIO;
    vtss::lock_guard<vtss::Critd> iolock(__FILE__, __LINE__, pMIO->mutex);

    if (pMIO->base.fd < 0) {
        T_E("No fifo open");
        return;
    }

    int ct = write(pMIO->base.fd,&ch,1);
    if (ct < 0) {
        T_E("Error in write: %s", strerror(errno));
    }
    return;
}

static
void cli_io_fifo_puts(cli_iolayer_t *pIO, const char *str)
{
    T_D("%s begin", __FUNCTION__);
    struct cli_io_fifo *pMIO = (struct cli_io_fifo *) pIO;
    vtss::lock_guard<vtss::Critd> iolock(__FILE__, __LINE__, pMIO->mutex);

    if (pMIO->base.fd < 0) {
        T_E("No fifo open");
        return;
    }

    int ct = write(pMIO->base.fd,str,strlen(str));
    if (ct < 0 ) {
        T_E("Error in write: %s", strerror(errno));
    }
    return;
}

static
void cli_io_fifo_dummy(cli_iolayer_t *pIO)
{
    T_D("pIO: %p", pIO);
}

static
void cli_io_fifo_close(cli_iolayer_t *pIO)
{
    T_D("pIO: %p", pIO);
    struct cli_io_fifo *pMIO = (struct cli_io_fifo *) pIO;

    int rc = close(pMIO->base.fd);
    if (rc < 0) {
        T_E("Could not close %s. %s", pMIO->tmp_fifo_name, strerror(errno));
    }

    rc = unlink(pMIO->tmp_fifo_name);
    if (rc < 0) {
        T_E("Could not remove %s. %s", pMIO->tmp_fifo_name, strerror(errno));
    }

    pMIO->process_done = 1;
}

// Default ctor for Web CLI IO layer entries
cli_io_fifo::cli_io_fifo() :
    mutex("cli_io_fifo", VTSS_MODULE_ID_WEB)
{
    T_D("%s begin", __FUNCTION__);
    base.cli_init = cli_io_fifo_init;
    base.cli_getch = cli_io_fifo_getch;
    base.cli_vprintf = cli_io_fifo_vprintf;
    base.cli_putchar = cli_io_fifo_putchar;
    base.cli_puts = cli_io_fifo_puts;
    base.cli_flush = cli_io_fifo_dummy;
    base.cli_close = cli_io_fifo_close;
    base.cli_login = 0;

    base.fd = -1;
    base.char_timeout = 0;
    base.bIOerr = 0;
    base.bEcho = FALSE;
    base.cDEL = CLI_DEL_KEY_WINDOWS;
    base.cBS = CLI_BS_KEY_WINDOWS;

    // Call base struct initialization
    base.cli_init(&base);
}

#define WEB_CLI_IO_TIMEOUT      36000   // 6 minutes

/*
 * Array of available IO layer instances
 */
static struct cli_io_fifo cli_io_fifo[WEB_CLI_IO_MAX];

cli_iolayer_t *
web_get_iolayer(int web_io_type)
{
    T_D("%s begin", __FUNCTION__);
    int i = web_io_type;

    if (web_io_type == WEB_CLI_IO_TYPE_PING) {
        for (; i < WEB_CLI_IO_MAX; i++) {
            if (!cli_io_fifo[i].in_use || (cli_io_fifo[i].in_use && time(NULL) - cli_io_fifo[i].start_time > WEB_CLI_IO_TIMEOUT)) {
                break;
            }
        }
    }

    if (i < WEB_CLI_IO_MAX) {
        // Found available entry
        cli_io_fifo[i].set_inuse(i);
        T_D("%s found io: %p", __FUNCTION__, &cli_io_fifo[i].base);
        return &cli_io_fifo[i].base;
    }

    return NULL;
}

cli_iolayer_t *web_set_cli(cli_iolayer_t *pIO)
{
    T_D("begin");
    cli_set_io_handle(pIO);
    struct cli_io_fifo *pMIO = (struct cli_io_fifo *) pIO;

    pMIO->base.fd = open(pMIO->tmp_fifo_name, O_WRONLY);
    if (pMIO->base.fd < 0) {
        T_E("Failed opening fifo: %s. %s", pMIO->tmp_fifo_name, strerror(errno));
        return NULL;
    }
    T_D("Opened %s for writing", pMIO->tmp_fifo_name);
    return pIO;
}

/* When web_io_type is equal to WEB_CLI_IO_TYPE_BOOT_LOADER or WEB_CLI_IO_TYPE_FIRMWARE,
   the parameter of "io" should be NULL.
   When web_io_type is equal to WEB_CLI_IO_TYPE_PING,
   the parameter of "io" should be specific memory address.
  */
void web_send_iolayer_output(int web_io_type, cli_iolayer_t *io, const char *mimetype)
{
    T_D("%s begin io = %p", __FUNCTION__, io);
    int i = web_io_type;

    if (web_io_type == WEB_CLI_IO_TYPE_PING) {
        for (; i < WEB_CLI_IO_MAX; i++) {
            if (cli_io_fifo[i].in_use && io == &cli_io_fifo[i].base) {
                break;
            }
        }
    }

    if (i < WEB_CLI_IO_MAX) {
        // Found specific entry - take lock to protect against updates from client thread
        struct cli_io_fifo *pMIO = (struct cli_io_fifo *) &cli_io_fifo[i];
        vtss::lock_guard<vtss::Critd> iolock(__FILE__, __LINE__, pMIO->mutex);

        if (pMIO->fd_web < 0) {
            pMIO->fd_web = open(pMIO->tmp_fifo_name, O_RDWR | O_NONBLOCK);
            if (pMIO->fd_web < 0) {
                T_E("Failed opening fifo: %s. %s", pMIO->tmp_fifo_name, strerror(errno));
                return;
            }
            T_D("Opened fifo %s (%d)", pMIO->tmp_fifo_name, pMIO->fd_web);
        }

        cyg_httpd_start_chunked(mimetype);

        char buf[256];
        int bytes_read;
        while ((bytes_read = read(pMIO->fd_web, buf, sizeof(buf)))>0) {
            T_D("Read (%d): %s", bytes_read, buf);
            cyg_httpd_write_chunked(buf, bytes_read);
        }
        if (bytes_read < 0 && errno != EAGAIN) {
            T_E("Error reading from fd_web=%d. %s", pMIO->fd_web, strerror(errno));
        }


        cyg_httpd_end_chunked();

        // Reset buffer
        pMIO->reset_buf();

        if (pMIO->process_done) {
            pMIO->in_use = 0;
            pMIO->process_done = 0;
        }
    }
}

/*****************************************************************************/
// cgi_escape()
// cgi_escape() encodes the string that is contained in @from to make it
// portable. A string is considered portable if it can be transmitted across
// any network to any computer that supports ASCII characters.
// To make a string portable, characters other than the following 62 ASCII
// characters must be encoded:
// 0-9, A-Z, a-z
// All other characters are converted to their two digit (%xx) hexadecimal
// equivalent (referred as the character's "hexadecimal escape sequence").
//
// This function is useful when the user can write any text in a field on
// a web page. If he uses the same character as you use as a delimiter when
// sending data from here to the user's browser, the user would experience
// problems.
// Therefore: In web.c you use cgi_escape() on free text fields, whereas
// in <your_web_page>.htm's processUpdate() function you use the JS function
// called unescape(), which performs the reverse operation. As field separator
// you must then use one of those characters that will get converted, e.g. '#'.
//
// The length of @to must be 3 times the length of @from, since in theory
// all characters in @from may have to be converted (the terminating NULL
// character is not encoded).
//
// The function returns the number of bytes excluding the terminating
// NULL byte written to @to.
/*****************************************************************************/
static int _cgi_escape(const char *from, char *to, size_t maxlen)
{
  char ch, *to_ptr = to;
  const char *from_ptr = from;
  int len = 0;

  /* peter, 2007/11,
     The unreserved characters are alphabet, digit, hyphen(-),
     dot(.), underscore(_) and tilde(~). [RFC 3986].
     Note:
     In the real enviorment(IE6 and FF2.0), Asterisk(*) is unreserved character, tilde(~) is not.
     escape !"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~
     =>
     <%20%21%22%23%24%25%26%27%28%29*%2B%2C-.%2F%3A%3B%3C%3D%3E%3F%40%5B%5C%5D%5E_%60%7B%7C%7D%7E> */
  while(maxlen-- && (ch = *from_ptr) != '\0') {
    if((ch >= '0' && ch <= '9') ||
       (ch >= 'A' && ch <= 'Z') ||
       (ch >= 'a' && ch <= 'z') ||
       (ch == '*') ||
       (ch == '-') ||
       (ch == '.') ||
       (ch == '_')) {
      *to_ptr++ = ch;
      len++;
    } else {
      sprintf(to_ptr, "%%%02X", ch);
      to_ptr += 3;
      len += 3;
    }
    from_ptr++;
  }
  *to_ptr = '\0';
  return len;
}

int cgi_escape(const char *from, char *to)
{
    return _cgi_escape(from, to, strlen(from));
}

int cgi_escape_n(const char *from, char *to, size_t maxlen)
{
    return _cgi_escape(from, to, maxlen);
}

/*****************************************************************************/
// a16toi()
// Converts a hex digit to an int.
/*****************************************************************************/
static int a16toi(char ch) {
  if(ch >= '0' && ch <= '9') {
    return ch - '0';
  }

  return toupper(ch) - 'A' + 10;
}

/*****************************************************************************/
// cgi_unescape()
// This function performs the opposite operation of cgi_escape(). Please
// see cgi_escape() for details.
// The only difference is that the browser converts a space to '+' and a '+'
// to "%2B" before it POSTs the data. Therefore, this function must
// convert back '+' to ' '. All other "%xx" sequences are converted normally.
//
// @from     is the string possibly containing escape sequences.
// @to       is the string where the escape sequences are converted into real chars.
// @from_len is the number of chars to take from @from excluding a possible
//           terminating NULL character (@from need not be NULL-terminated).
// @to_len   is the number of characters that there's room for in @to including
//           the terminating NULL character.
// The function returns FALSE if the @from string is invalid (e.g. there aren't
// two hex chars following a percent sign) or if there's not room in the
// @to string for all unescaped chars. TRUE otherwise.
/*****************************************************************************/
BOOL cgi_unescape(const char *from, char *to, uint from_len, uint to_len)
{
  uint from_i = 0, to_i = 0;
  char ch;

  if (to_len == 0 || from == NULL) {
      return FALSE;
  }

  while(from_i < from_len) {
    if(from[from_i] == '%') {
      // Check if there are chars enough left in @from to complete the conversion
      if(from_i + 2 >= from_len) {
        // There aren't.
        return FALSE;
      }

      // Check if the two next chars are hexadecimal.
      if(!isxdigit(from[from_i + 1])  || !isxdigit(from[from_i + 2])) {
        return FALSE;
      }

      ch = 16 * a16toi(from[from_i + 1])  + a16toi(from[from_i + 2]);

      from_i += 3;
    } else if(from[from_i] == '+') {
      ch = ' '; // Special handling of '+'. The browser converts spaces to '+' and '+' to "%2B" (tested with FF2.0, IE6.0 and IE7.0).
      from_i++;
    } else {
      ch = from[from_i++];
    }

    to[to_i++] = ch;
    if(to_i == to_len) {
      // Not room for trailing '\0'
      return FALSE;
    }
  }

  to[to_i] = '\0';
  return TRUE;
}

/*****************************************************************************/
// cgi_text_str_to_ascii_str()
//
// Example:  from = "A2#b" -> to = "0x410x320x230x62"
// @from     is the textual string.
// @to       is the string in ascii hex code format.
// @from_len is the number of chars to take from @from including a possible
//           terminating NULL character (@from must be NULL-terminated).
// @to_len   is the number of characters that there's enough room for @to
//           including the terminating NULL character.
// The function returns the number of bytes excluding the terminating
// NULL byte written to @to.
/*****************************************************************************/
int cgi_text_str_to_ascii_str(const char *from, char *to, uint from_len, uint to_len)
{
    uint    idx;
    int     length;

    if (!from || !to ||
        (to_len == 0) ||
        (to_len < ((from_len * 4) + 1))) {
        return -1;
    }

    idx = 0;
    length = 0;
    memset(to, 0x0, to_len);
    /* check from for every chars */
    while (idx < from_len) {
        if (!from[idx]) {
            break;
        }

        if ((((int)to_len - length) > 0) &&
            (((int)to_len - length) / 4)) {
            sprintf(to + length, "0x%02X", from[idx]);
        } else {
            length = 0;
            break;
        }

        idx++;
        length += 4;
    }

    to[length] = '\0';
    return length;
}

/*****************************************************************************/
// cgi_ascii_str_to_text_str()
//
// Example:  from = "0x410x320x230x62" -> to = "A2#b"
// @from     is the string in ascii hex code format.
// @to       is the string where the ascii hex codes converted into real chars.
// @from_len is the number of chars to take from @from excluding a possible
//           terminating NULL character (@from need not be NULL-terminated).
// @to_len   is the number of characters that there's room for in @to including
//           the terminating NULL character.
// The function returns FALSE if the @from or @to string is invalid (e.g. null
// pointer, ...) or if there's not room in the @to string for all chars.
// Otherwise, the function returns TRUE.
/*****************************************************************************/
BOOL cgi_ascii_str_to_text_str(const char *from, char *to, uint from_len, uint to_len)
{
    int     asc;
    uint    from_i, to_i;

    if (!from || !to ||
        (to_len == 0) ||
        (from_len % 4)) {
        return FALSE;
    }

    from_i = to_i = 0;
    memset(to, 0x0, to_len);
    /* check from every four chars */
    while (from_i < from_len) {
        /* Check if the first two chars are reserved keyword "0x" */
        if ((from[from_i] != '0') ||
            (from[from_i + 1] != 'x')) {
            to[0] = '\0';
            return FALSE;
        }

        /* Check if the next two chars are hexadecimal */
        if (!isxdigit(from[from_i + 2])  || !isxdigit(from[from_i + 3])) {
            to[0] = '\0';
            return FALSE;
        }

        asc = a16toi(from[from_i + 2]) * 16 + a16toi(from[from_i + 3]);
        if ((asc < 32) || (asc > 127)) {
            to[0] = '\0';
            return FALSE;
        }

        from_i += 4;
        to[to_i++] = asc;
        if (to_i == to_len) {
            /* Not enough room for trailing '\0' */
            to[0] = '\0';
            return FALSE;
        }
    }

    to[to_i] = '\0';
    return TRUE;
}

/*
 *****************************************************************************
 */


/* NB: This implementation has *fixed* search for preceeding \r\n */
static const char *bin_findstr(const char *haystack, const char *haystack_end,
                               const char *needle, size_t nlen)
{
    //diag_printf("%s: start\n", __FUNCTION__);
    while(haystack &&
          haystack < haystack_end &&
          (haystack = str_nextline(haystack, haystack_end))) {
        if(strncmp(haystack, needle, nlen) == 0)
            return haystack;
    }
    return NULL;
}

/*
 * POST formdata extraction
 */

int
cyg_httpd_form_parse_formdata(CYG_HTTPD_STATE* p, form_data_t *formdata, int maxdata)
{
    char *boundz;
    const char *start, *content_end;
    int i, bound_len;

    //diag_printf("%s: start\n", __FUNCTION__);
    if((boundz = vtss_get_formdata_boundary(p, &bound_len)) == NULL)
        return 0;
    //diag_printf("%s: boundary %s (%p)\n", __FUNCTION__, boundz, boundz);

    content_end = p->post_data + p->content_len;
    start = strstr(p->post_data, boundz);
    for(i = 0; i < maxdata && start && start < content_end; ) {
        const char *end, *data, *name;

        /* Point at headers */
        start += bound_len;

        name = strstr(start, "name=\"");
        if (!name) {
            // Malformed.
            break;
        }
        name += 6;
        strncpy(formdata[i].name, name, MAX_FORM_NAME_LEN - 1);

        /* Save form data  */
        formdata[i].content_header = strstr(start, "Content-Disposition:");

        /* Zap off endquote in copied data */
        char *endquote = strchr(formdata[i].name, '"');
        if(endquote)
            *endquote = '\0';

        if((data = strstr(start, VTSS_HTTPD_HEADER_END_MARKER))) {
            formdata[i].value = data + strlen(VTSS_HTTPD_HEADER_END_MARKER);
            /* This may search through *binary* data */
            if((end = bin_findstr(start, content_end, boundz, bound_len))) {
                formdata[i].value_len = (end - formdata[i].value)  - 2 /* \r\n */;
                i++;                /* Got one */
                //diag_printf("%s: have %d parts\n", __FUNCTION__, i);
            } else
                break;
            start = end; /* Advance past this */
        } else
            break; /* Malformed */
    }

    //diag_printf("%s: end %s\n", __FUNCTION__, boundz);
    VTSS_FREE(boundz);               /* VTSS_MALLOC'ed asciiz */

    return i;
}

int
cyg_httpd_form_parse_formdata_filename(CYG_HTTPD_STATE* p, const form_data_t *formdata, char *buffer, size_t maxfn)
{
    if (formdata->content_header) {
        const char *needle = "filename=\"";
        const char *ptr, *ptr_end;
        size_t fnlen;
        if ((ptr = strstr(formdata->content_header, needle)) == NULL) { /* Start */
            return 0;
        }
        ptr += strlen(needle);
        if((ptr_end = strchr(ptr, '"')) == NULL) {  /* End */
            return 0;
        }
        /* Size of data */
        fnlen = ptr_end - ptr;
        fnlen = fnlen < (maxfn - 1) ? fnlen : maxfn - 1;
        /* Copy it */
        memcpy(buffer, ptr, fnlen);
        buffer[fnlen] = '\0';
        return fnlen;
    }
    return 0;
}

static const char *
search_arg(const char *arglist, const char *name)
{
    int idlen = strlen(name);
    const char *start, *ptr;

    start = arglist;
    while(*arglist && (ptr = strstr(arglist, name))) {
        if(ptr[idlen] == '=' &&
           (ptr == start ||     /* ^name= match OR*/
            ptr[-1] == '&'))    /* &name= match */
            return ptr + idlen + 1; /* Match, skip past "name=" */
        /* False match, advance past match */
        arglist = ptr + idlen;
    }
    return NULL;
}

/*
 * POST variable extraction
 */

const char *
cyg_httpd_form_varable_find(CYG_HTTPD_STATE* p, const char *name)
{
    const char *value;

    /* Search POST formdata args */
    if(httpstate.content_type == CYG_HTTPD_CONTENT_TYPE_URLENCODED &&
       p->post_data &&
       (value = search_arg(p->post_data, name)))
        return value;

    /* Search URL args (GET *and* POST) - xyz?bla=1&z=45 */
    return search_arg(p->args, name);
}

const char *
cyg_httpd_form_varable_string(CYG_HTTPD_STATE* p, const char *name, size_t *pLen)
{
    int datalen = 0;
    const char *value = cyg_httpd_form_varable_find(p, name);
    if(value) {                 /* Match */
        while(value[datalen] && value[datalen] != '&') {
            datalen++;
        }
    }
    if(pLen)
        *pLen = datalen;
    return value;

}

cyg_bool
cyg_httpd_form_varable_int(CYG_HTTPD_STATE* p, const char *name, int *pVal)
{
    size_t len;
    const char *value;
    if((value = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
        // If the user has added a '+' before the number in the input edit field,
        // it will get converted to "%2B" before it reaches this file.
        // In that case we skip the initial "%2B" before calling atoi()
        if(value[0] == '%' && value[1] == '2' && toupper(value[2]) == 'B') {
            if(len > 3) {
                *pVal = atoi(&value[3]);
            } else {
                return FALSE;
            }
        } else{
            *pVal = atoi(value);
        }
        return TRUE;
    }
    return FALSE;
}

cyg_bool
cyg_httpd_form_variable_u32(CYG_HTTPD_STATE* p, const char *name, u32 *pVal)
{
    size_t len;
    const char *value;
    if((value = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
        *pVal = (u32) strtoul(value, NULL, 10);
        return TRUE;
    }
    return FALSE;
}

/* The parameter of 'idx' start from 1, it means the first matched.
   When the value is 2, it means the seconds matched and so on. */
const char *
cyg_httpd_form_multi_varable_find(CYG_HTTPD_STATE* p, const char *name, int idx)
{
    const char  *value;
    int         i, offset;

    /* Search POST formdata args */
    if (httpstate.content_type == CYG_HTTPD_CONTENT_TYPE_URLENCODED &&
        p->post_data &&
        (value = search_arg(p->post_data, name))) {
        if (idx == 1) {
            return value;
        } else {
            for (i = 2, offset = strlen(p->post_data) - strlen(value); i <= idx; i++, offset = strlen(p->post_data) - strlen(value)) {
                value = search_arg(p->post_data + offset, name);
                if (i == idx) {
                    return value;
                }
            }
        }
    }

    return NULL;
}

const char *
cyg_httpd_form_multi_varable_string(CYG_HTTPD_STATE* p, const char *name, size_t *pLen, int idx)
{
    int datalen = 0;
    const char *value = cyg_httpd_form_multi_varable_find(p, name, idx);
    if(value) {                 /* Match */
        while(value[datalen] && value[datalen] != '&') {
            datalen++;
        }
    }
    if(pLen)
        *pLen = datalen;
    return value;
}

cyg_bool
cyg_httpd_form_multi_varable_int(CYG_HTTPD_STATE* p, const char *name, int *pVal, int idx)
{
    size_t len;
    const char *value;

    if((value = cyg_httpd_form_multi_varable_string(p, name, &len, idx)) && len > 0) {
        // If the user has added a '+' before the number in the input edit field,
        // it will get converted to "%2B" before it reaches this file.
        // In that case we skip the initial "%2B" before calling atoi()
        if(value[0] == '%' && value[1] == '2' && toupper(value[2]) == 'B') {
            if(len > 3) {
                *pVal = atoi(&value[3]);
            } else {
                return FALSE;
            }
        } else{
            *pVal = atoi(value);
        }
        return TRUE;
    }
    return FALSE;
}

cyg_bool
cyg_httpd_form_varable_uint64(CYG_HTTPD_STATE* p, const char *name, u64 *pVal)
{
    size_t len;
    const char *value;
    char *endptr;
    if((value = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
        // If the user has added a '+' before the number in the input edit field,
        // it will get converted to "%2B" before it reaches this file.
        // In that case we skip the initial "%2B" before calling atoi()
        if(value[0] == '%' && value[1] == '2' && toupper(value[2]) == 'B') {
            if(len > 3) {
                *pVal = strtoull(&value[3],&endptr,10);
            } else {
                return FALSE;
            }
        } else{
            *pVal =strtoull(value,&endptr,10);
        }
        return TRUE;
    }
    return FALSE;
}


// Function that returns the value from a web form containing a integer. It checks
// if the value is within an allowed range given by min_value and max_value (both
// values included) . If the value isn't within the allowed ranged an error message
// is thrown, and the minimum value is returned.
int httpd_form_get_value_int(CYG_HTTPD_STATE* p, const char form_name[255],int min_value,int max_value)
{
    int form_value;
    if(cyg_httpd_form_varable_int(p, form_name, &form_value)) {
      if (form_value < min_value || form_value > max_value) {
          T_E("Invalid value. Form name = %s, form value = %u, min_value = %d, max_value = %d ", form_name, form_value, min_value, max_value);
          form_value =  min_value;
      }
    } else {
        T_E("Unknown form. Form name = %s, form_value = %d", form_name, form_value);
        form_value =  min_value;
    }

    return form_value;
}



cyg_bool
_cyg_httpd_form_varable_long_int(CYG_HTTPD_STATE* p, const char *name, long *pVal)
{
    size_t len;
    const char *value;

    if((value = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
        // If the user has added a '+' before the number in the input edit field,
        // it will get converted to "%2B" before it reaches this file.
        // In that case we skip the initial "%2B" before calling atol()
        if(value[0] == '%' && value[1] == '2' && toupper(value[2]) == 'B') {
            if(len > 3) {
                *pVal = atol(&value[3]);
            } else {
                return FALSE;
            }
        } else {
            *pVal = atol(value);
        }
        return TRUE;
    }
    return FALSE;
}


cyg_bool cyg_httpd_is_hex(char c)
{
   return (((c >= '0') && (c <= '9')) ||
            ((c >='A') && (c <= 'F')) ||
            ((c >= 'a') && (c <= 'f')));
}


cyg_bool
cyg_httpd_form_variable_mac(CYG_HTTPD_STATE* p, const char *name, mesa_mac_t *mac)
{
    size_t      len;
    const char  *value;
    uint        mac_addr[6];
    uint        i;
    char        str[20];

    value = cyg_httpd_form_varable_string(p, name, &len);
    if ( value && len > 0 ) {
        if ( cgi_unescape(value, str, len, 20) == FALSE ) {
            return FALSE;
        }

        if ( (sscanf(str, "%2x-%2x-%2x-%2x-%2x-%2x",  &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) == 6) ||
             (sscanf(str, "%2x.%2x.%2x.%2x.%2x.%2x",  &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) == 6) ||
             (sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",  &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) == 6) ||
             (sscanf(str, "%02x%02x%02x%02x%02x%02x", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) == 6) ) {
            for (i= 0 ; i < sizeof(mac->addr); i++) {
                mac->addr[i] = (uchar) mac_addr[i];
            }
            return TRUE;
        }
    }
    return FALSE;
}

cyg_bool
cyg_httpd_form_variable_int_fmt(CYG_HTTPD_STATE* p, int *pVal, const char *fmt, ...)
{
    char instance_name[ARG_NAME_MAXLEN];
    va_list va;
    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);
    return cyg_httpd_form_varable_int(p, instance_name, pVal);
}

cyg_bool
cyg_httpd_form_variable_long_int_fmt(CYG_HTTPD_STATE* p, ulong *pVal, const char *fmt, ...)
{
    char instance_name[ARG_NAME_MAXLEN];
    va_list va;
    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);
    return cyg_httpd_form_varable_long_int(p, instance_name, pVal);
}

cyg_bool
cyg_httpd_form_variable_u32_fmt(CYG_HTTPD_STATE* p, u32 *pVal, const char *fmt, ...)
{
    char instance_name[ARG_NAME_MAXLEN];
    va_list va;
    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);
    return cyg_httpd_form_variable_u32(p, instance_name, pVal);
}

cyg_bool
cyg_httpd_form_variable_check_fmt(CYG_HTTPD_STATE* p, const char *fmt, ...)
{
    char instance_name[ARG_NAME_MAXLEN];
    va_list va;
    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);
    return cyg_httpd_form_varable_find(p, instance_name) ?
        TRUE : FALSE;           /* "on" if checked - else not found */
}

const char *
cyg_httpd_form_variable_str_fmt(CYG_HTTPD_STATE* p, size_t *pLen, const char *fmt, ...)
{
    char instance_name[ARG_NAME_MAXLEN];
    va_list va;
    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);
    return cyg_httpd_form_varable_string(p, instance_name, pLen);
}

cyg_bool
cyg_httpd_str_to_hex(const char *str_p, ulong *hex_value_p)
{
    char token[20];
    int i=0, j=0;
    ulong k=0, temp=0;

    // Skip possible "0x" or "0X" prefix.
    if (str_p[0] == '0' && (str_p[1] == 'x' || str_p[1] == 'X')) {
        i = 2;
    }

    while (str_p[i] != '\0') {
        token[j++]=str_p[i++];
    }

    token[j]='\0';

    if (strlen(token)>8)
        return FALSE;

    i=0;
    while (token[i]!='\0') {
        if (!((token[i]>= '0' && token[i]<= '9') || (token[i]>= 'A' && token[i]<= 'F') || (token[i]>= 'a' && token[i]<= 'f'))) {
            return FALSE;
        }
        i++;
    }

    temp=0;
    for (i=0; i<j; i++) {
        if (token[i]>='0' && token[i]<='9') {
            k=token[i]-'0';
        }   else if (token[i]>='A' && token[i]<='F') {
            k=token[i]-'A'+10;
        } else if (token[i]>='a' && token[i]<='f') {
            k=token[i]-'a'+10;
        }
        temp=16*temp+ k;
    }

    *hex_value_p=temp;

    return TRUE;
}

cyg_bool
cyg_httpd_form_varable_hex(CYG_HTTPD_STATE* p, const char *name, ulong *pVal)
{
    size_t len;
    const char *value;
    char buf[12];
    unsigned int i;
    if((value = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
        for (i=0; i<len; i++) {
            buf[i] = value[i];
        }
        buf[i] = '\0';

        return (cyg_httpd_str_to_hex(buf, pVal));
    }
    return FALSE;
}

/* Input format is "xx-xx-xx-xx-xx-xx" or "xx.xx.xx.xx.xx.xx" or "xxxxxxxxxxxx" (x is a hexadecimal digit). */
/* Call new function to support "xx:xx:xx:xx:xx:xx" */
cyg_bool
cyg_httpd_form_varable_mac(CYG_HTTPD_STATE* p, const char *name, uchar pVal[6])
{
    return cyg_httpd_form_variable_mac(p, name, (mesa_mac_t *)pVal);
}

cyg_bool cyg_httpd_form_varable_ipv4(CYG_HTTPD_STATE* p, const char *name, mesa_ipv4_t *ipv4)
{
    size_t len;
    const char *buf;
    uint  a, b, c, d, m;

    if ((buf = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
            m = sscanf(buf, "%u.%u.%u.%u", &a, &b, &c, &d);
            if (m >= 4 && a < 256 && b < 256 && c < 256 && d < 256) {
                *ipv4 = ((a << 24) + (b << 16) + (c << 8) + d);
                return TRUE;
            } else if (a == 0 && b == 0 && c == 0 && d == 0) {
                *ipv4 = 0;
                return TRUE;
            }
    }

    return FALSE;
}

cyg_bool cyg_httpd_form_variable_ipv4_fmt(CYG_HTTPD_STATE *p, mesa_ipv4_t *pip, const char *fmt, ...)
{
    char    instance_name[256];
    va_list va;

    va_start(va, fmt);
    (void)vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);

    return cyg_httpd_form_varable_ipv4(p, instance_name, pip);
}

cyg_bool cyg_httpd_form_varable_ipv6(CYG_HTTPD_STATE* p, const char *name, mesa_ipv6_t *ipv6)
{
    const char  *buf;
    size_t      len;
    char        ip_buf[IPV6_ADDR_IBUF_MAX_LEN];

    if (ipv6 && ((buf = cyg_httpd_form_varable_string(p, name, &len)) != NULL)) {
        memset(&ip_buf[0], 0x0, sizeof(ip_buf));
        if (len > 0 && cgi_unescape(buf, ip_buf, len, sizeof(ip_buf))) {
            return (mgmt_txt2ipv6(ip_buf, ipv6) == VTSS_RC_OK);
        }

    }

    return FALSE;
}

cyg_bool cyg_httpd_form_variable_ipv6_fmt(CYG_HTTPD_STATE *p, mesa_ipv6_t *pip, const char *fmt, ...)
{
    char instance_name[256];
    va_list va;

    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);

    return cyg_httpd_form_varable_ipv6(p, instance_name, pip);
}

cyg_bool cyg_httpd_form_varable_ip(CYG_HTTPD_STATE* p, const char *name, mesa_ip_addr_t *pip)
{
    if (vtss_ip_hasipv6() && cyg_httpd_form_varable_ipv6(p, name, &pip->addr.ipv6)) {
        pip->type = MESA_IP_TYPE_IPV6;
        return TRUE;
    } else if (cyg_httpd_form_varable_ipv4(p, name, &pip->addr.ipv4)) {
        pip->type = MESA_IP_TYPE_IPV4;
        return TRUE;
    }

    return FALSE;
}

cyg_bool cyg_httpd_form_variable_ip_fmt(CYG_HTTPD_STATE *p, mesa_ip_addr_t *pip, const char *fmt, ...)
{
    char    instance_name[256];
    va_list va;

    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);

    return cyg_httpd_form_varable_ip(p, instance_name, pip);
}

char *
cyg_httpd_form_varable_strdup(CYG_HTTPD_STATE* p, const char *fmt, ...)
{
    size_t len;
    const char *value;
    char instance_name[ARG_NAME_MAXLEN];
    va_list va;
    va_start(va, fmt);
    (void) vsnprintf(instance_name, sizeof(instance_name), fmt, va);
    va_end(va);
    if((value = cyg_httpd_form_varable_string(p, instance_name, &len)) && len > 0) {
        char *dup = (char *)VTSS_MALLOC(len+1);
        if(dup) {
            (void) cgi_unescape(value, dup, len, len+1);
            dup[len] = '\0';
            return dup;
        }
    }
    return NULL;
}

/* Input format is "xx-xx-xx" or "xxxxxx" (x is a hexadecimal digit). */
cyg_bool
cyg_httpd_form_varable_oui(CYG_HTTPD_STATE* p, const char *name, uchar pVal[3])
{
    size_t len;
    const char *value;
    ulong i, j, k=0, m;
    char buf[4], var_value[20];

    if((value = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
        for (i=0; i<len; i++) {
            var_value[i] = value[i];
        }
        var_value[i] = '\0';

        for (i=0, j=0, k=0; i<len; i++) {
            if (var_value[i] == '-' || j==2) {
                buf[j]='\0';
                j = 0;
                if (!cyg_httpd_str_to_hex(buf, &m)) {
                    return FALSE;
                }
                pVal[k++]=(uchar)m;
                if (var_value[i] != '-' || j==2) {
                    buf[j++]=var_value[i];
                }
            } else {
                buf[j++]=var_value[i];
            }
        }
        buf[j]='\0';
        if (!cyg_httpd_str_to_hex(buf, &m)) {
            return FALSE;
        }
        pVal[k]=(uchar)m;
        return TRUE;
    }
    return FALSE;
}

vtss_isid_t web_retrieve_request_sid(CYG_HTTPD_STATE* p)
{
    return 1;
}

/*
 * ************************** Redirection ***************************
 */

void
redirect(CYG_HTTPD_STATE* p, const char *to)
{
    strncpy(p->url, to, sizeof(p->url)-1);
    cyg_httpd_send_error(CYG_HTTPD_STATUS_MOVED_TEMPORARILY);
}

/* Append a magic keyword "ResponseErrMsg=" in the HTML URL.
 * It is used to notify the Web page alert the error message.
 *
 * Note1: The total length (*to + *errmsg) should not over 497 bytes.
 * The maximum URL length is refer to CYG_HTTPD_MAXURL (512).
 * It is defined in \eCos\packages\net\athttpd\current\include\http.h
 *
 * Note2: Must adding processResponseErrMsg() in the Web page onload event.
 * For example:
 * <body class="content" onload="processResponseErrMsg(); requestUpdate();">
 */
void
redirect_errmsg(CYG_HTTPD_STATE* p, const char *to, const char *errmsg)
{
    char *new_url = NULL;

    if (!p || !to) {
        return;
    }

    if (errmsg) {
        if (((strlen(to) + strlen(errmsg) + strlen("ResponseErrMsg=") + 1 /* room for character '?' */) < (sizeof(p->url) - 1)) &&
        (new_url = (char *)(VTSS_MALLOC((sizeof(p->url) - 1)))) != NULL) {
            u32 i;
            BOOL has_args = FALSE;
            for (i = 0; i < strlen(to); i++) {
                if (to[i] == '?') {
                    has_args = TRUE;
                    break;
                }
            }
            sprintf(new_url, "/%s%sResponseErrMsg=%s", to, has_args ? "&" : "?", errmsg);
            strncpy(p->url, new_url, sizeof(p->url) - 1);
        } else {
            T_W("The length of new URL is out of buffer.");
            strncpy(p->url, to, sizeof(p->url) - 1);
        }
    } else {
        strncpy(p->url, to, sizeof(p->url) - 1);
    }

    if (new_url) {
        VTSS_FREE(new_url);
    }
    cyg_httpd_send_error(CYG_HTTPD_STATUS_MOVED_TEMPORARILY);
}

static void
redirect_get_or_post(CYG_HTTPD_STATE* p, const char *where)
{
    if(p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, where);
    } else {
        int ct;
        (void)cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "Error: %s", where);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        cyg_httpd_end_chunked();
    }
}

static BOOL
redirectInvalid(CYG_HTTPD_STATE* p, vtss_isid_t isid)
{
    if(vtss_switch_stackable()) {
        const char *where = STACK_ERR_URL;
        if(!VTSS_ISID_LEGAL(isid)) {
            T_D("Invalid, Redirect %d to %s!", isid, where);
            redirect_get_or_post(p, where);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL
redirectNonexisting(CYG_HTTPD_STATE* p, vtss_isid_t isid)
{
    if(vtss_switch_stackable()) {
        const char *where = STACK_ERR_URL;
        if(!msg_switch_exists(isid)) {
            T_D("Nonexistent, Redirect %d to %s!", isid, where);
            redirect_get_or_post(p, where);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
redirectUnmanagedOrInvalid(CYG_HTTPD_STATE* p, vtss_isid_t isid)
{
    if(vtss_switch_stackable()) {
        if(redirectInvalid(p, isid) ||
           redirectNonexisting(p, isid))
            return TRUE;
    }
    return FALSE;
}

void
send_custom_error(CYG_HTTPD_STATE *p,
                  const char      *title,
                  const char      *errtxt,
                  size_t           errtxt_len)
{
    int ct;
    (void)cyg_httpd_start_chunked("html");
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                  "<html>"
                  "<head><title>%s</title>"
                  "<link href=\"lib/normal.css\" rel=\"stylesheet\" type=\"text/css\">"
                  "</head>"
                  "<body><h1>%s</h1><pre class=\"alert\">", title, title);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    (void)cyg_httpd_write_chunked(errtxt, errtxt_len);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                  "</pre></body></html>");
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    cyg_httpd_end_chunked();
}


// Function for getting the method.
//
// Return: Returns the method or in case that something is wrong -1
int web_get_method(CYG_HTTPD_STATE *p, int module_id) {
    vtss_isid_t    sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    T_D ("SID =  %d", sid );

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        T_E("Invalid isid - redirecting - ISID =  %d", sid );
        return -1;
    }

    if (!VTSS_ISID_LEGAL(sid)) {
        return -1;
    }

    return p->method;
}

static void web_chip_family_get(bool &lu26, bool &srvl, bool &jr2)
{
    uint32_t chip_family = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);

    lu26 = chip_family == MESA_CHIP_FAMILY_CARACAL;
    srvl = chip_family == MESA_CHIP_FAMILY_OCELOT;
    jr2  = chip_family == MESA_CHIP_FAMILY_JAGUAR2 || chip_family == MESA_CHIP_FAMILY_SERVALT;
}

/****************************************************************************/
/*  Module Config JS lib routine                                            */
/****************************************************************************/
static size_t web_gen_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
#define BUF_SIZE 4096
#define P(...)                                                  \
    if (BUF_SIZE - s > 0) {                                     \
        int res = snprintf(buf + s, BUF_SIZE - s, __VA_ARGS__); \
        if (res > 0) {                                          \
            s += res;                                           \
        }                                                       \
    }

    int s = 0;
    char buf[BUF_SIZE];
    char g_dev_name[ICLI_DEV_NAME_MAX_LEN + 1];
    bool lu26, srvl, jr2;

    if (icli_dev_name_get(g_dev_name) != ICLI_RC_OK) {
        T_E("fail to get device name\n");
        return 0;
    }

    web_chip_family_get(lu26, srvl, jr2);
    P("var configArchJaguar_1   = 0;\n");
    P("var configArchJaguar_2   = %d;\n", jr2);
    P("var configArchJaguar_2_C = %d;\n", jr2);
    P("var configArchLuton26    = %d;\n", lu26);
    P("var configArchServal     = %d;\n", srvl);

#ifdef VTSS_SW_OPTION_BUILD_SMB
    P("var configBuildSMB     = 1;\n");
#else
    P("var configBuildSMB     = 0;\n");
#endif
    P("var configPortMin = %u;\n", iport2uport(VTSS_PORT_NO_START));
    P("var configNormalPortMax = %u;\n", port_count_max());
    P("var configRgmiiWifi = 0;\n");
    P("var configPortType = 0;\n");
    P("var configStackable = 0;\n");
    P("var configSidMin = %d;\n", VTSS_USID_START);
    P("var configSidMax = %d;\n", VTSS_USID_END - 1);
    P("var configDeviceName = \"%s\";\n", g_dev_name);
    P("var configSwitchName = \"%s\";\n", VTSS_PRODUCT_NAME);
    P("var configSwitchDescription = \"%s\";\n\n", VTSS_PRODUCT_DESC);
    P("var configSoftwareId = %d;\n\n", VTSS_SW_ID);
    P("var configHostNameLengthMax = %d;\n\n", VTSS_APPL_SYSUTIL_INPUT_HOSTNAME_LEN);
    P("var configDomainNameLengthMax = %d;\n\n", VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN);
    P("function configPortName(portno, long) {\n");
    P(" var portname = String(portno);\n");
    P(" if(long) portname = \"Port \" + portname;\n");
    P(" return portname;\n");
    P("}\n\n");

#undef BUF_SIZE
#undef P

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

/****************************************************************************/
/*  Config JS lib table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(web_gen_lib_config_js);

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
#define WEB_CSS_FILTER_MAX 32
static const char *web_css_filter_list[WEB_CSS_FILTER_MAX];
static int web_css_filter_cnt;

/* Register base name of feature to be included in CSS filter */
void web_css_filter_add(const char *const name)
{
    if ((web_css_filter_cnt + 1) < WEB_CSS_FILTER_MAX) {
        web_css_filter_list[web_css_filter_cnt] = name;
        web_css_filter_cnt++;
    }
}

static size_t web_gen_filter_add(char **base_ptr, char **cur_ptr, size_t *length, const char *name)
{
    static char buf[1024];

    (void)snprintf(buf, sizeof(buf), ".%s { display: none; }\n", name);
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

static size_t web_gen_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    int      i;
    size_t   file_size = 0;
    char     buf[128];
    bool     lu26, srvl, jr2;

    web_chip_family_get(lu26, srvl, jr2);


    file_size += web_gen_filter_add(base_ptr, cur_ptr, length, "hasWarmStart");

    file_size += web_gen_filter_add(base_ptr, cur_ptr, length, "SPOM_only");

    if (srvl || jr2) {
        file_size += web_gen_filter_add(base_ptr, cur_ptr, length, "has_arch_luton26");
    }

    if (lu26 || jr2) {
        file_size += web_gen_filter_add(base_ptr, cur_ptr, length, "has_arch_serval");
    }

    if (lu26 || srvl) {
        file_size += web_gen_filter_add(base_ptr, cur_ptr, length, "has_arch_jaguar_2");
    }

    /* CSS Generation: Generates for all of the modules those are not
     * included in the build.  This will be in the form of ".has_xxx {
     * display: none; }" here xxx is the module name.  All of them may
     * not be used at this point of time, this is mainly to achieve
     * uniform notation.  NOTE :If a new module is added to the
     * webstax, this will automatically generates you the class string
     * and the same name should be used in the respective html files
    */
    {
        char disabled_modules[] = XSTR(DISABLED_MODULES); // This will allocate a modifiable copy on the stack, which is needed by strtok_r()
        char *saveptr; // Local strtok_r() context
        char *ch = strtok_r(disabled_modules, " ", &saveptr);
        while (ch) {
            sprintf(buf, "has_%s", ch);
            file_size += web_gen_filter_add(base_ptr, cur_ptr, length, buf);
            ch = strtok_r(NULL, " ", &saveptr);
        }
    }

    for (i = 0; i < web_css_filter_cnt; i++) {
        sprintf(buf, "has_%s", web_css_filter_list[i]);
        file_size += web_gen_filter_add(base_ptr, cur_ptr, length, buf);
    }

    return file_size;
}
/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(web_gen_lib_filter_css);

/*
 * ************************* Semi-static Handlers Start *************************
 */
static void chunk_print_str(CYG_HTTPD_STATE* p, int &s, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    s += vsnprintf(p->outbuffer + s, sizeof(p->outbuffer) - s, fmt, ap);
    va_end(ap);

    if (s > (sizeof(p->outbuffer) / 2)) {
        cyg_httpd_write_chunked(p->outbuffer, s);
        s = 0;
    }
}

static int copy_to_coma(const char *from, char *to, int max)
{
    int i = 0;
    while (*from && *from != ',' && i < max - 1) {
        *to = *from;
        ++from;
        ++to;
        ++i;
    }
    *to = '\0';

    if (*from == ',') {
        i++;
    }

    return i;
}

void WEB_navbar_menu_item(CYG_HTTPD_STATE* p, int &size, const char *s, int &prev_level)
{
#define BUF_SIZE 128
#define P(...) chunk_print_str(p, size, __VA_ARGS__)
    int level = 0;
    char menu_buf[BUF_SIZE];
    char url_buf[BUF_SIZE];
    char name_buf[BUF_SIZE];
    int url_length = 0;
    int name_length = 0;

    // count leading spaces
    while (*s == ' ') {
        level++;
        s++;
    }

    s += copy_to_coma(s, menu_buf, BUF_SIZE);
    s += copy_to_coma(s, url_buf, BUF_SIZE);
    s += copy_to_coma(s, name_buf, BUF_SIZE);
    url_length = strnlen(url_buf, BUF_SIZE);
    name_length = strnlen(name_buf, BUF_SIZE);

    // Close existing scope ////////////////////////////////////////////////////
    int closed_scope = 0;
    while (level < prev_level) {
        prev_level --;
        closed_scope ++;
        if (closed_scope == 1) {
            P("</li>");
        }
        P("</ul></li>");
    }

    if (closed_scope == 0 && level == prev_level) {
        P("</li>");
    }

    // Open new scope //////////////////////////////////////////////////////////
    if (level > prev_level) {
        if (level == 0) {
            P("<ul class=\"level0\">");
        } else {
            P("<ul class=\"submenu level%d\">", level);
        }

        prev_level ++;
    }

    // Create new item /////////////////////////////////////////////////////////
    if (url_length) {
        P("<li class=\"link\">"
          "<a href=\"%s\" id=\"%s\" target=\"main\" name=\"%s\">"
          "%s"
          "</a>",
          url_buf, url_buf, url_buf, menu_buf);
    } else {
        if (name_length) {
            P("<li><a class=\"actuator\" href=\"#\" target=\"_self\" id=\"%s\">"
              "%s</a><br />", name_buf, menu_buf);
        } else {
            P("<li><a class=\"actuator\" href=\"#\" target=\"_self\">"
              "%s</a><br />", menu_buf);
        }
    }
#undef BUF_SIZE
#undef P
}

static void menu_item_end(CYG_HTTPD_STATE* p, int &size, int &prev_level) {
#define P(...) chunk_print_str(p, size, __VA_ARGS__)
    int closed_scope = 0;
    while (prev_level > -1) {
        prev_level --;
        closed_scope ++;
        if (closed_scope == 1) P("</li>");
        P("</ul>");
        if (prev_level >= 0) P("</li>");
    }
#undef P
}

i32 handler_navbar(CYG_HTTPD_STATE* p)
{
#define P(...) chunk_print_str(p, size, __VA_ARGS__)
    int level, size = 0;

    T_D("handler_navbar");
    cyg_httpd_start_chunked("html");

    P("<!DOCTYPE html>");
    P("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en-US\" xml:lang=\"en-US\">");
    P("<head>");
    P("<meta name=\"generator\" content=\"HTML Tidy for Linux (vers 25 March 2009), see www.w3.org\" />");
    P("<title>Index</title>");
    P("<link rel=\"stylesheet\" type=\"text/css\" href=\"lib/menu.css\" />");
    P("<script src=\"lib/mootools-core.js\" type=\"text/javascript\"></script>");
    P("<script src=\"lib/mootools-more.js\" type=\"text/javascript\"></script>");
    P("<script src=\"lib/config.js\" type=\"text/javascript\"></script>");
    P("<script src=\"lib/menu.js\" type=\"text/javascript\"></script>");
    P("<script src=\"lib/navbarupdate.js\" type=\"text/javascript\"></script>");
    P("<script src=\"lib/ajax.js\" type=\"text/javascript\"></script>");
    P("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=us-ascii\" />");
    P("</head>");
    P("<body>");
    P("<form method=\"post\" action=\"#\" enctype=\"multipart/form-data\">");
    P("<table id=\"menu\" summary=\"Main navigation menu\">");
    P("<tr class=\"selector\" id=\"selrow\" style=\"display:none\">");
    P("<td><select id=\"stackselect\" onchange=\"stackSelect(this);\">");
    P("<option value=\"-1\"> Switch 1 </option>");
    P("<option value=\"99\"> Switch 99:M </option>");
    P("</select></td>");
    P("<td width=\"100%%\"><img align=\"bottom\" alt=\"Refresh Stack List\" onclick=\"DoStackRefresh();\" src=\"images/refresh.gif\" title=\"Refresh Stack List\" /></td>");
    P("</tr>");
    P("<tr>");
    P("<td colspan=\"2\">");

    level = WEB_navbar(p, size);

    menu_item_end(p, size, level);
    P("</td>");
    P("</tr>");
    P("</table>");
    P("</form>");
    P("</body>");
    P("</html>");

    cyg_httpd_write_chunked(p->outbuffer, size); // flush buffer!
    size = 0;
    cyg_httpd_end_chunked();

#undef P
    return -1; // Do not further search the file system.
}
CYG_HTTPD_HANDLER_TABLE_ENTRY_FS_LIKE_MATCH(get_cb_navbar, "/navbar.htm", handler_navbar);


#define WEB_PAGE_DIS_MAX 256
static const char *web_page_dis_list[WEB_PAGE_DIS_MAX];
static int web_page_dis_cnt;

/* Register base name of web page to be disabled from navigation bar */
void web_page_disable(const char *const name)
{
    if ((web_page_dis_cnt + 1) < WEB_PAGE_DIS_MAX) {
        web_page_dis_list[web_page_dis_cnt] = name;
        web_page_dis_cnt++;
    }
}

i32 handler_banner_text_get(CYG_HTTPD_STATE* p)
{
    cyg_httpd_start_chunked("html");
    char *banner = (char *)icli_malloc(ICLI_BANNER_MAX_LEN + 1);
    if ( banner == NULL ) {
        T_E("memory insufficient for banner\n");
        return -1;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        // Not relevant now
    } else {
        char *banner = (char *)icli_malloc(ICLI_BANNER_MAX_LEN + 1);
        if (icli_banner_exec_get(banner) == VTSS_RC_OK) {
            strcpy(p->outbuffer, banner);
            cyg_httpd_write_chunked(p->outbuffer, strlen(banner));
        }
    }
    cyg_httpd_end_chunked();
    icli_free(banner);
    return -1;
}

i32 handler_navbar_update(CYG_HTTPD_STATE* p)
{
    int ct, i;

    cyg_httpd_start_chunked("javascript");
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "function NavBarUpdate()\n{\n");
    cyg_httpd_write_chunked(p->outbuffer, ct);

    /* Disable web pages */
    for (i = 0; i < web_page_dis_cnt; i++) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      " if ($('%s')) { $('%s').getParent('li').setStyle('display', 'none'); }\n",
                      web_page_dis_list[i], web_page_dis_list[i]);
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "}\n");
    cyg_httpd_write_chunked(p->outbuffer, ct);

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_navbar_update, "/lib/navbarupdate.js", handler_navbar_update);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_banner_text, "/stat/banner_text", handler_banner_text_get);

extern "C" int web_icli_cmd_register();

mesa_rc web_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        web_icli_cmd_register();
    }

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    return web_init_os(data);
}

