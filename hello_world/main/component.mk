#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

# //这条用于添加，里面含有C文件的文件夹
 COMPONENT_SRCDIRS += user_station
 COMPONENT_SRCDIRS += user_http_request
 COMPONENT_SRCDIRS += user_uart
 COMPONENT_SRCDIRS += user_mqtt_tcp
 COMPONENT_SRCDIRS += utils
 COMPONENT_SRCDIRS += user_softAP
 COMPONENT_SRCDIRS += user_udp_sever
# //这条用于添加，需要编译的头文件
 COMPONENT_ADD_INCLUDEDIRS += user_station/include/
 COMPONENT_ADD_INCLUDEDIRS += user_http_request/include/
 COMPONENT_ADD_INCLUDEDIRS += user_uart/include/
 COMPONENT_ADD_INCLUDEDIRS += user_mqtt_tcp/include/
 COMPONENT_ADD_INCLUDEDIRS += utils/include/
 COMPONENT_ADD_INCLUDEDIRS += user_softAP/include/
 COMPONENT_ADD_INCLUDEDIRS += user_udp_sever/include/
 