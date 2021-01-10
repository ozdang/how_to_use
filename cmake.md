# cmake 코드 작성법
cmake는 크로스 플랫폼 **빌드 시스템 생성기**(cross-platform build system generator)이며, 1999년부터 현재까지 개발되어 오래된 코드 작성법부터 최신의 코드 작성법까지 다양한 사용 방법이 공존하고 있음

실제로 github을 보면 cmake로 코드를 관리하는 각각의 프로젝트에서 여러 스타일의 코드들을 볼 수 있음

본 문서에서는 CMake에 대한 Modern 스타일의 소스코드 관리 방법을 정리함

대부분의 자료는 [CMake Documentation](https://cmake.org/cmake/help/latest/index.html)과 Meeting C++ 2018, 2019에서 발표된 [More Modern CMake](https://youtu.be/y7ndUhdQuU8)와 [Oh No! More Modern CMake](https://youtu.be/y9kSr5enrSk)의 내용을 참조함

## cmake 세대 구분
CMake 언어의 세대는 Traiditional, Moden, More Modern으로 나뉨(비공식?)

- Traiditional : 1999/2000년 초기 CMake부터 Modern CMake 전
- Modern : CMake 3.0 ~ / 2014년경 **타겟** 개념 추가
- More Modern : CMake 3.12 / 2018년경 대규모 구현 완료

## target과 build/usage-requirements
- target
    - 빌드 결과물인 실행파일(executable)과 라이브러리(library)
        ```cmake
        add_library(my_lib STATIC) # my_lib 이름을 가지는 target으로 libmy_lib.a를 생성(리눅스 환경에서의 static library 빌드 결과로 접두어 lib가 붙음)
        add_executable(main) # main 이름을 가지는 target으로 main 출력물을 생성
        ```
    - 빌드 관련 여러 속성(property)들을 포함하는 컨테이너
    - 속성으로는 타입, 소스파일, include 탐색 경로, 전처리 매크로, 링크 의존성, 컴파일러 옵션 등이 있음. [CMake 타겟에 대한 속성들](https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-targets)
        ```
        SOURCES
        LINK_LIBRARIES
        INCLUDE_DIRECTORIES
        COMPILE_DEFINITIONs
        COMPILE_FLAGS
        ...
        ```
    - 위의 속성들은 ', '를 구분자로 하는 리스트 구조임
    - 속성 필드들로 구성된 c/c++의 **구조체**라 보면 이해하기 쉬울 듯
        ```cpp
        // cpp style의 구조체 정의
        struct TARGET {
            string type; // 실행파일 또는 라이브러리
            list<string> sources; // 소스코드 파일 이름들
            list<string> link_libraries; // 링크 의존성
            list<string> include_directories; // 헤더파일 탐색 경로들
            list<string> compile_definitions; // 전처리 매크로 정의들
            list<string> compile_flags; // 지정한 컴파일 플래그들
            ...
        };
        // target 변수 선언(초기화 과정은 생략)
        TARGET my_lib;
        TARGET main;
        ```
    - 타겟에 속성을 추가하는 방법
        - `add_library` 또는 `add_executable` 함수는 TYPE 속성을 설정함
        - 속성별 `target_*` 함수 사용
            - `target_sources`, `target_link_libraries`, `target_include_directories` 등이 있음
                ```cmake
                # target에 소스를 추가
                target_sources(main PRIVATE main.cpp)
                # target에 링크 의존성(link dependencies)를 추가. "main은 my_lib 타겟(또는 라이브러리)가 필요하다"라는 의미
                target_link_libraries(main PRIVATE my_lib)
                ```
        - `set_target_properties(<target> PROPERTIES <property> <value>)` 함수 사용
            - `target_*` 함수들은 이 함수의 레퍼 형태
                ```cmake
                # target에 소스를 추가
                set_target_properties(main PROPERTIES SOURCES main.cpp)
                # target에 링크 의존성(link dependencies)를 추가. "main은 my_lib 타겟(또는 라이브러리)가 필요하다"라는 의미
                set_target_properties(main PROPERTIES LINK_LIBRARIES my_lib)        
                ```

- target 간 의존성
    - `target_link_libraries` 함수(또는 `set_target_properties`)를 사용하여 target 간 의존성을 생성할 수 있음
    - CMake는 target에 대한 속성을 의존성에 따라 전파될 수 있게 설계됨
    - CMake 코드 작성자는 속성이 다른 타겟으로 전파(propagation) 될지를 선택할 수 있음
    - `target_link_libraries(A PRIVATE B)`는 "**A 타겟은 B 타겟에 의존한다**"를 의미하며, **B 타겟에서 전파되도록 설정한 속성들**이 A 타겟으로 전파되어 A 타겟의 관련 속성으로 추가됨 
    
- target 속성 중 전파되는 속성들
    - 앞서 target 속성으로 `SOURCES`, `LINK_LIBRARIES` 등이 있음을 설명함
    - 추가적으로, target 속성 중 소스파일, include 탐색 경로, 전처리 매크로, 링크 의존성, 컴파일러 옵션 등에 대해 전파되는 속성들이 있음
    - 전파되는 속성들 앞에는 `INTERFACE_` 접두어가 붙음. [CMake 타겟에 대한 속성들](https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-targets)
    - 다음 위쪽은 전파되지 않는 속성들이고, `INTERFACE_`가 붙은 아래쪽은 전파되는 속성들임
        ```
        SOURCES
        LINK_LIBRARIES
        INCLUDE_DIRECTORIES
        COMPILE_DEFINITIONs
        COMPILE_FLAGS
        ...
        INTERFACE_SOURCES
        INTERFACE_LINK_LIBRARIES
        INTERFACE_INCLUDE_DIRECTORIES
        INTERFACE_COMPILE_DEFINITIONS
        INTERFACE_COMPILE_OPTIONS
        ...
        ```
    - 타겟의 `INTERFACE_*` 속성은 이 타겟을 의존하는 모든 타겟에 전파됨 
    - 예를 들어, `target_link_libraries(A PRIVATE B)`에서 B 타겟의 INTERFACE_INCLUDE_DIRECTORIES 속성은 A 타겟의 INCLUDE_DIRECTORIES 속성에 추가됨

- target 속성 중 전파되는 속성 설정
    - `set_target_properties`로 설정 가능, 그러나 `target_*` 커맨드를 활용하는 더 간편한 방법이 있음
    - 앞선 예제 코드들에서 `target_*` 커맨드에서 `PRIVATE` 키워드를 사용함
    - `PRIVATE` 키워드는 "해당 속성을 전파하지 않는다"를 의미함
    - 속성을 의존성 관계에 따라 전파하려면 `INTERFACE` 키워드를 사용해야함
    - 예, 라이브러리 코드들을 특정 디렉터리에 넣어서 관리할 때, 라이브러리 타겟에 대한 include 디렉터리 경로를 `INTERFACE`로 설정하여, 이 타겟에 의존하는 타겟들이 자동으로 include 디렉터리를 전파받을 수 있게 할 수 있음   
        ```cmake
        # root/subdirectory/CMakeLists.txt
        add_library(sub_lib)
        target_sources(sub_lib PRIVATE sub_lib.h sub_lib.cpp)
        # CMAKE_CURRENT_SOURCE_DIR 변수는 현재 소스코드가 있는 디렉터리 경로를 의미하며, 여기선 "root/subdirectory"로 변환됨
        target_include_directories(sub_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
        ```
        ```cmake
        # root/CMakeLists.txt
        add_subdirectory(subdirectory) # root/subdirectory/CMakeLists.txt를 읽는다.
        add_executable(main main.cpp)
        # main 타겟은 sub_lib 타겟에 의존함으로써 INCLUDE_DIRECTORY를 전파받아 "root/subdirectory"를 include 탐색 경로로 사용한다.
        target_link_libraries(main PRIVATE sub_lib)
        ```
        ```cpp
        // root/main.cpp
        // main.cpp는 "./subdirectory/sublib.h"로 안해도 됨
        #include "sublib.h"
        ...
        ```

- build-requirements
    - build-requirements란 target을 **빌드(build)**하기 위해 필요한 것들을 의미함
    - 공식 문서에서는 build-specification으로 표현함
    - `target_*` 함수에 `PRIVATE` 키워드를 지정하면 해당 속성은 build-requirements가 됨
       ```cmake
        # build-requirements 지정 예시
        target_sources( <target> PRIVATE <source-file>... )
        target_link_libraries( <target> PRIVATE <dependency>... )
        target_include_directories( <target> PRIVATE <include-search-dir>... )
        ```
    - 소스파일, include 탐색 경로, 전처리 매크로, 링크 의존성, 컴파일러 옵션 등이 build-requirements에 해당함

- usage-requirements
    - 다른 target에서 이 target을 **사용(use)**하기 위해 필요한 것들
    - target 속성 중 전파되는 속성들이 이에 해당함
        - 접두어 `INTERFACE_'가 붙은 속성들
        - 소스 파일, include 탐색 경로, 전처리 매크로, 링크 의존성, 컴파일러 옵션 등
    - 이 라이브러리를 **사용**하려면 어떤 전처리 매크로를 넣어줘야 하는지, 어떤 디렉터리를 include 해야하는지, 어떤 컴파일 옵션 등을 지정해야 하는지를 빌드 시스템 측면에서 기술한 것
    - `target_*` 함수에 `INTERFACE`로 지정한 것들
        ```cmake
        # usage-requirements 지정
        target_sources( <target> INTERFACE <source-file>... )
        target_link_libraries( <target> INTERFACE <dependency>... )
        target_include_directories( <target> INTERFACE <include-search-dir>... )
        ```

- `target_*` 커맨드의 `PRIVATE`, `INTERFACE`, `PUBLIC` 키워드
    - PRIVATE, INETERFACE는 위에서 설명했지만 한번 더 간단하게 설명하고, PUBLIC 키워드를 소개함
    - PRIVATE
        - 지정한 타겟에서만 사용됨(전파되지 않음)
        - 지정한 타겟을 **빌드**하기 위해 사용됨(build-requirements) 
    - INTERFACE
        - 지정한 타겟의 속성을 **지정한 타겟에 의존하는 타겟**에 전파(propagation)
        - 지정한 타겟을 **사용**하기 위해 필요한 속성으로 지정(usage-requirements)
        - 빌드하는데 사용되지 않음
    - PUBLIC
        - 해당 속성을 build-requirements와 usage-requirements으로 둘다 설정
        - 이 키워드를 사용하면 PRIVATE으로도 설정하고 INTERFACE로도 설정
        - 예를 들어, target_include_directories에 PUBLIC 키워드를 사용하면 `INCLUDE_DIRECTORIES` 속성과 `INTERFACE_INCLUDE_DIRECTORIES` 속성을 같은 값으로 설정함
        - 즉 빌드하는데도 사용하고, 사용하기 위해 필요하므로 전파되도록 함

- transitive usage-requirements
    - Transitive는 국어로 [추이적](https://ko.dict.naver.com/#/entry/koko/f36f417806e7467c9d9a12c741e318e2)이라함
        > 추이적 : x가 y와 관계가 있고 y가 z와 관계가 있으면, x가 z와 관계가 있는. 또는 그런 것.
    - A 타겟이 B 타겟에 의존하고, B 타겟이 C 타겟에 의존하면 C 타겟에 대한 속성이 B 타겟을 거쳐 A 타겟까지 전파되는 것을 말함
    - `target_link_libraries` 함수에서 `INTERFACE` 또는 `PUBLIC` 키워드를 지정하여 transitive usage-requirements 가능
    - 아래 코드를 예로 들어, lib_C -> lib_B -> main_A 순으로 usage-requirements 전파가 일어남
        ```cmake
        add_library(lib_C STATIC)
        target_include_directories(lib_C "some/header/directory")

        add_library(lib_B STATIC)
        target_link_libraries(lib_B PUBLIC lib_C) # lib_B 타겟은 "some/header/directory"를 자동으로 include 디렉터리로 설정

        add_executable(main_A)
        target_link_libraries(main PRIVATE lib_B) # main 타겟 또한 "some/header/directory"를 자동으로 include 디렉터리로 설정
        ```
    - [transitive usage-requirements](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#transitive-usage-requirements) 참조

- 헤더 파일의 usage-requirements 전파
    - 소스 파일 중 헤더 파일(.h)은 일반적으로 usage-requirements에 사용되지 않음. 그러나 Visual Studio 등의 IDE에서 프로젝트 구조를 읽는데 사용될 수 있음
        ```cmake
        # IDE를 고려한 소스코드 설정
        add_library(my_lib STATIC)
        target_sources(my_lib
            PUBLIC my_lib.h
            PRIVATE my_lib.cpp 
        )
        # cpp는 전파되므로 PUBLIC 또는 INTERFACE로 지정 시 의도치 않은 동작 가능
        ```

## OBJECT 라이브러리
- 오브젝트 라이브러리
    - `add_library` 함수에 `SHARED` 또는 `STATIC` 대신 `OBJECT`를 사용하여 오브젝트 라이브러리 타겟을 정의
    - [object-libraries](https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries)를 보면 Normal Libraries와 오브젝트 라이브러리가 구분되어 있음
    - 오브젝트 라이브러리를 `SHARED` 또는 `STATIC` 라이브러리로 이해하면 힘듬. 특수한 형태의 라이브러리임.
    - 오브젝트 라이브러리는 결과물이 오브젝트 파일(`*.o`)임.
    - 오브젝트 라이브러리 또한 `target_link_libraries`를 사용하여 타겟간 의존성 관계를 설정할 수 있음
    - usage-requirements는 오브젝트 라이브러리를 포함한 의존성 관계에서 동일하게 전파됨
    - 문제는 오브젝트 파일(`*.o`)의 전파임
    - 다음과 같이 1번의 전파만 이뤄지는 상황에서는 문제 없음
        ```cmake
        add_library(objLib OBJECT)

        add_executable(main_A)
        target_link_libraries(main PRIVATE objLib) # main.o와 objLib.o를 링크하여 main 실행파일 생성
        ```
    - 문제는 오브젝트 라이브러리가 포함된 transitive usage-requirements 상황에서 발생함
    - 규칙1) 오브젝트 라이브러리 타겟간 의존성에서 오브젝트 파일(`*.o`)은 전파되지 않음
        ```cmake
        add_library( obj OBJECT )
        ...
        add_library( obj2 OBJECT )
        target_sources( obj2 PRIVATE src.cpp )
        target_link_libraries( obj2 PRIVATE obj ) # PRIVATE, PUBLIC, INTERFACE 여부와 관계 없이 오브젝트 파일(`*.o`)은 전파되지 않음
        
        add_executable( exe )
        target_sources( exe PRIVATE main.cpp )
        target_link_libraries( exe PRIVATE obj2 )
        ```
    - 규칙2) 오브젝트 라이브러리가 아닌 타겟에 대한 의존성은 build-requirements만 적용됨
        ```cmake
        add_library( obj OBJECT )
        ...
        add_library( lib SHARED )
        target_sources( lib PRIVATE src.cpp )
        target_link_libraries( lib PRIVATE obj ) # lib.o와 obj.o를 링킹
        # INTERFACE 키워드를 사용하면 오브젝트 파일(`*.o`)이 exe 타겟으로 전파되지 않음(의도한 빌드 과정 실패)
        # PUBLIC은 PRIVATE과 동일

        add_executable( exe )
        target_sources( exe PRIVATE main.cpp )
        target_link_libraries( exe PRIVATE lib )
        ```
    - 규칙3) 오브젝트 라이브러리 타겟에 의존하는 오브젝트 라이브러리가 아닌 타겟에 대한 링크 의존성 전파(`INTERFACE_LINK_LIBRARIES` 속성)는 usage-requirements만 적용됨
        ```cmake
        add_library( obj OBJECT )
        ...
        add_library( lib SHARED )
        target_sources( lib PRIVATE src.cpp )
        target_link_libraries( obj PRIVATE lib ) # obj는 lib와 링크되지 않고, exe 타겟으로 전파되지 않음
        # INTERFACE 키워드는 lib를 exe로 전파함(exe가 obj를 거쳐 lib를 링크)
        # PUBLIC은 INTERFACE랑 동일함

        add_executable( exe )
        target_sources( exe PRIVATE main.cpp )
        target_link_libraries( exe PRIVATE obj )
        ```
    - [Oh No! More Modern CMake](https://youtu.be/y9kSr5enrSk)을 보면 자세한 이해 가능

- 오브젝트 파일 전파 방법
    - 오브젝트 라이브러리는 generator-expressions인 `$<TARGET_OBJECTS:objlib>`으로 오브젝트 파일(`*.o`)을 소스코드와 같이 지정 가능
    - `objlib`는 오브젝트 라이브러리 타겟 이름이며, `$<TARGET_OBJECTS:objlib>`은 빌드 시스템 생성 시간(Generation Time)에 `objlib.o`로 평가됨
        ```cmake
        # 공식 문서에서의 $<TARGET_OBJECTS:objlib> 사용 방법 안내
        add_library(... $<TARGET_OBJECTS:objlib> ...)
        add_executable(... $<TARGET_OBJECTS:objlib> ...)
        ```
    - 공식 문서에서의 안내는 컨테이너에서 필요 속성들을 관리하는 개념에 위배됨
    - 오브젝트 라이브러리의 자기 자신의 오브젝트 파일을 소스코드에 포함함으로써 컨테이너 및 전파 개념 사용 가능하며, 규칙 1도 극복하여 transitive usage-requirements 가능
        ```cmake
        add_library(a OBJECT)
        target_sources(a
            PRIVATE a.cpp
            INTERFACE $<TARGET_OBJECTS:a> # obj 타겟을 의존하는 모든 타겟의 소스코드(SOURCE 속성)에 obj.o를 전파하게 한다. 
        )

        add_library(b OBJECT)
        target_sources(b
            PRIVATE b.cpp
            INTERFACE $<TARGET_OBJECTS:b>
        )

        add_executable(main main.cpp)
        target_link_libraries(main PRIVATE a b)
        ```
    - [issues/18682](https://gitlab.kitware.com/cmake/cmake/-/issues/18682#note_875458) 참조

## 세대별 소스코드 스타일
### 소스코드 지정
- traditional
    ```cmake
    set(BASIC_SOURCES
        "src/BasicMath.cpp"
        "src/HeavyMath.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/Math.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/MathAPI.h" 
    )
    add_library( basicmath_ObjLib OBJECT ${BASIC_SOURCES})
    ```
- modern
    ```cmake
    # add_library에 반드시 소스파일 하나를 지정해야 했음
    add_library(basicmath_ObjLib OBJECT "src/dummy.cpp")
    target_sources(basicmath_ObjLib
        PRIVATE
            "src/BasicMath.cpp"
            "src/HeavyMath.cpp"
        PUBLIC 
            "${CMAKE_CURRENT_SOURCE_DIR}/include/Math.h"
        INTERFACE 
            "${CMAKE_CURRENT_SOURCE_DIR}/include/MathAPI.h"
    )
    ```
- more modern
    ```cmake
    # 3.11 버전부터 소스파일 지정 불필요
    add_library(basicmath_ObjLib OBJECT)
    target_sources(basicmath_ObjLib
        PRIVATE
            "src/BasicMath.cpp"
            "src/HeavyMath.cpp"
        PUBLIC 
            "${CMAKE_CURRENT_SOURCE_DIR}/include/Math.h"
        INTERFACE 
            "${CMAKE_CURRENT_SOURCE_DIR}/include/MathAPI.h"
    )
    ```
- more modern (3.13 버전 이후)
    ```cmake
    # 3.13 버전부터는 소스코드 경로가 상대 경로일 때 자동으로 ${CMAKE_CURRENT_SOURCE_DIR} 접두어 추가
    add_library(basicmath_ObjLib OBJECT)
    target_sources(basicmath_ObjLib
        PRIVATE
            "src/BasicMath.cpp
            "src/HeavyMath.cpp"
        PUBLIC 
            "include/Math.h"
        INTERFACE 
            "include/MathAPI.h"
    )

### build/usage requirements 추가
- traditional
    ```cmake
    add_library( basicmath_ObjLib OBJECT ${BASIC_SOURCES} )

    # include_directories는 CMakeLists.txt에 있는 모든 타겟에 적용되며, build-requirements만 적용
    include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include" )
    # 컴파일 플래그 설정은 전역으로 적용됨
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" CACHE STRING "C++ compile-flags" FORCE )
    ```

- modern
    ```cmake
    add_library( basicmath_ObjLib OBJECT "src/dummy.cpp" )

    # 타겟에만 적용되는 include 경로
    target_include_directories( basicmath_ObjLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
    # 타겟에만 적용되는 컴파일 플래그
    target_compile_features( basicmath_ObjLib PUBLIC cxx_constexpr )
    ```

- more modern
    - 변경 사항 없음

## 기타
- `target_*` 함수 목록
    ```
    target_include_directories( <target> PRIVATE <include-search-dir>... )
    target_compile_definitions( <target> PRIVATE <macro-definitions>... )
    target_compile_options( <target> PRIVATE <compiler-option>... )
    target_compile_features( <target> PRIVATE <feature>... )
    target_sources( <target> PRIVATE <source-file>... )
    target_precompile_headers( <target> PRIVATE <header-file>... )
    target_link_libraries( <target> PRIVATE <dependency>... )
    target_link_options( <target> PRIVATE <linker-option>... )
    target_link_directories( <target> PRIVATE <linker-search-dir>... )
    ```

- target_link_libraries에서 외부 라이브러리와 내부 타겟 구분을 위한 코드 스타일
    - 프로젝트 내부 라이브러리 : `target` 이름 그대로 사용
    - 외부 라이브러리 : gcc의 링크 옵션과 같이 `-l[외부 라이브러리]`로 사용. 예) `-ljpeg` `-lpng` 

- Cmake는 최소 3.16 버전부터 사용하는게 좋은 듯 싶다. 