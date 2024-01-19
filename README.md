# nECU_EE110
Plug and play solution for MaxxECU Mini on EE11x platform. Adds EGT, TC and knock control over CAN.

About
-
Designed for 1996-1999 4E-FE Toyota Corolla EE11x. Fits into stock ECU enclosure, uses the same stock plugs.

It was created to allow remapping and turbo support. Tailored to my needs but basic concepts and solutions may be used to simplify design. 
Knock detection part was my bachelor thesis "Methods for machine analysis of data from the knock combustion sensor of a gasoline engine".

Key Features
-
USB connection for both MaxxECU and onboard STM32 programmer/debbuger.
CAN bus connection allows for external oxygen sensor controller.

Changes compared to stock:
 - USB connector (mandatory),
 - Pneumatic passthrough (mandatory),
 - 24-pin connector #1 [User interface, CAN, Additional IO] (optional),
 - 24-pin connector #2 [EGT sensors, ABS sensors for TC] (optional).

Functionalities in progress
 - 
PCB tested and operational, code is during development or tests:
 - EGT termocouple (testing),
 - Speed sensing (testing and bugfixing),
 - User interface (bugfixing),
 - Knock sensing (optimization and testing),
 - Ignition missfire support (development),
 - Stock oxygen sensor heater (development),
 - Immobilizer communication (scheduled),
 - Additional analog sensors (scheduled),
 - Onboard UART connection (scheduled).
