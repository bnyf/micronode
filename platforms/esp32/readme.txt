esp32 

1. source scripts/provision.sh esp32
2. source scripts/esp32-build.sh
3. source scripts/esp32-flash.sh

··· notes ···
1. esp-idf-4.1 环境安装：下载并配置环境到 esp32 目录下，文件夹命名为 esp-idf。
2. mkspiffs 下载（ https://github.com/igrr/mkspiffs/releases/tag/0.2.3 ）：将适合自己系统的 mkspiffs 放在 mkspiffs 文件夹中，文件命名为 mkspiffs

3. 如果报错类似于 "can't find jerry.a"
执行 source scripts/esp32-clean.sh
