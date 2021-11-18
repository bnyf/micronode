if [ ! -d "platforms/esp32/build" ]; then
    mkdir ${MICRONODE}/platforms/esp32/build
else
    echo "build folder found"
fi

cmake ${MICRONODE}/platforms/esp32 -B platforms/esp32/build

make -C ${MICRONODE}/platforms/esp32/build -j8
