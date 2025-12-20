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
        # Build tag name from default_version (e.g., "4.0.0.0" -> "v4.0.0.0")
        set(BASE_TAG "v${default_version}")
        
        # Check if base tag exists
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --verify ${BASE_TAG}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE tag_exists
            OUTPUT_QUIET
            ERROR_QUIET)
        
        if(NOT ${tag_exists})
            # Count commits since base tag
            execute_process(COMMAND ${GIT_EXECUTABLE} rev-list --count ${BASE_TAG}..HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE count_status
                OUTPUT_VARIABLE COMMIT_COUNT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            
            if(NOT ${count_status})
                string(STRIP ${COMMIT_COUNT} COMMIT_COUNT)
                
                # Split default_version to get major.minor.patch (e.g., "4.0.0.0" -> "4.0.0")
                string(REPLACE "." ";" VERSION_PARTS ${default_version})
                list(LENGTH VERSION_PARTS VERSION_PARTS_LEN)
                if(${VERSION_PARTS_LEN} GREATER 2)
                    list(GET VERSION_PARTS 0 V_MAJOR)
                    list(GET VERSION_PARTS 1 V_MINOR)
                    list(GET VERSION_PARTS 2 V_PATCH)
                    # Build version with commit count as TWEAK (e.g., "4.0.0.368")
                    set(${version_info} "${V_MAJOR}.${V_MINOR}.${V_PATCH}.${COMMIT_COUNT}" PARENT_SCOPE)
                else()
                    # Fallback if version format unexpected
                    set(${version_info} "${default_version}.${COMMIT_COUNT}" PARENT_SCOPE)
                endif()
                
                # Get current short hash
                execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    RESULT_VARIABLE hash_status
                    OUTPUT_VARIABLE GIT_HASH
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
                
                if(NOT ${hash_status})
                    string(STRIP ${GIT_HASH} GIT_HASH)
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
