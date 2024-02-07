List of known corrections needed:
1 - USB_ST_N line needs 1.5k pullup to 3.3V
    = new resistor across line via and C202
2 - R205 as a pulldown not pullup
    = R205 3.3V leg soldered to C203 GND leg
3 - +3V3 not connected to any 3.3V voltage source
    = jumper from C24 to FB102
4 - Knock signal DC offset
    = added THT diodes (clipping) and 100k resistors
5 - voltage ripple on +5V line
    = 100mH and 470uF added
6 - AC signals mixed up
    = jumper wires and cut PCB line
7 - Low frequency supply voltage ripple
    = C53 and C58 removal
8 - High frequency supply voltage ripple
    = low ESR 100uF capacitor on 3V3 line
9 - Missing capacitors of U1
    = 1uF parallel to C9, 1uF on R29 (3V3_nECU line) and C10 (GND line), 4.7uF parallel to C44


Possible upgrades for future versions:
 - improve SNR of analog speed signal
 - UART from onboard debbuger
 - MaxxECU direct USB soldering