#pragma once

#ifndef IoCompleteRequest
#error dont include this on its own, it needs wdm.h already included
#endif

#define TO_STR(s) _TO_STR(s)
#define _TO_STR(s) #s

#define DBGI_FUN(part, fmt, ...) \
    DBGI(part, __FILE__ "@" TO_STR(__LINE__) "" fmt, ## __VA_ARGS__)

#define LOG_IRQL_NE(expected_irql) \
    do { \
        KIRQL test_irql = KeGetCurrentIrql(); \
        if (expected_irql != test_irql) { \
            DBGI_FUN(DBG_GENERAL, "IRQL: %u (expected %u)", KeGetCurrentIrql(), expected_irql); \
        } \
    } while (0)

#undef IoCompleteRequest
#define IoCompleteRequest(a,b) \
    DBGI_FUN(DBG_GENERAL, "IoCompleteRequest %p boost? %u", a, b); \
    LOG_IRQL_NE(PASSIVE_LEVEL); \
    IofCompleteRequest(a, b)

#undef IoCallDriver
#define IoCallDriver(a,b)   \
    DBGI_FUN(DBG_GENERAL, "IoCallDriver devobj:%p irp:%p", a, b); \
    LOG_IRQL_NE(PASSIVE_LEVEL); \
    IofCallDriver(a,b)
