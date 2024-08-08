@echo off
make
build\compiler_count.exe 64 8 16 512 128 2 2 1024 2048 500 256 8 0 1 8 4 2 8 32 "proj" 64 1024 1024 "wspp"
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 2048 8 0 1 8 4 2 8 32 "a2a" 197 1280 16 "lhd"
@REM build\compiler_count.exe 64 8 64 512 128 4 4 64 128 500 2048 8 0 1 8 4 2 8 32 "a2a" 197 1280 16 "lhd"
@REM build\compiler_count.exe 192 32 64 1536 384 1 1 2048 512 500 4096 8 0 1 8 4 2 8 32 "a2a" 197 1280 16 "lhd"
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 a2a 512 4096 1024 ph2
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 512 4096 1024 isap
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 512 4096 1024 ispp
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 512 4096 1024 wsap
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 512 4096 1024 wspp
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 1024 4096 512 isap
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 1024 4096 512 ispp
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 1024 4096 512 wsap
@REM build\compiler_count.exe 256 32 64 2048 512 1 1 64 128 500 128 8 0 0 8 4 2 8 32 proj 1024 4096 512 wspp
