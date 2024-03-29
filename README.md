# FuzzBtor2

FuzzBtor2 is a fuzzer to generate random word-level model checking problems in Btor2 format.

#### Research Paper
- [[TACAS'23](https://link.springer.com/chapter/10.1007/978-3-031-30820-8_5)] FuzzBtor2: A Random Generator of Word-Level Model Checking Problems in Btor2 Format

Our recommended running and compilation environment is Linux + gcc/g++.

## Compile

Assuming ``${FUZZER_DIR}/FuzzBtor2`` is the path where FuzzBtor2 is located, one can compile FuzzBtor2 as follows:

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
./fuzzbtor --seed 10 --max-depth 3 --constraints 0  --max-inputs 3 --possible-sizes 4..8
```
For more detailed usage, please try
```bash
./fuzzbtor -h
```
or refer to the corresponding paper.
