aux_source_directory(. SRC_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
# 指定库文件搜索路径
link_directories(/usr/lib64/mysql)

#生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${MODEL_LIST} ${REDIS_LIST})

#依赖的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread hiredis)