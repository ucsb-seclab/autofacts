add_library(FactsInserter MODULE
        # List your source files here.
        include/FactsInserterManager.h include/ModuleHelper.h
        src/FactsInserter.cpp include/FactsSelector.h
        src/FactsSelector.cpp
        include/FactsType.h src/FactsInserterManager.cpp src/ModuleHelper.cpp include/ReverseCallGraph.h src/ReverseCallGraph.cpp include/ContextSensitiveFacts.h src/ContextSensitiveFacts.cpp)

include_directories(include)

target_compile_features(FactsInserter PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(FactsInserter PROPERTIES
        COMPILE_FLAGS "-g"
        )

if(APPLE)
    set_target_properties(FactsInserter PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)
