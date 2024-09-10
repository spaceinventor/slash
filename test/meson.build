add_languages('cpp')

gtest_dep = dependency('gtest', main : true)
gtest_main_dep = dependency('gtest_main')

slash_completion_tests = executable(
    'slash_completion_tests',
    sources: [
        'completion_tests.cpp'
    ],
    dependencies: [slash_dep, gtest_dep, gtest_main_dep],
)

test('slash_completion_tests', slash_completion_tests)