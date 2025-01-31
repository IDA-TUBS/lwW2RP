#!/bin/bash


# only works when properly enabling all lttng related stuff! 

# First destroy all active lttng sessions
lttng destroy

rm -r ~/lightweightW2RP/test/logslttng_tracing/ust

lttng-sessiond --daemonize
#lttng list --userspace
lttng create fastrtps_tracing_session --output ~/lightweightW2RP/test/logs/lttng_tracing
lttng enable-event --userspace w2rp_trace:tracepoint_pub_int
lttng enable-event --userspace w2rp_trace:tracepoint_writer_int
lttng enable-event --userspace w2rp_trace:tracepoint_writer
lttng enable-event --userspace w2rp_trace:tracepoint_sub_int
lttng enable-event --userspace w2rp_trace:tracepoint_reader_int
lttng enable-event --userspace w2rp_trace:tracepoint_reader
lttng start

read  -p "Press Enter to end tracing." mainmenuinput

lttng destroy

TIMESTAMP=$(date +%s)

babeltrace2 ~/lightweightW2RP/test/logs/lttng_tracing/ >~/lightweightW2RP/test/logs/lttng_tracing/trace_$1_$2.log