#!/bin/bash
find . \( -path "./src/*" -o -path "./include/*" \) \( -iname "*.cpp" -o -iname "*.h" \) | xargs clang-format -i
