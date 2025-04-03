/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <stdio.h>
#include <vtss/basics/print_fmt.hxx>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/utility.hxx>

template <typename... ARGS>
void print_fmt(const char *format, ARGS && ... args) {
    vtss::StringStream stream;
    vtss::print_fmt(stream, format, vtss::forward<ARGS>(args)...);
    printf("%s", stream.begin());
}

void foo(int) {}

enum enum_data { A = 2, B, C};

int main(int argc, char *argv[]) {
    print_fmt("Printfmt:%04d After\n", 3);
    printf("Printf:%04d After\n\n", 3);

    print_fmt("Printfmt:%-4d After\n", 3);
    printf("Printf:%-4d After\n\n", 3);

    print_fmt("Printfmt:%04d After\n", -3);
    printf("Printf:%04d After\n\n", -3);

    print_fmt("Printfmt:%-4d After\n", -3);
    printf("Printf:%-4d After\n\n", -3);

    print_fmt("Printfmt:%4d After\n", 3);
    printf("Printf:%4d After\n\n", 3);

    print_fmt("Printfmt:%04d After\n", -3);
    printf("Printf:%04d After\n\n", -3);

    print_fmt("Printfmt:%+4d After\n", 3);
    printf("Printf:%+4d After\n\n", 3);

    print_fmt("Printfmt:%0+4d After\n", 3);
    printf("Printf:%0+4d After\n\n", 3);

    print_fmt("Printfmt:%01d After\n", 42);
    printf("Printf:%1d After\n\n", 42);

    print_fmt("Printfmt:%0 1d After\n", 42);
    printf("Printf:%0 1d After\n\n", 42);

    print_fmt("Printfmt:%-+4d After\n", 3);
    printf("Printf:%-+4d After\n\n", 3);

    print_fmt("Printfmt:%- 4d After\n", 3);
    printf("Printf:%- 4d After\n\n", 3);

    print_fmt("Printfmt:%o After\n", -8);
    printf("Printf:%o After\n\n", -8);

    print_fmt("Printfmt:%4s After\n", "a");
    printf("Printf:%4s After\n\n", "a");

    char c = '2';
    print_fmt("Printfmt:%c After\n", c);
    printf("Printf:%c After\n\n", c);

    unsigned int tmp = 42;
    print_fmt("Printfmt:%p After\n", &tmp);
    printf("Printf:%p After\n\n", &tmp);

    int tmp2 = 32;
    print_fmt("Printfmt: %p After\n", &tmp2);
    printf("Printf:%p After\n\n", &tmp2);

    unsigned long l = -12;
    printf("sizeof = %lu\n", sizeof(unsigned long));
    print_fmt("Printfmt: %lu After\n", l);
    printf("Printf: %lu After\n\n", l);

    char tmp3 = -1;
    print_fmt("Printfmt: %x\n", tmp3);
    printf("Print: %x\n", tmp3);

    const char *data = "1234";
    char arr[4] = {'1', '2', '3', '\0'};
    print_fmt("Printfmt: %s %s After\n", data, arr);
    printf("Print: %s %s After\n", data, arr);

    char tmp5 = 0xFF;
    print_fmt("Printfmt %hhx %x\n", tmp5, tmp5);
    printf("Printf %hhx %x\n", tmp5, tmp5);

    enum_data val = A;
    print_fmt("Printfmt %x\n", val);
    printf("Printf %x\n", val);

    long unsigned int d = 10;
    print_fmt("Printfmt %ld\n", d);
    printf("Printf %ld\n", d);

    size_t s = 2;
    print_fmt("Printfmt %x\n", s);
    printf("Printf %ld\n", s);

    using foo_ptr = void (*)(int);
    foo_ptr ptr = &foo;

    print_fmt("Printfmt %p\n", ptr);
    printf("Printf %p\n", ptr);

    print_fmt("Printfmt %p\n", (void*)foo);
    printf("Printf %p\n", foo);

    char buf[strlen("hh:mm:ss") + 2];
    //char *tmp = &buf[0];
    memset(buf, '\0', 10);
    buf[0] = 'A';
    buf[1] = 'S';
    buf[2] = 'Z';
    print_fmt("Printfmt %s\n", &buf[0]);
    printf("Printf %s\n", &buf[0]);

    print_fmt("Printfmt %p\n", &buf[0]);
    printf("Printf %p\n", &buf[0]);

    int data_tmp = 10;
    int *ptr_tmp = &data_tmp;
    print_fmt("Printfmt %d\n", ptr_tmp);
}
