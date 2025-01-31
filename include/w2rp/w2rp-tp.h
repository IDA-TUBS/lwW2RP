#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER w2rp_trace
#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./w2rp/w2rp-tp.h" 

#if !defined(_W2RP_TP_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _W2RP_TP_H

#include <lttng/tracepoint.h>

// add lttng-ust to w2rp's target_link_libraries + add # ${PROJECT_SOURCE_DIR}/src/w2rp/w2rp-tp.cpp to lib set

TRACEPOINT_EVENT(
	w2rp_trace, 
	tracepoint_pub_int,
	TP_ARGS(
		const char*, string_arg,
		int, integer_arg
	),
	TP_FIELDS(
		ctf_string(string_field, string_arg)
		ctf_integer(int, integer_field, integer_arg)
	)
)

TRACEPOINT_EVENT(
	w2rp_trace, 
	tracepoint_writer_int,
	TP_ARGS(
		const char*, string_arg,
		int, integer_arg
	),
	TP_FIELDS(
		ctf_string(string_field, string_arg)
		ctf_integer(int, integer_field, integer_arg)
	)
)


TRACEPOINT_EVENT(
	w2rp_trace, 
	tracepoint_writer,
	TP_ARGS(
		const char*, string_arg
	),
	TP_FIELDS(
		ctf_string(string_field, string_arg)
	)
)



TRACEPOINT_EVENT(
	w2rp_trace, 
	tracepoint_sub_int,
	TP_ARGS(
		const char*, string_arg,
		int, integer_arg
	),
	TP_FIELDS(
		ctf_string(string_field, string_arg)
		ctf_integer(int, integer_field, integer_arg)
	)
)

TRACEPOINT_EVENT(
	w2rp_trace, 
	tracepoint_reader_int,
	TP_ARGS(
		const char*, string_arg,
		int, integer_arg
	),
	TP_FIELDS(
		ctf_string(string_field, string_arg)
		ctf_integer(int, integer_field, integer_arg)
	)
)

TRACEPOINT_EVENT(
	w2rp_trace, 
	tracepoint_reader,
	TP_ARGS(
		const char*, string_arg
	),
	TP_FIELDS(
		ctf_string(string_field, string_arg)
	)
)

#endif /* _W2RP_TP_H */
#include <lttng/tracepoint-event.h>