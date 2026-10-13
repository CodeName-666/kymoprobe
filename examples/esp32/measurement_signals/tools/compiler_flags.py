"""Apply the restricted C++ options without passing them to C SDK sources."""
Import('env')
env.AppendUnique(CXXFLAGS=['-fno-exceptions', '-fno-rtti'])
