# SP Runtime Library
find_package(Threads REQUIRED)

add_library(sp_runtime
    src/Runtime.cpp
    src/threading/Concurrency.cpp
    src/threading/Channel.cpp
    src/threading/ThreadPool.cpp
    src/net/Net.cpp
    src/net/EventLoop.cpp
    src/net/NetBridge.cpp
    src/net/WebSocket.cpp
    src/net/Sse.cpp
    src/net/Redis.cpp
    src/crypto/Crypto.cpp
    src/crypto/Argon2.cpp
    src/crypto/Bcrypt.cpp
    src/crypto/Jwt.cpp
    src/crypto/Session.cpp
    src/crypto/SecurityBridge.cpp
    src/json/Json.cpp
    src/http/Http.cpp
    src/db/Sqlite.cpp
    src/db/Postgres.cpp
    src/db/DbPool.cpp
    src/db/DbBridge.cpp
    third_party/sqlite3/sqlite3.c
)

target_include_directories(sp_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite3>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(sp_runtime PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_WAL=1
    SQLITE_ENABLE_JSON1=1
)

target_compile_features(sp_runtime PUBLIC cxx_std_20)
target_link_libraries(sp_runtime PUBLIC Threads::Threads)
if(WIN32)
    target_link_libraries(sp_runtime PUBLIC ws2_32)
endif()


