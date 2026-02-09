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

option(UT_BUILD_SHARED_LIBS "Build SpeechProcessor as shared library" ${BUILD_SHARED_LIBS_DEFAULT})

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
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

################## kissfft
if(MSVC)
ExternalProject_Add(
  kissfft_project
  GIT_REPOSITORY 		 https://github.com/sheldonrobinson/kissfft.git
  GIT_TAG        		 origin/master
  GIT_SHALLOW 		 	 TRUE
  GIT_SUBMODULES_RECURSE TRUE
  CMAKE_ARGS		-DKISSFFT_OPENMP=ON
					-DKISSFFT_PKGCONFIG=ON
					-DKISSFFT_STATICL=OFF
					-DKISSFFT_TEST=OFF
					-DKISSFFT_TOOLS=OFF
					-DKISSFFT_USE_ALLOCA=OFF
					-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  
  BUILD_COMMAND				${cmd_wrapper} msbuild kissfft.sln /p:Configuration=$<CONFIG>
  INSTALL_COMMAND			${cmd_wrapper} msbuild INSTALL.vcxproj /p:Configuration=$<CONFIG>
  STEP_TARGETS  configure build install
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
else()
ExternalProject_Add(
  kissfft_project
  GIT_REPOSITORY 		 https://github.com/sheldonrobinson/kissfft.git
  GIT_TAG        		 origin/master
  GIT_SHALLOW 		 	 TRUE
  GIT_SUBMODULES_RECURSE TRUE
  CMAKE_GENERATOR		 Ninja
  CMAKE_ARGS		-DKISSFFT_OPENMP=ON
					-DKISSFFT_PKGCONFIG=ON
					-DKISSFFT_STATICL=OFF
					-DKISSFFT_TEST=OFF
					-DKISSFFT_TOOLS=OFF
					-DKISSFFT_USE_ALLOCA=OFF
					-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  
  BUILD_COMMAND		ninja build
  INSTALL_COMMAND	ninja install
  STEP_TARGETS  configure build install
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
endif()

ExternalProject_Get_property(kissfft_project INSTALL_DIR)
set(KISSFFT_ROOT_DIR ${INSTALL_DIR} CACHE PATH "kissfft dir" FORCE)

set(KISSFFT_BINPATH "${KISSFFT_ROOT_DIR}/bin/kissfft-float-openmp.dll")
set(KISSFFT_IMPORTLIBPATH "${KISSFFT_ROOT_DIR}/lib/kissfft-float-openmp.lib")

set(KISSFFT_INCLUDE_DIR "${KISSFFT_ROOT_DIR}/include" CACHE PATH "kissfft include dir" FORCE)

if(NOT EXISTS ${KISSFFT_INCLUDE_DIR})
 file(MAKE_DIRECTORY ${KISSFFT_INCLUDE_DIR})
endif()

if(NOT TARGET kissfft_shared)
	add_library(kissfft_shared SHARED IMPORTED GLOBAL)
	add_dependencies(kissfft_shared kissfft_project-install)
	set_target_properties(# Specifies the target library.
						   kissfft_shared
						   PROPERTIES 
						   IMPORTED_LOCATION	${KISSFFT_BINPATH}
						   IMPORTED_IMPLIB		${KISSFFT_IMPORTLIBPATH}
						 )
	if (MSVC)
		set_target_properties(kissfft_shared PROPERTIES 
					PDB_OUTPUT_DIRECTORY $<INSTALL_INTERFACE:${KISSFFT_ROOT_DIR}/lib>
					)
	endif()
endif()

if(NOT TARGET kissfft::kissfft)
	add_library(kissfft::kissfft INTERFACE IMPORTED)
	add_dependencies(kissfft::kissfft kissfft_project-install)
    set_target_properties(kissfft::kissfft PROPERTIES
						  INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${KISSFFT_INCLUDE_DIR}>
						  INTERFACE_POSITION_INDEPENDENT_CODE ON
						 )
endif()

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
									 --cmake_extra_defines CMAKE_INSTALL_PREFIX=<INSTALL_DIR>
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

####################### soundtouch
if(MSVC)
ExternalProject_Add(
  soundtouch_project
  GIT_REPOSITORY 		 https://github.com/sheldonrobinson/soundtouch.git
  GIT_TAG        		 origin/master
  GIT_SHALLOW 			 TRUE
  GIT_SUBMODULES_RECURSE TRUE

  CMAKE_ARGS		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  CMAKE_CACHE_ARGS	-DOPENMP:BOOL=ON 
					-DSOUNDSTRETCH:BOOL=OFF
					-DSOUNDTOUCH_DLL:BOOL=ON
					-DBUILD_SHARED_LIBS:BOOL=ON
  BUILD_COMMAND				${cmd_wrapper} msbuild SoundTouch.sln /p:Configuration=$<CONFIG>
  INSTALL_COMMAND			${cmd_wrapper} msbuild INSTALL.vcxproj /p:Configuration=$<CONFIG>
  STEP_TARGETS build install
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
else()
ExternalProject_Add(
  soundtouch_project
  GIT_REPOSITORY 		 https://github.com/sheldonrobinson/soundtouch.git
  GIT_TAG        		 origin/master
  GIT_SHALLOW 			 TRUE
  GIT_SUBMODULES_RECURSE TRUE
  CMAKE_GENERATOR		 Ninja
  CMAKE_ARGS		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  CMAKE_CACHE_ARGS	-DOPENMP:BOOL=ON 
					-DSOUNDSTRETCH:BOOL=OFF
					-DSOUNDTOUCH_DLL:BOOL=ON
					-DBUILD_SHARED_LIBS:BOOL=ON
  BUILD_COMMAND				ninja build
  INSTALL_COMMAND			ninja install
  STEP_TARGETS build install
  USES_TERMINAL_BUILD 		TRUE
  USES_TERMINAL_INSTALL 	TRUE
)
endif()

ExternalProject_Get_Property(soundtouch_project INSTALL_DIR)
set(SOUNDTOUCH_ROOT_DIR ${INSTALL_DIR} CACHE PATH "soundtouch dir" FORCE)

set(SOUNDTOUCH_INCLUDE_DIR "${SOUNDTOUCH_ROOT_DIR}/include" CACHE PATH "soundtouch include dir" FORCE)

set(SOUNDTOUCH_BINPATH "${SOUNDTOUCH_ROOT_DIR}/bin/SoundTouch.dll")
set(SOUNDTOUCH_IMPORTLIBPATH "${SOUNDTOUCH_ROOT_DIR}/lib/SoundTouch.lib")

set(SOUNDTOUCHDLL_BINPATH "${SOUNDTOUCH_ROOT_DIR}/bin/SoundTouchDLL.dll")
set(SOUNDTOUCHDLL_IMPORTLIBPATH "${SOUNDTOUCH_ROOT_DIR}/lib/SoundTouchDLL.lib")

if(NOT EXISTS ${SOUNDTOUCH_INCLUDE_DIR})
 file(MAKE_DIRECTORY ${SOUNDTOUCH_INCLUDE_DIR})
endif()

if(NOT TARGET SoundTouch)
	add_library(SoundTouch SHARED IMPORTED GLOBAL)
	add_dependencies(SoundTouch soundtouch_project-install)
    set_target_properties(SoundTouch
						   PROPERTIES 
						   IMPORTED_LOCATION	${SOUNDTOUCH_BINPATH}
						   IMPORTED_IMPLIB		${SOUNDTOUCH_IMPORTLIBPATH}
						   INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${SOUNDTOUCH_INCLUDE_DIR}>
						 )
endif()

if(NOT TARGET SoundTouchDLL)
	add_library(SoundTouchDLL SHARED IMPORTED GLOBAL)
	add_dependencies(SoundTouchDLL soundtouch_project-install)
    set_target_properties(SoundTouchDLL
						   PROPERTIES 
						   IMPORTED_LOCATION	${SOUNDTOUCHDLL_BINPATH}
						   IMPORTED_IMPLIB		${SOUNDTOUCHDLL_IMPORTLIBPATH}
						   INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${SOUNDTOUCH_INCLUDE_DIR}>
						 )
endif()
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
  CMAKE_ARGS	   -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_CXX_FLAGS="-I${ORT_HDRS_DIR}" -DCMAKE_C_FLAGS="-I${ORT_HDRS_DIR}"
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

add_dependencies(unnutts kissfft_project-install soundtouch_project-install ort_project-install piper_project-install)

target_link_directories(unnutts PUBLIC piper onnxruntime kissfft_shared SoundTouch SoundTouchDLL)


set_target_properties(unnutts PROPERTIES
  BUILD_WITH_INSTALL_RPATH TRUE
  LINKER_LANGUAGE CXX
  OUTPUT_NAME "unnutts"
  POSITION_INDEPENDENT_CODE True
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
