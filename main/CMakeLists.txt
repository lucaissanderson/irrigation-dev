file(GLOB_RECURSE srcs "irrigation-proj.cpp" "src/*.cpp" "src/*.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "./include"
                    REQUIRES esp_wifi nvs_flash driver)
