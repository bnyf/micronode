cdir=`cd $(dirname $0); pwd`;
export MICRONODE=$cdir/..

if [ $# -eq 0 ]
then
  echo "USAGE:"
  echo "  source script/build.sh {BOARD}"
  return 1
fi

BOARDNAME=$1

if [ "$BOARDNAME" = "esp32" ]; then
    echo "===== ESP32"
    export MKSPIFFS=${MICRONODE}/platforms/esp32/mkspiffs
    source ${MICRONODE}/platforms/esp32/esp-idf/export.sh
fi

if [ "$BOARDNAME" = "esp32c3" ]; then
    echo "===== ESP32C3"
    export MKSPIFFS=${MICRONODE}/platforms/esp32/mkspiffs
    source ${MICRONODE}/platforms/esp32/esp-idf-v4.4/export.sh
fi
