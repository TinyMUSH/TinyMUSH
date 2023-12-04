function(split_version version major minor patch tweak)
    string(REPLACE "." ";" VERSION_LIST ${version})
    list(LENGTH VERSION_LIST VERSION_LIST_LEN)
    if(${VERSION_LIST_LEN} GREATER 0)
        list(GET VERSION_LIST 0 V_MAJOR)
        set(${major} ${V_MAJOR} PARENT_SCOPE)
        if(${VERSION_LIST_LEN} GREATER 1)
            list(GET VERSION_LIST 1 V_MINOR)
            set(${minor} ${V_MINOR} PARENT_SCOPE)
            if(${VERSION_LIST_LEN} GREATER 2)
                list(GET VERSION_LIST 2 V_PATCH)
                set(${patch} ${V_PATCH} PARENT_SCOPE)
                if(${VERSION_LIST_LEN} GREATER 3)
                    list(GET VERSION_LIST 3 V_TWEAK)
                    set(${tweak} ${V_TWEAK} PARENT_SCOPE)
                endif()
            endif()
        endif()        
    endif()    
endfunction()

function(get_git_tag_date default_date date_info)
    set(${date_info} ${default_date} PARENT_SCOPE)

    if(NOT Git_FOUND)
        find_package(Git QUIET)
        if(NOT Git_FOUND)
            message(WARNING "Could not find Git, needed to find the version tag")
            return()
        endif()
    endif()

    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format="%ai"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE status
        OUTPUT_VARIABLE GIT_T
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ${status})
        string(REPLACE "\"" "" GIT_T ${GIT_T})
        string(STRIP ${GIT_T} GIT_T)
        set(${date_info} ${GIT_T} PARENT_SCOPE)
    endif()
endfunction()

function(get_git_tag_version default_version version_info hash_info dirty_info)
    set(${version_info} ${default_version} PARENT_SCOPE)
    set(${hash_info} "" PARENT_SCOPE)
    set(${dirty_info} 0 PARENT_SCOPE)

    if(NOT Git_FOUND)
        find_package(Git QUIET)
        if(NOT Git_FOUND)
            message(WARNING "Could not find Git, needed to find the version tag")
            return()
        endif()
    endif()

    if(GIT_EXECUTABLE)
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --match "v[0-9]*.[0-9]*.[0-9]*" --abbrev=8
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE status
            OUTPUT_VARIABLE GIT_V
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT ${status})
            string(STRIP ${GIT_V} GIT_V)
            string(REGEX REPLACE "-[0-9]+-g" "-" GIT_V ${GIT_V})
            string(REPLACE "v" "" GIT_V ${GIT_V})
            string(REPLACE "-" ";" GIT_V ${GIT_V})
            list (LENGTH GIT_V GIT_V_LEN)
            if(${GIT_V_LEN} GREATER 0)
                list(GET GIT_V 0 GIT_VERSION)
                set(${version_info} ${GIT_VERSION} PARENT_SCOPE)
                if(${GIT_V_LEN} GREATER 1)
                    list(GET GIT_V 1 GIT_HASH)
                    set(${hash_info} ${GIT_HASH} PARENT_SCOPE)
                endif()
            endif()
        endif()
        # Work out if the repository is dirty
        execute_process(COMMAND ${GIT_EXECUTABLE} update-index -q --refresh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_QUIET
            ERROR_QUIET)
        execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DIFF_INDEX
            ERROR_QUIET)
        string(COMPARE NOTEQUAL "${GIT_DIFF_INDEX}" "" GIT_DIRTY)
        if (${GIT_DIRTY})
            set(${dirty_info} 1 PARENT_SCOPE)
        else()
            set(${dirty_info} 0 PARENT_SCOPE)
        endif()
    endif()
endfunction()
