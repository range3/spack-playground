# thallium_failure_detector
Server/Client for demonstration of failure detection using the Phi accrual failure detector.
```
N. Hayashibara and M. Takizawa, "Performance Evaluation of the φ Accrual Failure Detector," 26th IEEE International Conference on Distributed Computing Systems Workshops (ICDCSW'06), 2006, pp. 46-46, doi: 10.1109/ICDCSW.2006.83.

N. Hayashibara, X. D´efago, R. Yared, and T. Katayama. The ϕ accrual failure detector. In Proc. 23nd IEEE Int’l Symp. on Reliable Distributed Systems (SRDS’04), pages 66–78, Florian´opolis, Brazil, October 2004.
```

## Usage
```bash
# server (observer, receives heartbeats)
./bin/thallium_failure_detector -S -a ofi+sockets

# client (imitates monitord process, sends heartbeats)
./bin/thallium_failure_detector -a ofi+sockets://172.17.0.2:36231 --times 100000000
```
