# =============================================================================
# Server Crypto子模块 - AES-256加密通信
# =============================================================================

set(SERVER_CRYPTO_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/server/crypto.h
)

set(SERVER_CRYPTO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/crypto.cpp
)

add_library(SmartSchedServerCrypto STATIC
    ${SERVER_CRYPTO_HEADERS}
    ${SERVER_CRYPTO_SOURCES}
)

target_include_directories(SmartSchedServerCrypto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
)

target_link_libraries(SmartSchedServerCrypto PUBLIC
    SmartSchedCommon
)

# OpenSSL链接（条件）
if(OPENSSL_FALLBACK)
    message(STATUS "Crypto: Using fallback crypto")
else()
    target_link_libraries(SmartSchedServerCrypto PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

# 安装
install(TARGETS SmartSchedServerCrypto
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT libraries
)
