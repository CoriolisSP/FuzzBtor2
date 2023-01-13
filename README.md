# FuzzBtor2

FuzzBtor2 is a fuzzer to generate random word-level model checking problems in Btor2 format.

Our recommended running and compilation environment is Linux + gcc/g++.

## Compile

Assuming ``${FUZZER_DIR}/FuzzBtor2`` is the path where FuzzBtoor2 is located, one can compile FuzzBtoor2 as follows:

```bash
cd ${FUZZER_DIR}/FuzzBtor2
make
```
The resulting executable file is ``${FUZZER_DIR}/FuzzBtor2``.

## Usage

The standard command to execute FuzzBtor2 in a Linux system is as follows.
```bash
./fuzzbtor [options]
```
Example:
```bash
./btor2fuzz --seed 10 --max-depth 3 --constraints 0  --max-inputs 3 --possible-sizes 4..8
```
For more detailed usage, please try
```bash
./fuzzbtor -h
```
or refer to the corresponding paper.
