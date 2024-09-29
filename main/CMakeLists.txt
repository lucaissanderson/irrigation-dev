file(GLOB_RECURSE srcs "irrigation-proj.cpp" "src/*.cpp")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "./include"
                    REQUIRES nvs_flash driver)
