# Builds SpeechProcessor
cmake_minimum_required(VERSION 3.26)
project(unnutts LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(FetchContent)
include(ExternalProject)

find_package(OpenMP REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)

if (MINGW)
	set(BUILD_SHARED_LIBS_DEFAULT OFF)
else()
	set(BUILD_SHARED_LIBS_DEFAULT ON)
endif()

option(UT_BUILD_SHARED_LIBS "Build as shared library" ${BUILD_SHARED_LIBS_DEFAULT})

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if (MSVC)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

#################### MSVC vcvars
FetchContent_Declare(
  ut-find-vcvars
  GIT_REPOSITORY 		 https://github.com/scikit-build/cmake-FindVcvars.git
  GIT_TAG        		 origin/master
  GIT_SHALLOW 			 TRUE
  GIT_SUBMODULES_RECURSE TRUE
  
  EXCLUDE_FROM_ALL
)

FetchContent_Populate(ut-find-vcvars)

file(REAL_PATH ${ut-find-vcvars_SOURCE_DIR} FINDVCVARS_SOURCE_DIR)

list(INSERT CMAKE_MODULE_PATH 0 ${FINDVCVARS_SOURCE_DIR})

set(cmd_wrapper)
if(MSVC)
  set(Vcvars_MSVC_ARCH 64)
  set(Vcvars_FIND_VCVARSALL TRUE)
  find_package(Vcvars REQUIRED)
  set(cmd_wrapper ${Vcvars_LAUNCHER})
endif()

if(NOT TARGET nlohmann_json)
	################## nlohmann
	FetchContent_Declare(
	  utnlohmann_json
	  GIT_REPOSITORY 		 https://github.com/nlohmann/json.git
	  GIT_TAG        		 v3.12.0
	  GIT_SHALLOW 			 TRUE
	  GIT_SUBMODULES_RECURSE TRUE
			 
	  OVERRIDE_FIND_PACKAGE
	)

	set(JSON_BuildTests OFF CACHE BOOL "Build the unit tests when BUILD_TESTING is enabled." FORCE)
	set(JSON_CI OFF CACHE BOOL "Enable CI build targets." FORCE)
	set(JSON_Diagnostics OFF CACHE BOOL  "Use extended diagnostic messages." FORCE)
	set(JSON_Diagnostic_Positions OFF CACHE BOOL "Enable diagnostic positions." FORCE)
	set(JSON_GlobalUDLs ON CACHE BOOL "Place user-defined string literals in the global namespace." FORCE)
	set(JSON_ImplicitConversions ON CACHE BOOL "Enable implicit conversions." FORCE)
	set(JSON_DisableEnumSerialization OFF CACHE BOOL "Disable default integer enum serialization." FORCE)
	set(JSON_LegacyDiscardedValueComparison OFF CACHE BOOL "Enable legacy discarded value comparison." FORCE)
	set(JSON_Install OFF CACHE BOOL "Install CMake targets during install step." FORCE)
	set(JSON_MultipleHeaders ON CACHE BOOL "Use non-amalgamated version of the library." FORCE)
	set(JSON_SystemInclude OFF CACHE BOOL "Include as system headers (skip for clang-tidy)." FORCE)

	FetchContent_MakeAvailable(utnlohmann_json)

	file(REAL_PATH ${utnlohmann_json_SOURCE_DIR} NLOHMANN_JSON_SOURCE_DIR)

	set(NLOHMANN_JSON_INCLUDE_DIR "${NLOHMANN_JSON_SOURCE_DIR}/single_include" "nholmann json include dir" FORCE)

	if(NOT TARGET nlohmann::json)
		add_library(nlohmann::json INTERFACE IMPORTED)
		set_target_properties(nlohmann::json PROPERTIES
							  INTERFACE_LINK_LIBRARIES nlohmann_json 							  
							  INTERFACE_POSITION_INDEPENDENT_CODE ON
							  )
	endif()
endif()
################## kissfft
FetchContent_Declare(
  kissfft
  GIT_REPOSITORY 		 https://github.com/sheldonrobinson/kissfft.git
  GIT_TAG        		 origin/master
  GIT_SHALLOW 		 	 TRUE
  GIT_SUBMODULES_RECURSE TRUE
  
  EXCLUDE_FROM_ALL
	
  OVERRIDE_FIND_PACKAGE
)

set(KISSFFT_OPENMP ON CACHE BOOL "Build kissfft with OpenMP support" FORCE)
set(KISSFFT_PKGCONFIG OFF CACHE BOOL "Build pkg-config files" FORCE)
set(KISSFFT_STATIC  OFF CACHE BOOL "Build kissfft as static (ON) or shared library (OFF)" FORCE)
set(KISSFFT_TEST  OFF CACHE BOOL "Build kissfft tests" FORCE)
set(KISSFFT_TOOLS  OFF CACHE BOOL "Build kissfft command-line tools" FORCE)
set(KISSFFT_USE_ALLOCA  OFF CACHE BOOL "Use alloca instead of malloc" FORCE)

FetchContent_MakeAvailable(kissfft)

file(REAL_PATH ${kissfft_SOURCE_DIR} KISFFT_SOURCE_DIR)
file(REAL_PATH ${kissfft_BINARY_DIR} KISSFFT_BINARY_DIR)

set(KISSFFT_INCUDE_DIR "${KISFFT_SOURCE_DIR}" CACHE PATH "kissfft include directory" FORCE)

####################### soundtouch
FetchContent_Declare(
  soundtouch
  GIT_REPOSITORY 		 https://github.com/sheldonrobinson/soundtouch.git
  GIT_TAG        		 origin/master
  GIT_SHALLOW 			 TRUE
  GIT_SUBMODULES_RECURSE TRUE

  EXCLUDE_FROM_ALL
	
  OVERRIDE_FIND_PACKAGE
)

set(SOUNDTOUCH_DLL ON CACHE BOOL "Build SoundTouchDLL C wrapper library" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build SoundTouchDLLas shared" FORCE)
set(NEON ON CACHE BOOL "Use ARM Neon SIMD instructions if in ARM CPU" FORCE)
set(OPENMP ON CACHE BOOL "Use parallel multicore calculation through OpenMP" FORCE)
set(SOUNDSTRETCH ON CACHE BOOL "Build soundstretch command line utility." FORCE)

FetchContent_MakeAvailable(soundtouch)


file(REAL_PATH ${soundtouch_SOURCE_DIR} SOUNDTOUCH_SOURCE_DIR)
file(REAL_PATH ${soundtouch_BINARY_DIR} SOUNDTOUCH_BINARY_DIR)

set(SOUNDTOUCH_INCUDE_DIR "${SOUNDTOUCH_SOURCE_DIR}/include" CACHE PATH "Sound include directory" FORCE)
set(SOUNDTOUCHDLL_INCUDE_DIR "${SOUNDTOUCH_SOURCE_DIR}/source/SoundTouchDLL" CACHE PATH "Sound include directory" FORCE)


set(SOUNDTOUCH_INCLUDE_DIR "${SOUNDTOUCH_SOURCE_DIR}/include"CACHE PATH "soundtouch include dir" FORCE)

######################### onnxruntime
if (MSVC)
ExternalProject_Add(
  ort_project
  GIT_REPOSITORY			https://github.com/sheldonrobinson/onnxruntime.git
  GIT_TAG					origin/main
  GIT_SHALLOW				TRUE
  GIT_SUBMODULES_RECURSE	TRUE
  BUILD_IN_SOURCE			ON
  CONFIGURE_COMMAND			""
  BUILD_COMMAND				${Python_EXECUTABLE} <SOURCE_DIR>/tools/ci_build/build.py
									 --cmake_generator "Visual Studio 17 2022"
									 --build_dir <SOURCE_DIR>
									 --config Release
									 --build_shared_lib 
									 --use_mimalloc 
									 --use_full_protobuf
									 --use_dml
									 --use_binskim_compliant_compile_flags
									 --compile_no_warning_as_error 
									 --parallel
									 --skip_submodule_sync
									 --skip_tests 
									 --skip_winml_tests 
									 --cmake_extra_defines CMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  INSTALL_COMMAND			${cmd_wrapper} msbuild <SOURCE_DIR>/Release/INSTALL.vcxproj /p:Configuration=Release
  STEP_TARGETS build install
  USES_TERMINAL_CONFIGURE	TRUE
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
else()
ExternalProject_Add(
  ort_project
  GIT_REPOSITORY			https://github.com/sheldonrobinson/onnxruntime.git
  GIT_TAG					origin/main
  GIT_SHALLOW				TRUE
  GIT_SUBMODULES_RECURSE	TRUE
  BUILD_IN_SOURCE			ON
  CONFIGURE_COMMAND			""
  BUILD_COMMAND				${Python_EXECUTABLE} <SOURCE_DIR>/tools/ci_build/build.py
									 --cmake_generator Ninja
									 --build_dir <SOURCE_DIR>
									 --config Release
									 --build_shared_lib 
									 --use_mimalloc 
									 --use_full_protobuf
									 --use_dml
									 --use_binskim_compliant_compile_flags
									 --compile_no_warning_as_error 
									 --parallel
									 --skip_submodule_sync
									 --skip_tests 
									 --skip_winml_tests 
									 --cmake_extra_defines CMAKE_CXX_FLAGS="-DORT_MANUAL_INIT" CMAKE_C_FLAGS="-DORT_MANUAL_INIT" CMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  INSTALL_COMMAND			ninja install
  STEP_TARGETS build install
  USES_TERMINAL_CONFIGURE	TRUE
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
endif()

ExternalProject_Get_property(ort_project INSTALL_DIR)
set(ONNXRUNTIME_ROOT_DIR ${INSTALL_DIR} CACHE PATH "onnxruntime root dir" FORCE)

set(ORT_HOME ${ONNXRUNTIME_ROOT_DIR})
set(ORT_LIB_DIR "${ORT_HOME}/lib")
set(ORT_BIN_DIR "${ORT_HOME}/bin")
set(ORT_INC_DIR "${ORT_HOME}/include")
set(ORT_HDRS_DIR "${ORT_HOME}/include/onnxruntime")

if(NOT EXISTS ${ORT_HDRS_DIR})
 file(MAKE_DIRECTORY ${ORT_HDRS_DIR})
endif()

set(ORT_BUILD_BINPATH "${ORT_HOME}/bin/onnxruntime.dll")
set(ORT_BUILD_PROVIDERS_BINPATH "${ORT_HOME}/bin/onnxruntime_providers_shared.dll")
set(ORT_BUILD_IMPORTLIBPATH "${ORT_HOME}/lib/onnxruntime.lib")
set(ORT_BUILD_PROVIDERS_IMPORTLIBPATH "${ORT_HOME}/lib/onnxruntime_providers_shared.lib")

if(NOT TARGET onnxruntime)
	add_library(onnxruntime SHARED IMPORTED GLOBAL)
	if (NOT USE_EXTERNAL_ONNXRUNTIME)
		add_dependencies(onnxruntime ort_project-install)
	endif()
	set_target_properties(# Specifies the target library.
						   onnxruntime
						   PROPERTIES 
						   IMPORTED_LOCATION			 ${ORT_BUILD_BINPATH}
						   IMPORTED_IMPLIB				 ${ORT_BUILD_IMPORTLIBPATH}
						   INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${ORT_INC_DIR}>
						 )
	if (MSVC)
		set_target_properties(onnxruntime PROPERTIES 
					PDB_OUTPUT_DIRECTORY $<INSTALL_INTERFACE:${ORT_LIB_DIR}>
					)
	endif()
endif()

if(NOT TARGET onnxruntime_providers_shared)
	add_library(onnxruntime_providers_shared SHARED IMPORTED GLOBAL)
	add_dependencies(onnxruntime_providers_shared ort_project-install)
	set_target_properties(# Specifies the target library.
						   onnxruntime_providers_shared
						   PROPERTIES 
						   IMPORTED_LOCATION	${ORT_BUILD_PROVIDERS_BINPATH}
						   IMPORTED_IMPLIB		${ORT_BUILD_PROVIDERS_IMPORTLIBPATH}
						   INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${ORT_INC_DIR}>
						 )
	if (MSVC)
		set_target_properties(onnxruntime_providers_shared PROPERTIES 
					PDB_OUTPUT_DIRECTORY $<INSTALL_INTERFACE:${ORT_LIB_DIR}>
					)
	endif()
endif()
if(NOT TARGET onnxruntime::onnxruntime)
	add_library(onnxruntime::onnxruntime INTERFACE IMPORTED)
	add_dependencies(onnxruntime::onnxruntime ort_project-install)
    set_target_properties(onnxruntime::onnxruntime PROPERTIES
						  INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${ORT_HDRS_DIR}>
						  INTERFACE_POSITION_INDEPENDENT_CODE ON
						 )
endif()

set(ORT_ROOT ${ORT_HOME} CACHE PATH "onnxruntime root directory" FORCE)
include_directories(${ORT_HOME}/include ${ORT_HOME}/include/onnxruntime)

####################### libpiper
if(MSVC)
ExternalProject_Add(
  piper_project
  GIT_REPOSITORY			https://github.com/sheldonrobinson/piper1-gpl.git
  GIT_TAG					origin/main
  GIT_SHALLOW				TRUE
  GIT_SUBMODULES_RECURSE	TRUE
  SOURCE_SUBDIR				libpiper

  CMAKE_ARGS	   -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_CXX_FLAGS="-I${ORT_HDRS_DIR}" -DCMAKE_C_FLAGS="-I${ORT_HDRS_DIR}"
  CMAKE_CACHE_ARGS -DONNXRUNTIME_DIR:PATH=${ONNXRUNTIME_ROOT_DIR}
  BUILD_COMMAND				${cmd_wrapper} msbuild piper.sln /p:Configuration=$<CONFIG>
  INSTALL_COMMAND			${cmd_wrapper} msbuild INSTALL.vcxproj /p:Configuration=$<CONFIG>
  STEP_TARGETS  configure build install
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
else()
ExternalProject_Add(
  piper_project
  GIT_REPOSITORY			https://github.com/sheldonrobinson/piper1-gpl.git
  GIT_TAG					origin/main
  GIT_SHALLOW				TRUE
  GIT_SUBMODULES_RECURSE	TRUE
  SOURCE_SUBDIR				libpiper
  CMAKE_GENERATOR			Ninja
  CMAKE_ARGS	   -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_CXX_FLAGS="-DORT_MANUAL_INIT -I${ORT_HDRS_DIR}" -DCMAKE_C_FLAGS="-DORT_MANUAL_INIT -I${ORT_HDRS_DIR}"
  CMAKE_CACHE_ARGS -DONNXRUNTIME_DIR:PATH=${ONNXRUNTIME_ROOT_DIR}
  BUILD_COMMAND				ninja build
  INSTALL_COMMAND			ninja install
  STEP_TARGETS  configure build install
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
endif()


ExternalProject_Add_StepDependencies(piper_project configure ort_project-install)
ExternalProject_Get_Property(piper_project INSTALL_DIR)
set(LIBPIPER_ROOT_DIR ${INSTALL_DIR} CACHE PATH "piper dir" FORCE)

add_dependencies(piper_project-configure ort_project-install )


set(LIBPIPER_INCLUDE_DIR "${LIBPIPER_ROOT_DIR}/include" CACHE PATH "piper include dir" FORCE)

set(LIBPIPER_BINPATH "${LIBPIPER_ROOT_DIR}/bin/piper.dll")
set(LIBPIPER_IMPORTLIBPATH "${LIBPIPER_ROOT_DIR}/lib/piper.lib")

if(NOT EXISTS ${LIBPIPER_INCLUDE_DIR})
 file(MAKE_DIRECTORY ${LIBPIPER_INCLUDE_DIR})
endif()

if(NOT TARGET piper)
	add_library(piper SHARED IMPORTED GLOBAL)
	add_dependencies(piper piper_project-install)
	set_target_properties(# Specifies the target library.
						   piper
						   PROPERTIES 
						   IMPORTED_LOCATION	${LIBPIPER_BINPATH}
						   IMPORTED_IMPLIB		${LIBPIPER_IMPORTLIBPATH}
						   INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${LIBPIPER_INCLUDE_DIR}>
						 )
	if (MSVC)
		set_target_properties(piper PROPERTIES 
					PDB_OUTPUT_DIRECTORY $<INSTALL_INTERFACE:${LIBPIPER_ROOT_DIR}/lib>
					)
	endif()
endif()

include_directories(${SOUNDTOUCH_INCLUDE_DIR} ${KISSFFT_INCLUDE_DIR} ${ORT_HDRS_DIR} ${ORT_INC_DIR} ${LIBPIPER_INCLUDE_DIR})


if (UT_BUILD_SHARED_LIBS)
	add_library(unnutts SHARED src/unnutts.cpp)
else()
	add_library(unnutts STATIC src/unnutts.cpp)
endif()

add_dependencies(unnutts kissfft SoundTouch SoundTouchDLL ort_project-install piper_project-install)

target_link_libraries(unnutts PUBLIC piper onnxruntime kissfft SoundTouch SoundTouchDLL)
target_include_directories(unnutts 
		PRIVATE 
			${SOUNDTOUCH_BINARY_DIR} 
			${SOUNDTOUCH_INCUDE_DIR} 
			${SOUNDTOUCHDLL_INCUDE_DIR}
			${KISSFFT_INCUDE_DIR}
			)


set_target_properties(unnutts PROPERTIES
  BUILD_WITH_INSTALL_RPATH TRUE
  LINKER_LANGUAGE CXX
  OUTPUT_NAME "unnutts"
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(unnutts PUBLIC include)
target_compile_features   (unnutts PRIVATE cxx_std_17) # don't bump

if (UT_BUILD_SHARED_LIBS)
    set_target_properties(unnutts PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(unnutts PRIVATE UNNU_TTS_BUILD)
    target_compile_definitions(unnutts PUBLIC  UNNU_TTS_SHARED)
endif()


install(TARGETS unnutts EXPORT unnutts_target
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(DIRECTORY include/ TYPE INCLUDE)
