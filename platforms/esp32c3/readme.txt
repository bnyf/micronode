esp32

make spiffs
${MKSPIFFS}/mkspiffs -c ${MICRONODE}/framework -b 4096 -p 256 -s 0x100000 framework.bin

spiffs flash
python ${IDF_PATH}/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/cu.usbserial-0001 --baud 115200 write_flash -z 0x110000 framework.bin

can't find jerry.a
rm -rf main/src main/tmp