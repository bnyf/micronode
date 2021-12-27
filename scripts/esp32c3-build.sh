if [ ! -d "platforms/esp32c3/build" ]; then
    mkdir ${MICRONODE}/platforms/esp32c3/build
else
    echo "build folder found"
fi

cmake ${MICRONODE}/platforms/esp32c3 -B platforms/esp32c3/build

make -C ${MICRONODE}/platforms/esp32c3/build -j8
