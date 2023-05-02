EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector_Generic:Conn_02x20_Odd_Even J1
U 1 1 6418B861
P 3150 2600
F 0 "J1" H 3200 3717 50  0000 C CNN
F 1 "GPIO_0" H 3200 3626 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x20_P2.54mm_Vertical" H 3150 2600 50  0001 C CNN
F 3 "~" H 3150 2600 50  0001 C CNN
	1    3150 2600
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_02x20_Odd_Even J3
U 1 1 6419255B
P 5550 2600
F 0 "J3" H 5600 3717 50  0000 C CNN
F 1 "GPIO_1" H 5600 3626 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x20_P2.54mm_Vertical" H 5550 2600 50  0001 C CNN
F 3 "~" H 5550 2600 50  0001 C CNN
	1    5550 2600
	1    0    0    -1  
$EndComp
Text GLabel 2950 2200 0    50   Input ~ 0
5v
Text GLabel 2950 3100 0    50   Input ~ 0
3v3
Text GLabel 3450 2200 2    50   Input ~ 0
GND
Text GLabel 3450 3100 2    50   Input ~ 0
GND
Text GLabel 5350 2200 0    50   Input ~ 0
5v
Text GLabel 5350 3100 0    50   Input ~ 0
3v3
Text GLabel 5850 2200 2    50   Input ~ 0
GND
Text GLabel 5850 3100 2    50   Input ~ 0
GND
Text GLabel 2950 1700 0    50   Input ~ 0
FPGA_CAM_TX
Text GLabel 5350 1700 0    50   Input ~ 0
FPGA_CAM_RX
Text GLabel 2950 1800 0    50   Input ~ 0
FPGA_GPS_TX
Text GLabel 5350 1800 0    50   Input ~ 0
FPGA_GPS_RX
Text GLabel 2950 1900 0    50   Input ~ 0
FPGA_GPS_PWR
Text GLabel 2950 2000 0    50   Input ~ 0
FPGA_CELL_TX
Text GLabel 5350 2000 0    50   Input ~ 0
FPGA_CELL_RX
Text GLabel 7000 1500 0    50   Input ~ 0
5v
Text GLabel 7000 1600 0    50   Input ~ 0
GND
Text GLabel 7000 1950 0    50   Input ~ 0
FPGA_CAM_TX
Text GLabel 7000 2050 0    50   Input ~ 0
FPGA_CAM_RX
$Comp
L Connector_Generic:Conn_02x20_Odd_Even J2
U 1 1 6419DB4B
P 3200 5500
F 0 "J2" H 3250 6617 50  0000 C CNN
F 1 "GPS_module_backplane" H 3250 6526 50  0000 C CNN
F 2 "backplane_conn:3020-40-0100-00" H 3200 5500 50  0001 C CNN
F 3 "~" H 3200 5500 50  0001 C CNN
	1    3200 5500
	1    0    0    -1  
$EndComp
Text GLabel 3500 4700 2    50   Input ~ 0
5v
Text GLabel 3500 4800 2    50   Input ~ 0
GND
Text GLabel 3500 4900 2    50   Input ~ 0
FPGA_GPS_TX
Text GLabel 3500 5000 2    50   Input ~ 0
FPGA_GPS_RX
Text GLabel 3500 5200 2    50   Input ~ 0
GND
Text GLabel 3500 5500 2    50   Input ~ 0
GND
Text GLabel 3500 6000 2    50   Input ~ 0
GND
Text GLabel 3500 6200 2    50   Input ~ 0
GND
Text GLabel 3000 6500 0    50   Input ~ 0
GND
Text GLabel 3000 5800 0    50   Input ~ 0
GND
Text GLabel 3000 5000 0    50   Input ~ 0
GND
Text GLabel 3000 4900 0    50   Input ~ 0
FPGA_GPS_PWR
Text GLabel 6550 3400 2    50   Input ~ 0
FPGA_CELL_RX
Text GLabel 6550 3500 2    50   Input ~ 0
FPGA_CELL_TX
Text GLabel 7600 3350 2    50   Input ~ 0
5v
Text GLabel 7600 3450 2    50   Input ~ 0
3v3
Text GLabel 7600 3550 2    50   Input ~ 0
GND
$Comp
L Connector:Conn_01x03_Male J4
U 1 1 641A325D
P 6350 3400
F 0 "J4" H 6458 3681 50  0000 C CNN
F 1 "Cell_left" H 6458 3590 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 6350 3400 50  0001 C CNN
F 3 "~" H 6350 3400 50  0001 C CNN
	1    6350 3400
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Male J5
U 1 1 641A7B26
P 7400 3450
F 0 "J5" H 7508 3731 50  0000 C CNN
F 1 "Cell_right" H 7508 3640 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 7400 3450 50  0001 C CNN
F 3 "~" H 7400 3450 50  0001 C CNN
	1    7400 3450
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x02_Female J6
U 1 1 641B034D
P 7200 1500
F 0 "J6" H 7228 1476 50  0000 L CNN
F 1 "Camera_power_header" H 7228 1385 50  0000 L CNN
F 2 "691214110002:691214110002" H 7200 1500 50  0001 C CNN
F 3 "~" H 7200 1500 50  0001 C CNN
	1    7200 1500
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x02_Female J7
U 1 1 641B25E2
P 7200 1950
F 0 "J7" H 7228 1926 50  0000 L CNN
F 1 "Camera_data_header" H 7228 1835 50  0000 L CNN
F 2 "691214110002:691214110002" H 7200 1950 50  0001 C CNN
F 3 "~" H 7200 1950 50  0001 C CNN
	1    7200 1950
	1    0    0    -1  
$EndComp
$EndSCHEMATC
