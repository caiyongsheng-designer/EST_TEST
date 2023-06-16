#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

# //这条用于添加，里面含有C文件的文件夹
 COMPONENT_SRCDIRS += user_station
 COMPONENT_SRCDIRS += user_http_request
 
# //这条用于添加，需要编译的头文件
 COMPONENT_ADD_INCLUDEDIRS += user_station/include/
 COMPONENT_ADD_INCLUDEDIRS += user_http_request/include/