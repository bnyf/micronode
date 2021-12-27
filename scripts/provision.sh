cdir=`cd $(dirname $0); pwd`;
export MICRONODE=$cdir/..

if [ $# -eq 0 ]
then
  echo "USAGE:"
  echo "  source script/build.sh {BOARD}"
  return 1
fi

BOARDNAME=$1
source ${MICRONODE}/deps/esp-idf/export.sh
PATH=$PATH:{MICRONODE}/scripts/

if [ "$BOARDNAME" = "esp32" ]; then
    echo "===== ESP32"
    export MKSPIFFS=${MICRONODE}/platforms/esp32/mkspiffs
fi

if [ "$BOARDNAME" = "esp32c3" ]; then
    echo "===== ESP32C3"
    export MKSPIFFS=${MICRONODE}/platforms/esp32c3/mkspiffs
fi
