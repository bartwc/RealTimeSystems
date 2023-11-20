cmake_minimum_required(VERSION 3.20.0)

set(BOARD stm32f4_disco)
set(CONF_FILE prj.conf)
find_package(Zephyr)
project(my_zephyr_app)

target_sources(app PRIVATE 
src/main.c 
src/gpio.c
src/task_main.c
src/scheduler.c
src/scheduler_impl.c
src/usb.c
)
