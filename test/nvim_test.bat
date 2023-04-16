@echo off
cd %~dp0\..
bazel build ...
nvim --clean -R -u .\test\nvim_test.lua

