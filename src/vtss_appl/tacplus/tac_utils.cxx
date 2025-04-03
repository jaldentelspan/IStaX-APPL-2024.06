#include "libtacplus.h"

void tac_error(const char *format, ...)
{
    va_list ap;

    /*lint -e{530} ... 'ap' is initialized by va_start() */
    va_start(ap, format);
    (void)vfprintf(stderr, format, ap);
    (void)fflush(stderr);
    va_end(ap);
}

void tac_free_avpairs(char **avp)
{
    int i = 0;
    while (avp[i] != NULL) {
        tac_free(avp[i++]);
    }
}

const char *tac_print_authen_status(int status)
{
    switch (status) {
    case TAC_PLUS_AUTHEN_STATUS_PASS:
        return "TAC_PLUS_AUTHEN_STATUS_PASS";
    case TAC_PLUS_AUTHEN_STATUS_FAIL:
        return "TAC_PLUS_AUTHEN_STATUS_FAIL";
    case TAC_PLUS_AUTHEN_STATUS_GETDATA:
        return "TAC_PLUS_AUTHEN_STATUS_GETDATA";
    case TAC_PLUS_AUTHEN_STATUS_GETUSER:
        return "TAC_PLUS_AUTHEN_STATUS_GETUSER";
    case TAC_PLUS_AUTHEN_STATUS_GETPASS:
        return "TAC_PLUS_AUTHEN_STATUS_GETPASS";
    case TAC_PLUS_AUTHEN_STATUS_RESTART:
        return "TAC_PLUS_AUTHEN_STATUS_RESTART";
    case TAC_PLUS_AUTHEN_STATUS_ERROR:
        return "TAC_PLUS_AUTHEN_STATUS_ERROR";
    case TAC_PLUS_AUTHEN_STATUS_FOLLOW:
        return "TAC_PLUS_AUTHEN_STATUS_FOLLOW";
    default:
        return "Unknown authenticaton status";
    }
}

const char *tac_print_author_status(int status)
{
    switch (status) {
    case TAC_PLUS_AUTHOR_STATUS_PASS_ADD:
        return "TAC_PLUS_AUTHOR_STATUS_PASS_ADD";
    case TAC_PLUS_AUTHOR_STATUS_PASS_REPL:
        return "TAC_PLUS_AUTHOR_STATUS_PASS_REPL";
    case TAC_PLUS_AUTHOR_STATUS_FAIL:
        return "TAC_PLUS_AUTHOR_STATUS_FAIL";
    case TAC_PLUS_AUTHOR_STATUS_ERROR:
        return "TAC_PLUS_AUTHOR_STATUS_ERROR";
    case TAC_PLUS_AUTHOR_STATUS_FOLLOW:
        return "TAC_PLUS_AUTHOR_STATUS_FOLLOW";
    default:
        return "Unknown authorization status";
    }
}

const char *tac_print_account_status(int status)
{
    switch (status) {
    case TAC_PLUS_ACCT_STATUS_SUCCESS:
        return "TAC_PLUS_ACCT_STATUS_SUCCESS";
    case TAC_PLUS_ACCT_STATUS_ERROR:
        return "TAC_PLUS_ACCT_STATUS_ERROR";
    case TAC_PLUS_ACCT_STATUS_FOLLOW:
        return "TAC_PLUS_ACCT_STATUS_FOLLOW";
    default:
        return "Unknown accounting status";
    }
}
