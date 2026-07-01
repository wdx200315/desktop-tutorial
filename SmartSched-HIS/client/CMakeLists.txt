# =============================================================================
# Client模块 - 客户端应用
# 患者终端、医生工作站、排队看板、管理端
# =============================================================================

# 客户端基础库
add_subdirectory(src/base)

# 患者挂号终端
if(BUILD_CLIENT_PATIENT)
    add_subdirectory(src/patient)
endif()

# 医生工作站
if(BUILD_CLIENT_DOCTOR)
    add_subdirectory(src/doctor)
endif()

# 排队看板
if(BUILD_CLIENT_DISPLAY)
    add_subdirectory(src/display)
endif()

# 管理端
if(BUILD_CLIENT_ADMIN)
    add_subdirectory(src/admin)
endif()

message(STATUS "Client模块配置完成")
