add_library(api_author_requests
    submit_extrinsic.cpp
    pending_extrinsics.cpp
    )

target_link_libraries(api_author_requests
    Boost::boost
    )
