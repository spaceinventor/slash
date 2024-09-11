add_languages('cpp')

gtest_dep = dependency('gtest', main : true)
gtest_main_dep = dependency('gtest_main')

slash_completion_tests = executable(
    'slash_completion_tests',
    sources: [
        'completion_tests.cpp'
    ],
    dependencies: [slash_dep, gtest_dep, gtest_main_dep],
    cpp_args: ['-DSLASH_SECTION="slash_tests"'],
    link_args : ['-Wl,-Map=slash_completion_tests.map']
)

test('slash_completion_tests', slash_completion_tests)