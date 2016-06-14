NECESSARILY:
1) Unit-tests for config.
2) Use more smart way for processing protocol statistics and allocating memory for counters (now it is array with fixed size).
3) Check with valgrind(memory leaks).

IN FUTURE:
1) Smart buffer for each type of packets.
This allows to do more sophisticated analysis.
2) Support other protocols.
