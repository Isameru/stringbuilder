add_executable(stringbuilder.test stringbuilder.test.cpp ../include/stringbuilder.h)
target_link_libraries(stringbuilder.test PRIVATE google::test)
add_test(stringbuilder.test stringbuilder.test)
