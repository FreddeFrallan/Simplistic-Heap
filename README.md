# Simplistic Heap

A simple implementation of a heap memory-segment written in C.
This allows you to use functions such as <b>Malloc</b> and <b>Free.</b>
Can become useful if you find yourself working in an evironment that only supports stack and static data, but you wish to remain a sain programmer and not declare everything pre-hand.

## Getting Started

To make use of this heap simply copy the files into your project and include the "MemoryManagment.h" file.
You then have some options that you can tinker with.

1. Heap Size:
    To determine how much global memory you wish to allocate for your heap you simple change the constant <b>HEAP_SIZE</b> found in <b>MemoryManager.c</b>.

2. Error functions
    There exists a separate file named <b>MemoryErrorManager.c</b>, in this file you can specify what will happen when the heap manager throws an error.


Once your happy with your heap settings:

1. Call the <b>initHeapMemory</b> function before using any of the functionalities.

2. Have fun with your staticly implemented Malloc and Free.


For additional instructions see the <b>Demo.c</b> file.

### Prerequisites

To be able to make use of this code you might have to have to compile your project with version <b>C99</b> or higher.
This is done by adding the <b>-std=c99</b> flag when you compile.


## Verification and testing

  This project was tested during and after development using the Albot.Online heap simulator.
  A short clip demonstrating a short test simulation can be found here: https://youtu.be/1ojwOF1NZok

## Versioning

  1.1
  
## Authors

  Fredrik Carlsson

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to Johan Montelius for teaching me about heaps and memory managment in the first place.
