${MKSPIFFS}/mkspiffs-0.2.3-7-gf248296-esp-idf-linux64/mkspiffs -c ${MICRONODE}/framework -b 4096 -p 256 -s 0x100000 framework.bin
sudo python ${IDF_PATH}/components/esptool_py/esptool/esptool.py --chip esp32c3 --port /dev/ttyUSB0 --baud 1152000 write_flash -z 0x210000 framework.bin
