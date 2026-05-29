cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/c++)

project(test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# find_package(OpenSSL REQUIRED)

# include(ProjectBoostCertify)
# find_package(BoostCertify)

# include(ProjectCryptoPP)
# find_package(CryptoPP)

# include(ProjectEthash)
# find_package(Ethash)

# include(ProjectScrypt)
# find_package(Scrypt)

# include(ProjectSecp256k1)
# find_package(Secp256k1)

# include(ProjectToolbox)
# find_package(Toolbox)

# include(ProjectBip3x)
# find_package(Bip3x)

find_package(web3cpp REQUIRED)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

add_executable(${PROJECT_NAME}
    ${SRC_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    web3cpp::web3cpp
)
